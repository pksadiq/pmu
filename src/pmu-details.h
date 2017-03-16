/* pmu-details.h
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

#define PMU_TYPE_DETAILS (pmu_details_get_type ())

G_DECLARE_FINAL_TYPE (PmuDetails, pmu_details, PMU, DETAILS, GObject)

void        pmu_details_save_settings    (void);
gchar      *pmu_details_get_station_name (void);
gchar      *pmu_details_get_admin_ip     (void);
guint       pmu_details_get_port_number  (void);
guint       pmu_details_get_pmu_id       (void);
gboolean    pmu_details_get_is_first_run (void);
PmuDetails *pmu_details_get_default      (void);

G_END_DECLS
