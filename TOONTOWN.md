# Build Instructions

## Prerequisites

### FMOD Core API Installation

FMOD is required for audio support. Download the FMOD Core API SDK, extract to project root, then run:

```bash
# macOS - Ex: panda3d/FMOD Programmers API
python install_fmod.py --sdk-path "path/to/FMOD Programmers API"

# Windows - Ex: panda3d/FMOD Studio API Windows
python install_fmod.py --sdk-path "path\to\FMOD Studio API Windows"
```

This copies FMOD headers and libraries to the appropriate thirdparty directory.

## Quick Start

### Build and Install

| Platform | Build Command | Wheel Filename |
|----------|--------------|----------------|
| **macOS** | `python makepanda/makepanda.py --everything --wheel --threads 8` | `panda3d-1.11.0-cp314-cp314-macosx_11_0_arm64.whl` |
| **Windows** | `python makepanda\makepanda.py --everything --wheel --msvc-version=14.3 --windows-sdk=10 --no-eigen --threads 8` | `panda3d-1.11.0-cp314-cp314-win_amd64.whl` |

**Install wheel (with IDE type stubs):**
```bash
python install_panda3d_pylib.py <wheel-filename>
```

The install script automatically:
- Installs/reinstalls the wheel in the active virtual environment
- Generates type stub files (`.pyi`) for IDE autocomplete and hover documentation
- Enables IntelliSense for Panda3D's C++ extension modules

### Build Options

**System installer (DMG/EXE):**
```bash
# macOS
python makepanda/makepanda.py --everything --installer --threads 8

# Windows
python makepanda\makepanda.py --everything --installer --msvc-version=14.3 --windows-sdk=10 --no-eigen --threads 8
```
Installs Panda3D SDK system-wide with C++ headers and Python bindings.

**C++ only (no Python bindings):**
```bash
python makepanda/makepanda.py --everything --no-python --threads 8
```

### Build Notes

- **Threading**: Adjust `--threads` based on CPU cores for faster builds
- **Python Detection**: macOS automatically detects Homebrew Python (ARM/Intel) - no need for `--python-incdir` or `--python-libdir`
- **FMOD**: Automatically included if libraries found in thirdparty directory

---

## Runtime Usage

### FMOD Audio Manager Verification

```python
manager.__class__.__name__  # Returns: "AudioManager" (Python binding shows base class)
manager.get_type()          # Returns: "FMODAudioManager" (correct!)
manager.is_valid()          # Returns: True
```

### Pause/Resume Pattern

**Recommended approach** (no visual glitches with MovieTexture):
```python
sound.setPlayRate(0.0)  # Pause
sound.setPlayRate(1.0)  # Resume
```

Benefits: No visual glitches, position preserved, channel stays alive, simple.

**Alternative** (causes MovieTexture glitch):
```python
pause_time = sound.getTime()
sound.stop()
sound.setTime(pause_time)
sound.play()
```
Causes MovieTexture to jump to first frame because `stop()` resets internal time.

### Clearing DSP Filters

DSP removal requires buffer flush to avoid lingering effects:

```python
def clear_filters(self):
    if self.sfxManagerList:
        manager = self.sfxManagerList[0]

        # Remove DSPs
        filter_props = FilterProperties()
        manager.configure_filters(filter_props)

        # Flush buffers by restarting playback
        if self.sound.status() == AudioSound.PLAYING:
            current_time = self.sound.getTime()
            was_paused = self.is_paused
            self.sound.stop()
            self.sound.setTime(current_time)
            self.sound.play()
            if was_paused:
                self.sound.setPlayRate(0.0)
```

See `samples/media-player/main.py` for complete example.

---

## Technical Details: FMOD Core API Migration

<details>
<summary><b>DSP System Reversion (Click to expand)</b></summary>

### Overview

The FMOD audio manager was updated from FMOD Ex to FMOD Core API. The DSP system was reverted from individual DSP classes back to FilterProperties-based approach while maintaining FMOD Core API compatibility.

### Changes Summary

**Reverted:**
- Individual DSP class system (ChorusDSP, LowpassDSP, CompressorDSP, etc.)
- Dirty flag pattern for per-frame DSP updates
- Multi-manager DSP tracking
- ~900 lines of DSP infrastructure code

**Restored:**
- FilterProperties-based DSP configuration using generic parameters (`conf._a` through `conf._n`)
- Simple DSP chain management with `USER_DSP_MAGIC` tracking
- Concurrent sound limiting (previously not implemented in FMOD Ex)
- ~200 lines of simpler code

**Kept:**
- Full FMOD Core API support
- All DSP types: lowpass, highpass, echo, flange, distortion, normalize, parameq, pitchshift, chorus, sfxreverb, compress
- Sound tracking with `phash_set` for better performance
- Concurrent sound limit enforcement

### FMOD Core API Compatibility Fixes

**1. DSP Chain Management:**
```cpp
// Old (FMOD Ex):
_channelgroup->getDSPHead(&head);

// New (FMOD Core):
_channelgroup->getDSP(0, &head);  // Index 0 = head DSP
```

**2. DSP Removal:**
```cpp
// Old (FMOD Ex):
prev->remove();

// New (FMOD Core):
_channelgroup->removeDSP(prev);  // Owner must remove DSP
```

**3. Parameter Setting:**
```cpp
// Old (FMOD Ex):
dsp->setParameter(index, value);

// New (FMOD Core):
dsp->setParameterFloat(index, value);  // Type-specific methods
```

**4. Parameter Names:**
- Fixed: `FMOD_DSP_NORMALIZE_THRESHHOLD` → `FMOD_DSP_NORMALIZE_THRESHOLD`

**5. DSP Removal from Channel Group (CRITICAL BUG FIX):**

The old graph traversal approach never found user DSPs added via `addDSP(index, dsp)`, causing filters to persist after clearing.

```cpp
// BROKEN (doesn't work with FMOD Core API):
while (1) {
  result = head->getNumInputs(&numinputs);
  if (numinputs != 1) break;
  result = head->getInput(0, &prev, nullptr);
  if (prev->getUserData() != USER_DSP_MAGIC) break;
  _channelgroup->removeDSP(prev);  // Never finds user DSPs!
}

// CORRECT (works with FMOD Core API):
int numdsps = 0;
_channelgroup->getNumDSPs(&numdsps);
for (int i = numdsps - 1; i >= 0; i--) {  // Backwards to avoid index shift
  FMOD::DSP *dsp;
  _channelgroup->getDSP(i, &dsp);
  void *userdata;
  dsp->getUserData(&userdata);
  if (userdata == USER_DSP_MAGIC) {
    _channelgroup->removeDSP(dsp);
    dsp->release();
  }
}
```

### Build System Setup

FMOD Core API libraries moved from `thirdparty/fmod/api/core/` to `thirdparty/darwin-libs-a/fmod/`:

```
thirdparty/darwin-libs-a/fmod/
├── include/
│   ├── fmod.h
│   ├── fmod.hpp
│   ├── fmod_common.h
│   ├── fmod_dsp.h
│   ├── fmod_dsp_effects.h
│   ├── fmod_errors.h
│   └── fmod_output.h
└── lib/
    ├── libfmod.dylib (2.5MB)
    └── libfmodL.dylib (2.8MB)
```

Old FMOD Ex library (`thirdparty/darwin-libs-a/fmodex/`) removed.

**Note:** Complete FMOD SDK remains in `thirdparty/fmod/api/` for reference:
- `core/` - FMOD Core API (copied to darwin-libs-a/fmod)
- `studio/` - FMOD Studio API
- `fsbank/` - FSBank API for sound bank creation

### Files Modified

**Headers:**
- `panda/src/audiotraits/fmodAudioManager.h` - Removed DSP class methods, restored FilterProperties methods
- `panda/src/audio/config_audio.cxx` - Removed DSP class includes
- `panda/src/audio/p3audio_composite1.cxx` - Removed DSP class source includes

**Implementation:**
- `panda/src/audiotraits/fmodAudioManager.cxx` - Replaced DSP class system with FilterProperties, fixed FMOD Core API calls
- `panda/src/audiotraits/fmodAudioSound.cxx` - Fixed class name references (FmodAudio* → FMODAudio*)

**Build Output:**
- `built/lib/libfmod.dylib` (2.5MB) - FMOD Core release library
- `built/lib/libfmodL.dylib` (2.8MB) - FMOD Core logging library
- `built/lib/libp3fmod_audio.dylib` (162KB) - Panda3D FMOD audio module

### Benefits

1. **Simpler codebase:** ~700 fewer lines of DSP management code
2. **Better compatibility:** Uses FilterProperties interface consistent with other audio backends
3. **FMOD Core features:** Concurrent sound limiting now works (was stub in FMOD Ex)
4. **Maintainability:** Generic parameter approach easier to maintain than individual DSP classes
5. **Performance:** No per-frame dirty DSP checks needed

</details>