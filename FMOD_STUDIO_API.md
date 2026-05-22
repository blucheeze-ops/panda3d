# FMOD Studio Audio API for Panda3D

The FMOD Studio audio system provides an event-driven, designer-authored audio pipeline for Panda3D. Unlike the Core audio system (which plays sound files directly), Studio audio works with **banks**, **events**, **parameters**, **buses**, and **VCAs** — all authored in the FMOD Studio desktop application.

## Configuration

Studio and Core audio are mutually exclusive. Set the audio system in your PRC config:

```
audio-system studio
studio-audio-library-name p3fmod_studio_audio
```

Additional config variables:

| Variable | Default | Description |
|---|---|---|
| `fmod-studio-live-update` | `false` | Enable FMOD Studio Live Update for real-time debugging |
| `fmod-studio-bank-path` | `""` | Default search directory for `.bank` files |

---

## StudioAudioManager

Factory-created singleton that wraps `FMOD::Studio::System`.

### Creation

```python
from panda3d.core import StudioAudioManager

mgr = StudioAudioManager.create_StudioAudioManager()
```

### System Lifecycle

```python
mgr.is_valid()    # bool — True if the FMOD system initialized successfully
mgr.update()      # Call each frame (or let Studio3DAudioManager handle it)
mgr.shutdown()    # Release all resources
```

### Bank Management

Banks must be loaded before any events they contain can be used. Always load `Master.bank` and `Master.strings.bank` first.

```python
mgr.load_bank("Master.bank")
mgr.load_bank("Master.strings.bank")
mgr.load_bank("SFX.bank")
mgr.load_bank("Music.bank", nonblocking=True)  # async load

mgr.unload_bank("SFX.bank")
mgr.unload_all_banks()
```

### Event Creation

```python
event = mgr.get_event("event:/sfx/footstep")
```

### Global Parameters

Global parameters affect all events that reference them.

```python
mgr.set_parameter("time_of_day", 0.75)
value = mgr.get_parameter("time_of_day")
mgr.set_parameter_by_label("weather", "rain")
```

### Bus Control

```python
mgr.set_bus_volume("bus:/SFX", 0.6)
vol = mgr.get_bus_volume("bus:/SFX")
mgr.set_bus_paused("bus:/Music", True)
mgr.set_bus_mute("bus:/Dialog", True)
mgr.stop_bus("bus:/SFX", allow_fadeout=True)
```

### VCA Control

```python
mgr.set_vca_volume("vca:/Master", 0.9)
vol = mgr.get_vca_volume("vca:/Master")
```

### Master Volume

Convenience methods that control the master bus.

```python
mgr.set_volume(0.8)
vol = mgr.get_volume()
```

### 3D Listener

```python
mgr.set_listener_count(1)
count = mgr.get_listener_count()

# listener index, position (x,y,z), velocity (x,y,z), forward (x,y,z), up (x,y,z)
mgr.set_listener_attributes(0,
    px, py, pz,
    vx, vy, vz,
    fx, fy, fz,
    ux, uy, uz)

mgr.set_3d_distance_factor(1.0)
mgr.set_3d_doppler_factor(1.0)
```

### Stop All

```python
mgr.stop_all_events(allow_fadeout=True)
```

---

## StudioAudioEvent

Wraps a single FMOD Studio event instance. Created via `mgr.get_event()`.

### Playback

```python
event.start()
event.stop(allow_fadeout=True)
event.key_off()                   # release a sustain point
event.set_paused(True)
is_paused = event.get_paused()
```

### Timeline

```python
event.set_timeline_position(2.5)  # seconds
pos = event.get_timeline_position()
dur = event.length()
```

### Volume & Pitch

```python
event.set_volume(0.8)             # 0.0 to 1.0
vol = event.get_volume()
event.set_pitch(1.2)              # 1.0 = normal speed
pitch = event.get_pitch()
```

### Per-Event Parameters

```python
event.set_parameter("surface", 3.0)
val = event.get_parameter("surface")
event.set_parameter_by_label("material", "gravel")
```

### 3D Attributes

```python
# Position + velocity
event.set_3d_attributes(px, py, pz, vx, vy, vz)

# Position + velocity + orientation (for directional emitters)
event.set_3d_attributes(px, py, pz, vx, vy, vz, fx, fy, fz, ux, uy, uz)
```

### Event Properties (read-only)

```python
event.is_3d()           # bool
event.is_oneshot()      # bool
event.get_event_path()  # "event:/sfx/footstep"
```

### Status

```python
status = event.status()
# Returns one of:
#   StudioAudioEvent.ES_stopped
#   StudioAudioEvent.ES_starting
#   StudioAudioEvent.ES_playing
#   StudioAudioEvent.ES_sustaining
#   StudioAudioEvent.ES_stopping

valid = event.is_valid()
```

### Reverb

```python
event.set_reverb_level(0, 0.8)   # reverb instance index, level
level = event.get_reverb_level(0)
```

### Finished Event Callback

Fire a Panda event when the Studio event stops:

```python
event.set_finished_event("explosion_done")
self.accept("explosion_done", self.on_explosion_done)
```

---

## Studio3DAudioManager

Python helper that automatically syncs event 3D positions and the listener each frame. Analogous to `Audio3DManager` but for Studio events.

```python
from direct.showbase.Studio3DAudioManager import Studio3DAudioManager
```

### Constructor

```python
audio3d = Studio3DAudioManager(mgr, listener_target=base.camera, root=base.render)
```

### Loading & Attaching Events

```python
event = audio3d.load_event("event:/sfx/footstep")
audio3d.attach_event_to_object(event, character_np)
event.start()

audio3d.detach_event(event)
events = audio3d.get_events_on_object(character_np)
```

### Listener

```python
audio3d.attach_listener(base.camera)
audio3d.detach_listener()
```

### Velocity

```python
# Manual velocity
audio3d.set_event_velocity(event, Vec3(1, 0, 0))
audio3d.set_listener_velocity(Vec3(0, 0, 0))

# Auto-compute from position deltas
audio3d.set_event_velocity_auto(event)
audio3d.set_listener_velocity_auto()

# Query
vel = audio3d.get_event_velocity(event)
vel = audio3d.get_listener_velocity()
```

### Distance & Doppler

```python
audio3d.set_distance_factor(1.0)
audio3d.set_doppler_factor(1.0)
```

### Cleanup

```python
audio3d.disable()
```

---

## Examples

### Basic Playback

```python
from panda3d.core import loadPrcFileData
loadPrcFileData("", "audio-system studio")
loadPrcFileData("", "studio-audio-library-name p3fmod_studio_audio")

from panda3d.core import StudioAudioManager
from direct.showbase.ShowBase import ShowBase

class MyApp(ShowBase):
    def __init__(self):
        super().__init__()

        mgr = StudioAudioManager.create_StudioAudioManager()
        mgr.load_bank("Master.bank")
        mgr.load_bank("Master.strings.bank")
        mgr.load_bank("SFX.bank")

        explosion = mgr.get_event("event:/sfx/explosion")
        explosion.set_volume(0.8)
        explosion.start()

app = MyApp()
app.run()
```

### Parameters

```python
# Global parameter — affects all events that reference it
mgr.set_parameter("time_of_day", 0.75)

# Per-event parameter
footstep = mgr.get_event("event:/sfx/footstep")
footstep.set_parameter("surface", 3.0)
footstep.set_parameter_by_label("material", "gravel")
footstep.start()
```

### Bus & VCA Mixing

```python
mgr.set_bus_volume("bus:/SFX", 0.6)
mgr.set_bus_volume("bus:/Music", 0.3)
mgr.set_bus_mute("bus:/Dialog", True)
mgr.set_vca_volume("vca:/Master", 0.9)
mgr.stop_bus("bus:/SFX", allow_fadeout=True)
```

### 3D Spatial Audio

```python
from direct.showbase.Studio3DAudioManager import Studio3DAudioManager

mgr = StudioAudioManager.create_StudioAudioManager()
mgr.load_bank("Master.bank")
mgr.load_bank("Master.strings.bank")
mgr.load_bank("SFX.bank")

audio3d = Studio3DAudioManager(mgr, base.camera)

# Attach an ambient event to a scene object
waterfall = audio3d.load_event("event:/ambient/waterfall")
audio3d.attach_event_to_object(waterfall, waterfall_np)
waterfall.start()

# Directional emitter with explicit orientation
siren = mgr.get_event("event:/sfx/siren")
siren.set_3d_attributes(
    10, 0, 5,     # position
    0, 0, 0,      # velocity
    0, 1, 0,      # forward
    0, 0, 1)      # up
siren.start()
```

### Sustain Points & Finished Events

```python
music = mgr.get_event("event:/music/battle_theme")
music.set_finished_event("battle_music_done")
music.start()

# Release the sustain point to let the event transition/fade out
music.key_off()

# React when the event fully stops
self.accept("battle_music_done", self.on_music_done)
```

### Bank Lifecycle (Level Streaming)

```python
mgr.load_bank("Level1.bank")
# ... play events from Level1 ...

mgr.unload_bank("Level1.bank")
mgr.load_bank("Level2.bank")
```
