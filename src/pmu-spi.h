/* pmu-spi.h
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

#define PMU_TYPE_SPI (pmu_spi_get_type ())

G_DECLARE_FINAL_TYPE (PmuSpi, pmu_spi, PMU, SPI, GObject)

void          pmu_spi_start_thread        (PmuWindow *window);
PmuSpi    *pmu_spi_get_default         (void);
GMainContext *pmu_spi_get_default_context (void);
gboolean      pmu_spi_start               (gpointer user_data);
gboolean      pmu_spi_stop                (gpointer user_data);

G_END_DECLS
