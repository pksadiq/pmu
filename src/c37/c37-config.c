/* c37-config.c
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

#include "assert.h"
#include "stdio.h"
#include "c37-config.h"

/*
 * This will be common to every configuration
 * SYNC (2) + frame size (2) + id code (2) + epoch time (4) +
 * fraction of second (4) + time base (4) + data rate (2) + check (2)
 */
#define CONFIG_COMMON_SIZE 22 /* bytes */

/*
 * Station name (16) + id code (2) + format (2) + phasor count (2) +
 * analog value count (2) + digital status words count (2) +
 * Nominal line frequency (2) + conf change count (2)
 */
#define CONFIG_COMMON_SIZE_PER_PMU 30 /* bytes */

typedef struct _CtsPmuConfig
{
  char station_name[16];
  uint16_t id_code;

  /**
   * 15 - 4 bits unused
   * bit 3, 2, 1: 1 for float point (4 byte) and 0 for integer (2 byte)
   * bit 3: frequency (deviation from real frequency) or dfreq (ROCOF)
   * bit 2: analog value
   * bit 1: phasors
   * bit 0: for phasors, set 1 for real + imaginary (rectangular). 1 for
   * magnitude and angle (polar)
   */
  uint16_t data_format;

  /**
   * Total number of values like Vr, Vr_angle, Vy, Vy_angle, Vb, Vb_angle,
   * Ir, Iy, Ib, etc.
   */
  uint16_t num_phasors;

  uint16_t num_analog_values;

  /**
   * Human readable name of phasors, and analog (eg: "Vy Angle", "Vr Angle")
   * Fill with spaces (0x20) at the freespace until the size is 16
   * Like "Vy angle        " (Shouldn't end with '\0').
   *
   * size: 16 bytes * (num_phasors + num_analog_values): +
   * 16 * 16 * num_status_words (Name for each breaker (16 breakers))
   */
  char **channel_names;

  /* 4 byte * num_phasors */
  uint32_t *conv_factor_phasor;

  /* 4 * num_anlalog_values */
  uint32_t *conv_factor_analog;

  /* 4 * num_status_words */
  uint32_t *status_word_masks;

  /**
   * total number of digital status (binary 1 for on or 0 for off) for breakers
   */
  uint16_t num_status_words;

  uint16_t nominal_freq;
  uint16_t conf_change_count;

} CtsPmuConfig;

typedef struct _CtsConfig
{
  uint16_t id_code;
  uint16_t num_pmu;

  uint32_t time_base;

  /* One per PMU */
  CtsPmuConfig *pmu_config;

  /* Rate of data transmissions */
  uint16_t data_rate;
} CtsConfig;

CtsConfig *config_default_one = NULL;
CtsConfig *config_default_two = NULL;

uint16_t
cts_config_get_id_code (CtsConfig *self)
{
  return self->id_code;
}

void
cts_config_set_id_code (CtsConfig *self,
                        uint16_t   id_code)
{
  self->id_code = id_code;
}

static void
pmu_config_clear_all_data (CtsPmuConfig *config)
{
 config->id_code = 0;
 config->data_format = 0;
 config->num_phasors = 0;
 config->num_analog_values = 0;
 config->num_status_words = 0;
 config->channel_names = NULL;
 config->conv_factor_phasor = NULL;
 config->conv_factor_analog = NULL;
 config->status_word_masks = NULL;
 config->nominal_freq = 50; /* Assume 50 Hz by default */
 config->conf_change_count = 0;
}

uint16_t
cts_config_get_pmu_count (CtsConfig *self)
{
  return self->num_pmu;
}

/**
 * Note: This is a very costly function.
 * Please use only once if possible
 */
uint16_t
cts_config_set_pmu_count (CtsConfig *self,
                          uint16_t   count)
{
  CtsPmuConfig *pmu_config = NULL;

  if (self->num_pmu && self->num_pmu == count)
    return count;

  pmu_config = realloc (self->pmu_config,
                        sizeof (*pmu_config) * count);

  if (pmu_config)
    {
      self->pmu_config = pmu_config;

      for (uint16_t i = self->num_pmu; i < count; i++)
        pmu_config_clear_all_data (self->pmu_config + i);

      self->num_pmu = count;
    }

  return self->pmu_config ? self->num_pmu : 0;
}

uint32_t
cts_config_get_time_base (CtsConfig *self)
{
  return self->time_base;
}

void
cts_config_set_time_base (CtsConfig *self,
                          uint32_t   time_base)
{
  self->time_base = time_base;
}

uint16_t
cts_config_get_data_rate (CtsConfig *self)
{
  return self->data_rate;
}

void
cts_config_set_data_rate (CtsConfig *self,
                          uint16_t   data_rate)
{
  self->data_rate = data_rate;
}

char *
cts_config_get_station_name_of_pmu (CtsConfig *self,
                                    uint16_t   pmu_index)
{
  return (self->pmu_config + pmu_index - 1)->station_name;
}

bool
cts_config_set_station_name_of_pmu (CtsConfig  *self,
                                    uint16_t    pmu_index,
                                    const char *station_name,
                                    size_t      name_size)
{
  if (pmu_index > self->num_pmu)
    return false;

  if (name_size > 16)
    name_size = 16;

  memcpy ((self->pmu_config + pmu_index - 1)->station_name,
          station_name, name_size);

  /* Fix for of by one error when used as array index */
  name_size--;

  /* Append spaces to the rest of data, if any */
  while (++name_size < 16)
    (self->pmu_config + pmu_index - 1)->station_name[name_size] = ' ';

  return true;
}

uint16_t
cts_config_get_id_code_of_pmu (CtsConfig *self,
                               uint16_t   pmu_index)
{
  if (pmu_index > self->num_pmu)
    return 0;

  return (self->pmu_config + pmu_index - 1)->id_code;
}

bool
cts_config_set_id_code_of_pmu (CtsConfig *self,
                               uint16_t   pmu_index,
                               uint16_t   id_code)
{
  if (pmu_index > self->num_pmu)
    return false;

  (self->pmu_config + pmu_index - 1)->id_code = id_code;
  return true;
}

bool
cts_config_get_freq_data_type_of_pmu (CtsConfig *self,
                                      uint16_t   pmu_index)
{
  if (pmu_index > self->num_pmu)
    return false;

  CtsPmuConfig *config = self->pmu_config + pmu_index - 1;

  return BIT_IS_SET (config->data_format, 3);
}

void
cts_config_set_freq_data_type_of_pmu (CtsConfig *self,
                                      uint16_t   pmu_index,
                                      bool       data_type)
{
  if (pmu_index > self->num_pmu)
    return;

  CtsPmuConfig *config = self->pmu_config + pmu_index - 1;

  if (data_type)
    SET_BIT (config->data_format, 3);
  else
    CLEAR_BIT (config->data_format, 3);
}

bool
cts_config_get_analog_data_type_of_pmu (CtsConfig *self,
                                        uint16_t   pmu_index)
{
  if (pmu_index > self->num_pmu)
    return false;

  CtsPmuConfig *config = self->pmu_config + pmu_index - 1;

  return BIT_IS_SET (config->data_format, 2);
}

void
cts_config_set_analog_data_type_of_pmu (CtsConfig *self,
                                        uint16_t   pmu_index,
                                        bool       data_type)
{
  if (pmu_index > self->num_pmu)
    return;

  CtsPmuConfig *config = self->pmu_config + pmu_index - 1;

  if (data_type)
    SET_BIT (config->data_format, 2);
  else
    CLEAR_BIT (config->data_format, 2);
}

bool
cts_config_get_phasor_data_type_of_pmu (CtsConfig *self,
                                        uint16_t   pmu_index)
{
  if (pmu_index > self->num_pmu)
    return false;

  CtsPmuConfig *config = self->pmu_config + pmu_index - 1;

  return BIT_IS_SET (config->data_format, 1);
}

void
cts_config_set_phasor_data_type_of_pmu (CtsConfig *self,
                                        uint16_t   pmu_index,
                                        bool       data_type)
{
  if (pmu_index > self->num_pmu)
    return;

  CtsPmuConfig *config = self->pmu_config + pmu_index - 1;

  if (data_type)
    SET_BIT (config->data_format, 1);
  else
    CLEAR_BIT (config->data_format, 1);
}

bool
cts_config_get_phasor_complex_type_of_pmu (CtsConfig *self,
                                           uint16_t   pmu_index)
{
  if (pmu_index > self->num_pmu)
    return false;

  CtsPmuConfig *config = self->pmu_config + pmu_index - 1;

  return BIT_IS_SET (config->data_format, 0);
}

void
cts_config_set_phasor_complex_type_of_pmu (CtsConfig *self,
                                           uint16_t   pmu_index,
                                           bool       data_type)
{
  if (pmu_index > self->num_pmu)
    return;

  CtsPmuConfig *config = self->pmu_config + pmu_index - 1;

  if (data_type)
    SET_BIT (config->data_format, 0);
  else
    CLEAR_BIT (config->data_format, 0);
}

static bool
cts_config_set_values_of_pmu (CtsConfig  *self,
                              uint32_t  **ptr,
                              uint16_t    pmu_index,
                              uint16_t    count)
{
  uint32_t *data = NULL;

  data = realloc (ptr, sizeof **ptr * count);

  if (!count)
    {
      *ptr = NULL;
      return true;
    }

  if (data)
    {
      *ptr = data;
      return true;
    }
  return false;
}

uint16_t
cts_config_get_number_of_phasors_of_pmu (CtsConfig *self,
                                         uint16_t   pmu_index)
{
  if (pmu_index > self->num_pmu)
    return 0;

  return (self->pmu_config + pmu_index - 1)->num_phasors;
}

uint16_t
cts_config_set_number_of_phasors_of_pmu (CtsConfig *self,
                                         uint16_t   pmu_index,
                                         uint16_t   count)
{
  CtsPmuConfig *config;
  bool done;

  if (pmu_index > self->num_pmu)
    return 0;

  config = self->pmu_config + pmu_index - 1;
  done = cts_config_set_values_of_pmu (self, &config->conv_factor_phasor,
                                       pmu_index, count);

  if (done)
    config->num_phasors = count;

  return config->num_phasors;
}

uint16_t
cts_config_get_number_of_analog_vals_of_pmu (CtsConfig *self,
                                             uint16_t   pmu_index)
{
  if (pmu_index > self->num_pmu)
    return 0;

  return (self->pmu_config + pmu_index - 1)->num_analog_values;
}

uint16_t
cts_config_set_number_of_analog_vals_of_pmu (CtsConfig *self,
                                             uint16_t   pmu_index,
                                             uint16_t   count)
{
  CtsPmuConfig *config;
  bool done;

  if (pmu_index > self->num_pmu)
    return 0;

  config = self->pmu_config + pmu_index - 1;
  done = cts_config_set_values_of_pmu (self, &config->conv_factor_analog,
                                       pmu_index, count);

  if (done)
    config->num_analog_values = count;

  return config->num_analog_values;
}

uint16_t
cts_config_get_number_of_status_words_of_pmu (CtsConfig *self,
                                              uint16_t   pmu_index)
{
  if (pmu_index > self->num_pmu)
    return 0;

  return (self->pmu_config + pmu_index - 1)->num_status_words;
}

uint16_t
cts_config_set_number_of_status_word_of_pmu (CtsConfig *self,
                                             uint16_t   pmu_index,
                                             uint16_t   count)
{
  CtsPmuConfig *config;
  bool done;

  if (pmu_index > self->num_pmu)
    return 0;

  config = self->pmu_config + pmu_index - 1;
  done = cts_config_set_values_of_pmu (self, &config->status_word_masks,
                                       pmu_index, count);

  if (done)
    config->num_status_words = count;

  return config->num_status_words;
}

char **
cts_config_get_channel_names_of_pmu (CtsConfig *self,
                                     uint16_t   pmu_index)
{
  if (pmu_index > self->num_pmu)
    return NULL;

  return (self->pmu_config + pmu_index - 1)->channel_names;
}

bool
cts_config_set_channel_names_of_pmu (CtsConfig  *self,
                                     uint16_t    pmu_index,
                                     char      **channel_names)
{
  CtsPmuConfig *config = NULL;

  if (pmu_index > self->num_pmu)
    return false;

  config = (self->pmu_config + pmu_index - 1);

  if (!config->num_phasors)
    return false; /* Atleast one phasor is required */

  config->channel_names = channel_names;
  return true;
}


bool
cts_config_set_conv_factor_phasor_of_pmu (CtsConfig *self,
                                          uint16_t   pmu_index,
                                          uint16_t   phasor_index,
                                          uint32_t   data)
{
  CtsPmuConfig *config;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;

  if (phasor_index > config->num_phasors)
    return false;

  *(config->conv_factor_phasor + phasor_index - 1) = data;
  return true;
}

bool
cts_config_set_all_conv_factor_phasor_of_pmu (CtsConfig *self,
                                              uint16_t   pmu_index,
                                              uint32_t   data)
{
  CtsPmuConfig *config;
  uint16_t num_phasors;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;
  num_phasors = config->num_phasors;

  for (uint16_t i = 0; i < num_phasors; i++)
    {
      bool status = cts_config_set_conv_factor_phasor_of_pmu (self, pmu_index,
                                                              i, data);
      if (!status)
        return false;
    }
  return true;
}

bool
cts_config_set_all_conv_factor_phasor_of_all_pmu (CtsConfig *self,
                                                  uint32_t   data)
{
  uint16_t num_pmu;

  num_pmu = self->num_pmu;

  for (uint16_t i = 0; i < num_pmu; i++)
    {
      bool status = cts_config_set_all_conv_factor_phasor_of_pmu (self, i,
                                                                  data);
      if (!status)
        return false;
    }
  return true;
}

bool
cts_config_set_conv_factor_analog_of_pmu (CtsConfig *self,
                                          uint16_t   pmu_index,
                                          uint16_t   analog_index,
                                          uint32_t   data)
{
  CtsPmuConfig *config;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;

  if (analog_index > config->num_analog_values)
    return false;

  *(config->conv_factor_analog + analog_index - 1) = data;
  return true;
}

bool
cts_config_set_all_conv_factor_analog_of_pmu (CtsConfig *self,
                                              uint16_t   pmu_index,
                                              uint32_t   data)
{
  CtsPmuConfig *config;
  uint16_t num_analog;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;
  num_analog = config->num_analog_values;

  for (uint16_t i = 0; i < num_analog; i++)
    {
      bool status = cts_config_set_conv_factor_analog_of_pmu (self, pmu_index,
                                                              i, data);
      if (!status)
        return false;
    }
  return true;
}

bool
cts_config_set_all_conv_factor_analog_of_all_pmu (CtsConfig *self,
                                                  uint32_t   data)
{
  uint16_t num_pmu;

  num_pmu = self->num_pmu;

  for (uint16_t i = 0; i < num_pmu; i++)
    {
      bool status = cts_config_set_all_conv_factor_analog_of_pmu (self, i,
                                                                  data);
      if (!status)
        return false;
    }
  return true;
}

bool
cts_config_set_status_word_masks_of_pmu (CtsConfig *self,
                                         uint16_t   pmu_index,
                                         uint16_t   status_index,
                                         uint32_t   data)
{
  CtsPmuConfig *config;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;

  if (status_index > config->num_status_words)
    return false;

  *(config->status_word_masks + status_index - 1) = data;
  return true;
}

bool
cts_config_set_all_status_word_masks_of_pmu (CtsConfig *self,
                                              uint16_t   pmu_index,
                                              uint32_t   data)
{
  CtsPmuConfig *config;
  uint16_t num_status;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;
  num_status = config->num_status_words;

  for (uint16_t i = 0; i < num_status; i++)
    {
      bool status = cts_config_set_status_word_masks_of_pmu (self, pmu_index,
                                                             i, data);
      if (!status)
        return false;
    }
  return true;
}

bool
cts_config_set_all_status_word_masks_of_all_pmu (CtsConfig *self,
                                                  uint32_t   data)
{
  uint16_t num_pmu;

  num_pmu = self->num_pmu;

  for (uint16_t i = 0; i < num_pmu; i++)
    {
      bool status = cts_config_set_all_status_word_masks_of_pmu (self, i,
                                                                 data);
      if (!status)
        return false;
    }
  return true;
}

bool
cts_config_set_nominal_frequency_of_pmu (CtsConfig *self,
                                         uint16_t   pmu_index,
                                         uint16_t   nominal_freq)
{
  if (pmu_index > self->num_pmu)
    return false;

  (self->pmu_config + pmu_index - 1)->nominal_freq = nominal_freq;
  return true;
}

bool
cts_config_increment_change_count_of_pmu (CtsConfig *self,
                                          uint16_t   pmu_index)
{
  if (pmu_index > self->num_pmu)
    return false;

  (self->pmu_config + pmu_index - 1)->conf_change_count++;
  return true;
}

static CtsConfig *
cts_config_new (void)
{
  CtsConfig *self = NULL;

  self = malloc (sizeof *self);

  if (self)
    {
      /* Initialize dangerous variables */
      self->num_pmu = 0;
      self->pmu_config = NULL;
    }

  return self;
}

CtsConfig *
cts_config_get_default_config_one (void)
{
  if (config_default_one == NULL)
    config_default_one = cts_config_new ();

  return config_default_one;
}

/* TODO: Don't replicate data in one */
CtsConfig *
cts_config_get_default_config_two (void)
{
  if (config_default_two == NULL)
    config_default_two = cts_config_new ();

  return config_default_two;
}

static size_t
get_per_pmu_total_size (CtsPmuConfig *pmu_config)
{
  size_t pmu_size;

  pmu_size = 16 * (pmu_config->num_phasors
                   + pmu_config->num_analog_values
                   + 16 * pmu_config->num_status_words);

  pmu_size += 4 * (pmu_config->num_phasors
                   + pmu_config->num_analog_values
                   + pmu_config->num_status_words);

  return pmu_size + CONFIG_COMMON_SIZE_PER_PMU;
}

static size_t
calc_total_size (CtsConfig *self)
{
  uint16_t num_pmu;
  size_t total_pmu_size = 0;

  num_pmu = self->num_pmu;

  for (uint16_t i = 0; i < num_pmu; i++)
    total_pmu_size += get_per_pmu_total_size (self->pmu_config + i);

  return total_pmu_size + CONFIG_COMMON_SIZE;
}

static void
populate_raw_data_of_config_part1 (CtsConfig  *config,
                                   size_t      frame_size,
                                   byte      **pptr)
{
  uint16_t *byte2 = malloc (sizeof (*byte2));
  uint32_t *byte4 = malloc (sizeof (*byte4));

  *byte2 = htons (SYNC_CONFIG_ONE);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  *byte2 = htons (frame_size);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  *byte4 = htons (pmu_common_get_time_seconds());
  memcpy (*pptr, byte4, 4);
  *pptr += 4;

  /* TODO: configure Leap seconds and quality */
  *byte4 = htons (pmu_common_get_fraction_of_seconds());
  memcpy (*pptr, byte4, 4);
  *pptr += 4;

  *byte4 = htons (config->time_base);
  memcpy (*pptr, byte4, 4);
  *pptr += 4;

  free (byte2);
  free (byte4);
}

static void
populate_raw_data_of_config_part2 (CtsConfig  *config,
                                   size_t      frame_size,
                                   byte       *data,
                                   byte      **pptr)
{
  uint16_t *byte2 = malloc (sizeof (*byte2));

  *byte2 = htons (config->data_rate);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  *byte2 = htons (pmu_common_get_crc(data, frame_size - 1));
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  free (byte2);
}

static void
populate_raw_data_of_pmu_part1 (CtsPmuConfig  *config,
                                byte         **pptr)
{
  uint16_t *byte2 = malloc (sizeof (*byte2));

  memcpy (*pptr, config->station_name, 16);
  *pptr += 16;

  *byte2 = htons (config->id_code);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  *byte2 = htons (config->data_format);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  *byte2 = htons (config->num_phasors);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  *byte2 = htons (config->num_analog_values);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  *byte2 = htons (config->num_status_words);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  free (byte2);
}

static void
copy_pmu_channel_names (char **channel_names,
                        byte **pptr)
{
  if (channel_names == NULL)
    return;

  while (*channel_names)
    {
      memcpy (*pptr, *channel_names, 16);
      *pptr += 16;
      channel_names++;
    }
}

static void
populate_raw_data_of_pmu_part2 (CtsPmuConfig  *config,
                                byte         **pptr)
{
  uint16_t *byte2 = malloc (sizeof (*byte2));
  uint32_t *byte4 = malloc (sizeof (*byte4));

  for (uint16_t i = 0; i < config->num_phasors; i++)
    {
      *byte4 = htonl (*config->conv_factor_phasor);
      memcpy (*pptr, byte4, 4);
      *pptr += 4;
    }

  for (uint16_t i = 0; i < config->num_analog_values; i++)
    {
      *byte4 = htonl (*config->conv_factor_analog);
      memcpy (*pptr, byte4, 4);
      *pptr += 4;
    }

  for (uint16_t i = 0; i < config->num_status_words; i++)
    {
      *byte4 = htonl (*config->status_word_masks);
      memcpy (*pptr, byte4, 4);
      *pptr += 4;
    }

  *byte2 = htons (config->nominal_freq);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  *byte2 = htons (config->conf_change_count);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  free (byte2);
  free (byte4);
}

static byte *
populate_raw_data (CtsConfig *self)
{
  CtsPmuConfig *config;
  size_t len;
  byte *data;
  byte *copy;
  uint16_t num_pmu;

  len = calc_total_size (self);
  data = malloc (len);

  if (data == NULL)
    return NULL;

  copy = data;
  num_pmu = self->num_pmu;

  populate_raw_data_of_config_part1 (self, len, &copy);

  for (uint16_t i = 0; i < num_pmu; i++)
    {
      config = self->pmu_config + i;
      populate_raw_data_of_pmu_part1 (config, &copy);
      copy_pmu_channel_names (config->channel_names, &copy);
      populate_raw_data_of_pmu_part2 (config, &copy);
    }

  populate_raw_data_of_config_part2 (self, len, data, &copy);

  return data;
}

/**
 * Raw data in Big Endian order (ie, network order)
 */
byte *
cts_config_get_raw_data (CtsConfig *self)
{
  byte *data;
  if (self == NULL)
    return NULL;

  data = populate_raw_data (self);

  return data;
}
