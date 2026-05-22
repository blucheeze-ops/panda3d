"""Contains the Studio3DAudioManager class for FMOD Studio 3D audio."""

__all__ = ['Studio3DAudioManager']

from panda3d.core import Vec3, VBase3, WeakNodePath, ClockObject
from direct.task.TaskManagerGlobal import Task, taskMgr


class Studio3DAudioManager:
    """
    Manages 3D spatialization for FMOD Studio events, analogous to
    Audio3DManager but designed for the StudioAudioManager/StudioAudioEvent
    interface.

    Attach events to NodePaths and this manager will automatically update
    their 3D attributes (position, velocity) each frame, as well as the
    listener position/orientation.
    """

    def __init__(self, studio_manager, listener_target=None, root=None,
                 task_priority=51):
        """
        Args:
            studio_manager: A StudioAudioManager instance.
            listener_target: NodePath the listener follows (typically base.camera).
            root: The coordinate space root (defaults to base.render).
            task_priority: Priority for the per-frame update task.
        """
        self.studio_manager = studio_manager
        self.listener_target = listener_target

        if root is None:
            self.root = base.render
        else:
            self.root = root

        self.event_dict = {}
        self.vel_dict = {}
        self.listener_vel = VBase3(0, 0, 0)

        taskMgr.add(self.update, "Studio3DAudioManager-updateTask",
                    task_priority)

    def load_event(self, event_path):
        """
        Create a Studio event instance from an event path.

        Args:
            event_path: FMOD Studio event path, e.g. "event:/sfx/footstep".

        Returns:
            A StudioAudioEvent instance, or None on failure.
        """
        if event_path:
            return self.studio_manager.get_event(event_path)
        return None

    def attach_event_to_object(self, event, object):
        """
        Attach an event to a NodePath so its 3D position is updated each frame.
        If the event is already attached to another object, it is detached first.

        Args:
            event: A StudioAudioEvent instance.
            object: A NodePath to track.
        """
        for known_object in list(self.event_dict.keys()):
            if event in self.event_dict[known_object]:
                self.event_dict[known_object].remove(event)
                if len(self.event_dict[known_object]) == 0:
                    del self.event_dict[known_object]

        key = WeakNodePath(object)
        if key not in self.event_dict:
            self.event_dict[key] = []

        self.event_dict[key].append(event)

    def detach_event(self, event):
        """
        Stop tracking the event's 3D position.

        Args:
            event: A StudioAudioEvent instance.

        Returns:
            True if the event was found and detached, False otherwise.
        """
        for known_object in list(self.event_dict.keys()):
            if event in self.event_dict[known_object]:
                self.event_dict[known_object].remove(event)
                if len(self.event_dict[known_object]) == 0:
                    del self.event_dict[known_object]
                return True
        return False

    def get_events_on_object(self, object):
        """
        Returns a list of events attached to the given NodePath.
        """
        key = WeakNodePath(object)
        if key not in self.event_dict:
            return []
        return list(self.event_dict[key])

    def set_event_velocity(self, event, velocity):
        """
        Set a manual velocity vector for an event (units/sec), used for
        Doppler calculations. Relative to the root.
        """
        if isinstance(velocity, tuple) and len(velocity) == 3:
            velocity = VBase3(*velocity)
        if not isinstance(velocity, VBase3):
            raise TypeError("Invalid argument, expected VBase3 or 3-tuple")
        self.vel_dict[event] = velocity

    def set_event_velocity_auto(self, event):
        """
        Compute velocity automatically from the attached object's position
        delta each frame.
        """
        self.vel_dict[event] = None

    def get_event_velocity(self, event):
        """
        Get the current velocity of an event.
        """
        if event in self.vel_dict:
            vel = self.vel_dict[event]
            if vel is not None:
                return vel

            for known_object in list(self.event_dict.keys()):
                if event in self.event_dict[known_object]:
                    node_path = known_object.getNodePath()
                    if not node_path:
                        del self.event_dict[known_object]
                        continue
                    clock = ClockObject.getGlobalClock()
                    dt = clock.getDt()
                    if dt > 0:
                        return node_path.getPosDelta(self.root) / dt
                    return VBase3(0, 0, 0)

        return VBase3(0, 0, 0)

    def set_listener_velocity(self, velocity):
        """
        Set a manual velocity vector for the listener (units/sec).
        """
        if isinstance(velocity, tuple) and len(velocity) == 3:
            velocity = VBase3(*velocity)
        if not isinstance(velocity, VBase3):
            raise TypeError("Invalid argument, expected VBase3 or 3-tuple")
        self.listener_vel = velocity

    def set_listener_velocity_auto(self):
        """
        Compute listener velocity automatically from position delta each frame.
        """
        self.listener_vel = None

    def get_listener_velocity(self):
        """
        Get the current velocity of the listener.
        """
        if self.listener_vel is not None:
            return self.listener_vel
        elif self.listener_target is not None:
            clock = ClockObject.getGlobalClock()
            dt = clock.getDt()
            if dt > 0:
                return self.listener_target.getPosDelta(self.root) / dt
            return VBase3(0, 0, 0)
        else:
            return VBase3(0, 0, 0)

    def attach_listener(self, object):
        """
        Set the listener target. Sounds will be heard relative to this object.
        Typically base.camera.
        """
        self.listener_target = object

    def detach_listener(self):
        """
        Remove the listener target. Listener stays at the root origin.
        """
        self.listener_target = None

    def set_distance_factor(self, factor):
        """
        Set the distance unit scale for 3D audio. Default is 1.0 (meters).
        """
        self.studio_manager.set_3d_distance_factor(factor)

    def set_doppler_factor(self, factor):
        """
        Set the Doppler effect scale. Default is 1.0.
        Use >1.0 for exaggerated Doppler, <1.0 for diminished.
        """
        self.studio_manager.set_3d_doppler_factor(factor)

    def update(self, task=None):
        """
        Per-frame update: sync all attached event positions and the listener.
        Called automatically as a task.
        """
        # Update event 3D attributes
        for known_object, events in list(self.event_dict.items()):
            node_path = known_object.getNodePath()
            if not node_path:
                del self.event_dict[known_object]
                continue

            pos = node_path.getPos(self.root)

            for event in events:
                vel = self.get_event_velocity(event)
                event.set_3d_attributes(
                    pos[0], pos[1], pos[2],
                    vel[0], vel[1], vel[2])

        # Update listener
        if self.listener_target:
            pos = self.listener_target.getPos(self.root)
            forward = self.root.getRelativeVector(
                self.listener_target, Vec3.forward())
            up = self.root.getRelativeVector(
                self.listener_target, Vec3.up())
            vel = self.get_listener_velocity()
            self.studio_manager.set_listener_attributes(
                0,
                pos[0], pos[1], pos[2],
                vel[0], vel[1], vel[2],
                forward[0], forward[1], forward[2],
                up[0], up[1], up[2])
        else:
            self.studio_manager.set_listener_attributes(
                0,
                0, 0, 0,
                0, 0, 0,
                0, 1, 0,
                0, 0, 1)

        # Pump the Studio system
        self.studio_manager.update()

        return Task.cont

    def disable(self):
        """
        Stop the update task and detach everything.
        """
        taskMgr.remove("Studio3DAudioManager-updateTask")
        self.detach_listener()
        for known_object in list(self.event_dict.keys()):
            for event in self.event_dict[known_object]:
                self.detach_event(event)
        self.event_dict.clear()
        self.vel_dict.clear()
