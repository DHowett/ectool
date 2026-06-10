/* Copyright 2024 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef EC_TOOLS_ECTOOL_COMM_WIN32_RING0_H_
#define EC_TOOLS_ECTOOL_COMM_WIN32_RING0_H_

#include <stdint.h>

bool ring0_load(void);
uint8_t ring0_inb(uint16_t port);
void ring0_outb(uint8_t value, uint16_t port);

#endif  // EC_TOOLS_ECTOOL_COMM_WIN32_RING0_H_