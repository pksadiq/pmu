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

  GtkWidget *start_button;
  GtkWidget *stop_button;
  GtkWidget *menu_button;
  GtkWidget *revealer;
};


G_DEFINE_TYPE (PmuWindow, pmu_window, GTK_TYPE_APPLICATION_WINDOW)

static void sync_ntp_time_cb (GSimpleAction       *action,
                              GVariant            *param,
                              gpointer             user_data);

static const GActionEntry win_entries[] = {
  { "sync-ntp",  sync_ntp_time_cb },
};


static void sync_ntp_time_cb (GSimpleAction *action,
                              GVariant      *param,
                              gpointer       user_data)
{
  g_print ("NTP time updated\n");
}

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
start_button_clicked_cb (GtkWidget *button,
                         PmuWindow *window)
{
  gtk_revealer_set_reveal_child (GTK_REVEALER (window->revealer), TRUE);
  gtk_widget_hide (window->start_button);
  gtk_widget_show (window->stop_button);
  g_print ("start button clicked\n");
}

static void
revealer_button_clicked_cb (GtkWidget *button,
                            PmuWindow *window)
{
  gtk_revealer_set_reveal_child (GTK_REVEALER (window->revealer), FALSE);
}

static void
stop_button_clicked_cb (GtkWidget *button,
                        PmuWindow *window)
{
  gtk_widget_hide (window->stop_button);
  gtk_widget_show (window->start_button);

  g_print ("stop button clicked\n");
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
  gtk_widget_class_bind_template_child (widget_class, PmuWindow, start_button);
  gtk_widget_class_bind_template_child (widget_class, PmuWindow, stop_button);
  gtk_widget_class_bind_template_child (widget_class, PmuWindow, revealer);

  gtk_widget_class_bind_template_callback (widget_class, start_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, stop_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, revealer_button_clicked_cb);
}

static void
pmu_window_init (PmuWindow *self)
{
  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   win_entries,
                                   G_N_ELEMENTS (win_entries),
                                   self);

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
