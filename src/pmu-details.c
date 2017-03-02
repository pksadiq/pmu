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

#include "pmu-details.h"


struct _PmuDetails
{
  GObject parent_instance;

  gboolean settings_modified;
  gboolean first_run;
  gchar   *station_name;
  gchar   *admin_ip;
  guint    pmu_id;
  guint    port_number;
};

GSettings *settings;

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
      self->settings_modified = TRUE;
      break;
    case PROP_PMU_ID:
      self->pmu_id = g_value_get_uint (value);
      self->settings_modified = TRUE;
      break;
    case PROP_STATION_NAME:
      g_free (self->station_name);
      self->station_name = g_value_dup_string (value);
      self->settings_modified = TRUE;
      break;
    case PROP_ADMIN_IP:
      g_free (self->admin_ip);
      self->admin_ip = g_value_dup_string (value);
      self->settings_modified = TRUE;
      break;
    case PROP_PORT_NUMBER:
      self->port_number = g_value_get_uint (value);
      self->settings_modified = TRUE;
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
                             1, 1000, 1,
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
                             1025, 20000, 4713,
                             G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_PORT_NUMBER, pspec);

  settings = g_settings_new ("org.sadiqpk.pmu");
}

void
pmu_details_save_settings (PmuDetails *self)
{
  if (!self->first_run && !self->settings_modified)
    return;

  g_settings_set_uint (settings, "pmu-id", self->pmu_id);
  g_settings_set_uint (settings, "port", self->port_number);
  g_settings_set_string (settings, "admin-ip", self->admin_ip);
  g_settings_set_string (settings, "station-name", self->station_name);
  g_settings_set_boolean (settings, "first-run", 0);
}

static void
pmu_details_populate (PmuDetails *self)
{
  self->first_run = g_settings_get_boolean (settings, "first-run");
  self->pmu_id = g_settings_get_uint (settings, "pmu-id");
  self->station_name = g_settings_get_string (settings, "station-name");
  self->admin_ip = g_settings_get_string (settings, "admin-ip");
  self->port_number = g_settings_get_uint (settings, "port");
}

static void
pmu_details_init (PmuDetails *self)
{
  self->settings_modified = FALSE;

  pmu_details_populate (self);
}

gchar *
pmu_details_get_station_name (PmuDetails *self)
{
  return self->station_name;
}

gchar *
pmu_details_get_admin_ip (PmuDetails *self)
{
  return self->admin_ip;
}

guint
pmu_details_get_port_number (PmuDetails *self)
{
  return self->port_number;
}

guint
pmu_details_get_pmu_id (PmuDetails *self)
{
  return self->pmu_id;
}

gboolean
pmu_details_get_is_first_run (PmuDetails *self)
{
  return self->first_run;
}

PmuDetails *
pmu_details_new (void)
{
  return g_object_new (PMU_TYPE_DETAILS,
                       NULL);
}
