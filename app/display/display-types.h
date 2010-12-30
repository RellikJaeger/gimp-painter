/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#ifndef __DISPLAY_TYPES_H__
#define __DISPLAY_TYPES_H__


#include "widgets/widgets-types.h"

#include "display/display-enums.h"


#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 9, 15)
#define USE_CAIRO_REGION
#endif

#ifndef USE_CAIRO_REGION
#define cairo_region_t GdkRegion
#define cairo_rectangle_int_t GdkRectangle
#define cairo_region_destroy gdk_region_destroy
#define cairo_region_union gdk_region_union
#define cairo_region_create_rectangle gdk_region_rectangle
#define cairo_region_union_rectangle gdk_region_union_with_rect
#define cairo_region_get_extents gdk_region_get_clipbox
#define cairo_region_subtract gdk_region_subtract
#define cairo_region_xor gdk_region_xor
#endif


typedef struct _GimpCanvas            GimpCanvas;
typedef struct _GimpCanvasItem        GimpCanvasItem;
typedef struct _GimpCanvasGroup       GimpCanvasGroup;

typedef struct _GimpDisplay           GimpDisplay;
typedef struct _GimpDisplayShell      GimpDisplayShell;

typedef struct _GimpImageWindow       GimpImageWindow;

typedef struct _GimpCursorView        GimpCursorView;
typedef struct _GimpNavigationEditor  GimpNavigationEditor;
typedef struct _GimpScaleComboBox     GimpScaleComboBox;
typedef struct _GimpStatusbar         GimpStatusbar;

typedef struct _Selection             Selection;


#endif /* __DISPLAY_TYPES_H__ */
