/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file studioAudioManager.h
 * @author jovan
 * @date 2026-04-13
 *
 * Abstract base class for event/bank-based audio managers such as FMOD Studio.
 * This is a separate hierarchy from AudioManager, which is designed for
 * file-based audio.  StudioAudioManager works with designer-authored events
 * loaded from bank files, global and per-event parameters, mixer buses, and
 * VCAs.
 */

#ifndef STUDIOAUDIOMANAGER_H
#define STUDIOAUDIOMANAGER_H

#include "config_audio.h"
#include "studioAudioEvent.h"
#include "filename.h"

class StudioAudioManager;
typedef StudioAudioManager *Create_StudioAudioManager_proc();

class EXPCL_PANDA_AUDIO StudioAudioManager : public TypedReferenceCount {
PUBLISHED:
  static PT(StudioAudioManager) create_StudioAudioManager();
  virtual ~StudioAudioManager();

  // System lifecycle.
  virtual bool is_valid() = 0;
  virtual void update() = 0;
  virtual void shutdown() = 0;

  // Bank management.
  virtual bool load_bank(const Filename &bank_path, bool nonblocking = false) = 0;
  virtual void unload_bank(const Filename &bank_path) = 0;
  virtual void unload_all_banks() = 0;

  // Event creation.
  virtual PT(StudioAudioEvent) get_event(const std::string &event_path) = 0;

  // Global parameters.
  virtual void set_parameter(const std::string &name, PN_stdfloat value,
                             bool ignore_seek_speed = false) = 0;
  virtual PN_stdfloat get_parameter(const std::string &name) const = 0;
  virtual void set_parameter_by_label(const std::string &name,
                                      const std::string &label) = 0;

  // Bus control.
  virtual void set_bus_volume(const std::string &bus_path, PN_stdfloat volume) = 0;
  virtual PN_stdfloat get_bus_volume(const std::string &bus_path) const = 0;
  virtual void set_bus_paused(const std::string &bus_path, bool paused) = 0;
  virtual void set_bus_mute(const std::string &bus_path, bool mute) = 0;
  virtual void stop_bus(const std::string &bus_path, bool allow_fadeout = true) = 0;

  // VCA control.
  virtual void set_vca_volume(const std::string &vca_path, PN_stdfloat volume) = 0;
  virtual PN_stdfloat get_vca_volume(const std::string &vca_path) const = 0;

  // Master volume (convenience, delegates to master bus).
  virtual void set_volume(PN_stdfloat volume) = 0;
  virtual PN_stdfloat get_volume() const = 0;

  // 3D listener.
  virtual void set_listener_count(int count) = 0;
  virtual int get_listener_count() const = 0;
  virtual void set_listener_attributes(int listener,
                                       PN_stdfloat px, PN_stdfloat py, PN_stdfloat pz,
                                       PN_stdfloat vx, PN_stdfloat vy, PN_stdfloat vz,
                                       PN_stdfloat fx, PN_stdfloat fy, PN_stdfloat fz,
                                       PN_stdfloat ux, PN_stdfloat uy, PN_stdfloat uz) = 0;
  virtual void set_3d_distance_factor(PN_stdfloat factor) = 0;
  virtual void set_3d_doppler_factor(PN_stdfloat factor) = 0;

  // Stop all events.
  virtual void stop_all_events(bool allow_fadeout = true) = 0;

  virtual void output(std::ostream &out) const;
  virtual void write(std::ostream &out) const;

public:
  static void register_StudioAudioManager_creator(Create_StudioAudioManager_proc *proc);

protected:
  StudioAudioManager();

  static Create_StudioAudioManager_proc *_create_StudioAudioManager;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedReferenceCount::init_type();
    register_type(_type_handle, "StudioAudioManager",
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
operator << (std::ostream &out, const StudioAudioManager &mgr) {
  mgr.output(out);
  return out;
}

#endif /* STUDIOAUDIOMANAGER_H */
