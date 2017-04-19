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

/**
 * cts_common_calc_crc:
 * @data: pointer to data for which crc has to be calculated.
 * @data_length: size of complete data, excluding last 2 bytes CRC.
 * @header: (nullable): 4 bytes header data (2 byte SYNC and 2 byte FRAME
 * size).
 *
 * If @header is not %NULL, @data should not contain SYNC and FRAME size bytes
 * (the first 4 bytes). Else, @data should begin with SYNC byte.
 *
 * In any case, @data_length is the total length of data (including the 2 byte
 * SYNC and 2 byte FRAME size) excluding the 2 byte CRC.
 *
 * For example, the @data_length of the smallest COMMAND frame will be 16.
 *
 * This function is based on the sample code in IEEE Std C37.118.2-2011.
 *
 * Returns: The 2 byte CRC in host order.
 */
uint16_t
cts_common_calc_crc (const byte *data, size_t data_length, const byte *header)
{
  uint16_t crc = 0xFFFF;
  uint16_t temp;
  uint16_t quick;

  if (header != NULL)
    {
      data_length = data_length - 4;

      for (size_t j = 0; j < 4; j++)
        {
          temp = (crc >> 8) ^ header[j];
          crc <<= 8;
          quick = temp ^ (temp >> 4);
          crc ^= quick;
          quick <<= 5;
          crc ^= quick;
          quick <<= 7;
          crc ^= quick;
        }
    }
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

/**
 * cts_common_get_crc:
 * @data: pointer to received data for which crc has to be extracted.
 * @offset: offset such that data[offset] will point to the first byte
 * of the CRC.
 *
 * Returns: The 2 byte CRC extracted from @data in host order.
 */
uint16_t
cts_common_get_crc (const byte *data, uint16_t offset)
{
  uint16_t length;
  /* Eg: if data frame size is 20 bytes, offset will be 18 */
  const byte *offset_data = data + offset;
  memcpy (&length, offset_data, 2);

  length = ntohs (length);

  return length;
}

/**
 * cts_common_check_crc:
 * @data: pointer to data for which crc has to be matched.
 * @data_length: size of complete data, excluding last 2 bytes CRC.
 * @header: (nullable): 4 bytes header data (2 byte SYNC and 2 byte FRAME
 * size).
 * @offset: offset such that data[offset] will point to the first byte
 * of the CRC.
 *
 * If @header is not %NULL, @data should not contain SYNC and FRAME size bytes
 * (the first 4 bytes). Else, @data should begin with SYNC byte.
 *
 * In any case, @data_length is the total length of data (including the 2 byte
 * SYNC and 2 byte FRAME size) excluding the 2 byte CRC.
 *
 * For example, the @data_length of the smallest COMMAND frame will be 16.
 *
 * This function is based on the sample code in IEEE Std C37.118.2-2011.
 *
 * Returns: %TRUE if CRC matches. Else, return %FALSE.
 */
bool
cts_common_check_crc (const byte *data, size_t data_length, const byte *header, uint16_t offset)
{
  uint16_t old_crc = cts_common_get_crc (data, offset);
  uint16_t new_crc = cts_common_calc_crc (data, data_length, header);

  return old_crc == new_crc;
}

/**
 * cts_common_get_size:
 * @data: pointer to data for which size has to be retrieved.
 * @offset: offset such that data[offset] will point to the first
 * byte of size. If data contains SYNC bytes, offset will be 2.
 *
 * Retrieve the data size, That is, the size of complete IEEE C37.118.2-2011
 * formatted frame (From SYNC to last byte of CRC).
 *
 * The size is supposed be stored in data as 2 bytes in network order
 * (Big Endian).
 *
 * Returns: a 16 bit unsigned integer.
 */
uint16_t
cts_common_get_size (const byte *data, uint16_t offset)
{
  uint16_t length;
  /* Eg: if data begins with SYNC bytes, offset will be 2 */
  const byte *offset_data = data + offset;

  /* Get the 2 bytes from data. memcpy was used instead of casting
   * to avoid possible alignment issues
   */
  memcpy (&length, offset_data, 2);

  length = ntohs (length);

  return length;
}

/**
 * cts_common_get_time_seconds:
 *
 * Returns the seconds since epoch (1970 January 1) in UTC.
 *
 * Returns: a 32 bit unsigned integer.
 */
uint32_t
cts_common_get_time_seconds (void)
{
  return time (NULL);
}

/**
 * cts_common_get_fraction_of_second:
 * @time_base: The precision of fraction of second
 *
 * Calculate the fraction of second to the precision of @time_base.
 * @time_base should be selected based on the precision you wish
 * to set for the frame.
 *
 * For example, if you are conveying that the time will be accurate
 * to the order of a microsecond, you can select @time_base as 10^6.
 *
 * The @value returned will be such that @value/@time_base is always
 * less than 1.
 *
 * cts_common_get_time_seconds() + @value will give the precise time
 * since epoch.
 *
 * Returns: a 32 bit unsigned integer.
 */
uint32_t
cts_common_get_fraction_of_second (uint32_t time_base)
{
  struct timespec ts;
  uint32_t time;

  /*  This is a C11 function */
  timespec_get(&ts, TIME_UTC);

  time = ts.tv_nsec * (time_base / pow (10, 9));

  /* Return the last 24 bits only */
  return time & 0x00FFFFFF;
}

/**
 * cts_common_get_type:
 * @data: IEEE C37.118.2-2011 formatted frame.
 *
 * Returns the SYNC type of the frame. The data is assumed
 * to have network byte order (Big Endian order).
 *
 * One of the following is returned: #CTS_TYPE_DATA, #CTS_TYPE_HEADER,
 * #CTS_TYPE_CONFIG1, #CTS_TYPE_CONFIG2, #CTS_TYPE_CONFIG3
 * #CTS_TYPE_COMMAND.
 *
 * If SYNC is invalid #CTS_TYPE_INVALID is returned.
 *
 * Returns: an integer.
 */
int
cts_common_get_type (const byte *data)
{
  if (data == NULL || *data != CTS_TYPE_SYNC)
    return CTS_TYPE_INVALID;

  switch (data[1])
    {
    case CTS_TYPE_DATA:
    case CTS_TYPE_HEADER:
    case CTS_TYPE_CONFIG1:
    case CTS_TYPE_CONFIG2:
    case CTS_TYPE_CONFIG3:
    case CTS_TYPE_COMMAND:
      return data[1];
    default:
      return CTS_TYPE_INVALID;
    }
}

void
cts_common_set_time (uint32_t *time)
{
  *time = cts_common_get_time_seconds ();
}

void
cts_common_set_frac_of_second (uint32_t *frac_of_second,
                               uint16_t  time_base)
{
  uint32_t data;
  uint32_t new_frac_of_second;

  data = *frac_of_second;
  new_frac_of_second = cts_common_get_fraction_of_second (time_base);
  data = (data & 0xFF000000) | (new_frac_of_second & 0x00FFFFFF);
  *frac_of_second = data;
}
