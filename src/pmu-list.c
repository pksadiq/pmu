/* pmu-list.c
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
#include "pmu-window.h"
#include "pmu-server.h"
#include "pmu-spi.h"
#include "pmu-details.h"

#include "pmu-list.h"


struct _PmuList
{
  GtkGrid parent_instance;

  GtkWidget *tree_view_data;
  GtkWidget *tree_view_details;

  GtkListStore *pmu_data_store;
  GtkListStore *pmu_details_store;

  guint update_time;       /* in seconds */
  guint update_timeout_id;
};


G_DEFINE_TYPE (PmuList, pmu_list, GTK_TYPE_GRID)

enum {
  PROP_0,
  PROP_UPDATE_TIME,
  N_PROPS
};

static void
pmu_list_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  PmuList *self = PMU_LIST (object);

  switch (prop_id)
    {
    case PROP_UPDATE_TIME:
      g_value_set_uint (value, self->update_time);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
pmu_list_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  PmuList *self = PMU_LIST (object);

  switch (prop_id)
    {
    case PROP_UPDATE_TIME:
      self->update_time = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
pmu_list_finalize (GObject *object)
{
  G_OBJECT_CLASS (pmu_list_parent_class)->finalize (object);
}

static void
pmu_list_class_init (PmuListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GParamSpec *pspec;

  object_class->finalize = pmu_list_finalize;

  object_class->get_property = pmu_list_get_property;
  object_class->set_property = pmu_list_set_property;

  pspec = g_param_spec_uint ("update-time", NULL, NULL,
                             1, 100, 1,
                             G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_UPDATE_TIME, pspec);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/sadiqpk/pmu/ui/pmu-list.ui");

  gtk_widget_class_bind_template_child (widget_class, PmuList, tree_view_data);
  gtk_widget_class_bind_template_child (widget_class, PmuList, tree_view_details);
  gtk_widget_class_bind_template_child (widget_class, PmuList, pmu_details_store);
  gtk_widget_class_bind_template_child (widget_class, PmuList, pmu_data_store);
}

static void
pmu_list_update_details (gpointer  user_data,
                         PmuList  *self)
{
  GtkTreeIter iter;
  g_autofree gchar *admin_ip = NULL;

  admin_ip = g_strdup (pmu_details_get_admin_ip ());
  if (admin_ip == NULL)
    return;

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->pmu_details_store), &iter);

  if (pmu_server_is_running ())
    gtk_list_store_set (self->pmu_details_store, &iter, 1, admin_ip, -1);
  else
    gtk_list_store_set (self->pmu_details_store, &iter, 1, "", -1);
}

static gboolean
pmu_list_setup_details (PmuList *self)
{
  g_signal_connect (pmu_server_get_default (),
                    "server-started",
                    G_CALLBACK (pmu_list_update_details),
                    self);

  g_signal_connect (pmu_server_get_default (),
                    "server-stopped",
                    G_CALLBACK (pmu_list_update_details),
                    self);

  pmu_list_update_details (NULL,
                           self);

  return G_SOURCE_REMOVE;
}

static gboolean
update_list (gpointer user_data)
{
  PmuList      *list = PMU_LIST (user_data);
  CtsData      *cts_data;
  CtsConf      *cts_conf;
  GBytes       *bytes;
  const guchar *data;
  gchar        *value_string;
  GtkTreeIter   iter, iter_next;
  gsize         size;
  int           count;
  gshort        value[2];

  bytes = pmu_spi_data_pop_head ();

  if (bytes == NULL || !PMU_IS_LIST (list))
    return G_SOURCE_CONTINUE;

  data = g_bytes_get_data (bytes, &size);

  cts_data = cts_data_get_default ();
  cts_conf = cts_data_get_conf (cts_data);

  cts_data_populate_from_raw_data (cts_data, data, FALSE);

  g_bytes_unref (bytes);

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list->pmu_data_store), &iter);
  iter_next = iter;

  gtk_tree_model_iter_next (GTK_TREE_MODEL (list->pmu_data_store), &iter_next);

  count = cts_conf_get_num_of_phasors_of_pmu (cts_conf, 1);

  for (int i = 0, k = 1; i < count - 2; i++, k++)
    {
      cts_data_get_phasor_value_of_pmu (cts_data, 1, i + 1, value);

      value_string = g_strdup_printf ("%d", value[0]);
      gtk_list_store_set (list->pmu_data_store, &iter, i % 3 + 1, value_string, -1);
      g_free (value_string);

      value_string = g_strdup_printf ("%d", value[1]);
      gtk_list_store_set (list->pmu_data_store, &iter_next, i % 3 + 1, value_string, -1);
      g_free (value_string);

      if (k % 3 == 0 && k < count)
        {
          iter = iter_next;
          gtk_tree_model_iter_next (GTK_TREE_MODEL (list->pmu_data_store), &iter);
          iter_next = iter;
          gtk_tree_model_iter_next (GTK_TREE_MODEL (list->pmu_data_store), &iter_next);
        }
    }

  cts_data_get_phasor_value_of_pmu (cts_data, 1, count - 1, value);

  value_string = g_strdup_printf ("%d", value[0]);
  gtk_list_store_set (list->pmu_data_store, &iter, 1, value_string, -1);
  g_free (value_string);

  value_string = g_strdup_printf ("%d", value[1]);
  gtk_list_store_set (list->pmu_data_store, &iter, 3, value_string, -1);
  g_free (value_string);

  gtk_tree_model_iter_next (GTK_TREE_MODEL (list->pmu_data_store), &iter);

  cts_data_get_phasor_value_of_pmu (cts_data, 1, count, value);

  value_string = g_strdup_printf ("%d", value[0]);
  gtk_list_store_set (list->pmu_data_store, &iter, 1, value_string, -1);
  g_free (value_string);

  value_string = g_strdup_printf ("%d", value[1]);
  gtk_list_store_set (list->pmu_data_store, &iter, 3, value_string, -1);
  g_free (value_string);

  iter = iter_next;
  gtk_tree_model_iter_next (GTK_TREE_MODEL (list->pmu_data_store), &iter);

  count = cts_conf_get_num_of_analogs_of_pmu (cts_conf, 1);

  for (int i = 0, k = 1; i < count; i++, k++)
    {
      cts_data_get_analog_value_of_pmu (cts_data, 1, i + 1, value);

      value_string = g_strdup_printf ("%d", value[0]);
      gtk_list_store_set (list->pmu_data_store, &iter, i % 3 + 1, value_string, -1);
      g_free (value_string);

      if (k == 10)
        break;

      if (k % 3 == 0)
        {
          gtk_tree_model_iter_next (GTK_TREE_MODEL (list->pmu_data_store), &iter);
        }
    }

  cts_data_get_analog_value_of_pmu (cts_data, 1, count - 3, value);
  value_string = g_strdup_printf ("%d", value[0]);
  gtk_list_store_set (list->pmu_data_store, &iter, 3, value_string, -1);
  g_free (value_string);

  gtk_tree_model_iter_next (GTK_TREE_MODEL (list->pmu_data_store), &iter);
  cts_data_get_analog_value_of_pmu (cts_data, 1, count - 2, value);
  value_string = g_strdup_printf ("%d", value[0]);
  gtk_list_store_set (list->pmu_data_store, &iter, 1, value_string, -1);
  g_free (value_string);

  gtk_tree_model_iter_next (GTK_TREE_MODEL (list->pmu_data_store), &iter);
  cts_data_get_analog_value_of_pmu (cts_data, 1, count - 1, value);
  value_string = g_strdup_printf ("%2.1f", value[0] / 10.0);
  gtk_list_store_set (list->pmu_data_store, &iter, 1, value_string, -1);
  g_free (value_string);

  cts_data_get_analog_value_of_pmu (cts_data, 1, count, value);
  value_string = g_strdup_printf ("%1.1f", value[0] / 10.0);
  gtk_list_store_set (list->pmu_data_store, &iter, 3, value_string, -1);
  g_free (value_string);

  return G_SOURCE_CONTINUE;
}

static void
update_time_cb (PmuList  *self,
                gpointer  user_data)
{
  if (self->update_timeout_id)
    g_source_remove (self->update_timeout_id);

  self->update_timeout_id = g_timeout_add_seconds (self->update_time, update_list, self);
  g_print ("changed here\n");
}

static void
pmu_list_init (PmuList *self)
{
  GtkTreeSelection *selection;

  gtk_widget_init_template (GTK_WIDGET (self));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (self->tree_view_details));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (self->tree_view_data));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);

  pmu_list_setup_details (self);

  self->update_timeout_id = 0;
  g_signal_connect (self, "notify::update-time", G_CALLBACK (update_time_cb), NULL);
  g_object_set (G_OBJECT (self), "update-time", 3, NULL);
}

PmuList *
pmu_list_new (void)
{
  return g_object_new (PMU_TYPE_LIST,
                       "update-time", 1,
                       NULL);
}

