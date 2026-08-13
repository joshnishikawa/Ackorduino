#include "ChordAnalyzer.h"
#include "ChordDefinitions.h"
#include <stdio.h>
#include <string.h>

ChordAnalyzer::ChordAnalyzer() {
    reset();
}

void ChordAnalyzer::reset() {
    memset(noteArray, 0, sizeof(noteArray));
    sustainPressed = false;
    changed = true;
    cachedResult.recognized = false;
    cachedResult.notesPressed = 0;
    cachedResult.lowestNote = -1;
    cachedResult.chordName[0] = '\0';
}

void ChordAnalyzer::noteOn(uint8_t note, uint8_t velocity) {
    if (note >= 128) return;
    if (velocity == 0) {
        noteOff(note);
        return;
    }
    if (noteArray[note] == 0) {
        changed = true;
    }
    noteArray[note] = 2;
}

void ChordAnalyzer::noteOff(uint8_t note) {
    if (note >= 128) return;
    uint8_t newState = sustainPressed ? 1 : 0;
    if (noteArray[note] != newState) {
        changed = true;
        noteArray[note] = newState;
    }
}

void ChordAnalyzer::sustainControl(uint8_t value) {
    bool newSustain = (value >= 64);
    if (sustainPressed && !newSustain) {
        // Sustain pedal released: clear notes held only by sustain (state 1)
        for (int i = 0; i < 128; i++) {
            if (noteArray[i] == 1) {
                noteArray[i] = 0;
                changed = true;
            }
        }
    }
    sustainPressed = newSustain;
}

const ChordAnalysisResult& ChordAnalyzer::analyze() {
    if (!changed) {
        return cachedResult;
    }
    changed = false;

    // 1. Count active notes and locate lowest note
    uint8_t activeCount = 0;
    int8_t lowest = -1;

    for (int i = 127; i >= 0; i--) {
        if (noteArray[i] > 0) {
            lowest = (int8_t)i;
            activeCount++;
        }
    }

    cachedResult.notesPressed = activeCount;
    cachedResult.lowestNote = lowest;

    // 2. Handle 0 notes
    if (activeCount == 0) {
        cachedResult.recognized = false;
        cachedResult.chordName[0] = '\0';
        return cachedResult;
    }

    uint8_t lowestNoteClass = (uint8_t)lowest % 12;

    // 3. Handle 1 note
    if (activeCount == 1) {
        cachedResult.recognized = true;
        snprintf(cachedResult.chordName, sizeof(cachedResult.chordName), "%s", NOTE_NAMES[lowestNoteClass]);
        return cachedResult;
    }

    // 4. Construct 12-bit mask of semitones relative to lowestNote
    uint16_t mask = 0;
    for (int i = 0; i < 128; i++) {
        if (noteArray[i] > 0) {
            uint8_t rel = (uint8_t)(i - lowest) % 12;
            mask |= (1 << rel);
        }
    }

    // 5. Look up in CHORD_DEFINITIONS array
    for (uint16_t i = 0; i < NUM_CHORD_DEFINITIONS; i++) {
        uint16_t defMask;
        uint8_t baseRelation;
        const char* namePtr;

#if defined(ARDUINO) && defined(pgm_read_word)
        defMask = pgm_read_word(&(CHORD_DEFINITIONS[i].mask));
        baseRelation = pgm_read_byte(&(CHORD_DEFINITIONS[i].baseRelation));
        namePtr = (const char*)pgm_read_ptr(&(CHORD_DEFINITIONS[i].name));
#else
        defMask = CHORD_DEFINITIONS[i].mask;
        baseRelation = CHORD_DEFINITIONS[i].baseRelation;
        namePtr = CHORD_DEFINITIONS[i].name;
#endif

        if (defMask == mask) {
            cachedResult.recognized = true;
            uint8_t rootIdx = (lowestNoteClass - baseRelation + 12) % 12;
            if (baseRelation == 0) {
                snprintf(cachedResult.chordName, sizeof(cachedResult.chordName), "%s%s",
                         NOTE_NAMES[rootIdx], namePtr);
            } else {
                snprintf(cachedResult.chordName, sizeof(cachedResult.chordName), "%s%s/%s",
                         NOTE_NAMES[rootIdx], namePtr, NOTE_NAMES[lowestNoteClass]);
            }
            return cachedResult;
        }
    }

    // 6. Unknown chord - list unique notes in pitch order starting from lowest note
    cachedResult.recognized = false;
    int pos = snprintf(cachedResult.chordName, sizeof(cachedResult.chordName), "%s", NOTE_NAMES[lowestNoteClass]);

    for (uint8_t step = 1; step < 12; step++) {
        if (mask & (1 << step)) {
            uint8_t noteIdx = (lowestNoteClass + step) % 12;
            if (pos < (int)sizeof(cachedResult.chordName) - 1) {
                pos += snprintf(cachedResult.chordName + pos, sizeof(cachedResult.chordName) - pos, ", %s", NOTE_NAMES[noteIdx]);
            }
        }
    }

    return cachedResult;
}
