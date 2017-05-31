/* c37-header.h
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

#ifndef C37_HEADER_H
#define C37_HEADER_H

#include "c37-common.h"

#include "c37-conf.h"

/* This will be common to every header
 * SYNC (2) + frame size (2) + id code (2) + SOC time (4) +
 * fraction of second (4) + crc check (2)
 */
#define HEADER_COMMON_SIZE 16

#define SYNC_HEADER 0xAA11

byte *cts_header_get_bin (CtsConf    *conf,
                          const char *name);

#endif /* C37_HEADER_H */

