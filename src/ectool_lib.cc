#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battery.h"
#include "comm-host.h"
#include "comm-usb.h"
#include "chipset.h"
#include "compile_time_macros.h"
#include "crc.h"
#include "ec_panicinfo.h"
#include "ec_flash.h"
#include "ec_version.h"
#include "i2c.h"
#include "lightbar.h"
#include "lock/gec_lock.h"
#include "misc_util.h"
#include "panic.h"
#include "usb_pd.h"

#include "framework_oem_ec_commands.h"

#ifndef _WIN32
#include "cros_ec_dev.h"
#endif

#define USB_VID_GOOGLE 0x18d1
#define USB_PID_HAMMER 0x5022
#define GEC_LOCK_TIMEOUT_SECS 30 /* 30 secs */
#define interfaces COMM_ALL

int libectool_init();
void libectool_release();
int read_mapped_temperature(int id);
static uint8_t read_mapped_mem8(uint8_t offset);


extern "C" {
int ascii_mode = 0;
bool is_on_ac();
void pause_fan_control();
void set_fan_speed(int speed);
float get_max_temperature();
float get_max_non_battery_temperature();



// -----------------------------------------------------------------------------
// Top-level endpoint functions
// -----------------------------------------------------------------------------

bool is_on_ac() {
    if (libectool_init() < 0)
        fprintf(stderr, "Failed initializing EC connection\n");

    uint8_t flags = read_mapped_mem8(EC_MEMMAP_BATT_FLAG);
    bool ac_present = (flags & EC_BATT_FLAG_AC_PRESENT);

    libectool_release();

    return ac_present;
}

void pause_fan_control() {
    if (libectool_init() < 0)
        fprintf(stderr, "Failed initializing EC connection\n");
        
    int rv = ec_command(EC_CMD_THERMAL_AUTO_FAN_CTRL, 0, NULL, 0, NULL, 0);

    if (rv < 0)
        fprintf(stderr, "Failed to enable auto fan control\n");
    
    libectool_release();
}

void set_fan_speed(int speed) {
    if (libectool_init() < 0)
        fprintf(stderr, "Failed initializing EC connection\n");

    struct ec_params_pwm_set_fan_duty_v0 p_v0;
    int rv;

    if (speed < 0 || speed > 100) {
        fprintf(stderr, "Error: Fan speed must be between 0 and 100.\n");
        return;
    }

    p_v0.percent = speed;
    rv = ec_command(EC_CMD_PWM_SET_FAN_DUTY, 0, &p_v0, sizeof(p_v0),
            NULL, 0);
    if (rv < 0)
        fprintf(stderr, "Error: Can't set speed\n");

    libectool_release();
}

// Get the maximum temperature from all sensors
float get_max_temperature() {
    if (libectool_init() < 0)
        fprintf(stderr, "Failed initializing EC connection\n");
        
    float max_temp = -1.0f;
    int mtemp, temp;
    int id;

    for (id = 0; id < EC_MAX_TEMP_SENSOR_ENTRIES; id++) {
        mtemp = read_mapped_temperature(id);
        switch (mtemp) {
        case EC_TEMP_SENSOR_NOT_PRESENT:
            break;
        case EC_TEMP_SENSOR_ERROR:
            fprintf(stderr, "Sensor %d error\n", id);
            break;
        case EC_TEMP_SENSOR_NOT_POWERED:
            fprintf(stderr, "Sensor %d disabled\n", id);
            break;
        case EC_TEMP_SENSOR_NOT_CALIBRATED:
            fprintf(stderr, "Sensor %d not calibrated\n",
                id);
            break;
        default:
            temp = K_TO_C(mtemp + EC_TEMP_SENSOR_OFFSET);
        }

        if (temp > max_temp) {
            max_temp = temp;
        }
    }
    libectool_release();
    return max_temp;
}

float get_max_non_battery_temperature()
{
    if (libectool_init() < 0)
        fprintf(stderr, "Failed initializing EC connection\n");
    
	struct ec_params_temp_sensor_get_info p;
	struct ec_response_temp_sensor_get_info r;
	int rv;
    float max_temp = -1.0f;
    int mtemp, temp;
    int id;

    for (p.id = 0; p.id < EC_MAX_TEMP_SENSOR_ENTRIES; p.id++) {
        if (read_mapped_temperature(p.id) == EC_TEMP_SENSOR_NOT_PRESENT)
            continue;
        rv = ec_command(EC_CMD_TEMP_SENSOR_GET_INFO, 0, &p,
                sizeof(p), &r, sizeof(r));
        if (rv < 0)
            continue;

        printf("%d: %d %s\n", p.id, r.sensor_type,
               r.sensor_name);
        
        if(strcmp(r.sensor_name, "Battery")){ // not eqaul to battery
            mtemp = read_mapped_temperature(p.id);
            switch (mtemp) {
                case EC_TEMP_SENSOR_NOT_PRESENT:
                    break;
                case EC_TEMP_SENSOR_ERROR:
                    fprintf(stderr, "Sensor %d error\n", id);
                    break;
                case EC_TEMP_SENSOR_NOT_POWERED:
                    fprintf(stderr, "Sensor %d disabled\n", id);
                    break;
                case EC_TEMP_SENSOR_NOT_CALIBRATED:
                    fprintf(stderr, "Sensor %d not calibrated\n",
                        id);
                    break;
                default:
                    temp = K_TO_C(mtemp + EC_TEMP_SENSOR_OFFSET);
                }
            temp = K_TO_C(mtemp + EC_TEMP_SENSOR_OFFSET);
            if (temp > max_temp) {
                max_temp = temp;
            }
        }
    }
    
    libectool_release();
    return max_temp;
}
}

// -----------------------------------------------------------------------------
//  Helper functions
// -----------------------------------------------------------------------------

int libectool_init()
{
    char device_name[41] = CROS_EC_DEV_NAME;
    uint16_t vid = USB_VID_GOOGLE, pid = USB_PID_HAMMER;
    int i2c_bus = -1;
    /*
     * First try the preferred /dev interface (which has a built-in mutex).
     * If the COMM_DEV flag is excluded or comm_init_dev() fails,
     * then try alternative interfaces.
     */
    if (!(interfaces & COMM_DEV) || comm_init_dev(device_name)) {
        /* For non-USB alt interfaces, we need to acquire the GEC lock */
        if (!(interfaces & COMM_USB) &&
            acquire_gec_lock(GEC_LOCK_TIMEOUT_SECS) < 0) {
            fprintf(stderr, "Could not acquire GEC lock.\n");
            return -1;
        }
        /* If the interface is set to USB, try that (no lock needed) */
        if (interfaces == COMM_USB) {
#ifndef _WIN32
            if (comm_init_usb(vid, pid)) {
                fprintf(stderr, "Couldn't find EC on USB.\n");
                /* Release the lock if it was acquired */
                release_gec_lock();
                return -1;
            }
#endif
        } else if (comm_init_alt(interfaces, device_name, i2c_bus)) {
            fprintf(stderr, "Couldn't find EC\n");
            release_gec_lock();
            return -1;
        }
    }

    /* Initialize ring buffers for sending/receiving EC commands */
    if (comm_init_buffer()) {
        fprintf(stderr, "Couldn't initialize buffers\n");
        release_gec_lock();
        return -1;
    }

    return 0;
}

void libectool_release()
{
    /* Release the GEC lock. (This is safe even if no lock was acquired.) */
    release_gec_lock();

#ifndef _WIN32
    /* If the interface in use was USB, perform additional cleanup */
    if (interfaces == COMM_USB)
        comm_usb_exit();
#endif
}

int read_mapped_temperature(int id)
{
	int rv;

	if (!read_mapped_mem8(EC_MEMMAP_THERMAL_VERSION)) {
		/*
		 *  The temp_sensor_init() is not called, which implies no
		 * temp sensor is defined.
		 */
		rv = EC_TEMP_SENSOR_NOT_PRESENT;
	} else if (id < EC_TEMP_SENSOR_ENTRIES)
		rv = read_mapped_mem8(EC_MEMMAP_TEMP_SENSOR + id);
	else if (read_mapped_mem8(EC_MEMMAP_THERMAL_VERSION) >= 2)
		rv = read_mapped_mem8(EC_MEMMAP_TEMP_SENSOR_B + id -
				      EC_TEMP_SENSOR_ENTRIES);
	else {
		/* Sensor in second bank, but second bank isn't supported */
		rv = EC_TEMP_SENSOR_NOT_PRESENT;
	}
	return rv;
}

static uint8_t read_mapped_mem8(uint8_t offset)
{
	int ret;
	uint8_t val;

	ret = ec_readmem(offset, sizeof(val), &val);
	if (ret <= 0) {
		fprintf(stderr, "failure in %s(): %d\n", __func__, ret);
		exit(1);
	}
	return val;
}