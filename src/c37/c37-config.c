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
      self->num_pmu = count;
      self->pmu_config = pmu_config;
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
                                    size_t     pmu_index)
{
  return (self->pmu_config + pmu_index - 1)->station_name;
}

bool
cts_config_set_station_name_of_pmu (CtsConfig  *self,
                                    size_t      pmu_index,
                                    const char *station_name,
                                    size_t      name_size)
{
  if (pmu_index > self->num_pmu);
    return false;

  if (name_size > 16)
    name_size = 16;

  memcpy ((self->pmu_config + pmu_index - 1)->station_name,
          station_name, name_size);

  /* Append spaces to the rest of data, if any */
  while (++name_size <= 16)
    (self->pmu_config + pmu_index - 1)->station_name[name_size] = ' ';

  return true;
}

uint16_t
cts_config_get_id_code_of_pmu (CtsConfig *self,
                               size_t     pmu_index)
{
  if (pmu_index > self->num_pmu)
    return 0;

  return (self->pmu_config + pmu_index - 1)->id_code;
}

bool
cts_config_set_id_code_of_pmu (CtsConfig *self,
                               size_t     pmu_index,
                               uint16_t   id_code)
{
  if (pmu_index > self->num_pmu)
    return false;

  (self->pmu_config + pmu_index - 1)->id_code = id_code;
  return true;
}

uint16_t
cts_config_get_data_format_of_pmu (CtsConfig *self,
                                   size_t     pmu_index)
{
  if (pmu_index > self->num_pmu)
    return 0;

  return (self->pmu_config + pmu_index - 1)->data_format;
}

bool
cts_config_set_data_format_of_pmu (CtsConfig *self,
                               size_t     pmu_index,
                               uint16_t   data_format)
{
  if (pmu_index > self->num_pmu)
    return false;

  (self->pmu_config + pmu_index - 1)->data_format = data_format;
  return true;
}

uint16_t
cts_config_get_number_of_phasors_of_pmu (CtsConfig *self,
                                         size_t     pmu_index)
{
  if (pmu_index > self->num_pmu)
    return 0;

  return (self->pmu_config + pmu_index - 1)->num_phasors;
}

bool
cts_config_set_number_of_phasors_of_pmu (CtsConfig *self,
                                         size_t     pmu_index,
                                         uint16_t   count)
{
  if (pmu_index > self->num_pmu)
    return false;

  (self->pmu_config + pmu_index - 1)->num_phasors = count;
  return true;
}

uint16_t
cts_config_get_number_of_analog_vals_of_pmu (CtsConfig *self,
                                             size_t     pmu_index)
{
  if (pmu_index > self->num_pmu)
    return 0;

  return (self->pmu_config + pmu_index - 1)->num_analog_values;
}

bool
cts_config_set_number_of_analog_vals_of_pmu (CtsConfig *self,
                                             size_t     pmu_index,
                                             uint16_t   count)
{
  if (pmu_index > self->num_pmu)
    return false;

  (self->pmu_config + pmu_index - 1)->num_analog_values = count;
  return true;
}

uint16_t
cts_config_get_number_of_status_words_of_pmu (CtsConfig *self,
                                              size_t     pmu_index)
{
  if (pmu_index > self->num_pmu)
    return 0;

  return (self->pmu_config + pmu_index - 1)->num_status_words;
}

bool
cts_config_set_no_of_status_word_of_pmu (CtsConfig *self,
                                         size_t     pmu_index,
                                         uint16_t   count)
{
  if (pmu_index > self->num_pmu)
    return false;

  (self->pmu_config + pmu_index - 1)->num_status_words = count;
  return true;
}

char **
cts_config_get_channel_names_of_pmu (CtsConfig *self,
                                     size_t     pmu_index)
{
  if (pmu_index > self->num_pmu)
    return NULL;

  return (self->pmu_config + pmu_index - 1)->channel_names;
}

bool
cts_config_set_channel_names_of_pmu (CtsConfig  *self,
                                     size_t      pmu_index,
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

void
cts_config_free (CtsConfig *self)
{
  free (self);
  self = NULL;
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
  size_t total_pmu_size;

  total_pmu_size = get_per_pmu_total_size(self->pmu_config) * self->num_pmu;

  return total_pmu_size + CONFIG_COMMON_SIZE;
}

static byte *
populate_raw_data (CtsConfig *self)
{

}

/**
 * Raw data in Big Endian order (ie, network order)
 * This function supports only one Little Endian architectures
 */
byte *
cts_config_get_raw_data (CtsConfig *self)
{
  size_t size;
  byte *data = NULL;

  size = calc_total_size (self);
  data = malloc (size);

  if (data == NULL)
    return NULL;

  populate_raw_data (self);

  return data;
}
