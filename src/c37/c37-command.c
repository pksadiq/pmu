/* c37-command.c
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


uint16_t
cts_command_get_type (const byte *command,
                      uint16_t    offset)
{
  uint16_t command_type;

  if (command == NULL)
    return CTS_COMMAND_INVALID;

  /* Eg: if command frame size is 20 bytes including SYNC and FRAME size,
   * offset should be 4.
   */
  const byte *offset_data = command + offset;
  memcpy (&command_type, offset_data, 2);

  command_type = ntohs (command_type);

  switch (command_type)
    {
    case CTS_COMMAND_DATA_ON:
    case CTS_COMMAND_DATA_OFF:
    case CTS_COMMAND_SEND_HDR:
    case CTS_COMMAND_SEND_CONFIG1:
    case CTS_COMMAND_SEND_CONFIG2:
    case CTS_COMMAND_SEND_CONFIG3:
    case CTS_COMMAND_EXTENDED_FRAME:
      return command_type;
    }

  /* These are reserved for user's custom commands */
  if ((command_type & 0x0F00) == 0xF00)
    return CTS_COMMAND_USER;

  return CTS_COMMAND_INVALID;
}

