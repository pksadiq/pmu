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
  guint    admin_port;
};

GSettings *settings;

G_DEFINE_TYPE (PmuDetails, pmu_details, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_FIRST_RUN,
  PROP_PMU_ID,
  PROP_STATION_NAME,
  PROP_ADMIN_IP,
  PROP_ADMIN_PORT,
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
    case PROP_ADMIN_PORT:
      g_value_set_uint (value, self->admin_port);
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
    case PROP_ADMIN_PORT:
      self->admin_port = g_value_get_uint (value);
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

  object_class->finalize = pmu_details_finalize;
  object_class->get_property = pmu_details_get_property;
  object_class->set_property = pmu_details_set_property;

  settings = g_settings_new ("org.sadiqpk.pmu");
}

void
pmu_details_save_settings (PmuDetails *self)
{
  if (!self->settings_modified)
    return;

  g_settings_set_uint (settings, "pmu-id", self->pmu_id);
  g_settings_set_uint (settings, "port", self->admin_port);
  g_settings_set_string (settings, "admin-ip", self->admin_ip);
  g_settings_set_string (settings, "pmu-id", self->station_name);
  g_settings_set_boolean (settings, "first-run", self->first_run);
}

static void
pmu_details_populate (PmuDetails *self)
{
  self->first_run = g_settings_get_boolean (settings, "first-run");
  self->pmu_id = g_settings_get_uint (settings, "pmu-id");
  self->station_name = g_settings_get_string (settings, "station-name");
  self->admin_ip = g_settings_get_string (settings, "admin-ip");
  self->admin_port = g_settings_get_uint (settings, "port");
}

static void
pmu_details_init (PmuDetails *self)
{
  self->settings_modified = FALSE;

  pmu_details_populate (self);
}

PmuDetails *
pmu_details_new (void)
{
  return g_object_new (PMU_TYPE_DETAILS,
                       NULL);
}

