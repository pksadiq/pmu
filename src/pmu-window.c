/* pmu-window.c
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


struct _PmuWindow
{
  GtkApplicationWindow parent_instance;

  GtkWidget *menu_button;
};


G_DEFINE_TYPE (PmuWindow, pmu_window, GTK_TYPE_APPLICATION_WINDOW)

static void
pmu_window_finalize (GObject *object)
{
  G_OBJECT_CLASS (pmu_window_parent_class)->finalize (object);
}

static void
pmu_window_constructed (GObject *object)
{
  PmuWindow  *window;
  GMenuModel *menu;
  GAction    *action;
  g_autoptr(GtkBuilder) builder = NULL;

  window = PMU_WINDOW (object);

  builder = gtk_builder_new_from_resource ("/org/sadiqpk/pmu/gtk/menus.ui");
  menu = G_MENU_MODEL (gtk_builder_get_object (builder, "win-menu"));
  gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (window->menu_button), menu);

  G_OBJECT_CLASS (pmu_window_parent_class)->constructed (object);
}

static void
pmu_window_class_init (PmuWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = pmu_window_finalize;
  object_class->constructed = pmu_window_constructed;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/sadiqpk/pmu/ui/pmu-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PmuWindow, menu_button);
}

static void
pmu_window_init (PmuWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

PmuWindow *
pmu_window_new (PmuApp *app)
{
  g_assert (GTK_IS_APPLICATION (app));

  return g_object_new (PMU_TYPE_WINDOW,
                       "application", app,
                       NULL);
}
