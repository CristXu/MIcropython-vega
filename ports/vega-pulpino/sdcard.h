/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef _SDCARD_H_
#define _SDCARD_H_
#include "mpconfigport.h"
// SD card detect switch
#define MICROPY_HW_SDCARD_DETECT_PIN        (pin_33)
#define MICROPY_HW_SDCARD_DETECT_PULL       (1) // (GPIO_PULLUP)
#define MICROPY_HW_SDCARD_DETECT_PRESENT    (2) // (GPIO_PIN_RESET)

// this is a fixed size and should not be changed
#define SDCARD_BLOCK_SIZE (512)
enum _sdcard_state {
	kWithout_Sdcard = -5,
} sdcard_state_t;

status_t sdcard_init(void);
status_t sdcard_deinit(void);
bool sdcard_is_present(void);
bool sdcard_power_on(void);
void sdcard_power_off(void);
uint64_t sdcard_get_capacity_in_bytes(void);
uint32_t sdcard_get_lba_count(void);

// these return 0 on success, non-zero on error
mp_uint_t sdcard_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks);
mp_uint_t sdcard_write_blocks(const uint8_t *src, uint32_t block_num, uint32_t num_blocks);

extern const struct _mp_obj_type_t pyb_sdcard_type;
extern const struct _mp_obj_base_t pyb_sdcard_obj;

struct _fs_user_mount_t;
void sdcard_init_vfs(struct _fs_user_mount_t *vfs, int part);
#endif
