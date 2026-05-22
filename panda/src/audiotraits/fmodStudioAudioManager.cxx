/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file fmodStudioAudioManager.cxx
 * @author jovan
 * @date 2026-04-13
 */

#include "pandabase.h"
#include "config_audio.h"
#include "config_fmodStudioAudio.h"
#include "dcast.h"

#include "fmodStudioAudioManager.h"
#include "fmodStudioAudioEvent.h"
#include "filename.h"
#include "virtualFileSystem.h"
#include "reMutexHolder.h"
#include "config_putil.h"

#include <fmod_studio.hpp>
#include <fmod.hpp>
#include <fmod_errors.h>

TypeHandle FMODStudioAudioManager::_type_handle;

ReMutex FMODStudioAudioManager::_lock;
FMOD::Studio::System *FMODStudioAudioManager::_studio_system = nullptr;
FMOD::System *FMODStudioAudioManager::_core_system = nullptr;
bool FMODStudioAudioManager::_system_is_valid = false;
FMODStudioAudioManager::ManagerSet FMODStudioAudioManager::_all_managers;

void _fmod_studio_audio_errcheck(const char *context, FMOD_RESULT result) {
  if (result != FMOD_OK) {
    audio_error(context << ": " << FMOD_ErrorString(result));
  }
}

/**
 * Factory function.
 */
StudioAudioManager *Create_FmodStudioAudioManager() {
  audio_debug("Create_FmodStudioAudioManager()");
  return new FMODStudioAudioManager;
}

/**
 *
 */
FMODStudioAudioManager::
FMODStudioAudioManager() {
  ReMutexHolder holder(_lock);
  FMOD_RESULT result;

  _all_managers.insert(this);
  _is_valid = false;

  if (_studio_system == nullptr) {
    // Create the Studio system singleton.
    result = FMOD::Studio::System::create(&_studio_system);
    fmod_studio_audio_errcheck("FMOD::Studio::System::create()", result);
    if (result != FMOD_OK) {
      _studio_system = nullptr;
      return;
    }

    // Check FMOD version.
    unsigned int version;
    result = _studio_system->getCoreSystem(&_core_system);
    fmod_studio_audio_errcheck("getCoreSystem()", result);

    if (_core_system != nullptr) {
      result = _core_system->getVersion(&version);
      fmod_studio_audio_errcheck("getVersion()", result);
      if (version < FMOD_VERSION) {
        audio_error("FMOD version mismatch: headers " << FMOD_VERSION
                    << ", library " << version);
      }
    }

    // Configure speaker mode if requested.
    if (fmod_speaker_mode != FSM_unspecified) {
      FMOD_SPEAKERMODE mode = (FMOD_SPEAKERMODE)(int)fmod_speaker_mode;
      result = _core_system->setSoftwareFormat(0, mode, 0);
      fmod_studio_audio_errcheck("setSoftwareFormat()", result);
    }

    // Initialize Studio (which initializes Core internally).
    FMOD_STUDIO_INITFLAGS studio_flags = FMOD_STUDIO_INIT_NORMAL;
    if (fmod_studio_live_update) {
      studio_flags |= FMOD_STUDIO_INIT_LIVEUPDATE;
    }

    result = _studio_system->initialize(
        fmod_number_of_sound_channels, studio_flags,
        FMOD_INIT_NORMAL, nullptr);
    fmod_studio_audio_errcheck("Studio::System::initialize()", result);
    if (result != FMOD_OK) {
      _studio_system->release();
      _studio_system = nullptr;
      _core_system = nullptr;
      return;
    }

    _system_is_valid = true;
  }

  _is_valid = _system_is_valid;
}

/**
 *
 */
FMODStudioAudioManager::
~FMODStudioAudioManager() {
  ReMutexHolder holder(_lock);

  // Stop and release all events we still own.
  EventSet events_copy = _all_events;
  for (FMODStudioAudioEvent *evt : events_copy) {
    evt->cleanup();
  }
  _all_events.clear();

  // Unload banks.
  for (auto &pair : _banks) {
    if (pair.second != nullptr) {
      pair.second->unload();
    }
  }
  _banks.clear();

  _all_managers.erase(this);

  if (_all_managers.empty() && _studio_system != nullptr) {
    _studio_system->release();
    _studio_system = nullptr;
    _core_system = nullptr;
    _system_is_valid = false;
  }
}

bool FMODStudioAudioManager::
is_valid() {
  return _is_valid;
}

void FMODStudioAudioManager::
update() {
  ReMutexHolder holder(_lock);
  if (_studio_system != nullptr) {
    _studio_system->update();
  }
}

void FMODStudioAudioManager::
shutdown() {
  ReMutexHolder holder(_lock);

  EventSet events_copy = _all_events;
  for (FMODStudioAudioEvent *evt : events_copy) {
    evt->cleanup();
  }
  _all_events.clear();

  unload_all_banks();
  _is_valid = false;
}

/**
 * Loads a bank file.  Reads through VFS into memory, then passes to FMOD
 * Studio via loadBankMemory.
 */
bool FMODStudioAudioManager::
load_bank(const Filename &bank_path, bool nonblocking) {
  ReMutexHolder holder(_lock);

  if (_studio_system == nullptr) {
    audio_error("load_bank: Studio system not initialized");
    return false;
  }

  // Check if already loaded.
  if (_banks.count(bank_path) > 0) {
    audio_debug("load_bank: " << bank_path << " already loaded");
    return true;
  }

  // Read the bank file through VFS.
  VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
  Filename resolved = bank_path;

  // Try to resolve against the model path and the studio bank path.
  if (!vfs->resolve_filename(resolved, get_model_path())) {
    // Try with the configured bank path prefix.
    std::string bank_dir = fmod_studio_bank_path;
    if (!bank_dir.empty()) {
      Filename prefixed(bank_dir, bank_path);
      if (vfs->resolve_filename(prefixed, get_model_path())) {
        resolved = prefixed;
      } else {
        // Last resort: try the raw path.
        resolved = bank_path;
      }
    }
  }

  std::string data;
  if (!vfs->read_file(resolved, data, true)) {
    audio_error("load_bank: could not read " << resolved);
    return false;
  }

  FMOD_STUDIO_LOAD_BANK_FLAGS flags = FMOD_STUDIO_LOAD_BANK_NORMAL;
  if (nonblocking) {
    flags |= FMOD_STUDIO_LOAD_BANK_NONBLOCKING;
  }

  FMOD::Studio::Bank *bank = nullptr;
  FMOD_RESULT result = _studio_system->loadBankMemory(
      data.data(), (int)data.size(),
      FMOD_STUDIO_LOAD_MEMORY, flags, &bank);
  fmod_studio_audio_errcheck("loadBankMemory()", result);

  if (result != FMOD_OK || bank == nullptr) {
    return false;
  }

  _banks[bank_path] = bank;
  audio_debug("load_bank: loaded " << bank_path);
  return true;
}

void FMODStudioAudioManager::
unload_bank(const Filename &bank_path) {
  ReMutexHolder holder(_lock);

  auto it = _banks.find(bank_path);
  if (it != _banks.end()) {
    if (it->second != nullptr) {
      it->second->unload();
    }
    _banks.erase(it);
    audio_debug("unload_bank: unloaded " << bank_path);
  }
}

void FMODStudioAudioManager::
unload_all_banks() {
  ReMutexHolder holder(_lock);

  for (auto &pair : _banks) {
    if (pair.second != nullptr) {
      pair.second->unload();
    }
  }
  _banks.clear();
}

/**
 * Creates a new event instance from the given event path (e.g.
 * "event:/sfx/footstep").  Returns nullptr if the event cannot be found.
 */
PT(StudioAudioEvent) FMODStudioAudioManager::
get_event(const std::string &event_path) {
  ReMutexHolder holder(_lock);

  if (_studio_system == nullptr) {
    audio_error("get_event: Studio system not initialized");
    return nullptr;
  }

  FMOD::Studio::EventDescription *desc = nullptr;
  FMOD_RESULT result = _studio_system->getEvent(event_path.c_str(), &desc);
  if (result != FMOD_OK || desc == nullptr) {
    audio_error("get_event: could not find event '" << event_path << "': "
                << FMOD_ErrorString(result));
    return nullptr;
  }

  FMOD::Studio::EventInstance *instance = nullptr;
  result = desc->createInstance(&instance);
  if (result != FMOD_OK || instance == nullptr) {
    audio_error("get_event: could not create instance for '"
                << event_path << "': " << FMOD_ErrorString(result));
    return nullptr;
  }

  PT(FMODStudioAudioEvent) event = new FMODStudioAudioEvent(this, desc, instance, event_path);
  _all_events.insert(event.p());
  return event.p();
}

// Global parameters.

void FMODStudioAudioManager::
set_parameter(const std::string &name, PN_stdfloat value, bool ignore_seek_speed) {
  ReMutexHolder holder(_lock);
  if (_studio_system == nullptr) return;
  FMOD_RESULT result = _studio_system->setParameterByName(
      name.c_str(), (float)value, ignore_seek_speed);
  fmod_studio_audio_errcheck("setParameterByName()", result);
}

PN_stdfloat FMODStudioAudioManager::
get_parameter(const std::string &name) const {
  ReMutexHolder holder(_lock);
  float value = 0.0f;
  if (_studio_system != nullptr) {
    _studio_system->getParameterByName(name.c_str(), &value);
  }
  return (PN_stdfloat)value;
}

void FMODStudioAudioManager::
set_parameter_by_label(const std::string &name, const std::string &label) {
  ReMutexHolder holder(_lock);
  if (_studio_system == nullptr) return;
  FMOD_RESULT result = _studio_system->setParameterByNameWithLabel(
      name.c_str(), label.c_str());
  fmod_studio_audio_errcheck("setParameterByNameWithLabel()", result);
}

// Bus control.

void FMODStudioAudioManager::
set_bus_volume(const std::string &bus_path, PN_stdfloat volume) {
  ReMutexHolder holder(_lock);
  if (_studio_system == nullptr) return;
  FMOD::Studio::Bus *bus = nullptr;
  FMOD_RESULT result = _studio_system->getBus(bus_path.c_str(), &bus);
  if (result == FMOD_OK && bus != nullptr) {
    bus->setVolume((float)volume);
  }
}

PN_stdfloat FMODStudioAudioManager::
get_bus_volume(const std::string &bus_path) const {
  ReMutexHolder holder(_lock);
  if (_studio_system == nullptr) return 0.0f;
  FMOD::Studio::Bus *bus = nullptr;
  FMOD_RESULT result = _studio_system->getBus(bus_path.c_str(), &bus);
  float volume = 0.0f;
  if (result == FMOD_OK && bus != nullptr) {
    bus->getVolume(&volume);
  }
  return (PN_stdfloat)volume;
}

void FMODStudioAudioManager::
set_bus_paused(const std::string &bus_path, bool paused) {
  ReMutexHolder holder(_lock);
  if (_studio_system == nullptr) return;
  FMOD::Studio::Bus *bus = nullptr;
  FMOD_RESULT result = _studio_system->getBus(bus_path.c_str(), &bus);
  if (result == FMOD_OK && bus != nullptr) {
    bus->setPaused(paused);
  }
}

void FMODStudioAudioManager::
set_bus_mute(const std::string &bus_path, bool mute) {
  ReMutexHolder holder(_lock);
  if (_studio_system == nullptr) return;
  FMOD::Studio::Bus *bus = nullptr;
  FMOD_RESULT result = _studio_system->getBus(bus_path.c_str(), &bus);
  if (result == FMOD_OK && bus != nullptr) {
    bus->setMute(mute);
  }
}

void FMODStudioAudioManager::
stop_bus(const std::string &bus_path, bool allow_fadeout) {
  ReMutexHolder holder(_lock);
  if (_studio_system == nullptr) return;
  FMOD::Studio::Bus *bus = nullptr;
  FMOD_RESULT result = _studio_system->getBus(bus_path.c_str(), &bus);
  if (result == FMOD_OK && bus != nullptr) {
    bus->stopAllEvents(allow_fadeout ? FMOD_STUDIO_STOP_ALLOWFADEOUT
                                    : FMOD_STUDIO_STOP_IMMEDIATE);
  }
}

// VCA control.

void FMODStudioAudioManager::
set_vca_volume(const std::string &vca_path, PN_stdfloat volume) {
  ReMutexHolder holder(_lock);
  if (_studio_system == nullptr) return;
  FMOD::Studio::VCA *vca = nullptr;
  FMOD_RESULT result = _studio_system->getVCA(vca_path.c_str(), &vca);
  if (result == FMOD_OK && vca != nullptr) {
    vca->setVolume((float)volume);
  }
}

PN_stdfloat FMODStudioAudioManager::
get_vca_volume(const std::string &vca_path) const {
  ReMutexHolder holder(_lock);
  if (_studio_system == nullptr) return 0.0f;
  FMOD::Studio::VCA *vca = nullptr;
  FMOD_RESULT result = _studio_system->getVCA(vca_path.c_str(), &vca);
  float volume = 0.0f;
  if (result == FMOD_OK && vca != nullptr) {
    vca->getVolume(&volume);
  }
  return (PN_stdfloat)volume;
}

// Master volume (convenience via master bus).

void FMODStudioAudioManager::
set_volume(PN_stdfloat volume) {
  set_bus_volume("bus:/", volume);
}

PN_stdfloat FMODStudioAudioManager::
get_volume() const {
  return get_bus_volume("bus:/");
}

// 3D listener.

void FMODStudioAudioManager::
set_listener_count(int count) {
  ReMutexHolder holder(_lock);
  if (_studio_system == nullptr) return;
  _studio_system->setNumListeners(count);
}

int FMODStudioAudioManager::
get_listener_count() const {
  ReMutexHolder holder(_lock);
  int count = 1;
  if (_studio_system != nullptr) {
    _studio_system->getNumListeners(&count);
  }
  return count;
}

/**
 * Sets the 3D listener attributes.  Panda uses Y-forward, Z-up; FMOD uses
 * Z-forward, Y-up, so we swap Y and Z.
 */
void FMODStudioAudioManager::
set_listener_attributes(int listener,
                        PN_stdfloat px, PN_stdfloat py, PN_stdfloat pz,
                        PN_stdfloat vx, PN_stdfloat vy, PN_stdfloat vz,
                        PN_stdfloat fx, PN_stdfloat fy, PN_stdfloat fz,
                        PN_stdfloat ux, PN_stdfloat uy, PN_stdfloat uz) {
  ReMutexHolder holder(_lock);
  if (_studio_system == nullptr) return;

  FMOD_3D_ATTRIBUTES attrs;
  // Panda Y-forward Z-up → FMOD Z-forward Y-up: swap Y and Z.
  attrs.position = {(float)px, (float)pz, (float)py};
  attrs.velocity = {(float)vx, (float)vz, (float)vy};
  attrs.forward  = {(float)fx, (float)fz, (float)fy};
  attrs.up       = {(float)ux, (float)uz, (float)uy};

  FMOD_RESULT result = _studio_system->setListenerAttributes(listener, &attrs);
  fmod_studio_audio_errcheck("setListenerAttributes()", result);
}

void FMODStudioAudioManager::
set_3d_distance_factor(PN_stdfloat factor) {
  ReMutexHolder holder(_lock);
  if (_core_system == nullptr) return;
  FMOD_RESULT result = _core_system->set3DSettings(
      1.0f, (float)factor, 1.0f);
  fmod_studio_audio_errcheck("set3DSettings(distance_factor)", result);
}

void FMODStudioAudioManager::
set_3d_doppler_factor(PN_stdfloat factor) {
  ReMutexHolder holder(_lock);
  if (_core_system == nullptr) return;
  // Preserve existing distance and rolloff, only change doppler.
  float doppler_cur, distance_cur, rolloff_cur;
  _core_system->get3DSettings(&doppler_cur, &distance_cur, &rolloff_cur);
  FMOD_RESULT result = _core_system->set3DSettings(
      (float)factor, distance_cur, rolloff_cur);
  fmod_studio_audio_errcheck("set3DSettings(doppler_factor)", result);
}

// Stop all events via the master bus.
void FMODStudioAudioManager::
stop_all_events(bool allow_fadeout) {
  ReMutexHolder holder(_lock);
  if (_studio_system == nullptr) return;
  FMOD::Studio::Bus *master = nullptr;
  FMOD_RESULT result = _studio_system->getBus("bus:/", &master);
  if (result == FMOD_OK && master != nullptr) {
    master->stopAllEvents(allow_fadeout ? FMOD_STUDIO_STOP_ALLOWFADEOUT
                                       : FMOD_STUDIO_STOP_IMMEDIATE);
  }
}

void FMODStudioAudioManager::
release_event(FMODStudioAudioEvent *event) {
  ReMutexHolder holder(_lock);
  _all_events.erase(event);
}
