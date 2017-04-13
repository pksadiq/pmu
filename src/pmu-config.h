/* pmu-config.h
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

char *channel_names[] = {
  /* 4 Phasors */
  "VA              ",
  "VB              ",
  "VC              ",

  /* 3 Analog values */
  "ANALOG1         ",
  "ANALOG2         ",
  "ANALOG3         ",

  /* 16 digital breakers */
  "BREAKER 1 STATUS",
  "BREAKER 2 STATUS",
  "BREAKER 3 STATUS",
  "BREAKER 4 STATUS",
  "BREAKER 5 STATUS",
  "BREAKER 6 STATUS",
  "BREAKER 7 STATUS",
  "BREAKER 8 STATUS",
  "BREAKER 9 STATUS",
  "BREAKER A STATUS",
  "BREAKER B STATUS",
  "BREAKER C STATUS",
  "BREAKER D STATUS",
  "BREAKER E STATUS",
  "BREAKER F STATUS",
  "BREAKER G STATUS",
  NULL,
};
