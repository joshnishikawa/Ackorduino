#include "ChordDisplay.h"
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// --- PROGMEM BITMAPS FOR MUSICAL GLYPHS ---

// 32px Musical Flat Bitmap (16x32 px) - shifted down 3 rows and padded 2px on left
static const uint8_t FLAT_BITMAP_32[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 3 blank rows at top to prevent bezel cutoff
  0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x00,
  0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x3C, 0x0C, 0x7E, 0x0C, 0xCF,
  0x0D, 0x87, 0x0F, 0x07, 0x0E, 0x07, 0x0C, 0x06, 0x0C, 0x0C, 0x0C, 0x18, 0x0C, 0x30, 0x0C, 0x60,
  0x0C, 0xC0, 0x0D, 0x80, 0x0F, 0x00, 0x0E, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 16px Musical Flat Bitmap (8x16 px)
static const uint8_t FLAT_BITMAP_16[] PROGMEM = {
  0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x4C, 0x56, 0x66, 0x44, 0x48, 0x50, 0x60, 0x40, 0x00, 0x00
};

// 32px Musical Sharp Bitmap (16x32 px)
static const uint8_t SHARP_BITMAP_32[] PROGMEM = {
  0x00, 0x30, 0x00, 0x30, 0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x34, 0x0C, 0x3C,
  0x0C, 0xFC, 0x0F, 0xFC, 0x3F, 0xFC, 0x3F, 0xF0, 0x3F, 0x30, 0x3C, 0x30, 0x2C, 0x30, 0x0C, 0x30,
  0x0C, 0x30, 0x0C, 0x34, 0x0C, 0x3C, 0x0C, 0xFC, 0x0F, 0xFC, 0x3F, 0xFC, 0x3F, 0xF0, 0x3F, 0x30,
  0x3C, 0x30, 0x2C, 0x30, 0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x00, 0x0C, 0x00
};

// 16px Musical Sharp Bitmap (8x16 px)
static const uint8_t SHARP_BITMAP_16[] PROGMEM = {
  0x04, 0x24, 0x24, 0x26, 0x3E, 0x7C, 0x64, 0x24, 0x26, 0x2E, 0x7E, 0x74, 0x24, 0x24, 0x24, 0x20
};

// 16px Delta Triangle Bitmap for Major 7th (12x16 px)
static const uint8_t TRIANGLE_BITMAP_16[] PROGMEM = {
  0x02, 0x00, 0x03, 0x00, 0x07, 0x00, 0x05, 0x00, 
  0x0D, 0x80, 0x09, 0x80, 0x08, 0x80, 0x18, 0xC0,
  0x10, 0xC0, 0x10, 0x40, 0x30, 0x60, 0x20, 0x60, 
  0x20, 0x60, 0x70, 0xF0, 0x7F, 0xF0, 0xFF, 0xF8
};

ChordDisplay::ChordDisplay(TwoWire& wire, uint8_t addr, int8_t sda, int8_t scl, uint8_t screenWidth, uint8_t screenHeight)
    : wireBus(&wire), i2cAddr(addr), sdaPin(sda), sclPin(scl), width(screenWidth), height(screenHeight),
      display(screenWidth, screenHeight, &wire, CHORD_DISPLAY_RESET_PIN) {
}

bool ChordDisplay::begin() {
    // If specific SDA and SCL pins were provided, configure them if supported by platform
    if (sdaPin >= 0 && sclPin >= 0) {
        pinMode(sdaPin, INPUT_PULLUP);
        pinMode(sclPin, INPUT_PULLUP);

#if defined(TEENSYDUINO) || defined(ARDUINO_ARCH_RP2040)
        wireBus->setSDA(sdaPin);
        wireBus->setSCL(sclPin);
        wireBus->begin();
#elif defined(ESP32) || defined(ESP8266)
        wireBus->begin(sdaPin, sclPin);
#else
        wireBus->begin();
#endif
    } else {
        wireBus->begin();
    }

    // Run diagnostic bus scan
    scanI2C();

    // Initialize OLED Display
    bool displayOK = display.begin(SSD1306_SWITCHCAPVCC, i2cAddr);
    if (!displayOK) {
        // Try alternate standard address (0x3D <-> 0x3C)
        uint8_t altAddr = (i2cAddr == 0x3C) ? 0x3D : 0x3C;
        displayOK = display.begin(SSD1306_SWITCHCAPVCC, altAddr);
        if (displayOK) {
            i2cAddr = altAddr;
        }
    }

    if (displayOK) {
        Serial.println(F("[OLED]: Display initialized successfully!"));
        showSplash();
    } else {
        Serial.println(F("[OLED]: ERROR - SSD1306 display.begin() failed!"));
        Serial.println(F("[OLED]: Check I2C wiring (SDA, SCL, VCC, GND) and address (0x3C/0x3D)"));
    }

    return displayOK;
}

void ChordDisplay::showSplash() {
    display.clearDisplay();
    display.setFont(&FreeSans12pt7b);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 18);
    display.print(F("Ackorduino"));

    display.setFont(&FreeSans9pt7b);
    display.setCursor(0, 31);
    display.print(F("Chord Analyzer"));
    display.display();
    delay(1500);

    display.clearDisplay();
    display.setFont(&FreeSans12pt7b);
    display.setCursor(0, 22);
    display.print(F("Listening"));
    display.display();
    display.setFont(NULL); // Reset to default font
}

void ChordDisplay::scanI2C() {
    Serial.println(F("[I2C Scan]: Scanning configured I2C bus..."));
    byte count = 0;
    for (byte address = 1; address < 127; address++) {
        wireBus->beginTransmission(address);
        byte error = wireBus->endTransmission();

        if (error == 0) {
            Serial.print(F("  -> Found I2C device at address 0x"));
            if (address < 16) Serial.print(F("0"));
            Serial.print(address, HEX);
            Serial.println(F(" !"));
            count++;
        }
    }
    if (count == 0) {
        Serial.println(F("  -> NO I2C devices found on this bus!"));
        Serial.println(F("     Check connections: SDA, SCL, VCC, GND"));
    } else {
        Serial.print(F("[I2C Scan]: Scan complete. Total devices found: "));
        Serial.println(count);
    }
}

int16_t ChordDisplay::printFormattedText(const char* str, int16_t startX, int16_t startY, uint8_t size, bool convertAccidentals) {
    int16_t curX = startX;
    int16_t curY = startY;

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    for (size_t i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        if (convertAccidentals && c == '^') {
            if (curX + 13 > width) {
                curX = 0;
                curY += 16;
            }
            int16_t bmpY = (curY >= 13) ? (curY - 13) : 0;
            display.drawBitmap(curX, bmpY, TRIANGLE_BITMAP_16, 12, 16, SSD1306_WHITE);
            curX += 13;
        } else if (convertAccidentals && c == 'b') {
            if (size >= 4) {
                // Size 4 (FreeSans18pt7b baseline at y=28) -> 32px root flat bitmap
                if (curX + 16 > width) {
                    curX = 0;
                    curY = 28;
                }
                display.drawBitmap(curX, 0, FLAT_BITMAP_32, 16, 32, SSD1306_WHITE);
                curX += 14;
            } else {
                // 16px flat bitmap
                if (curX + 10 > width) {
                    curX = 0;
                    curY += 16;
                }
                int16_t bmpY = (curY >= 12) ? (curY - 12) : 0;
                display.drawBitmap(curX, bmpY, FLAT_BITMAP_16, 8, 16, SSD1306_WHITE);
                curX += 10;
            }
        } else if (convertAccidentals && c == '#') {
            if (size >= 4) {
                // Size 4 (FreeSans18pt7b baseline at y=28) -> 32px root sharp bitmap
                if (curX + 16 > width) {
                    curX = 0;
                    curY = 28;
                }
                display.drawBitmap(curX, 0, SHARP_BITMAP_32, 16, 32, SSD1306_WHITE);
                curX += 16;
            } else {
                // 16px sharp bitmap
                if (curX + 10 > width) {
                    curX = 0;
                    curY += 16;
                }
                int16_t bmpY = (curY >= 12) ? (curY - 12) : 0;
                display.drawBitmap(curX, bmpY, SHARP_BITMAP_16, 8, 16, SSD1306_WHITE);
                curX += 10;
            }
        } else {
            // Measure proportional character bounds with current FreeSans font
            char tempBuf[2] = { c, '\0' };
            int16_t x1, y1;
            uint16_t w, h;
            display.getTextBounds(tempBuf, curX, curY, &x1, &y1, &w, &h);

            if (curX + w > width && curX > 0) {
                curX = 0;
                curY += (size >= 3) ? 16 : 14;
            }
            display.setCursor(curX, curY);
            display.write(c);
            curX += (w > 0) ? (w + 1) : (6 * size);
        }
    }
    return curX;
}

void ChordDisplay::update(const ChordAnalysisResult& result) {
    display.clearDisplay();

    if (result.notesPressed == 0) {
        // "Listening" prompt with FreeSans12pt
        display.setFont(&FreeSans12pt7b);
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(0, 22);
        display.print(F("Listening"));
    } else if (!result.recognized) {
        // Unrecognized raw notes cluster in FreeSans9pt
        display.setFont(&FreeSans9pt7b);
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        printFormattedText(result.chordName, 0, 13, 2, true);
    } else {
        // --- CHORD RENDERING WITH FREESANS ---
        char mainStr[32];  // Root Note + 'm'
        char superStr[32]; // Suffix extensions

        snprintf(mainStr, sizeof(mainStr), "%s", result.rootStr);
        superStr[0] = '\0';

        if (result.suffixStr[0] != '\0') {
            if (result.suffixStr[0] == 'm' && result.suffixStr[1] != 'a') {
                // Minor quality 'm' kept with root note
                strcat(mainStr, "m");
                const char* rest = result.suffixStr + 1;
                while (*rest == ' ') rest++; // Skip leading spaces
                snprintf(superStr, sizeof(superStr), "%s", rest);
            } else {
                snprintf(superStr, sizeof(superStr), "%s", result.suffixStr);
            }
        }

        uint8_t superLen = strlen(superStr);
        uint8_t bassLen = result.bassStr[0] ? (strlen(result.bassStr) + 1) : 0;

        // 1. Draw Root Note (+ 'm') in FreeSans18pt7b (baseline at y = 28, spans ~30px height!)
        display.setFont(&FreeSans18pt7b);
        display.setTextSize(1);
        int16_t nextX = printFormattedText(mainStr, 0, 28, 4, true);

        // 2. Draw Suffix/Extension:
        // Main extension in FreeSans9pt7b (aligned to top baseline y = 12)
        // Sub-modifiers ("no5", "no3", "add9", "add11", etc.) in default font size 1
        if (superLen > 0) {
            const char* modKeywords[] = {
                "no5", "no3", "add 9", "add 11", "add 13", "add9", "add11", "add13", "add#5", "alt"
            };
            const int numModKeywords = sizeof(modKeywords) / sizeof(modKeywords[0]);

            const char* matchPtr = NULL;
            for (int k = 0; k < numModKeywords; k++) {
                const char* p = strstr(superStr, modKeywords[k]);
                if (p != NULL) {
                    if (matchPtr == NULL || p < matchPtr) {
                        matchPtr = p;
                    }
                }
            }

            if (matchPtr != NULL) {
                char prefixPart[32];
                size_t prefixLen = matchPtr - superStr;
                if (prefixLen >= sizeof(prefixPart)) prefixLen = sizeof(prefixPart) - 1;
                strncpy(prefixPart, superStr, prefixLen);
                prefixPart[prefixLen] = '\0';

                while (prefixLen > 0 && prefixPart[prefixLen - 1] == ' ') {
                    prefixPart[--prefixLen] = '\0';
                }

                if (prefixLen > 0) {
                    display.setFont(&FreeSans9pt7b);
                    nextX = printFormattedText(prefixPart, nextX + 1, 13, 3, true);
                    nextX += 2;
                }

                // Render modifier in compact default font size 1 at top
                display.setFont(NULL);
                display.setTextSize(1);
                nextX = printFormattedText(matchPtr, nextX, 0, 1, true);
            } else {
                display.setFont(&FreeSans9pt7b);
                nextX = printFormattedText(superStr, nextX + 1, 13, 3, true);
            }
        }

        // 3. Draw Slash Bass Note in FreeSans9pt7b (aligned to bottom baseline y = 30)
        if (bassLen > 0) {
            display.setFont(&FreeSans9pt7b);
            display.setTextSize(1);
            display.setCursor(nextX, 30);
            display.write('/');
            nextX += 9;
            printFormattedText(result.bassStr, nextX, 30, 3, true);
        }
    }

    display.display();
}
