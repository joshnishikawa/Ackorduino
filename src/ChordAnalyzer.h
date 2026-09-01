#ifndef CHORD_ANALYZER_H
#define CHORD_ANALYZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
// Note names in 12-tone chromatic scale starting from C (index 0)
static const char* const NOTE_NAMES[12] = {
    "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
};

struct ChordAnalysisResult {
    bool recognized;           // True if the chord pattern matches a known definition
    uint8_t notesPressed;      // Number of active notes currently held/sustained
    int8_t lowestNote;         // MIDI note number of lowest active note (-1 if none)
    char rootStr[16];          // Root note string (e.g. "C", "Eb", "F#")
    char suffixStr[32];        // Superscript quality/extension string (e.g. "maj7", "m7", "7b5")
    char bassStr[16];          // Slash bass string (e.g. "E", "G", or empty if root in bass)
    char chordName[64];        // Full chord name string (e.g. "Cmaj7/E")
    char chordDescription[64]; // Detailed chord description (e.g. "C Major 7th (3rd in Bass)")
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
