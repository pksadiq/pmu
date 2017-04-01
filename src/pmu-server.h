/* pmu-server.h
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

#pragma once

#include <gtk/gtk.h>

#include "pmu-types.h"

G_BEGIN_DECLS

#define REQUEST_HEADER_SIZE 4
#define PMU_TYPE_SERVER (pmu_server_get_type ())

G_DECLARE_FINAL_TYPE (PmuServer, pmu_server, PMU, SERVER, GObject)

void          pmu_server_start_thread        (PmuWindow *window);
PmuServer    *pmu_server_get_default         (void);
GMainContext *pmu_server_get_default_context (void);
gboolean      pmu_server_start               (PmuWindow *window);
gboolean      pmu_server_stop                (gpointer user_data);
gboolean      pmu_server_is_running          (void);

G_END_DECLS
