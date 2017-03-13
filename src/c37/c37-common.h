/* pmu-common.h
 *
 * Copyright (C) 2017 Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <arpa/inet.h>

typedef unsigned char byte;

enum CtsType{
  CTS_TYPE_INVALID,
  CTS_TYPE_DATA    = 0x01,
  CTS_TYPE_HEADER  = 0x11,
  CTS_TYPE_CONFIG1 = 0x21,
  CTS_TYPE_CONFIG2 = 0x31,
  CTS_TYPE_COMMAND = 0x41,
  CTS_TYPE_CONFIG3 = 0x52,
  CTS_TYPE_SYNC    = 0xAA,
};

#define SET_BIT(value, bit_index) (value |= (1 << bit_index))
#define CLEAR_BIT(value, bit_index) (value &= ~(1 << bit_index))
#define TOGGLE_BIT(value, bit_index) (value ^= (1 << bit_index))
#define BIT_IS_SET(value, bit_index) (value & (1 << bit_index))


unsigned short
cts_common_calc_crc (const byte *data, size_t data_length, const byte *header);
uint32_t
cts_common_get_time_seconds (void);
uint32_t
cts_common_get_fraction_of_seconds (void);
int
cts_common_get_type (const byte *data);
uint16_t
cts_common_get_size (const byte *data, uint16_t offset);
uint16_t
cts_common_get_crc (const byte *data, uint16_t offset);
