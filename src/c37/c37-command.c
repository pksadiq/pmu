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
cts_command_get_type (uint16_t command)
{
  switch (command)
    {
    case CTS_COMMAND_INVALID:
    case CTS_COMMAND_DATA_ON:
    case CTS_COMMAND_DATA_OFF:
    case CTS_COMMAND_SEND_HDR:
    case CTS_COMMAND_SEND_CONFIG1:
    case CTS_COMMAND_SEND_CONFIG2:
    case CTS_COMMAND_SEND_CONFIG3:
    case CTS_COMMAND_EXTENDED_FRAME:
      return command;
    }

  if (command & 0xF000)
    return CTS_COMMAND_RESERVED;
  if (command & 0x0F00)
    return CTS_COMMAND_USER;
  if (command & 0x00FF)
    return CTS_COMMAND_RESERVED;

  return CTS_COMMAND_INVALID;
}

