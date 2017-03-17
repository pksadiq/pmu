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

enum ValueType {
  VALUE_TYPE_INT                  = 0x00,
  VALUE_TYPE_FLOAT                = 0x01,
  VALUE_TYPE_RECTANGULAR          = 0x00,
  VALUE_TYPE_POLAR                = 0x01,
  VALUE_TYPE_VOLTAGE              = 0x00,
  VALUE_TYPE_CURRENT              = 0x01,
  VALUE_TYPE_SINGLE_POINT_ON_WAVE = 0x00,
  VALUE_TYPE_RMS                  = 0x01,
  VALUE_TYPE_PEAK                 = 0x02,
  VALUE_TYPE_INVALID              = 0xFF
};

uint16_t cts_config_get_id_code (CtsConfig *self);
void     cts_config_set_id_code (CtsConfig *self,
                                 uint16_t   id_code);

uint16_t   cts_config_get_pmu_count (CtsConfig  *self);
uint16_t   cts_config_set_pmu_count (CtsConfig  *self,
                                     uint16_t    count);

uint32_t   cts_config_get_time_base (CtsConfig  *self);
void       cts_config_set_time_base (CtsConfig  *self,
                                     uint32_t    time_base);

uint16_t cts_config_get_data_rate (CtsConfig *self);
void     cts_config_set_data_rate (CtsConfig *self,
                                   uint16_t   data_rate);

char *cts_config_get_station_name_of_pmu (CtsConfig *self,
                                          uint16_t   pmu_index);
bool cts_config_set_station_name_of_pmu  (CtsConfig  *self,
                                          uint16_t    pmu_index,
                                          const char *station_name,
                                          size_t      name_size);


uint16_t cts_config_get_id_code_of_pmu (CtsConfig *self,
                                        uint16_t   pmu_index);
bool     cts_config_set_id_code_of_pmu (CtsConfig *self,
                                        uint16_t   pmu_index,
                                        uint16_t   id_code);

byte cts_config_get_freq_data_type_of_pmu (CtsConfig *self,
                                           uint16_t   pmu_index);
void cts_config_set_freq_data_type_of_pmu (CtsConfig *self,
                                           uint16_t   pmu_index,
                                           byte       data_type);

byte cts_config_get_analog_data_type_of_pmu (CtsConfig *self,
                                             uint16_t   pmu_index);
void cts_config_set_analog_data_type_of_pmu (CtsConfig *self,
                                             uint16_t   pmu_index,
                                             bool       data_type);

byte cts_config_get_phasor_data_type_of_pmu (CtsConfig *self,
                                             uint16_t   pmu_index);
void cts_config_set_phasor_data_type_of_pmu (CtsConfig *self,
                                             uint16_t   pmu_index,
                                             byte       data_type);

byte cts_config_get_phasor_complex_type_of_pmu (CtsConfig *self,
                                                uint16_t   pmu_index);
void cts_config_set_phasor_complex_type_of_pmu (CtsConfig *self,
                                                uint16_t   pmu_index,
                                                bool       is_polar);

uint16_t cts_config_get_number_of_phasors_of_pmu (CtsConfig *self,
                                                  uint16_t   pmu_index);
uint16_t cts_config_set_number_of_phasors_of_pmu (CtsConfig *self,
                                                  uint16_t   pmu_index,
                                                  uint16_t   count);

uint16_t cts_config_get_number_of_analog_vals_of_pmu (CtsConfig *self,
                                                      uint16_t   pmu_index);
uint16_t cts_config_set_number_of_analog_vals_of_pmu (CtsConfig *self,
                                                      uint16_t   pmu_index,
                                                      uint16_t   count);

uint16_t cts_config_get_number_of_status_words_of_pmu (CtsConfig *self,
                                                       uint16_t   pmu_index);
uint16_t cts_config_set_number_of_status_words_of_pmu (CtsConfig *self,
                                                       uint16_t   pmu_index,
                                                       uint16_t   count);

char **cts_config_get_channel_names_of_pmu (CtsConfig *self,
                                             uint16_t   pmu_index);
bool   cts_config_set_channel_names_of_pmu (CtsConfig *self,
                                            uint16_t   pmu_index,
                                            char     **channel_names);

bool cts_config_set_phasor_conv_factor_of_pmu         (CtsConfig *self,
                                                       uint16_t   pmu_index,
                                                       uint16_t   phasor_index,
                                                       uint32_t   data);
bool cts_config_set_all_phasor_conv_factor_of_pmu     (CtsConfig *self,
                                                       uint16_t   pmu_index,
                                                       uint32_t   data);
bool cts_config_set_all_phasor_conv_factor_of_all_pmu (CtsConfig *self,
                                                       uint32_t   data);

uint32_t cts_config_get_phasor_conv_factor_from_data  (uint32_t multiplier,
                                                       byte     type);
uint32_t cts_config_get_analog_conv_factor_from_data  (uint32_t multiplier,
                                                       byte     type);
uint32_t cts_config_get_digital_status_word_from_data (uint16_t upper,
                                                       uint16_t lower);

bool cts_config_set_analog_conv_factor_of_pmu         (CtsConfig *self,
                                                       uint16_t   pmu_index,
                                                       uint16_t   analog_index,
                                                       uint32_t   data);
bool cts_config_set_all_analog_conv_factor_of_pmu     (CtsConfig *self,
                                                       uint16_t   pmu_index,
                                                       uint32_t   data);
bool cts_config_set_all_analog_conv_factor_of_all_pmu (CtsConfig *self,
                                                       uint32_t   data);

bool cts_config_set_status_word_masks_of_pmu         (CtsConfig *self,
                                                      uint16_t   pmu_index,
                                                      uint16_t   status_index,
                                                      uint32_t   data);
bool cts_config_set_all_status_word_masks_of_pmu     (CtsConfig *self,
                                                      uint16_t   pmu_index,
                                                      uint32_t   data);
bool cts_config_set_all_status_word_masks_of_all_pmu (CtsConfig *self,
                                                      uint32_t   data);

bool cts_config_set_nominal_frequency_of_pmu (CtsConfig *self,
                                              uint16_t   pmu_index,
                                              uint16_t   nominal_freq);

bool cts_config_increment_change_count_of_pmu (CtsConfig *self,
                                               uint16_t   pmu_index);
bool cts_config_set_change_count_of_pmu       (CtsConfig *self,
                                               uint16_t   pmu_index,
                                               uint16_t   count);

byte      *cts_config_get_raw_data             (CtsConfig *self);
void       cts_config_free                     (CtsConfig  *self);
CtsConfig  *cts_config_get_default_config_one  (void);
CtsConfig  *cts_config_get_default_config_two  (void);
