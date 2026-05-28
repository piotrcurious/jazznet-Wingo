#ifndef JAZZ_LOGIC_H
#define JAZZ_LOGIC_H

// If we are in the Arduino environment, we use its types and MIDI instance.
// If we are in the test environment, we use the mocks.
#ifdef MOCK_TESTING
#include "tests/mock_arduino.h"
extern MockMIDI MIDI;
extern MockSerial Serial;
#else
#include <Arduino.h>
#include <MIDI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> MIDI;
#endif

// Mutex handling for thread safety
#ifdef MOCK_TESTING
#include <mutex>
extern std::mutex ensembleMutex;
#define ENSEMBLE_LOCK() ensembleMutex.lock()
#define ENSEMBLE_UNLOCK() ensembleMutex.unlock()
#else
extern SemaphoreHandle_t ensembleMutex;
#define ENSEMBLE_LOCK() xSemaphoreTake(ensembleMutex, portMAX_DELAY)
#define ENSEMBLE_UNLOCK() xSemaphoreGive(ensembleMutex)
#endif

// Constants
extern const int ERROR_THRESHOLD_1;
extern const int ERROR_THRESHOLD_2;
extern const int ERROR_THRESHOLD_3;
extern const int ERROR_THRESHOLD_4;
extern const int ERROR_THRESHOLD_5;
extern const int MAX_NOTES_PER_CHORD;
extern const int ADC_RESOLUTION;

// Chord Definitions
extern const int iiChord_abs[];
extern const int VChord_abs[];
extern const int IChord_abs[];
extern const int IVChord_abs[];
extern const int viChord_abs[];
extern const int iiiChord_abs[];

// EV Context Structure
struct EVContext {
    int error;    // 0-127
    int speed;    // 0-127
    int throttle; // 0-127
    int brake;    // 0-127
    // GPS context
    int heading;    // 0-359 degrees
    int altitude;   // meters
    int satellites; // signal quality (0-12)
    double latitude;
    double longitude;
};

// Ensemble Dynamics
enum MusicalRole { ROLE_LEAD, ROLE_COMPING, ROLE_BASS };

struct PeerState {
    uint8_t macAddr[6];
    int currentChordIdx;
    int intensity;
    int dissonance;
    int speed;
    double latitude;
    double longitude;
    MusicalRole role;
    int currentKeyOffset;
    bool listening;
    long firstSeen;
    long lastSeen;
    bool active;
};

struct EnsembleContext {
    PeerState peers[4];
    int peerCount;
    int collectiveTension;
    int ensembleKeyOffset; // -6 to +6 relative to local baseNote
    bool inCallAndResponse;
    int callAndResponseTicks;
};

// Predictors and Engine
struct TheoryPrediction {
    int nextChordIdx;
    int harmonyTension; // 0-100
};

struct PsychoacousticPrediction {
    int perceivedDissonance; // 0-100
    int energeticIntensity;  // 0-100
    int rhythmicJitter;      // 0-100
};

struct TheoryPredictor {
    int currentChordIdx;
    TheoryPrediction predict(const EVContext& context, const EnsembleContext& ensemble);
};

struct EnsemblePredictor {
    PsychoacousticPrediction predict(const EVContext& context, const EnsembleContext& ensemble);
};

struct PsychoacousticPredictor {
    int previousError;
    int trend;
    PsychoacousticPrediction predict(const EVContext& context);
};

struct CorrelationEngine {
    TheoryPredictor theory;
    PsychoacousticPredictor psycho;
    EnsemblePredictor ensemblePredictor;
    EnsembleContext ensemble;
    MusicalRole localRole;

    void process(const EVContext& context, int baseNote);
    void updatePeer(uint8_t* mac, int chordIdx, int intensity, int dissonance, int speed, double lat, double lon, int keyOffset, bool listening);

private:
    void cleanupPeers();
    int calculateTransposition(const EVContext& context, int baseNote);
    void performMusicalLogic(const EVContext& context, const PsychoacousticPrediction& pp, const TheoryPrediction& tp, int transOffset);
};

// Functions
bool isDissonant(int note, const int* contextNotes, int contextNotesCount);
int predictError(int currentError);
void sendChord(const int* chordDefinition, int chordDefSize, int transpositionOffset, int velocity = 100);
bool loadPatternFromSD(const char* filename, int* patternNotes, int* patternSize, int maxNotes);
void playChordProgression(const EVContext& context, int currentBaseNote);
void playChordProgressionWithEnsemble(const EVContext& context, const EnsembleContext& ensemble, int currentBaseNote);
void updateEnsemblePeer(uint8_t* mac, int chordIdx, int intensity, int dissonance, int speed, double lat, double lon, int keyOffset, bool listening);
void setLocalRole(MusicalRole role);
void logEnsembleStatus();
int getCurrentChordIdx();
int getCurrentKeyOffset();
bool isLocalListening();
void resetImprovisation();
void sendMIDINoteOnWrapper(int note, int velocity = 127);
void sendMIDINoteOffWrapper(int note);
void visualFeedback(int intensity);
void stopLastPlayedNotes();

#endif
