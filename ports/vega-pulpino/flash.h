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

#ifndef _FLASH_H_
#define _FLASH_H_

#include "stdlib.h"
#include "stdio.h"

// flash cfg
#define FLASH_SIZE                  0x40 0000  /* 32Mb */
#define FLASH_SECTOR_SIZE           0x1000     /* 4K   */
#define FLASH_PAGE_SIZE             0x100      /* 256B */

typedef enum {
	ID = 0,
	JDEC_ID = 1,
}ID_TYPE;

void spi_flash_init();
void spi_flash_read_id(uint8_t *id, ID_TYPE type);
void spi_flash_read(uint8_t* pData, uint32_t addr, uint32_t size);
int spi_flash_write_page(uint8_t* pData, uint32_t addr);
void spi_flash_erase_sector(uint32_t addr);
void spi_flash_erase_chip(void);
int spi_flash_validate(uint32_t addr, uint32_t size, uint8_t* data);
#endif


