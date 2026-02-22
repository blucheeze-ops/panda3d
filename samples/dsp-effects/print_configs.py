#!/usr/bin/env python
"""
print_configs.py -- print get_*_config output for every DSP effect type.

Adds one slot of each effect to a FilterProperties, then calls the
corresponding get_*_config reader and prints the returned values.
"""
from panda3d.core import FilterProperties

fp = FilterProperties()

# Add one slot of each effect (non-default values where useful for clarity).
fp.add_lowpass   (5000.0, 1.0)
fp.add_highpass  (200.0,  0.7)
fp.add_echo      (1.0, 0.5, 500.0, 0.5)
fp.add_flange    (0.45, 0.55, 1.0, 0.1)
fp.add_distort   (0.5)
fp.add_normalize (5000.0, 0.0, 2.0)
fp.add_parameq   (8000.0, 1.0, 3.0)
fp.add_pitchshift(1.5, 1024.0, 4.0)
fp.add_chorus    (1.0, 0.0, 0.0, 0.0, 16.0, 0.5, 1.0)
fp.add_sfxreverb ()  # use defaults
fp.add_compress  (-20.0, 10.0, 100.0, 5.0)
fp.add_fader     (0.0)
fp.add_limiter   (1000.0, 0.0, 0.0, 0.0)
fp.add_pan       (1, -50.0)    # stereo mode, panned left
fp.add_tremolo   (5.0, 1.0)
fp.add_delay     (100.0, 200.0, 400.0)

print(f"Total filters: {fp.get_num_filters()}\n")

CONFIGS = [
    ("lowpass",    fp.get_lowpass_config,    0,  ["cutoff_freq", "resonance_q"]),
    ("highpass",   fp.get_highpass_config,   1,  ["cutoff_freq", "resonance_q"]),
    ("echo",       fp.get_echo_config,       2,  ["drymix", "wetmix", "delay", "decayratio"]),
    ("flange",     fp.get_flange_config,     3,  ["drymix", "wetmix", "depth", "rate"]),
    ("distort",    fp.get_distort_config,    4,  ["level"]),
    ("normalize",  fp.get_normalize_config,  5,  ["fadetime", "threshold", "maxamp"]),
    ("parameq",    fp.get_parameq_config,    6,  ["center_freq", "bandwidth", "gain"]),
    ("pitchshift", fp.get_pitchshift_config, 7,  ["pitch", "fftsize", "overlap"]),
    ("chorus",     fp.get_chorus_config,     8,  ["drymix", "wet1", "wet2", "wet3", "delay", "rate", "depth"]),
    ("sfxreverb",  fp.get_sfxreverb_config,  9,  ["drylevel", "room", "roomhf", "decaytime",
                                                   "decayhfratio", "reflectionslevel",
                                                   "reflectionsdelay", "reverblevel",
                                                   "reverbdelay", "diffusion", "density",
                                                   "hfreference", "roomlf", "lfreference"]),
    ("compress",   fp.get_compress_config,   10, ["threshold", "attack", "release", "gainmakeup"]),
    ("fader",      fp.get_fader_config,      11, ["gain"]),
    ("limiter",    fp.get_limiter_config,    12, ["releasetime", "ceiling", "maximizergain", "mode"]),
    ("pan",        fp.get_pan_config,        13, ["mode", "stereo_position", "direction",
                                                   "extent_2d", "rotation", "lfe_level",
                                                   "stereo_mode", "stereo_separation",
                                                   "stereo_axis", "enabled_speakers",
                                                   "pos_3d_x", "pos_3d_y", "pos_3d_z",
                                                   "rolloff", "min_distance", "max_distance",
                                                   "extent_mode", "sound_size", "min_extent",
                                                   "pan_blend", "lfe_upmix_enabled",
                                                   "surround_speaker_mode", "height_blend",
                                                   "override_range"]),
    ("tremolo",    fp.get_tremolo_config,    14, ["frequency", "depth", "shape", "skew",
                                                   "duty", "square", "phase", "spread"]),
    ("delay",      fp.get_delay_config,      15, ["ch0", "ch1", "maxdelay"]),
]

for name, getter, idx, param_names in CONFIGS:
    values = list(getter(idx))
    print(f"[{idx:2d}] {name}:")
    for pname, val in zip(param_names, values):
        print(f"       {pname} = {val}")

    # Sanity checks
    assert len(values) == len(param_names), \
        f"  ERROR: expected {len(param_names)} params, got {len(values)}"
    assert list(getter(idx + 1)) == [], \
        f"  ERROR: wrong-type read should return empty list"
    print()

print("All config readers verified.")
