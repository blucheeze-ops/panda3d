#!/usr/bin/env python
"""
pan_circle.py  --  Pan DSP stereo / surround orbit demo
========================================================
Demonstrates two Pan DSP modes and a lowpass sweep.

  [S] Stereo sweep   -- mode=1, stereo_position -100..+100 (L↔R)
                        Simple L/R pan; works on every stereo device.

  [P] Surround orbit -- mode=2, 3D position (pan_blend=1) orbiting in the
                        horizontal XZ plane.  FMOD maps the 3D position to
                        5.1 speaker channels; macOS applies HRTF for AirPods
                        Pro Spatial Audio → true front/back depth.
                        On plain stereo output only L/R is audible.

  [L] Lowpass sweep  -- cutoff 200..8000 Hz sweep, no panning.

AirPods Pro / full 3-D orbit TIP
---------------------------------
FMOD can output multichannel audio that macOS routes through AirPods Pro's
HRTF Spatial Audio engine, giving true front/back depth.

To enable it, uncomment the 5.1 line below, then:
  1. Connect AirPods Pro.
  2. System Settings → AirPods Pro → Spatial Audio → Fixed or Head Tracked.
  3. Run this script and switch to [P] Surround orbit.

Controls
--------
  S / P / L   Stereo sweep / Surround orbit / Lowpass sweep
  Up / Down   Faster / slower
  Q / Esc     Quit
"""
from panda3d.core import loadPrcFileData
loadPrcFileData("", "audio-library-name p3fmod_audio")
loadPrcFileData("", "notify-level-audio debug")
loadPrcFileData("", "win-size 620 420")
loadPrcFileData("", "window-title Pan DSP Demo")
loadPrcFileData("", "fmod-speaker-mode 5.1")

import math
import os

from panda3d.core import FilterProperties, TextNode, NodePath, LineSegs
from direct.showbase.ShowBase import ShowBase
from direct.gui.OnscreenText import OnscreenText

SPEED_DEFAULT = 0.10
SPEED_STEP    = 0.05

# FMOD Pan DSP mode constants
STEREO      = 1   # FMOD_DSP_PAN_MODE_STEREO   — uses stereo_position
SURROUND    = 2   # FMOD_DSP_PAN_MODE_SURROUND  — uses direction (2D path)
DISTRIBUTED = 0   # stereo_mode DISTRIBUTED: direction/extent steer the pan
                  # (DISCRETE=1 ignores direction; uses stereo_axis instead)
ALL_SPEAKERS = 4095
ROLLOFF_LIN  = 1


class PanCircle(ShowBase):

    def __init__(self):
        ShowBase.__init__(self)
        self.setBackgroundColor(0.06, 0.06, 0.10)
        self.disableMouse()

        sound_path = os.path.join(os.path.dirname(__file__), "tank.mp3")
        self.sfx = self.loader.loadSfx(sound_path)
        self.sfx.setLoop(True)
        self.sfx.play()

        self._speed = SPEED_DEFAULT
        self._mode  = "stereo"
        self._fp    = FilterProperties()

        self._build_ui()
        self._build_compass()

        self.accept("q",                 self.userExit)
        self.accept("Q",                 self.userExit)
        self.accept("escape",            self.userExit)
        self.accept("s",                 self._use_stereo)
        self.accept("S",                 self._use_stereo)
        self.accept("p",                 self._use_orbit)
        self.accept("P",                 self._use_orbit)
        self.accept("l",                 self._use_lowpass)
        self.accept("L",                 self._use_lowpass)
        self.accept("arrow_up",          self._faster)
        self.accept("arrow_down",        self._slower)
        self.accept("arrow_up-repeat",   self._faster)
        self.accept("arrow_down-repeat", self._slower)

        self._init_chain()
        self.taskMgr.add(self._update, "sweep")

    # ------------------------------------------------------------------
    # UI
    # ------------------------------------------------------------------

    def _build_ui(self):
        self._mode_lbl = OnscreenText(
            text=self._mode_text(),
            pos=(0, 0.90), scale=0.048,
            fg=(0.9, 0.9, 1.0, 1), align=TextNode.ACenter,
        )
        self._pos_lbl = OnscreenText(
            text="", pos=(0, 0.74), scale=0.060,
            fg=(0.5, 1.0, 0.5, 1), align=TextNode.ACenter,
        )
        self._ok_lbl = OnscreenText(
            text="", pos=(0, 0.59), scale=0.050,
            fg=(1.0, 0.75, 0.3, 1), align=TextNode.ACenter,
        )
        self._spd_lbl = OnscreenText(
            text=self._speed_text(), pos=(0, -0.90), scale=0.045,
            fg=(0.50, 0.50, 0.70, 1), align=TextNode.ACenter,
        )
        OnscreenText(
            text="(uncomment fmod-speaker-mode 5.1 for AirPods Pro Spatial Audio)",
            pos=(0, -0.98), scale=0.038,
            fg=(0.45, 0.45, 0.60, 1), align=TextNode.ACenter,
        )
        # Cardinal labels around compass
        for text, pos in [("FRONT", (0, 0.40)), ("BACK", (0, -0.40)),
                          ("LEFT",  (-0.58, 0)), ("RIGHT", (0.58, 0))]:
            OnscreenText(text=text, pos=pos, scale=0.040,
                         fg=(0.55, 0.55, 0.75, 1), align=TextNode.ACenter)

    def _build_compass(self):
        """Draw a static ring and a movable dot in 2-D render space."""
        RADIUS = 0.30
        SEGS   = 64
        ring = LineSegs("compass_ring")
        ring.setColor(0.3, 0.3, 0.5, 1)
        ring.setThickness(1.5)
        ring.moveTo(RADIUS, 0, 0)
        for i in range(1, SEGS + 1):
            a = 2 * math.pi * i / SEGS
            ring.drawTo(RADIUS * math.sin(a), 0, RADIUS * math.cos(a))
        aspect2d.attachNewNode(ring.create())

        cross = LineSegs("compass_cross")
        cross.setColor(0.25, 0.25, 0.40, 1)
        cross.setThickness(1)
        cross.moveTo(-RADIUS, 0, 0); cross.drawTo(RADIUS, 0, 0)
        cross.moveTo(0, 0, -RADIUS); cross.drawTo(0, 0, RADIUS)
        aspect2d.attachNewNode(cross.create())

        cm = LineSegs("listener")
        cm.setColor(0.8, 0.8, 1.0, 1)
        cm.setThickness(3)
        S = 0.025
        cm.moveTo(-S, 0, 0); cm.drawTo(S, 0, 0)
        cm.moveTo(0, 0, -S); cm.drawTo(0, 0, S)
        aspect2d.attachNewNode(cm.create())

        dot = LineSegs("dot")
        dot.setColor(0.3, 1.0, 0.4, 1)
        dot.setThickness(8)
        dot.moveTo(0, 0, 0); dot.drawTo(0.001, 0, 0)
        self._dot_np: NodePath = aspect2d.attachNewNode(dot.create())
        self._compass_radius = RADIUS

    # ------------------------------------------------------------------
    # DSP chain management
    # ------------------------------------------------------------------

    def _init_chain(self):
        mgr = self._mgr()
        if not mgr:
            return
        self._fp.clear()
        if self._mode == "stereo":
            # mode=1: STEREO — stereo_position drives L/R balance (-100..+100).
            self._fp.add_pan(STEREO, 0.0)
        elif self._mode == "orbit":
            # mode=2: SURROUND, 3D position path (pan_blend=1).
            # Normal circular orbit, radius 1.0, min_distance 0.5, max_distance 5.0.
            self._fp.add_pan(
                SURROUND,        # mode
                0.0,             # stereo_position  (unused in surround)
                0.0,             # direction        (unused when pan_blend=1)
                0.0,             # extent_2d = 0 → point source
                0.0,             # rotation
                0.0,             # lfe_level
                DISTRIBUTED,     # stereo_mode
                60.0,            # stereo_separation
                0.0,             # stereo_axis
                ALL_SPEAKERS,    # enabled_speakers
                0.0, 0.0, 1.0,   # pos_3d: start at front (x=0, y=0, z=1)
                ROLLOFF_LIN,     # rolloff
                0.5, 5.0,        # min/max distance (normal orbit)
                0,               # extent_mode
                0.0, 0.0,        # sound_size, min_extent
                1.0,             # pan_blend = 1 → 3D position path
                False,           # lfe_upmix_enabled
                0,               # surround_speaker_mode
                0.0,             # height_blend
                True,            # override_range
            )
        else:  # lowpass
            self._fp.add_lowpass(200.0, 1.0)
        mgr.configure_filters(self._fp)

    def _update(self, task):
        t   = task.time * self._speed * 2.0 * math.pi
        mgr = self._mgr()
        if mgr is None:
            return task.cont

        r = self._compass_radius

        if self._mode == "stereo":
            # stereo_position: -100 = full left, +100 = full right.
            stereo_pos = math.sin(t) * 100.0
            self._fp.update_pan(0, STEREO, stereo_pos)
            ok = mgr.update_filters(self._fp)
            # Dot moves left/right only (no front/back in stereo mode).
            self._dot_np.setPos(stereo_pos / 100.0 * r, 0, 0)
            self._pos_lbl.setText(f"stereo_position: {stereo_pos:+.1f}")
            self._ok_lbl.setText(f"update_filters: {ok}")

        elif self._mode == "orbit":
            # Normal circular orbit in XZ plane, radius 1.0, y=0.
            x = math.sin(t)   # +right  −left
            z = math.cos(t)   # +forward  −backward
            direction = math.degrees(math.atan2(x, z))  # for display only
            self._fp.update_pan(
                0,               # slot index
                SURROUND,        # mode
                0.0,             # stereo_position  (unused)
                0.0,             # direction        (unused when pan_blend=1)
                0.0,             # extent_2d = 0 → point source
                0.0,             # rotation
                0.0,             # lfe_level
                DISTRIBUTED,     # stereo_mode
                60.0,            # stereo_separation
                0.0,             # stereo_axis
                ALL_SPEAKERS,    # enabled_speakers
                x, 0.0, z,       # pos_3d: orbiting in XZ plane at ear level
                ROLLOFF_LIN,     # rolloff
                0.5, 5.0,        # min/max distance
                0,               # extent_mode
                0.0, 0.0,        # sound_size, min_extent
                1.0,             # pan_blend = 1 → 3D position
                False,           # lfe_upmix_enabled
                0,               # surround_speaker_mode
                0.0,             # height_blend
                True,            # override_range
            )
            ok = mgr.update_filters(self._fp)
            self._dot_np.setPos(x * r, 0, z * r)
            self._pos_lbl.setText(f"pos_3d: ({x:+.2f}, 0, {z:+.2f})  dir: {direction:+.0f}°")
            self._ok_lbl.setText(f"update_filters: {ok}")

        elif self._mode == "lowpass":
            cutoff = 200 + (math.sin(t) * 0.5 + 0.5) * 7800
            self._fp.update_lowpass(0, cutoff, 1.0)
            ok = mgr.update_filters(self._fp)
            self._dot_np.setPos(0, 0, 0)
            self._pos_lbl.setText(f"lowpass cutoff: {cutoff:.0f} Hz")
            self._ok_lbl.setText(f"update_filters: {ok}")

        return task.cont

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _mgr(self):
        return self.sfxManagerList[0] if self.sfxManagerList else None

    def _mode_text(self):
        modes = {
            "stereo":  "[S] Stereo  <-- ACTIVE  |  [P] Orbit  |  [L] Lowpass",
            "orbit":   "[S] Stereo  |  [P] Orbit  <-- ACTIVE  |  [L] Lowpass",
            "lowpass": "[S] Stereo  |  [P] Orbit  |  [L] Lowpass  <-- ACTIVE",
        }
        return modes[self._mode]

    def _use_stereo(self):
        self._mode = "stereo"
        self._mode_lbl.setText(self._mode_text())
        self._init_chain()

    def _use_orbit(self):
        self._mode = "orbit"
        self._mode_lbl.setText(self._mode_text())
        self._init_chain()

    def _use_lowpass(self):
        self._mode = "lowpass"
        self._mode_lbl.setText(self._mode_text())
        self._init_chain()

    def _faster(self):
        self._speed = min(self._speed + SPEED_STEP, 5.0)
        self._spd_lbl.setText(self._speed_text())

    def _slower(self):
        self._speed = max(self._speed - SPEED_STEP, 0.05)
        self._spd_lbl.setText(self._speed_text())

    def _speed_text(self):
        return f"speed: {self._speed:.2f} rev/s   (Up / Down to change)"


PanCircle().run()
