"""
Test script for FMOD Studio audio integration.

Loads Master.bank and Master.strings.bank from fmod_bank_files/,
creates an event instance for "event:/Character/Footstep", and plays it.

Usage:
    python test_studio_bank.py

Requires:
    - Panda3D built with FMOD Studio support
    - PRC config: audio-system studio
"""

from panda3d.core import loadPrcFileData

# Configure FMOD audio before any other Panda imports.
# Setting audio-library-name to p3fmod_audio enables both Core and Studio audio.
loadPrcFileData("", "audio-library-name p3fmod_audio")
loadPrcFileData("", "notify-level-audio debug")

from direct.showbase.ShowBase import ShowBase
from panda3d.core import StudioAudioManager
import sys
import os

BANK_DIR = os.path.join(os.path.dirname(__file__), "..", "..", "fmod_bank_files")
MASTER_BANK = os.path.join(BANK_DIR, "Master.bank")
MASTER_STRINGS_BANK = os.path.join(BANK_DIR, "Master.strings.bank")

EVENT_PATH = "event:/Cog/Footstep"


class StudioAudioTest(ShowBase):
    def __init__(self):
        super().__init__(windowType="none")

        print("=" * 60)
        print("FMOD Studio Audio Test")
        print("=" * 60)

        # Create the Studio audio manager.
        self.studio_mgr = StudioAudioManager.create_StudioAudioManager()
        if not self.studio_mgr or not self.studio_mgr.is_valid():
            print("ERROR: Failed to create StudioAudioManager.")
            sys.exit(1)
        print("[OK] StudioAudioManager created")

        # Load banks.
        if not self.studio_mgr.load_bank(MASTER_BANK):
            print(f"ERROR: Failed to load {MASTER_BANK}")
            sys.exit(1)
        print(f"[OK] Loaded {MASTER_BANK}")

        if not self.studio_mgr.load_bank(MASTER_STRINGS_BANK):
            print(f"ERROR: Failed to load {MASTER_STRINGS_BANK}")
            sys.exit(1)
        print(f"[OK] Loaded {MASTER_STRINGS_BANK}")

        # Create an event instance.
        self.footstep = self.studio_mgr.get_event(EVENT_PATH)
        if not self.footstep or not self.footstep.is_valid():
            print(f"ERROR: Failed to create event '{EVENT_PATH}'")
            sys.exit(1)
        print(f"[OK] Created event: {self.footstep.get_event_path()}")
        print(f"     is_3d:     {self.footstep.is_3d()}")
        print(f"     is_oneshot: {self.footstep.is_oneshot()}")

        # Set up a finished callback.
        self.footstep.set_finished_event("footstep_done")
        self.accept("footstep_done", self.on_footstep_done)

        # Play the event.
        print("\nPlaying footstep event...")
        self.footstep.set_volume(1.0)
        self.footstep.start()
        print(f"     status: {self.footstep.status()}")

        # Update task to pump the Studio system each frame.
        self.taskMgr.add(self.studio_update, "studio_update")

        # Auto-quit after a few seconds.
        self.play_count = 0
        self.taskMgr.doMethodLater(1.0, self.play_again, "play_again")

    def studio_update(self, task):
        """Pump the FMOD Studio system each frame."""
        self.studio_mgr.update()
        return task.cont

    def play_again(self, task):
        """Play the footstep a few more times, then quit."""
        self.play_count += 1
        if self.play_count > 30:
            print("\nDone. Shutting down.")
            self.studio_mgr.stop_all_events(allow_fadeout=True)
            self.taskMgr.doMethodLater(0.5, self.quit_task, "quit")
            return task.done

        print(f"\nPlaying footstep #{self.play_count + 1}...")
        self.footstep.start()
        return task.again

    def on_footstep_done(self):
        print("  -> footstep_done event received")

    def quit_task(self, task):
        self.studio_mgr.shutdown()
        print("[OK] StudioAudioManager shut down")
        sys.exit(0)


if __name__ == "__main__":
    app = StudioAudioTest()
    app.run()
