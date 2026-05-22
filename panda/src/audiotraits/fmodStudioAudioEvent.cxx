/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file fmodStudioAudioEvent.cxx
 * @author jovan
 * @date 2026-04-13
 */

#include "pandabase.h"
#include "fmodStudioAudioEvent.h"
#include "fmodStudioAudioManager.h"
#include "reMutexHolder.h"
#include "throw_event.h"

#include <fmod_studio.hpp>
#include <fmod_errors.h>

TypeHandle FMODStudioAudioEvent::_type_handle;

FMODStudioAudioEvent::
FMODStudioAudioEvent(FMODStudioAudioManager *manager,
                     FMOD::Studio::EventDescription *desc,
                     FMOD::Studio::EventInstance *instance,
                     const std::string &event_path) :
  _manager(manager),
  _description(desc),
  _instance(instance),
  _event_path(event_path),
  _volume(1.0f),
  _pitch(1.0f),
  _stopped_flag(false)
{
  // Store a pointer to ourselves in the instance's user data so the callback
  // can reach us.
  if (_instance != nullptr) {
    _instance->setUserData(this);
  }
}

FMODStudioAudioEvent::
~FMODStudioAudioEvent() {
  cleanup();
  if (_manager != nullptr) {
    _manager->release_event(this);
    _manager = nullptr;
  }
}

/**
 * Releases the underlying FMOD instance.  Called from the destructor and from
 * the manager's shutdown/destructor.
 */
void FMODStudioAudioEvent::
cleanup() {
  if (_instance != nullptr) {
    _instance->setUserData(nullptr);
    _instance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
    _instance->release();
    _instance = nullptr;
  }
  _description = nullptr;
}

// Playback.

void FMODStudioAudioEvent::
start() {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  if (_instance == nullptr) return;
  FMOD_RESULT result = _instance->start();
  fmod_studio_audio_errcheck("EventInstance::start()", result);
  _stopped_flag = false;
}

void FMODStudioAudioEvent::
stop(bool allow_fadeout) {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  if (_instance == nullptr) return;
  FMOD_RESULT result = _instance->stop(
      allow_fadeout ? FMOD_STUDIO_STOP_ALLOWFADEOUT : FMOD_STUDIO_STOP_IMMEDIATE);
  fmod_studio_audio_errcheck("EventInstance::stop()", result);
}

void FMODStudioAudioEvent::
key_off() {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  if (_instance == nullptr) return;
  FMOD_RESULT result = _instance->keyOff();
  fmod_studio_audio_errcheck("EventInstance::keyOff()", result);
}

void FMODStudioAudioEvent::
set_paused(bool paused) {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  if (_instance == nullptr) return;
  _instance->setPaused(paused);
}

bool FMODStudioAudioEvent::
get_paused() const {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  bool paused = false;
  if (_instance != nullptr) {
    _instance->getPaused(&paused);
  }
  return paused;
}

// Timeline.

void FMODStudioAudioEvent::
set_timeline_position(PN_stdfloat time_sec) {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  if (_instance == nullptr) return;
  int ms = (int)(time_sec * 1000.0f);
  _instance->setTimelinePosition(ms);
}

PN_stdfloat FMODStudioAudioEvent::
get_timeline_position() const {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  int ms = 0;
  if (_instance != nullptr) {
    _instance->getTimelinePosition(&ms);
  }
  return (PN_stdfloat)ms / 1000.0f;
}

PN_stdfloat FMODStudioAudioEvent::
length() const {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  int ms = 0;
  if (_description != nullptr) {
    _description->getLength(&ms);
  }
  return (PN_stdfloat)ms / 1000.0f;
}

// Volume and pitch.

void FMODStudioAudioEvent::
set_volume(PN_stdfloat volume) {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  _volume = volume;
  if (_instance != nullptr) {
    _instance->setVolume((float)volume);
  }
}

PN_stdfloat FMODStudioAudioEvent::
get_volume() const {
  return _volume;
}

void FMODStudioAudioEvent::
set_pitch(PN_stdfloat pitch) {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  _pitch = pitch;
  if (_instance != nullptr) {
    _instance->setPitch((float)pitch);
  }
}

PN_stdfloat FMODStudioAudioEvent::
get_pitch() const {
  return _pitch;
}

// Per-event parameters.

void FMODStudioAudioEvent::
set_parameter(const std::string &name, PN_stdfloat value, bool ignore_seek_speed) {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  if (_instance == nullptr) return;
  FMOD_RESULT result = _instance->setParameterByName(
      name.c_str(), (float)value, ignore_seek_speed);
  fmod_studio_audio_errcheck("EventInstance::setParameterByName()", result);
}

PN_stdfloat FMODStudioAudioEvent::
get_parameter(const std::string &name) const {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  float value = 0.0f;
  if (_instance != nullptr) {
    _instance->getParameterByName(name.c_str(), &value);
  }
  return (PN_stdfloat)value;
}

void FMODStudioAudioEvent::
set_parameter_by_label(const std::string &name, const std::string &label) {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  if (_instance == nullptr) return;
  FMOD_RESULT result = _instance->setParameterByNameWithLabel(
      name.c_str(), label.c_str());
  fmod_studio_audio_errcheck("EventInstance::setParameterByNameWithLabel()", result);
}

// 3D attributes.

void FMODStudioAudioEvent::
set_3d_attributes(PN_stdfloat px, PN_stdfloat py, PN_stdfloat pz,
                  PN_stdfloat vx, PN_stdfloat vy, PN_stdfloat vz) {
  // Use default forward (0,0,1) and up (0,1,0) in FMOD space.
  set_3d_attributes(px, py, pz, vx, vy, vz, 0, 1, 0, 0, 0, 1);
}

void FMODStudioAudioEvent::
set_3d_attributes(PN_stdfloat px, PN_stdfloat py, PN_stdfloat pz,
                  PN_stdfloat vx, PN_stdfloat vy, PN_stdfloat vz,
                  PN_stdfloat fx, PN_stdfloat fy, PN_stdfloat fz,
                  PN_stdfloat ux, PN_stdfloat uy, PN_stdfloat uz) {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  if (_instance == nullptr) return;

  FMOD_3D_ATTRIBUTES attrs;
  // Panda Y-forward Z-up → FMOD Z-forward Y-up: swap Y and Z.
  attrs.position = {(float)px, (float)pz, (float)py};
  attrs.velocity = {(float)vx, (float)vz, (float)vy};
  attrs.forward  = {(float)fx, (float)fz, (float)fy};
  attrs.up       = {(float)ux, (float)uz, (float)uy};

  FMOD_RESULT result = _instance->set3DAttributes(&attrs);
  fmod_studio_audio_errcheck("EventInstance::set3DAttributes()", result);
}

// Event properties.

bool FMODStudioAudioEvent::
is_3d() const {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  bool val = false;
  if (_description != nullptr) {
    _description->is3D(&val);
  }
  return val;
}

bool FMODStudioAudioEvent::
is_oneshot() const {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  bool val = false;
  if (_description != nullptr) {
    _description->isOneshot(&val);
  }
  return val;
}

const std::string &FMODStudioAudioEvent::
get_event_path() const {
  return _event_path;
}

// Status.

StudioAudioEvent::EventStatus FMODStudioAudioEvent::
status() const {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  if (_instance == nullptr) return ES_stopped;

  FMOD_STUDIO_PLAYBACK_STATE state;
  FMOD_RESULT result = _instance->getPlaybackState(&state);
  if (result != FMOD_OK) return ES_stopped;

  switch (state) {
  case FMOD_STUDIO_PLAYBACK_PLAYING:    return ES_playing;
  case FMOD_STUDIO_PLAYBACK_SUSTAINING: return ES_sustaining;
  case FMOD_STUDIO_PLAYBACK_STOPPED:    return ES_stopped;
  case FMOD_STUDIO_PLAYBACK_STARTING:   return ES_starting;
  case FMOD_STUDIO_PLAYBACK_STOPPING:   return ES_stopping;
  default:                              return ES_stopped;
  }
}

bool FMODStudioAudioEvent::
is_valid() const {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  return _instance != nullptr && _instance->isValid();
}

// Reverb.

void FMODStudioAudioEvent::
set_reverb_level(int index, PN_stdfloat level) {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  if (_instance == nullptr) return;
  _instance->setReverbLevel(index, (float)level);
}

PN_stdfloat FMODStudioAudioEvent::
get_reverb_level(int index) const {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  float level = 0.0f;
  if (_instance != nullptr) {
    _instance->getReverbLevel(index, &level);
  }
  return (PN_stdfloat)level;
}

// Finished event.

void FMODStudioAudioEvent::
set_finished_event(const std::string &event_name) {
  ReMutexHolder holder(FMODStudioAudioManager::_lock);
  _finished_event = event_name;

  if (_instance == nullptr) return;

  if (!event_name.empty()) {
    _instance->setCallback(event_callback,
                           FMOD_STUDIO_EVENT_CALLBACK_STOPPED);
  } else {
    _instance->setCallback(nullptr, FMOD_STUDIO_EVENT_CALLBACK_ALL);
  }
}

const std::string &FMODStudioAudioEvent::
get_finished_event() const {
  return _finished_event;
}

/**
 * FMOD callback invoked on the Studio thread when an event stops.  We set a
 * flag that the manager's update() checks on the main thread to throw the
 * Panda event (Panda events are not thread-safe).
 */
FMOD_RESULT F_CALLBACK FMODStudioAudioEvent::
event_callback(FMOD_STUDIO_EVENT_CALLBACK_TYPE type,
               FMOD_STUDIO_EVENTINSTANCE *fmod_event,
               void *parameters) {
  if (type != FMOD_STUDIO_EVENT_CALLBACK_STOPPED) return FMOD_OK;

  // Retrieve our wrapper from the user data.
  FMOD::Studio::EventInstance *instance = (FMOD::Studio::EventInstance *)fmod_event;
  void *userdata = nullptr;
  instance->getUserData(&userdata);

  if (userdata != nullptr) {
    FMODStudioAudioEvent *self = (FMODStudioAudioEvent *)userdata;
    if (!self->_finished_event.empty()) {
      // Throw the Panda event.  throw_event is safe to call from any thread
      // when using the async event queue.
      throw_event(self->_finished_event);
    }
  }

  return FMOD_OK;
}

void FMODStudioAudioEvent::
output(std::ostream &out) const {
  out << get_type() << " " << _event_path;
}
