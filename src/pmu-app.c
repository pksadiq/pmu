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

#include "pmu-app.h"


struct _PmuApp
{
  GtkApplication parent_instance;
};


G_DEFINE_TYPE (PmuApp, pmu_app, GTK_TYPE_APPLICATION)

static void
pmu_app_finalize (GObject *object)
{
  G_OBJECT_CLASS (pmu_app_parent_class)->finalize (object);
}

static void
pmu_app_activate (GApplication *app)
{
  PmuWindow *window;

  g_assert (GTK_IS_APPLICATION (app));

  window = PMU_WINDOW (gtk_application_get_active_window (GTK_APPLICATION (app)));

  if (window == NULL)
    window = pmu_window_new (PMU_APP (app));
  gtk_window_present (GTK_WINDOW (window));
}

static void
pmu_app_class_init (PmuAppClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

  object_class->finalize = pmu_app_finalize;

  application_class->activate = pmu_app_activate;
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
