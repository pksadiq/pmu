/* c37-data.c
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

#include "c37-data.h"
#include "assert.h"
#include "stdio.h"

#define SYNC_DATA 0xAA

typedef struct _CtsPmuData
{
  uint16_t stat;

  byte freq_type;
  byte analog_type;
  // XXX: is it worth saving 3 bytes on 32 bit here some way? */
  byte phasor_type;

  uint16_t num_phasors;

  /* Use Only either of the one */
  /* One for real and other for imaginary */
  uint16_t (*phasor_int)[2];

  /* The same as above, but if values are floats */
  float (*phasor_float)[2]; /* Pointer to an array of size 2 */

  union {
    uint16_t int_val;
    float    float_val;
  } freq_deviation;

  union {
    uint16_t int_val;
    float    float_val;
  } rocof;

  uint16_t num_analogs;
  uint16_t num_status_words;

  /* Use either of one. Never both */
  uint16_t *analog_int;
  float    *analog_float;

  uint16_t *status_word;
} CtsPmuData;

typedef struct _CtsData
{
  uint16_t frame_size;
  uint16_t id_code;

  uint32_t epoch_seconds;
  uint32_t frac_of_second;

  uint16_t num_pmu;
  uint16_t check;

  CtsConf *config;

  CtsPmuData *pmu_data;
} CtsData;

CtsData *default_data = NULL;

byte
cts_pmu_data_get_freq_type (CtsPmuData *pmu_data)
{
  return pmu_data->freq_type;
}

byte
cts_pmu_data_get_analog_type (CtsPmuData *pmu_data)
{
  return pmu_data->analog_type;
}

byte
cts_pmu_data_get_phasor_type (CtsPmuData *pmu_data)
{
  return pmu_data->phasor_type;
}

bool
cts_data_get_freq_deviation_of_pmu (CtsData  *self,
                                    uint16_t  pmu_index,
                                    void     *freq_deviation)
{
  CtsPmuData *pmu_data;

  if (pmu_index > self->num_pmu)
    return false;

  pmu_data = self->pmu_data + pmu_index - 1;

  if (pmu_data->freq_type == VALUE_TYPE_FLOAT)
    {
      float *value = freq_deviation;

      *value = pmu_data->freq_deviation.float_val;

      return true;
    }
  else if (pmu_data->freq_type == VALUE_TYPE_INT)
    {
      uint16_t *value = freq_deviation;

      *value = pmu_data->freq_deviation.int_val;

      return true;
    }

  return false;
}

bool
cts_data_get_rocof_of_pmu (CtsData  *self,
                           uint16_t  pmu_index,
                           void     *rocof)
{
  CtsPmuData *pmu_data;

  if (pmu_index > self->num_pmu)
    return false;

  pmu_data = self->pmu_data + pmu_index - 1;

  if (pmu_data->freq_type == VALUE_TYPE_FLOAT)
    {
      float *value = rocof;

      *value = pmu_data->rocof.float_val;

      return true;
    }
  else if (pmu_data->freq_type == VALUE_TYPE_INT)
    {
      uint16_t *value = rocof;

      *value = pmu_data->rocof.int_val;

      return true;
    }

  return false;
}

bool
cts_data_get_phasor_value_of_pmu (CtsData  *self,
                                  uint16_t  pmu_index,
                                  uint16_t  phasor_index,
                                  void     *phasor_value)
{
  CtsPmuData *pmu_data;

  if (pmu_index > self->num_pmu)
    return false;

  pmu_data = self->pmu_data + pmu_index - 1;

  if (phasor_index > pmu_data->num_phasors)
    return false;

  if (pmu_data->phasor_type == VALUE_TYPE_FLOAT)
    {
      float *value = phasor_value;

      value[0] = (*(pmu_data->phasor_float + phasor_index - 1)) [0];
      value[1] = (*(pmu_data->phasor_float + phasor_index - 1)) [1];

      return true;
    }
  else if (pmu_data->phasor_type == VALUE_TYPE_INT)
    {
      uint16_t *value = phasor_value;

      value[0] = (*(pmu_data->phasor_int + phasor_index - 1)) [0];
      value[1] = (*(pmu_data->phasor_int + phasor_index - 1)) [1];

      return true;
    }
  return false;

}

bool
cts_data_get_analog_value_of_pmu (CtsData  *self,
                                  uint16_t  pmu_index,
                                  uint16_t  analog_index,
                                  void     *analog_value)
{
  CtsPmuData *pmu_data;

  if (pmu_index > self->num_pmu)
    return false;

  pmu_data = self->pmu_data + pmu_index - 1;

  if (analog_index > pmu_data->num_analogs)
    return false;

  if (pmu_data->analog_type == VALUE_TYPE_FLOAT)
    {
      float *value = analog_value;

      *value = *(pmu_data->analog_float + analog_index - 1);

      return true;
    }
  else if (pmu_data->analog_type == VALUE_TYPE_INT)
    {
      uint16_t *value = analog_value;

      *value = *(pmu_data->analog_int + analog_index - 1);
      return true;
    }
  return false;
}

bool
cts_data_get_status_word_of_pmu (CtsData  *self,
                                 uint16_t  pmu_index,
                                 uint16_t  status_word_index,
                                 uint16_t *status_word)
{
  CtsPmuData *pmu_data;

  if (pmu_index > self->num_pmu)
    return false;

  pmu_data = self->pmu_data + pmu_index - 1;

  if (status_word_index > pmu_data->num_status_words)
    return false;

  *status_word = *(pmu_data->status_word + status_word_index - 1);

  return true;
}

static uint16_t
get_per_pmu_total_size (CtsData    *self,
                        CtsPmuData *pmu_data,
                        uint16_t    pmu_index)
{
  uint16_t pmu_size;
  byte size;

  pmu_size = DATA_COMMON_SIZE_PER_PMU;

  if (cts_pmu_data_get_freq_type (pmu_data) == VALUE_TYPE_FLOAT)
    size = 4;
  else
    size = 2;

  /* Frequency and ROCOF */
  pmu_size += size * 2;


  if (cts_pmu_data_get_analog_type (pmu_data) == VALUE_TYPE_FLOAT)
    size = 4;
  else
    size = 2;

  pmu_size += size * pmu_data->num_analogs;


  if (cts_pmu_data_get_phasor_type (pmu_data) == VALUE_TYPE_FLOAT)
    size = 8;
  else
    size = 4;

  pmu_size += size * pmu_data->num_phasors;

  /* Digital Status words */
  pmu_size += 2 * pmu_data->num_status_words;

  return pmu_size;
}

uint8_t
cts_data_get_data_size_of_pmu (CtsData  *self,
                               uint16_t  pmu_index)
{
  uint8_t data_size;

  data_size = get_per_pmu_total_size (self, self->pmu_data + pmu_index - 1,
                                      pmu_index);

  return data_size + DATA_COMMON_SIZE;
}

uint8_t
cts_pmu_data_get_default_data_size (uint16_t pmu_index)
{
  CtsData *cts_data;
  uint8_t data_size;

  cts_data = cts_data_get_default ();
  data_size = get_per_pmu_total_size (cts_data,
                                      cts_data->pmu_data + pmu_index - 1,
                                      pmu_index);

  return data_size + DATA_COMMON_SIZE;
}

static uint16_t
calc_total_size (CtsData *self)
{
  uint16_t num_pmu;
  size_t total_pmu_size = 0;

  num_pmu = self->num_pmu;

  for (uint16_t i = 0; i < num_pmu; i++)
    total_pmu_size += get_per_pmu_total_size (self,
                                              self->pmu_data + i,
                                              i + 1);

  return total_pmu_size + DATA_COMMON_SIZE;
}

void
cts_data_update_frame_size (CtsData *self)
{
  self->frame_size = calc_total_size (self);
  printf ("Total size: %d\n", self->frame_size);
}

uint16_t
cts_data_get_frame_size (CtsData *self)
{
  return self->frame_size;
}

static CtsData *
cts_data_new (void)
{
  CtsData *self = NULL;

  self = malloc (sizeof *self);

  if (self)
    {
      self->epoch_seconds = 0;
      self->frac_of_second = 0;
      self->num_pmu = 0;
      self->pmu_data = NULL;
      self->config = NULL;
    }

  return self;
}

CtsData *
cts_data_get_default (void)
{
  if (default_data == NULL)
    default_data = cts_data_new ();

  return default_data;
}

static void
clear_all_data (CtsPmuData *pmu_data)
{
  pmu_data->stat = 0;
  pmu_data->phasor_int = NULL;
  pmu_data->phasor_float = NULL;
  pmu_data->freq_deviation.int_val = 0;
  pmu_data->rocof.int_val = 0;
  pmu_data->analog_int = NULL;
  pmu_data->analog_float = NULL;
  pmu_data->status_word = NULL;
  pmu_data->num_analogs = 0;
  pmu_data->num_phasors = 0;
  pmu_data->num_status_words = 0;
}

static bool
allocate_data_memory_for_pmu (CtsPmuData *pmu_data,
                              CtsConf    *config,
                              uint16_t    pmu_index)
{
  if (pmu_data->phasor_type == VALUE_TYPE_FLOAT)
    pmu_data->phasor_float = malloc (sizeof *pmu_data->phasor_float *
                                     pmu_data->num_phasors);
  else if (pmu_data->phasor_type == VALUE_TYPE_INT)
    pmu_data->phasor_int = malloc (sizeof *pmu_data->phasor_int *
                                   pmu_data->num_phasors);

  if (pmu_data->phasor_float == NULL && pmu_data->phasor_int == NULL)
    return false;


  if (pmu_data->analog_type == VALUE_TYPE_FLOAT)
    pmu_data->analog_float = malloc (sizeof *pmu_data->analog_float *
                                     pmu_data->num_analogs);
  else if (pmu_data->analog_type == VALUE_TYPE_INT)
    pmu_data->analog_int = malloc (sizeof *pmu_data->analog_int *
                                   pmu_data->num_analogs);

  if (pmu_data->analog_float == NULL && pmu_data->analog_int == NULL)
    return false;


  pmu_data->status_word = malloc (sizeof *pmu_data->status_word *
                                  pmu_data->num_status_words);

  if (pmu_data->status_word == NULL)
    return false;

  return true;
}

static void
set_config_of_pmu (CtsPmuData *pmu_data,
                   CtsConf    *config,
                   uint16_t    pmu_index)
{
  pmu_data->phasor_type = cts_conf_get_phasor_data_type_of_pmu (config,
                                                                pmu_index);

  pmu_data->analog_type = cts_conf_get_analog_data_type_of_pmu (config,
                                                                pmu_index);

  pmu_data->freq_type = cts_conf_get_freq_data_type_of_pmu (config,
                                                            pmu_index);

  pmu_data->num_analogs = cts_conf_get_num_of_analogs_of_pmu (config,
                                                              pmu_index);

  pmu_data->num_phasors = cts_conf_get_num_of_phasors_of_pmu (config,
                                                              pmu_index);

  pmu_data->num_status_words = cts_conf_get_num_of_status_of_pmu (config,
                                                                  pmu_index);
}

bool
cts_data_set_config (CtsData *self,
                     CtsConf *config)
{
  uint16_t num_pmu;

  num_pmu = cts_conf_get_num_of_pmu (config);
  if (num_pmu)
    self->pmu_data = malloc (sizeof *self->pmu_data * num_pmu);

  if (self->pmu_data == NULL)
    return false;

  self->num_pmu = num_pmu;
  self->config = config;

  for (uint16_t i = 0; i < num_pmu; i++)
    {
      bool success;

      clear_all_data (self->pmu_data + i);
      set_config_of_pmu (self->pmu_data + i, config, i + 1);
      success = allocate_data_memory_for_pmu (self->pmu_data + i,
                                              config, i + 1);
      if (!success)
        return false;
    }

  return true;
}

CtsConf *
cts_data_get_conf (CtsData *self)
{
  return self->config;
}

void
cts_data_populate_from_raw_data (CtsData     *self,
                                 const byte **data,
                                 bool         is_data_only)
{
  uint16_t count;
  uint16_t *byte2;
  uint32_t *byte4;

  byte2 = malloc (sizeof *byte2);
  byte4 = malloc (sizeof *byte4);

  if (!is_data_only)
    {
      /* Frame size */
      memcpy (byte2, *data, 2);
      self->frame_size = ntohs (*byte2);
      *data += 2;

      /* ID code */
      memcpy (byte2, *data, 2);
      self->id_code = ntohs (*byte2);
      *data += 2;

      /* time (in seconds since epoch) */
      memcpy (byte4, *data, 4);
      self->epoch_seconds = ntohl (*byte4);
      *data += 4;

      /* Fraction of seconds */
      memcpy (byte4, *data, 4);
      self->frac_of_second = ntohl (*byte4);
      *data += 4;
    }

  for (int16_t i = 0; i < self->num_pmu; i++)
    {
      CtsPmuData *pmu_data = self->pmu_data + i;

      if (!is_data_only)
        {
          /* Status flags */
          memcpy (byte2, *data, 2);
          pmu_data->stat = ntohs (*byte2);
          *data += 2;
        }

      /* Phasors */
      count = pmu_data->num_phasors;

      if (pmu_data->phasor_type == VALUE_TYPE_INT)
        {
          for (uint16_t i = 0; i < count; i++)
            {
              /* Real or Magnitude */
              memcpy (byte2, *data, 2);
              *(pmu_data->phasor_int + i) [0] = ntohs (*byte2);
              *data += 2;

              /* Imaginary or Angle */
              memcpy (byte2, *data, 2);
              *(pmu_data->phasor_int + i) [1] = ntohs (*byte2);
              *data += 2;
            }
        }
      else if (pmu_data->phasor_type == VALUE_TYPE_FLOAT)
        {
          for (uint16_t i = 0; i < count; i++)
            {
              /* Real or Magnitude */
              memcpy (byte4, *data, 4);
              *(pmu_data->phasor_float + i) [0] = ntohl (*byte4);
              *data += 4;

              /* Imaginary or Angle */
              memcpy (byte4, *data, 4);
              *(pmu_data->phasor_float + i) [1] = ntohl (*byte4);
              *data += 4;
            }
        }

      /* Analog values */
      count = pmu_data->num_analogs;

      if (pmu_data->analog_type == VALUE_TYPE_INT)
        {
          for (uint16_t i = 0; i < count; i++)
            {
              memcpy (byte2, *data, 2);
              *(pmu_data->analog_int + i) = ntohs (*byte2);
              *data += 2;
            }
        }
      else if (pmu_data->analog_type == VALUE_TYPE_FLOAT)
        {
          for (uint16_t i = 0; i < count; i++)
            {
              memcpy (byte4, *data, 4);
              *(pmu_data->analog_float + i) = ntohl (*byte4);
              *data += 4;
            }
        }

      /* Frequency Deviation */
      if (pmu_data->freq_type == VALUE_TYPE_INT)
        {
          memcpy (byte2, *data, 2);
          pmu_data->freq_deviation.int_val = ntohs (*byte2);
          *data += 2;
        }
      else if (pmu_data->freq_type == VALUE_TYPE_FLOAT)
        {
          memcpy (byte4, *data, 4);
          pmu_data->freq_deviation.float_val = ntohl (*byte4);
          *data += 4;
        }

      /* ROCOF */
      if (pmu_data->freq_type == VALUE_TYPE_INT)
        {
          memcpy (byte2, *data, 2);
          pmu_data->rocof.int_val = ntohs (*byte2);
          *data += 2;
        }
      else if (pmu_data->freq_type == VALUE_TYPE_FLOAT)
        {
          memcpy (byte4, *data, 4);
          pmu_data->rocof.float_val = ntohl (*byte4);
          *data += 4;
        }

      /* Digital Status words */
      count = pmu_data->num_status_words;
      for (uint16_t i = 0; i < count; i++)
        {
          memcpy (byte2, *data, 2);
          *(pmu_data->status_word + i) = ntohs (*byte2);
          *data += 2;
        }
    }

  if (!is_data_only)
    {
      /* Cyclic redundancy check */
      memcpy (byte2, *data, 2);
      self->check = ntohs (*byte2);
      *data += 2;
    }

  free (byte2);
  free (byte4);
}
