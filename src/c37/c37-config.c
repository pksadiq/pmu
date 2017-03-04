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

#include "c37-config.h"

typedef struct _CtsConfig
{
  uint32_t time_base;
  uint16_t num_pmu;

  /* '\0' not required */
  char *station_name;
  uint16_t id_code;
  uint16_t data_format;

  uint16_t num_phasors;
  uint16_t num_analog_values;
  uint16_t num_status_words;

  /* Rate of data transmissions */
  uint16_t data_rate;

  /* 16 bytes * (num_phasors + num_analog_values) + 16 * 16 * num_status_words */
  char *channel_names;

  /* 4 * num_phasors */
  byte *conv_factor_phasor;

  /* 4 * num_anlalog_values */
  byte *conv_factor_analog;

  /* 4 * num_status_words */
  byte *status_word_masks;

  /*
   * repeat as many PMUs present
   * pmu_details[1] = Nominal line frequency code and flags
   * pmu_details[2] = Configuration change count
   * This is a pointer to an array of uint16_t
   */
  uint16_t (*pmu_details)[2];
} CtsConfig;

CtsConfig *config_default_one = NULL;
CtsConfig *config_default_two = NULL;

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
cts_config_get_pmu_count (CtsConfig *self)
{
  return self->num_pmu;
}

uint16_t
cts_config_set_pmu_count (CtsConfig *self,
                          uint16_t   count)
{
  uint16_t (*pmu_details)[2];

  if (self->num_pmu && self->num_pmu == count)
    return count;

  pmu_details = realloc (self->pmu_details,
                         sizeof *self->pmu_details * count);

  if (pmu_details)
    {
      self->num_pmu = count;
      self->pmu_details = pmu_details;
    }

  return self->pmu_details ? self->num_pmu : 0;
}

char *
cts_config_get_station_name (CtsConfig *self)
{
  return self->station_name;
}

bool
cts_config_set_station_name (CtsConfig  *self,
                             const char *station_name,
                             size_t      n)
{
  if (n > 16)
    return false;

  if (self->station_name == NULL)
    self->station_name = malloc (16);

  if (self->station_name)
    {
      memcpy (self->station_name, station_name, n);
      if (n < 16)
        self->station_name[n] = '\0';
      return true;
    }
  return false;
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
      self->num_phasors = 0;
      self->num_analog_values = 0;
      self->num_status_words = 0;
      self->station_name = NULL;
      self->channel_names = NULL;
      self->conv_factor_phasor = NULL;
      self->conv_factor_analog = NULL;
      self->status_word_masks = NULL;
      self->pmu_details = NULL;
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
  free (self->station_name);
  free (self->channel_names);
  free (self->conv_factor_phasor);
  free (self->conv_factor_analog);
  free (self->status_word_masks);
  free (self->pmu_details);
  free (self);
  self = NULL;
}
