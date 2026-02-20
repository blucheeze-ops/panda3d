#!/usr/bin/env python
"""
DSP Effects Tester
==================
Interactive GUI for testing every FilterProperties DSP effect against tank.mp3
using the FMOD audio backend.

Controls
--------
  Left panel   : click an effect to select it
  Center panel : adjust parameter sliders, then press Apply to add to the chain
  Right panel  : play/pause/stop, volume, active chain display

  Space  - Play / Pause
  S      - Stop
  Enter  - Apply current effect
  C      - Clear all effects
"""

# Must come before all other Panda3D imports.
from panda3d.core import loadPrcFileData
loadPrcFileData("", "audio-library-name p3fmod_audio")
loadPrcFileData("", "win-size 1280 800")
loadPrcFileData("", "window-title FMOD DSP Effects Tester")

import os

from panda3d.core import FilterProperties, TextNode
from direct.showbase.ShowBase import ShowBase
from direct.gui.DirectGui import (
    DirectButton, DirectSlider, DirectLabel,
    DirectFrame, DirectScrolledFrame,
)
from direct.gui.OnscreenText import OnscreenText
from panda3d.core import AudioSound

# ---------------------------------------------------------------------------
# Effect catalogue
# Each row: (display_name, FilterProperties_method, [(label, min, max, default)])
# ---------------------------------------------------------------------------
EFFECTS = [
    ("Lowpass", "add_lowpass", [
        ("Cutoff Freq (Hz)", 10,    22000, 5000),
        ("Resonance Q",      0.707, 10,    1.0),
    ]),
    ("Highpass", "add_highpass", [
        ("Cutoff Freq (Hz)", 10,    22000, 5000),
        ("Resonance Q",      0.707, 10,    1.0),
    ]),
    ("Echo", "add_echo", [
        ("Dry Mix",     0,  1,    1.0),
        ("Wet Mix",     0,  1,    0.5),
        ("Delay (ms)",  10, 5000, 500),
        ("Decay Ratio", 0,  1,    0.5),
    ]),
    ("Flange", "add_flange", [
        ("Dry Mix",   0,    1,  1.0),
        ("Wet Mix",   0,    1,  0.5),
        ("Depth",     0.01, 1,  1.0),
        ("Rate (Hz)", 0,    20, 0.1),
    ]),
    ("Distort", "add_distort", [
        ("Level", 0, 1, 0.5),
    ]),
    ("Normalize", "add_normalize", [
        ("Fade Time (ms)", 0, 20000,  5000),
        ("Threshold",      0, 0.1,    0.1),
        ("Max Amp",        1, 100000, 20),
    ]),
    ("Parametric EQ", "add_parameq", [
        ("Center Freq (Hz)", 20,   22000, 8000),
        ("Bandwidth (oct)",  0.2,  5.0,   1.0),
        ("Gain",             0.05, 3.0,   1.0),
    ]),
    ("Pitch Shift", "add_pitchshift", [
        ("Pitch",    0.5, 2.0,  1.0),
        ("FFT Size", 256, 4096, 1024),
        ("Overlap",  1,   32,   4),
    ]),
    ("Chorus", "add_chorus", [
        ("Dry Mix",    0,   1,   1.0),
        ("Wet 1",      0,   1,   0.5),
        ("Wet 2",      0,   1,   0.5),
        ("Wet 3",      0,   1,   0.5),
        ("Delay (ms)", 0.1, 100, 16),
        ("Rate (Hz)",  0,   20,  0.8),
        ("Depth",      0,   1,   0.03),
    ]),
    ("SFX Reverb", "add_sfxreverb", [
        ("Dry Level (dB)",        -10000, 0,     0),
        ("Room (dB)",             -10000, 0,     -10000),
        ("Room HF (dB)",          -10000, 0,     0),
        ("Decay Time (s)",        0.1,    20,    1.0),
        ("Decay HF Ratio",        0.1,    2.0,   0.5),
        ("Reflections (dB)",      -10000, 1000,  -10000),
        ("Reflections Delay (s)", 0,      0.3,   0.02),
        ("Reverb Level (dB)",     -10000, 2000,  0),
        ("Reverb Delay (s)",      0,      0.1,   0.04),
        ("Diffusion (%)",         0,      100,   100),
        ("Density (%)",           0,      100,   100),
        ("HF Reference (Hz)",     1000,   20000, 5000),
        ("Room LF (dB)",          -10000, 0,     0),
        ("LF Reference (Hz)",     20,     1000,  250),
    ]),
    ("Compressor", "add_compress", [
        ("Threshold (dB)",   -60, 0,    0),
        ("Attack (ms)",      0.1, 500,  50),
        ("Release (ms)",     10,  5000, 50),
        ("Gain Makeup (dB)", 0,   30,   0),
    ]),
    ("Fader", "add_fader", [
        ("Gain (dB)", -80, 10, 0),
    ]),
    ("Limiter", "add_limiter", [
        ("Release Time (ms)",     1,   1000, 60),
        ("Ceiling (dB)",         -12,  0,    0),
        ("Maximizer Gain (dB)",   0,   12,   0),
        ("Mode  0=Brickwall 1=Peak", 0, 1,   0),
    ]),
    # Pan exposes the first 9 of 24 parameters (stereo / basic surround).
    # The remaining params (3D position, extent, etc.) use FMOD defaults.
    ("Pan", "add_pan", [
        ("Mode  0=Mono 1=Stereo 2=Surround", 0, 2,    2),
        ("Stereo Position",                  -1, 1,    0),
        ("2D Direction",                   -180, 180,  0),
        ("2D Extent",                         0, 360,  360),
        ("2D Rotation",                    -180, 180,  0),
        ("LFE Level (dB)",                  -80, 12,   0),
        ("Stereo Mode  0=Discrete 1=Matrix",  0, 1,    1),
        ("Stereo Separation",                 0, 360,  60),
        ("Stereo Axis",                    -180, 180,  0),
    ]),
    ("Tremolo", "add_tremolo", [
        ("Frequency (Hz)", 0.1, 20, 5.0),
        ("Depth",          0,   1,  1.0),
        ("Shape",          0,   1,  0.0),
        ("Skew",          -1,   1,  0.0),
        ("Duty Cycle",     0,   1,  0.5),
        ("Square",         0,   1,  0.0),
        ("Phase",         -1,   1,  0.0),
        ("Spread",        -1,   1,  0.0),
    ]),
    ("Delay", "add_delay", [
        ("Ch0 Delay (ms)", 0, 10000, 0),
        ("Ch1 Delay (ms)", 0, 10000, 0),
        ("Max Delay (ms)", 0, 10000, 400),
    ]),
]

# ---------------------------------------------------------------------------
# Colour palette
# ---------------------------------------------------------------------------
BG        = (0.10, 0.10, 0.15, 1)
PANEL     = (0.13, 0.13, 0.20, 1)
ACCENT    = (0.28, 0.52, 0.88, 1)
BTN_DEF   = (0.20, 0.25, 0.38, 1)
BTN_GREEN = (0.18, 0.52, 0.28, 1)
BTN_RED   = (0.52, 0.18, 0.18, 1)
BTN_BLUE  = (0.18, 0.38, 0.68, 1)
BTN_ORG   = (0.52, 0.32, 0.10, 1)
T_BRIGHT  = (0.95, 0.95, 1.00, 1)
T_DIM     = (0.60, 0.60, 0.75, 1)
T_YELLOW  = (1.00, 0.95, 0.50, 1)
T_GREEN   = (0.45, 1.00, 0.55, 1)


class DspTester(ShowBase):

    def __init__(self):
        ShowBase.__init__(self)
        self.setBackgroundColor(*BG[:3])
        self.disableMouse()

        # ── audio ──────────────────────────────────────────────────────────
        sound_path = os.path.join(os.path.dirname(__file__), "tank.mp3")
        self.sfx = self.loader.loadSfx(sound_path)
        self.sfx.setLoop(True)
        self._paused = False

        # ── state ──────────────────────────────────────────────────────────
        self._sel_idx = 0           # index into EFFECTS
        self._sliders = []          # [(DirectSlider, DirectLabel)]
        self._chain   = []          # [(display_name, method_name, [values])]
        self._btns    = []

        # ── build UI & select first effect ─────────────────────────────────
        self._build_ui()
        self._select(0)

        # ── keyboard shortcuts ──────────────────────────────────────────────
        self.accept("space", self._play_pause)
        self.accept("s",     self._stop)
        self.accept("S",     self._stop)
        self.accept("enter", self._apply)
        self.accept("c",     self._clear)
        self.accept("C",     self._clear)

    # -----------------------------------------------------------------------
    # UI construction
    # -----------------------------------------------------------------------

    def _build_ui(self):
        # Title bar
        OnscreenText(
            text="FMOD DSP Effects Tester",
            pos=(0, 0.93), scale=0.068,
            fg=T_YELLOW, align=TextNode.ACenter,
        )

        # ── LEFT PANEL: effect selector ─────────────────────────────────────
        # Panel background spans x: -1.60 → -0.70  (width 0.90)
        DirectFrame(
            frameSize=(0, 0.90, -0.97, 0.90),
            frameColor=PANEL, pos=(-1.60, 0, 0),
        )
        OnscreenText(
            text="DSP Effects", pos=(-1.15, 0.83),
            scale=0.047, fg=T_DIM, align=TextNode.ACenter,
        )

        BH     = 0.107   # vertical slot per button
        BTN_HW = 0.375   # button half-width
        BX     = -1.15   # button column centre x

        for i, effect in enumerate(EFFECTS):
            name = effect[0]
            y = 0.73 - i * BH
            btn = DirectButton(
                text=name,
                text_scale=0.039, text_fg=T_BRIGHT,
                frameSize=(-BTN_HW, BTN_HW, -0.033, 0.050),
                frameColor=BTN_DEF, relief=1,
                pos=(BX, 0, y),
                command=self._select, extraArgs=[i],
            )
            self._btns.append(btn)

        # ── CENTER PANEL: parameter sliders ─────────────────────────────────
        # Panel background spans x: -0.66 → 0.66  (width 1.32)
        DirectFrame(
            frameSize=(0, 1.32, -0.97, 0.90),
            frameColor=PANEL, pos=(-0.66, 0, 0),
        )

        self._effect_title = OnscreenText(
            text="", pos=(0.0, 0.82), scale=0.056,
            fg=T_GREEN, align=TextNode.ACenter,
        )

        # Scrollable region — deep enough for SFX Reverb's 14 params
        self._scroll = DirectScrolledFrame(
            frameSize=(-0.62, 0.61, -0.76, 0.74),
            canvasSize=(-0.60, 0.58, -2.80, 0.70),
            scrollBarWidth=0.040,
            frameColor=(0.07, 0.07, 0.11, 1),
            pos=(0.0, 0, 0.0),
        )
        self._canvas = self._scroll.getCanvas()

        # Apply / Clear buttons sit below the scroll area
        DirectButton(
            text="Apply Effect  [Enter]",
            text_scale=0.043, text_fg=T_BRIGHT,
            frameSize=(-0.29, 0.29, -0.039, 0.055),
            frameColor=BTN_GREEN, relief=1,
            pos=(-0.20, 0, -0.876), command=self._apply,
        )
        DirectButton(
            text="Clear All  [C]",
            text_scale=0.043, text_fg=T_BRIGHT,
            frameSize=(-0.22, 0.22, -0.039, 0.055),
            frameColor=BTN_RED, relief=1,
            pos=(0.46, 0, -0.876), command=self._clear,
        )

        # ── RIGHT PANEL: playback + chain list ───────────────────────────────
        # Panel background spans x: 0.82 → 1.60  (width 0.78)
        DirectFrame(
            frameSize=(0, 0.78, -0.97, 0.90),
            frameColor=PANEL, pos=(0.82, 0, 0),
        )

        RX = 1.21   # centre x of right panel

        OnscreenText(
            text="Playback", pos=(RX, 0.82),
            scale=0.049, fg=T_DIM, align=TextNode.ACenter,
        )
        DirectButton(
            text="Play / Pause  [Space]",
            text_scale=0.041, text_fg=T_BRIGHT,
            frameSize=(-0.31, 0.31, -0.037, 0.053),
            frameColor=BTN_BLUE, relief=1,
            pos=(RX, 0, 0.68), command=self._play_pause,
        )
        DirectButton(
            text="Stop  [S]",
            text_scale=0.041, text_fg=T_BRIGHT,
            frameSize=(-0.31, 0.31, -0.037, 0.053),
            frameColor=BTN_ORG, relief=1,
            pos=(RX, 0, 0.55), command=self._stop,
        )

        OnscreenText(
            text="Volume", pos=(RX, 0.41),
            scale=0.043, fg=T_DIM, align=TextNode.ACenter,
        )
        self._vol_lbl = OnscreenText(
            text="1.00", pos=(RX, 0.32),
            scale=0.039, fg=T_BRIGHT, align=TextNode.ACenter,
        )
        self._vol_sl = DirectSlider(
            range=(0, 1), value=1.0, pageSize=0.05,
            scale=0.30, pos=(RX, 0, 0.21),
            command=self._set_volume,
        )

        OnscreenText(
            text="Active Chain", pos=(RX, 0.04),
            scale=0.045, fg=T_DIM, align=TextNode.ACenter,
        )
        self._chain_lbl = OnscreenText(
            text="(none)", pos=(RX, -0.07),
            scale=0.035, fg=T_GREEN, align=TextNode.ACenter,
        )

        self._status_lbl = OnscreenText(
            text="stopped", pos=(RX, -0.77),
            scale=0.035, fg=T_DIM, align=TextNode.ACenter,
        )

        self.taskMgr.add(self._tick_status, "tick_status")

    # -----------------------------------------------------------------------
    # Effect selection
    # -----------------------------------------------------------------------

    def _select(self, idx):
        self._sel_idx = idx
        name = EFFECTS[idx][0]

        for i, btn in enumerate(self._btns):
            btn["frameColor"] = ACCENT if i == idx else BTN_DEF

        self._effect_title.setText(name)
        self._rebuild_sliders(idx)

    def _rebuild_sliders(self, idx):
        for sl, lbl in self._sliders:
            sl.destroy()
            lbl.destroy()
        self._sliders.clear()

        params = EFFECTS[idx][2]
        slot_h = 0.115
        y0     = 0.60

        for i, (label_text, pmin, pmax, default) in enumerate(params):
            y = y0 - i * slot_h

            lbl = DirectLabel(
                text=f"{label_text}: {default:.4g}",
                text_scale=0.035,
                text_fg=T_BRIGHT,
                text_align=TextNode.ALeft,
                frameColor=(0, 0, 0, 0),
                pos=(-0.56, 0, y + 0.046),
                parent=self._canvas,
            )
            sl = DirectSlider(
                range=(pmin, pmax),
                value=default,
                pageSize=(pmax - pmin) / 20.0,
                scale=0.52,
                pos=(0.02, 0, y),
                parent=self._canvas,
                command=self._slider_moved,
                extraArgs=[i, label_text, lbl],
            )
            self._sliders.append((sl, lbl))

        # Scroll back to top whenever the effect changes
        self._scroll.verticalScroll["value"] = 0

    def _slider_moved(self, idx, label_text, lbl):
        val = self._sliders[idx][0]["value"]
        lbl["text"] = f"{label_text}: {val:.4g}"

    # -----------------------------------------------------------------------
    # Filter chain
    # -----------------------------------------------------------------------

    def _apply(self):
        name, method = EFFECTS[self._sel_idx][:2]
        vals = [sl["value"] for sl, _ in self._sliders]
        self._chain.append((name, method, list(vals)))
        self._push_filters()
        self._refresh_chain_label()

    def _clear(self):
        self._chain.clear()
        if self.sfxManagerList:
            self.sfxManagerList[0].configure_filters(FilterProperties())
        self._refresh_chain_label()

    def _push_filters(self):
        fp = FilterProperties()
        for _, method, vals in self._chain:
            getattr(fp, method)(*vals)
        if self.sfxManagerList:
            self.sfxManagerList[0].configure_filters(fp)

    def _refresh_chain_label(self):
        if not self._chain:
            self._chain_lbl.setText("(none)")
        else:
            lines = [f"{i + 1}. {entry[0]}" for i, entry in enumerate(self._chain)]
            self._chain_lbl.setText("\n".join(lines))

    # -----------------------------------------------------------------------
    # Playback
    # -----------------------------------------------------------------------

    def _play_pause(self):
        if self._paused:
            self.sfx.setPlayRate(1.0)
            self._paused = False
        elif self.sfx.status() == AudioSound.PLAYING:
            self.sfx.setPlayRate(0.0)
            self._paused = True
        else:
            self.sfx.play()

    def _stop(self):
        self.sfx.stop()
        self._paused = False

    def _set_volume(self):
        val = self._vol_sl["value"]
        self.sfx.setVolume(val)
        self._vol_lbl.setText(f"{val:.2f}")

    def _tick_status(self, task):
        s = self.sfx.status()
        if self._paused:
            txt = "paused"
        elif s == AudioSound.PLAYING:
            t = self.sfx.getTime()
            L = self.sfx.length()
            txt = f"playing  {t:.1f}s / {L:.1f}s"
        else:
            txt = "stopped"
        self._status_lbl.setText(txt)
        return task.cont


DspTester().run()
