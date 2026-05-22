/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file studioAudioEvent.cxx
 * @author jovan
 * @date 2026-04-13
 */

#include "studioAudioEvent.h"

TypeHandle StudioAudioEvent::_type_handle;

StudioAudioEvent::
StudioAudioEvent() {
}

StudioAudioEvent::
~StudioAudioEvent() {
}

void StudioAudioEvent::
output(std::ostream &out) const {
  out << get_type();
}

void StudioAudioEvent::
write(std::ostream &out) const {
  out << (*this) << "\n";
}

std::ostream &
operator << (std::ostream &out, StudioAudioEvent::EventStatus status) {
  switch (status) {
  case StudioAudioEvent::ES_stopped:
    return out << "stopped";
  case StudioAudioEvent::ES_starting:
    return out << "starting";
  case StudioAudioEvent::ES_playing:
    return out << "playing";
  case StudioAudioEvent::ES_sustaining:
    return out << "sustaining";
  case StudioAudioEvent::ES_stopping:
    return out << "stopping";
  }
  return out << "**invalid EventStatus (" << (int)status << ")**";
}
