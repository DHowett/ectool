/* Copyright 2013 The ChromiumOS Authors + 2024 DHowett + 2024 tomasiser
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <chrono>
#include <thread>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <windows.h>
#include <devioctl.h>
#include <ioapiset.h>

#include "comm-host.h"

#define INITIAL_UDELAY 5 /* 5 us */
#define MAXIMUM_UDELAY 10000 /* 10 ms */

class WinRing0PortInputOutput {
public:
	bool load() {
		driver_handle = CreateFileA("\\\\.\\WinRing0_1_2_0", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		return is_loaded();
	}

	bool unload() {
		if (!is_loaded()) { return false; }
		bool close_handle_success = CloseHandle(driver_handle);
		driver_handle = NULL;
		return close_handle_success;
	}

	bool is_loaded() const {
		return driver_handle != NULL && driver_handle != INVALID_HANDLE_VALUE;
	}

	uint8_t read_byte(uint32_t port) const {
		if (!is_loaded()) { std::string message = "WinRing0PortInputOutput::read_byte(" + std::to_string(port) + ") failed as the driver is not loaded\n"; fprintf(stderr, message.c_str()); throw std::runtime_error(message); }
		constexpr DWORD IOCTL_OLS_READ_IO_PORT_BYTE = 2621464780;
		uint32_t input = port;
		uint32_t output = 0;
		DWORD bytes_returned = 0;
		BOOL io_control_success = DeviceIoControl(driver_handle, IOCTL_OLS_READ_IO_PORT_BYTE, &input, sizeof(input), &output, sizeof(output), &bytes_returned, NULL);
		if (!io_control_success) { std::string message = "WinRing0PortInputOutput::read_byte(" + std::to_string(port) + ") failed as DeviceIoControl returned false"; fprintf(stderr, message.c_str()); throw std::runtime_error(message); }
		return (uint8_t)(output & 0xFF);
	}

	void write_byte(uint8_t value, uint32_t port) const {
		if (!is_loaded()) { std::string message = "WinRing0PortInputOutput::write_byte(" + std::to_string(value) + ", " + std::to_string(port) + ") failed as the driver is not loaded"; fprintf(stderr, message.c_str()); throw std::runtime_error(message); }
		constexpr DWORD IOCTL_OLS_WRITE_IO_PORT_BYTE = 2621481176;
		WriteByteData input{ port = port, value = value };
		DWORD bytes_returned = 0;
		BOOL io_control_success = DeviceIoControl(driver_handle, IOCTL_OLS_WRITE_IO_PORT_BYTE, &input, sizeof(input), NULL, 0, &bytes_returned, NULL);
		if (!io_control_success) { std::string message = "WinRing0PortInputOutput::write_byte(" + std::to_string(value) + ", " + std::to_string(port) + ") failed as DeviceIoControl returned false"; fprintf(stderr, message.c_str()); throw std::runtime_error(message); }
	}

private:
	HANDLE driver_handle = NULL;

#pragma pack(push, 1)
	struct WriteByteData {
		uint32_t port;
		uint8_t value;
	};
#pragma pack(pop)
};

static WinRing0PortInputOutput ring0;
static int ec_lpc_memmap_base;

/*
 * Wait for the EC to be unbusy.  Returns 0 if unbusy, non-zero if
 * timeout.
 */
static int wait_for_ec(int status_addr, int timeout_usec)
{
	int i;
	int delay = INITIAL_UDELAY;

	for (i = 0; i < timeout_usec; i += delay) {
		/*
		 * Delay first, in case we just sent out a command but the EC
		 * hasn't raised the busy flag.  However, I think this doesn't
		 * happen since the LPC commands are executed in order and the
		 * busy flag is set by hardware.  Minor issue in any case,
		 * since the initial delay is very short.
		 */
		std::this_thread::sleep_for(std::chrono::microseconds((std::min)(delay, timeout_usec - i)));

		if (!(ring0.read_byte(status_addr) & EC_LPC_STATUS_BUSY_MASK))
			return 0;

		/* Increase the delay interval after a few rapid checks */
		if (i > 20)
			delay = (std::min)(delay * 2, MAXIMUM_UDELAY);
	}
	return -1; /* Timeout */
}

static int ec_command_winring0(int command, int version, const void* outdata,
	int outsize, void* indata, int insize)
{
	struct ec_lpc_host_args args;
	const uint8_t* d;
	uint8_t* dout;
	int csum;
	int i;

	/* Fill in args */
	args.flags = EC_HOST_ARGS_FLAG_FROM_HOST;
	args.command_version = version;
	args.data_size = outsize;

	/* Initialize checksum */
	csum = command + args.flags + args.command_version + args.data_size;

	/* Write data and update checksum */
	for (i = 0, d = (uint8_t*)outdata; i < outsize; i++, d++) {
		ring0.write_byte(*d, EC_LPC_ADDR_HOST_PARAM + i);
		csum += *d;
	}

	/* Finalize checksum and write args */
	args.checksum = (uint8_t)csum;
	for (i = 0, d = (const uint8_t*)&args; i < sizeof(args); i++, d++)
		ring0.write_byte(*d, EC_LPC_ADDR_HOST_ARGS + i);

	ring0.write_byte(command, EC_LPC_ADDR_HOST_CMD);

	if (wait_for_ec(EC_LPC_ADDR_HOST_CMD, 1000000)) {
		fprintf(stderr, "Timeout waiting for EC response\n");
		return -EC_RES_ERROR;
	}

	/* Check result */
	i = ring0.read_byte(EC_LPC_ADDR_HOST_DATA);
	if (i) {
		fprintf(stderr, "EC returned error result code %d\n", i);
		return -EECRESULT - i;
	}

	/* Read back args */
	for (i = 0, dout = (uint8_t*)&args; i < sizeof(args); i++, dout++)
		*dout = ring0.read_byte(EC_LPC_ADDR_HOST_ARGS + i);

	/*
	 * If EC didn't modify args flags, then somehow we sent a new-style
	 * command to an old EC, which means it would have read its params
	 * from the wrong place.
	 */
	if (!(args.flags & EC_HOST_ARGS_FLAG_TO_HOST)) {
		fprintf(stderr, "EC protocol mismatch\n");
		return -EC_RES_INVALID_RESPONSE;
	}

	if (args.data_size > insize) {
		fprintf(stderr, "EC returned too much data\n");
		return -EC_RES_INVALID_RESPONSE;
	}

	/* Start calculating response checksum */
	csum = command + args.flags + args.command_version + args.data_size;

	/* Read response and update checksum */
	for (i = 0, dout = (uint8_t*)indata; i < args.data_size; i++, dout++) {
		*dout = ring0.read_byte(EC_LPC_ADDR_HOST_PARAM + i);
		csum += *dout;
	}

	/* Verify checksum */
	if (args.checksum != (uint8_t)csum) {
		fprintf(stderr, "EC response has invalid checksum\n");
		return -EC_RES_INVALID_CHECKSUM;
	}

	/* Return actual amount of data received */
	return args.data_size;
}

static int ec_command_winring0_3(int command, int version, const void* outdata,
	int outsize, void* indata, int insize)
{
	struct ec_host_request rq;
	struct ec_host_response rs;
	const uint8_t* d;
	uint8_t* dout;
	int csum = 0;
	int i;

	/* Fail if output size is too big */
	if (outsize + sizeof(rq) > EC_LPC_HOST_PACKET_SIZE)
		return -EC_RES_REQUEST_TRUNCATED;

	/* Fill in request packet */
	/* TODO(crosbug.com/p/23825): This should be common to all protocols */
	rq.struct_version = EC_HOST_REQUEST_VERSION;
	rq.checksum = 0;
	rq.command = command;
	rq.command_version = version;
	rq.reserved = 0;
	rq.data_len = outsize;

	/* Copy data and start checksum */
	for (i = 0, d = (const uint8_t*)outdata; i < outsize; i++, d++) {
		ring0.write_byte(*d, EC_LPC_ADDR_HOST_PACKET + sizeof(rq) + i);
		csum += *d;
	}

	/* Finish checksum */
	for (i = 0, d = (const uint8_t*)&rq; i < sizeof(rq); i++, d++)
		csum += *d;

	/* Write checksum field so the entire packet sums to 0 */
	rq.checksum = (uint8_t)(-csum);

	/* Copy header */
	for (i = 0, d = (const uint8_t*)&rq; i < sizeof(rq); i++, d++)
		ring0.write_byte(*d, EC_LPC_ADDR_HOST_PACKET + i);

	/* Start the command */
	ring0.write_byte(EC_COMMAND_PROTOCOL_3, EC_LPC_ADDR_HOST_CMD);

	if (wait_for_ec(EC_LPC_ADDR_HOST_CMD, 1000000)) {
		fprintf(stderr, "Timeout waiting for EC response\n");
		return -EC_RES_ERROR;
	}

	/* Check result */
	i = ring0.read_byte(EC_LPC_ADDR_HOST_DATA);
	if (i) {
		fprintf(stderr, "EC returned error result code %d\n", i);
		return -EECRESULT - i;
	}

	/* Read back response header and start checksum */
	csum = 0;
	for (i = 0, dout = (uint8_t*)&rs; i < sizeof(rs); i++, dout++) {
		*dout = ring0.read_byte(EC_LPC_ADDR_HOST_PACKET + i);
		csum += *dout;
	}

	if (rs.struct_version != EC_HOST_RESPONSE_VERSION) {
		fprintf(stderr, "EC response version mismatch\n");
		return -EC_RES_INVALID_RESPONSE;
	}

	if (rs.reserved) {
		fprintf(stderr, "EC response reserved != 0\n");
		return -EC_RES_INVALID_RESPONSE;
	}

	if (rs.data_len > insize) {
		fprintf(stderr, "EC returned too much data\n");
		return -EC_RES_RESPONSE_TOO_BIG;
	}

	/* Read back data and update checksum */
	for (i = 0, dout = (uint8_t*)indata; i < rs.data_len; i++, dout++) {
		*dout = ring0.read_byte(EC_LPC_ADDR_HOST_PACKET + sizeof(rs) + i);
		csum += *dout;
	}

	/* Verify checksum */
	if ((uint8_t)csum) {
		fprintf(stderr, "EC response has invalid checksum\n");
		return -EC_RES_INVALID_CHECKSUM;
	}

	/* Return actual amount of data received */
	return rs.data_len;
}

static int ec_readmem_winring0(int offset, int bytes, void* dest)
{
	int i = offset;
	char* s = (char*)(dest);
	int cnt = 0;

	if (offset >= EC_MEMMAP_SIZE - bytes)
		return -1;

	if (bytes) { /* fixed length */
		for (; cnt < bytes; i++, s++, cnt++)
			*s = ring0.read_byte(ec_lpc_memmap_base + i);
	}
	else { /* string */
		for (; i < EC_MEMMAP_SIZE; i++, s++) {
			*s = ring0.read_byte(ec_lpc_memmap_base + i);
			cnt++;
			if (!*s)
				break;
		}
	}

	return cnt;
}

static int ec_try_init_winring0(int memmap_base)
{
	/*
	 * Test if LPC command args are supported.
	 *
	 * The cheapest way to do this is by looking for the memory-mapped
	 * flag.  This is faster than sending a new-style 'hello' command and
	 * seeing whether the EC sets the EC_HOST_ARGS_FLAG_FROM_HOST flag
	 * in args when it responds.
	 */
	if (ring0.read_byte(memmap_base + EC_MEMMAP_ID) != 'E' ||
		ring0.read_byte(memmap_base + EC_MEMMAP_ID + 1) != 'C') {
		return -5;
	}

	ec_lpc_memmap_base = memmap_base;
	return 0;
}

int comm_init_winring0(void)
{
	int i, rv;
	int byte = 0xff;

	/* Load the communication with WinRing0 */
	if (!ring0.load()) {
		fprintf(stderr, "Couldn't open communication with the WinRing0 driver. Keep in mind that for this interface:\n");
		fprintf(stderr, "  - The application must be executed with admin privileges.\n");
		fprintf(stderr, "  - The WinRing0 1.2.0 driver must be already running on the system (e.g., via Libre Hardware Monitor).\n");
		return -3;
	}

	/*
	 * Test if the I/O port has been configured for Chromium EC LPC
	 * interface.  Chromium EC guarantees that at least one status bit will
	 * be 0, so if the command and data bytes are both 0xff, very likely
	 * that Chromium EC is not present.  See crosbug.com/p/10963.
	 */
	byte &= ring0.read_byte(EC_LPC_ADDR_HOST_CMD);
	byte &= ring0.read_byte(EC_LPC_ADDR_HOST_DATA);
	if (byte == 0xff) {
		fprintf(stderr, "Port 0x%x,0x%x are both 0xFF.\n",
			EC_LPC_ADDR_HOST_CMD, EC_LPC_ADDR_HOST_DATA);
		fprintf(stderr,
			"Very likely this board doesn't have a Chromium EC.\n");
		return -4;
	}

	rv = ec_try_init_winring0(EC_LPC_ADDR_MEMMAP);
	if (rv < 0)
	{
		// Fall back to the Framework Laptop 13 (AMD Ryzen 7040 Series) MMIO address
		rv = ec_try_init_winring0(0xE00);
	}

	if (rv < 0)
	{
		fprintf(stderr, "Missing Chromium EC memory map.\n");
		return rv;
	}

	/* Check which command version we'll use */
	i = ring0.read_byte(ec_lpc_memmap_base + EC_MEMMAP_HOST_CMD_FLAGS);

	if (i & EC_HOST_CMD_FLAG_VERSION_3) {
		/* Protocol version 3 */
		ec_command_proto = ec_command_winring0_3;
		ec_max_outsize = EC_LPC_HOST_PACKET_SIZE -
			sizeof(struct ec_host_request);
		ec_max_insize = EC_LPC_HOST_PACKET_SIZE -
			sizeof(struct ec_host_response);

	}
	else if (i & EC_HOST_CMD_FLAG_LPC_ARGS_SUPPORTED) {
		/* Protocol version 2 */
		ec_command_proto = ec_command_winring0;
		ec_max_outsize = ec_max_insize = EC_PROTO2_MAX_PARAM_SIZE;

	}
	else {
		fprintf(stderr, "EC doesn't support protocols we need.\n");
		return -5;
	}

	/* Either one supports reading mapped memory directly. */
	ec_readmem = ec_readmem_winring0;
	return 0;
}
