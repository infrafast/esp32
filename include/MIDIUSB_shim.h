// utiliser la lib de lathoub (Arduino-USBMIDI) sur ESP32-S3 en lui fournissant un shim qui émule MIDIUSB.h et redirige les appels vers TinyUSB du core Arduino-ESP32 3.x.
// L’idée : créer un fichier MIDIUSB.h local qui expose midiEventPacket_t, MidiUSB.read(), MidiUSB.sendMIDI(), MidiUSB.flush(), mais qui, sous le capot, utilise l’API TinyUSB (tud_midi_packet_read/write). 
// La lib de lathoub reste inchangée.
// (La lib officielle précise qu’elle dépend de MIDIUSB et propose TinyUSB comme alternative pour ESP32
// include/MIDIUSB.h  (shim pour ESP32-S2/S3 + Arduino-ESP32 3.x)
// Emule l'API Arduino MIDIUSB en s'appuyant sur TinyUSB du core.
// Crée include/MIDIUSB.h (ou src/MIDIUSB.h) dans ton projet pour qu’il masque la lib MIDIUSB officielle.
// Ce shim mappe l’API MIDIUSB vers TinyUSB (paquets de 4 octets)

#pragma once
#include <stdint.h>

// TinyUSB MIDI (paquets 4 octets) — API dispo via le core arduino-esp32
extern "C" {
  #include "class/midi/midi_device.h" // déclare tud_midi_packet_read/write
}

// Même type que la lib MIDIUSB officielle
typedef struct {
  uint8_t header;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3;
} midiEventPacket_t;

// Objet global compatible: MidiUSB.read()/sendMIDI()/flush()
class _MidiUSBShim {
public:
  // Lit un paquet USB-MIDI (4 octets). header==0 si rien.
  midiEventPacket_t read() {
    midiEventPacket_t p{0,0,0,0};
    uint8_t raw[4];
    if (tud_midi_packet_read(raw)) { // TinyUSB : lit un paquet si dispo
      p.header = raw[0]; p.byte1 = raw[1]; p.byte2 = raw[2]; p.byte3 = raw[3];
    }
    return p;
  }

  // Envoie un paquet USB-MIDI (4 octets).
  void sendMIDI(const midiEventPacket_t &ev) {
    uint8_t raw[4] = {ev.header, ev.byte1, ev.byte2, ev.byte3};
    (void) tud_midi_packet_write(raw); // TinyUSB : écrit immédiatement
  }

  // TinyUSB envoie déjà au fil de l'eau → no-op
  void flush() {}
};

static _MidiUSBShim MidiUSB;
