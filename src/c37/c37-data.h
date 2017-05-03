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

#ifndef C37_DATA_H
#define C37_DATA_H


#include "c37-common.h"
#include "c37-conf.h"

/*
 * This will be common to every data
 * SYNC(2) + frame size (2) + id code (2) + epoch time (4) +
 * fraction of second (4) + check (2)
 */
#define DATA_COMMON_SIZE 16

/*
 * This will be common for each PMU data
 * STAT (2)
 */
#define DATA_COMMON_SIZE_PER_PMU 2

#define SYNC_DATA 0xAA01

#define DATA_FRAME_COMMON_SIZE (DATA_COMMON_SIZE + DATA_COMMON_SIZE_PER_PMU)

typedef struct _CtsData CtsData;
typedef struct _CtsPmuData PmuData;

CtsData *cts_data_get_default (void);
bool     cts_data_set_config  (CtsData *self,
                               CtsConf *config);

void     cts_data_update_frame_size (CtsData *self);
uint16_t cts_data_get_frame_size    (CtsData *self);

uint16_t cts_data_get_data_size_of_pmu      (CtsData  *self,
                                             uint16_t  pmu_index);
uint16_t cts_pmu_data_get_default_data_size (uint16_t pmu_index);

void cts_data_populate_from_raw_data (CtsData    *self,
                                      const byte *data,
                                      bool        is_data_only);
void cts_data_update_raw_data        (CtsData *self,
                                      byte    *data);

bool cts_data_get_rocof_of_pmu        (CtsData  *self,
                                       uint16_t  pmu_index,
                                       void     *rocof);
bool cts_data_get_phasor_value_of_pmu (CtsData  *self,
                                       uint16_t  pmu_index,
                                       uint16_t  phasor_index,
                                       void     *phasor_value);
bool cts_data_get_analog_value_of_pmu (CtsData  *self,
                                       uint16_t  pmu_index,
                                       uint16_t  analog_index,
                                       void     *analog_value);

CtsConf *cts_data_get_conf (CtsData *self);


#endif /* C37_DATA_H */
