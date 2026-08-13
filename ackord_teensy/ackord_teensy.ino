/*
 * Teensy 3.6 MIDI Chord Analyzer Sketch
 * Ported from Delphi 'ackord'
 *
 * Displays detected chords on:
 *   1. 128x32 I2C OLED Display (SSD1306) connected to Wire2: Pin 4 (SDA2) & Pin 3 (SCL2)
 *   2. USB Serial Monitor (115200 baud)
 *
 * MIDI Inputs Supported:
 *   1. Hardware Serial1 MIDI on Pin 0 (RX1) at standard 31250 baud.
 *   2. Native USB Device MIDI.
 *
 * Required Libraries (install via Arduino Library Manager):
 *   - MIDI Library (by FortySevenEffects)
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 *
 * Board Selection in Arduino IDE (Teensyduino):
 *   - Board: "Teensy 3.6"
 *   - USB Type: "Serial + MIDI" (or "MIDI")
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MIDI.h>
#include "ChordAnalyzer.h"
#include "config.h"

// OLED Display Parameters (128x32 px on Wire2: Pin 4 SDA2 & Pin 3 SCL2)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define OLED_I2C_ADDR 0x3C // Standard I2C address for SSD1306 128x32

// OLED Instance using Wire2 (Pin 4 SDA2, Pin 3 SCL2)
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire2, OLED_RESET);

// Hardware Serial MIDI Instance on Serial1 (Pin 0 = RX1, Pin 1 = TX1)
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI1);

// Core chord analyzer instance
static ChordAnalyzer analyzer;

// Prototypes
void updateOLED(const ChordAnalysisResult& result);
void showSplashScreen();
void scanI2C();

// Callback: MIDI Note On (shared by Hardware Serial Pin 0 and USB MIDI)
void OnNoteOn(byte channel, byte note, byte velocity) {
    analyzer.noteOn(note, velocity);
}

// Callback: MIDI Note Off (shared by Hardware Serial Pin 0 and USB MIDI)
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

    Serial.println(F("=========================================="));
    Serial.println(F("   Teensy 3.6 MIDI Chord Analyzer         "));
    Serial.println(F("   Ported from Delphi 'ackord'            "));
    Serial.println(F("=========================================="));

    // Enable internal pullups for Wire2 pins just in case external pullups are missing
    pinMode(3, INPUT_PULLUP);
    pinMode(4, INPUT_PULLUP);

    // 2. Initialize Wire2 I2C: Pin 4 = SDA2, Pin 3 = SCL2
    Wire2.setSDA(4);
    Wire2.setSCL(3);
    Wire2.begin();

    // Run I2C bus scan to verify OLED hardware connection & address
    scanI2C();

    // 3. Initialize OLED Display
    bool displayOK = display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR);
    if (!displayOK) {
        // Try alternate 0x3D address if 0x3C failed
        displayOK = display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
    }

    if (displayOK) {
        Serial.println(F("[OLED]: Display initialized successfully!"));
        showSplashScreen();
    } else {
        Serial.println(F("[OLED]: ERROR - SSD1306 display.begin() failed!"));
        Serial.println(F("[OLED]: Check wiring (SDA->Pin 4, SCL->Pin 3, VCC->3.3V, GND->GND)"));
    }

    // 4. Initialize Hardware Serial MIDI on Pin 0 (RX1) @ 31250 baud
    MIDI1.begin(MIDI_CHANNEL_OMNI);
    MIDI1.setHandleNoteOn(OnNoteOn);
    MIDI1.setHandleNoteOff(OnNoteOff);
    MIDI1.setHandleControlChange(OnControlChange);
    MIDI1.turnThruOff();

    // 5. Register USB MIDI handlers if USB MIDI enabled in Tools menu
#if defined(USB_MIDI) || defined(USB_EVERYTHING) || defined(USB_MIDI_SERIAL) || defined(USB_MIDI4_SERIAL) || defined(USB_MIDI16_SERIAL)
    usbMIDI.setHandleNoteOn(OnNoteOn);
    usbMIDI.setHandleNoteOff(OnNoteOff);
    usbMIDI.setHandleControlChange(OnControlChange);
#endif

    Serial.println(F("=========================================="));
    Serial.println(F("Listening for MIDI input on:"));
    Serial.println(F("  - Hardware Serial1 (Pin 0 RX1)"));
    Serial.println(F("  - USB Device MIDI"));
    Serial.println(F("=========================================="));
}

void loop() {
    // 1. Read Hardware Serial MIDI (Pin 0 / RX1)
    MIDI1.read();

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
            if (!result.recognized) {
                Serial.print(F(" (Unrecognized)"));
            }
            Serial.print(F("  [Notes: "));
            Serial.print(result.notesPressed);
            Serial.println(F("]"));
        }

        // Update 128x32 OLED Display
        updateOLED(result);
    }
}

// Shows diagnostic splash screen on startup
void showSplashScreen() {
    display.clearDisplay();
    
    // Outer double border
    display.drawRect(0, 0, 128, 32, SSD1306_WHITE);
    display.drawRect(2, 2, 124, 28, SSD1306_WHITE);

    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(28, 8);
    display.print(F("ACKORD"));

    display.display();

    // Brief screen flash (invert toggle) to visually confirm OLED hardware reaction
    delay(200);
    display.invertDisplay(true);
    delay(250);
    display.invertDisplay(false);
    delay(1000); // Hold splash screen for 1 second

    // Show ready screen
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(28, 12);
    display.print(F("Play a chord"));
    display.display();
}

// Scans Wire2 (Pins 3 & 4) for I2C devices and prints diagnostic report to Serial
void scanI2C() {
    Serial.println(F("[I2C Scan]: Scanning Wire2 (Pin 4 SDA2, Pin 3 SCL2)..."));
    byte count = 0;
    for (byte address = 1; address < 127; address++) {
        Wire2.beginTransmission(address);
        byte error = Wire2.endTransmission();

        if (error == 0) {
            Serial.print(F("  -> Found I2C device at address 0x"));
            if (address < 16) Serial.print(F("0"));
            Serial.print(address, HEX);
            Serial.println(F(" !"));
            count++;
        }
    }
    if (count == 0) {
        Serial.println(F("  -> NO I2C devices found on Wire2 (Pins 3 & 4)!"));
        Serial.println(F("     Check connections: SDA -> Pin 4, SCL -> Pin 3, VCC -> 3.3V, GND -> GND"));
    } else {
        Serial.print(F("[I2C Scan]: Scan complete. Total devices found: "));
        Serial.println(count);
    }
}

// Renders the chord name centered on 128x32 OLED display
void updateOLED(const ChordAnalysisResult& result) {
    display.clearDisplay();

    if (result.notesPressed == 0) {
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(28, 12);
        display.print(F("Play a chord"));
    } else {
        uint8_t len = strlen(result.chordName);
        uint8_t textSize = (len > 10) ? 1 : 2;

        display.setTextSize(textSize);
        display.setTextColor(SSD1306_WHITE);

        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(result.chordName, 0, 0, &x1, &y1, &w, &h);

        int16_t cursorX = (SCREEN_WIDTH - w) / 2;
        if (cursorX < 0) cursorX = 0;
        int16_t cursorY = (SCREEN_HEIGHT - h) / 2;
        if (cursorY < 0) cursorY = 0;

        display.setCursor(cursorX, cursorY);
        display.print(result.chordName);

        // Show dot indicator in top right if chord is unrecognized
        if (!result.recognized) {
            display.fillCircle(124, 4, 2, SSD1306_WHITE);
        }
    }

    display.display();
}
