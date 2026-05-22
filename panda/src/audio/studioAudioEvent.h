/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file studioAudioEvent.h
 * @author jovan
 * @date 2026-04-13
 *
 * Abstract base class for a playable event instance in an event/bank-based
 * audio system such as FMOD Studio.  This parallels AudioSound but is
 * designed around the Studio paradigm: events have parameters, sustain
 * points, and richer playback states.
 */

#ifndef STUDIOAUDIOEVENT_H
#define STUDIOAUDIOEVENT_H

#include "config_audio.h"
#include "typedReferenceCount.h"
#include "pointerTo.h"
#include "luse.h"

class EXPCL_PANDA_AUDIO StudioAudioEvent : public TypedReferenceCount {
PUBLISHED:
  virtual ~StudioAudioEvent();

  // Playback states reported by status().
  enum EventStatus {
    ES_stopped,
    ES_starting,
    ES_playing,
    ES_sustaining,
    ES_stopping,
  };

  // Playback control.
  virtual void start() = 0;
  virtual void stop(bool allow_fadeout = true) = 0;
  virtual void key_off() = 0;
  virtual void set_paused(bool paused) = 0;
  virtual bool get_paused() const = 0;

  // Timeline.
  virtual void set_timeline_position(PN_stdfloat time_sec) = 0;
  virtual PN_stdfloat get_timeline_position() const = 0;
  virtual PN_stdfloat length() const = 0;

  // Volume and pitch.
  virtual void set_volume(PN_stdfloat volume) = 0;
  virtual PN_stdfloat get_volume() const = 0;
  virtual void set_pitch(PN_stdfloat pitch) = 0;
  virtual PN_stdfloat get_pitch() const = 0;

  // Per-event parameters.
  virtual void set_parameter(const std::string &name, PN_stdfloat value,
                             bool ignore_seek_speed = false) = 0;
  virtual PN_stdfloat get_parameter(const std::string &name) const = 0;
  virtual void set_parameter_by_label(const std::string &name,
                                      const std::string &label) = 0;

  // 3D attributes — position + velocity.
  virtual void set_3d_attributes(PN_stdfloat px, PN_stdfloat py, PN_stdfloat pz,
                                 PN_stdfloat vx, PN_stdfloat vy, PN_stdfloat vz) = 0;
  // 3D attributes — position + velocity + orientation.
  virtual void set_3d_attributes(PN_stdfloat px, PN_stdfloat py, PN_stdfloat pz,
                                 PN_stdfloat vx, PN_stdfloat vy, PN_stdfloat vz,
                                 PN_stdfloat fx, PN_stdfloat fy, PN_stdfloat fz,
                                 PN_stdfloat ux, PN_stdfloat uy, PN_stdfloat uz) = 0;

  // Event properties (read-only, from the event description).
  virtual bool is_3d() const = 0;
  virtual bool is_oneshot() const = 0;
  virtual const std::string &get_event_path() const = 0;

  // Status.
  virtual EventStatus status() const = 0;
  virtual bool is_valid() const = 0;

  // Reverb send.
  virtual void set_reverb_level(int index, PN_stdfloat level) = 0;
  virtual PN_stdfloat get_reverb_level(int index) const = 0;

  // Finished-event callback (throws a Panda event when the event stops).
  virtual void set_finished_event(const std::string &event_name) = 0;
  virtual const std::string &get_finished_event() const = 0;

  MAKE_PROPERTY(timeline_position, get_timeline_position, set_timeline_position);
  MAKE_PROPERTY(volume, get_volume, set_volume);
  MAKE_PROPERTY(pitch, get_pitch, set_pitch);
  MAKE_PROPERTY(paused, get_paused, set_paused);
  MAKE_PROPERTY(event_path, get_event_path);

  virtual void output(std::ostream &out) const;
  virtual void write(std::ostream &out) const;

protected:
  StudioAudioEvent();

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedReferenceCount::init_type();
    register_type(_type_handle, "StudioAudioEvent",
                  TypedReferenceCount::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() { init_type(); return get_class_type(); }

private:
  static TypeHandle _type_handle;
};

inline std::ostream &
operator << (std::ostream &out, const StudioAudioEvent &evt) {
  evt.output(out);
  return out;
}

EXPCL_PANDA_AUDIO std::ostream &
operator << (std::ostream &out, StudioAudioEvent::EventStatus status);

#endif /* STUDIOAUDIOEVENT_H */
