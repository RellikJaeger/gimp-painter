#ifndef __GIMP_MYPAINT_BRUSH_PRIVATE_H__
#define __GIMP_MYPAINT_BRUSH_PRIVATE_H__

extern "C" {
#include <glib.h>
#include <cairo.h>
};
#include "mypaintbrush-mapping.hpp"
#include "mypaintbrush-enum-settings.h"

extern "C++" {
#include "base/scopeguard.hpp"
class GimpMypaintBrushPrivate {
  public:
  struct Value {
    float base_value;
    Mapping *mapping;
  };
  private:
  char *parent_brush_name;
  char *group;
  Value  settings[BRUSH_MAPPING_COUNT];
  bool   switches[BRUSH_BOOL_COUNT];
  gchar* text[BRUSH_TEXT_COUNT];
  cairo_surface_t *icon_image;

  public:
  GimpMypaintBrushPrivate();
  ~GimpMypaintBrushPrivate();
  Value* get_setting(int index) {
    g_assert (0 <= index && index < BRUSH_MAPPING_COUNT);
    return &settings[index];
  }
  void set_base_value (int index, float value);
  void set_bool_value (int index, bool value);
  bool get_bool_value (int index);
  float get_base_value (int index);
  void allocate_mapping (int index);
  void deallocate_mapping (int index);
  char* get_parent_brush_name();
  void  set_parent_brush_name(char *name);
  char* get_group();
  void  set_group(char *name);
	void get_new_preview(guchar* dest, int width, int height, int bytes, int stride);
	void set_icon_image(cairo_surface_t* image);
	cairo_surface_t* get_icon_image();
	GimpMypaintBrushPrivate* duplicate();
};

void destroy_gimp_mypaint_brush_private(gpointer data);

}
#endif
