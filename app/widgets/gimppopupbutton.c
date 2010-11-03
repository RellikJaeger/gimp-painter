/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPopup.c
 * Copyright (C) 2003-2005 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"
#include "core/gimpmarshal.h"

#include "gimpviewrenderer.h"

#include "gimppopupbutton.h"

#include "gimp-intl.h"
enum
{
  CANCEL = 0,
  CONFIRM,
  LAST_SIGNAL
};


static void     gimp_popup_finalize     (GObject            *object);

static void     gimp_popup_map          (GtkWidget          *widget);
static gboolean gimp_popup_button_press (GtkWidget          *widget,
                                          GdkEventButton     *bevent);
static gboolean gimp_popup_key_press    (GtkWidget          *widget,
                                          GdkEventKey        *kevent);

static void     gimp_popup_real_cancel  (GimpPopup *popup);
static void     gimp_popup_real_confirm (GimpPopup *popup);


G_DEFINE_TYPE (GimpPopup, gimp_popup, GTK_TYPE_WINDOW)

#define parent_class gimp_popup_parent_class

static guint popup_signals[LAST_SIGNAL];


static void
gimp_popup_class_init (GimpPopupClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet  *binding_set;

  popup_signals[CANCEL] =
    g_signal_new ("cancel",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpPopupClass, cancel),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  popup_signals[CONFIRM] =
    g_signal_new ("confirm",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpPopupClass, confirm),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize           = gimp_popup_finalize;

  widget_class->map                = gimp_popup_map;
  widget_class->button_press_event = gimp_popup_button_press;
  widget_class->key_press_event    = gimp_popup_key_press;

  klass->cancel                    = gimp_popup_real_cancel;
  klass->confirm                   = gimp_popup_real_confirm;


  binding_set = gtk_binding_set_by_class (klass);

  gtk_binding_entry_add_signal (binding_set, GDK_Escape, 0,
                                "cancel", 0);
/*
  gtk_binding_entry_add_signal (binding_set, GDK_Return, 0,
                                "confirm", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Enter, 0,
                                "confirm", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_ISO_Enter, 0,
                                "confirm", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_space, 0,
                                "confirm", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Space, 0,
                                "confirm", 0);
*/
}

static void
gimp_popup_init (GimpPopup *popup)
{
//  popup->view_type         = GIMP_VIEW_TYPE_LIST;
  popup->default_view_size = GIMP_VIEW_SIZE_SMALL;
  popup->view_size         = GIMP_VIEW_SIZE_SMALL;
  popup->view_border_width = 1;

  popup->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (popup->frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (popup), popup->frame);
  gtk_widget_show (popup->frame);
}

static void
gimp_popup_finalize (GObject *object)
{
/*
  GimpPopup *popup = GIMP_POPUP (object);
*/
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_popup_grab_notify (GtkWidget *widget,
                        gboolean   was_grabbed)
{
  if (was_grabbed)
    return;

  /* ignore grabs on one of our children, like the scrollbar */
  if (gtk_widget_is_ancestor (gtk_grab_get_current (), widget))
    return;

  g_signal_emit (widget, popup_signals[CANCEL], 0);
}

static gboolean
gimp_popup_grab_broken_event (GtkWidget          *widget,
                              GdkEventGrabBroken *event)
{
  gimp_popup_grab_notify (widget, FALSE);

  return FALSE;
}

static void
gimp_popup_map (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (parent_class)->map (widget);

  /*  grab with owner_events == TRUE so the popup's widgets can
   *  receive events. we filter away events outside this toplevel
   *  away in button_press()
   */
  if (gdk_pointer_grab (gtk_widget_get_window (widget), TRUE,
                        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                        GDK_POINTER_MOTION_MASK,
                        NULL, NULL, GDK_CURRENT_TIME) == 0)
    {
      if (gdk_keyboard_grab (gtk_widget_get_window (widget), TRUE,
                             GDK_CURRENT_TIME) == 0)
        {
          gtk_grab_add (widget);

          g_signal_connect (widget, "grab-notify",
                            G_CALLBACK (gimp_popup_grab_notify),
                            widget);
          g_signal_connect (widget, "grab-broken-event",
                            G_CALLBACK (gimp_popup_grab_broken_event),
                            widget);

          return;
        }
      else
        {
          gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
                                      GDK_CURRENT_TIME);
        }
    }

  /*  if we could not grab, destroy the popup instead of leaving it
   *  around uncloseable.
   */
  g_signal_emit (widget, popup_signals[CANCEL], 0);
}

static gboolean
gimp_popup_button_press (GtkWidget      *widget,
                         GdkEventButton *bevent)
{
  GtkWidget *event_widget;
  gboolean   cancel = FALSE;

  event_widget = gtk_get_event_widget ((GdkEvent *) bevent);

  if (event_widget == widget)
    {
      GtkAllocation allocation;

      gtk_widget_get_allocation (widget, &allocation);

      /*  the event was on the popup, which can either be really on the
       *  popup or outside gimp (owner_events == TRUE, see map())
       */
      if (bevent->x < 0                ||
          bevent->y < 0                ||
          bevent->x > allocation.width ||
          bevent->y > allocation.height)
        {
          /*  the event was outsde gimp  */

          cancel = TRUE;
        }
    }
  else if (gtk_widget_get_toplevel (event_widget) != widget)
    {
      /*  the event was on a gimp widget, but not inside the popup  */

      cancel = TRUE;
    }

  if (cancel)
    g_signal_emit (widget, popup_signals[CANCEL], 0);

  return cancel;
}

static gboolean
gimp_popup_key_press (GtkWidget   *widget,
                      GdkEventKey *kevent)
{
  GtkBindingSet *binding_set;

  binding_set =
    gtk_binding_set_by_class (GIMP_POPUP_GET_CLASS (widget));

  /*  invoke the popup's binding entries manually, because otherwise
   *  the focus widget (GtkTreeView e.g.) would consume it
   */
  if (gtk_binding_set_activate (binding_set,
                                kevent->keyval,
                                kevent->state,
                                GTK_OBJECT (widget)))
    {
      return TRUE;
    }

  return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, kevent);
}

static void
gimp_popup_real_cancel (GimpPopup *popup)
{
  GtkWidget *widget = GTK_WIDGET (popup);

  if (gtk_grab_get_current () == widget)
    gtk_grab_remove (widget);

  gtk_widget_destroy (widget);
}

static void
gimp_popup_real_confirm (GimpPopup *popup)
{
  GtkWidget  *widget = GTK_WIDGET (popup);
  GimpObject *object;
/*
  object = gimp_context_get_by_type (popup->context,
                                     gimp_get_children_type (popup->container));
  gimp_context_set_by_type (popup->orig_context,
                            gimp_get_children_type (popup->container),
                            object);
*/
  if (gtk_grab_get_current () == widget)
    gtk_grab_remove (widget);

  gtk_widget_destroy (widget);
}
/*
static void
gimp_popup_context_changed (GimpContext        *context,
                            GimpPopup       *popup)
{
  GdkEvent *current_event;
  gboolean  confirm = FALSE;

  current_event = gtk_get_current_event ();

  if (current_event)
    {
      if (((GdkEventAny *) current_event)->type == GDK_BUTTON_PRESS ||
          ((GdkEventAny *) current_event)->type == GDK_BUTTON_RELEASE)
        confirm = TRUE;

      gdk_event_free (current_event);
    }

  if (confirm)
    g_signal_emit (popup, popup_signals[CONFIRM], 0);
}
*/

GtkWidget *
gimp_popup_new (GtkWidget *view)
{
  GimpPopup *popup;

  popup = g_object_new (GIMP_TYPE_POPUP,
                        "type", GTK_WINDOW_POPUP,
                        NULL);
  gtk_window_set_resizable (GTK_WINDOW (popup), FALSE);

  popup->default_view_size = 0;
  popup->view_size         = 0;
  popup->view_border_width = 0;

  gimp_popup_set_view (popup, view);

  return GTK_WIDGET (popup);
}

void
gimp_popup_show (GimpPopup *popup,
                 GtkWidget *widget)
{
  GdkScreen      *screen;
  GtkRequisition  requisition;
  GtkAllocation   allocation;
  GdkRectangle    rect;
  gint            monitor;
  gint            orig_x;
  gint            orig_y;
  gint            x;
  gint            y;

  g_return_if_fail (GIMP_IS_POPUP (popup));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_widget_size_request (GTK_WIDGET (popup), &requisition);

  gtk_widget_get_allocation (widget, &allocation);
  gdk_window_get_origin (gtk_widget_get_window (widget), &orig_x, &orig_y);

  if (! gtk_widget_get_has_window (widget))
    {
      orig_x += allocation.x;
      orig_y += allocation.y;
    }

  screen = gtk_widget_get_screen (widget);

  monitor = gdk_screen_get_monitor_at_point (screen, orig_x, orig_y);
  gdk_screen_get_monitor_geometry (screen, monitor, &rect);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      x = orig_x + allocation.width - requisition.width;

      if (x < rect.x)
        x -= allocation.width - requisition.width;
    }
  else
    {
      x = orig_x;

      if (x + requisition.width > rect.x + rect.width)
        x += allocation.width - requisition.width;
    }

  y = orig_y + allocation.height;

  if (y + requisition.height > rect.y + rect.height)
    y = orig_y - requisition.height;

  gtk_window_move (GTK_WINDOW (popup), x, y);
  gtk_widget_show (GTK_WIDGET (popup));
}


void
gimp_popup_set_view_size (GimpPopup *popup,
                                    gint                view_size)
{
/*
  GtkWidget     *scrolled_win;
  GtkWidget     *viewport;
  GtkAllocation  allocation;

  g_return_if_fail (GIMP_IS_POPUP (popup));

  scrolled_win = GIMP_BOX (popup->editor->view)->scrolled_win;
  viewport     = gtk_bin_get_child (GTK_BIN (scrolled_win));

  gtk_widget_get_allocation (viewport, &allocation);

  view_size = CLAMP (view_size, GIMP_VIEW_SIZE_TINY,
                     MIN (GIMP_VIEW_SIZE_GIGANTIC,
                          allocation.width - 2 * popup->view_border_width));
*/
  if (view_size != popup->view_size)
    {
      popup->view_size = view_size;
/*
      gimp_view_set_view_size (popup->editor->view,
                                         popup->view_size,
                                         popup->view_border_width);
*/
    }
}


void
gimp_popup_set_view (GimpPopup *popup, GtkWidget *view)
{
//  GtkWidget  *button;

  popup->view = view;
  /*
  gtk_widget_set_size_request (view,
                               6  * (popup->default_view_size +
                                     2 * popup->view_border_width),
                               10 * (popup->default_view_size +
                                     2 * popup->view_border_width));
  */
  gtk_container_add (GTK_CONTAINER (popup->frame), GTK_WIDGET (popup->view));
  gtk_widget_show (GTK_WIDGET (popup->view));

  gtk_widget_grab_focus (GTK_WIDGET (popup->view));
}
/*  private functions  */
/* no private functions */


/**
 * GimpPopupButton
 */

enum
{
  PROP_0 = 0,
  PROP_POPUP_VIEW_SIZE
};

enum
{
  CREATE_VIEW = 0,
  POPUP_BUTTON_LAST_SIGNAL
};


static void     gimp_popup_button_finalize     (GObject            *object);
static void     gimp_popup_button_set_property (GObject            *object,
                                                   guint               property_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);
static void     gimp_popup_button_get_property (GObject            *object,
                                                   guint               property_id,
                                                   GValue             *value,
                                                   GParamSpec         *pspec);
static gboolean gimp_popup_button_scroll_event (GtkWidget          *widget,
                                                   GdkEventScroll     *sevent);
static void     gimp_popup_button_clicked      (GtkButton          *button);

static void     gimp_popup_button_popup_closed (GimpPopup *popup,
                                                   GimpPopupButton *button);

static guint popup_button_signals[POPUP_BUTTON_LAST_SIGNAL];

G_DEFINE_TYPE (GimpPopupButton, gimp_popup_button, GIMP_TYPE_BUTTON)
#undef parent_class
#define parent_class gimp_popup_button_parent_class


static void
gimp_popup_button_class_init (GimpPopupButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  object_class->finalize     = gimp_popup_button_finalize;
  object_class->get_property = gimp_popup_button_get_property;
  object_class->set_property = gimp_popup_button_set_property;

  widget_class->scroll_event = gimp_popup_button_scroll_event;

  button_class->clicked      = gimp_popup_button_clicked;

  popup_button_signals[CREATE_VIEW] =
    g_signal_new ("create-view",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);

  g_object_class_install_property (object_class, PROP_POPUP_VIEW_SIZE,
                                   g_param_spec_int ("popup-view-size",
                                                     NULL, NULL,
                                                     GIMP_VIEW_SIZE_TINY,
                                                     GIMP_VIEW_SIZE_GIGANTIC,
                                                     GIMP_VIEW_SIZE_SMALL,
                                                     GIMP_PARAM_READWRITE));
}

static void
gimp_popup_button_init (GimpPopupButton *button)
{
  button->popup_view_type   = GIMP_VIEW_TYPE_LIST;
  button->popup_view_size   = GIMP_VIEW_SIZE_SMALL;

  button->button_view_size  = GIMP_VIEW_SIZE_SMALL;
  button->view_border_width = 1;
}

static void
gimp_popup_button_finalize (GObject *object)
{
//  GimpPopupButton *button = GIMP_POPUP_BUTTON (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_popup_button_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpPopupButton *button = GIMP_POPUP_BUTTON (object);

  switch (property_id)
    {
   case PROP_POPUP_VIEW_SIZE:
      gimp_popup_button_set_view_size (button, g_value_get_int (value));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_popup_button_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpPopupButton *button = GIMP_POPUP_BUTTON (object);

  switch (property_id)
    {
   case PROP_POPUP_VIEW_SIZE:
      g_value_set_int (value, button->popup_view_size);
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_popup_button_scroll_event (GtkWidget      *widget,
                                GdkEventScroll *sevent)
{
/*
  GimpPopupButton *button = GIMP_POPUP_BUTTON (widget);
  GimpObject         *object;
  gint                index;

  object = gimp_context_get_by_type (button->context,
                                     gimp_get_children_type (button->container));

  index = gimp_get_child_index (button->container, object);

  if (index != -1)
    {
      gint n_children;
      gint new_index = index;

      n_children = gimp_get_n_children (button->container);

      if (sevent->direction == GDK_SCROLL_UP)
        {
          if (index > 0)
            new_index--;
          else
            new_index = n_children - 1;
        }
      else if (sevent->direction == GDK_SCROLL_DOWN)
        {
          if (index == (n_children - 1))
            new_index = 0;
          else
            new_index++;
        }

      if (new_index != index)
        {
          object = gimp_get_child_by_index (button->container,
                                                      new_index);

          if (object)
            gimp_context_set_by_type (button->context,
                                      gimp_get_children_type (button->container),
                                      object);
        }
    }
*/
  return TRUE;
}

static void
gimp_popup_button_clicked (GtkButton *button)
{
  GimpPopupButton *popup_button = GIMP_POPUP_BUTTON (button);
  GtkWidget       *popup;
  GtkWidget       *view;
  
  view = gimp_popup_button_create_view (popup_button);
  
  g_return_if_fail (GTK_IS_WIDGET (view));

  popup = gimp_popup_new (view);

  g_signal_connect (popup, "cancel",
                    G_CALLBACK (gimp_popup_button_popup_closed),
                    button);
  g_signal_connect (popup, "confirm",
                    G_CALLBACK (gimp_popup_button_popup_closed),
                    button);

  gimp_popup_show (GIMP_POPUP (popup), GTK_WIDGET (button));
}

static void
gimp_popup_button_popup_closed (GimpPopup *popup,
                                   GimpPopupButton *button)
{
/*
  gimp_popup_button_set_view_type (button,
                                   gimp_popup_get_view_type (popup));
  gimp_popup_button_set_view_size (button,
                                   gimp_popup_get_view_size (popup));
*/
}


/*  public functions  */

GtkWidget *
gimp_popup_button_new (GtkWidget *label)
{
  GimpPopupButton *button;

  button = g_object_new (GIMP_TYPE_POPUP_BUTTON,
                         NULL);

  button->button_view_size  = 0;
  button->view_border_width = 0;

  button->label = label;
  gtk_container_add (GTK_CONTAINER (button), button->label);
  gtk_widget_show (button->label);

  return GTK_WIDGET (button);
}

gint
gimp_popup_button_get_view_size (GimpPopupButton *button)
{
  g_return_val_if_fail (GIMP_IS_POPUP_BUTTON (button), GIMP_VIEW_SIZE_SMALL);

  return button->popup_view_size;
}

void
gimp_popup_button_set_view_size (GimpPopupButton *button,
                                    gint                view_size)
{
  g_return_if_fail (GIMP_IS_POPUP_BUTTON (button));

  if (view_size != button->popup_view_size)
    {
      button->popup_view_size = view_size;

      g_object_notify (G_OBJECT (button), "popup-view-size");
    }
}

GtkWidget*
gimp_popup_button_create_view (GimpPopupButton *button)
{
  GtkWidget *result = NULL;
  g_signal_emit (G_OBJECT(button), popup_button_signals[CREATE_VIEW], 0, &result, NULL);
  
  g_return_val_if_fail (GTK_IS_WIDGET (result), NULL);
  
  return result;
}

