/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file config_fmodStudioAudio.h
 * @author jovan
 * @date 2026-04-13
 */

#ifndef CONFIG_FMODSTUDIOAUDIO_H
#define CONFIG_FMODSTUDIOAUDIO_H

#include "pandabase.h"

#include "notifyCategoryProxy.h"
#include "configVariableBool.h"
#include "configVariableString.h"
#include "dconfig.h"
#include "studioAudioManager.h"

ConfigureDecl(config_fmodStudioAudio, EXPCL_FMOD_STUDIO_AUDIO, EXPTP_FMOD_STUDIO_AUDIO);
NotifyCategoryDecl(fmodStudioAudio, EXPCL_FMOD_STUDIO_AUDIO, EXPTP_FMOD_STUDIO_AUDIO);

extern EXPCL_FMOD_STUDIO_AUDIO ConfigVariableBool fmod_studio_live_update;
extern EXPCL_FMOD_STUDIO_AUDIO ConfigVariableString fmod_studio_bank_path;

extern "C" EXPCL_FMOD_STUDIO_AUDIO void init_libFmodStudioAudio();
extern "C" EXPCL_FMOD_STUDIO_AUDIO Create_StudioAudioManager_proc *get_studio_audio_manager_func_fmod_studio_audio();

#endif // CONFIG_FMODSTUDIOAUDIO_H
