#ifndef __CROS_EC_FRAMEWORK_OEM_EC_COMMANDS_H
#define __CROS_EC_FRAMEWORK_OEM_EC_COMMANDS_H

#include "ec_commands.h"

/*****************************************************************************/
/* Charge limit control */
#define FW_EC_CMD_CHARGE_LIMIT   0x3E03

#define FW_EC_CHARGE_LIMIT_CLEAR    BIT(0)
#define FW_EC_CHARGE_LIMIT_SET      BIT(1)
#define FW_EC_CHARGE_LIMIT_QUERY    BIT(3)
#define FW_EC_CHARGE_LIMIT_OVERRIDE BIT(7)
struct fw_ec_params_charge_limit {
	uint8_t flags;
	uint16_t limit; // 0-100
} __ec_align1;

struct fw_ec_response_charge_limit {
	uint16_t limit;
} __ec_align1;

/*****************************************************************************/
/* PS/2 Mouse Emulation Mode */
#define FW_EC_CMD_SET_PS2_EMULATION 0x3E08

struct fw_ec_params_set_ps2_emulation {
	uint8_t enabled; // 0x00 = disabled, 0x01 = enabled
} __ec_align1;

/*****************************************************************************/
/* Keyboard Remapping */
#define FW_EC_CMD_SET_KEY_MAPPING 0x3E0C
struct scancode_matrix_pair {
	uint8_t row;
	uint8_t column;
	uint16_t scancode;
} __ec_align1;

enum fw_ec_key_mapping_op {
	FW_EC_KEY_MAPPING_GET,
	FW_EC_KEY_MAPPING_SET,
};

struct fw_ec_params_set_key_mapping {
	uint32_t count;
	uint32_t op;
	struct scancode_matrix_pair pairs[]; // up to 0x20
} __ec_align1;

/*****************************************************************************/
/* vPro Remote Wake */
#define FW_EC_CMD_SET_VPRO_REMOTE_WAKE 0x3E0D
struct fw_ec_params_set_vpro_remote_wake {
	uint8_t enabled; // 0x00 = disabled, 0x01 = enabled
} __ec_align1;

/*****************************************************************************/
/* Power button brightness levels */
#define FW_EC_CMD_SET_POWER_BUTTON_BRIGHTNESS 0x3E0E

enum fw_ec_power_button_brightness {
	FW_EC_POWER_BUTTON_BRIGHTNESS_HIGH,
	FW_EC_POWER_BUTTON_BRIGHTNESS_MEDIUM,
	FW_EC_POWER_BUTTON_BRIGHTNESS_LOW
};

enum fw_ec_power_button_brightess_op {
	FW_EC_POWER_BUTTON_BRIGHTNESS_SET,
	FW_EC_POWER_BUTTON_BRIGHTNESS_GET,
};

struct fw_ec_params_set_power_button_brightness {
	uint8_t brightness; // from above constants
	uint8_t op;
} __ec_align1;

#endif
