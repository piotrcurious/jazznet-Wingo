#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include "alsa_midi_client.h"

#define A0 0
#define A1 1
#define OUTPUT 1
#define HIGH 1
#define LOW 0
#define MIDI_CHANNEL_OMNI 0

// Mock Serial
class MockSerial {
public:
    void begin(int baud) {}
    void print(const char* s) { /* std::cerr << s; */ }
    void print(int i) { /* std::cerr << i; */ }
    void println(const char* s) { /* std::cerr << s << std::endl; */ }
    void println(int i) { /* std::cerr << i << std::endl; */ }
};

// Mock MIDI
class MockMIDI {
public:
    struct NoteEvent {
        int note;
        int velocity;
        int channel;
        bool on;
    };
    std::vector<NoteEvent> events;
    bool liveMode = false;
    ALSAMIDIClient* alsaClient = nullptr;

    void begin(int channel) {}

    void sendNoteOn(int note, int velocity, int channel) {
        events.push_back({note, velocity, channel, true});
        if (alsaClient) {
            alsaClient->sendNoteOn(channel - 1, note, velocity);
        }
    }

    void sendNoteOff(int note, int velocity, int channel) {
        events.push_back({note, velocity, channel, false});
        if (alsaClient) {
            alsaClient->sendNoteOff(channel - 1, note, velocity);
        }
    }
};

// Global functions
inline int analogRead(int pin) { return 0; }
inline void pinMode(int pin, int mode) {}
inline void digitalWrite(int pin, int val) {}
inline void delay(int ms) {
    // skip sleep in tests to avoid timeouts
    // usleep(ms * 1000);
}

inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
  if (in_max == in_min) return out_min;
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

template<class T, class L, class H>
inline T constrain(T mvg, L lo, H hi) {
  return (mvg < lo) ? lo : ((mvg > hi) ? hi : mvg);
}

inline long random(long min, long max) {
    static bool seeded = false;
    if (!seeded) {
        srand(42);
        seeded = true;
    }
    if (max == min) return min;
    return min + (rand() % (max - min));
}

inline long millis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

#endif
