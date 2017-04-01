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

#include "pmu-details.h"

#include "pmu-list.h"


struct _PmuList
{
  GtkGrid parent_instance;

  GtkWidget *tree_view_data;
  GtkWidget *tree_view_details;

  GtkListStore *pmu_data_store;
  GtkListStore *pmu_details_store;
};


G_DEFINE_TYPE (PmuList, pmu_list, GTK_TYPE_GRID)

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

  object_class->finalize = pmu_list_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/sadiqpk/pmu/ui/pmu-list.ui");

  gtk_widget_class_bind_template_child (widget_class, PmuList, tree_view_data);
  gtk_widget_class_bind_template_child (widget_class, PmuList, tree_view_details);
  gtk_widget_class_bind_template_child (widget_class, PmuList, pmu_details_store);
  gtk_widget_class_bind_template_child (widget_class, PmuList, pmu_data_store);
}

static void
pmu_list_init (PmuList *self)
{
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  g_autofree gchar *admin_ip = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (self->tree_view_details));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (self->tree_view_data));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);

  admin_ip = g_strdup (pmu_details_get_admin_ip ());
  if (admin_ip != NULL)
    {
      gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->pmu_details_store), &iter);
      gtk_list_store_set (self->pmu_details_store, &iter, 1, admin_ip, -1);
    }
}

PmuList *
pmu_list_new (void)
{
  return g_object_new (PMU_TYPE_LIST,
                       NULL);
}

