/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file filterProperties.h
 * @author jyelon
 * @date 2007-08-01
 */

#ifndef FILTERPROPERTIES_H
#define FILTERPROPERTIES_H

#include "config_audio.h"
#include "typedReferenceCount.h"
#include "pta_stdfloat.h"

/**
 * Stores a configuration for a set of audio DSP filters.
 */

class EXPCL_PANDA_AUDIO FilterProperties : public TypedReferenceCount {
 PUBLISHED:
  FilterProperties();
  ~FilterProperties();
  INLINE void clear();
  INLINE void add_lowpass(PN_stdfloat cutoff_freq, PN_stdfloat resonance_q);
  INLINE void add_highpass(PN_stdfloat cutoff_freq, PN_stdfloat resonance_q);
  INLINE void add_echo(PN_stdfloat drymix, PN_stdfloat wetmix, PN_stdfloat delay, PN_stdfloat decayratio);
  INLINE void add_flange(PN_stdfloat drymix, PN_stdfloat wetmix, PN_stdfloat depth, PN_stdfloat rate);
  INLINE void add_distort(PN_stdfloat level);
  INLINE void add_normalize(PN_stdfloat fadetime, PN_stdfloat threshold, PN_stdfloat maxamp);
  INLINE void add_parameq(PN_stdfloat center_freq, PN_stdfloat bandwidth, PN_stdfloat gain);
  INLINE void add_pitchshift(PN_stdfloat pitch, PN_stdfloat fftsize, PN_stdfloat overlap);
  INLINE void add_chorus(PN_stdfloat drymix, PN_stdfloat wet1, PN_stdfloat wet2, PN_stdfloat wet3, PN_stdfloat delay, PN_stdfloat rate, PN_stdfloat depth);
  INLINE void add_sfxreverb(PN_stdfloat drylevel=0, PN_stdfloat room=-10000, PN_stdfloat roomhf=0, PN_stdfloat decaytime=1,
                            PN_stdfloat decayhfratio=0.5, PN_stdfloat reflectionslevel=-10000, PN_stdfloat reflectionsdelay=0.02,
                            PN_stdfloat reverblevel=0, PN_stdfloat reverbdelay=0.04, PN_stdfloat diffusion=100,
                            PN_stdfloat density=100, PN_stdfloat hfreference=5000, PN_stdfloat roomlf=0, PN_stdfloat lfreference=250);
  INLINE void add_compress(PN_stdfloat threshold, PN_stdfloat attack, PN_stdfloat release, PN_stdfloat gainmakeup);
  INLINE void add_fader(PN_stdfloat gain = 0.0f);
  INLINE void add_limiter(PN_stdfloat releasetime = 1000.0f, PN_stdfloat ceiling = 0.0f,
                          PN_stdfloat maximizergain = 0.0f, PN_stdfloat mode = 0.0f);
  INLINE void add_pan(int mode = 2,
                      PN_stdfloat stereo_position = 0.0f,
                      PN_stdfloat direction = 0.0f,
                      PN_stdfloat extent_2d = 360.0f,
                      PN_stdfloat rotation = 0.0f,
                      PN_stdfloat lfe_level = 0.0f,
                      int stereo_mode = 1,
                      PN_stdfloat stereo_separation = 60.0f,
                      PN_stdfloat stereo_axis = 0.0f,
                      int enabled_speakers = 4095,
                      PN_stdfloat pos_3d_x = 0.0f,
                      PN_stdfloat pos_3d_y = 0.0f,
                      PN_stdfloat pos_3d_z = 0.0f,
                      int rolloff = 0,
                      PN_stdfloat min_distance = 1.0f,
                      PN_stdfloat max_distance = 20.0f,
                      int extent_mode = 0,
                      PN_stdfloat sound_size = 0.0f,
                      PN_stdfloat min_extent = 0.0f,
                      PN_stdfloat pan_blend = 0.0f,
                      bool lfe_upmix_enabled = false,
                      int surround_speaker_mode = 0,
                      PN_stdfloat height_blend = 0.0f,
                      bool override_range = true);
  INLINE void add_tremolo(PN_stdfloat frequency = 5.0f, PN_stdfloat depth = 1.0f,
                          PN_stdfloat shape = 0.0f, PN_stdfloat skew = 0.0f,
                          PN_stdfloat duty = 0.5f, PN_stdfloat square = 0.0f,
                          PN_stdfloat phase = 0.0f, PN_stdfloat spread = 0.0f);
  INLINE void add_delay(PN_stdfloat ch0 = 0.0f, PN_stdfloat ch1 = 0.0f,
                        PN_stdfloat maxdelay = 400.0f);

  // Query
  INLINE int get_num_filters() const;

  // Per-slot readers — return values in the same order as the corresponding
  // add_* parameters.  Returns an empty list if the slot index is out of
  // range or the DSP type at that slot does not match.
  INLINE PTA_stdfloat get_lowpass_config   (int index) const;
  INLINE PTA_stdfloat get_highpass_config  (int index) const;
  INLINE PTA_stdfloat get_echo_config      (int index) const;
  INLINE PTA_stdfloat get_flange_config    (int index) const;
  INLINE PTA_stdfloat get_distort_config   (int index) const;
  INLINE PTA_stdfloat get_normalize_config (int index) const;
  INLINE PTA_stdfloat get_parameq_config   (int index) const;
  INLINE PTA_stdfloat get_pitchshift_config(int index) const;
  INLINE PTA_stdfloat get_chorus_config    (int index) const;
  INLINE PTA_stdfloat get_sfxreverb_config (int index) const;
  INLINE PTA_stdfloat get_compress_config  (int index) const;
  INLINE PTA_stdfloat get_fader_config     (int index) const;
  INLINE PTA_stdfloat get_limiter_config   (int index) const;
  INLINE PTA_stdfloat get_pan_config       (int index) const;
  INLINE PTA_stdfloat get_tremolo_config   (int index) const;
  INLINE PTA_stdfloat get_delay_config     (int index) const;

  // Per-slot writers — mutate the parameters of an existing slot in-place.
  // Returns false if the index is out of range or the DSP type at that slot
  // does not match; returns true on success.
  INLINE bool update_lowpass   (int index, PN_stdfloat cutoff_freq, PN_stdfloat resonance_q);
  INLINE bool update_highpass  (int index, PN_stdfloat cutoff_freq, PN_stdfloat resonance_q);
  INLINE bool update_echo      (int index, PN_stdfloat drymix, PN_stdfloat wetmix,
                                PN_stdfloat delay, PN_stdfloat decayratio);
  INLINE bool update_flange    (int index, PN_stdfloat drymix, PN_stdfloat wetmix,
                                PN_stdfloat depth, PN_stdfloat rate);
  INLINE bool update_distort   (int index, PN_stdfloat level);
  INLINE bool update_normalize (int index, PN_stdfloat fadetime, PN_stdfloat threshold,
                                PN_stdfloat maxamp);
  INLINE bool update_parameq   (int index, PN_stdfloat center_freq, PN_stdfloat bandwidth,
                                PN_stdfloat gain);
  INLINE bool update_pitchshift(int index, PN_stdfloat pitch, PN_stdfloat fftsize,
                                PN_stdfloat overlap);
  INLINE bool update_chorus    (int index, PN_stdfloat drymix, PN_stdfloat wet1,
                                PN_stdfloat wet2, PN_stdfloat wet3, PN_stdfloat delay,
                                PN_stdfloat rate, PN_stdfloat depth);
  INLINE bool update_sfxreverb (int index, PN_stdfloat drylevel, PN_stdfloat room,
                                PN_stdfloat roomhf, PN_stdfloat decaytime,
                                PN_stdfloat decayhfratio, PN_stdfloat reflectionslevel,
                                PN_stdfloat reflectionsdelay, PN_stdfloat reverblevel,
                                PN_stdfloat reverbdelay, PN_stdfloat diffusion,
                                PN_stdfloat density, PN_stdfloat hfreference,
                                PN_stdfloat roomlf, PN_stdfloat lfreference);
  INLINE bool update_compress  (int index, PN_stdfloat threshold, PN_stdfloat attack,
                                PN_stdfloat release, PN_stdfloat gainmakeup);
  INLINE bool update_fader     (int index, PN_stdfloat gain);
  INLINE bool update_limiter   (int index, PN_stdfloat releasetime, PN_stdfloat ceiling,
                                PN_stdfloat maximizergain, PN_stdfloat mode);
  INLINE bool update_pan       (int index, int mode = 2,
                                PN_stdfloat stereo_position = 0.0f,
                                PN_stdfloat direction = 0.0f,
                                PN_stdfloat extent_2d = 360.0f,
                                PN_stdfloat rotation = 0.0f,
                                PN_stdfloat lfe_level = 0.0f,
                                int stereo_mode = 1,
                                PN_stdfloat stereo_separation = 60.0f,
                                PN_stdfloat stereo_axis = 0.0f,
                                int enabled_speakers = 4095,
                                PN_stdfloat pos_3d_x = 0.0f,
                                PN_stdfloat pos_3d_y = 0.0f,
                                PN_stdfloat pos_3d_z = 0.0f,
                                int rolloff = 0,
                                PN_stdfloat min_distance = 1.0f,
                                PN_stdfloat max_distance = 20.0f,
                                int extent_mode = 0,
                                PN_stdfloat sound_size = 0.0f,
                                PN_stdfloat min_extent = 0.0f,
                                PN_stdfloat pan_blend = 0.0f,
                                bool lfe_upmix_enabled = false,
                                int surround_speaker_mode = 0,
                                PN_stdfloat height_blend = 0.0f,
                                bool override_range = true);
  INLINE bool update_tremolo   (int index, PN_stdfloat frequency = 5.0f,
                                PN_stdfloat depth = 1.0f, PN_stdfloat shape = 0.0f,
                                PN_stdfloat skew = 0.0f, PN_stdfloat duty = 0.5f,
                                PN_stdfloat square = 0.0f, PN_stdfloat phase = 0.0f,
                                PN_stdfloat spread = 0.0f);
  INLINE bool update_delay     (int index, PN_stdfloat ch0 = 0.0f,
                                PN_stdfloat ch1 = 0.0f, PN_stdfloat maxdelay = 400.0f);

 public:

  enum FilterType {
    FT_lowpass,
    FT_highpass,
    FT_echo,
    FT_flange,
    FT_distort,
    FT_normalize,
    FT_parameq,
    FT_pitchshift,
    FT_chorus,
    FT_sfxreverb,
    FT_compress,
    FT_fader,
    FT_limiter,
    FT_pan,
    FT_tremolo,
    FT_delay,
  };

  struct FilterConfig {
    FilterType  _type;
    PN_stdfloat _a,_b,_c,_d;
    PN_stdfloat _e,_f,_g,_h;
    PN_stdfloat _i,_j,_k,_l;
    PN_stdfloat _m,_n,_o,_p;
    PN_stdfloat _q,_r,_s,_t;
    PN_stdfloat _u,_v,_w,_x;
  };

  typedef pvector<FilterConfig> ConfigVector;

 private:
  void add_filter(FilterType t, PN_stdfloat a=0, PN_stdfloat b=0, PN_stdfloat c=0, PN_stdfloat d=0,
                                PN_stdfloat e=0, PN_stdfloat f=0, PN_stdfloat g=0, PN_stdfloat h=0,
                                PN_stdfloat i=0, PN_stdfloat j=0, PN_stdfloat k=0, PN_stdfloat l=0,
                                PN_stdfloat m=0, PN_stdfloat n=0, PN_stdfloat o=0, PN_stdfloat p=0,
                                PN_stdfloat q=0, PN_stdfloat r=0, PN_stdfloat s=0, PN_stdfloat t_=0,
                                PN_stdfloat u=0, PN_stdfloat v=0, PN_stdfloat w=0, PN_stdfloat x=0);
  ConfigVector _config;

 public:
  INLINE const ConfigVector &get_config();

 public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedReferenceCount::init_type();
    register_type(_type_handle, "FilterProperties",
                  TypedReferenceCount::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

 private:
  static TypeHandle _type_handle;
};

#include "filterProperties.I"

#endif // FILTERPROPERTIES_H