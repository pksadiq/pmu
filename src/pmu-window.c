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


#include "pmu-setup-window.h"
#include "pmu-list.h"
#include "pmu-details.h"
#include "pmu-server.h"
#include "pmu-spi.h"

#include "pmu-window.h"


struct _PmuWindow
{
  GtkApplicationWindow parent_instance;

  GtkWidget *header_bar;
  GtkWidget *start_button;
  GtkWidget *stop_button;
  GtkWidget *menu_button;
  GtkWidget *info_label;
  GtkWidget *revealer;

  guint revealer_timeout_id;
};

static GThread *ntp_thread;


G_DEFINE_TYPE (PmuWindow, pmu_window, GTK_TYPE_APPLICATION_WINDOW)

enum {
  PROP_0,
  N_PROPS
};

static void sync_ntp_time_cb  (GSimpleAction       *action,
                               GVariant            *param,
                               gpointer             user_data);
static void show_setup_window (GSimpleAction       *action,
                               GVariant            *param,
                               gpointer             user_data);
static void update_time_cb    (GSimpleAction       *action,
                               GVariant            *param,
                               gpointer             user_data);
static void time_change_state (GSimpleAction       *action,
                               GVariant            *param,
                               gpointer             user_data);


static const GActionEntry win_entries[] = {
  { "sync-ntp",  sync_ntp_time_cb  },
  { "settings",  show_setup_window },
  { "update-time", update_time_cb, "i", "1", time_change_state},
};

static void
time_change_state (GSimpleAction *action,
                   GVariant      *param,
                   gpointer       user_data)
{
}

static void
update_time_cb (GSimpleAction *action,
                GVariant      *param,
                gpointer       user_data)
{
  g_simple_action_set_state (action, param);
}

static gboolean
revealer_timeout (gpointer user_data)
{
  PmuWindow *window = PMU_WINDOW (user_data);

  if (window->revealer_timeout_id)
    {
      g_source_remove (window->revealer_timeout_id);
      window->revealer_timeout_id = 0;
    }

  gtk_revealer_set_reveal_child (GTK_REVEALER (window->revealer), FALSE);
  return G_SOURCE_REMOVE;
}

static gboolean
show_ntp_update_revealer (gpointer user_data)
{
  PmuWindow *window = PMU_WINDOW (user_data);

  gtk_label_set_label (GTK_LABEL (window->info_label), "NTP Sync completed");

  revealer_timeout (user_data);
  gtk_revealer_set_reveal_child (GTK_REVEALER (window->revealer), TRUE);
  window->revealer_timeout_id = g_timeout_add_seconds (5, revealer_timeout, window);

  return FALSE;
}

static gpointer
sync_ntp_time (gpointer user_data)
{
  GError *error = NULL;
  gint exit_status = 1;

  if (!g_spawn_command_line_sync ("/usr/bin/pkexec good.sh",
                                  NULL, NULL,
                                  &exit_status,
                                  &error))
    {
      g_warning ("%s", error->message);
      g_clear_error (&error);
      return NULL;
    }

  if (exit_status == 0)
    g_idle_add (show_ntp_update_revealer,
                user_data);

  return NULL;
}

static void
show_setup_window (GSimpleAction *action,
                   GVariant      *param,
                   gpointer       user_data)
{
  PmuSetupWindow *window;
  PmuWindow *self;
  PmuApp *app;

  self = PMU_WINDOW (user_data);
  app = PMU_APP (gtk_window_get_application (GTK_WINDOW (self)));

  window = pmu_setup_window_new (app);
  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (self));
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);

  gtk_window_present (GTK_WINDOW (window));
}

static void sync_ntp_time_cb (GSimpleAction *action,
                              GVariant      *param,
                              gpointer       user_data)
{
  ntp_thread = g_thread_new (NULL,
                             sync_ntp_time,
                             user_data);
}

static void
pmu_window_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
pmu_window_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
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
  g_autoptr(GtkBuilder) builder = NULL;

  window = PMU_WINDOW (object);

  builder = gtk_builder_new_from_resource ("/org/sadiqpk/pmu/gtk/menus.ui");
  menu = G_MENU_MODEL (gtk_builder_get_object (builder, "win-menu"));
  gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (window->menu_button), menu);

  G_OBJECT_CLASS (pmu_window_parent_class)->constructed (object);
}

gboolean
pmu_window_server_started_cb (PmuWindow *self)
{
  gtk_label_set_label (GTK_LABEL (self->info_label), "PMU Server started successfully");

  revealer_timeout (self);
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->revealer), TRUE);
  self->revealer_timeout_id = g_timeout_add_seconds (5, revealer_timeout, self);

  gtk_widget_hide (self->start_button);
  gtk_widget_show (self->stop_button);

  return G_SOURCE_REMOVE;
}

gboolean
pmu_window_server_stopped_cb (PmuWindow *self)
{
  gtk_label_set_label (GTK_LABEL (self->info_label), "PMU Server stopped");

  revealer_timeout (self);
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->revealer), TRUE);
  self->revealer_timeout_id = g_timeout_add_seconds (5, revealer_timeout, self);

  gtk_widget_hide (self->stop_button);
  gtk_widget_show (self->start_button);

  return G_SOURCE_REMOVE;
}

gboolean
pmu_window_spi_failed_cb (PmuWindow *self)
{
  gtk_label_set_label (GTK_LABEL (self->info_label), "Starting SPI failed");

  revealer_timeout (self);
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->revealer), TRUE);
  self->revealer_timeout_id = g_timeout_add_seconds (5, revealer_timeout, self);

  return G_SOURCE_REMOVE;
}

static void
start_button_clicked_cb (GtkWidget *button,
                         PmuWindow *window)
{
  GMainContext *context;

  context = pmu_server_get_default_context ();
  g_main_context_invoke (context, (GSourceFunc) pmu_server_start, window);
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
  GMainContext *context;

  context = pmu_server_get_default_context ();
  g_main_context_invoke (context, (GSourceFunc) pmu_server_stop, NULL);
}

static void
pmu_window_class_init (PmuWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = pmu_window_get_property;
  object_class->set_property = pmu_window_set_property;
  object_class->finalize = pmu_window_finalize;
  object_class->constructed = pmu_window_constructed;

  g_type_ensure (PMU_TYPE_LIST);
  g_type_ensure (PMU_TYPE_DETAILS);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/sadiqpk/pmu/ui/pmu-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PmuWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, PmuWindow, menu_button);
  gtk_widget_class_bind_template_child (widget_class, PmuWindow, start_button);
  gtk_widget_class_bind_template_child (widget_class, PmuWindow, stop_button);
  gtk_widget_class_bind_template_child (widget_class, PmuWindow, info_label);
  gtk_widget_class_bind_template_child (widget_class, PmuWindow, revealer);

  gtk_widget_class_bind_template_callback (widget_class, start_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, stop_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, revealer_button_clicked_cb);
}

gboolean
pmu_window_set_subtitle (GBinding     *binding,
                         const GValue *from_value,
                         GValue       *value,
                         gpointer      user_data)
{
  gchar *subtitle;

  subtitle = g_strdup_printf ("%s - %u", pmu_details_get_station_name (),
                              pmu_details_get_pmu_id ());
  g_value_set_string (value, subtitle);
  g_free (subtitle);

  return TRUE;
}

static void
pmu_window_init (PmuWindow *self)
{
  gchar *subtitle;

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   win_entries,
                                   G_N_ELEMENTS (win_entries),
                                   self);

  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_bind_property_full (pmu_details_get_default (), "station-name",
                               self->header_bar, "subtitle",
                               G_BINDING_DEFAULT,
                               pmu_window_set_subtitle, NULL,
                               NULL, NULL);

  subtitle = g_strdup_printf ("%s - %u", pmu_details_get_station_name (),
                              pmu_details_get_pmu_id ());

  g_object_set (self->header_bar, "subtitle", subtitle, NULL);

  g_free (subtitle);

  pmu_server_start_thread (self);
  pmu_spi_start_thread (self);
}

PmuWindow *
pmu_window_new (PmuApp *app)
{
  g_assert (GTK_IS_APPLICATION (app));

  return g_object_new (PMU_TYPE_WINDOW,
                       "application", app,
                       NULL);
}
