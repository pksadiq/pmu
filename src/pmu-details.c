/* pmu-details.c
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

#include "c37/c37.h"
#include "pmu-config.h"

#include "pmu-details.h"


struct _PmuDetails
{
  GObject parent_instance;

  gchar   *station_name;
  gchar   *admin_ip;
  gboolean first_run;
  guint    pmu_id;
  guint    port_number;
};

GSettings *settings;
PmuDetails *default_details;

G_DEFINE_TYPE (PmuDetails, pmu_details, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_FIRST_RUN,
  PROP_PMU_ID,
  PROP_STATION_NAME,
  PROP_ADMIN_IP,
  PROP_PORT_NUMBER,
  N_PROPS
};

static void
pmu_details_finalize (GObject *object)
{
  PmuDetails *self = PMU_DETAILS (object);

  pmu_details_save_settings ();

  g_free (self->station_name);
  g_free (self->admin_ip);

  G_OBJECT_CLASS (pmu_details_parent_class)->finalize (object);
}

static void
pmu_details_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  PmuDetails *self = PMU_DETAILS (object);

  switch (prop_id)
    {
    case PROP_FIRST_RUN:
      g_value_set_boolean (value, self->first_run);
      break;
    case PROP_PMU_ID:
      g_value_set_uint (value, self->pmu_id);
      break;
    case PROP_STATION_NAME:
      g_value_set_string (value, self->station_name);
      break;
    case PROP_ADMIN_IP:
      g_value_set_string (value, self->admin_ip);
      break;
    case PROP_PORT_NUMBER:
      g_value_set_uint (value, self->port_number);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
pmu_details_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  PmuDetails *self = PMU_DETAILS (object);

  switch (prop_id)
    {
    case PROP_FIRST_RUN:
      self->first_run = g_value_get_boolean (value);
      break;
    case PROP_PMU_ID:
      self->pmu_id = g_value_get_uint (value);
      break;
    case PROP_STATION_NAME:
      g_free (self->station_name);
      self->station_name = g_value_dup_string (value);
      break;
    case PROP_ADMIN_IP:
      g_free (self->admin_ip);
      self->admin_ip = g_value_dup_string (value);
      break;
    case PROP_PORT_NUMBER:
      self->port_number = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
pmu_details_class_init (PmuDetailsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  object_class->finalize = pmu_details_finalize;
  object_class->get_property = pmu_details_get_property;
  object_class->set_property = pmu_details_set_property;

  pspec = g_param_spec_boolean ("first-run", NULL, NULL,
                                1,
                                G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_FIRST_RUN, pspec);

  pspec = g_param_spec_uint ("pmu-id", NULL, NULL,
                             1, 65535, 1,
                             G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_PMU_ID, pspec);

  pspec = g_param_spec_string ("station-name", NULL, NULL,
                               NULL,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_STATION_NAME, pspec);

  pspec = g_param_spec_string ("admin-ip", NULL, NULL,
                               NULL,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_ADMIN_IP, pspec);

  pspec = g_param_spec_uint ("port-number", NULL, NULL,
                             1025, 65535, 4713,
                             G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_PORT_NUMBER, pspec);

  settings = g_settings_new ("org.sadiqpk.pmu");
}

void
pmu_details_save_settings (void)
{
  g_settings_set_uint (settings, "pmu-id", default_details->pmu_id);
  g_settings_set_uint (settings, "port", default_details->port_number);
  g_settings_set_string (settings, "admin-ip", default_details->admin_ip);
  g_settings_set_string (settings, "station-name", default_details->station_name);
  g_settings_set_boolean (settings, "first-run", 0);
}

static void
pmu_details_populate (PmuDetails *self)
{
  g_object_set (self,
                "first-run", g_settings_get_boolean (settings, "first-run"),
                "pmu-id", g_settings_get_uint (settings, "pmu-id"),
                "station-name", g_settings_get_string (settings, "station-name"),
                "admin-ip", g_settings_get_string (settings, "admin-ip"),
                "port-number", g_settings_get_uint (settings, "port"),
                NULL);
}

static void
pmu_details_init (PmuDetails *self)
{
  pmu_details_populate (self);
}

gchar *
pmu_details_get_station_name (void)
{
  if (default_details)
    return default_details->station_name;

  return NULL;
}

gchar *
pmu_details_get_admin_ip (void)
{
  if (default_details)
    return default_details->admin_ip;

  return NULL;
}

guint
pmu_details_get_port_number (void)
{
  if (default_details)
    return default_details->port_number;

  return 0;
}

guint
pmu_details_get_pmu_id (void)
{
  if (default_details)
    return default_details->pmu_id;

  return 0;
}

gboolean
pmu_details_get_is_first_run (void)
{
  if (default_details)
    return default_details->first_run;

  /* Assume that it is running first */
  return TRUE;
}

void
pmu_details_configure_pmu (PmuDetails *details)
{
  CtsData *data;
  CtsConf *config1 = cts_conf_get_default_config_one ();

  cts_conf_set_id_code (config1, pmu_details_get_pmu_id ());
  cts_conf_set_time_base (config1, 1000);
  cts_conf_set_data_rate (config1, 1000);
  cts_conf_set_num_of_pmu (config1, 1);
  cts_conf_set_station_name_of_pmu (config1, 1,
                                    pmu_details_get_station_name (),
                                    strlen (pmu_details_get_station_name ()));
  cts_conf_set_id_code_of_pmu (config1, 1, pmu_details_get_pmu_id ());

  cts_conf_set_num_of_phasors_of_pmu (config1, 1, 3);
  cts_conf_set_num_of_analogs_of_pmu (config1, 1, 3);
  cts_conf_set_num_of_status_of_pmu (config1, 1, 1);

  cts_conf_set_freq_data_type_of_pmu (config1, 1, VALUE_TYPE_INT);

  cts_conf_set_analog_data_type_of_pmu (config1, 1, VALUE_TYPE_INT);
  cts_conf_set_all_analog_measure_type_of_pmu (config1, 1, VALUE_TYPE_RMS);
  cts_conf_set_all_analog_conv_of_pmu (config1, 1, 10000);

  cts_conf_set_phasor_data_type_of_pmu (config1, 1, VALUE_TYPE_INT);
  cts_conf_set_phasor_complex_type_of_pmu (config1, 1, VALUE_TYPE_POLAR);
  cts_conf_set_all_phasor_measure_type_of_pmu (config1, 1, VALUE_TYPE_VOLTAGE);
  cts_conf_set_all_phasor_conv_of_pmu (config1, 1, 10000);

  cts_conf_set_channel_names_of_pmu (config1, 1, channel_names);
  cts_conf_update_time (config1);

  data = cts_data_get_default ();
  cts_data_set_config (data, config1);
}

static PmuDetails *
pmu_details_new (void)
{
  default_details = g_object_new (PMU_TYPE_DETAILS,
                                  NULL);
  if (default_details)
    pmu_details_configure_pmu (default_details);
  return default_details;
}

PmuDetails *
pmu_details_get_default (void)
{
  if (default_details == NULL)
    pmu_details_new ();

  return default_details;
}
