/* c37-bin.h
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

#ifndef C37_BIN_H
#define C37_BIN_H


#include "c37-common.h"

byte     cts_bin_get_type           (const byte *data);
uint16_t cts_bin_get_frame_size     (const byte *data);
uint16_t cts_bin_get_id_code        (const byte *data,
                                     bool        skip_first);
uint32_t cts_bin_get_time_seconds   (const byte *data,
                                     bool        skip_first);
uint32_t cts_bin_get_frac_of_second (const byte *data,
                                     uint32_t    time_base,
                                     bool        skip_first);
uint16_t cts_bin_get_crc            (const byte *data,
                                     const byte *header);


uint16_t cts_bin_get_command_type (const byte *data,
                                   bool        skip_first);



#endif /* C37_BIN_H */

