/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpFilterLayer
 * Copyright (C) 2009  Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_FILTER_LAYER_H__
#define __GIMP_FILTER_LAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "core/gimpgrouplayer.h"


#define GIMP_TYPE_FILTER_LAYER            (gimp_filter_layer_get_type ())
#define GIMP_FILTER_LAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FILTER_LAYER, GimpFilterLayer))
#define GIMP_FILTER_LAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FILTER_LAYER, GimpFilterLayerClass))
#define GIMP_IS_FILTER_LAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FILTER_LAYER))
#define GIMP_IS_FILTER_LAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FILTER_LAYER))
#define GIMP_FILTER_LAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FILTER_LAYER, GimpFilterLayerClass))

typedef struct _GimpFilterLayerClass GimpFilterLayerClass;

struct _GimpFilterLayer
{
  GimpLayer  parent_instance;
};

struct _GimpFilterLayerClass
{
  GimpLayerClass  parent_class;
};

GType            gimp_filter_layer_get_type       (void) G_GNUC_CONST;
GimpLayer      * gimp_filter_layer_new            (GimpImage      *image);
GimpProjection * gimp_filter_layer_get_projection (GimpFilterLayer *group);

void             gimp_filter_layer_suspend_resize (GimpFilterLayer *group,
                                                  gboolean        push_undo);
void             gimp_filter_layer_resume_resize  (GimpFilterLayer *group,
                                                  gboolean        push_undo);
#ifdef __cplusplus
}

class FilterLayerInterface {
public:
  static GimpLayer*               new_instance(GimpImage* image);
  static FilterLayerInterface*    cast(gpointer obj);
  static bool                     is_instance(gpointer obj);

  virtual GimpProjection*         get_projection () = 0;
  virtual void                    suspend_resize (gboolean        push_undo) = 0;
  virtual void                    resume_resize  (gboolean        push_undo) = 0;
};

#endif


#endif /* __GIMP_FILTER_LAYER_H__ */
