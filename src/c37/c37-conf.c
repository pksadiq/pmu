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
#include "c37-conf.h"

#define FREQUENCY_DATA_TYPE_BIT 3
#define ANALOG_DATA_TYPE_BIT 2
#define PHASOR_DATA_TYPE_BIT 1
#define PHASOR_COMPLEX_TYPE_BIT 0
/*
 * This will be common to every configuration
 * SYNC (2) + frame size (2) + id code (2) + epoch time (4) +
 * fraction of second (4) + time base (4) + pmu num (2) + data rate (2) +
 * check (2)
 */
#define CONFIG_COMMON_SIZE 24 /* bytes */

/*
 * Station name (16) + id code (2) + format (2) + phasor count (2) +
 * analog value count (2) + digital status words count (2) +
 * Nominal line frequency (2) + conf change count (2)
 */
#define CONFIG_COMMON_SIZE_PER_PMU 30 /* bytes */

typedef struct _CtsPmuConf
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

} CtsPmuConf;

typedef struct _CtsConf
{
  uint16_t id_code;
  uint16_t num_pmu;

  uint16_t frame_size;

  /* Rate of data transmissions */
  int16_t data_rate;

  uint32_t time_base;
  uint32_t epoch_seconds;
  uint32_t frac_of_second;

  /* One per PMU */
  CtsPmuConf *pmu_config;

} CtsConf;

CtsConf *config_default_one = NULL;
CtsConf *config_default_two = NULL;

/**
 * cts_conf_get_id_code:
 * @self: A valid configuration
 *
 * Returns the ID code of the PMU/PDC.
 *
 * Returns: a value b/w 1 and 65535 including those. 0 signals an error
 * or it hints that the configuration is not yet complete.
 */
uint16_t
cts_conf_get_id_code (CtsConf *self)
{
  return self->id_code;
}

/**
 * cts_conf_set_id_code:
 * @self: A valid configuration
 * @id_code: A 2 byte integer between (and including) 1 and 65535
 */
void
cts_conf_set_id_code (CtsConf  *self,
                      uint16_t  id_code)
{
  self->id_code = id_code;
}

static void
pmu_config_clear_all_data (CtsPmuConf *config)
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
  config->nominal_freq = 1; /* Assume 50 Hz by default */
  config->conf_change_count = 0;
}

/**
 * cts_conf_get_num_of_pmu:
 * @self: A valid configuration
 *
 * Returns the total number of PMU connected.
 * This should be always 1 if this code is being
 * run on PMU.
 *
 * Returns: a value between 1 and 65535 including those. 0
 * hints a non fully configured state (if the code is being run
 * on PMU) or no PMU is connected (if the code is being run on
 * a PDC)
 */
uint16_t
cts_conf_get_num_of_pmu (CtsConf *self)
{
  return self->num_pmu;
}

/**
 * cts_conf_set_num_of_pmu:
 * @self: A valid configuration
 * @count: A 2 byte integer between (and including) 1 and 65535
 *
 * Set the number of PMUs connected. If this code is being
 * run on a PMU, @count should be 1.
 *
 * Along with setting count, this code also allocates enough memory
 * To handle @count number of PMUs on success.
 *
 * Returns: a value between 1 and 65535 including those.
 * The memory for this many PMUs shall be allocated.
 * If memory allocation succeeded, @count will be returned.
 * Else, the previous PMU count shall be returned.
 */
uint16_t
cts_conf_set_num_of_pmu (CtsConf  *self,
                         uint16_t  count)
{
  CtsPmuConf *pmu_config = NULL;

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

/**
 * cts_conf_get_time_base:
 * @self: A valid configuration
 *
 * Returns the resolution of fractional second that shall be returned
 * by cts_common_get_fraction_of_second(). See cts_conf_set_time_base()
 * for more details.
 *
 * Returns: A unsigned 32 bit integer
 */
uint32_t
cts_conf_get_time_base (CtsConf *self)
{
  return self->time_base;
}

/**
 * cts_conf_set_time_base:
 * @self: A valid configuration
 * @time_base: the time base.
 *
 * Set the resolution of fractional second that shall be returned
 * by cts_common_get_fraction_of_second().
 *
 * This @time_base shall be used to extract the right fraction of second
 * got via cts_common_get_fraction_of_second().
 *
 * Say for example, if cts_common_get_fraction_of_second() returns
 * 9000, and @time_base is 10000, this means that the real fraction
 * of second is 0.9 seconds (That is,
 * cts_common_get_fraction_of_second()/@time_base)
 *
 * And the real time will be cts_common_get_time_seconds() +
 * cts_common_get_fraction_of_second()/cts_conf_get_time_base() seconds
 * Since epoch (Jan. 1 1970, the UNIX time)
 *
 * Returns: A unsigned 32 bit integer
 */
void
cts_conf_set_time_base (CtsConf  *self,
                        uint32_t  time_base)
{
  self->time_base = time_base;
}

void
cts_conf_update_time (CtsConf *self)
{
  cts_common_set_time (&self->epoch_seconds);
  cts_common_set_frac_of_second (&self->frac_of_second, self->time_base);
}

uint32_t
cts_conf_get_time_in_seconds (CtsConf *self)
{
  return self->epoch_seconds;
}

uint32_t
cts_conf_get_fraction_of_second (CtsConf *self)
{
  return self->frac_of_second;
}

/**
 * cts_conf_get_data_rate:
 * @self: A valid configuration
 *
 * The rate of transmitted phasor data via network. Please see
 * cts_conf_set_data_rate() for more details.
 *
 * Returns: A signed 16 bit integer
 */
int16_t
cts_conf_get_data_rate (CtsConf *self)
{
  return self->data_rate;
}

/**
 * cts_conf_set_data_rate:
 * @self: A valid configuration
 * @data_rate: A signed 16 bit integer
 *
 * The rate of transmitted pashor data via network.
 * If @data_rate is > 0, @data_rate denotes the number of frames per second.
 * If @data_rate is < 0, @data_rate is negative of seconds per frame.
 *
 * Say for example, if @data_rate is 10, 10 frames will be transfered
 * every second. If @data_rate is -10, then 1 frame will be transfered
 * every 10 seconds.
 */
void
cts_conf_set_data_rate (CtsConf *self,
                        int16_t  data_rate)
{
  self->data_rate = data_rate;
}

/**
 * cts_conf_get_station_name_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the station name
 * has to be retrieved. If this code is being run on a PMU
 * This will be always 1.
 *
 * The returned array shall not be ending with '\0'. It will
 * Always be 16 bytes. And the name will be filled with
 * white space (0x20) if name is less than 16 bytes in size.
 *
 * Returns: (nullable) (transfer none): An array of 16 byte char
 */
char *
cts_conf_get_station_name_of_pmu (CtsConf  *self,
                                  uint16_t  pmu_index)
{
  return (self->pmu_config + pmu_index - 1)->station_name;
}

/**
 * cts_conf_set_station_name_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the station name
 * has to be set. If this code is being run on a PMU
 * This will be always 1.
 * @station_name: the station name to be set.
 * @name_size: the size of @station_name excluding '\0', as
 * got from functions like @strlen.
 *
 * Note: Only the first 16 bytes of @station_name will be stored,
 * even if @name_size is greater than 16.
 *
 * Returns: %true if succeeded setting name, else %false.
 */
bool
cts_conf_set_station_name_of_pmu (CtsConf    *self,
                                  uint16_t    pmu_index,
                                  const char *station_name,
                                  size_t      name_size)
{
  if (pmu_index > self->num_pmu || station_name == NULL)
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

/**
 * cts_conf_get_id_code_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the id code
 * has to be retrieved. If this code is being run on a PMU
 * This will be always 1.
 *
 *
 * Returns: a 2 byte unsigned integer. A value 0 denotes
 * an error, or the value is not set.
 */
uint16_t
cts_conf_get_id_code_of_pmu (CtsConf  *self,
                             uint16_t  pmu_index)
{
  if (pmu_index > self->num_pmu)
    return 0;

  return (self->pmu_config + pmu_index - 1)->id_code;
}

/**
 * cts_conf_set_id_code_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the id code
 * has to be set. If this code is being run on a PMU
 * This will be always 1.
 * @id_code: The ID code to set for PMU with index @pmu_index.
 * @id_code should be a 2 byte unsigned integer, and not 0.
 *
 * Returns: %true if ID code is set. %false if a PMU with index
 * @pmu_index doesn't exit.
 */
bool
cts_conf_set_id_code_of_pmu (CtsConf  *self,
                             uint16_t  pmu_index,
                             uint16_t  id_code)
{
  if (pmu_index > self->num_pmu)
    return false;

  (self->pmu_config + pmu_index - 1)->id_code = id_code;
  return true;
}

/**
 * cts_conf_get_freq_data_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the frequency/ROCOF data type
 * has to be retrieved. If this code is being run on a PMU
 * This will be always 1.
 *
 *
 * Returns: %VALUE_TYPE_FLOAT if frequency (and ROCOF) data type
 * is floating point (4 byte), or %VALUE_TYPE_INT if data type is
 * an integer (2 byte unsigned).
 * if @pmu_index is invalid, %VALUE_TYPE_INVALID is returned.
 */
byte
cts_conf_get_freq_data_type_of_pmu (CtsConf  *self,
                                    uint16_t  pmu_index)
{
  if (pmu_index > self->num_pmu)
    return VALUE_TYPE_INVALID;

  CtsPmuConf *config = self->pmu_config + pmu_index - 1;

  if (BIT_IS_SET (config->data_format, FREQUENCY_DATA_TYPE_BIT))
    return VALUE_TYPE_FLOAT;
  else
    return VALUE_TYPE_INT;
}

/**
 * cts_conf_set_freq_data_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the frequency/ROCOF
 * data type has to be set. If this code is being run on a PMU
 * This will be always 1.
 * @data_type: the data type of frequency or ROCOF to be set.
 *
 * @data_type can be %VALUE_TYPE_FLOAT for 4 byte floating
 * point, or %VALUE_TYPE_INT for 2 byte unsigned integer.
 */
void
cts_conf_set_freq_data_type_of_pmu (CtsConf  *self,
                                    uint16_t  pmu_index,
                                    byte      data_type)
{
  if (pmu_index > self->num_pmu)
    return;

  CtsPmuConf *config = self->pmu_config + pmu_index - 1;

  if (data_type == VALUE_TYPE_FLOAT)
    SET_BIT (config->data_format, FREQUENCY_DATA_TYPE_BIT);
  else
    CLEAR_BIT (config->data_format, FREQUENCY_DATA_TYPE_BIT);
}

/**
 * cts_conf_get_analog_data_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the Analog
 * has to be retrieved. If this code is being run on a PMU
 * This will be always 1.
 *
 *
 * Returns: %VALUE_TYPE_FLOAT if Analog data type
 * is floating point (4 byte), or %VALUE_TYPE_INT if data type is
 * an integer (2 byte unsigned).
 * if @pmu_index is invalid, %VALUE_TYPE_INVALID is returned.
 */
byte
cts_conf_get_analog_data_type_of_pmu (CtsConf  *self,
                                      uint16_t  pmu_index)
{
  if (pmu_index > self->num_pmu)
    return VALUE_TYPE_INVALID;

  CtsPmuConf *config = self->pmu_config + pmu_index - 1;

  if (BIT_IS_SET (config->data_format, ANALOG_DATA_TYPE_BIT))
    return VALUE_TYPE_FLOAT;
  else
    return VALUE_TYPE_INT;
}

/**
 * cts_conf_set_analog_data_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the Analog
 * data type has to be set. If this code is being run on a PMU
 * This will be always 1.
 * @data_type: the data type of Analog value
 *
 * @data_type can be %VALUE_TYPE_FLOAT for 4 byte floating
 * point, or %VALUE_TYPE_INT for 2 byte unsigned integer.
 */
void
cts_conf_set_analog_data_type_of_pmu (CtsConf  *self,
                                      uint16_t  pmu_index,
                                      bool      data_type)
{
  if (pmu_index > self->num_pmu)
    return;

  CtsPmuConf *config = self->pmu_config + pmu_index - 1;

  if (data_type == VALUE_TYPE_FLOAT)
    SET_BIT (config->data_format, ANALOG_DATA_TYPE_BIT);
  else
    CLEAR_BIT (config->data_format, ANALOG_DATA_TYPE_BIT);
}

/**
 * cts_conf_get_phasor_data_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the Phasor data type
 * has to be retrieved. If this code is being run on a PMU
 * This will be always 1.
 *
 *
 * Returns: %VALUE_TYPE_FLOAT if Phasor data type
 * is floating point (4 byte), or %VALUE_TYPE_INT if data type is
 * an integer (2 byte unsigned).
 * if @pmu_index is invalid, %VALUE_TYPE_INVALID is returned.
 */
byte
cts_conf_get_phasor_data_type_of_pmu (CtsConf  *self,
                                      uint16_t  pmu_index)
{
  if (pmu_index > self->num_pmu)
    return VALUE_TYPE_INVALID;

  CtsPmuConf *config = self->pmu_config + pmu_index - 1;

  if (BIT_IS_SET (config->data_format, PHASOR_DATA_TYPE_BIT))
    return VALUE_TYPE_FLOAT;
  else
    return VALUE_TYPE_INT;
}

/**
 * cts_conf_set_phasor_data_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the Phasor
 * data type has to be set. If this code is being run on a PMU
 * This will be always 1.
 * @data_type: the data type of Phasor value
 *
 * @data_type can be %VALUE_TYPE_FLOAT for 4 byte floating
 * point, or %VALUE_TYPE_INT for 2 byte unsigned integer.
 */
void
cts_conf_set_phasor_data_type_of_pmu (CtsConf  *self,
                                      uint16_t  pmu_index,
                                      byte      data_type)
{
  if (pmu_index > self->num_pmu)
    return;

  CtsPmuConf *config = self->pmu_config + pmu_index - 1;

  if (data_type == VALUE_TYPE_FLOAT)
    SET_BIT (config->data_format, PHASOR_DATA_TYPE_BIT);
  else
    CLEAR_BIT (config->data_format, PHASOR_DATA_TYPE_BIT);
}

/**
 * cts_conf_get_phasor_complex_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the Phasor Complex type
 * has to be retrieved. If this code is being run on a PMU
 * This will be always 1.
 *
 *
 * Returns: %VALUE_TYPE_RECTANGULAR if Phasor data type
 * is set as real and imaginary, or %VALUE_TYPE_POLAR if data type is
 * divided as magnitude and angle (That is, polar type).
 * if @pmu_index is invalid, %VALUE_TYPE_INVALID is returned.
 */
byte
cts_conf_get_phasor_complex_type_of_pmu (CtsConf  *self,
                                         uint16_t  pmu_index)
{
  if (pmu_index > self->num_pmu)
    return VALUE_TYPE_INVALID;

  CtsPmuConf *config = self->pmu_config + pmu_index - 1;

  return BIT_IS_SET (config->data_format, PHASOR_COMPLEX_TYPE_BIT);
}

/**
 * cts_conf_set_phasor_complex_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the Phasor Complex type
 * has to be set. If this code is being run on a PMU
 * This will be always 1.
 * @data_type: the complex data type of Phasor value
 *
 * @data_type can be %VALUE_TYPE_RECTANGULAR if complex type
 * has rectangular coordinates (divided as real and imaginary)
 * or %VALUE_TYPE_POLAR if coordinates are  divided as magnitude and angle.
 */
void
cts_conf_set_phasor_complex_type_of_pmu (CtsConf  *self,
                                         uint16_t  pmu_index,
                                         bool      is_polar)
{
  if (pmu_index > self->num_pmu)
    return;

  CtsPmuConf *config = self->pmu_config + pmu_index - 1;

  if (is_polar)
    SET_BIT (config->data_format, PHASOR_COMPLEX_TYPE_BIT);
  else
    CLEAR_BIT (config->data_format, PHASOR_COMPLEX_TYPE_BIT);
}

static bool
cts_conf_set_values_of_pmu (CtsConf   *self,
                            uint32_t **ptr,
                            uint16_t   pmu_index,
                            uint16_t   count)
{
  uint32_t *data = NULL;

  data = realloc (*ptr, sizeof **ptr * count);

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

  *ptr = NULL;
  return false;
}

/**
 * cts_conf_get_num_of_phasors_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the number of Phasors
 * has to be retrieved. If this code is being run on a PMU
 * This will be always 1.
 *
 * Returns: An unsigned 16 bit integer. 0 denotes that @pmu_index
 * is invalid, or number of Phasors is not set.
 */
uint16_t
cts_conf_get_num_of_phasors_of_pmu (CtsConf  *self,
                                    uint16_t  pmu_index)
{
  if (pmu_index > self->num_pmu)
    return 0;

  return (self->pmu_config + pmu_index - 1)->num_phasors;
}

/**
 * cts_conf_set_num_of_phasors_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the number of Phasors
 * has to be set. If this code is being run on a PMU,
 * this will be always 1.
 * @count: the Phasor count to be set.
 *
 * Along with setting count, this function also allocates the memory
 * required for @count Phasors.
 *
 * Returns: returns @count if allocating memory succeeded. Else,
 * returns the previously set value of number of Phasors.
 */
uint16_t
cts_conf_set_num_of_phasors_of_pmu (CtsConf  *self,
                                    uint16_t  pmu_index,
                                    uint16_t  count)
{
  CtsPmuConf *config;
  bool done;

  if (pmu_index > self->num_pmu)
    return 0;

  config = self->pmu_config + pmu_index - 1;
  done = cts_conf_set_values_of_pmu (self, &config->conv_factor_phasor,
                                     pmu_index, count);

  if (done)
    config->num_phasors = count;

  return config->num_phasors;
}

/**
 * cts_conf_get_num_of_analogs_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the number of Analog values
 * has to be retrieved. If this code is being run on a PMU,
 * this will be always 1.
 *
 * Returns: An unsigned 16 bit integer. 0 denotes that @pmu_index
 * is invalid, or number of Analog values is not set.
 */
uint16_t
cts_conf_get_num_of_analogs_of_pmu (CtsConf  *self,
                                    uint16_t  pmu_index)
{
  if (pmu_index > self->num_pmu)
    return 0;

  return (self->pmu_config + pmu_index - 1)->num_analog_values;
}

/**
 * cts_conf_set_num_of_analogs_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the number of Analog values
 * has to be set. If this code is being run on a PMU,
 * this will be always 1.
 * @count: the Analog value count to be set.
 *
 * Along with setting count, this function also allocates the memory
 * required for @count Analog values.
 *
 * Returns: returns @count if allocating memory succeeded. Else,
 * returns the previously set value of number of Analog values.
 */
uint16_t
cts_conf_set_num_of_analogs_of_pmu (CtsConf  *self,
                                    uint16_t  pmu_index,
                                    uint16_t  count)
{
  CtsPmuConf *config;
  bool done;

  if (pmu_index > self->num_pmu)
    return 0;

  config = self->pmu_config + pmu_index - 1;
  done = cts_conf_set_values_of_pmu (self, &config->conv_factor_analog,
                                     pmu_index, count);

  if (done)
    config->num_analog_values = count;

  return config->num_analog_values;
}

/**
 * cts_conf_get_num_of_status_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the number of digital status words
 * has to be retrieved. If this code is being run on a PMU,
 * this will be always 1.
 *
 * Returns: An unsigned 16 bit integer. 0 denotes that @pmu_index
 * is invalid, or number of digital status words is not set.
 */
uint16_t
cts_conf_get_num_of_status_of_pmu (CtsConf  *self,
                                   uint16_t  pmu_index)
{
  if (pmu_index > self->num_pmu)
    return 0;

  return (self->pmu_config + pmu_index - 1)->num_status_words;
}

/**
 * cts_conf_set_num_of_status_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the number of digital status words
 * has to be set. If this code is being run on a PMU,
 * this will be always 1.
 * @count: the digital status words count to be set.
 *
 * Along with setting count, this function also allocates the memory
 * required for @count digital status words.
 *
 * Returns: returns @count if allocating memory succeeded. Else,
 * returns the previously set value of digital status words count.
 */
uint16_t
cts_conf_set_num_of_status_of_pmu (CtsConf  *self,
                                   uint16_t  pmu_index,
                                   uint16_t  count)
{
  CtsPmuConf *config;
  bool done;

  if (pmu_index > self->num_pmu)
    return 0;

  config = self->pmu_config + pmu_index - 1;
  done = cts_conf_set_values_of_pmu (self, &config->status_word_masks,
                                     pmu_index, count);

  if (done)
    config->num_status_words = count;

  return config->num_status_words;
}

/**
 * cts_conf_get_channel_names_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the channel names
 * has to be retrieved. If this code is being run on a PMU,
 * this will be always 1.
 *
 * Returns: (nullable) (transfer none): A pointer to an array of chars.
 * Each array will exactly 16 bytes in size. The last item in the array shall
 * be %NULL.
 * if @pmu_index is invalid, %NULL is returned.
 */
char **
cts_conf_get_channel_names_of_pmu (CtsConf  *self,
                                   uint16_t  pmu_index)
{
  if (pmu_index > self->num_pmu)
    return NULL;

  return (self->pmu_config + pmu_index - 1)->channel_names;
}

/**
 * cts_conf_set_channel_names_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the channel names
 * has to be set. If this code is being run on a PMU,
 * this will be always 1.
 * @channel_names: a pointer to an array of 16 byte chars each,
 * that ends with %NULL.
 *
 * Set the channel names of pmu with index @pmu_index.
 * No copy of @channel_names is made. ensure that @channel_names
 * exist as long as #CtsConf exist.
 *
 * Phasor count should be atleast one for the function to succeed.
 *
 * Returns: %true if channel names were set and %false otherwise.
 */
bool
cts_conf_set_channel_names_of_pmu (CtsConf   *self,
                                   uint16_t   pmu_index,
                                   char     **channel_names)
{
  CtsPmuConf *config = NULL;

  if (pmu_index > self->num_pmu)
    return false;

  config = (self->pmu_config + pmu_index - 1);

  if (!config->num_phasors)
    return false; /* Atleast one phasor is required */

  config->channel_names = channel_names;
  return true;
}

/**
 * cts_conf_set_phasor_measure_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the Phasor measurement type
 * has to be set. If this code is being run on a PMU,
 * this will be always 1.
 * @phasor_index: The phasor index which has to be changed.
 * @type: The type (voltage or current) of phasor.
 *
 * @type can be %VALUE_TYPE_CURRENT or %VALUE_TYPE_VOLTAGE.
 *
 * Returns: %true if phasor measurement type was set and %false otherwise.
 */
bool
cts_conf_set_phasor_measure_type_of_pmu (CtsConf  *self,
                                         uint16_t  pmu_index,
                                         uint16_t  phasor_index,
                                         byte      type)
{
  CtsPmuConf *config;
  uint32_t data;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;

  if (phasor_index > config->num_phasors)
    return false;

  data = *(config->conv_factor_phasor + phasor_index - 1);
  /* Save to the  1st byte of a 32 bit int */
  data = (data & 0x00FFFFFF) | (type << 24);
  *(config->conv_factor_phasor + phasor_index - 1) = data;

  return true;
}

/**
 * cts_conf_set_all_phasor_measure_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the Phasor measurement type
 * has to be set. If this code is being run on a PMU,
 * this will be always 1.
 * @type: The type (voltage or current) of phasor.
 *
 * @type can be %VALUE_TYPE_CURRENT or %VALUE_TYPE_VOLTAGE.
 *
 * Returns: %true if phasor measurement type was set and %false otherwise.
 */
bool
cts_conf_set_all_phasor_measure_type_of_pmu (CtsConf  *self,
                                             uint16_t  pmu_index,
                                             byte      type)
{
  CtsPmuConf *config;
  uint16_t num_phasors;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;
  num_phasors = config->num_phasors;

  for (uint16_t i = 1; i <= num_phasors; i++)
    {
      bool status = cts_conf_set_phasor_measure_type_of_pmu (self,
                                                             pmu_index,
                                                             i, type);
      if (!status)
        return false;
    }
  return true;
}

/**
 * cts_conf_set_all_phasor_measure_type_of_all_pmu:
 * @self: A valid configuration
 * @type: The type (voltage or current) of phasor.
 *
 * @type can be %VALUE_TYPE_CURRENT or %VALUE_TYPE_VOLTAGE.
 *
 * Returns: %true if phasor measurement type was set and %false otherwise.
 */
bool
cts_conf_set_all_phasor_measure_type_of_all_pmu (CtsConf *self,
                                                 byte     type)
{
  uint16_t num_pmu;

  num_pmu = self->num_pmu;

  for (uint16_t i = 1; i <= num_pmu; i++)
    {
      bool status = cts_conf_set_all_phasor_measure_type_of_pmu (self, i,
                                                                 type);
      if (!status)
        return false;
    }
  return true;
}

/**
 * cts_conf_set_phasor_measure_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the Phasor measurement type
 * has to be retrieved. If this code is being run on a PMU,
 * this will be always 1.
 * @phasor_index: The phasor index for which the value has to be retrieved.
 *
 * Returns: %VALUE_TYPE_CURRENT if current is being measured, or
 * %VALUE_TYPE_VOLTAGE for voltage measurements. %VALUE_TYPE_INVALID
 * is returned on error.
 */
byte
cts_conf_get_phasor_measure_type_of_pmu (CtsConf  *self,
                                         uint16_t  pmu_index,
                                         uint16_t  phasor_index)
{
  CtsPmuConf *config;
  uint32_t data;
  byte measurement_type;

  if (pmu_index > self->num_pmu)
    return VALUE_TYPE_INVALID;

  config = self->pmu_config + pmu_index - 1;

  if (phasor_index > config->num_phasors)
    return VALUE_TYPE_INVALID;

  data = *(config->conv_factor_phasor + phasor_index - 1);

  /* Get the last byte */
  measurement_type = data >> 24;

  if (measurement_type == VALUE_TYPE_CURRENT ||
      measurement_type == VALUE_TYPE_VOLTAGE)
    return measurement_type;
  else
    return VALUE_TYPE_INVALID;
}

/**
 * cts_conf_set_phasor_conv_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the Phasor measurement type
 * has to be set. If this code is being run on a PMU,
 * this will be always 1.
 * @phasor_index: The phasor index which has to be changed.
 * @conv_factor: The convertion factor of transmitted Phasor value.
 * Should be an unsigned integer not greater than 24 bits.
 *
 * @conv_factor is used to interpret the phasor value transfered.
 * Say for example,  if @conv_factor is 100000, and if
 * the phasor value (retrieved via :TODO:) is 140. Then the real phasor
 * voltage value will be 140 * 100000 * 10^(-5) V (or A if current). That is
 * @transmitted_value * @conv_factor * 10^(-5) V (or A if current).
 *
 * The logic for calculating a sensible @conv_factor:
 * 1. Get the maximum possible value that shall be measured, say max_value.
 * 2. The maximu possible value that can be saved in 2 bytes (the max possbile
 *    size of phasor integers) is 32768 (and the negative half)
 * 3. Now the @conv_factor can be calculated as:
 *    @conv_factor = max_value/32768 * 10^5.
 *
 * This convertion factor shall not be used if the tranmitted data
 * is set as float (using cts_conf_set_phasor_data_type_of_pmu())
 *
 * Returns: %true if Phasor convertion factor was set and %false otherwise.
 */
bool
cts_conf_set_phasor_conv_of_pmu (CtsConf  *self,
                                 uint16_t  pmu_index,
                                 uint16_t  phasor_index,
                                 uint32_t  conv_factor)
{
  CtsPmuConf *config;
  uint32_t data;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;

  if (phasor_index > config->num_phasors)
    return false;

  data = *(config->conv_factor_phasor + phasor_index - 1);
  /* Save to the last 3 bytes */
  data = (data & 0xFF000000) | (conv_factor & 0x00FFFFFF);
  *(config->conv_factor_phasor + phasor_index - 1) = data;

  return true;
}

/**
 * cts_conf_set_all_phasor_conv_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the Phasor measurement type
 * has to be set. If this code is being run on a PMU,
 * this will be always 1.
 * @conv_factor: The convertion factor of transmitted Phasor value.
 * Should be an unsigned integer not greater than 24 bits.
 *
 * Set @conv_factor as convertion factor for every phasor values of PMU
 * with index @pmu_index.
 * See cts_conf_set_phasor_conv_of_pmu() for more details
 *
 * Returns: %true if Phasor convertion factor was set and %false otherwise.
 */
bool
cts_conf_set_all_phasor_conv_of_pmu (CtsConf  *self,
                                     uint16_t  pmu_index,
                                     uint32_t  conv_factor)
{
  CtsPmuConf *config;
  uint16_t num_phasors;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;
  num_phasors = config->num_phasors;

  for (uint16_t i = 1; i <= num_phasors; i++)
    {
      bool status = cts_conf_set_phasor_conv_of_pmu (self, pmu_index,
                                                     i, conv_factor);
      if (!status)
        return false;
    }
  return true;
}

/**
 * cts_conf_set_all_phasor_conv_of_all_pmu:
 * @self: A valid configuration
 * @conv_factor: The convertion factor of transmitted Phasor value.
 * Should be an unsigned integer not greater than 24 bits.
 *
 * Set @conv_factor as convertion factor for every phasor of every PMU.
 * See cts_conf_set_phasor_conv_of_pmu() for more details
 *
 * Returns: %true if Phasor convertion factor was set and %false otherwise.
 */
bool
cts_conf_set_all_phasor_conv_of_all_pmu (CtsConf  *self,
                                         uint32_t  conv_factor)
{
  uint16_t num_pmu;

  num_pmu = self->num_pmu;

  for (uint16_t i = 1; i <= num_pmu; i++)
    {
      bool status = cts_conf_set_all_phasor_conv_of_pmu (self, i,
                                                         conv_factor);
      if (!status)
        return false;
    }
  return true;
}

/**
 * cts_conf_set_analog_measure_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the Analog measurement type
 * has to be set. If this code is being run on a PMU,
 * this will be always 1.
 * @analog_index: The analog index of which the type has to be changed.
 * @type: The type of analog.
 *
 * @type can be %VALUE_TYPE_SINGLE_POINT_ON_WAVE, %VALUE_TYPE_RMS,
 * or %VALUE_TYPE_PEAK.
 *
 * Returns: %true if analog measurement type was set and %false otherwise.
 */
bool
cts_conf_set_analog_measure_type_of_pmu (CtsConf  *self,
                                         uint16_t  pmu_index,
                                         uint16_t  analog_index,
                                         byte      type)
{
  CtsPmuConf *config;
  uint32_t data;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;

  if (analog_index > config->num_analog_values)
    return false;

  data = *(config->conv_factor_analog + analog_index - 1);
  /* Save to the  1st byte of a 32 bit int */
  data = (data & 0x00FFFFFF) | (type << 24);
  *(config->conv_factor_analog + analog_index - 1) = data;

  return true;
}

/**
 * cts_conf_set_all_analog_measure_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the Analog measurement type
 * has to be set. If this code is being run on a PMU,
 * this will be always 1.
 * @type: The type of analog.
 *
 * @type can be %VALUE_TYPE_SINGLE_POINT_ON_WAVE, %VALUE_TYPE_RMS,
 * or %VALUE_TYPE_PEAK.
 *
 * Returns: %true if analog measurement type was set and %false otherwise.
 */
bool
cts_conf_set_all_analog_measure_type_of_pmu (CtsConf  *self,
                                             uint16_t  pmu_index,
                                             byte      type)
{
  CtsPmuConf *config;
  uint16_t num_analogs;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;
  num_analogs = config->num_analog_values;

  for (uint16_t i = 1; i <= num_analogs; i++)
    {
      bool status = cts_conf_set_analog_measure_type_of_pmu (self,
                                                             pmu_index,
                                                             i, type);
      if (!status)
        return false;
    }
  return true;
}

/**
 * cts_conf_set_all_analog_measure_type_of_all_pmu:
 * @self: A valid configuration
 * @type: The type of Analog value.
 *
 * @type can be %VALUE_TYPE_SINGLE_POINT_ON_WAVE, %VALUE_TYPE_RMS,
 * or %VALUE_TYPE_PEAK.
 *
 * Returns: %true if analog measurement type was set and %false otherwise.
 */
bool
cts_conf_set_all_analog_measure_type_of_all_pmu (CtsConf *self,
                                                 byte     type)
{
  uint16_t num_pmu;

  num_pmu = self->num_pmu;

  for (uint16_t i = 1; i <= num_pmu; i++)
    {
      bool status = cts_conf_set_all_analog_measure_type_of_pmu (self, i,
                                                                 type);
      if (!status)
        return false;
    }
  return true;
}

/**
 * cts_conf_get_analog_measure_type_of_pmu:
 * @self: A valid configuration
 * @pmu_index: The index of PMU of which the analog measurement type
 * has to be retrieved. If this code is being run on a PMU,
 * this will be always 1.
 * @analog_index: The analog index for which the value has to be retrieved.
 *
 * Returns: %VALUE_TYPE_SINGLE_POINT_ON_WAVE, %VALUE_TYPE_RMS,
 * or %VALUE_TYPE_PEAK.
 * %VALUE_TYPE_INVALID is returned on error.
 */
byte
cts_conf_get_analog_measure_type_of_pmu (CtsConf  *self,
                                         uint16_t  pmu_index,
                                         uint16_t  analog_index)
{
  CtsPmuConf *config;
  uint32_t data;
  byte measurement_type;

  if (pmu_index > self->num_pmu)
    return VALUE_TYPE_INVALID;

  config = self->pmu_config + pmu_index - 1;

  if (analog_index > config->num_analog_values)
    return VALUE_TYPE_INVALID;

  data = *(config->conv_factor_analog + analog_index - 1);

  /* Get the last byte */
  measurement_type = data >> 24;

  if (measurement_type == VALUE_TYPE_RMS ||
      measurement_type == VALUE_TYPE_PEAK ||
      measurement_type == VALUE_TYPE_SINGLE_POINT_ON_WAVE)
    return measurement_type;
  else
    return VALUE_TYPE_INVALID;
}

bool
cts_conf_set_analog_conv_of_pmu (CtsConf  *self,
                                 uint16_t  pmu_index,
                                 uint16_t  analog_index,
                                 uint32_t  conv_factor)
{
  CtsPmuConf *config;
  uint32_t data;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;

  if (analog_index > config->num_analog_values)
    return false;

  data = *(config->conv_factor_analog + analog_index - 1);
  /* Save to the last 3 bytes */
  data = (data & 0xFF000000) | (conv_factor & 0x00FFFFFF);
  *(config->conv_factor_analog + analog_index - 1) = data;

  return true;
}

bool
cts_conf_set_all_analog_conv_of_pmu (CtsConf  *self,
                                     uint16_t  pmu_index,
                                     uint32_t  conv_factor)
{
  CtsPmuConf *config;
  uint16_t num_analog;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;
  num_analog = config->num_analog_values;

  for (uint16_t i = 1; i <= num_analog; i++)
    {
      bool status = cts_conf_set_analog_conv_of_pmu (self, pmu_index,
                                                     i, conv_factor);
      if (!status)
        return false;
    }
  return true;
}

bool
cts_conf_set_all_analog_conv_of_all_pmu (CtsConf  *self,
                                         uint32_t  conv_factor)
{
  uint16_t num_pmu;

  num_pmu = self->num_pmu;

  for (uint16_t i = 1; i <= num_pmu; i++)
    {
      bool status = cts_conf_set_all_analog_conv_of_pmu (self, i,
                                                         conv_factor);
      if (!status)
        return false;
    }
  return true;
}

bool
cts_conf_set_status_normal_masks_of_pmu (CtsConf  *self,
                                         uint16_t  pmu_index,
                                         uint16_t  status_index,
                                         uint16_t  state)
{
  CtsPmuConf *config;
  uint32_t data;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;

  if (status_index > config->num_status_words)
    return false;

  data = *(config->status_word_masks + status_index - 1);
  /* Save to the first 2 bytes */
  data = (data & 0x0000FFFF) | (state & 0xFFFF0000);
  *(config->status_word_masks + status_index - 1) = data;

  return true;
}

bool
cts_conf_set_all_status_normal_masks_of_pmu (CtsConf  *self,
                                             uint16_t  pmu_index,
                                             uint16_t  state)
{
  CtsPmuConf *config;
  uint16_t num_status;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;
  num_status = config->num_status_words;

  for (uint16_t i = 1; i <= num_status; i++)
    {
      bool status =
        cts_conf_set_status_normal_masks_of_pmu (self,
                                                 pmu_index,
                                                 i, state);
      if (!status)
        return false;
    }
  return true;
}

bool
cts_conf_set_all_status_normal_masks_of_all_pmu (CtsConf  *self,
                                                 uint16_t  state)
{
  uint16_t num_pmu;

  num_pmu = self->num_pmu;

  for (uint16_t i = 1; i <= num_pmu; i++)
    {
      bool status =
        cts_conf_set_all_status_normal_masks_of_pmu (self, i,
                                                     state);
      if (!status)
        return false;
    }
  return true;
}

bool
cts_conf_set_status_validity_masks_of_pmu (CtsConf  *self,
                                           uint16_t  pmu_index,
                                           uint16_t  status_index,
                                           uint16_t  validity)
{
  CtsPmuConf *config;
  uint32_t data;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;

  if (status_index > config->num_status_words)
    return false;

  data = *(config->status_word_masks + status_index - 1);
  /* Save to the last 2 bytes */
  data = (data & 0xFFFF0000) | (validity & 0x0000FFFF);
  *(config->status_word_masks + status_index - 1) = data;

  return true;
}

bool
cts_conf_set_all_status_validity_masks_of_pmu (CtsConf  *self,
                                               uint16_t  pmu_index,
                                               uint16_t  validity)
{
  CtsPmuConf *config;
  uint16_t num_status;

  if (pmu_index > self->num_pmu)
    return false;

  config = self->pmu_config + pmu_index - 1;
  num_status = config->num_status_words;

  for (uint16_t i = 1; i <= num_status; i++)
    {
      bool status =
        cts_conf_set_status_validity_masks_of_pmu (self,
                                                   pmu_index,
                                                   i, validity);
      if (!status)
        return false;
    }
  return true;
}

bool
cts_conf_set_all_status_validity_masks_of_all_pmu (CtsConf  *self,
                                                   uint16_t  validity)
{
  uint16_t num_pmu;

  num_pmu = self->num_pmu;

  for (uint16_t i = 1; i <= num_pmu; i++)
    {
      bool status =
        cts_conf_set_all_status_validity_masks_of_pmu (self, i,
                                                       validity);
      if (!status)
        return false;
    }
  return true;
}

bool
cts_conf_set_nominal_freq_of_pmu (CtsConf  *self,
                                  uint16_t  pmu_index,
                                  uint16_t  freq)
{
  if (pmu_index > self->num_pmu)
    return false;

  if (freq == 60)
    freq = 0;
  else
    freq = 1;

  (self->pmu_config + pmu_index - 1)->nominal_freq = freq;
  return true;
}

bool
cts_conf_increment_change_count_of_pmu (CtsConf  *self,
                                        uint16_t  pmu_index)
{
  if (pmu_index > self->num_pmu)
    return false;

  (self->pmu_config + pmu_index - 1)->conf_change_count++;
  return true;
}

bool
cts_conf_set_change_count_of_pmu (CtsConf  *self,
                                  uint16_t  pmu_index,
                                  uint16_t  count)
{
  if (pmu_index > self->num_pmu)
    return false;

  (self->pmu_config + pmu_index - 1)->conf_change_count = count;
  return true;
}

static CtsConf *
cts_conf_new (void)
{
  CtsConf *self = NULL;

  self = malloc (sizeof *self);

  if (self)
    {
      /* Initialize dangerous variables */
      self->num_pmu = 0;
      self->pmu_config = NULL;
    }

  return self;
}

CtsConf *
cts_conf_get_default_config_one (void)
{
  if (config_default_one == NULL)
    config_default_one = cts_conf_new ();

  return config_default_one;
}

/* TODO: Don't replicate data in one */
CtsConf *
cts_conf_get_default_config_two (void)
{
  if (config_default_two == NULL)
    config_default_two = cts_conf_new ();

  return config_default_two;
}

static size_t
get_per_pmu_total_size (CtsConf    *self,
                        CtsPmuConf *pmu_config,
                        uint16_t    pmu_index)
{
  size_t pmu_size;
  byte size;

  pmu_size = 16 * (pmu_config->num_phasors +
                   pmu_config->num_analog_values +
                   16 * pmu_config->num_status_words);


  if (cts_conf_get_freq_data_type_of_pmu (self, pmu_index) == VALUE_TYPE_FLOAT)
    size = 4;
  else
    size = 2;

  pmu_size += size * pmu_config->num_status_words;


  if (cts_conf_get_analog_data_type_of_pmu (self, pmu_index) == VALUE_TYPE_FLOAT)
    size = 4;
  else
    size = 2;

  pmu_size += size * pmu_config->num_analog_values;


  if (cts_conf_get_phasor_data_type_of_pmu (self, pmu_index) == VALUE_TYPE_FLOAT)
    size = 4;
  else
    size = 2;

  pmu_size += size * pmu_config->num_phasors;

  return pmu_size + CONFIG_COMMON_SIZE_PER_PMU;
}

static size_t
calc_total_size (CtsConf *self)
{
  uint16_t num_pmu;
  size_t total_pmu_size = 0;

  num_pmu = self->num_pmu;

  for (uint16_t i = 0; i < num_pmu; i++)
    total_pmu_size += get_per_pmu_total_size (self,
                                              self->pmu_config + i,
                                              i + 1);

  return total_pmu_size + CONFIG_COMMON_SIZE;
}

static void
populate_raw_data_of_conf_part1 (CtsConf  *config,
                                 size_t    frame_size,
                                 byte    **pptr)
{
  uint16_t *byte2 = malloc (sizeof (*byte2));
  uint32_t *byte4 = malloc (sizeof (*byte4));

  *byte2 = htons (SYNC_CONFIG_TWO);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  *byte2 = htons (frame_size);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  *byte2 = htons (config->id_code);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  *byte4 = htonl (0x448527F0U);
  /* *byte4 = htonl (pmu_common_get_time_seconds()); */
  memcpy (*pptr, byte4, 4);
  *pptr += 4;

  /* TODO: configure Leap seconds and quality */
  *byte4 = htonl (0X56071098);
  /* *byte4 = htonl (pmu_common_get_fraction_of_seconds()); */
  memcpy (*pptr, byte4, 4);
  *pptr += 4;

  *byte4 = htonl (config->time_base);
  memcpy (*pptr, byte4, 4);
  *pptr += 4;

  *byte2 = htons (config->num_pmu);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  free (byte2);
  free (byte4);
}

static void
populate_raw_data_of_conf_part2 (CtsConf  *config,
                                 size_t    frame_size,
                                 byte     *data,
                                 byte    **pptr)
{
  uint16_t *byte2 = malloc (sizeof (*byte2));

  *byte2 = htons (config->data_rate);
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  *byte2 = htons (cts_common_calc_crc(data, frame_size - 1, NULL));
  memcpy (*pptr, byte2, 2);
  *pptr += 2;

  free (byte2);
}

static void
populate_raw_data_of_pmu_part1 (CtsPmuConf  *config,
                                byte       **pptr)
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
populate_raw_data_of_pmu_part2 (CtsPmuConf  *config,
                                byte       **pptr)
{
  uint16_t *byte2 = malloc (sizeof (*byte2));
  uint32_t *byte4 = malloc (sizeof (*byte4));

  for (uint16_t i = 0; i < config->num_phasors; i++)
    {
      *byte4 = htonl (*(config->conv_factor_phasor + i));
      memcpy (*pptr, byte4, 4);
      *pptr += 4;
    }

  for (uint16_t i = 0; i < config->num_analog_values; i++)
    {
      *byte4 = htonl (*(config->conv_factor_analog + i));
      memcpy (*pptr, byte4, 4);
      *pptr += 4;
    }

  for (uint16_t i = 0; i < config->num_status_words; i++)
    {
      *byte4 = htonl (*(config->status_word_masks + i));
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
populate_raw_data (CtsConf *self)
{
  CtsPmuConf *config;
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

  populate_raw_data_of_conf_part1 (self, len, &copy);

  for (uint16_t i = 0; i < num_pmu; i++)
    {
      config = self->pmu_config + i;
      populate_raw_data_of_pmu_part1 (config, &copy);
      copy_pmu_channel_names (config->channel_names, &copy);
      populate_raw_data_of_pmu_part2 (config, &copy);
    }

  populate_raw_data_of_conf_part2 (self, len, data, &copy);

  return data;
}

/**
 * cts_conf_get_raw_data:
 * @self: A valid configuration
 *
 * Returns the data populated from #CtsConf that can be directly
 * transfered over network.
 *
 * Returns: (nullable) (transfer full): return pointer to #byte in
 * Big Endian (network) order. free with free() when no longer needed.
 */
byte *
cts_conf_get_raw_data (CtsConf *self)
{
  byte *data;
  if (self == NULL)
    return NULL;

  data = populate_raw_data (self);

  return data;
}

/**
 * cts_conf_update_frame_size:
 * @self: A valid configuration
 *
 * Update the configuration size based on configuration.
 * This function should be called ONLY after setting
 * cts_conf_set_num_of_pmu(), cts_conf_set_num_of_analogs_of_pmu()
 * cts_conf_set_num_of_phasors_of_pmu(), cts_conf_set_phasor_data_type_of_pmu(),
 * cts_conf_set_analog_data_type_of_pmu(), cts_conf_set_num_of_status_of_pmu(),
 * and cts_conf_set_freq_data_type_of_pmu().
 *
 * The updated frame size can be retrieved by calling
 * cts_conf_get_frame_size().
 */
void
cts_conf_update_frame_size (CtsConf *self)
{
  self->frame_size = calc_total_size (self);
}

uint16_t
cts_conf_get_frame_size (CtsConf *self)
{
  return self->frame_size;
}
