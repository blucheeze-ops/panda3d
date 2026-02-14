## Build Instructions

Run build on my Macbook Air:

Create an installable for MacOS (dmg). Will install the panda3d SDK system wide (C++ headers, python bindings).
    python makepanda/makepanda.py --everything --installer \
    --python-incdir /opt/homebrew/opt/python@3.14/Frameworks/Python.framework/Versions/3.14/include \
    --python-libdir /opt/homebrew/opt/python@3.14/Frameworks/Python.framework/Versions/3.14/lib --threads 8

Create a python wheel so that it can be directly installed in a python env for a python game project.
    python makepanda/makepanda.py --everything --wheel \
    --python-incdir /opt/homebrew/opt/python@3.14/Frameworks/Python.framework/Versions/3.14/include \
    --python-libdir /opt/homebrew/opt/python@3.14/Frameworks/Python.framework/Versions/3.14/lib --threads 8

Build: python3 makepanda/makepanda.py --everything --no-python --threads 8 2>&1 | tail -n 20
pip install panda3d-1.11.0-cp314-cp314-macosx_11_0_arm64.whl
pip install --force-reinstall panda3d-1.11.0-cp314-cp314-macosx_11_0_arm64.whl

Windows:

python .\makepanda\makepanda.py --everything --installer --msvc-version=14.3 --windows-sdk=10 --no-eigen --threads=16

python .\makepanda\makepanda.py --everything --wheel --msvc-version=14.3 --windows-sdk=10 --no-eigen --threads=16

## FMOD Audio System Changes

### DSP System Reversion (FMOD Core API)

The FMOD audio manager was updated to use FMOD Core API instead of FMOD Ex, but the DSP system was reverted from individual DSP classes back to the simpler FilterProperties-based approach while maintaining FMOD Core API compatibility.

#### What Changed

**Reverted:**
- Individual DSP class system (ChorusDSP, LowpassDSP, CompressorDSP, etc.)
- Dirty flag pattern for per-frame DSP updates
- Multi-manager DSP tracking
- ~900 lines of DSP infrastructure code

**Restored:**
- FilterProperties-based DSP configuration using generic parameters (conf._a through conf._n)
- Simple DSP chain management with USER_DSP_MAGIC tracking
- Concurrent sound limiting functionality (previously not implemented in FMOD Ex version)
- ~200 lines of simpler, more maintainable code

**Kept:**
- Full FMOD Core API support
- All DSP types: lowpass, highpass, echo, flange, distortion, normalize, parameq, pitchshift, chorus, sfxreverb, compress
- Sound tracking with phash_set for better performance
- Concurrent sound limit enforcement

#### FMOD Core API Compatibility Fixes

1. **DSP Chain Management:**
   ```cpp
   // Old (FMOD Ex):
   _channelgroup->getDSPHead(&head);

   // New (FMOD Core):
   _channelgroup->getDSP(0, &head);  // Index 0 = head DSP
   ```

2. **DSP Removal:**
   ```cpp
   // Old (FMOD Ex):
   prev->remove();

   // New (FMOD Core):
   _channelgroup->removeDSP(prev);  // Owner must remove DSP
   ```

3. **Parameter Setting:**
   ```cpp
   // Old (FMOD Ex):
   dsp->setParameter(index, value);

   // New (FMOD Core):
   dsp->setParameterFloat(index, value);  // Type-specific methods
   ```

4. **Parameter Names:**
   - Fixed: `FMOD_DSP_NORMALIZE_THRESHHOLD` → `FMOD_DSP_NORMALIZE_THRESHOLD`

5. **DSP Removal from Channel Group (CRITICAL BUG FIX):**
   ```cpp
   // BROKEN approach (doesn't work with FMOD Core API):
   // Traversing DSP graph using head->getInput() assumes specific connection structure
   while (1) {
     result = head->getNumInputs(&numinputs);
     if (numinputs != 1) break;
     result = head->getInput(0, &prev, nullptr);
     if (prev->getUserData() != USER_DSP_MAGIC) break;
     _channelgroup->removeDSP(prev);  // Never finds user DSPs!
   }

   // CORRECT approach (works with FMOD Core API):
   // Iterate through channel group's DSP list directly
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

   **Why this matters:** The old graph traversal approach never found user DSPs added via `addDSP(index, dsp)`, so `configure_filters()` with empty FilterProperties would return success but not actually remove any DSPs. This caused filters to persist even after clearing.

#### Build System Setup

The FMOD Core API libraries were moved from `thirdparty/fmod/api/core/` to `thirdparty/darwin-libs-a/fmod/` to match makepanda's expected directory structure:

```bash
# Directory structure created:
thirdparty/darwin-libs-a/fmod/
├── include/
│   ├── fmod/
│   │   └── fmod.h (symlink to ../fmod.h)
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

The old FMOD Ex library (`thirdparty/darwin-libs-a/fmodex/`) was removed as it's no longer needed.

**Note:** The complete FMOD SDK remains in `thirdparty/fmod/api/` for reference and includes:
- `core/` - FMOD Core API (headers and libraries copied to darwin-libs-a/fmod)
- `studio/` - FMOD Studio API
- `fsbank/` - FSBank API for sound bank creation

#### Files Modified

**Header Files:**
- `panda/src/audiotraits/fmodAudioManager.h` - Removed DSP class methods, restored FilterProperties methods
- `panda/src/audio/config_audio.cxx` - Removed DSP class includes
- `panda/src/audio/p3audio_composite1.cxx` - Removed DSP class source includes

**Implementation Files:**
- `panda/src/audiotraits/fmodAudioManager.cxx` - Replaced DSP class system with FilterProperties, fixed FMOD Core API calls
- `panda/src/audiotraits/fmodAudioSound.cxx` - Fixed class name references (FmodAudio* → FMODAudio*)

**Build Output:**
- `built/lib/libfmod.dylib` (2.5MB) - FMOD Core release library
- `built/lib/libfmodL.dylib` (2.8MB) - FMOD Core logging library
- `built/lib/libp3fmod_audio.dylib` (162KB) - Panda3D FMOD audio module

#### Benefits

1. **Simpler codebase:** ~700 fewer lines of DSP management code
2. **Better compatibility:** Uses FilterProperties interface consistent with other audio backends
3. **FMOD Core features:** Concurrent sound limiting now works (was stub in FMOD Ex version)
4. **Maintainability:** Generic parameter approach is easier to maintain than individual DSP classes
5. **Performance:** No per-frame dirty DSP checks needed



## Testing & Verification

### FMOD Audio Manager Status

FMOD Core API is working correctly in the Python wheel. Note that Python bindings show the base class name:
```python
manager.__class__.__name__  # Returns: "AudioManager"
manager.get_type()          # Returns: "FMODAudioManager" (correct!)
manager.is_valid()          # Returns: True
```

### Pause/Resume Pattern

The AudioSound interface doesn't have a dedicated pause() method. The best way to implement pause/resume is using `setPlayRate()`:

**Recommended pattern (no visual glitches):**
```python
# Pause - set play rate to 0
sound.setPlayRate(0.0)

# Resume - restore play rate
sound.setPlayRate(1.0)
```

**Benefits:**
- ✅ No visual glitches with synchronized MovieTexture
- ✅ Position preserved (getTime() returns paused position)
- ✅ Channel stays alive (status remains PLAYING)
- ✅ Simple and clean

**Alternative (has visual glitch with MovieTexture):**
```python
# Pause
pause_time = sound.getTime()
sound.stop()

# Resume
sound.setTime(pause_time)
sound.play()
```
This approach causes MovieTexture to jump to first frame during pause because `stop()` resets internal time to 0.

### Clearing DSP Filters

When clearing DSP filters using `configure_filters()` with an empty FilterProperties, the DSPs are correctly removed from the chain, but FMOD's internal buffers still contain audio that was already processed with the effects. This causes the echo/effects to persist briefly until the buffers drain naturally.

**Solution: Flush buffers by restarting playback**
```python
def clear_filters(self):
    if self.sfxManagerList:
        manager = self.sfxManagerList[0]

        # Remove DSPs from chain
        filter_props = FilterProperties()
        manager.configure_filters(filter_props)

        # Flush FMOD's internal buffers by restarting playback
        if self.sound.status() == AudioSound.PLAYING:
            current_time = self.sound.getTime()
            was_paused = self.is_paused
            self.sound.stop()
            self.sound.setTime(current_time)
            self.sound.play()
            # Restore pause state if needed
            if was_paused:
                self.sound.setPlayRate(0.0)
```

**Benefits:**
- ✅ Immediate feedback - filters are cleared instantly
- ✅ Works whether paused or playing
- ✅ Preserves playback position and pause state
- ✅ No user-perceptible glitches

See `samples/media-player/main.py` for a complete working example.