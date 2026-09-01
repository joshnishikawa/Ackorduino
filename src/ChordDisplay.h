#ifndef CHORD_DISPLAY_H
#define CHORD_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ChordAnalyzer.h"

// Default SSD1306 OLED dimensions
#define CHORD_DISPLAY_DEFAULT_WIDTH  128
#define CHORD_DISPLAY_DEFAULT_HEIGHT 32
#define CHORD_DISPLAY_DEFAULT_ADDR   0x3C
#define CHORD_DISPLAY_RESET_PIN      -1

class ChordDisplay {
private:
    TwoWire* wireBus;
    uint8_t i2cAddr;
    int8_t sdaPin;
    int8_t sclPin;
    uint8_t width;
    uint8_t height;
    Adafruit_SSD1306 display;

    int16_t printFormattedText(const char* str, int16_t startX, int16_t startY, uint8_t size, bool convertAccidentals);

public:
    // Constructor: configurable I2C bus, address, pins, and screen dimensions
    // Defaults to standard &Wire, 0x3C address, default board I2C pins, and 128x32 OLED
    ChordDisplay(TwoWire& wire = Wire, 
                 uint8_t addr = CHORD_DISPLAY_DEFAULT_ADDR, 
                 int8_t sda = -1, 
                 int8_t scl = -1,
                 uint8_t screenWidth = CHORD_DISPLAY_DEFAULT_WIDTH,
                 uint8_t screenHeight = CHORD_DISPLAY_DEFAULT_HEIGHT);

    // Initialize I2C bus, scan devices, initialize SSD1306, and show splash screen
    bool begin();

    // Show initial splash/listening screen
    void showSplash();

    // Render chord analysis result onto the OLED display
    void update(const ChordAnalysisResult& result);

    // Diagnostic scan of the configured I2C bus
    void scanI2C();

    // Direct access to underlying Adafruit_SSD1306 instance if needed
    Adafruit_SSD1306& getDriver() { return display; }
};

#endif // CHORD_DISPLAY_H
