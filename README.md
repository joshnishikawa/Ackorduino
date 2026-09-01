# Ackorduino

MIDI Chord Analyzer library for Arduino and Teensy, ported from David Henningsson's `ackord`.

Ackorduino inspects held and sustained MIDI notes in real-time, matching them against comprehensive musical chord definitions to identify roots, qualities, extensions/modifiers (e.g., `maj7`, `7♭9`, `13♯11`, `no5`), and slash-bass inversions.

---

## Features

- **Robust Chord Identification**: Recognizes triads, 7ths, 9ths, 11ths, 13ths, altered dominants, suspensions, omissions (`no3`, `no5`), and add notes.
- **Inversion & Slash Bass Detection**: Automatically determines root notes versus lowest bass notes for accurate slash-chord representation (e.g., `Cmaj7/E`).
- **Sustain Pedal Support**: Tracks MIDI CC 64 (Damper/Sustain) pedal state seamlessly with note-on/off events.
- **Custom Typography & Bitmaps (Display Example)**:
  - Custom PROGMEM pixel-perfect musical Flat (♭), Sharp (♯), and Major 7th Delta (Δ) bitmaps.
  - Proportional 32px typography for full-height root letters and non-superscripted minor `'m'`.
  - Top-aligned superscript chord extensions and bottom-aligned slash bass notes.

---

## Architecture & Board Compatibility

Ackorduino is compatible with all major Arduino architectures:
- **AVR** (Uno, Nano, Mega, Leonardo)
- **Teensy** (2.0, 3.2, 3.5, 3.6, 4.0, 4.1, MicroMod)
- **ESP32 & ESP8266**
- **RP2040** (Raspberry Pi Pico, Arduino Nano RP2040 Connect)
- **SAMD** (Zero, MKR series)
- **STM32**

---

## Hardware Setup

### 1. I2C OLED Display (SSD1306 128x32 / 128x64)
- **Default (Uno, Nano, Mega, ESP, RP2040, SAMD)**: Standard I2C pins (`SDA`, `SCL`), 3.3V/5V, GND.
- **Teensy 3.6 (Wire2 custom bus example)**:
  - **SDA**: Pin 4 (`SDA2`)
  - **SCL**: Pin 3 (`SCL2`)

### 2. MIDI Inputs Supported
- **Hardware Serial MIDI**: Standard 31250 baud MIDI input (`Serial1` on 32-bit boards / Teensy / Mega, or `Serial` on Uno).
- **USB MIDI**: Native plug-and-play USB MIDI device on supported boards (Teensy, Leonardo, Micro, SAMD, RP2040).

---

## Getting Started

### Required Libraries
Install the following via the Arduino Library Manager:
- [MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library) (by FortySevenEffects)
- [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)

### Board Selection in Arduino IDE
- **Teensyduino**: Set USB Type to `"Serial + MIDI"` (or `"MIDI"`).
- **Other Arduino Boards**: Set standard board type.

---

## Example Usage

See the provided [`examples/analyze/`](examples/analyze/) sketch for a full working example with both Hardware Serial and USB MIDI support as well as SSD1306 OLED display output.

```cpp
#include "ChordAnalyzer.h"

ChordAnalyzer analyzer;

void OnNoteOn(byte channel, byte note, byte velocity) {
    analyzer.noteOn(note, velocity);
}

void OnNoteOff(byte channel, byte note, byte velocity) {
    analyzer.noteOff(note);
}

void loop() {
    if (analyzer.hasChanged()) {
        const ChordAnalysisResult& result = analyzer.analyze();
        if (result.recognized) {
            Serial.printf("Chord: %s -> %s\n", result.chordName, result.chordDescription);
        }
    }
}
```

---

## License
MIT License / Open Source
