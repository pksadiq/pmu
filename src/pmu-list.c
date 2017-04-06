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

#include "pmu-window.h"
#include "pmu-server.h"
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
pmu_list_setup_details (gpointer user_data)
{
  g_signal_connect (pmu_server_get_default (),
                    "server-started",
                    G_CALLBACK (pmu_list_update_details),
                    (PmuList *) user_data);

  g_signal_connect (pmu_server_get_default (),
                    "server-stopped",
                    G_CALLBACK (pmu_list_update_details),
                    (PmuList *) user_data);

  return G_SOURCE_REMOVE;
}

static gboolean
update_list (gpointer user_data)
{
  PmuList *list = PMU_LIST (user_data);

  g_print ("List updated\n");
  return G_SOURCE_CONTINUE;
}

static void
update_time_cb (PmuList  *self,
                gpointer  user_data)
{
  if (self->update_timeout_id)
    {
      g_source_remove (self->update_timeout_id);
      self->update_timeout_id = 0;
    }

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

  // XXX: Hack to delay until pmu server instance is created
  g_timeout_add_seconds (1,
                         pmu_list_setup_details,
                         self);

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

