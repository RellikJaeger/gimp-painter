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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"

#include "paint/gimpsourcecore.h"
#include "paint/gimpsourceoptions.h"

#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimpsourcetool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static gboolean      gimp_source_tool_has_display   (GimpTool            *tool,
                                                     GimpDisplay         *display);
static GimpDisplay * gimp_source_tool_has_image     (GimpTool            *tool,
                                                     GimpImage           *image);
static void          gimp_source_tool_control       (GimpTool            *tool,
                                                     GimpToolAction       action,
                                                     GimpDisplay         *display);
static void          gimp_source_tool_button_press  (GimpTool            *tool,
                                                     const GimpCoords    *coords,
                                                     guint32              time,
                                                     GdkModifierType      state,
                                                     GimpButtonPressType  press_type,
                                                     GimpDisplay         *display);
static void          gimp_source_tool_motion        (GimpTool            *tool,
                                                     const GimpCoords    *coords,
                                                     guint32              time,
                                                     GdkModifierType      state,
                                                     GimpDisplay         *display);
static void          gimp_source_tool_cursor_update (GimpTool            *tool,
                                                     const GimpCoords    *coords,
                                                     GdkModifierType      state,
                                                     GimpDisplay         *display);
static void          gimp_source_tool_modifier_key  (GimpTool            *tool,
                                                     GdkModifierType      key,
                                                     gboolean             press,
                                                     GdkModifierType      state,
                                                     GimpDisplay         *display);
static void          gimp_source_tool_oper_update   (GimpTool            *tool,
                                                     const GimpCoords    *coords,
                                                     GdkModifierType      state,
                                                     gboolean             proximity,
                                                     GimpDisplay         *display);

static void          gimp_source_tool_draw          (GimpDrawTool        *draw_tool);


G_DEFINE_TYPE (GimpSourceTool, gimp_source_tool, GIMP_TYPE_BRUSH_TOOL)

#define parent_class gimp_source_tool_parent_class


static void
gimp_source_tool_class_init (GimpSourceToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->has_display   = gimp_source_tool_has_display;
  tool_class->has_image     = gimp_source_tool_has_image;
  tool_class->control       = gimp_source_tool_control;
  tool_class->button_press  = gimp_source_tool_button_press;
  tool_class->motion        = gimp_source_tool_motion;
  tool_class->modifier_key  = gimp_source_tool_modifier_key;
  tool_class->oper_update   = gimp_source_tool_oper_update;
  tool_class->cursor_update = gimp_source_tool_cursor_update;

  draw_tool_class->draw     = gimp_source_tool_draw;
}

static void
gimp_source_tool_init (GimpSourceTool *source)
{
  source->show_source_outline = TRUE;
}

static gboolean
gimp_source_tool_has_display (GimpTool    *tool,
                              GimpDisplay *display)
{
  GimpSourceTool *source_tool = GIMP_SOURCE_TOOL (tool);

  return (display == source_tool->src_display ||
          GIMP_TOOL_CLASS (parent_class)->has_display (tool, display));
}

static GimpDisplay *
gimp_source_tool_has_image (GimpTool  *tool,
                            GimpImage *image)
{
  GimpSourceTool *source_tool = GIMP_SOURCE_TOOL (tool);
  GimpDisplay    *display;

  display = GIMP_TOOL_CLASS (parent_class)->has_image (tool, image);

  if (! display && source_tool->src_display)
    {
      if (image && gimp_display_get_image (source_tool->src_display) == image)
        display = source_tool->src_display;

      /*  NULL image means any display  */
      if (! image)
        display = source_tool->src_display;
    }

  return display;
}

static void
gimp_source_tool_control (GimpTool       *tool,
                          GimpToolAction  action,
                          GimpDisplay    *display)
{
  GimpSourceTool *source_tool = GIMP_SOURCE_TOOL (tool);

  /*  chain up early so the draw tool can undraw the source marker
   *  while we still know about source drawable and display
   */
  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      source_tool->src_display = NULL;
      g_object_set (GIMP_PAINT_TOOL (tool)->core,
                    "src-drawable", NULL,
                    NULL);
      break;
    }
}

static void
gimp_source_tool_button_press (GimpTool            *tool,
                               const GimpCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               GimpButtonPressType  press_type,
                               GimpDisplay         *display)
{
  GimpPaintTool  *paint_tool  = GIMP_PAINT_TOOL (tool);
  GimpSourceTool *source_tool = GIMP_SOURCE_TOOL (tool);
  GimpSourceCore *source      = GIMP_SOURCE_CORE (paint_tool->core);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
    {
      source->set_source = TRUE;

      source_tool->src_display = display;
    }
  else
    {
      source->set_source = FALSE;
    }

  GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                press_type, display);

  source_tool->src_x = source->src_x;
  source_tool->src_y = source->src_y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_source_tool_motion (GimpTool         *tool,
                         const GimpCoords *coords,
                         guint32           time,
                         GdkModifierType   state,
                         GimpDisplay      *display)
{
  GimpSourceTool *source_tool = GIMP_SOURCE_TOOL (tool);
  GimpPaintTool  *paint_tool  = GIMP_PAINT_TOOL (tool);
  GimpSourceCore *source      = GIMP_SOURCE_CORE (paint_tool->core);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  source_tool->src_x = source->src_x;
  source_tool->src_y = source->src_y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_source_tool_modifier_key (GimpTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               GimpDisplay     *display)
{
  GimpSourceTool    *source_tool = GIMP_SOURCE_TOOL (tool);
  GimpPaintTool     *paint_tool  = GIMP_PAINT_TOOL (tool);
  GimpSourceOptions *options     = GIMP_SOURCE_TOOL_GET_OPTIONS (tool);

  if (options->use_source && key == GDK_CONTROL_MASK)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      if (press)
        {
          paint_tool->status = source_tool->status_set_source;

          source_tool->show_source_outline = FALSE;
        }
      else
        {
          paint_tool->status = source_tool->status_paint;

          source_tool->show_source_outline = TRUE;
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
gimp_source_tool_cursor_update (GimpTool         *tool,
                                const GimpCoords *coords,
                                GdkModifierType   state,
                                GimpDisplay      *display)
{
  GimpSourceOptions  *options  = GIMP_SOURCE_TOOL_GET_OPTIONS (tool);
  GimpCursorType      cursor   = GIMP_CURSOR_MOUSE;
  GimpCursorModifier  modifier = GIMP_CURSOR_MODIFIER_NONE;

  if (options->use_source)
    {
      if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
        {
          cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
        }
      else if (! GIMP_SOURCE_CORE (GIMP_PAINT_TOOL (tool)->core)->src_drawable)
        {
          modifier = GIMP_CURSOR_MODIFIER_BAD;
        }
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_source_tool_oper_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              GimpDisplay      *display)
{
  GimpSourceTool    *source_tool = GIMP_SOURCE_TOOL (tool);
  GimpSourceOptions *options     = GIMP_SOURCE_TOOL_GET_OPTIONS (tool);

  if (proximity)
    {
      GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);

      if (options->use_source)
        paint_tool->status_ctrl = source_tool->status_set_source_ctrl;
      else
        paint_tool->status_ctrl = NULL;
    }

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  if (options->use_source)
    {
      GimpSourceCore *source = GIMP_SOURCE_CORE (GIMP_PAINT_TOOL (tool)->core);

      if (source->src_drawable == NULL)
        {
          if (state & GDK_CONTROL_MASK)
            {
              gimp_tool_replace_status (tool, display, "%s",
                                        source_tool->status_set_source);
            }
          else
            {
              gimp_tool_replace_status (tool, display, "%s%s%s",
                                        gimp_get_mod_name_control (),
                                        gimp_get_mod_separator (),
                                        source_tool->status_set_source);
            }
        }
      else
        {
          gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

          source_tool->src_x = source->src_x;
          source_tool->src_y = source->src_y;

          if (! source->first_stroke)
            {
              switch (options->align_mode)
                {
                case GIMP_SOURCE_ALIGN_YES:
                  source_tool->src_x = coords->x + source->offset_x;
                  source_tool->src_y = coords->y + source->offset_y;
                  break;

                case GIMP_SOURCE_ALIGN_REGISTERED:
                  source_tool->src_x = coords->x;
                  source_tool->src_y = coords->y;
                  break;

                default:
                  break;
                }
            }

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
        }
    }
}

static void
gimp_source_tool_draw (GimpDrawTool *draw_tool)
{
  GimpSourceTool    *source_tool = GIMP_SOURCE_TOOL (draw_tool);
  GimpSourceOptions *options     = GIMP_SOURCE_TOOL_GET_OPTIONS (draw_tool);
  GimpSourceCore    *source;

  source = GIMP_SOURCE_CORE (GIMP_PAINT_TOOL (draw_tool)->core);

  if (options->use_source && source->src_drawable && source_tool->src_display)
    {
      GimpDisplay   *tmp_display = draw_tool->display;
      gint           off_x;
      gint           off_y;

      draw_tool->display = source_tool->src_display;

      gimp_item_get_offset (GIMP_ITEM (source->src_drawable), &off_x, &off_y);

      if (source_tool->show_source_outline)
        gimp_brush_tool_draw_brush (GIMP_BRUSH_TOOL (source_tool),
                                    source_tool->src_x + off_x,
                                    source_tool->src_y + off_y,
                                    FALSE);

      gimp_draw_tool_add_handle (draw_tool,
                                 GIMP_HANDLE_CROSS,
                                 source_tool->src_x + off_x,
                                 source_tool->src_y + off_y,
                                 GIMP_TOOL_HANDLE_SIZE_CROSS,
                                 GIMP_TOOL_HANDLE_SIZE_CROSS,
                                 GIMP_HANDLE_ANCHOR_CENTER);

      draw_tool->display = tmp_display;
    }

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}
