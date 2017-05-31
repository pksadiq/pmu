/* c37-header.c
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

#include "c37-header.h"

/**
 * cts_header_get_bin:
 * @name: (nullable): the name to be included in header
 *
 * Generates binary data in c37 header format.
 *
 * Returns (transfer full): A newly allocated bytes in network order.
 * Free the data with free() when no longer required.
 */

byte *
cts_header_get_bin (CtsConf    *conf,
                    const char *name)
{
  int length, frame_size;
  byte *data = NULL, *copy;
  uint16_t byte2;
  uint32_t byte4;

  if (name == NULL)
    return NULL;

  length = strlen (name);
  frame_size = length + HEADER_COMMON_SIZE;

  data = malloc (frame_size);
  copy = data;

  if (data == NULL)
    return NULL;

  byte2 = htons (SYNC_HEADER);
  memcpy (copy, &byte2, 2);
  copy += 2;

  byte2 = htons (frame_size);
  memcpy (copy, &byte2, 2);
  copy += 2;

  byte2 = htons (cts_conf_get_id_code (conf));
  memcpy (copy, &byte2, 2);
  copy += 2;

  byte4 = htonl (cts_common_get_time ());
  memcpy (copy, &byte4, 4);
  copy += 4;

  byte4 = htonl (cts_common_get_frac_of_second (cts_conf_get_time_base
                                                 (conf)));
  memcpy (copy, &byte4, 4);
  copy += 4;

  memcpy (copy, name, length);
  copy += length;

  byte2 = htons (cts_common_calc_crc(data, frame_size - 2, NULL));
  memcpy (copy, &byte2, 2);

  return data;
}
