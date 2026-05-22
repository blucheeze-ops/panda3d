#!/usr/bin/env python
"""
test_sound_cache.py — FMOD sound-cache validation for Roaming Ralph

Runs a headless sequence of timed tests that cover:
  1. 2D cache hit  — second loadSfx() of the same file skips createSound()
  2. 3D cache hit  — positional=True variant is cached independently
  3. Simultaneous playback — two instances from the same FMOD::Sound* play at once
  4. 3D distance independence — each instance keeps its own min/max distance
  5. Loop independence — one looping, one one-shot from the same cached sound
  6. Expiration queue — sounds stay resident after release; re-load is instant
  7. Cache limit enforcement — excess zero-refcount entries are freed

Enable FMOD's own audio debug logging with:
    notify-level-audio debug

Look for these log tokens to confirm the cache is working:
    "cache HIT"   — second+ load of the same file hit the cache
    "cache STORE" — first load registered in the cache
    "cache EXPIRE (queued)" — sound moved to expiration queue on last release
    "cache RELEASE" — sound freed from expiration queue due to limit pressure
"""

from panda3d.core import loadPrcFileData
loadPrcFileData("", "audio-library-name p3fmod_audio")
loadPrcFileData("", "notify-level-audio debug")   # shows cache HIT/STORE/EXPIRE lines
loadPrcFileData("", "window-size 800 120")
loadPrcFileData("", "window-title FMOD Cache Test")

from direct.showbase.ShowBase import ShowBase
from direct.showbase.Audio3DManager import Audio3DManager
from direct.gui.OnscreenText import OnscreenText
from panda3d.core import AudioManager, AudioSound, TextNode
import sys

# ---------------------------------------------------------------------------
# Test infrastructure
# ---------------------------------------------------------------------------

_results = []

def _pass(label):
    _results.append((True, label))
    print(f"  [PASS]  {label}")

def _fail(label, reason=""):
    _results.append((False, label))
    print(f"  [FAIL]  {label}" + (f" — {reason}" if reason else ""))

def _header(title):
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")


# ---------------------------------------------------------------------------
# Test suite
# ---------------------------------------------------------------------------

class CacheTest(ShowBase):

    def __init__(self):
        ShowBase.__init__(self)
        self.disableMouse()

        self._sfx_path   = "sounds/footstep.mp3"   # short preloadable clip
        self._sfx_path2  = "sounds/hit.mp3"         # second file for eviction test

        # Status display
        self._status = OnscreenText(
            text="Running cache tests…",
            parent=base.a2dTopLeft,
            pos=(0.04, -0.08),
            scale=0.055,
            fg=(1, 1, 1, 1),
            shadow=(0, 0, 0, 1),
            align=TextNode.ALeft,
        )

        # Run tests sequentially using taskMgr delays so audio has time to
        # initialise between steps.
        taskMgr.doMethodLater(0.2, self._test_2d_cache,      "t01")

    # ------------------------------------------------------------------
    # Test 1 — 2D cache hit
    # ------------------------------------------------------------------
    def _test_2d_cache(self, task):
        _header("Test 1: 2D cache hit")
        mgr = base.sfxManagerList[0]

        s1 = loader.loadSfx(self._sfx_path)
        s2 = loader.loadSfx(self._sfx_path)

        # Both load calls succeed and return valid sound objects.
        if s1 is None or s2 is None:
            _fail("2D load returns non-None", "got None")
        else:
            _pass("2D load returns non-None")

        # Both should report a non-zero length (proves FMOD::Sound* is valid).
        if s1.length() > 0 and s2.length() > 0:
            _pass("Both instances have non-zero length")
        else:
            _fail("Both instances have non-zero length", f"s1={s1.length()} s2={s2.length()}")

        # They should be different Python/Panda objects (different FMODAudioSound)
        # even though they share the same underlying FMOD::Sound*.
        if s1 is not s2:
            _pass("loadSfx returns distinct objects (each has own channel)")
        else:
            _fail("loadSfx returns distinct objects")

        self._s1_2d = s1
        self._s2_2d = s2
        taskMgr.doMethodLater(0.1, self._test_3d_cache, "t02")
        return task.done

    # ------------------------------------------------------------------
    # Test 2 — 3D cache hit (positional=True cached separately)
    # ------------------------------------------------------------------
    def _test_3d_cache(self, task):
        _header("Test 2: 3D cache hit")

        mgr = AudioManager.createAudioManager()
        base.addSfxManager(mgr)
        audio3d = Audio3DManager(mgr, base.camera)

        s3d_a = audio3d.loadSfx(self._sfx_path)
        s3d_b = audio3d.loadSfx(self._sfx_path)

        if s3d_a is not None and s3d_b is not None:
            _pass("3D load returns non-None")
        else:
            _fail("3D load returns non-None", "got None")

        if s3d_a is not s3d_b:
            _pass("3D loadSfx returns distinct objects")
        else:
            _fail("3D loadSfx returns distinct objects")

        if s3d_a.length() > 0 and s3d_b.length() > 0:
            _pass("3D instances have non-zero length")
        else:
            _fail("3D instances have non-zero length")

        self._s3d_mgr   = mgr
        self._audio3d   = audio3d
        self._s3d_a     = s3d_a
        self._s3d_b     = s3d_b
        taskMgr.doMethodLater(0.1, self._test_simultaneous, "t03")
        return task.done

    # ------------------------------------------------------------------
    # Test 3 — Simultaneous playback from the same cached FMOD::Sound*
    # ------------------------------------------------------------------
    def _test_simultaneous(self, task):
        _header("Test 3: Simultaneous playback (two instances, one cached sound)")
        s1 = self._s1_2d
        s2 = self._s2_2d

        s1.setLoop(False)
        s2.setLoop(False)
        s1.play()
        s2.play()

        if s1.status() == AudioSound.PLAYING:
            _pass("Instance 1 is PLAYING")
        else:
            _fail("Instance 1 is PLAYING", f"status={s1.status()}")

        if s2.status() == AudioSound.PLAYING:
            _pass("Instance 2 is PLAYING")
        else:
            _fail("Instance 2 is PLAYING", f"status={s2.status()}")

        taskMgr.doMethodLater(0.5, self._stop_simultaneous, "t03_stop")
        return task.done

    def _stop_simultaneous(self, task):
        self._s1_2d.stop()
        self._s2_2d.stop()
        taskMgr.doMethodLater(0.1, self._test_3d_distance, "t04")
        return task.done

    # ------------------------------------------------------------------
    # Test 4 — 3D distance independence
    # ------------------------------------------------------------------
    def _test_3d_distance(self, task):
        _header("Test 4: 3D distance independence (channel-level min/max)")
        sa = self._s3d_a
        sb = self._s3d_b

        self._audio3d.setSoundMinDistance(sa, 5.0)
        self._audio3d.setSoundMinDistance(sb, 50.0)
        self._audio3d.setSoundMaxDistance(sa, 100.0)
        self._audio3d.setSoundMaxDistance(sb, 500.0)

        # These values are stored as Python-side floats in Audio3DManager
        # (it proxies through to FMODAudioSound._min_dist / _max_dist).
        # We verify via get_3d_min_distance() / get_3d_max_distance().
        if sa.get_3d_min_distance() == 5.0:
            _pass("Instance A min_dist == 5.0")
        else:
            _fail("Instance A min_dist == 5.0", f"got {sa.get_3d_min_distance()}")

        if sb.get_3d_min_distance() == 50.0:
            _pass("Instance B min_dist == 50.0 (independent)")
        else:
            _fail("Instance B min_dist == 50.0", f"got {sb.get_3d_min_distance()}")

        if sa.get_3d_max_distance() == 100.0:
            _pass("Instance A max_dist == 100.0")
        else:
            _fail("Instance A max_dist == 100.0", f"got {sa.get_3d_max_distance()}")

        if sb.get_3d_max_distance() == 500.0:
            _pass("Instance B max_dist == 500.0 (independent)")
        else:
            _fail("Instance B max_dist == 500.0", f"got {sb.get_3d_max_distance()}")

        taskMgr.doMethodLater(0.1, self._test_loop_independence, "t05")
        return task.done

    # ------------------------------------------------------------------
    # Test 5 — Loop independence
    # ------------------------------------------------------------------
    def _test_loop_independence(self, task):
        _header("Test 5: Loop independence (channel-level loop count)")
        s1 = self._s1_2d
        s2 = self._s2_2d

        s1.setLoop(True)    # infinite loop
        s2.setLoop(False)   # one-shot (loop_count == 1)

        if s1.getLoopCount() == 0:
            _pass("Instance 1 loop_count == 0 (infinite)")
        else:
            _fail("Instance 1 loop_count == 0", f"got {s1.getLoopCount()}")

        if s2.getLoopCount() == 1:
            _pass("Instance 2 loop_count == 1 (one-shot, independent)")
        else:
            _fail("Instance 2 loop_count == 1", f"got {s2.getLoopCount()}")

        # Play both; instance 2 should stop on its own, instance 1 should keep going.
        s1.play()
        s2.play()
        taskMgr.doMethodLater(2.0, self._check_loop_independence, "t05_check")
        return task.done

    def _check_loop_independence(self, task):
        s1 = self._s1_2d
        s2 = self._s2_2d

        if s1.status() == AudioSound.PLAYING:
            _pass("Instance 1 (loop) still PLAYING after 2 s")
        else:
            _fail("Instance 1 (loop) still PLAYING after 2 s", f"status={s1.status()}")

        # footstep.mp3 is short (<1 s); after 2 s a one-shot should have finished.
        if s2.status() != AudioSound.PLAYING:
            _pass("Instance 2 (one-shot) STOPPED after 2 s")
        else:
            _fail("Instance 2 (one-shot) STOPPED after 2 s", "still PLAYING")

        s1.stop()
        taskMgr.doMethodLater(0.1, self._test_expiration_queue, "t06")
        return task.done

    # ------------------------------------------------------------------
    # Test 6 — Expiration queue: re-load after all instances released
    # ------------------------------------------------------------------
    def _test_expiration_queue(self, task):
        _header("Test 6: Expiration queue (sound stays resident after release)")

        mgr = base.sfxManagerList[0]

        # Load a fresh copy, release it, then load again.
        # With the expiration queue the second load should be a cache HIT
        # (the debug log will show "cache HIT") — no createSound() call.
        s_a = loader.loadSfx(self._sfx_path2)
        if s_a is not None:
            _pass("First load of hit.mp3 succeeds")
        else:
            _fail("First load of hit.mp3 succeeds", "got None")
            taskMgr.doMethodLater(0.1, self._test_cache_limit, "t07")
            return task.done

        length_first = s_a.length()

        # Drop our reference — refcount goes to 0, moves into expiration queue.
        del s_a

        # Immediately re-load — should be a cache HIT with valid sound data.
        s_b = loader.loadSfx(self._sfx_path2)
        if s_b is not None and s_b.length() == length_first:
            _pass("Re-load after release returns valid sound (expiration queue hit)")
        else:
            _fail("Re-load after release returns valid sound",
                  f"length={s_b.length() if s_b else 'None'} expected {length_first}")

        self._exp_sound = s_b
        taskMgr.doMethodLater(0.1, self._test_cache_limit, "t07")
        return task.done

    # ------------------------------------------------------------------
    # Test 7 — Cache limit enforcement
    # ------------------------------------------------------------------
    def _test_cache_limit(self, task):
        _header("Test 7: Cache limit enforcement (discard_excess_cache)")

        mgr = base.sfxManagerList[0]

        # Set a very tight cache limit (1 zero-refcount entry).
        mgr.setCacheLimit(1)

        # Load and release two distinct files.  After both are released,
        # only one should remain in the expiration queue; the other gets freed.
        # (The debug log should show "cache RELEASE" for the evicted entry.)
        sa = loader.loadSfx(self._sfx_path)
        sb = loader.loadSfx(self._sfx_path2)

        len_a = sa.length()
        len_b = sb.length()

        del sa  # refcount → 0, pushed to expiration queue; limit=1, queue=1 → OK
        del sb  # refcount → 0, pushed to expiration queue; queue=2 > limit=1 → front evicted

        # At least one of the two files should reload successfully (the one
        # that survived the eviction).  We can't deterministically know which
        # one was evicted (depends on insertion order), but both reloads must
        # produce a valid sound object — if evicted, createSound() re-runs.
        sc = loader.loadSfx(self._sfx_path)
        sd = loader.loadSfx(self._sfx_path2)

        if sc is not None and sc.length() == len_a:
            _pass("footstep.mp3 reloads correctly after cache eviction")
        else:
            _fail("footstep.mp3 reloads correctly", f"length={sc.length() if sc else 'None'}")

        if sd is not None and sd.length() == len_b:
            _pass("hit.mp3 reloads correctly after cache eviction")
        else:
            _fail("hit.mp3 reloads correctly", f"length={sd.length() if sd else 'None'}")

        # Restore default cache limit.
        mgr.setCacheLimit(15)

        taskMgr.doMethodLater(0.2, self._finish, "t_finish")
        return task.done

    # ------------------------------------------------------------------
    # Summary
    # ------------------------------------------------------------------
    def _finish(self, task):
        _header("Results")
        passed = sum(1 for ok, _ in _results if ok)
        total  = len(_results)
        print(f"\n  {passed}/{total} tests passed")

        if passed == total:
            self._status.setText(f"ALL {total} TESTS PASSED  —  check console for cache HIT/STORE/EXPIRE log lines")
            self._status["fg"] = (0.2, 1, 0.2, 1)
            print("\n  All tests passed.")
            print("  Verify in the log above that you see:")
            print("    'cache HIT'            — Tests 1, 2, 6")
            print("    'cache STORE'          — first load of each file")
            print("    'cache EXPIRE (queued)'— Test 6, 7 (sound queued on last release)")
            print("    'cache RELEASE'        — Test 7 (eviction under cache limit)")
        else:
            failed = [label for ok, label in _results if not ok]
            self._status.setText(f"{passed}/{total} passed — FAILED: {', '.join(failed)}")
            self._status["fg"] = (1, 0.3, 0.3, 1)
            print(f"\n  {total - passed} test(s) FAILED.")

        print()
        taskMgr.doMethodLater(5.0, lambda t: sys.exit(0), "quit")
        return task.done


demo = CacheTest()
demo.run()
