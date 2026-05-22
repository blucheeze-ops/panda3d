/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file fmodStudioAudioEvent.h
 * @author jovan
 * @date 2026-04-13
 *
 * Concrete StudioAudioEvent backed by FMOD Studio's EventInstance.
 */

#ifndef FMODSTUDIOAUDIOEVENT_H
#define FMODSTUDIOAUDIOEVENT_H

#include "pandabase.h"
#include "studioAudioEvent.h"

#include <fmod_studio.hpp>
#include <fmod_errors.h>

class FMODStudioAudioManager;

class EXPCL_FMOD_STUDIO_AUDIO FMODStudioAudioEvent : public StudioAudioEvent {
  friend class FMODStudioAudioManager;

public:
  FMODStudioAudioEvent(FMODStudioAudioManager *manager,
                       FMOD::Studio::EventDescription *desc,
                       FMOD::Studio::EventInstance *instance,
                       const std::string &event_path);
  virtual ~FMODStudioAudioEvent();

  // Playback.
  virtual void start() override;
  virtual void stop(bool allow_fadeout = true) override;
  virtual void key_off() override;
  virtual void set_paused(bool paused) override;
  virtual bool get_paused() const override;

  // Timeline.
  virtual void set_timeline_position(PN_stdfloat time_sec) override;
  virtual PN_stdfloat get_timeline_position() const override;
  virtual PN_stdfloat length() const override;

  // Volume and pitch.
  virtual void set_volume(PN_stdfloat volume) override;
  virtual PN_stdfloat get_volume() const override;
  virtual void set_pitch(PN_stdfloat pitch) override;
  virtual PN_stdfloat get_pitch() const override;

  // Per-event parameters.
  virtual void set_parameter(const std::string &name, PN_stdfloat value,
                             bool ignore_seek_speed = false) override;
  virtual PN_stdfloat get_parameter(const std::string &name) const override;
  virtual void set_parameter_by_label(const std::string &name,
                                      const std::string &label) override;

  // 3D attributes.
  virtual void set_3d_attributes(PN_stdfloat px, PN_stdfloat py, PN_stdfloat pz,
                                 PN_stdfloat vx, PN_stdfloat vy, PN_stdfloat vz) override;
  virtual void set_3d_attributes(PN_stdfloat px, PN_stdfloat py, PN_stdfloat pz,
                                 PN_stdfloat vx, PN_stdfloat vy, PN_stdfloat vz,
                                 PN_stdfloat fx, PN_stdfloat fy, PN_stdfloat fz,
                                 PN_stdfloat ux, PN_stdfloat uy, PN_stdfloat uz) override;

  // Event properties.
  virtual bool is_3d() const override;
  virtual bool is_oneshot() const override;
  virtual const std::string &get_event_path() const override;

  // Status.
  virtual EventStatus status() const override;
  virtual bool is_valid() const override;

  // Reverb.
  virtual void set_reverb_level(int index, PN_stdfloat level) override;
  virtual PN_stdfloat get_reverb_level(int index) const override;

  // Finished event.
  virtual void set_finished_event(const std::string &event_name) override;
  virtual const std::string &get_finished_event() const override;

  virtual void output(std::ostream &out) const override;

private:
  void cleanup();
  static FMOD_RESULT F_CALLBACK
  event_callback(FMOD_STUDIO_EVENT_CALLBACK_TYPE type,
                 FMOD_STUDIO_EVENTINSTANCE *event,
                 void *parameters);

  FMODStudioAudioManager *_manager;
  FMOD::Studio::EventDescription *_description;
  FMOD::Studio::EventInstance *_instance;

  std::string _event_path;
  PN_stdfloat _volume;
  PN_stdfloat _pitch;
  std::string _finished_event;

  // Flag set by the FMOD callback thread; read in manager's update().
  bool _stopped_flag;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    StudioAudioEvent::init_type();
    register_type(_type_handle, "FMODStudioAudioEvent",
                  StudioAudioEvent::get_class_type());
  }
  virtual TypeHandle get_type() const override {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() override { init_type(); return get_class_type(); }

private:
  static TypeHandle _type_handle;
};

#endif /* FMODSTUDIOAUDIOEVENT_H */
