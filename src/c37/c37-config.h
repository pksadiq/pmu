/* c37-config.h
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

typedef struct _CtsPmuConfig CtsPmuConfig;
typedef struct _CtsConfig CtsConfig;

#define SYNC_CONFIG_ONE 0xAA21
#define SYNC_CONFIG_TWO 0xAA31

#define NOMINAL_FREQ_50 0x01 /* Hertz */
#define NOMINAL_FREQ_60 0x00 /* Hertz */

#define TYPE_FLOAT 1
#define TYPE_INT 0

#define TYPE_POLAR 1
#define TYPE_RECTANGULAR 0

uint32_t   cts_config_get_time_base     (CtsConfig  *self);

void       cts_config_set_time_base     (CtsConfig  *self,
                                         uint32_t    time_base);

uint16_t   cts_config_get_pmu_count     (CtsConfig  *self);

uint16_t   cts_config_set_pmu_count     (CtsConfig  *self,
                                         uint16_t    count);

char      *cts_config_get_station_name  (CtsConfig  *self);

void       cts_config_set_station_name  (CtsConfig  *self,
                                         const char *station_name,
                                         size_t      n);
void       cts_config_free              (CtsConfig  *self);

CtsConfig *cts_config_get_default_config_one (void);
CtsConfig *cts_config_get_default_config_two (void);



/* Test */

byte *
cts_config_get_raw_data (CtsConfig *self);

bool
cts_config_set_station_name_of_pmu (CtsConfig  *self,
                                    uint16_t    pmu_index,
                                    const char *station_name,
                                    size_t      name_size);


