/*
 * cpceg.h
 *
 * Copyright 2021 AmatCoder
 *
 * This file is part of CPCEG.
 *
 * CPCEG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * CPCEG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CPCEG; if not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

#ifndef __CPCEG_H__
#define __CPCEG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

void gtk_create_window_new (void);
int gtk_loop (void);
void gtk_update_cairo_surface (unsigned char* frame, int offset_x, int offset_y, int stride);

G_END_DECLS

#endif
