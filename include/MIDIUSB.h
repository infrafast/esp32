#pragma once

// Arduino-USBMIDI includes <MIDIUSB.h>. On ESP32-S2/S3 the project uses the
// TinyUSB-backed compatibility layer instead of the AVR MIDIUSB library.
#if defined(INTERVAL_USE_NATIVE_TINYUSB_MIDI)
  // macOS's case-insensitive filesystem makes <midiusb.h> resolve to this
  // uppercase compatibility header as well. Skip this project shim and find
  // ESP32TinyUSB's native MIDIusb declaration instead.
  #include_next <midiusb.h>
#elif defined(ARDUINO_ARCH_ESP32)
  #include "MIDIUSB_shim.h"
#else
  // Keep compatibility with platforms that provide their own MIDIUSB.h.
  #include_next <MIDIUSB.h>
#endif
