/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPresetGui
 * Copyright (C) 2017 seagetch <sigetch@gmail.com>
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

#include "base/delegators.hpp"
#include "base/glib-cxx-types.hpp"
__DECLARE_GTK_CLASS__(GObject, G_TYPE_OBJECT);
#include "base/glib-cxx-impl.hpp"
#include "base/glib-cxx-utils.hpp"

extern "C" {
#include "config.h"
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#include "presets-enums.h"

#include "core/gimp.h"

#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"
#include "core/gimpdata.h"

#include "gimp-intl.h"

#include "core/gimpparamspecs.h"
#include "config/gimpcoreconfig.h"

#include "widgets/widgets-types.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpactiongroup.h"

#include "actions/dialogs-commands.h"
}
#include "presets/gimpjsonresource.h"
#include "preset-factory-gui.h"

#include "layer-preset-gui.h"


using namespace GLib;

extern GLib::GClassWrapper<UseCStructs<GimpBaseConfig, GimpCoreConfig> > core_class;

////////////////////////////////////////////////////////////////////////////
// PresetGuiConfig


////////////////////////////////////////////////////////////////////////////
// PresetGuiFactory

static PresetGuiFactory* singleton = NULL;
PresetGuiFactory* get_preset_factory_gui() {
  if (!singleton)
    singleton = new PresetGuiFactory();
  return singleton;
}

void PresetGuiFactory::initialize()
{
  register_config( LayerPresetGuiConfig::config() );
}

void PresetGuiFactory::entry_point(Gimp* gimp)
{
  PresetFactory::entry_point(gimp);
}


GArray*
PresetGuiFactory::prefs_entry_point ()
{
  GArray* result = g_array_new(FALSE, TRUE, sizeof(PrefsDialogEntry));
  auto array = ref<PrefsDialogEntry>(result);

  registry().each([&](auto key, auto it) {
    auto config = dynamic_cast<PresetGuiConfig*>(it);
    PrefsDialogEntry* entry = config->prefs_dialog_entry();
    if (entry) {
      g_print("PresetGuiFactory::prefs_entry_point: add %p\n", entry);
      array.append( *entry );
    }
  });
  g_print("PresetGuiFactory::prefs_entry_point: end\n");
  return result;
}


void
PresetGuiFactory::dialogs_actions_entry_point (GimpActionGroup* group)
{
  g_print("PresetGuiFactory::dialog_entry_point: group=%p\n", group);

  // Dialog actions
  auto _array = hold ( g_array_new(FALSE, TRUE, sizeof(GimpStringActionEntry)) );
  auto array  = ref<GimpStringActionEntry>(_array);

  registry().each([&](auto key, auto it) {
    auto config = dynamic_cast<PresetGuiConfig*>(it);
    auto entry = config->view_dialog_action_entry();
    if (entry)
      array.append(*entry);
  });

  gimp_action_group_add_string_actions (group, "dialogs-action",
                                        &array[0],
                                        array.size(),
                                        G_CALLBACK (dialogs_create_dockable_cmd_callback));
  g_print("PresetGuiFactory::dialog_entry_point: end\n");

}


void
PresetGuiFactory::on_initialize (Gimp               *gimp,
                                 GimpInitStatusFunc  status_callback)
{
  PresetFactory::on_initialize (gimp, status_callback);
}


void
PresetGuiFactory::on_restore (Gimp               *gimp,
                              GimpInitStatusFunc  status_callback)
{
  PresetFactory::on_restore(gimp, status_callback);

  // Dialog initialization.

  registry().each([&](auto key, auto it) {
    GimpDialogFactory* dialog_factory = gimp_dialog_factory_get_singleton();
    PresetGuiConfig* gui_config = dynamic_cast<PresetGuiConfig*>(it);

    g_print("PresetGuiFactory::on_restore::GimpDialogFactory=%p\n", dialog_factory);
    GimpDialogFactoryEntry* entry = gui_config->view_dialog_new_entry();
    gimp_dialog_factory_register_entry (dialog_factory,
                                        entry->identifier,
                                        gettext (entry->name),
                                        gettext (entry->blurb),
                                        entry->stock_id,
                                        entry->help_id,
                                        entry->new_func,
                                        entry->restore_func,
                                        entry->view_size,
                                        entry->singleton,
                                        entry->session_managed,
                                        entry->remember_size,
                                        entry->remember_if_open,
                                        entry->hideable,
                                        entry->image_window,
                                        entry->dockable);
    g_print("PresetGuiFactory::on_restore::end\n");

  });
}


gboolean
PresetGuiFactory::on_exit(Gimp *gimp,
                          gboolean force)
{
  gboolean result = PresetFactory::on_exit(gimp, force);
  return result;
}


void
preset_factory_gui_entry_point (Gimp* gimp)
{
  get_preset_factory_gui()->entry_point(gimp);
}


GArray*
preset_factory_gui_prefs_entry_point ()
{
  return get_preset_factory_gui()->prefs_entry_point();
}


void
preset_factory_gui_dialogs_actions_entry_point (GimpActionGroup* group)
{
  return get_preset_factory_gui()->dialogs_actions_entry_point(group);
}
