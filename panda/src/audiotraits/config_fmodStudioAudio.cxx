/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file config_fmodStudioAudio.cxx
 * @author jovan
 * @date 2026-04-13
 */

#include "pandabase.h"

#include "config_fmodStudioAudio.h"
#include "studioAudioManager.h"
#include "fmodStudioAudioManager.h"
#include "fmodStudioAudioEvent.h"
#include "pandaSystem.h"
#include "dconfig.h"

#if !defined(CPPPARSER) && !defined(LINK_ALL_STATIC) && !defined(BUILDING_FMOD_STUDIO_AUDIO)
  #error Buildsystem error: BUILDING_FMOD_STUDIO_AUDIO not defined
#endif

ConfigureDef(config_fmodStudioAudio);
NotifyCategoryDef(fmodStudioAudio, ":audio");

ConfigVariableBool fmod_studio_live_update
("fmod-studio-live-update", false,
 PRC_DESC("When true, enables FMOD Studio Live Update so that the running "
          "application can be connected to from FMOD Studio for real-time "
          "authoring and debugging."));

ConfigVariableString fmod_studio_bank_path
("fmod-studio-bank-path", "",
 PRC_DESC("Default directory to search for FMOD Studio bank files.  If set, "
          "load_bank() will also try <bank-path>/<filename> when resolving "
          "bank paths through the VFS."));

ConfigureFn(config_fmodStudioAudio) {
  init_libFmodStudioAudio();
}

/**
 * Initializes the library.  This must be called at least once before any of
 * the functions or classes in this library can be used.
 */
void
init_libFmodStudioAudio() {
  static bool initialized = false;
  if (initialized) {
    return;
  }

  initialized = true;
  FMODStudioAudioManager::init_type();
  FMODStudioAudioEvent::init_type();

  StudioAudioManager::register_StudioAudioManager_creator(
      &Create_FmodStudioAudioManager);

  PandaSystem *ps = PandaSystem::get_global_ptr();
  ps->add_system("FMOD Studio");
  ps->add_system("audio");
  ps->set_system_tag("audio", "studio_implementation", "FMOD Studio");
}

/**
 * Returns the factory function for creating an FMODStudioAudioManager.
 * Called when this library is loaded as a DSO plugin.
 */
Create_StudioAudioManager_proc *
get_studio_audio_manager_func_fmod_studio_audio() {
  init_libFmodStudioAudio();
  return &Create_FmodStudioAudioManager;
}
