#ifndef CHORD_ANALYZER_H
#define CHORD_ANALYZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "config.h"

struct ChordAnalysisResult {
    bool recognized;          // True if the chord pattern matches a known definition
    uint8_t notesPressed;     // Number of active notes currently held/sustained
    int8_t lowestNote;        // MIDI note number of lowest active note (-1 if none)
    char chordName[64];       // Formatted string representation of the chord
};

class ChordAnalyzer {
private:
    uint8_t noteArray[128];   // Note states: 0 = off, 1 = sustained, 2 = pressed
    bool sustainPressed;      // True if MIDI CC 64 sustain pedal is held
    bool changed;             // True if note state changed since last analysis
    ChordAnalysisResult cachedResult;

public:
    ChordAnalyzer();

    // Reset all notes and sustain pedal state
    void reset();

    // MIDI Event Processors
    void noteOn(uint8_t note, uint8_t velocity);
    void noteOff(uint8_t note);
    void sustainControl(uint8_t value);

    // Get current analysis result
    const ChordAnalysisResult& analyze();

    // Query if note state changed
    bool hasChanged() const { return changed; }
};

#endif // CHORD_ANALYZER_H
