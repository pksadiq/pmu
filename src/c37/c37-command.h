/* c37-command.h
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

#include "c37-common.h"

enum CtsCommand {
  CTS_COMMAND_INVALID,
  CTS_COMMAND_DATA_OFF       = 0x01,
  CTS_COMMAND_DATA_ON        = 0x02,
  CTS_COMMAND_SEND_HDR       = 0x03,
  CTS_COMMAND_SEND_CONFIG1   = 0x04,
  CTS_COMMAND_SEND_CONFIG2   = 0x05,
  CTS_COMMAND_SEND_CONFIG3   = 0x06,
  CTS_COMMAND_EXTENDED_FRAME = 0x08,
  CTS_COMMAND_USER           = 0xFF,
};


uint16_t
cts_command_get_type (const byte *command,
                      uint16_t    offset);
