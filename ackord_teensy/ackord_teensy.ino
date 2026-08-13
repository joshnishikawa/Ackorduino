/*
 * Teensy 3.6 MIDI Chord Analyzer Sketch
 * Ported from Delphi 'ackord'
 *
 * Custom Bitmap & 32px Typography Features:
 *   - PROGMEM Pixel-Perfect Musical Flat (♭) & Sharp (♯) Bitmaps
 *   - 32px Full-Height Main Letters (Root Note & 'm' Minor indicator at full 32px height)
 *   - 'm' is NOT superscripted (rendered at full 32px size alongside the root note)
 *   - Superscript Chord Extensions (7, maj7, 7♭9, 13♯11) positioned at top-right
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

// --- PROGMEM BITMAPS FOR MUSICAL GLYPHS ---

// 32px Musical Flat Bitmap (12x32 px)
static const uint8_t FLAT_BITMAP_32[] PROGMEM = {
0xE0, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0xE0, 0x00,
0xE0, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0xE3, 0x80, 0xE7, 0xC0, 0xEF, 0xE0,
0xF9, 0xF0, 0xF0, 0xF0, 0xE0, 0x70, 0xE0, 0x70, 0xE0, 0x70, 0xE0, 0x60, 0xE0, 0xC0, 0xE1, 0x80,
0xE3, 0x00, 0xE6, 0x00, 0xEC, 0x00, 0xF8, 0x00, 0xF0, 0x00, 0xE0, 0x00, 0xC0, 0x00, 0x80, 0x00
};

// 16px Musical Flat Bitmap (8x16 px)
static const uint8_t FLAT_BITMAP_16[] PROGMEM = {
0xC0, 0x00, 0xC0, 0x00, 0xC0, 0x00, 0xC0, 0x00, 0xC0, 0x00, 0xC0, 0x00, 0xC0, 0x00, 0xDC, 0x00,
0xEE, 0x00, 0xC6, 0x00, 0xC4, 0x00, 0xC8, 0x00, 0xD0, 0x00, 0xE0, 0x00, 0xC0, 0x00, 0x80, 0x00
};

// 32px Musical Sharp Bitmap (16x32 px)
static const uint8_t SHARP_BITMAP_32[] PROGMEM = {
    0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30,
    0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30, 0x7F, 0xFC,
    0xFF, 0xFE, 0xFE, 0xFF, 0xFC, 0xFE, 0x0C, 0x30,
    0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30,
    0x0C, 0x30, 0x0C, 0x30, 0x7F, 0xFC, 0xFF, 0xFE,
    0xFE, 0xFF, 0xFC, 0xFE, 0x0C, 0x30, 0x0C, 0x30,
    0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30,
    0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30
};

// 16px Musical Sharp Bitmap (8x16 px)
static const uint8_t SHARP_BITMAP_16[] PROGMEM = {
    0x24, 0x24, 0x24, 0xFE, 0xFE, 0x24, 0x24, 0x24,
    0xFE, 0xFE, 0x24, 0x24, 0x24, 0x24, 0x00, 0x00
};

// 16px Delta Triangle Bitmap for Major 7th (12x16 px)
static const uint8_t TRIANGLE_BITMAP_16[] PROGMEM = {
    0x02, 0x00, 0x03, 0x00, 0x07, 0x00, 0x05, 0x00, 0x0D, 0x80, 0x09, 0x80, 0x08, 0x80, 0x18, 0xC0,
0x10, 0xC0, 0x10, 0x40, 0x30, 0x60, 0x20, 0x60, 0x20, 0x60, 0x70, 0xF0, 0x7F, 0xF0, 0xFF, 0xF8
};

// Prototypes
void updateOLED(const ChordAnalysisResult& result);
void showSplashScreen();
void scanI2C();
int16_t printFormattedText(const char* str, int16_t startX, int16_t startY, uint8_t size, bool convertAccidentals);

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
            Serial.print(F(" -> "));
            Serial.print(result.chordDescription);
            if (!result.recognized) {
                Serial.print(F(" (Unrecognized)"));
            }
            Serial.print(F("  [Notes: "));
            Serial.print(result.notesPressed);
            Serial.println(F("]"));
        }

        // Update 128x32 OLED Display with Bitmap Musical Glyphs & 32px Typography
        updateOLED(result);
    }
}

// Renders formatted text, replacing '^' with Delta Triangle (Δ), 'b' with Flat (♭), and '#' with Sharp (♯)
int16_t printFormattedText(const char* str, int16_t startX, int16_t startY, uint8_t size, bool convertAccidentals) {
    int16_t curX = startX;
    uint8_t charW = 6 * size;

    display.setTextSize(size);
    display.setTextColor(SSD1306_WHITE);

    for (size_t i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        if (convertAccidentals && c == '^') {
            // Render bitmap Delta Triangle (Δ) for Major 7th
            display.drawBitmap(curX, startY, TRIANGLE_BITMAP_16, 12, 16, SSD1306_WHITE);
            curX += 11;
        } else if (convertAccidentals && c == 'b') {
            // Render bitmap Flat (♭)
            if (size >= 3) {
                // 32px Root Note Flat
                display.drawBitmap(curX, startY, FLAT_BITMAP_32, 12, 32, SSD1306_WHITE);
                curX += 13;
            } else {
                // Superscript / Extension Flat -> Uses FLAT_BITMAP_16 (12x16 px)
                display.drawBitmap(curX, startY, FLAT_BITMAP_16, 12, 16, SSD1306_WHITE);
                curX += 11;
            }
        } else if (convertAccidentals && c == '#') {
            // Render bitmap Sharp (♯)
            if (size >= 3) {
                display.drawBitmap(curX, startY, SHARP_BITMAP_32, 16, 32, SSD1306_WHITE);
                curX += 18;
            } else {
                display.drawBitmap(curX, startY, SHARP_BITMAP_16, 8, 16, SSD1306_WHITE);
                curX += 9;
            }
        } else {
            display.setCursor(curX, startY);
            display.write(c);
            curX += charW;
        }
    }
    return curX;
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

// Renders the chord with 32px full-height main letters, 'm' on baseline, and superscript extensions
void updateOLED(const ChordAnalysisResult& result) {
    display.clearDisplay();

    if (result.notesPressed == 0) {
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(28, 12);
        display.print(F("Play a chord"));
    } else if (!result.recognized) {
        // Unrecognized note cluster fallback
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(result.chordName, 0, 0, &x1, &y1, &w, &h);
        int16_t curX = (SCREEN_WIDTH - w) / 2;
        if (curX < 0) curX = 0;
        
        printFormattedText(result.chordName, curX, 12, 1, true);
        display.fillCircle(124, 4, 2, SSD1306_WHITE); // Dot indicator for unrecognized
    } else {
        // --- 32PX FULL-HEIGHT TYPOGRAPHY & SUPERSCRIPT RENDERER ---
        char mainStr[32];  // Root Note + 'm' (rendered at full 32px height)
        char superStr[32]; // Superscript extensions (7, maj7, 7♭9, 13♯11)

        // 1. Separate Root Note & 'm' from Superscript Extensions
        snprintf(mainStr, sizeof(mainStr), "%s", result.rootStr);
        superStr[0] = '\0';

        if (result.suffixStr[0] != '\0') {
            if (result.suffixStr[0] == 'm' && result.suffixStr[1] != 'a') {
                // Minor quality 'm' (e.g. 'm', 'm7', 'm9', 'm11', 'm6', 'm7b5', 'm add9')
                // Keep 'm' attached to mainStr so it renders at full 32px height alongside the root!
                strcat(mainStr, "m");
                const char* rest = result.suffixStr + 1;
                while (*rest == ' ') rest++; // Skip leading spaces
                snprintf(superStr, sizeof(superStr), "%s", rest);
            } else {
                // Suffix is maj7, 7, 7b9, 13#11, add9, sus4, etc.
                snprintf(superStr, sizeof(superStr), "%s", result.suffixStr);
            }
        }

        uint8_t mainLen = strlen(mainStr);
        uint8_t superLen = strlen(superStr);
        uint8_t bassLen = result.bassStr[0] ? (strlen(result.bassStr) + 1) : 0; // Includes '/'

        // Determine size for Main Letters (Root + 'm')
        // Default to TextSize = 4 (32px full screen height!)
        uint8_t mainSize = 4;
        uint8_t superSize = 2; // ALWAYS 16px height (TextSize 2) for consistent superscripts!

        uint16_t mainWidth = mainLen * (6 * mainSize);
        uint16_t superWidth = superLen * (6 * superSize);
        uint16_t bassWidth = bassLen * 12;

        uint16_t totalWidth = mainWidth + superWidth + bassWidth;

        // Auto-scale main letters if the chord is extremely wide
        if (totalWidth > 124) {
            mainSize = 3; // 24px height
            mainWidth = mainLen * (6 * mainSize);
            totalWidth = mainWidth + superWidth + bassWidth;
            if (totalWidth > 124) {
                mainSize = 2; // 16px height
                mainWidth = mainLen * (6 * mainSize);
                totalWidth = mainWidth + superWidth + bassWidth;
            }
        }

        // Center entire chord horizontally on 128px OLED screen
        int16_t startX = (SCREEN_WIDTH - totalWidth) / 2;
        if (startX < 0) startX = 0;

        int16_t mainY = (mainSize == 4) ? 0 : ((32 - (8 * mainSize)) / 2); // 32px height at y = 0
        int16_t superY = 0;  // Superscript aligned at top right!
        int16_t bassY = 16;  // Slash bass at bottom right

        // A. Draw Main Letters (Root Note + 'm' at 32px full height!)
        int16_t nextX = printFormattedText(mainStr, startX, mainY, mainSize, true);

        // B. Draw Superscript Extension at top-right (e.g. 7, maj7, 7♭9, 13♯11)
        if (superLen > 0) {
            nextX = printFormattedText(superStr, nextX, superY, superSize, true);
        }

        // C. Draw Slash Bass Note at bottom-right (e.g. /G, /E♭)
        if (bassLen > 0) {
            display.setTextSize(2);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(nextX, bassY);
            display.write('/');
            nextX += 12;
            printFormattedText(result.bassStr, nextX, bassY, 2, true);
        }
    }

    display.display();
}
