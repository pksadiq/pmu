/* pmu-setup-window.c
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

#include "pmu-app.h"
#include "pmu-utils.h"

#include "pmu-setup-window.h"


struct _PmuSetupWindow
{
  GtkApplicationWindow parent_instance;

  GtkWidget *cancel_button;
  GtkWidget *save_button;
  GtkWidget *station_name_entry;
  GtkWidget *pmu_id_entry;
  GtkWidget *port_number_entry;
  GtkWidget *admin_ip_entry;
};


G_DEFINE_TYPE (PmuSetupWindow, pmu_setup_window, GTK_TYPE_APPLICATION_WINDOW)

static void
pmu_setup_window_finalize (GObject *object)
{
  G_OBJECT_CLASS (pmu_setup_window_parent_class)->finalize (object);
}

static void
cancel_button_clicked_cb (GtkWidget      *button,
                          PmuSetupWindow *window)
{
  GApplication *app = G_APPLICATION (gtk_window_get_application (GTK_WINDOW (window)));

  g_application_quit (app);
}

static void
entry_text_changed_cb (GObject    *gobject,
                       GParamSpec *pspec,
                       gpointer    user_data)
{
  PmuSetupWindow *window = PMU_SETUP_WINDOW (user_data);

  if (gtk_entry_get_text_length (GTK_ENTRY (window->station_name_entry)) == 0
      || gtk_entry_get_text_length (GTK_ENTRY (window->admin_ip_entry)) == 0)
    {
      gtk_widget_set_sensitive (window->save_button, FALSE);
      return;
    }

  if (g_hostname_is_ip_address (gtk_entry_get_text (GTK_ENTRY (window->admin_ip_entry))))
    gtk_widget_set_sensitive (window->save_button, TRUE);
}

static void
pmu_setup_window_class_init (PmuSetupWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = pmu_setup_window_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/sadiqpk/pmu/ui/pmu-setup-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PmuSetupWindow, save_button);
  gtk_widget_class_bind_template_child (widget_class, PmuSetupWindow, cancel_button);
  gtk_widget_class_bind_template_child (widget_class, PmuSetupWindow, station_name_entry);
  gtk_widget_class_bind_template_child (widget_class, PmuSetupWindow, pmu_id_entry);
  gtk_widget_class_bind_template_child (widget_class, PmuSetupWindow, port_number_entry);
  gtk_widget_class_bind_template_child (widget_class, PmuSetupWindow, admin_ip_entry);

  gtk_widget_class_bind_template_callback (widget_class, cancel_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, entry_text_changed_cb);

}

static void
pmu_setup_window_init (PmuSetupWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

PmuSetupWindow *
pmu_setup_window_new (PmuApp *app)
{
  return g_object_new (PMU_TYPE_SETUP_WINDOW,
                       "application", app,
                       NULL);
}

