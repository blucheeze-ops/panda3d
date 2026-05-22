#!/usr/bin/env python
"""
lowpass_sweep.py -- Lowpass filter sweep demo
Sweeps the cutoff from 200 Hz to 8000 Hz and back on tank.mp3.
"""
from panda3d.core import loadPrcFileData
loadPrcFileData("", "audio-library-name p3fmod_audio")

import math, time, os
from panda3d.core import FilterProperties
from direct.showbase.ShowBase import ShowBase

base = ShowBase()

sound_path = os.path.join(os.path.dirname(__file__), "tank.mp3")
sfx = base.loader.loadSfx(sound_path)
sfx.setLoop(True)
sfx.play()

mgr = base.sfxManagerList[0]
fp = FilterProperties()
t = 0

while True:
    cutoff = 200 + (math.sin(t) * 0.5 + 0.5) * 7800  # sweeps 200..8000 Hz
    fp.clear()
    fp.add_lowpass(cutoff, 1.0)
    mgr.configure_filters(fp)
    print(f"lowpass cutoff: {cutoff:.0f} Hz")
    t += 0.5
    time.sleep(0.0167) # Simulate ~60 FPS updates
