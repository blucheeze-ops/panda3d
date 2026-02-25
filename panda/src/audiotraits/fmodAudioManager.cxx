/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file fmodAudioManager.cxx
 * @author cort
 * @date 2003-01-22
 * Prior system by: cary
 * @author Stan Rosenbaum "Staque" - Spring 2006
 * @author lachbr
 * @date 2020-10-04
 */

#include "pandabase.h"
#include "config_audio.h"
#include "config_fmodAudio.h"
#include "dcast.h"

// Panda headers.
#include "config_audio.h"
#include "config_putil.h"
#include "fmodAudioManager.h"
#include "fmodAudioSound.h"
#include "filename.h"
#include "virtualFileSystem.h"
#include "reMutexHolder.h"

// FMOD Headers.
#include <fmod.hpp>
#include <fmod_errors.h>

TypeHandle FMODAudioManager::_type_handle;

ReMutex FMODAudioManager::_lock;
FMOD::System *FMODAudioManager::_system;

FMODAudioManager::ManagerList FMODAudioManager::_all_managers;

bool FMODAudioManager::_system_is_valid = false;

PN_stdfloat FMODAudioManager::_doppler_factor = 1;
PN_stdfloat FMODAudioManager::_distance_factor = 1;
PN_stdfloat FMODAudioManager::_drop_off_factor = 1;

#define FMOD_MIN_SAMPLE_RATE 80000
#define FMOD_MAX_SAMPLE_RATE 192000
#define USER_DSP_MAGIC ((void*)0x7012AB35)

// Central dispatcher for audio errors.

void _fmod_audio_errcheck(const char *context, FMOD_RESULT result) {
  if (result != FMOD_OK) {
    audio_error(context << ": " << FMOD_ErrorString(result) );
  }
}

/**
 * Factory Function
 */
AudioManager *Create_FmodAudioManager() {
  audio_debug("Create_FmodAudioManager()");
  return new FMODAudioManager;
}


/**
 *
 */
FMODAudioManager::
FMODAudioManager() {
  ReMutexHolder holder(_lock);
  FMOD_RESULT result;

  // We need a temporary variable to check the FMOD version.
  unsigned int      version;

  _all_managers.insert(this);

  _concurrent_sound_limit = 0;

  ////////////////////////////////////////////////////////////
  // Initialize the 3D listener (camera) attributes.
  //
  _position.x = 0;
  _position.y = 0;
  _position.z = 0;

  _velocity.x = 0;
  _velocity.y = 0;
  _velocity.z = 0;

  _forward.x = 0;
  _forward.y = 0;
  _forward.z = 0;

  _up.x = 0;
  _up.y = 0;
  _up.z = 0;

  ////////////////////////////////////////////////////////////

  _active = true;

  _saved_outputtype = FMOD_OUTPUTTYPE_AUTODETECT;

  if (!_system) {
    // Create the global FMOD System object.  This one object must be shared
    // by all FmodAudioManagers (this is particularly true on OSX, but the
    // FMOD documentation is unclear as to whether this is the intended design
    // on all systems).

    result = FMOD::System_Create(&_system);
    fmod_audio_errcheck("FMOD::System_Create()", result);

    // Lets check the version of FMOD to make sure the headers and libraries
    // are correct.
    result = _system->getVersion(&version);
    fmod_audio_errcheck("_system->getVersion()", result);

    if (version < FMOD_VERSION) {
      audio_error("You are using an old version of FMOD.  This program requires: " << FMOD_VERSION);
    }

    // Determine the sample rate and speaker mode for the system.  We will use
    // the default configuration that FMOD chooses unless the user specifies
    // custom values via config variables.

    int sample_rate;
    FMOD_SPEAKERMODE speaker_mode;
    int num_raw_speakers;
    _system->getSoftwareFormat(&sample_rate,
                               &speaker_mode,
                               &num_raw_speakers);

    audio_debug("fmod-mixer-sample-rate: " << fmod_mixer_sample_rate);
    if (fmod_mixer_sample_rate.get_value() != -1) {
      if (fmod_mixer_sample_rate.get_value() >= FMOD_MIN_SAMPLE_RATE &&
          fmod_mixer_sample_rate.get_value() <= FMOD_MAX_SAMPLE_RATE) {
          sample_rate = fmod_mixer_sample_rate;
          audio_debug("Using user specified sample rate");
      } else {
        fmodAudio_cat.warning()
          << "fmod-mixer-sample-rate had an out-of-range value: "
          << fmod_mixer_sample_rate
          << ". Valid range is [" << FMOD_MIN_SAMPLE_RATE << ", "
          << FMOD_MAX_SAMPLE_RATE << "]\n";
      }
    }

    if (fmod_speaker_mode == FSM_unspecified) {
      if (fmod_use_surround_sound) {
        // fmod-use-surround-sound is the old variable, now replaced by fmod-
        // speaker-mode.  This is for backward compatibility.
        speaker_mode = FMOD_SPEAKERMODE_5POINT1;
      }
    } else {
      speaker_mode = (FMOD_SPEAKERMODE)fmod_speaker_mode.get_value();
    }

    // Set the mixer and speaker format.
    result = _system->setSoftwareFormat(sample_rate, speaker_mode,
                                        num_raw_speakers);
    fmod_audio_errcheck("_system->setSoftwareFormat()", result);

    // Now initialize the system.
    int nchan = fmod_number_of_sound_channels;
    int flags = FMOD_INIT_NORMAL;

    result = _system->init(nchan, flags, 0);
    if (result == FMOD_ERR_TOOMANYCHANNELS) {
      fmodAudio_cat.error()
        << "Value too large for fmod-number-of-sound-channels: " << nchan
        << "\n";
    } else {
      fmod_audio_errcheck("_system->init()", result);
    }

    _system_is_valid = (result == FMOD_OK);

    if (_system_is_valid) {
      result = _system->set3DSettings( _doppler_factor, _distance_factor, _drop_off_factor);
      fmod_audio_errcheck("_system->set3DSettings()", result);
    }
  }

  _is_valid = _system_is_valid;

  memset(&_midi_info, 0, sizeof(_midi_info));
  _midi_info.cbsize = sizeof(_midi_info);

  Filename dls_pathname = get_dls_pathname();

#ifdef IS_OSX
  // Here's a big kludge.  Don't ever let FMOD try to load this OSX-provided
  // file; it crashes messily if you do.
  // FIXME: Is this still true on FMOD Core?
  if (dls_pathname == "/System/Library/Components/CoreAudio.component/Contents/Resources/gs_instruments.dls") {
    dls_pathname = "";
  }
#endif  // IS_OSX

  if (!dls_pathname.empty()) {
    _dlsname = dls_pathname.to_os_specific();
    _midi_info.dlsname = _dlsname.c_str();
  }

  if (_is_valid) {
    result = _system->createChannelGroup("UserGroup", &_channelgroup);
    fmod_audio_errcheck("_system->createChannelGroup()", result);
  }
}

/**
 *
 */
FMODAudioManager::
~FMODAudioManager() {
  ReMutexHolder holder(_lock);

  // Be sure to delete associated sounds before deleting the manager!
  FMOD_RESULT result;

  // Release all of our sounds
  _sounds_playing.clear();
  _all_sounds.clear();

  // Remove me from the managers list.
  _all_managers.erase(this);

  if (_channelgroup) {
    _channelgroup->release();
    _channelgroup = nullptr;
  }

  if (_all_managers.empty()) {
    result = _system->release();
    fmod_audio_errcheck("_system->release()", result);
    _system = nullptr;
    _system_is_valid = false;
  }
}

/**
 * Configure the global DSP filter chain.
 *
 * FMOD has a relatively powerful DSP implementation.  It is likely that most
 * configurations will be supported.
 */
bool FMODAudioManager::
configure_filters(FilterProperties *config) {
  ReMutexHolder holder(_lock);
  FMOD_RESULT result;
  FMOD::DSP *head;
  // FMOD Core API: Use getDSP(0) to get the head DSP instead of getDSPHead
  result = _channelgroup->getDSP(0, &head);
  if (result != 0) {
    audio_error("Getting DSP head: " << FMOD_ErrorString(result) );
    return false;
  }
  update_dsp_chain(head, config);
  _active_filters = config;
  return true;
}

/**
 * This just check to make sure the FMOD System is up and running correctly.
 */
bool FMODAudioManager::
is_valid() {
  return _is_valid;
}

/**
 * This is what creates a sound instance.
 */
PT(AudioSound) FMODAudioManager::
get_sound(const Filename &file_name, bool positional, int) {
  ReMutexHolder holder(_lock);
  // Needed so People use Panda's Generic UNIX Style Paths for Filename.
  // path.to_os_specific() converts it back to the proper OS version later on.

  Filename path = file_name;

  VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
  vfs->resolve_filename(path, get_model_path());

  // Locate the file on disk.
  path.set_binary();
  PT(VirtualFile) file = vfs->get_file(path);
  if (file != nullptr) {
    // Build a new AudioSound from the audio data.
    PT(FMODAudioSound) sound = new FMODAudioSound(this, file, positional);

    _all_sounds.insert(sound);
    return sound;
  } else {
    audio_error("createSound(" << path << "): File not found.");
    return get_null_sound();
  }
}

/**
 * This is what creates a sound instance.
 */
PT(AudioSound) FMODAudioManager::
get_sound(MovieAudio *source, bool positional, int) {
  nassert_raise("FMOD audio manager does not support MovieAudio sources");
  return nullptr;
}

/**
 * This is to query if you are using a multichannel setup.
 */
int FMODAudioManager::
get_speaker_setup() {
  ReMutexHolder holder(_lock);
  FMOD_RESULT result;

  int sample_rate;
  FMOD_SPEAKERMODE speaker_mode;
  int num_raw_speakers;
  result = _system->getSoftwareFormat(&sample_rate,
                                      &speaker_mode,
                                      &num_raw_speakers);
  fmod_audio_errcheck("_system->getSpeakerMode()", result);

  switch (speaker_mode) {
  case FMOD_SPEAKERMODE_RAW:
    return AudioManager::SPEAKERMODE_raw;
  case FMOD_SPEAKERMODE_MONO:
    return AudioManager::SPEAKERMODE_mono;
  case FMOD_SPEAKERMODE_STEREO:
    return AudioManager::SPEAKERMODE_stereo;
  case FMOD_SPEAKERMODE_QUAD:
    return AudioManager::SPEAKERMODE_quad;
  case FMOD_SPEAKERMODE_SURROUND:
    return AudioManager::SPEAKERMODE_surround;
  case FMOD_SPEAKERMODE_5POINT1:
    return AudioManager::SPEAKERMODE_5point1;
  case FMOD_SPEAKERMODE_7POINT1:
    return AudioManager::SPEAKERMODE_7point1;
  case FMOD_SPEAKERMODE_7POINT1POINT4:
    return AudioManager::SPEAKERMODE_7point1point4;
  case FMOD_SPEAKERMODE_MAX:
    return AudioManager::SPEAKERMODE_max;
  default:
    return AudioManager::SPEAKERMODE_COUNT;
  }
}

/**
 * This is to set up FMOD to use a MultiChannel Setup.  This method is pretty
 * much useless.  To set a speaker setup in FMOD for Surround Sound, stereo,
 * or whatever you have to set the SpeakerMode BEFORE you Initialize FMOD.
 * Since Panda Inits the FMODAudioManager right when you Start it up, you are
 * never given an oppertunity to call this function.  That is why I stuck a
 * BOOL in the CONFIG.PRC file, whichs lets you flag if you want to use a
 * Multichannel or not.  That will set the speaker setup when an instance of
 * this class is constructed.  Still I put this here as a measure of good
 * faith, since you can query the speaker setup after everything in Init.
 * Also, maybe someone will completely hack Panda someday, in which one can
 * init or re-init the AudioManagers after Panda is running.
 */
void FMODAudioManager::
set_speaker_setup(AudioManager::SpeakerModeCategory cat) {
  ///ReMutexHolder holder(_lock);
  //FMOD_RESULT result;
  //FMOD_SPEAKERMODE speakerModeType = (FMOD_SPEAKERMODE)cat;
  //result = _system->setSpeakerMode( speakerModeType);
  //fmod_audio_errcheck("_system->setSpeakerMode()", result);
  fmodAudio_cat.warning()
    << "FMODAudioManager::set_speaker_setup() doesn't do anything\n";
}

/**
 * Sets the volume of the AudioManager.  It is not an override, but a
 * multiplier.
 */
void FMODAudioManager::
set_volume(PN_stdfloat volume) {
  ReMutexHolder holder(_lock);
  FMOD_RESULT result;
  result = _channelgroup->setVolume(volume);
  fmod_audio_errcheck("_channelgroup->setVolume()", result);
}

/**
 * Returns the AudioManager's volume.
 */
PN_stdfloat FMODAudioManager::
get_volume() const {
  ReMutexHolder holder(_lock);
  float volume;
  FMOD_RESULT result;
  result = _channelgroup->getVolume(&volume);
  fmod_audio_errcheck("_channelgroup->getVolume()", result);
  return (PN_stdfloat)volume;
}

/**
 * Changes output mode to write all audio to a wav file.
 */
void FMODAudioManager::
set_wavwriter(bool outputwav) {
  ReMutexHolder holder(_lock);
  if (outputwav) {
    _system->getOutput(&_saved_outputtype);
    _system->setOutput(FMOD_OUTPUTTYPE_WAVWRITER);
  }
  else {
    _system->setOutput(_saved_outputtype);
  }
}

/**
 * Turn on/off.
 */
void FMODAudioManager::
set_active(bool active) {
  ReMutexHolder holder(_lock);
  if (_active != active) {
    _active = active;

    // Tell our AudioSounds to adjust:
    for (AllSounds::iterator i = _all_sounds.begin();
         i != _all_sounds.end();
         ++i) {
      (*i)->set_active(_active);
    }
  }
}

/**
 *
 */
bool FMODAudioManager::
get_active() const {
  return _active;
}

/**
 * Stop playback on all sounds managed by this manager.
 */
void FMODAudioManager::
stop_all_sounds() {
  ReMutexHolder holder(_lock);
  // We have to walk through this list with some care, since stopping a sound
  // may also remove it from the set (if there are no other references to the
  // sound).
  AllSounds::iterator i;
  i = _all_sounds.begin();
  while (i != _all_sounds.end()) {
    AllSounds::iterator next = i;
    ++next;

    (*i)->stop();
    i = next;
  }
}

/**
 * Perform all per-frame update functions.
 */
void FMODAudioManager::
update() {
  ReMutexHolder holder(_lock);

  // Call finished() and release our reference to sounds that have finished
  // playing.
  update_sounds();

  // Update the FMOD system
  _system->update();
}

/**
 * Set position of the "ear" that picks up 3d sounds NOW LISTEN UP!!! THIS IS
 * IMPORTANT! Both Panda3D and FMOD use a left handed coordinate system.  But
 * there is a major difference!  In Panda3D the Y-Axis is going into the
 * Screen and the Z-Axis is going up.  In FMOD the Y-Axis is going up and the
 * Z-Axis is going into the screen.  The solution is simple, we just flip the
 * Y and Z axis, as we move coordinates from Panda to FMOD and back.  What
 * does did mean to average Panda user?  Nothing, they shouldn't notice
 * anyway.  But if you decide to do any 3D audio work in here you have to keep
 * it in mind.  I told you, so you can't say I didn't.
 */
void FMODAudioManager::
audio_3d_set_listener_attributes(PN_stdfloat px, PN_stdfloat py, PN_stdfloat pz, PN_stdfloat vx, PN_stdfloat vy, PN_stdfloat vz, PN_stdfloat fx, PN_stdfloat fy, PN_stdfloat fz, PN_stdfloat ux, PN_stdfloat uy, PN_stdfloat uz) {
  ReMutexHolder holder(_lock);
  audio_debug("FMODAudioManager::audio_3d_set_listener_attributes()");

  FMOD_RESULT result;

  _position.x = px;
  _position.y = pz;
  _position.z = py;

  _velocity.x = vx;
  _velocity.y = vz;
  _velocity.z = vy;

  _forward.x = fx;
  _forward.y = fz;
  _forward.z = fy;

  _up.x = ux;
  _up.y = uz;
  _up.z = uy;

  result = _system->set3DListenerAttributes( 0, &_position, &_velocity, &_forward, &_up);
  fmod_audio_errcheck("_system->set3DListenerAttributes()", result);

}

/**
 * Get position of the "ear" that picks up 3d sounds
 */
void FMODAudioManager::
audio_3d_get_listener_attributes(PN_stdfloat *px, PN_stdfloat *py, PN_stdfloat *pz, PN_stdfloat *vx, PN_stdfloat *vy, PN_stdfloat *vz, PN_stdfloat *fx, PN_stdfloat *fy, PN_stdfloat *fz, PN_stdfloat *ux, PN_stdfloat *uy, PN_stdfloat *uz) {
  audio_error("audio3dGetListenerAttributes: currently unimplemented. Get the attributes of the attached object");

}

/**
 * Set units per meter (Fmod uses meters internally for its sound-
 * spacialization calculations)
 */
void FMODAudioManager::
audio_3d_set_distance_factor(PN_stdfloat factor) {
  ReMutexHolder holder(_lock);
  audio_debug( "FMODAudioManager::audio_3d_set_distance_factor( factor= " << factor << ")" );

  FMOD_RESULT result;

  _distance_factor = factor;

  result = _system->set3DSettings( _doppler_factor, _distance_factor, _drop_off_factor);
  fmod_audio_errcheck("_system->set3DSettings()", result);
}

/**
 * Gets units per meter (Fmod uses meters internally for its sound-
 * spacialization calculations)
 */
PN_stdfloat FMODAudioManager::
audio_3d_get_distance_factor() const {
  audio_debug("FMODAudioManager::audio_3d_get_distance_factor()");

  return _distance_factor;
}

/**
 * Exaggerates or diminishes the Doppler effect.  Defaults to 1.0
 */
void FMODAudioManager::
audio_3d_set_doppler_factor(PN_stdfloat factor) {
  ReMutexHolder holder(_lock);
  audio_debug("FMODAudioManager::audio_3d_set_doppler_factor(factor="<<factor<<")");

  FMOD_RESULT result;

  _doppler_factor = factor;

  result = _system->set3DSettings( _doppler_factor, _distance_factor, _drop_off_factor);
  fmod_audio_errcheck("_system->set3DSettings()", result);
}

/**
 *
 */
PN_stdfloat FMODAudioManager::
audio_3d_get_doppler_factor() const {
  audio_debug("FMODAudioManager::audio_3d_get_doppler_factor()");

  return _doppler_factor;
}

/**
 * Control the effect distance has on audability.  Defaults to 1.0
 */
void FMODAudioManager::
audio_3d_set_drop_off_factor(PN_stdfloat factor) {
  ReMutexHolder holder(_lock);
  audio_debug("FMODAudioManager::audio_3d_set_drop_off_factor("<<factor<<")");

  FMOD_RESULT result;

  _drop_off_factor = factor;

  result = _system->set3DSettings( _doppler_factor, _distance_factor, _drop_off_factor);
  fmod_audio_errcheck("_system->set3DSettings()", result);

}

/**
 *
 */
PN_stdfloat FMODAudioManager::
audio_3d_get_drop_off_factor() const {
  ReMutexHolder holder(_lock);
  audio_debug("FMODAudioManager::audio_3d_get_drop_off_factor()");

  return _drop_off_factor;
}

/**
 *
 */
void FMODAudioManager::
set_concurrent_sound_limit(unsigned int limit) {
  ReMutexHolder holder(_lock);
  _concurrent_sound_limit = limit;
  reduce_sounds_playing_to(_concurrent_sound_limit);
}

/**
 *
 */
unsigned int FMODAudioManager::
get_concurrent_sound_limit() const {
  return _concurrent_sound_limit;
}

/**
 *
 */
void FMODAudioManager::
reduce_sounds_playing_to(unsigned int count) {
  ReMutexHolder holder(_lock);

  // first give all sounds that have finished a chance to stop, so that these
  // get stopped first
  update_sounds();

  int limit = _sounds_playing.size() - count;
  while (limit-- > 0) {
    SoundsPlaying::iterator sound = _sounds_playing.begin();
    nassertv(sound != _sounds_playing.end());
    // When the user stops a sound, there is still a PT in the user's hand.
    // When we stop a sound here, however, this can remove the last PT.  This
    // can cause an ugly recursion where stop calls the destructor, and the
    // destructor calls stop.  To avoid this, we create a temporary PT, stop
    // the sound, and then release the PT.
    PT(FMODAudioSound) s = (*sound);
    s->stop();
  }
}

/**
 * NOT USED FOR FMOD!!! Clears a sound out of the sound cache.
 */
void FMODAudioManager::
uncache_sound(const Filename &file_name) {
  audio_debug("FMODAudioManager::uncache_sound(\""<<file_name<<"\")");
}

/**
 * NOT USED FOR FMOD!!! Clear out the sound cache.
 */
void FMODAudioManager::
clear_cache() {
  audio_debug("FMODAudioManager::clear_cache()");
}

/**
 * NOT USED FOR FMOD!!! Set the number of sounds that the cache can hold.
 */
void FMODAudioManager::
set_cache_limit(unsigned int count) {
  audio_debug("FMODAudioManager::set_cache_limit(count="<<count<<")");
}

/**
 * NOT USED FOR FMOD!!! Gets the number of sounds that the cache can hold.
 */
unsigned int FMODAudioManager::
get_cache_limit() const {
  audio_debug("FMODAudioManager::get_cache_limit() returning ");
  return 0;
}

FMOD_RESULT FMODAudioManager::
get_speaker_mode(FMOD_SPEAKERMODE &mode) const {
  int num_samples;
  int num_raw_speakers;

  return _system->getSoftwareFormat(&num_samples, &mode,
                                    &num_raw_speakers);
}

/**
 * Maps a FilterProperties::FilterType to its corresponding FMOD_DSP_TYPE.
 * Returns FMOD_DSP_TYPE_UNKNOWN for unrecognised types.
 */
static FMOD_DSP_TYPE
filter_type_to_fmod(FilterProperties::FilterType ft) {
  switch (ft) {
  case FilterProperties::FT_lowpass:    return FMOD_DSP_TYPE_LOWPASS;
  case FilterProperties::FT_highpass:   return FMOD_DSP_TYPE_HIGHPASS;
  case FilterProperties::FT_echo:       return FMOD_DSP_TYPE_ECHO;
  case FilterProperties::FT_flange:     return FMOD_DSP_TYPE_FLANGE;
  case FilterProperties::FT_distort:    return FMOD_DSP_TYPE_DISTORTION;
  case FilterProperties::FT_normalize:  return FMOD_DSP_TYPE_NORMALIZE;
  case FilterProperties::FT_parameq:    return FMOD_DSP_TYPE_PARAMEQ;
  case FilterProperties::FT_pitchshift: return FMOD_DSP_TYPE_PITCHSHIFT;
  case FilterProperties::FT_chorus:     return FMOD_DSP_TYPE_CHORUS;
  case FilterProperties::FT_sfxreverb:  return FMOD_DSP_TYPE_SFXREVERB;
  case FilterProperties::FT_compress:   return FMOD_DSP_TYPE_COMPRESSOR;
  case FilterProperties::FT_fader:      return FMOD_DSP_TYPE_FADER;
  case FilterProperties::FT_limiter:    return FMOD_DSP_TYPE_LIMITER;
  case FilterProperties::FT_pan:        return FMOD_DSP_TYPE_PAN;
  case FilterProperties::FT_tremolo:    return FMOD_DSP_TYPE_TREMOLO;
  case FilterProperties::FT_delay:      return FMOD_DSP_TYPE_DELAY;
  default:                               return FMOD_DSP_TYPE_UNKNOWN;
  }
}

/**
 * Applies FilterConfig parameters to an already-created FMOD DSP object.
 * Returns true on success.  Caller is responsible for releasing the DSP on
 * failure if it was freshly created.
 */
static bool
apply_dsp_params(FMOD::DSP *dsp, const FilterProperties::FilterConfig &conf) {
  bool ok = true;
  auto chk = [&ok](FMOD_RESULT r) { if (r != FMOD_OK) ok = false; };

  switch (conf._type) {
  case FilterProperties::FT_lowpass:
    chk(dsp->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF,     conf._a));
    chk(dsp->setParameterFloat(FMOD_DSP_LOWPASS_RESONANCE,  conf._b));
    break;
  case FilterProperties::FT_highpass:
    chk(dsp->setParameterFloat(FMOD_DSP_HIGHPASS_CUTOFF,    conf._a));
    chk(dsp->setParameterFloat(FMOD_DSP_HIGHPASS_RESONANCE, conf._b));
    break;
  case FilterProperties::FT_echo:
    chk(dsp->setParameterFloat(FMOD_DSP_ECHO_DELAY,         conf._c));
    chk(dsp->setParameterFloat(FMOD_DSP_ECHO_FEEDBACK,      conf._d));
    chk(dsp->setParameterFloat(FMOD_DSP_ECHO_DRYLEVEL,      conf._a));
    chk(dsp->setParameterFloat(FMOD_DSP_ECHO_WETLEVEL,      conf._b));
    break;
  case FilterProperties::FT_flange:
    chk(dsp->setParameterFloat(FMOD_DSP_FLANGE_MIX,         conf._a));
    chk(dsp->setParameterFloat(FMOD_DSP_FLANGE_DEPTH,       conf._c));
    chk(dsp->setParameterFloat(FMOD_DSP_FLANGE_RATE,        conf._d));
    break;
  case FilterProperties::FT_distort:
    chk(dsp->setParameterFloat(FMOD_DSP_DISTORTION_LEVEL,   conf._a));
    break;
  case FilterProperties::FT_normalize:
    chk(dsp->setParameterFloat(FMOD_DSP_NORMALIZE_FADETIME,  conf._a));
    chk(dsp->setParameterFloat(FMOD_DSP_NORMALIZE_THRESHOLD, conf._b));
    chk(dsp->setParameterFloat(FMOD_DSP_NORMALIZE_MAXAMP,    conf._c));
    break;
  case FilterProperties::FT_parameq:
    chk(dsp->setParameterFloat(FMOD_DSP_PARAMEQ_CENTER,     conf._a));
    chk(dsp->setParameterFloat(FMOD_DSP_PARAMEQ_BANDWIDTH,  conf._b));
    chk(dsp->setParameterFloat(FMOD_DSP_PARAMEQ_GAIN,       conf._c));
    break;
  case FilterProperties::FT_pitchshift:
    chk(dsp->setParameterFloat(FMOD_DSP_PITCHSHIFT_PITCH,   conf._a));
    chk(dsp->setParameterFloat(FMOD_DSP_PITCHSHIFT_FFTSIZE, conf._b));
    break;
  case FilterProperties::FT_chorus:
    chk(dsp->setParameterFloat(FMOD_DSP_CHORUS_MIX,         conf._a));
    chk(dsp->setParameterFloat(FMOD_DSP_CHORUS_RATE,        conf._f));
    chk(dsp->setParameterFloat(FMOD_DSP_CHORUS_DEPTH,       conf._g));
    break;
  case FilterProperties::FT_sfxreverb:
    chk(dsp->setParameterFloat(FMOD_DSP_SFXREVERB_DECAYTIME,        conf._d));
    chk(dsp->setParameterFloat(FMOD_DSP_SFXREVERB_EARLYDELAY,       conf._g));
    chk(dsp->setParameterFloat(FMOD_DSP_SFXREVERB_LATEDELAY,        conf._i));
    chk(dsp->setParameterFloat(FMOD_DSP_SFXREVERB_HFREFERENCE,      conf._l));
    chk(dsp->setParameterFloat(FMOD_DSP_SFXREVERB_HFDECAYRATIO,     conf._e));
    chk(dsp->setParameterFloat(FMOD_DSP_SFXREVERB_DIFFUSION,        conf._j));
    chk(dsp->setParameterFloat(FMOD_DSP_SFXREVERB_DENSITY,          conf._k));
    chk(dsp->setParameterFloat(FMOD_DSP_SFXREVERB_LOWSHELFFREQUENCY,conf._n));
    chk(dsp->setParameterFloat(FMOD_DSP_SFXREVERB_LOWSHELFGAIN,     conf._m));
    chk(dsp->setParameterFloat(FMOD_DSP_SFXREVERB_HIGHCUT,          conf._c));
    chk(dsp->setParameterFloat(FMOD_DSP_SFXREVERB_EARLYLATEMIX,     conf._f));
    chk(dsp->setParameterFloat(FMOD_DSP_SFXREVERB_WETLEVEL,         conf._b));
    chk(dsp->setParameterFloat(FMOD_DSP_SFXREVERB_DRYLEVEL,         conf._a));
    break;
  case FilterProperties::FT_compress:
    chk(dsp->setParameterFloat(FMOD_DSP_COMPRESSOR_THRESHOLD,  conf._a));
    chk(dsp->setParameterFloat(FMOD_DSP_COMPRESSOR_ATTACK,     conf._b));
    chk(dsp->setParameterFloat(FMOD_DSP_COMPRESSOR_RELEASE,    conf._c));
    chk(dsp->setParameterFloat(FMOD_DSP_COMPRESSOR_GAINMAKEUP, conf._d));
    break;
  case FilterProperties::FT_fader:
    chk(dsp->setParameterFloat(FMOD_DSP_FADER_GAIN, conf._a));
    break;
  case FilterProperties::FT_limiter:
    chk(dsp->setParameterFloat(FMOD_DSP_LIMITER_RELEASETIME,   conf._a));
    chk(dsp->setParameterFloat(FMOD_DSP_LIMITER_CEILING,       conf._b));
    chk(dsp->setParameterFloat(FMOD_DSP_LIMITER_MAXIMIZERGAIN, conf._c));
    chk(dsp->setParameterBool (FMOD_DSP_LIMITER_MODE, (bool)conf._d));
    break;
  case FilterProperties::FT_pan:
    // 22 of 24 FMOD_DSP_PAN parameters are set here.
    // FMOD_DSP_PAN_OVERALL_GAIN is internal/read-only and intentionally skipped.
    // FMOD_DSP_PAN_ATTENUATION_RANGE is read-only (FMOD-managed) and intentionally skipped.
    {
      // 2D parameters
      chk(dsp->setParameterInt  (FMOD_DSP_PAN_MODE,                 (int)conf._a));
      chk(dsp->setParameterFloat(FMOD_DSP_PAN_2D_STEREO_POSITION,   conf._b));
      chk(dsp->setParameterFloat(FMOD_DSP_PAN_2D_DIRECTION,         conf._c));
      chk(dsp->setParameterFloat(FMOD_DSP_PAN_2D_EXTENT,            conf._d));
      chk(dsp->setParameterFloat(FMOD_DSP_PAN_2D_ROTATION,          conf._e));
      chk(dsp->setParameterFloat(FMOD_DSP_PAN_2D_LFE_LEVEL,         conf._f));
      chk(dsp->setParameterInt  (FMOD_DSP_PAN_2D_STEREO_MODE,       (int)conf._g));
      chk(dsp->setParameterFloat(FMOD_DSP_PAN_2D_STEREO_SEPARATION, conf._h));
      chk(dsp->setParameterFloat(FMOD_DSP_PAN_2D_STEREO_AXIS,       conf._i));
      chk(dsp->setParameterInt  (FMOD_DSP_PAN_ENABLED_SPEAKERS,     (int)conf._j));
      // 3D position uses FMOD_DSP_PARAMETER_3DATTRIBUTES_MULTI (multi-listener
      // variant).  numlisteners=1 covers the typical single-player case; the
      // multi-listener case (split-screen/VR) would need a richer API.
      // weight[0]=1.0 ensures this listener is fully active (zero-init gives 0).
      // velocity/forward/up are hardcoded to sensible point-source defaults
      // (no Doppler, standard FMOD orientation); expose via a future API if needed.
      FMOD_DSP_PARAMETER_3DATTRIBUTES_MULTI attrs = {};
      attrs.numlisteners = 1;
      attrs.relative[0].position = {conf._k, conf._l, conf._m};
      attrs.relative[0].velocity = {0, 0, 0};
      attrs.relative[0].forward  = {0, 0, 1};
      attrs.relative[0].up       = {0, 1, 0};
      attrs.weight[0] = 1.0f;
      // absolute must also carry valid (non-zero) orientation vectors;
      // FMOD validates the struct regardless of pan mode and rejects
      // zero-initialized forward/up with FMOD_ERR_INVALID_PARAM.
      attrs.absolute.position = {0, 0, 0};
      attrs.absolute.velocity = {0, 0, 0};
      attrs.absolute.forward  = {0, 0, 1};
      attrs.absolute.up       = {0, 1, 0};
      chk(dsp->setParameterData(FMOD_DSP_PAN_3D_POSITION, &attrs, sizeof(attrs)));
      // 3D distance / extent parameters
      chk(dsp->setParameterInt  (FMOD_DSP_PAN_3D_ROLLOFF,      (int)conf._n));
      chk(dsp->setParameterFloat(FMOD_DSP_PAN_3D_MIN_DISTANCE, conf._o));
      chk(dsp->setParameterFloat(FMOD_DSP_PAN_3D_MAX_DISTANCE, conf._p));
      chk(dsp->setParameterInt  (FMOD_DSP_PAN_3D_EXTENT_MODE,  (int)conf._q));
      chk(dsp->setParameterFloat(FMOD_DSP_PAN_3D_SOUND_SIZE,   conf._r));
      chk(dsp->setParameterFloat(FMOD_DSP_PAN_3D_MIN_EXTENT,   conf._s));
      chk(dsp->setParameterFloat(FMOD_DSP_PAN_3D_PAN_BLEND,    conf._t));
      // LFE, speaker mode, height, and range override
      chk(dsp->setParameterInt  (FMOD_DSP_PAN_LFE_UPMIX_ENABLED,    (int)conf._u));
      chk(dsp->setParameterInt  (FMOD_DSP_PAN_SURROUND_SPEAKER_MODE, (int)conf._v));
      chk(dsp->setParameterFloat(FMOD_DSP_PAN_2D_HEIGHT_BLEND,       conf._w));
      chk(dsp->setParameterBool (FMOD_DSP_PAN_OVERRIDE_RANGE,        (bool)conf._x));
    }
    break;
  case FilterProperties::FT_tremolo:
    chk(dsp->setParameterFloat(FMOD_DSP_TREMOLO_FREQUENCY, conf._a));
    chk(dsp->setParameterFloat(FMOD_DSP_TREMOLO_DEPTH,     conf._b));
    chk(dsp->setParameterFloat(FMOD_DSP_TREMOLO_SHAPE,     conf._c));
    chk(dsp->setParameterFloat(FMOD_DSP_TREMOLO_SKEW,      conf._d));
    chk(dsp->setParameterFloat(FMOD_DSP_TREMOLO_DUTY,      conf._e));
    chk(dsp->setParameterFloat(FMOD_DSP_TREMOLO_SQUARE,    conf._f));
    chk(dsp->setParameterFloat(FMOD_DSP_TREMOLO_PHASE,     conf._g));
    chk(dsp->setParameterFloat(FMOD_DSP_TREMOLO_SPREAD,    conf._h));
    break;
  case FilterProperties::FT_delay:
    // FMOD requires maxdelay to be set before the channel delays.
    chk(dsp->setParameterFloat(FMOD_DSP_DELAY_MAXDELAY, conf._c));
    chk(dsp->setParameterFloat(FMOD_DSP_DELAY_CH0,      conf._a));
    chk(dsp->setParameterFloat(FMOD_DSP_DELAY_CH1,      conf._b));
    break;
  default:
    break;
  }

  if (!ok) {
    audio_error("Could not configure DSP parameters for effect type " << conf._type);
  }
  return ok;
}

/**
 * Converts a FilterConfig to an FMOD_DSP
 */
FMOD::DSP *FMODAudioManager::
make_dsp(const FilterProperties::FilterConfig &conf) {
  ReMutexHolder holder(_lock);

  FMOD_DSP_TYPE dsptype = filter_type_to_fmod(conf._type);
  if (dsptype == FMOD_DSP_TYPE_UNKNOWN) {
    audio_error("Garbage in DSP configuration data");
    return nullptr;
  }

  FMOD::DSP *dsp;
  FMOD_RESULT result = _system->createDSPByType(dsptype, &dsp);
  if (result != FMOD_OK) {
    audio_error("Could not create DSP object");
    return nullptr;
  }

  if (!apply_dsp_params(dsp, conf)) {
    dsp->release();
    return nullptr;
  }

  dsp->setUserData(USER_DSP_MAGIC);
  return dsp;
}

/**
 * Alters a DSP chain to make it match the specified configuration.
 * Always performs a full rebuild: removes all user DSPs then adds new ones.
 * For in-place parameter updates without rebuilding, use update_filters().
 */
void FMODAudioManager::
update_dsp_chain(FMOD::DSP *head, FilterProperties *config) {
  ReMutexHolder holder(_lock);
  const FilterProperties::ConfigVector &conf = config->get_config();
  FMOD_RESULT result;

  int numdsps = 0;
  result = _channelgroup->getNumDSPs(&numdsps);
  fmod_audio_errcheck("_channelgroup->getNumDSPs()", result);

  // Remove all user DSPs backwards to avoid index shifting.
  for (int i = numdsps - 1; i >= 0; i--) {
    FMOD::DSP *dsp;
    result = _channelgroup->getDSP(i, &dsp);
    fmod_audio_errcheck("_channelgroup->getDSP()", result);
    void *userdata;
    dsp->getUserData(&userdata);
    if (userdata == USER_DSP_MAGIC) {
      result = _channelgroup->removeDSP(dsp);
      fmod_audio_errcheck("_channelgroup->removeDSP()", result);
      result = dsp->release();
      fmod_audio_errcheck("dsp->release()", result);
    }
  }

  // Add new DSPs from config.
  for (int i = 0; i < (int)conf.size(); i++) {
    FMOD::DSP *dsp = make_dsp(conf[i]);
    if (dsp == nullptr) {
      audio_error("make_dsp returned nullptr for effect index " << i);
      continue;
    }
    result = _channelgroup->addDSP(i, dsp);
    fmod_audio_errcheck("_channelgroup->addDSP()", result);
  }
}

/**
 * Updates the parameters of the active DSP chain in-place.
 * The structure (count and types) of config must exactly match the currently
 * active chain, otherwise returns false without modifying anything.
 * Use configure_filters() when you need to change the chain structure.
 */
bool FMODAudioManager::
update_filters(FilterProperties *config) {
  ReMutexHolder holder(_lock);
  const FilterProperties::ConfigVector &conf = config->get_config();

  // Collect current user DSPs in order.
  int numdsps = 0;
  _channelgroup->getNumDSPs(&numdsps);
  pvector<FMOD::DSP *> user_dsps;
  for (int i = 0; i < numdsps; i++) {
    FMOD::DSP *dsp;
    _channelgroup->getDSP(i, &dsp);
    void *userdata;
    dsp->getUserData(&userdata);
    if (userdata == USER_DSP_MAGIC) user_dsps.push_back(dsp);
  }

  // Reject if count or any type doesn't match.
  if (user_dsps.size() != conf.size()) return false;
  for (size_t i = 0; i < conf.size(); i++) {
    FMOD_DSP_TYPE existing_type;
    user_dsps[i]->getType(&existing_type);
    if (existing_type != filter_type_to_fmod(conf[i]._type)) return false;
  }

  // Structure matches -- push updated parameters directly to FMOD.
  for (size_t i = 0; i < conf.size(); i++) {
    apply_dsp_params(user_dsps[i], conf[i]);
  }
  return true;
}

/**
 * Inform the manager that a sound is about to play.
 */
void FMODAudioManager::
starting_sound(FMODAudioSound *sound) {
  ReMutexHolder holder(_lock);

  // If the sound is already in there, don't do anything.
  if (_sounds_playing.find(sound) != _sounds_playing.end()) {
    return;
  }

  // first give all sounds that have finished a chance to stop, so that these
  // get stopped first
  update_sounds();

  if (_concurrent_sound_limit) {
    reduce_sounds_playing_to(_concurrent_sound_limit-1); // because we're about to add one
  }

  _sounds_playing.insert(sound);
}

/**
 * Inform the manager that a sound is finished or someone called stop on the
 * sound (this should not be called if a sound is only paused).
 */
void FMODAudioManager::
stopping_sound(FMODAudioSound *sound) {
  ReMutexHolder holder(_lock);

  _sounds_playing.erase(sound); // This could case the sound to destruct.
}

/**
 * Removes the indicated sound from the manager's list of sounds.
 */
void FMODAudioManager::
release_sound(FMODAudioSound *sound) {
  ReMutexHolder holder(_lock);
  AllSounds::iterator ai = _all_sounds.find(sound);
  if (ai != _all_sounds.end()) {
    _all_sounds.erase(ai);
  }
}

/**
 * Calls finished() on any sounds that have finished playing.
 */
void FMODAudioManager::
update_sounds() {
  ReMutexHolder holder(_lock);

  // See if any of our playing sounds have ended we must first collect a
  // seperate list of finished sounds and then iterated over those again
  // calling their finished method.  We can't call finished() within a loop
  // iterating over _sounds_playing since finished() modifies _sounds_playing
  SoundsPlaying sounds_finished;

  SoundsPlaying::iterator i = _sounds_playing.begin();
  for (; i != _sounds_playing.end(); ++i) {
    FMODAudioSound *sound = (*i);
    if (sound->status() != AudioSound::PLAYING) {
      sounds_finished.insert(*i);
    }
  }

  i = sounds_finished.begin();
  for (; i != sounds_finished.end(); ++i) {
    (**i).finished();
  }
}
