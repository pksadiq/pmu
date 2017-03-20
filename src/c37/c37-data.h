/* c37-data.h
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
#include "c37-conf.h"

typedef struct _CtsData CtsData;
typedef struct _CtsPmuData PmuData;

CtsData *cts_data_get_default (void);
bool     cts_data_set_config  (CtsData *self,
                               CtsConf *config);

void     cts_data_update_frame_size (CtsData *self);
uint16_t cts_data_get_frame_size    (CtsData *self);
