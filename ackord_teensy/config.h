#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// Note names in 12-tone chromatic scale starting from C (index 0)
static const char* const NOTE_NAMES[12] = {
    "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
};

// Default serial baud rate for debugging/output
#define SERIAL_BAUD_RATE 115200

// MIDI settings
#define MIDI_CHANNEL_OMNI 0 // Listen to all channels

#endif // CONFIG_H
