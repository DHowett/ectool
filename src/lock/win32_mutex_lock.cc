/* Copyright 2022 Dustin L. Howett
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * win32_mutex_lock.c: Implementation for a binary semaphore using a win32 mutex
 */

#include <Windows.h>

#include <string>

#include "ipc_lock.h"
#include "locks.h"

static int lock_is_held(struct ipc_lock *lock)
{
	return lock->is_held;
}

static int mutex_lock_open_or_create(struct ipc_lock *lock)
{
	std::string name = "Global\\";
	name += lock->filename;
	HANDLE hMutex = CreateMutexA(NULL, FALSE, name.c_str());
	if (!hMutex) {
		fprintf(stderr, "Failed to take global mutex %s: %d\n", name.c_str(), GetLastError());
		return -1;
	}
	lock->context = hMutex;
	return 0;
}

static int mutex_lock_get(struct ipc_lock *lock, int timeout_msecs)
{
	return WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)lock->context, timeout_msecs) ? 0 : -1;
}

static void mutex_lock_release(struct ipc_lock *lock)
{
	ReleaseMutex(lock->context);
}

/*
 * timeout <0 = no timeout (try forever)
 * timeout 0  = do not wait (return immediately)
 * timeout >0 = wait up to $timeout milliseconds
 *
 * returns 0 to indicate lock acquired
 * returns >0 to indicate lock was already held
 * returns <0 to indicate failed to acquire lock
 */
int acquire_lock(struct ipc_lock *lock, int timeout_msecs)
{
	/* check if it is already held */
	if (lock_is_held(lock))
		return 1;

	if (mutex_lock_open_or_create(lock))
		return -1;

	if (mutex_lock_get(lock, timeout_msecs)) {
		lock->is_held = 0;
		CloseHandle((HANDLE)lock->context);
		return -1;
	} else {
		lock->is_held = 1;
	}

	return 0;
}

/*
 * returns 0 if lock was released successfully
 * returns -1 if lock had not been held before the call
 */
int release_lock(struct ipc_lock *lock)
{
	if (lock_is_held(lock)) {
		mutex_lock_release(lock);
		lock->is_held = 0;
		return 0;
	}

	return -1;
}

