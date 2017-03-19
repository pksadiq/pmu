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

#define SYNC_DATA 0xAA

/*
 * This will be common to every data
 * SYNC(2) + frame size (2) + id code (2) + epoch time (4) +
 * fraction of second (4) + check (2)
 */
#define DATA_COMMON_SIZE 16

/*
 * This will be common for each PMU data with freq and ROCOF as floats
 * STAT (2) + FREQ (4) + ROCOF (4)
 */
#define DATA_COMMON_SIZE_PER_PMU_FLOAT_FREQ 10

/*
 * This will be common for each PMU data with freq and ROCOF as integers
 * STAT (2) + FREQ (2) + ROCOF (2)
 */
#define DATA_COMMON_SIZE_PER_PMU_INT_FREQ 6

typedef struct _CtsPmuData
{
  uint16_t stat;

  byte freq_type;
  byte analog_type;
  // XXX: is it worth saving 3 bytes on 32 bit here some way? */
  byte phasor_type;

  /* Use Only either of the one */
  /* One for real and other for imaginary */
  uint16_t (*phasor_int)[2];

  /* The same as above, but if values are floats */
  uint32_t (*phasor_float)[2]; /* Pointer to an array of size 2 */

  union {
    uint16_t int_val;
    uint32_t float_val;
  } freq_deviation;

  union {
    uint16_t int_val;
    uint32_t float_val;
  } rocof;

  /* Use either of one. Never both */
  uint16_t *analog_int;
  uint32_t *analog_float;

  uint16_t *digital_data;
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
  pmu_data->digital_data = NULL;
}

static bool
allocate_data_memory_for_pmu (CtsPmuData *pmu_data,
                              CtsConf    *config,
                              uint16_t    pmu_index)
{
  uint16_t count;
  byte type;

  type = cts_conf_get_phasor_data_type_of_pmu (config, pmu_index);
  count = cts_conf_get_num_of_phasors_of_pmu (config, pmu_index);

  if (type == VALUE_TYPE_FLOAT)
    pmu_data->phasor_float = malloc (sizeof *pmu_data->phasor_float * count);
  else if (type == VALUE_TYPE_INT)
    pmu_data->phasor_int = malloc (sizeof *pmu_data->phasor_int * count);

  if (pmu_data->phasor_float == NULL && pmu_data->phasor_int == NULL)
    return false;

  type = cts_conf_get_analog_data_type_of_pmu (config, pmu_index);
  count = cts_conf_get_num_of_analogs_of_pmu (config, pmu_index);

  if (type == VALUE_TYPE_FLOAT)
    pmu_data->analog_float = malloc (sizeof *pmu_data->analog_float * count);
  else if (type == VALUE_TYPE_INT)
    pmu_data->analog_int = malloc (sizeof *pmu_data->analog_int * count);

  if (pmu_data->analog_float == NULL && pmu_data->analog_int == NULL)
    return false;


  count = cts_conf_get_num_of_status_of_pmu (config, pmu_index);

  pmu_data->digital_data = malloc (sizeof *pmu_data->digital_data * count);

  if (pmu_data->digital_data == NULL)
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

void
cts_data_populate_from_raw_data (CtsData  *self,
                                 byte    **data,
                                 bool      is_data_only)
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
      self->epoch_seconds = ntohl (*byte2);
      *data += 2;

      /* ID code */
      memcpy (byte2, *data, 2);
      self->epoch_seconds = ntohl (*byte2);
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
      count = cts_conf_get_num_of_phasors_of_pmu (self->config,
                                                  i + 1);
      if (pmu_data->phasor_type == VALUE_TYPE_INT)
        {
          for (uint16_t i = 0; i < count; i++)
            {
              /* Real or Magnitude */
              memcpy(byte2, *data, 2);
              *(pmu_data->phasor_int + i) [0] = ntohs (*byte2);
              *data += 2;

              /* Imaginary or Angle */
              memcpy(byte2, *data, 2);
              *(pmu_data->phasor_int + i) [1] = ntohs (*byte2);
              *data += 2;
            }
        }
      else if (pmu_data->phasor_type == VALUE_TYPE_FLOAT)
        {
          for (uint16_t i = 0; i < count; i++)
            {
              memcpy(byte4, *data, 4);
              *(pmu_data->phasor_float + i) [0] = ntohs (*byte4);
              *data += 4;

              memcpy(byte4, *data, 4);
              *(pmu_data->phasor_float + i) [1] = ntohs (*byte4);
              *data += 4;
            }
        }

      count = cts_conf_get_num_of_analogs_of_pmu (self->config,
                                                    i + 1);
      if (pmu_data->analog_type == VALUE_TYPE_INT)
        {
          for (uint16_t i = 0; i < count; i++)
            {
              memcpy(byte2, *data, 2);
              *(pmu_data->analog_int + i) = ntohs (*byte2);
              *data += 2;
            }
        }
      else if (pmu_data->analog_type == VALUE_TYPE_FLOAT)
        {
          for (uint16_t i = 0; i < count; i++)
            {
              memcpy(byte4, *data, 4);
              *(pmu_data->analog_float + i) = ntohl (*byte4);
              *data += 4;
            }
        }

      if (pmu_data->freq_type == VALUE_TYPE_INT)
        {
          memcpy(byte2, *data, 2);
          pmu_data->freq_deviation.int_val = ntohs (*byte2);
          *data += 2;
        }
      else if (pmu_data->freq_type == VALUE_TYPE_FLOAT)
        {
          memcpy(byte4, *data, 4);
          pmu_data->freq_deviation.float_val = ntohl (*byte2);
          *data += 4;
        }

      if (pmu_data->freq_type == VALUE_TYPE_INT)
        {
          memcpy(byte2, *data, 2);
          pmu_data->rocof.int_val = ntohl (*byte2);
          *data += 2;
        }
      else if (pmu_data->freq_type == VALUE_TYPE_FLOAT)
        {
          memcpy(byte4, *data, 4);
          pmu_data->rocof.float_val = ntohl (*byte2);
          *data += 4;
        }

      count = cts_conf_get_num_of_status_of_pmu (self->config,
                                                 i + 1);
      for (uint16_t i = 0; i < count; i++)
        {
          memcpy(byte2, *data, 2);
          *(pmu_data->digital_data + i) = ntohs(*byte2);
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
