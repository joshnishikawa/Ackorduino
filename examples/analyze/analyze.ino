/*
 * Ackorduino MIDI Chord Analyzer Example
 *
 * Demonstrates real-time chord detection on:
 *   1. Hardware Serial MIDI (or USB MIDI on supported boards)
 *   2. SSD1306 OLED Display (I2C)
 *   3. Serial Monitor (115200 baud)
 *
 * For wiring diagrams, board-specific options, and library dependencies,
 * see README.md in the library root.
 */

#include <MIDI.h>
#include "ChordAnalyzer.h"
#include "ChordDisplay.h"

// Core chord analyzer instance
static ChordAnalyzer analyzer;

// OLED Display Configuration:
// - Default (Uno, Nano, Mega, ESP32, ESP8266, RP2040, etc.): standard Wire bus
// - Teensy 3.6 / specific pin configuration:
#if defined(TEENSYDUINO) && defined(Wire2)
// Example: Teensy 3.6 using Wire2 on SDA2=Pin 4, SCL2=Pin 3
static ChordDisplay chordDisplay(Wire2, 0x3C, 4, 3);
#elif defined(ESP32)
// Example for ESP32 with custom I2C pins (e.g. SDA=21, SCL=22)
static ChordDisplay chordDisplay(Wire, 0x3C, 21, 22);
#else
// Default: Standard Wire bus and default hardware I2C pins for your board
static ChordDisplay chordDisplay;
#endif

// Hardware Serial MIDI Instance (Serial1 or Serial on AVR)
#if defined(HAVE_HWSERIAL1) || defined(ARDUINO_ARCH_SAMD) || defined(ESP32) || defined(TEENSYDUINO)
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_PORT);
#else
MIDI_CREATE_DEFAULT_INSTANCE();
#endif

// Callback: MIDI Note On
void OnNoteOn(byte channel, byte note, byte velocity) {
    analyzer.noteOn(note, velocity);
}

// Callback: MIDI Note Off
void OnNoteOff(byte channel, byte note, byte velocity) {
    analyzer.noteOff(note);
}

// Callback: MIDI Control Change (Sustain pedal CC 64)
void OnControlChange(byte channel, byte control, byte value) {
    if (control == 64) {
        analyzer.sustainControl(value);
    }
}

void setup() {
    // 1. Initialize USB Serial for debugging output
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000); // Give Serial monitor time to connect

    // 2. Initialize OLED Display
    chordDisplay.begin();

    // 3. Initialize Hardware Serial MIDI
#if defined(HAVE_HWSERIAL1) || defined(ARDUINO_ARCH_SAMD) || defined(ESP32) || defined(TEENSYDUINO)
    MIDI_PORT.begin(MIDI_CHANNEL_OMNI);
    MIDI_PORT.setHandleNoteOn(OnNoteOn);
    MIDI_PORT.setHandleNoteOff(OnNoteOff);
    MIDI_PORT.setHandleControlChange(OnControlChange);
    MIDI_PORT.turnThruOff();
#else
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.setHandleNoteOn(OnNoteOn);
    MIDI.setHandleNoteOff(OnNoteOff);
    MIDI.setHandleControlChange(OnControlChange);
    MIDI.turnThruOff();
#endif

    // 4. Register USB MIDI handlers (if supported by board, e.g., Teensy USB MIDI)
#if defined(USB_MIDI) || defined(USB_EVERYTHING) || defined(USB_MIDI_SERIAL) || defined(USB_MIDI4_SERIAL) || defined(USB_MIDI16_SERIAL)
    usbMIDI.setHandleNoteOn(OnNoteOn);
    usbMIDI.setHandleNoteOff(OnNoteOff);
    usbMIDI.setHandleControlChange(OnControlChange);
#endif

    Serial.println(F("=========================================="));
    Serial.println(F("Ackorduino: Listening for MIDI input"));
    Serial.println(F("=========================================="));
}

void loop() {
    // 1. Read Hardware Serial MIDI
#if defined(HAVE_HWSERIAL1) || defined(ARDUINO_ARCH_SAMD) || defined(ESP32) || defined(TEENSYDUINO)
    MIDI_PORT.read();
#else
    MIDI.read();
#endif

    // 2. Read USB Device MIDI (if enabled)
#if defined(USB_MIDI) || defined(USB_EVERYTHING) || defined(USB_MIDI_SERIAL) || defined(USB_MIDI4_SERIAL) || defined(USB_MIDI16_SERIAL)
    usbMIDI.read();
#endif

    // 3. Update OLED and Serial when chord/note state changes
    if (analyzer.hasChanged()) {
        const ChordAnalysisResult& result = analyzer.analyze();

        // Update Serial Monitor
        if (result.notesPressed == 0) {
            Serial.println(F("[Chord]: (none)"));
        } else {
            Serial.print(F("[Chord]: "));
            Serial.print(result.chordName);
            Serial.print(F(" -> "));
            Serial.print(result.chordDescription);
            if (!result.recognized) {
                Serial.print(F(" (Unrecognized)"));
            }
            Serial.print(F("  [Notes: "));
            Serial.print(result.notesPressed);
            Serial.println(F("]"));
        }

        // Update OLED Display
        chordDisplay.update(result);
    }
}
