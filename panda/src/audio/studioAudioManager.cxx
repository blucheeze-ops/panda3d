/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file studioAudioManager.cxx
 * @author jovan
 * @date 2026-04-13
 */

#include "studioAudioManager.h"
#include "config_audio.h"
#include "load_dso.h"
#include "config_putil.h"

using std::string;

TypeHandle StudioAudioManager::_type_handle;

Create_StudioAudioManager_proc *StudioAudioManager::_create_StudioAudioManager = nullptr;

void StudioAudioManager::
register_StudioAudioManager_creator(Create_StudioAudioManager_proc *proc) {
  nassertv(_create_StudioAudioManager == nullptr || _create_StudioAudioManager == proc);
  _create_StudioAudioManager = proc;
}

/**
 * Factory method.  Returns a platform-specific StudioAudioManager, or nullptr
 * if no Studio audio backend is available.
 *
 * The Studio DSO name is derived from audio-library-name automatically:
 * "p3fmod_audio" -> "p3fmod_studio_audio".  No separate config variable is
 * needed — setting audio-library-name to p3fmod_audio enables both Core and
 * Studio audio.
 */
PT(StudioAudioManager) StudioAudioManager::
create_StudioAudioManager() {
  if (_create_StudioAudioManager != nullptr) {
    PT(StudioAudioManager) mgr = (*_create_StudioAudioManager)();
    if (mgr != nullptr && mgr->is_valid()) {
      return mgr;
    }
    audio_error("  StudioAudioManager created but is not valid");
    return nullptr;
  }

  // Derive the Studio library name from audio-library-name.
  // e.g. "p3fmod_audio" -> "p3fmod_studio_audio"
  string core_lib = audio_library_name;
  string studio_lib;
  string::size_type pos = core_lib.rfind("_audio");
  if (pos != string::npos) {
    studio_lib = core_lib.substr(0, pos) + "_studio_audio";
  }

  audio_debug("create_StudioAudioManager()\n  audio_library_name=\""
              << core_lib << "\"\n  derived studio lib=\"" << studio_lib << "\"");

  if (studio_lib.empty()) {
    audio_debug("  Cannot derive Studio library name, returning nullptr");
    return nullptr;
  }

  // Try loading the DSO dynamically.
  static bool lib_load = false;
  if (!lib_load) {
    lib_load = true;
    Filename dl_name = Filename::dso_filename(
        "lib" + studio_lib + ".so");
    dl_name.to_os_specific();
    audio_debug("  dl_name=\"" << dl_name << "\"");
    void *handle = load_dso(get_plugin_path().get_value(), dl_name);
    if (handle == nullptr) {
      audio_error("  load_dso(" << dl_name << ") failed for Studio audio");
      audio_error("    " << load_dso_error());
    } else {
      string sym_lib = studio_lib;
      if (sym_lib.substr(0, 2) == "p3") {
        sym_lib = sym_lib.substr(2);
      }
      string symbol_name = "get_studio_audio_manager_func_" + sym_lib;
      void *dso_symbol = get_dso_symbol(handle, symbol_name);
      if (dso_symbol == nullptr) {
        unload_dso(handle);
        audio_error("  Studio audio library did not provide " << symbol_name);
      } else {
        typedef Create_StudioAudioManager_proc *FuncType();
        Create_StudioAudioManager_proc *factory_func = (*(FuncType *)dso_symbol)();
        if (_create_StudioAudioManager == nullptr) {
          register_StudioAudioManager_creator(factory_func);
        }
      }
    }
  }

  if (_create_StudioAudioManager == nullptr) {
    audio_debug("  No Studio audio backend registered, returning nullptr");
    return nullptr;
  }

  PT(StudioAudioManager) mgr = (*_create_StudioAudioManager)();
  if (mgr != nullptr && mgr->is_valid()) {
    return mgr;
  }
  audio_error("  StudioAudioManager created but is not valid");
  return nullptr;
}

StudioAudioManager::
StudioAudioManager() {
}

StudioAudioManager::
~StudioAudioManager() {
}

void StudioAudioManager::
output(std::ostream &out) const {
  out << get_type();
}

void StudioAudioManager::
write(std::ostream &out) const {
  out << (*this) << "\n";
}
