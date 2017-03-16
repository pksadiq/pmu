/* pmu-app.c
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
#include "pmu-setup-window.h"
#include "pmu-details.h"
#include "pmu-server.h"
#include "pmu-config.h"
#include "c37/c37.h"

#include "pmu-app.h"


struct _PmuApp
{
  GtkApplication parent_instance;

  int port;

  CtsConfig  *pmu_config_one;
  CtsConfig  *pmu_config_two;
};


G_DEFINE_TYPE (PmuApp, pmu_app, GTK_TYPE_APPLICATION)

static void pmu_app_show_about (GSimpleAction *action,
                                GVariant      *parameter,
                                gpointer       user_data);

static void pmu_app_quit       (GSimpleAction *action,
                                GVariant      *parameter,
                                gpointer       user_data);


static const GActionEntry app_entries[] = {
  { "about",  pmu_app_show_about },
  { "quit",   pmu_app_quit       },
};

static void pmu_app_show_about (GSimpleAction *action,
                                GVariant      *parameter,
                                gpointer       user_data)
{
  PmuApp      *self;
  const gchar *authors[] = {
    "Mohammed Sadiq <sadiq@sadiqpk.org>",
    NULL
  };

  self = PMU_APP (user_data);

  g_assert (G_IS_APPLICATION (self));
  g_assert (G_IS_SIMPLE_ACTION (action));

  gtk_show_about_dialog (gtk_application_get_active_window (GTK_APPLICATION (self)),
                         "program-name", ("PMU"),
                         "version", "0.1.0",
                         "copyright", "Copyright \xC2\xA9 2017 Mohammed Sadiq",
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "authors", authors,
                         // "artists", authors,
                         "logo-icon-name", "org.gnome.Todo",
                         "translator-credits", ("translator-credits"),
                         NULL);
}

static void pmu_app_quit (GSimpleAction *action,
                          GVariant      *parameter,
                          gpointer       user_data)
{
  GApplication *app = user_data;

  g_assert (G_IS_APPLICATION (app));
  g_assert (G_IS_SIMPLE_ACTION (action));

  g_application_quit (app);
}

static void
pmu_app_finalize (GObject *object)
{
  G_OBJECT_CLASS (pmu_app_parent_class)->finalize (object);
}

static void
pmu_app_activate (GApplication *app)
{
  GtkWidget *window;
  PmuApp    *self;
  gboolean   first_run;

  g_assert (GTK_IS_APPLICATION (app));

  self = PMU_APP (app);
  first_run = pmu_details_get_is_first_run ();

  if (first_run)
    {
      window = GTK_WIDGET (pmu_setup_window_new (PMU_APP (app)));
      gtk_window_present (GTK_WINDOW (window));
    }
  else
    {
      window = GTK_WIDGET (gtk_application_get_active_window (GTK_APPLICATION (app)));

      if (window == NULL)
        window = GTK_WIDGET (pmu_window_new (PMU_APP (app)));
    }

  gtk_window_present (GTK_WINDOW (window));
}

static void
pmu_app_update_pmu_config (PmuApp    *self,
                           CtsConfig *config)
{
  cts_config_set_pmu_count (config, 1);
  cts_config_set_station_name_of_pmu (config, 1, "Good", 4);
  guchar *b = cts_config_get_raw_data (config);
}

static void
pmu_app_startup (GApplication *app)
{
  PmuApp *self = PMU_APP (app);
  g_autofree char *css = NULL;
  g_autoptr(GtkCssProvider) css_provider = NULL;
  g_autoptr(GFile) file = NULL;
  const char *path;

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries,
                                   G_N_ELEMENTS (app_entries),
                                   app);
  pmu_details_get_default ();

  self->pmu_config_one = cts_config_get_default_config_one ();
  self->pmu_config_two = cts_config_get_default_config_two ();

  pmu_app_update_pmu_config (self, self->pmu_config_one);

  G_APPLICATION_CLASS (pmu_app_parent_class)->startup (app);

  css_provider = gtk_css_provider_new ();

  path = "resource:///org/sadiqpk/pmu/css/pmu.css";
  file = g_file_new_for_uri (path);

  gtk_css_provider_load_from_file (css_provider, file, NULL);
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (css_provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

static void
pmu_app_class_init (PmuAppClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

  object_class->finalize = pmu_app_finalize;

  application_class->activate = pmu_app_activate;
  application_class->startup = pmu_app_startup;
}

static void
pmu_app_init (PmuApp *self)
{
}

PmuApp *
pmu_app_new (void)
{
  return g_object_new (PMU_TYPE_APP,
                       "application-id", "org.sadiqpk.pmu",
                       "flags", G_APPLICATION_FLAGS_NONE,
                       NULL);
}
