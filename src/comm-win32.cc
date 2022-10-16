/* Copyright 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <devioctl.h>
#include <ioapiset.h>

#include "comm-host.h"
#include "ec_commands.h"
#include "misc_util.h"

#include "../../FrameworkWindowsUtils/CrosEC/Public.h"

static HANDLE fd;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(t) (sizeof(t) / sizeof(t[0]))
#endif

static const char * const meanings[] = {
	"SUCCESS",
	"INVALID_COMMAND",
	"ERROR",
	"INVALID_PARAM",
	"ACCESS_DENIED",
	"INVALID_RESPONSE",
	"INVALID_VERSION",
	"INVALID_CHECKSUM",
	"IN_PROGRESS",
	"UNAVAILABLE",
	"TIMEOUT",
	"OVERFLOW",
	"INVALID_HEADER",
	"REQUEST_TRUNCATED",
	"RESPONSE_TOO_BIG",
	"BUS_ERROR",
	"BUSY",
	"INVALID_HEADER_VERSION",
	"INVALID_HEADER_CRC",
	"INVALID_DATA_CRC",
	"DUP_UNAVAILABLE",
};

static const char *strresult(int i)
{
	if (i < 0 || i >= ARRAY_SIZE(meanings))
		return "<unknown>";
	return meanings[i];
}

static int ec_command_win32(int command, int version,
			     const void *outdata, int outsize,
			     void *indata, int insize)
{
	PCROSEC_COMMAND s_cmd;

	assert(outsize == 0 || outdata != NULL);
	assert(insize == 0 || indata != NULL);

	int size = sizeof(CROSEC_COMMAND) + MAX(outsize, insize);
	s_cmd = (PCROSEC_COMMAND)(malloc(size));
	if (s_cmd == NULL)
		return -EC_RES_ERROR;

	s_cmd->command = command;
	s_cmd->version = version;
	s_cmd->result = 0xff;
	s_cmd->outsize = outsize;
	s_cmd->insize = insize;
	memcpy(CROSEC_COMMAND_DATA(s_cmd), outdata, outsize);

	DWORD r = 0;
	BOOL s = DeviceIoControl(fd, IOCTL_CROSEC_XCMD, s_cmd, size, s_cmd, size, &r, NULL);
	if (!s) {
		int gle = GetLastError();
		fprintf(stderr, "ioctl errno %x, EC result %d (%s)\n",
			gle, s_cmd->result,
			strresult(s_cmd->result));
		r = -EC_RES_ERROR;
	} else {
		memcpy(indata, CROSEC_COMMAND_DATA(s_cmd), MIN(s_cmd->insize, insize));
		r -= sizeof(CROSEC_COMMAND);
		if (s_cmd->result != EC_RES_SUCCESS) {
			fprintf(stderr, "EC result %d (%s)\n", s_cmd->result,
				strresult(s_cmd->result));
			r = -EECRESULT - s_cmd->result;
		}
	}
	free(s_cmd);

	return r;
}

static int ec_readmem_win32(int offset, int bytes, void *dest)
{
	CROSEC_READMEM s_mem;
	struct ec_params_read_memmap r_mem;
	static int fake_it;

	if (!fake_it) {
		s_mem.offset = offset;
		s_mem.bytes = bytes;
		DWORD rv = 0;
		if (!DeviceIoControl(fd, IOCTL_CROSEC_RDMEM, &s_mem, sizeof(s_mem), &s_mem, sizeof(s_mem), &rv, NULL)) {
			fake_it = 1;
		} else {
			memcpy(dest, s_mem.buffer, bytes);
			return bytes;
		}
	}

	r_mem.offset = offset;
	r_mem.size = bytes;
	return ec_command_win32(EC_CMD_READ_MEMMAP, 0,
				 &r_mem, sizeof(r_mem),
				 dest, bytes);
}

static int ec_pollevent_dev(unsigned long mask, void *buffer, size_t buf_size,
			    int timeout)
{
	return 0;
#if 0 // TODO: Not Implemented
	int rv;
	struct pollfd pf = { .fd = fd, .events = POLLIN };

	ioctl(fd, CROS_EC_DEV_IOCEVENTMASK_V2, mask);

	rv = poll(&pf, 1, timeout);
	if (rv != 1)
		return rv;

	if (pf.revents != POLLIN)
		return -pf.revents;

	return read(fd, buffer, buf_size);
#endif
}

int comm_init_dev(const char *device_name)
{
	int (*ec_cmd_readmem)(int offset, int bytes, void *dest);
	char version[80];
	char device[80] = "/dev/";
	int r;
	char *s;

	fd = CreateFileA(device_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (fd == NULL)
		return 1;

	ec_command_proto = ec_command_win32;
	ec_cmd_readmem = ec_readmem_win32;

	if (ec_cmd_readmem(EC_MEMMAP_ID, 2, version) == 2 &&
	    version[0] == 'E' && version[1] == 'C')
		ec_readmem = ec_cmd_readmem;
	ec_pollevent = ec_pollevent_dev;

	/*
	 * Set temporary size, will be updated later.
	 */
	ec_max_outsize = EC_PROTO2_MAX_PARAM_SIZE - 8;
	ec_max_insize = EC_PROTO2_MAX_PARAM_SIZE;

	return 0;
}
