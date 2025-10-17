/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include "comm-win32-ring0.h"

#include <windows.h>

#include <devioctl.h>
#include <ioapiset.h>

#include <stdexcept>
#include <string>

class WinRing0PortInputOutput {
 public:
	bool load() {
		driver_handle = CreateFileA("\\\\.\\WinRing0_1_2_0",
					GENERIC_READ | GENERIC_WRITE, 0, 0,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (!is_loaded()) {
			fprintf(stderr, "Failed to open WinRing0_1_2_0 device: error code %lu\n", GetLastError());
		}
		return is_loaded();
	}

	bool unload() {
		if (!is_loaded()) {
			return false;
		}
		bool close_handle_success = CloseHandle(driver_handle);
		driver_handle = NULL;
		return close_handle_success;
	}

	bool is_loaded() const {
		return driver_handle != NULL && driver_handle != INVALID_HANDLE_VALUE;
	}

	uint8_t read_byte(uint32_t port) const {
		if (!is_loaded()) {
			std::string message =
				"WinRing0PortInputOutput::read_byte(" +
				std::to_string(port) +
				") failed as the driver is not loaded\n";
			fprintf(stderr, message.c_str());
			throw std::runtime_error(message);
		}
		constexpr DWORD IOCTL_OLS_READ_IO_PORT_BYTE = 2621464780;
		uint32_t input = port;
		uint32_t output = 0;
		DWORD bytes_returned = 0;
		BOOL io_control_success = DeviceIoControl(
			driver_handle, IOCTL_OLS_READ_IO_PORT_BYTE, &input,
			sizeof(input), &output, sizeof(output), &bytes_returned, NULL);
		if (!io_control_success) {
			std::string message =
				"WinRing0PortInputOutput::read_byte(" +
				std::to_string(port) +
				") failed as DeviceIoControl returned false";
			fprintf(stderr, message.c_str());
			throw std::runtime_error(message);
		}
		return (uint8_t)(output & 0xFF);
	}

	void write_byte(uint8_t value, uint32_t port) const {
		if (!is_loaded()) {
			std::string message =
				"WinRing0PortInputOutput::write_byte(" +
				std::to_string(value) +
				", " +
				std::to_string(port) +
				") failed as the driver is not loaded";
			fprintf(stderr, message.c_str());
			throw std::runtime_error(message);
		}
		constexpr DWORD IOCTL_OLS_WRITE_IO_PORT_BYTE = 2621481176;
		WriteByteData input{
			.port = port,
			.value = value,
		};
		DWORD bytes_returned = 0;
		BOOL io_control_success = DeviceIoControl(
			driver_handle, IOCTL_OLS_WRITE_IO_PORT_BYTE, &input,
			sizeof(input), NULL, 0, &bytes_returned, NULL);
		if (!io_control_success) {
			std::string message =
				"WinRing0PortInputOutput::write_byte(" +
				std::to_string(value) +
				", " +
				std::to_string(port) +
				") failed as DeviceIoControl returned false";
			fprintf(stderr, message.c_str());
			throw std::runtime_error(message);
		}
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

bool ring0_load(void) {
	return ring0.load();
}

uint8_t ring0_inb(uint16_t port) {
	return ring0.read_byte(port);
}

void ring0_outb(uint8_t value, uint16_t port) {
	ring0.write_byte(value, port);
}
