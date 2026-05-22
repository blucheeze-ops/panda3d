/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file fmodStudioAudioManager.h
 * @author jovan
 * @date 2026-04-13
 *
 * Concrete StudioAudioManager backed by the FMOD Studio API.  Manages
 * banks, event instances, buses, VCAs, and the Studio::System singleton.
 */

#ifndef FMODSTUDIOAUDIOMANAGER_H
#define FMODSTUDIOAUDIOMANAGER_H

#include "pandabase.h"
#include "pmap.h"
#include "pset.h"
#include "reMutex.h"

#include "studioAudioManager.h"

#include <fmod_studio.hpp>
#include <fmod.hpp>
#include <fmod_errors.h>

class FMODStudioAudioEvent;

extern void _fmod_studio_audio_errcheck(const char *context, FMOD_RESULT n);

#ifdef NDEBUG
#define fmod_studio_audio_errcheck(context, n)
#else
#define fmod_studio_audio_errcheck(context, n) _fmod_studio_audio_errcheck(context, n)
#endif

class EXPCL_FMOD_STUDIO_AUDIO FMODStudioAudioManager : public StudioAudioManager {
  friend class FMODStudioAudioEvent;

public:
  FMODStudioAudioManager();
  virtual ~FMODStudioAudioManager();

  // StudioAudioManager interface.
  virtual bool is_valid() override;
  virtual void update() override;
  virtual void shutdown() override;

  virtual bool load_bank(const Filename &bank_path, bool nonblocking = false) override;
  virtual void unload_bank(const Filename &bank_path) override;
  virtual void unload_all_banks() override;

  virtual PT(StudioAudioEvent) get_event(const std::string &event_path) override;

  virtual void set_parameter(const std::string &name, PN_stdfloat value,
                             bool ignore_seek_speed = false) override;
  virtual PN_stdfloat get_parameter(const std::string &name) const override;
  virtual void set_parameter_by_label(const std::string &name,
                                      const std::string &label) override;

  virtual void set_bus_volume(const std::string &bus_path, PN_stdfloat volume) override;
  virtual PN_stdfloat get_bus_volume(const std::string &bus_path) const override;
  virtual void set_bus_paused(const std::string &bus_path, bool paused) override;
  virtual void set_bus_mute(const std::string &bus_path, bool mute) override;
  virtual void stop_bus(const std::string &bus_path, bool allow_fadeout = true) override;

  virtual void set_vca_volume(const std::string &vca_path, PN_stdfloat volume) override;
  virtual PN_stdfloat get_vca_volume(const std::string &vca_path) const override;

  virtual void set_volume(PN_stdfloat volume) override;
  virtual PN_stdfloat get_volume() const override;

  virtual void set_listener_count(int count) override;
  virtual int get_listener_count() const override;
  virtual void set_listener_attributes(int listener,
                                       PN_stdfloat px, PN_stdfloat py, PN_stdfloat pz,
                                       PN_stdfloat vx, PN_stdfloat vy, PN_stdfloat vz,
                                       PN_stdfloat fx, PN_stdfloat fy, PN_stdfloat fz,
                                       PN_stdfloat ux, PN_stdfloat uy, PN_stdfloat uz) override;
  virtual void set_3d_distance_factor(PN_stdfloat factor) override;
  virtual void set_3d_doppler_factor(PN_stdfloat factor) override;

  virtual void stop_all_events(bool allow_fadeout = true) override;

private:
  void release_event(FMODStudioAudioEvent *event);

private:
  static ReMutex _lock;
  static FMOD::Studio::System *_studio_system;
  static FMOD::System *_core_system;
  static bool _system_is_valid;

  typedef pset<FMODStudioAudioManager *> ManagerSet;
  static ManagerSet _all_managers;

  bool _is_valid;

  typedef pmap<Filename, FMOD::Studio::Bank *> BankMap;
  BankMap _banks;

  typedef pset<FMODStudioAudioEvent *> EventSet;
  EventSet _all_events;

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    StudioAudioManager::init_type();
    register_type(_type_handle, "FMODStudioAudioManager",
                  StudioAudioManager::get_class_type());
  }
  virtual TypeHandle get_type() const override {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() override { init_type(); return get_class_type(); }

private:
  static TypeHandle _type_handle;
};

EXPCL_FMOD_STUDIO_AUDIO StudioAudioManager *Create_FmodStudioAudioManager();

#endif /* FMODSTUDIOAUDIOMANAGER_H */
