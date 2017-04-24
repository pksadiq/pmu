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


#include "c37-command.h"

#include "c37-bin.h"

byte
cts_bin_get_type (const byte *data)
{
  if (data && data[0] == CTS_TYPE_SYNC)
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

uint16_t
cts_bin_get_frame_size (const byte *data)
{
  uint16_t frame_size;

  if (data == NULL)
    return 0;

  /* Skip SYNC bytes */
  data += 2;

  memcpy (&frame_size, data, 2);
  frame_size = ntohs (frame_size);

  return frame_size;
}

uint16_t
cts_bin_get_id_code (const byte *data,
                     bool        skip_first)
{
  uint16_t id_code;

  if (skip_first)
    data += 4;

  memcpy (&id_code, data, 2);
  id_code = ntohs (id_code);

  return id_code;
}

uint32_t
cts_bin_get_time_seconds (const byte *data,
                          bool        skip_first)
{
  uint32_t time;

  if (data == NULL)
    return 0;

  if (skip_first)
    data += 4;

  /*  Skip id code */
  data += 2;

  memcpy (&time, data, 4);
  time = ntohl (time);

  return time;
}

uint32_t
cts_bin_get_frac_of_second (const byte *data,
                            uint32_t    time_base,
                            bool        skip_first)
{
  uint32_t frac_of_second;

  if (data == NULL)
    return 0;

  if (skip_first)
    data += 4;

  /*  skip 2 byte id code and 4 byte SOC time */
  data += 6;

  memcpy (&frac_of_second, data, 4);
  frac_of_second = ntohl (frac_of_second);

  if (time_base)
    frac_of_second = frac_of_second * (pow (10, 9) / time_base);

  return frac_of_second;
}

uint16_t
cts_bin_get_crc (const byte *data,
                 const byte *header)
{
  uint16_t frame_size;
  uint16_t crc;

  if (data == NULL)
    return 0;

  if (header != NULL)
    /* Also skip the 4 byte header size */
    frame_size = cts_bin_get_frame_size (header) - 4;
  else
    frame_size = cts_bin_get_frame_size (data);

  memcpy (&crc, data + frame_size - 2, 2);
  crc = htons (crc);

  return crc;
}

uint16_t
cts_bin_get_command_type (const byte *data,
                          bool        skip_first)
{
  uint16_t command;

  if (skip_first)
    data += 4;

  memcpy (&command, data + COMMAND_MINIMUM_FRAME_SIZE - 8, 2);
  command = htons (command);

  return cts_command_get_type (command);
}
