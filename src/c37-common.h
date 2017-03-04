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
#include <stdbool.h>

typedef unsigned char byte;

#define SET_BIT(value, bit_index) (value |= (1 << bit_index))
#define CLEAR_BIT(value, bit_index) (value &= ~(1 << bit_index))
#define TOGGLE_BIT(value, bit_index) (value ^= (1 << bit_index))
#define BIT_IS_SET(value, bit_index) (value & (1 << bit_index))


unsigned short
pmu_common_get_crc (const byte *data, size_t data_length);

