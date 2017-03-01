/* pmu-app.h
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

G_BEGIN_DECLS

#define PMU_TYPE_APP (pmu_app_get_type ())

G_DECLARE_FINAL_TYPE (PmuApp, pmu_app, PMU, APP, GtkApplication)

PmuApp *pmu_app_new (void);

G_END_DECLS
