/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/temp-buf.h"

#include "core/gimpbrush.h"
#include "core/gimpmypaintbrush.h"
#include "core/gimptoolinfo.h"
#include "core/mypaintbrush-brushsettings.h"
#include "core/gimpmypaintbrush-private.hpp"
#include "core/gimpcurve.h"
  // for BRUSH VIEW
#include "core/gimp.h"
#include "core/gimpdatafactory.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "widgets/gimpview.h"
#include "widgets/gimpviewrenderer.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimppopupbutton.h"
#include "widgets/gimpcontainerbox.h"


#include "paint/gimpmypaintoptions.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpspinscale.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-constructors.h"
#include "widgets/gimppopupbutton.h"
#include "widgets/gimpcurveview.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpairbrushtool.h"
#include "gimpclonetool.h"
#include "gimpconvolvetool.h"
#include "gimpdodgeburntool.h"
#include "gimperasertool.h"
#include "gimphealtool.h"
#include "gimpinktool.h"
#include "gimpmypaintoptions-gui.h"
#include "gimppenciltool.h"
#include "gimpperspectiveclonetool.h"
#include "gimpsmudgetool.h"
#include "gimptooloptions-gui.h"
#include "gimpmypaintbrushoptions-gui.h"
}; // extern "C"

#include "base/delegators.hpp"
#include "base/scopeguard.hpp"
#include "base/glib-cxx-utils.hpp"
#include "gimptooloptions-gui-cxx.hpp"
#include "paint/gimpmypaintoptions-history.hpp"

#include "gimp-intl.h"

#include "gimpmypaint-gui-base.hpp"
#include "gimpmypaintbrusheditor.hpp"

///////////////////////////////////////////////////////////////////////////////
class MypaintDetailOptionsPopupPrivate {
  static const int BRUSH_VIEW_SIZE = 256;

  GimpContainer*         container;
  GimpContext*           context;
  Delegator::Connection* brush_changed_handler;
  PageRemindAction       page_reminder;
  
public:
  MypaintDetailOptionsPopupPrivate(GimpContainer* ctn, GimpContext* ctx) 
    : page_reminder(NULL)
  {
    container = ctn;
    context   = ctx;

    brush_changed_handler = NULL;
  }
  
  ~MypaintDetailOptionsPopupPrivate();
  void create(GObject  *button,
              GtkWidget **result);
  void destroy(GObject* object);
  void update_brush (GObject *adjustment);
  void notify_brush (GObject *brush, GParamSpec *pspec);
  void brush_changed (GObject *object, GimpData *brush_data);

};


void
MypaintDetailOptionsPopupPrivate::update_brush (GObject* object)
{
  GtkAdjustment *adjustment = GTK_ADJUSTMENT(object);
  gdouble              d_value_brush;
  gdouble              d_value_adj;
  gint                 i_value_brush;
  gint                 i_value_adj;
  GimpMypaintBrush    *brush;
  gchar               *prop_name;
  GtkAdjustment       *adj;

  g_return_if_fail (G_IS_OBJECT (adj));
  g_return_if_fail (G_IS_OBJECT (context));

  brush = gimp_context_get_mypaint_brush (context);

}

void
MypaintDetailOptionsPopupPrivate::notify_brush (GObject* object,
                                   GParamSpec         *pspec)
{
  GimpMypaintBrush *brush = GIMP_MYPAINT_BRUSH (object);
  GtkAdjustment *adj     = NULL;
  gdouble        d_value = 0.0;
  gint           i_value = 0;
  
  g_print ("notify_brush: %s\n", pspec->name);

  if (! strcmp (pspec->name, "shape"))
    {
    }
  else
    {
    }
}

void
MypaintDetailOptionsPopupPrivate::brush_changed (GObject*  object,
                                    GimpData* brush_data)
{
  GimpMypaintBrush        *brush        = GIMP_MYPAINT_BRUSH (brush_data);
  gdouble                  radius       = 0.0;
  gdouble                  ratio        = 0.0;
  gdouble                  angle        = 0.0;
  gboolean                 editable     = false;

  g_print("brush changed\n");

}

MypaintDetailOptionsPopupPrivate::~MypaintDetailOptionsPopupPrivate ()
{
}

void
MypaintDetailOptionsPopupPrivate::destroy (GObject* object)
{
  if (container) {
    g_object_unref (G_OBJECT (container));
    container = NULL;
  }

  if (context) {
    if (brush_changed_handler) {
      delete brush_changed_handler;
      brush_changed_handler = NULL;
    }
  }
}

void
MypaintDetailOptionsPopupPrivate::create (GObject* object,
                             GtkWidget **result)
{
  GtkButton*                     button = GTK_BUTTON(object);
  GimpContainerEditor           *editor;
  GimpBrush                     *brush;
  GtkWidget                     *vbox;
  GtkWidget                     *frame;
  GtkWidget                     *box;
  GtkWidget                     *table;
  GtkWidget                     *frame2;
  GimpViewType                   view_type = GIMP_VIEW_TYPE_GRID;
  GimpViewSize                   view_size = GIMP_VIEW_SIZE_MEDIUM;
  gint                           view_border_width = 1;
  gint                           default_view_size = GIMP_VIEW_SIZE_MEDIUM;
  GimpToolOptionsTableIncrement  inc = gimp_tool_options_table_increment (FALSE);
  GList                         *children;
  GtkAdjustment                 *adj = NULL;

  // 
  // Brush Selector
  //
  container = gimp_data_factory_get_container (context->gimp->brush_factory);
  brush     = gimp_context_get_brush (context);

  g_object_ref(G_OBJECT(container));
  
  g_return_if_fail (GIMP_IS_CONTAINER (container));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (view_size >  0 &&
                    view_size <= GIMP_VIEWABLE_MAX_BUTTON_SIZE);
  g_return_if_fail (view_border_width >= 0 &&
                    view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH);

  *result    = gtk_hbox_new (FALSE, 1);
  gtk_widget_show (*result);
  
  editor = GIMP_CONTAINER_EDITOR (
    g_object_new (GIMP_TYPE_CONTAINER_EDITOR,
      "view-type", view_type,
      "container", container,
      "context",   context,
      "view-size", view_size,
      "view-border-width", view_border_width,
      NULL));
  gimp_container_view_set_reorderable (GIMP_CONTAINER_VIEW (editor->view),
                                       FALSE);

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (editor->view),
                                       6  * (default_view_size +
                                             2 * view_border_width),
                                       10 * (default_view_size +
                                             2 * view_border_width));

  gtk_box_pack_start (GTK_BOX (*result), GTK_WIDGET (editor), TRUE, TRUE, 0);      
  gtk_widget_show (GTK_WIDGET (editor));
  

  brush_changed_handler = 
    g_signal_connect_delegator (G_OBJECT(context),
                                gimp_context_type_to_signal_name (GIMP_TYPE_BRUSH),
                                Delegator::delegator(this, &MypaintDetailOptionsPopupPrivate::brush_changed));

/*
//  g_signal_connect (brush, "notify", G_CALLBACK (notify_brush), p);
*/
  if (context && brush)
    brush_changed (G_OBJECT(context), GIMP_DATA(brush));
  children = gtk_container_get_children (GTK_CONTAINER (table));  
  gimp_tool_options_setup_popup_layout (children, FALSE);
  
  g_signal_connect_delegator (G_OBJECT (*result), "destroy", 
                              Delegator::delegator(this, &MypaintDetailOptionsPopupPrivate::destroy));

  //
  // Dynamics Editor
  // 
  MypaintBrushEditorPrivate* editor_priv = new MypaintBrushEditorPrivate(GIMP_TOOL_OPTIONS(context), MypaintBrushEditorPrivate::TAB);
  editor_priv->set_page_reminder(&page_reminder);
  GtkWidget* dynamics_editor = editor_priv->create();
  gtk_widget_show(dynamics_editor);
  
  gtk_box_pack_start (GTK_BOX (*result), GTK_WIDGET (dynamics_editor), TRUE, TRUE, 0);      
}


///////////////////////////////////////////////////////////////////////////////
class MypaintOptionsGUIPrivate : public MypaintGUIPrivateBase {
  bool is_toolbar;
  
public:
  MypaintOptionsGUIPrivate(GimpToolOptions* options, bool toolbar);
  GtkWidget* create();

  void destroy(GObject* o);
  void reset_size(GObject *o);
};


MypaintOptionsGUIPrivate::
MypaintOptionsGUIPrivate(GimpToolOptions* opts, bool toolbar) :
  MypaintGUIPrivateBase(opts), is_toolbar(toolbar)
{
}

GtkWidget *
MypaintOptionsGUIPrivate::create ()
{
  GObject            *config  = G_OBJECT (options);
  GtkWidget          *vbox    = gimp_tool_options_gui_full (GIMP_TOOL_OPTIONS(options), is_toolbar);
  GtkWidget          *hbox;
  GtkWidget          *frame;
  GtkWidget          *table;
  GtkWidget          *menu;
  GtkWidget          *scale;
  GtkWidget          *label;
  GtkWidget          *button;
  GtkWidget          *incremental_toggle = NULL;
  GType             tool_type;
  GList            *children;
  GimpToolOptionsTableIncrement inc = gimp_tool_options_table_increment (is_toolbar);  

  tool_type = GIMP_TOOL_OPTIONS(options)->tool_info->tool_type;
  GHashTableHolder<gchar*, MyPaintBrushSettings*> brush_settings_dict(mypaint_brush_get_brush_settings_dict());

  /*  the main table  */
  table = gimp_tool_options_table (3, is_toolbar);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  the opacity scale  */
  scale = gimp_prop_opacity_spin_scale_new (config, "opacity",
                                            _("Opacity"));
//  gtk_widget_set_size_request (scale, 200, -1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  the brush  */
    {
      GtkWidget *button;
      MypaintOptionsPropertyGUIPrivate* scale_obj;
      
      if (is_toolbar)
        button = gimp_mypaint_brush_button_with_popup (config);
      else {
        button = gimp_mypaint_brush_button_with_popup (config);
/*        button = gimp_prop_brush_box_new (NULL, GIMP_CONTEXT(options),
                                          _("MypaintBrush"), 2,
                                          "mypaint-brush-view-type", "mypaint-brush-view-size",
                                          "gimp-mypaint-brush-editor");*/
      }
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      /* brush size */
      if (is_toolbar)
        hbox = vbox;
      else {
        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
        gtk_widget_show (hbox);
      }

      scale_obj = 
        new MypaintOptionsPropertyGUIPrivate(options, brush_settings_dict.ptr(), "radius-logarithmic");
      scale = scale_obj->create();
      gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
      gtk_widget_show (scale);

      button = gimp_stock_button_new (GIMP_STOCK_RESET, NULL);
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_image_set_from_stock (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (button))),
                                GIMP_STOCK_RESET, GTK_ICON_SIZE_MENU);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_signal_connect_delegator (G_OBJECT(button), "clicked",
                                  Delegator::delegator(this, &MypaintOptionsGUIPrivate::reset_size));

      gimp_help_set_help_data (button,
                               _("Reset size to brush's native size"), NULL);

      if (is_toolbar)
        hbox = vbox;
      else {
        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
        gtk_widget_show (hbox);
      }

      scale_obj = 
        new MypaintOptionsPropertyGUIPrivate(options, brush_settings_dict.ptr(), "slow-tracking");
      scale = scale_obj->create();
      gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
      gtk_widget_show (scale);

      if (is_toolbar)
        hbox = vbox;
      else {
        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
        gtk_widget_show (hbox);
      }
      scale_obj = 
        new MypaintOptionsPropertyGUIPrivate(options, brush_settings_dict.ptr(), "hardness");
      scale = scale_obj->create();
      gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
      gtk_widget_show (scale);

//      frame = dynamics_options_gui (options, tool_type, is_toolbar);
//      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
//      gtk_widget_show (frame);
    }

  if (is_toolbar) {
    children = gtk_container_get_children (GTK_CONTAINER (vbox));  
    gimp_tool_options_setup_popup_layout (children, FALSE);

    GimpViewSize   view_size = GIMP_VIEW_SIZE_MEDIUM;
    GimpContext*   context   = GIMP_CONTEXT (config);
    GimpContainer* container = gimp_data_factory_get_container (context->gimp->brush_factory);
    
    const gchar* prop_name = gimp_context_type_to_prop_name (gimp_container_get_children_type (container));

    GtkWidget* label_widget = gimp_prop_view_new (G_OBJECT (context), prop_name,
                                     context, view_size);
    gtk_widget_show (label_widget);
  
    MypaintDetailOptionsPopupPrivate* priv = new MypaintDetailOptionsPopupPrivate(container, context);
  
    GtkWidget* brush_button = gimp_tool_options_button_with_popup (label_widget,
                                Delegator::delegator(priv, &MypaintDetailOptionsPopupPrivate::create),
                                priv);
    gtk_box_pack_start(GTK_BOX(hbox), brush_button, FALSE, FALSE, 0);
    gtk_widget_show(brush_button);
   }

  g_object_set_cxx_object(G_OBJECT(vbox), "behavior-options-private", this);
  gtk_widget_show(vbox);
  return vbox;
}

void
MypaintOptionsGUIPrivate::reset_size (GObject* o)
{
/*
 GimpMypaintBrush *brush = gimp_context_get_brush (GIMP_CONTEXT (paint_options));

 if (brush)
   {
     g_object_set (paint_options,
                   "brush-size", (gdouble) MAX (brush->mask->width,
                                                brush->mask->height),
                   NULL);
   }
*/
}
///////////////////////////////////////////////////////////////////////////////
/*  public functions  */
extern "C" {

GtkWidget *
gimp_mypaint_options_gui (GimpToolOptions *tool_options)
{
  g_print("MYPAINTOPTIONSGUI::create...\n");
  MypaintOptionsGUIPrivate* priv = new MypaintOptionsGUIPrivate(tool_options, false);
  MypaintBrushEditorPrivate* editor_priv = new MypaintBrushEditorPrivate(tool_options);
  GtkWidget* priv_widget = priv->create();
  GtkWidget* editor_widget = editor_priv->create();

  GtkWidget* options_box = gtk_notebook_new();
  gtk_widget_show (options_box);
//  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(options_box), FALSE);

//  gtk_notebook_insert_page(GTK_NOTEBOOK(options_box), brushsetting_vbox, label, 0);
//  gtk_notebook_insert_page(GTK_NOTEBOOK(editor->options_box), brushinputs, label, 1);
//  gtk_notebook_insert_page(GTK_NOTEBOOK(editor->options_box), brushicon, label, 2);
  gtk_notebook_insert_page(GTK_NOTEBOOK(options_box), priv_widget, gtk_label_new(_("Basic")), 0);
  gtk_notebook_insert_page(GTK_NOTEBOOK(options_box), editor_widget, gtk_label_new("Details"), 1);
//  gtk_notebook_set_current_page (GTK_NOTEBOOK(options_box), 1);
#if 0
  GtkWidget*options_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_show (options_box);
  g_print("MYPAINTOPTIONSGUI::add PRIV_WIDGET=%lx\n", (gulong)(gpointer)priv_widget);
  gtk_box_pack_start(GTK_BOX(options_box), priv_widget, TRUE, TRUE, 2);  
  g_print("MYPAINTOPTIONSGUI::add EDITOR_WIDGET=%lx\n", (gulong)(gpointer)editor_widget);
  gtk_box_pack_start(GTK_BOX(options_box), editor_widget, TRUE, TRUE, 2);  
#endif
  return options_box;
}

GtkWidget *
gimp_mypaint_options_gui_horizontal (GimpToolOptions *tool_options)
{
  MypaintOptionsGUIPrivate* priv = new MypaintOptionsGUIPrivate(tool_options, true);
  return priv->create();
}

}; // extern "C"
