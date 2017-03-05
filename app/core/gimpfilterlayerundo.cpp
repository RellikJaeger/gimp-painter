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

extern "C" {
#include "config.h"

#include <gegl.h>

#include "core-types.h"

#include "gimpimage.h"
#include "gimpitemundo.h"
}

#include "gimpfilterlayer.h"
#include "gimpfilterlayerundo.h"
#include "base/delegators.hpp"
#include "base/glib-cxx-utils.hpp"
#include "base/glib-cxx-impl.hpp"
#include <type_traits>


struct FilterLayerUndo;

namespace GtkCXX {

struct FilterLayerUndo : virtual public ImplBase
{
  typedef FilterLayerUndoTraits Traits;
  FilterLayerUndo(GObject* obj) : ImplBase(obj) { };

  GimpImageBaseType  prev_type;

  static void class_init(GimpFilterLayerUndoClass* klass);

  virtual void init();
  virtual void constructed();
  virtual void pop        (GimpUndoMode  undo_mode, GimpUndoAccumulator * accum);
};

};

extern const char gimp_filter_layer_undo_name[] = "GimpFilterLayerUndo";
typedef GtkCXX::Traits<GimpItemUndo, GimpItemUndoClass, gimp_item_undo_get_type> ParentTraits;
typedef GtkCXX::ClassDefinition<gimp_filter_layer_undo_name,
                                GtkCXX::ClassHolder<ParentTraits, GimpFilterLayerUndo, GimpFilterLayerUndoClass>,
                                GtkCXX::FilterLayerUndo>
                                Class;

GType gimp_filter_layer_undo_get_type() {
  return Class::get_type();
}

void
GtkCXX::FilterLayerUndo::class_init (GimpFilterLayerUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  Class::__(&object_class->constructed ).bind< &GtkCXX::FilterLayerUndo::constructed >();

  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);
  Class::__(&undo_class->pop           ).bind< &GtkCXX::FilterLayerUndo::pop         >();

  ImplBase::class_init<GimpFilterLayerUndoClass, FilterLayerUndo>(klass);
}

void GtkCXX::FilterLayerUndo::init () {
}

void GtkCXX::FilterLayerUndo::constructed ()
{
  GObject     *group;

  if (G_OBJECT_CLASS (Class::parent_class)->constructed)
    G_OBJECT_CLASS (Class::parent_class)->constructed (g_object);

  g_assert (FilterLayerInterface::is_instance (GIMP_ITEM_UNDO (g_object)->item));

  group = G_OBJECT(GIMP_ITEM_UNDO (g_object)->item);

  switch (GIMP_UNDO (g_object)->undo_type)
    {
    case GIMP_UNDO_GROUP_LAYER_SUSPEND:
    case GIMP_UNDO_GROUP_LAYER_RESUME:
      break;

    case GIMP_UNDO_GROUP_LAYER_CONVERT:
      prev_type = (GimpImageBaseType)GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (GIMP_DRAWABLE (group)));
      break;

    default:
      g_assert_not_reached ();
    }
}

void GtkCXX::FilterLayerUndo::pop (GimpUndoMode         undo_mode,
                                   GimpUndoAccumulator *accum)
{
  GimpUndo*            undo;
  auto i_filter = FilterLayerInterface::cast(GIMP_ITEM_UNDO(g_object)->item);
  undo  = GIMP_UNDO(g_object);

  GIMP_UNDO_CLASS (Class::parent_class)->pop (undo, undo_mode, accum);

  switch (GIMP_UNDO(g_object)->undo_type){
  case GIMP_UNDO_GROUP_LAYER_SUSPEND:
  case GIMP_UNDO_GROUP_LAYER_RESUME:
    if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
        undo->undo_type == GIMP_UNDO_GROUP_LAYER_SUSPEND) ||
        (undo_mode       == GIMP_UNDO_MODE_REDO &&
            undo->undo_type == GIMP_UNDO_GROUP_LAYER_RESUME)) {
      /*    resume group layer auto-resizing  */

      i_filter->resume_resize(FALSE);
    } else {
      /*  suspend group layer auto-resizing  */

      i_filter->suspend_resize (FALSE);
    }
    break;

  case GIMP_UNDO_GROUP_LAYER_CONVERT:
  {
    GimpImageBaseType type;

    type = (GimpImageBaseType)GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (GIMP_DRAWABLE (g_object)));
    gimp_drawable_convert_type (GIMP_DRAWABLE (g_object), NULL,
        prev_type, FALSE);
    prev_type = type;
  }
  break;

  default:
    g_assert_not_reached ();
  }
}
