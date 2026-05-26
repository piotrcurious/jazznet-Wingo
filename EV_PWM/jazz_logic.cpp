#include "jazz_logic.h"

#ifdef MOCK_TESTING
#include <fstream>
#include <sstream>
MockMIDI MIDI;
MockSerial Serial;
#else
#include <SD.h>
#endif

const int ERROR_THRESHOLD_1 = 21;
const int ERROR_THRESHOLD_2 = 42;
const int ERROR_THRESHOLD_3 = 63;
const int ERROR_THRESHOLD_4 = 84;
const int ERROR_THRESHOLD_5 = 105;
const int MAX_NOTES_PER_CHORD = 4;
#ifdef ESP32
const int ADC_RESOLUTION = 4095;
#else
const int ADC_RESOLUTION = 1023;
#endif

static int previousError = 0;
static int trend = 0;
static int predictionState = 0;

static int lastPlayedNotes[MAX_NOTES_PER_CHORD] = {-1, -1, -1, -1};
static int lastPlayedNotesCount = 0;
static int lastTargetNotes[MAX_NOTES_PER_CHORD] = {-1, -1, -1, -1};
static int lastTargetNotesCount = 0;

const int iiChord_abs[] = {62, 65, 69, 72}; // Dm7
const int VChord_abs[] =  {67, 71, 74, 77}; // G7
const int IChord_abs[] =  {60, 64, 67, 71}; // Cmaj7
const int IVChord_abs[] = {65, 69, 72, 76}; // Fmaj7
const int viChord_abs[] = {69, 72, 76, 79}; // Am7
const int iiiChord_abs[] ={64, 67, 71, 74}; // Em7

const int* allChords[] = {iiChord_abs, VChord_abs, IChord_abs, IVChord_abs, viChord_abs, iiiChord_abs};
const char* chordNames[] = {"ii", "V", "I", "IV", "vi", "iii"};

// Markov transition matrix (simplified)
// Rows: current chord, Cols: next chord probabilities (scaled to 100)
static const int transitionMatrix[6][6] = {
  { 5, 70,  5,  5,  5, 10}, // ii -> V (strong), iii, vi
  { 5,  5, 70,  5, 10,  5}, // V  -> I (strong), vi
  {10, 10,  5, 20, 30, 25}, // I  -> IV, vi, iii
  {20, 30, 10,  5, 10, 25}, // IV -> V, ii, iii
  {50, 10,  5, 10,  5, 20}, // vi -> ii, iii
  {10, 10,  5, 10, 60,  5}  // iii-> vi
};

static int currentChordIdx = 2; // Start on I

void resetImprovisation() {
    currentChordIdx = 2;
    lastTargetNotesCount = 0;
    predictionState = 0;
    trend = 0;
}

bool isDissonant(int note, const int* contextNotes, int contextNotesCount) {
  for (int i = 0; i < contextNotesCount; ++i) {
    if (contextNotes[i] == -1) continue;
    int interval = abs(note - contextNotes[i]) % 12;
    // In jazz, major 2nds (2) and minor 7ths (10) are often acceptable.
    // Tritones (6) are essential in dominant chords.
    // Minor 2nd (1) is usually the most dissonant.
    if (interval == 1) {
      return true;
    }
  }
  return false;
}

int predictError(int currentError) {
  // Simple trend analysis: if error is increasing, assume it will continue.
  // We use predictionState to add some hysteresis to the trend detection.
  switch (predictionState) {
    case 0: // Stable
      if (currentError > previousError) {
        trend = 1;
        predictionState = 1; // Increasing
      } else if (currentError < previousError) {
        trend = -1;
        predictionState = 2; // Decreasing
      }
      break;
    case 1: // Increasing
      if (currentError < previousError) {
        predictionState = 0; // Became unstable, reset to stable and see next time
        trend = 0;
      }
      break;
    case 2: // Decreasing
      if (currentError > previousError) {
        predictionState = 0; // Became unstable
        trend = 0;
      }
      break;
  }

  int randomFactor = random(-2, 3);
  // Add some "momentum" to the prediction based on the trend
  int predictedError = currentError + trend * 3 + randomFactor;
  predictedError = constrain(predictedError, 0, 127);

  previousError = currentError;
  return predictedError;
}

void sendMIDINoteOnWrapper(int note, int velocity) {
  if (note >= 0 && note <= 127) {
    #ifdef MOCK_TESTING
    MIDI.sendNoteOn(note, velocity, 1);
    #else
    MIDI.sendNoteOn(note, velocity, 1);
    Serial.print("MIDI Note On: ");
    Serial.print(note);
    Serial.print(" Velocity: ");
    Serial.println(velocity);
    #endif
  }
}

void sendMIDINoteOffWrapper(int note) {
  if (note >= 0 && note <= 127) {
    #ifdef MOCK_TESTING
    MIDI.sendNoteOff(note, 0, 1);
    #else
    MIDI.sendNoteOff(note, 0, 1);
    Serial.print("MIDI Note Off: ");
    Serial.println(note);
    #endif
  }
}

bool loadPatternFromSD(const char* filename, int* patternNotes, int* patternSize, int maxNotes) {
    *patternSize = 0;
#ifdef MOCK_TESTING
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line) && *patternSize < maxNotes) {
        std::stringstream ss(line);
        std::string val;
        while (std::getline(ss, val, ',') && *patternSize < maxNotes) {
            try {
                patternNotes[*patternSize] = std::stoi(val);
                (*patternSize)++;
            } catch (...) {}
        }
    }
    return true;
#else
    char fullpath[64];
    snprintf(fullpath, sizeof(fullpath), "/%s", filename);
    File file = SD.open(fullpath);
    if (!file) return false;
    while (file.available() && *patternSize < maxNotes) {
        // Simple CSV-like parsing: assumes notes separated by commas or newlines
        int note = file.parseInt();
        if (note >= 0 && note <= 127) {
            patternNotes[*patternSize] = note;
            (*patternSize)++;
        }
    }
    file.close();
    return *patternSize > 0;
#endif
}

void stopLastPlayedNotes() {
  for (int i = 0; i < lastPlayedNotesCount; ++i) {
    if (lastPlayedNotes[i] != -1) {
      sendMIDINoteOffWrapper(lastPlayedNotes[i]);
      lastPlayedNotes[i] = -1;
    }
  }
  lastPlayedNotesCount = 0;
}

void sendChord(const int* chordDefinition, int chordDefSize, int transpositionOffset, int velocity) {
  stopLastPlayedNotes();

  int currentChordNotes[MAX_NOTES_PER_CHORD] = {-1, -1, -1, -1};
  int currentChordNotesCount = 0;

  // Simple voice leading: find the best octave for each note in the new chord
  // to be close to the average of the last played notes.
  int targetCenter = 64 + transpositionOffset;
  if (lastTargetNotesCount > 0) {
    long sum = 0;
    for (int i = 0; i < lastTargetNotesCount; ++i) sum += lastTargetNotes[i];
    targetCenter = sum / lastTargetNotesCount;
  }

  for (int i = 0; i < chordDefSize && currentChordNotesCount < MAX_NOTES_PER_CHORD; ++i) {
    int noteBase = (chordDefinition[i] + transpositionOffset) % 12;

    // Find octave that brings noteBase closest to targetCenter
    int bestNote = -1;
    int minDiff = 128;
    for (int octave = 3; octave <= 6; ++octave) {
      int candidate = noteBase + octave * 12;
      int diff = abs(candidate - targetCenter);
      if (diff < minDiff) {
        minDiff = diff;
        bestNote = candidate;
      }
    }

    if (bestNote >= 0 && bestNote <= 127) {
      if (!isDissonant(bestNote, currentChordNotes, currentChordNotesCount)) {
        sendMIDINoteOnWrapper(bestNote, velocity);
        currentChordNotes[currentChordNotesCount] = bestNote;
        lastPlayedNotes[currentChordNotesCount] = bestNote;
        currentChordNotesCount++;
        lastPlayedNotesCount++;
      }
    }
  }

  // Update lastTargetNotes for next call
  for (int i = 0; i < currentChordNotesCount; ++i) {
    lastTargetNotes[i] = currentChordNotes[i];
  }
  lastTargetNotesCount = currentChordNotesCount;
}

void playChordProgression(const EVContext& context, int currentBaseNote) {
  const int* chord1Def;
  const int* chord2Def;
  int chord1Size = 4;
  int chord2Size = 4;

  // Heading influences the "mode" or transposition offset
  int headingOffset = map(context.heading % 360, 0, 359, 0, 11);

  // Altitude shifts the overall register
  int altitudeOffset = (context.altitude / 100);

  // Base transposition offset
  int transpositionOffset = (currentBaseNote - 60) + headingOffset + altitudeOffset;

  // Speed influences the register (octave)
  int speedOffset = map(context.speed, 0, 127, -12, 12);
  transpositionOffset += speedOffset;

  // Signal quality (satellites) adds musical jitter/instability
  int jitter = 0;
  if (context.satellites < 6) {
    jitter = random(-5, 6);
  }

  // Markov-based chord selection
  // The error value influences how "surprising" (entropic) the next chord is.
  int nextChordIdx = currentChordIdx;
  int r = random(0, 100);
  int cumulative = 0;

  // Entropy injection: if error is high, we deviate from the most likely transition
  if (context.error > ERROR_THRESHOLD_3) {
      // Increase entropy based on error
      int entropy = map(context.error, ERROR_THRESHOLD_3, 127, 0, 50);
      r = (r + entropy) % 100;
  }

  bool found = false;
  for (int i = 0; i < 6; ++i) {
    cumulative += transitionMatrix[currentChordIdx][i];
    if (r < cumulative) {
      nextChordIdx = i;
      found = true;
      break;
    }
  }

  // Fallback (should not happen with good transition matrix)
  if (!found) nextChordIdx = random(0, 6);

  chord1Def = allChords[currentChordIdx];
  chord2Def = allChords[nextChordIdx];
  currentChordIdx = nextChordIdx;

  // Geospatial location for "recurring themes"
  // Use lat/lon to seed a local variations
  long geoSeed = (long)(context.latitude * 100) + (long)(context.longitude * 100);

  // Jazznet pattern integration: try to load a pattern based on location
  int jazznetNotes[16];
  int jazznetSize = 0;
  bool useJazznet = false;

  if (context.satellites >= 6) { // Only use Jazznet if we have good GPS
      char filename[32];
      // Use lat/lon to pick a file: /jazznet/P<seed>.CSV
      snprintf(filename, sizeof(filename), "jazznet/P%ld.CSV", geoSeed % 100);
      if (loadPatternFromSD(filename, jazznetNotes, &jazznetSize, 16)) {
          useJazznet = true;
      }
  }

  // Velocity influenced by throttle and error, but dampened by brake
  // Signal jitter also affects velocity
  int baseVelocity = map(context.error, 0, 127, 40, 80) + map(context.throttle, 0, 127, 0, 40) + jitter;
  int velocity = constrain(baseVelocity - map(context.brake, 0, 127, 0, 50), 30, 120);

  // Rhythm (delay) influenced by speed and error, slowed down by brake
  // Jitter affects timing slightly
  int baseDelay = map(context.error, 0, 127, 500, 200) - map(context.speed, 0, 127, 0, 100) + (jitter * 5);
  int actualDelay = constrain(baseDelay + map(context.brake, 0, 127, 0, 400), 100, 1000);

  // Syncopation: shift the timing of the second chord based on throttle, speed, and satellites (jitter)
  int syncopation = 0;
  if (context.throttle > 70 || context.speed > 70 || context.satellites < 4) {
      int maxSync = actualDelay / 3;
      syncopation = random(-maxSync, maxSync);
  }

  // Novelty Factor: high error or low satellite count increases novelty
  int noveltyFactor = constrain(map(context.error, 0, 127, 0, 50) + map(12 - context.satellites, 0, 12, 0, 50), 0, 100);

  auto playVariedChord = [&](const int* baseChord, int size, int trans, int vel) {
      int variedChord[MAX_NOTES_PER_CHORD];
      int count = 0;
      for (int i = 0; i < size && count < MAX_NOTES_PER_CHORD; ++i) {
          int note = baseChord[i];
          if (random(0, 100) < noveltyFactor) {
              // Substitute or add color
              int choice = random(0, 3);
              if (choice == 0) note += 2;  // Add a 9th
              else if (choice == 1) note += 5; // Add an 11th
              else note += 9; // Add a 13th
          }
          variedChord[count++] = note;
      }

      // If error is very high, arpeggiate instead of playing a block chord
      if (context.error > ERROR_THRESHOLD_4) {
          int arpDelay = actualDelay / (count > 0 ? count : 1);
          for (int i = 0; i < count; ++i) {
              sendChord(&variedChord[i], 1, trans, vel);
              delay(arpDelay);
          }
      } else {
          sendChord(variedChord, count, trans, vel);
      }
  };

  if (useJazznet && jazznetSize >= 4) {
      // Familiarity: play the pattern as is or with variations
      playVariedChord(jazznetNotes, jazznetSize > 4 ? 4 : jazznetSize, transpositionOffset, velocity);
  } else {
      playVariedChord(chord1Def, chord1Size, transpositionOffset, velocity);
  }
  visualFeedback(255);
  delay(actualDelay);

  // If braking hard, maybe don't play the second chord or play it very softly
  if (context.brake < 100) {
    int velocity2 = constrain(velocity + trend * 10, 30, 127);
    if (useJazznet && jazznetSize >= 8) {
        playVariedChord(jazznetNotes + 4, (jazznetSize - 4) > 4 ? 4 : (jazznetSize - 4), transpositionOffset, velocity2);
    } else {
        playVariedChord(chord2Def, chord2Size, transpositionOffset, velocity2);
    }
    visualFeedback(128);
  delay(actualDelay + syncopation);
  }

  stopLastPlayedNotes();
  visualFeedback(0);
}

void visualFeedback(int intensity) {
#ifndef MOCK_TESTING
  if (intensity > 0) {
    digitalWrite(13, HIGH);
  } else {
    digitalWrite(13, LOW);
  }
#endif
}
