/* pmu-common.c
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

#include "c37-common.h"

/* Based on the sample code in IEEE Std C37.118.2-2011 */
uint16_t
pmu_common_get_crc (const byte *data, size_t data_length)
{
  uint16_t crc = 0xFFFF;
  uint16_t temp;
  uint16_t quick;

  for (size_t i = 0; i < data_length; i++)
    {
      temp = (crc >> 8) ^ data[i];
      crc <<= 8;
      quick = temp ^ (temp >> 4);
      crc ^= quick;
      quick <<= 5;
      crc ^= quick;
      quick <<= 7;
      crc ^= quick;
    }

  return crc;
}

uint32_t
pmu_common_get_current_seconds (void)
{
  return time (NULL);
}
