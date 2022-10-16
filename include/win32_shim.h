#pragma once

#define _CRT_RAND_S

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdlib.h>
#include <time.h>
#include <io.h>

#define BUILD_ASSERT(...)
#define CROS_EC_DEV_NAME "\\\\.\\GLOBALROOT\\Device\\CrosEC"

#define COMDAT_INLINE __declspec(noinline) inline

COMDAT_INLINE void usleep(unsigned us) {
	Sleep(us/1000);
}

#define strcasecmp _stricmp
#define strncasecmp _strnicmp

#define STDOUT_FILENO 0

COMDAT_INLINE int write(int fd, const void* buffer, unsigned int count) {
	return _write(fd, buffer, count);
}

COMDAT_INLINE struct tm* localtime_r(const time_t* timer, struct tm* buf) {
	localtime_s(buf, timer);
	return buf;
}

COMDAT_INLINE int rand_r(unsigned int* /*seedp*/) {
	unsigned int x = 0;
	rand_s(&x);
	return static_cast<int>(x & 0x7FFFFFFF);
}
