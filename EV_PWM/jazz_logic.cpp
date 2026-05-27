#include "jazz_logic.h"

#ifdef MOCK_TESTING
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstring>
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

static CorrelationEngine engine = {{2}, {0, 0}, {}, {}};

void resetImprovisation() {
    engine.theory.currentChordIdx = 2;
    engine.psycho.previousError = 0;
    engine.psycho.trend = 0;
    engine.ensemble.peerCount = 0;
    for(int i=0; i<4; i++) engine.ensemble.peers[i].active = false;
    lastTargetNotesCount = 0;
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

TheoryPrediction TheoryPredictor::predict(const EVContext& context, const EnsembleContext& ensemble) {
    int nextChordIdx = currentChordIdx;
    int r = random(0, 100);
    int cumulative = 0;

    // Influence from ensemble: tendency to gravitate towards peers' chords
    // "Leadership" dynamics: peers with higher intensity have more influence.
    // "Harmonic Mimicry": peers also influence related chords (dominant/subdominant).
    int ensembleInfluence[6] = {0, 0, 0, 0, 0, 0};
    if (ensemble.peerCount > 0) {
        for (int i = 0; i < 4; i++) {
            if (ensemble.peers[i].active) {
                int peerChord = ensemble.peers[i].currentChordIdx;
                if (peerChord >= 0 && peerChord < 6) {
                    int leadership = map(ensemble.peers[i].intensity, 0, 127, 0, 20);
                    int baseWeight = 10 + leadership;
                    ensembleInfluence[peerChord] += baseWeight;

                    // Mimicry: small influence on related chords (simplified)
                    int dominant = (peerChord + 1) % 6;
                    int subdominant = (peerChord + 5) % 6;
                    ensembleInfluence[dominant] += baseWeight / 3;
                    ensembleInfluence[subdominant] += baseWeight / 3;
                }
            }
        }
    }

    int entropy = map(context.speed, 0, 127, 0, 40) + map(context.altitude % 1000, 0, 999, 0, 30);
    if (entropy > 20) {
        r = (r + entropy) % 100;
    }

    int totalWeight = 0;
    for (int i = 0; i < 6; ++i) {
        totalWeight += transitionMatrix[currentChordIdx][i] + ensembleInfluence[i];
    }
    r = random(0, totalWeight);

    bool found = false;
    for (int i = 0; i < 6; ++i) {
        int weight = transitionMatrix[currentChordIdx][i] + ensembleInfluence[i];
        cumulative += weight;
        if (r < cumulative) {
            nextChordIdx = i;
            found = true;
            break;
        }
    }
    if (!found) nextChordIdx = random(0, 6);

    int tensions[] = {50, 90, 10, 40, 70, 30};
    int harmonyTension = tensions[nextChordIdx];
    if (context.speed > 90 && (nextChordIdx == 2 || nextChordIdx == 5)) {
        harmonyTension += 30;
    }

    return {nextChordIdx, harmonyTension};
}

PsychoacousticPrediction EnsemblePredictor::predict(const EVContext& context, const EnsembleContext& ensemble) {
    int collectiveDissonance = 0;
    int collectiveIntensity = 0;
    int activePeers = 0;

    for (int i = 0; i < 4; i++) {
        if (ensemble.peers[i].active) {
            collectiveIntensity += ensemble.peers[i].intensity;
            collectiveDissonance += ensemble.peers[i].dissonance;
            activePeers++;
        }
    }

    if (activePeers > 0) {
        collectiveIntensity /= activePeers;
        collectiveDissonance /= activePeers;
    }

    return {collectiveDissonance, collectiveIntensity, 0};
}

PsychoacousticPrediction PsychoacousticPredictor::predict(const EVContext& context) {
    int currentTotalStress = (context.error + context.speed / 2);
    int stressDiff = currentTotalStress - previousError;

    if (stressDiff > 15) trend = 2;
    else if (stressDiff > 0) trend = 1;
    else if (stressDiff < -15) trend = -2;
    else if (stressDiff < 0) trend = -1;
    else trend = 0;

    int dissonance = map(context.error, 0, 127, 0, 70);
    if (context.speed > 80) dissonance += map(context.speed, 80, 127, 0, 30);

    int intensity = map(context.throttle, 0, 127, 30, 100);
    if (context.brake > 10) {
        intensity -= map(context.brake, 10, 127, 0, 80);
    }

    int jitter = 0;
    if (context.satellites < 5) jitter += map(5 - context.satellites, 0, 5, 0, 50);
    if (context.speed > 110) jitter += map(context.speed, 110, 127, 0, 30);

    previousError = currentTotalStress;
    return {constrain(dissonance, 0, 100), constrain(intensity, 0, 100), constrain(jitter, 0, 100)};
}

void CorrelationEngine::process(const EVContext& context, int baseNote) {
    // Cleanup inactive peers
    long now = millis();
    for (int i = 0; i < 4; i++) {
        if (ensemble.peers[i].active && (now - ensemble.peers[i].lastSeen > 5000)) {
            ensemble.peers[i].active = false;
            ensemble.peerCount--;
        }
    }

    TheoryPrediction tp = theory.predict(context, ensemble);
    PsychoacousticPrediction pp = psycho.predict(context);
    PsychoacousticPrediction ep = ensemblePredictor.predict(context, ensemble);

    theory.currentChordIdx = tp.nextChordIdx;

    // Blend ensemble intensity and dissonance
    if (ensemble.peerCount > 0) {
        pp.energeticIntensity = (pp.energeticIntensity * 7 + ep.energeticIntensity * 3) / 10;
        pp.perceivedDissonance = (pp.perceivedDissonance * 8 + ep.perceivedDissonance * 2) / 10;

        // Stateful Call and Response
        if (ensemble.callAndResponseTicks > 0) {
            ensemble.callAndResponseTicks--;
        } else {
            if (ep.energeticIntensity > 85 && random(0, 100) < 20) {
                ensemble.inCallAndResponse = true;
                ensemble.callAndResponseTicks = 4; // Stay in mode for 4 iterations
            } else {
                ensemble.inCallAndResponse = false;
            }
        }
    } else {
        ensemble.inCallAndResponse = false;
        ensemble.callAndResponseTicks = 0;
    }

    long geoSeed = (long)(context.latitude * 100) + (long)(context.longitude * 100);
    int patternIdx = (geoSeed + (context.speed / 10)) % 100;

    int jazznetNotes[32];
    int jazznetSize = 0;
    bool useJazznet = false;

    char filename[128];
    const char* typeStr = "chord";
    const char* modeStr = "maj7-chord";

    if (pp.perceivedDissonance > 75) {
        typeStr = "scale";
        modeStr = "locrian";
    } else if (pp.perceivedDissonance > 50) {
        typeStr = "arpeggio";
        modeStr = "min7-arpeggio";
    } else if (tp.harmonyTension > 60) {
        typeStr = "chord";
        modeStr = "seventh-chord";
    }

    snprintf(filename, sizeof(filename), "jazznet/%s/%s/C-4-%s-0_0.CSV", typeStr, modeStr, modeStr);

    if (context.satellites >= 3) {
        loadPatternFromSD(filename, jazznetNotes, &jazznetSize, 32);
        useJazznet = (jazznetSize > 0);
    }

    int headingOffset = map(context.heading % 360, 0, 359, 0, 11);
    int altitudeOffset = (context.altitude / 100);
    int speedOffset = map(context.speed, 0, 127, -12, 12);
    int transpositionOffset = (baseNote - 60) + headingOffset + altitudeOffset + speedOffset;

    int velocity = map(pp.energeticIntensity, 0, 100, 30, 120);
    // Peer leadership influences local tempo
    int tempoOffset = 0;
    if (ensemble.peerCount > 0) {
        int maxPeerSpeed = 0;
        for(int i=0; i<4; i++) if(ensemble.peers[i].active && ensemble.peers[i].speed > maxPeerSpeed) maxPeerSpeed = ensemble.peers[i].speed;
        tempoOffset = map(maxPeerSpeed, 0, 127, -50, 50);
    }
    int actualDelay = constrain(map(100 - pp.energeticIntensity, 0, 100, 200, 800) - tempoOffset, 100, 1200);
    int syncopation = map(pp.rhythmicJitter, 0, 100, 0, actualDelay / 3);

    auto playVariedChordLocal = [&](const int* baseChord, int size, int trans, int vel, int novelty) {
        int variedChord[MAX_NOTES_PER_CHORD];
        int count = 0;
        for (int i = 0; i < size && count < MAX_NOTES_PER_CHORD; ++i) {
            int note = baseChord[i];
            if (random(0, 100) < novelty) {
                int choice = random(0, 3);
                if (choice == 0) note += 2;
                else if (choice == 1) note += 5;
                else note += 9;
            }
            variedChord[count++] = note;
        }

        if (pp.perceivedDissonance > 80) {
            int arpDelay = actualDelay / (count > 0 ? count : 1);
            for (int i = 0; i < count; ++i) {
                sendChord(&variedChord[i], 1, trans, vel);
                delay(arpDelay);
            }
        } else {
            sendChord(variedChord, count, trans, vel);
        }
    };

    int novelty = (pp.perceivedDissonance + tp.harmonyTension) / 2;

    if (useJazznet) {
        for (int offset = 0; offset < jazznetSize; offset += 4) {
            if (ensemble.inCallAndResponse && offset > 0 && random(0,100) < 70) continue; // Skip notes in "listen" mode

            int currentSegmentSize = (jazznetSize - offset > 4) ? 4 : (jazznetSize - offset);
            int currentVelocity = (offset == 0) ? velocity : constrain(velocity + psycho.trend * 5, 30, 127);
            playVariedChordLocal(jazznetNotes + offset, currentSegmentSize, transpositionOffset, currentVelocity, novelty);
            visualFeedback(offset == 0 ? 255 : 128);
            delay(actualDelay + (offset + 4 >= jazznetSize ? syncopation : 0));
            if (context.brake > 90) break;
        }
    } else {
        playVariedChordLocal(allChords[tp.nextChordIdx], 4, transpositionOffset, velocity, novelty);
        visualFeedback(255);
        delay(actualDelay);
        if (context.brake < 100) {
            int nextIdx = (tp.nextChordIdx + 1) % 6;
            playVariedChordLocal(allChords[nextIdx], 4, transpositionOffset, velocity, novelty);
            visualFeedback(128);
            delay(actualDelay + syncopation);
        }
    }

    stopLastPlayedNotes();
    visualFeedback(0);
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

void CorrelationEngine::updatePeer(uint8_t* mac, int chordIdx, int intensity, int dissonance, int speed) {
    if (chordIdx < 0 || chordIdx >= 6) return; // Validation

    int slot = -1;
    for (int i = 0; i < 4; i++) {
        if (ensemble.peers[i].active && memcmp(ensemble.peers[i].macAddr, mac, 6) == 0) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        for (int i = 0; i < 4; i++) {
            if (!ensemble.peers[i].active) {
                slot = i;
                ensemble.peerCount++;
                break;
            }
        }
    }

    if (slot != -1) {
        memcpy(ensemble.peers[slot].macAddr, mac, 6);
        ensemble.peers[slot].currentChordIdx = chordIdx;
        ensemble.peers[slot].intensity = intensity;
        ensemble.peers[slot].dissonance = dissonance;
        ensemble.peers[slot].speed = speed;
        ensemble.peers[slot].lastSeen = millis();
        ensemble.peers[slot].active = true;
    }
}

void playChordProgression(const EVContext& context, int currentBaseNote) {
  engine.process(context, currentBaseNote);
}

void playChordProgressionWithEnsemble(const EVContext& context, const EnsembleContext& ensemble, int currentBaseNote) {
    engine.ensemble = ensemble;
    engine.process(context, currentBaseNote);
}

void updateEnsemblePeer(uint8_t* mac, int chordIdx, int intensity, int dissonance, int speed) {
    engine.updatePeer(mac, chordIdx, intensity, dissonance, speed);
}

int getCurrentChordIdx() {
    return engine.theory.currentChordIdx;
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
