/* c37-bin.c
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

#include "c37-bin.h"

byte
cts_bin_get_type (const byte *data)
{
  if (data[0] == CTS_TYPE_SYNC)
    data++;
  else
    return CTS_TYPE_INVALID;

  switch (data[0])
    {
    case CTS_TYPE_DATA:
    case CTS_TYPE_HEADER:
    case CTS_TYPE_CONFIG1:
    case CTS_TYPE_CONFIG2:
    case CTS_TYPE_COMMAND:
    case CTS_TYPE_CONFIG3:
      return data[0];
    default:
      return CTS_TYPE_INVALID;
    }
}
