/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file filterProperties.I
 * @author jyelon
 * @date 2007-08-01
 */

/**
 * Removes all DSP postprocessing.
 */
INLINE void FilterProperties::
clear() {
  _config.clear();
}

/**
 * Add a lowpass filter to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_lowpass(PN_stdfloat cutoff_freq, PN_stdfloat resonance_q) {
  add_filter(FT_lowpass, cutoff_freq, resonance_q);
}

/**
 * Add a highpass filter to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_highpass(PN_stdfloat cutoff_freq, PN_stdfloat resonance_q) {
  add_filter(FT_highpass, cutoff_freq, resonance_q);
}

/**
 * Add a echo filter to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_echo(PN_stdfloat drymix, PN_stdfloat wetmix, PN_stdfloat delay, PN_stdfloat decayratio) {
  add_filter(FT_echo, drymix, wetmix, delay, decayratio);
}

/**
 * Add a flange filter to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_flange(PN_stdfloat drymix, PN_stdfloat wetmix, PN_stdfloat depth, PN_stdfloat rate) {
  add_filter(FT_flange, drymix, wetmix, depth, rate);
}

/**
 * Add a distort filter to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_distort(PN_stdfloat level) {
  add_filter(FT_distort, level);
}

/**
 * Add a normalize filter to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_normalize(PN_stdfloat fadetime, PN_stdfloat threshold, PN_stdfloat maxamp) {
  add_filter(FT_normalize, fadetime, threshold, maxamp);
}

/**
 * Add a parameq filter to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_parameq(PN_stdfloat center_freq, PN_stdfloat bandwidth, PN_stdfloat gain) {
  add_filter(FT_parameq, center_freq, bandwidth, gain);
}

/**
 * Add a pitchshift filter to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_pitchshift(PN_stdfloat pitch, PN_stdfloat fftsize, PN_stdfloat overlap) {
  add_filter(FT_pitchshift, pitch, fftsize, overlap);
}

/**
 * Add a chorus filter to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_chorus(PN_stdfloat drymix, PN_stdfloat wet1, PN_stdfloat wet2, PN_stdfloat wet3, PN_stdfloat delay, PN_stdfloat rate, PN_stdfloat depth) {
  add_filter(FT_chorus, drymix, wet1, wet2, wet3, delay, rate, depth);
}

/**
 * Add a reverb filter to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_sfxreverb(PN_stdfloat drylevel, PN_stdfloat room, PN_stdfloat roomhf, PN_stdfloat decaytime,
              PN_stdfloat decayhfratio, PN_stdfloat reflectionslevel, PN_stdfloat reflectionsdelay,
              PN_stdfloat reverblevel, PN_stdfloat reverbdelay, PN_stdfloat diffusion,
              PN_stdfloat density, PN_stdfloat hfreference, PN_stdfloat roomlf, PN_stdfloat lfreference) {
  add_filter(FT_sfxreverb, drylevel, room, roomhf, decaytime, decayhfratio, reflectionslevel, reflectionsdelay,
                           reverblevel, reverbdelay, diffusion, density, hfreference, roomlf, lfreference);
}

/**
 * Add a compress filter to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_compress(PN_stdfloat threshold, PN_stdfloat attack, PN_stdfloat release, PN_stdfloat gainmakeup) {
  add_filter(FT_compress, threshold, attack, release, gainmakeup);
}

/**
 * Add a fader to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_fader(PN_stdfloat gain) {
  add_filter(FT_fader, gain);
}

/**
 * Add a limiter to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_limiter(PN_stdfloat releasetime, PN_stdfloat ceiling,
            PN_stdfloat maximizergain, PN_stdfloat mode) {
  add_filter(FT_limiter, releasetime, ceiling, maximizergain, mode);
}

/**
 * Add a pan filter to the end of the DSP chain.
 *
 * mode:                0=mono, 1=stereo, 2=surround (default: surround)
 * stereo_position:     -100 (full left) to 100 (full right)
 * direction:           surround pan direction in degrees [-180, 180]
 * extent_2d:           surround pan extent in degrees [0, 360]
 * rotation:            surround pan rotation in degrees [-180, 180]
 * lfe_level:           LFE / subwoofer level in dB [-80, 20]
 * stereo_mode:         0=distributed, 1=discrete
 * stereo_separation:   stereo separation in degrees [-180, 180] (discrete mode)
 * stereo_axis:         stereo axis in degrees [-180, 180] (discrete mode)
 * enabled_speakers:    bitmask of active speakers [0, 0xFFF]; default 0xFFF (all)
 * pos_3d_x/y/z:        relative 3D position of the sound source
 * rolloff:             0=linearsquared, 1=linear, 2=inverse, 3=inversetapered, 4=custom
 * min_distance:        3D min attenuation distance (when override_range=1)
 * max_distance:        3D max attenuation distance (when override_range=1)
 * extent_mode:         0=auto, 1=user, 2=off
 * sound_size:          extent of the sound source in distance units
 * min_extent:          minimum extent in degrees [0, 360]
 * pan_blend:           2D/3D pan blend [0, 1]; 0=full 2D (direction), 1=full 3D (position)
 * lfe_upmix_enabled:   0=off, 1=on; upmix non-LFE signal to LFE channel
 * surround_speaker_mode: FMOD speaker mode for surround (0=default system mode)
 * height_blend:        blend between ground and top speakers [-1, 1]
 * override_range:      1=use min/max_distance, 0=use FMOD attenuation range
 */
INLINE void FilterProperties::
add_pan(int mode, PN_stdfloat stereo_position,
        PN_stdfloat direction, PN_stdfloat extent_2d,
        PN_stdfloat rotation, PN_stdfloat lfe_level,
        int stereo_mode, PN_stdfloat stereo_separation,
        PN_stdfloat stereo_axis, int enabled_speakers,
        PN_stdfloat pos_3d_x, PN_stdfloat pos_3d_y, PN_stdfloat pos_3d_z,
        int rolloff, PN_stdfloat min_distance, PN_stdfloat max_distance,
        int extent_mode, PN_stdfloat sound_size, PN_stdfloat min_extent,
        PN_stdfloat pan_blend, bool lfe_upmix_enabled,
        int surround_speaker_mode, PN_stdfloat height_blend,
        bool override_range) {
  add_filter(FT_pan,
             mode, stereo_position, direction, extent_2d,
             rotation, lfe_level, stereo_mode, stereo_separation,
             stereo_axis, enabled_speakers, pos_3d_x, pos_3d_y,
             pos_3d_z, rolloff, min_distance, max_distance,
             extent_mode, sound_size, min_extent, pan_blend,
             lfe_upmix_enabled, surround_speaker_mode, height_blend,
             override_range);
}

/**
 * Add a tremolo filter to the end of the DSP chain.
 */
INLINE void FilterProperties::
add_tremolo(PN_stdfloat frequency, PN_stdfloat depth, PN_stdfloat shape,
            PN_stdfloat skew, PN_stdfloat duty, PN_stdfloat square,
            PN_stdfloat phase, PN_stdfloat spread) {
  add_filter(FT_tremolo, frequency, depth, shape, skew, duty, square, phase, spread);
}

/**
 * Add a delay filter to the end of the DSP chain.
 * ch0 and ch1 are the left and right channel delays in milliseconds.
 * maxdelay sets the maximum allowable delay and must be >= ch0 and ch1.
 */
INLINE void FilterProperties::
add_delay(PN_stdfloat ch0, PN_stdfloat ch1, PN_stdfloat maxdelay) {
  add_filter(FT_delay, ch0, ch1, maxdelay);
}

/**
 * Intended for use by AudioManager and AudioSound implementations: allows
 * access to the config vector.
 */
INLINE const FilterProperties::ConfigVector &FilterProperties::
get_config() {
  return _config;
}

/**
 * Returns the number of DSP filters currently in this FilterProperties.
 */
INLINE int FilterProperties::
get_num_filters() const {
  return (int)_config.size();
}

// ---------------------------------------------------------------------------
// Per-slot readers -- helper macro to reduce boilerplate
// ---------------------------------------------------------------------------
#define FP_READ_GUARD(idx, ft) \
  PTA_stdfloat r; \
  if ((idx) < 0 || (idx) >= (int)_config.size()) return r; \
  if (_config[(idx)]._type != (ft)) return r;

INLINE PTA_stdfloat FilterProperties::
get_lowpass_config(int index) const {
  FP_READ_GUARD(index, FT_lowpass)
  r.push_back(_config[index]._a);  // cutoff_freq
  r.push_back(_config[index]._b);  // resonance_q
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_highpass_config(int index) const {
  FP_READ_GUARD(index, FT_highpass)
  r.push_back(_config[index]._a);  // cutoff_freq
  r.push_back(_config[index]._b);  // resonance_q
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_echo_config(int index) const {
  FP_READ_GUARD(index, FT_echo)
  r.push_back(_config[index]._a);  // drymix
  r.push_back(_config[index]._b);  // wetmix
  r.push_back(_config[index]._c);  // delay
  r.push_back(_config[index]._d);  // decayratio
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_flange_config(int index) const {
  FP_READ_GUARD(index, FT_flange)
  r.push_back(_config[index]._a);  // drymix
  r.push_back(_config[index]._b);  // wetmix
  r.push_back(_config[index]._c);  // depth
  r.push_back(_config[index]._d);  // rate
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_distort_config(int index) const {
  FP_READ_GUARD(index, FT_distort)
  r.push_back(_config[index]._a);  // level
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_normalize_config(int index) const {
  FP_READ_GUARD(index, FT_normalize)
  r.push_back(_config[index]._a);  // fadetime
  r.push_back(_config[index]._b);  // threshold
  r.push_back(_config[index]._c);  // maxamp
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_parameq_config(int index) const {
  FP_READ_GUARD(index, FT_parameq)
  r.push_back(_config[index]._a);  // center_freq
  r.push_back(_config[index]._b);  // bandwidth
  r.push_back(_config[index]._c);  // gain
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_pitchshift_config(int index) const {
  FP_READ_GUARD(index, FT_pitchshift)
  r.push_back(_config[index]._a);  // pitch
  r.push_back(_config[index]._b);  // fftsize
  r.push_back(_config[index]._c);  // overlap
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_chorus_config(int index) const {
  FP_READ_GUARD(index, FT_chorus)
  r.push_back(_config[index]._a);  // drymix
  r.push_back(_config[index]._b);  // wet1
  r.push_back(_config[index]._c);  // wet2
  r.push_back(_config[index]._d);  // wet3
  r.push_back(_config[index]._e);  // delay
  r.push_back(_config[index]._f);  // rate
  r.push_back(_config[index]._g);  // depth
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_sfxreverb_config(int index) const {
  FP_READ_GUARD(index, FT_sfxreverb)
  r.push_back(_config[index]._a);  // drylevel
  r.push_back(_config[index]._b);  // room
  r.push_back(_config[index]._c);  // roomhf
  r.push_back(_config[index]._d);  // decaytime
  r.push_back(_config[index]._e);  // decayhfratio
  r.push_back(_config[index]._f);  // reflectionslevel
  r.push_back(_config[index]._g);  // reflectionsdelay
  r.push_back(_config[index]._h);  // reverblevel
  r.push_back(_config[index]._i);  // reverbdelay
  r.push_back(_config[index]._j);  // diffusion
  r.push_back(_config[index]._k);  // density
  r.push_back(_config[index]._l);  // hfreference
  r.push_back(_config[index]._m);  // roomlf
  r.push_back(_config[index]._n);  // lfreference
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_compress_config(int index) const {
  FP_READ_GUARD(index, FT_compress)
  r.push_back(_config[index]._a);  // threshold
  r.push_back(_config[index]._b);  // attack
  r.push_back(_config[index]._c);  // release
  r.push_back(_config[index]._d);  // gainmakeup
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_fader_config(int index) const {
  FP_READ_GUARD(index, FT_fader)
  r.push_back(_config[index]._a);  // gain
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_limiter_config(int index) const {
  FP_READ_GUARD(index, FT_limiter)
  r.push_back(_config[index]._a);  // releasetime
  r.push_back(_config[index]._b);  // ceiling
  r.push_back(_config[index]._c);  // maximizergain
  r.push_back(_config[index]._d);  // mode
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_pan_config(int index) const {
  FP_READ_GUARD(index, FT_pan)
  r.push_back(_config[index]._a);  // mode
  r.push_back(_config[index]._b);  // stereo_position
  r.push_back(_config[index]._c);  // direction
  r.push_back(_config[index]._d);  // extent_2d
  r.push_back(_config[index]._e);  // rotation
  r.push_back(_config[index]._f);  // lfe_level
  r.push_back(_config[index]._g);  // stereo_mode
  r.push_back(_config[index]._h);  // stereo_separation
  r.push_back(_config[index]._i);  // stereo_axis
  r.push_back(_config[index]._j);  // enabled_speakers
  r.push_back(_config[index]._k);  // pos_3d_x
  r.push_back(_config[index]._l);  // pos_3d_y
  r.push_back(_config[index]._m);  // pos_3d_z
  r.push_back(_config[index]._n);  // rolloff
  r.push_back(_config[index]._o);  // min_distance
  r.push_back(_config[index]._p);  // max_distance
  r.push_back(_config[index]._q);  // extent_mode
  r.push_back(_config[index]._r);  // sound_size
  r.push_back(_config[index]._s);  // min_extent
  r.push_back(_config[index]._t);  // pan_blend
  r.push_back(_config[index]._u);  // lfe_upmix_enabled
  r.push_back(_config[index]._v);  // surround_speaker_mode
  r.push_back(_config[index]._w);  // height_blend
  r.push_back(_config[index]._x);  // override_range
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_tremolo_config(int index) const {
  FP_READ_GUARD(index, FT_tremolo)
  r.push_back(_config[index]._a);  // frequency
  r.push_back(_config[index]._b);  // depth
  r.push_back(_config[index]._c);  // shape
  r.push_back(_config[index]._d);  // skew
  r.push_back(_config[index]._e);  // duty
  r.push_back(_config[index]._f);  // square
  r.push_back(_config[index]._g);  // phase
  r.push_back(_config[index]._h);  // spread
  return r;
}

INLINE PTA_stdfloat FilterProperties::
get_delay_config(int index) const {
  FP_READ_GUARD(index, FT_delay)
  r.push_back(_config[index]._a);  // ch0
  r.push_back(_config[index]._b);  // ch1
  r.push_back(_config[index]._c);  // maxdelay
  return r;
}

#undef FP_READ_GUARD

// ---------------------------------------------------------------------------
// Per-slot writers -- helper macro to reduce boilerplate
// ---------------------------------------------------------------------------
#define FP_WRITE_GUARD(idx, ft) \
  if ((idx) < 0 || (idx) >= (int)_config.size()) return false; \
  if (_config[(idx)]._type != (ft)) return false;

INLINE bool FilterProperties::
update_lowpass(int index, PN_stdfloat cutoff_freq, PN_stdfloat resonance_q) {
  FP_WRITE_GUARD(index, FT_lowpass)
  _config[index]._a = cutoff_freq;
  _config[index]._b = resonance_q;
  return true;
}

INLINE bool FilterProperties::
update_highpass(int index, PN_stdfloat cutoff_freq, PN_stdfloat resonance_q) {
  FP_WRITE_GUARD(index, FT_highpass)
  _config[index]._a = cutoff_freq;
  _config[index]._b = resonance_q;
  return true;
}

INLINE bool FilterProperties::
update_echo(int index, PN_stdfloat drymix, PN_stdfloat wetmix,
            PN_stdfloat delay, PN_stdfloat decayratio) {
  FP_WRITE_GUARD(index, FT_echo)
  _config[index]._a = drymix;
  _config[index]._b = wetmix;
  _config[index]._c = delay;
  _config[index]._d = decayratio;
  return true;
}

INLINE bool FilterProperties::
update_flange(int index, PN_stdfloat drymix, PN_stdfloat wetmix,
              PN_stdfloat depth, PN_stdfloat rate) {
  FP_WRITE_GUARD(index, FT_flange)
  _config[index]._a = drymix;
  _config[index]._b = wetmix;
  _config[index]._c = depth;
  _config[index]._d = rate;
  return true;
}

INLINE bool FilterProperties::
update_distort(int index, PN_stdfloat level) {
  FP_WRITE_GUARD(index, FT_distort)
  _config[index]._a = level;
  return true;
}

INLINE bool FilterProperties::
update_normalize(int index, PN_stdfloat fadetime, PN_stdfloat threshold,
                 PN_stdfloat maxamp) {
  FP_WRITE_GUARD(index, FT_normalize)
  _config[index]._a = fadetime;
  _config[index]._b = threshold;
  _config[index]._c = maxamp;
  return true;
}

INLINE bool FilterProperties::
update_parameq(int index, PN_stdfloat center_freq, PN_stdfloat bandwidth,
               PN_stdfloat gain) {
  FP_WRITE_GUARD(index, FT_parameq)
  _config[index]._a = center_freq;
  _config[index]._b = bandwidth;
  _config[index]._c = gain;
  return true;
}

INLINE bool FilterProperties::
update_pitchshift(int index, PN_stdfloat pitch, PN_stdfloat fftsize,
                  PN_stdfloat overlap) {
  FP_WRITE_GUARD(index, FT_pitchshift)
  _config[index]._a = pitch;
  _config[index]._b = fftsize;
  _config[index]._c = overlap;
  return true;
}

INLINE bool FilterProperties::
update_chorus(int index, PN_stdfloat drymix, PN_stdfloat wet1, PN_stdfloat wet2,
              PN_stdfloat wet3, PN_stdfloat delay, PN_stdfloat rate,
              PN_stdfloat depth) {
  FP_WRITE_GUARD(index, FT_chorus)
  _config[index]._a = drymix;
  _config[index]._b = wet1;
  _config[index]._c = wet2;
  _config[index]._d = wet3;
  _config[index]._e = delay;
  _config[index]._f = rate;
  _config[index]._g = depth;
  return true;
}

INLINE bool FilterProperties::
update_sfxreverb(int index, PN_stdfloat drylevel, PN_stdfloat room,
                 PN_stdfloat roomhf, PN_stdfloat decaytime,
                 PN_stdfloat decayhfratio, PN_stdfloat reflectionslevel,
                 PN_stdfloat reflectionsdelay, PN_stdfloat reverblevel,
                 PN_stdfloat reverbdelay, PN_stdfloat diffusion,
                 PN_stdfloat density, PN_stdfloat hfreference,
                 PN_stdfloat roomlf, PN_stdfloat lfreference) {
  FP_WRITE_GUARD(index, FT_sfxreverb)
  _config[index]._a = drylevel;
  _config[index]._b = room;
  _config[index]._c = roomhf;
  _config[index]._d = decaytime;
  _config[index]._e = decayhfratio;
  _config[index]._f = reflectionslevel;
  _config[index]._g = reflectionsdelay;
  _config[index]._h = reverblevel;
  _config[index]._i = reverbdelay;
  _config[index]._j = diffusion;
  _config[index]._k = density;
  _config[index]._l = hfreference;
  _config[index]._m = roomlf;
  _config[index]._n = lfreference;
  return true;
}

INLINE bool FilterProperties::
update_compress(int index, PN_stdfloat threshold, PN_stdfloat attack,
                PN_stdfloat release, PN_stdfloat gainmakeup) {
  FP_WRITE_GUARD(index, FT_compress)
  _config[index]._a = threshold;
  _config[index]._b = attack;
  _config[index]._c = release;
  _config[index]._d = gainmakeup;
  return true;
}

INLINE bool FilterProperties::
update_fader(int index, PN_stdfloat gain) {
  FP_WRITE_GUARD(index, FT_fader)
  _config[index]._a = gain;
  return true;
}

INLINE bool FilterProperties::
update_limiter(int index, PN_stdfloat releasetime, PN_stdfloat ceiling,
               PN_stdfloat maximizergain, PN_stdfloat mode) {
  FP_WRITE_GUARD(index, FT_limiter)
  _config[index]._a = releasetime;
  _config[index]._b = ceiling;
  _config[index]._c = maximizergain;
  _config[index]._d = mode;
  return true;
}

INLINE bool FilterProperties::
update_pan(int index, int mode, PN_stdfloat stereo_position,
           PN_stdfloat direction, PN_stdfloat extent_2d,
           PN_stdfloat rotation, PN_stdfloat lfe_level,
           int stereo_mode, PN_stdfloat stereo_separation,
           PN_stdfloat stereo_axis, int enabled_speakers,
           PN_stdfloat pos_3d_x, PN_stdfloat pos_3d_y, PN_stdfloat pos_3d_z,
           int rolloff, PN_stdfloat min_distance, PN_stdfloat max_distance,
           int extent_mode, PN_stdfloat sound_size, PN_stdfloat min_extent,
           PN_stdfloat pan_blend, bool lfe_upmix_enabled,
           int surround_speaker_mode, PN_stdfloat height_blend,
           bool override_range) {
  FP_WRITE_GUARD(index, FT_pan)
  _config[index]._a = mode;
  _config[index]._b = stereo_position;
  _config[index]._c = direction;
  _config[index]._d = extent_2d;
  _config[index]._e = rotation;
  _config[index]._f = lfe_level;
  _config[index]._g = stereo_mode;
  _config[index]._h = stereo_separation;
  _config[index]._i = stereo_axis;
  _config[index]._j = enabled_speakers;
  _config[index]._k = pos_3d_x;
  _config[index]._l = pos_3d_y;
  _config[index]._m = pos_3d_z;
  _config[index]._n = rolloff;
  _config[index]._o = min_distance;
  _config[index]._p = max_distance;
  _config[index]._q = extent_mode;
  _config[index]._r = sound_size;
  _config[index]._s = min_extent;
  _config[index]._t = pan_blend;
  _config[index]._u = lfe_upmix_enabled;
  _config[index]._v = surround_speaker_mode;
  _config[index]._w = height_blend;
  _config[index]._x = override_range;
  return true;
}

INLINE bool FilterProperties::
update_tremolo(int index, PN_stdfloat frequency, PN_stdfloat depth,
               PN_stdfloat shape, PN_stdfloat skew, PN_stdfloat duty,
               PN_stdfloat square, PN_stdfloat phase, PN_stdfloat spread) {
  FP_WRITE_GUARD(index, FT_tremolo)
  _config[index]._a = frequency;
  _config[index]._b = depth;
  _config[index]._c = shape;
  _config[index]._d = skew;
  _config[index]._e = duty;
  _config[index]._f = square;
  _config[index]._g = phase;
  _config[index]._h = spread;
  return true;
}

INLINE bool FilterProperties::
update_delay(int index, PN_stdfloat ch0, PN_stdfloat ch1, PN_stdfloat maxdelay) {
  FP_WRITE_GUARD(index, FT_delay)
  _config[index]._a = ch0;
  _config[index]._b = ch1;
  _config[index]._c = maxdelay;
  return true;
}

#undef FP_WRITE_GUARD