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

static const int transitionMatrix[6][6] = {
  { 5, 70,  5,  5,  5, 10},
  { 5,  5, 70,  5, 10,  5},
  {10, 10,  5, 20, 30, 25},
  {20, 30, 10,  5, 10, 25},
  {50, 10,  5, 10,  5, 20},
  {10, 10,  5, 10, 60,  5}
};

#ifdef MOCK_TESTING
std::mutex ensembleMutex;
#else
SemaphoreHandle_t ensembleMutex = NULL;
#endif

void initEnsembleMutex() {
#ifndef MOCK_TESTING
    if (ensembleMutex == NULL) {
        ensembleMutex = xSemaphoreCreateMutex();
    }
#endif
}

static CorrelationEngine engine = {{2, MOOD_RESOLUTION}, {0, 0}, {}, {}, ROLE_COMPING, {0, 0, 0, 0, 1.0f, false, 100, 0, 2, 0}};

void resetImprovisation() {
    engine.theory.currentChordIdx = 2;
    engine.theory.localMood = MOOD_RESOLUTION;
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
    // Avoid minor seconds in the same octave-class.
    // Note: Tritones (6) are allowed as they are essential to Jazz dominant harmony.
    if (interval == 1) return true;
  }
  return false;
}

TheoryPrediction TheoryPredictor::predict(const EVContext& context, const EnsembleContext& ensemble) {
    if (ensemble.peerCount > 0) {
        int tensionCount = 0;
        int groupMoodSum = 0;
        int trustedPeers = 0;
        for(int i=0; i<4; i++) {
            if(ensemble.peers[i].active) {
                // If a peer is highly dissonant (clashRate), reduce their influence
                if (ensemble.peers[i].clashRate > 70) continue;

                if (ensemble.peers[i].intensity > 90) tensionCount++;
                groupMoodSum += (int)ensemble.peers[i].mood;
                trustedPeers++;
            }
        }

        // Ensemble Tension Waves: synchronizing mood transitions
        if (trustedPeers > 0) {
            int avgMood = groupMoodSum / trustedPeers;
            if (random(0,100) < 20) localMood = (EnsembleMood)avgMood;
        }

        if (tensionCount > ensemble.peerCount / 2) {
            if (random(0,100) < 40) localMood = MOOD_TENSION;
        } else if (tensionCount == 0) {
            if (random(0,100) < 30) {
                // Resolution or Discovery based on variety
                localMood = (random(0,100) < 30) ? MOOD_DISCOVERY : MOOD_RESOLUTION;
            }
        }
    }

    int nextChordIdx = currentChordIdx;
    int cumulative = 0;
    int ensembleInfluence[6] = {0, 0, 0, 0, 0, 0};
    if (ensemble.peerCount > 0) {
        long now = millis();
        int peerChordVotes[6] = {0, 0, 0, 0, 0, 0};
        for (int i = 0; i < 4; i++) {
            if (ensemble.peers[i].active) {
                // Influence Gating: Peers with high clash rates have reduced harmonic pull
                int awarenessGating = map(constrain(ensemble.peers[i].clashRate, 0, 100), 0, 100, 10, 0);

                int peerChord = ensemble.peers[i].currentChordIdx;
                if (peerChord >= 0 && peerChord < 6) {
                    long activeSec = (now - ensemble.peers[i].firstSeen) / 1000;
                    int trustBonus = (activeSec > 30) ? 15 : (int)(activeSec / 2);
                    double dl = context.latitude - ensemble.peers[i].latitude;
                    double dg = context.longitude - ensemble.peers[i].longitude;
                    long distSq = (long)((dl*dl + dg*dg) * 1000000);
                    int distanceScale = map(constrain(distSq, 0, 1000), 0, 1000, 25, 5);
                    int leadership = map(ensemble.peers[i].intensity, 0, 127, 0, 25);
                    int baseWeight = (12 + leadership + trustBonus) * distanceScale * awarenessGating / 100;
                    ensembleInfluence[peerChord] += baseWeight;
                    ensembleInfluence[(peerChord + 1) % 6] += baseWeight / 2;
                    ensembleInfluence[(peerChord + 5) % 6] += baseWeight / 2;
                    peerChordVotes[peerChord]++;
                }
            }
        }
        // Social Harmonic Entrainment: resolve collective ii-V or IV-V to I
        if (peerChordVotes[1] > 0 && (peerChordVotes[0] > 0 || peerChordVotes[3] > 0)) ensembleInfluence[2] += 40;
    }

    int totalWeight = 0;
    for (int i = 0; i < 6; ++i) totalWeight += transitionMatrix[currentChordIdx][i] + ensembleInfluence[i];
    int r = random(0, totalWeight);

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
    if (localMood == MOOD_TENSION) tensions[nextChordIdx] += 20;
    else if (localMood == MOOD_RESOLUTION) tensions[nextChordIdx] -= 10;
    int harmonyTension = tensions[nextChordIdx];
    if (context.speed > 90 && (nextChordIdx == 2 || nextChordIdx == 5)) harmonyTension += 30;
    return {nextChordIdx, harmonyTension};
}

PsychoacousticPrediction EnsemblePredictor::predict(const EVContext& context, const EnsembleContext& ensemble) {
    int collectiveDissonance = 0, collectiveIntensity = 0, activePeersCount = 0;
    for (int i = 0; i < 4; i++) {
        if (ensemble.peers[i].active) {
            collectiveIntensity += ensemble.peers[i].intensity;
            collectiveDissonance += ensemble.peers[i].dissonance;
            activePeersCount++;
        }
    }
    if (activePeersCount > 0) {
        collectiveIntensity /= activePeersCount;
        collectiveDissonance /= activePeersCount;
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
    if (context.brake > 10) intensity -= map(context.brake, 10, 127, 0, 80);
    int jitter = 0;
    if (context.satellites < 5) jitter += map(5 - context.satellites, 0, 5, 0, 50);
    if (context.speed > 110) jitter += map(context.speed, 110, 127, 0, 30);
    previousError = currentTotalStress;
    return {constrain(dissonance, 0, 100), constrain(intensity, 0, 100), constrain(jitter, 0, 100)};
}

void CorrelationEngine::cleanupPeers() {
    long now = millis();
    for (int i = 0; i < 4; i++) {
        if (ensemble.peers[i].active && (now - ensemble.peers[i].lastSeen > 5000)) {
            ensemble.peers[i].active = false;
            ensemble.peerCount--;
        }
    }
}

int CorrelationEngine::calculateTransposition(const EVContext& context, int baseNote) {
    int headingOffset = map(context.heading % 360, 0, 359, 0, 11);
    int altitudeOffset = (context.altitude / 100);
    int speedOffset = map(context.speed, 0, 127, -12, 12);
    // Data-Aware Transposition: if data quality is low, reduce influence of heading/altitude
    int geoWeight = (awareness.dataQuality > 50) ? 10 : 2;
    int transpositionOffset = (baseNote + ensemble.ensembleKeyOffset - 60) + (headingOffset * geoWeight / 10) + (altitudeOffset * geoWeight / 10) + speedOffset;
    if (localRole == ROLE_BASS) transpositionOffset -= 12;
    else if (localRole == ROLE_LEAD) transpositionOffset += 12;
    return transpositionOffset;
}

void CorrelationEngine::process(const EVContext& context, int baseNote) {
    ENSEMBLE_LOCK();
    cleanupPeers();

    if (ensemble.peerCount > 0) {
        bool peerHigherIntensity = false;
        bool peerNeedsSupport = false;
        for(int i=0; i<4; i++) {
            if(ensemble.peers[i].active) {
                if (ensemble.peers[i].intensity > context.throttle + 20) peerHigherIntensity = true;
                // Altruistic Awareness: Detect peers with high clash rates
                if (ensemble.peers[i].clashRate > 50) peerNeedsSupport = true;
            }
        }

        // If a peer is struggling (high clashRate) and we are confident, switch to COMPING to stabilize
        if (peerNeedsSupport && awareness.confidence > 0.7f && localRole != ROLE_BASS) {
            localRole = ROLE_COMPING;
        } else if (peerHigherIntensity && random(0,100) < 10) {
            localRole = ROLE_COMPING;
        } else if (!peerHigherIntensity && context.throttle > 100 && random(0,100) < 10) {
            localRole = ROLE_LEAD;
        }

        int targetKeyOffset = 0, activePeersCount = 0;
        for(int i=0; i<4; i++) {
            if(ensemble.peers[i].active) {
                targetKeyOffset += ensemble.peers[i].currentKeyOffset;
                activePeersCount++;
            }
        }
        if (activePeersCount > 0) {
            targetKeyOffset /= activePeersCount;
            if (random(0, 100) < 30) {
                if (ensemble.ensembleKeyOffset < targetKeyOffset) ensemble.ensembleKeyOffset++;
                else if (ensemble.ensembleKeyOffset > targetKeyOffset) ensemble.ensembleKeyOffset--;
            }
        }
    }

    TheoryPrediction tp = theory.predict(context, ensemble);
    PsychoacousticPrediction pp = psycho.predict(context);
    PsychoacousticPrediction ep = ensemblePredictor.predict(context, ensemble);

    // Temporal Awareness: Boredom & Repetition tracking
    if (tp.nextChordIdx == awareness.lastChordIdx) {
        awareness.chordRepetitionCount++;
        if (awareness.chordRepetitionCount > 3) {
            awareness.boredom = (awareness.boredom * 8 + 100) / 10;
        }
    } else {
        awareness.chordRepetitionCount = 0;
        awareness.boredom = (awareness.boredom * 9) / 10;
    }
    awareness.lastChordIdx = tp.nextChordIdx;

    // Trigger state change if bored
    if (awareness.boredom > 70) {
        if (theory.localMood == MOOD_RESOLUTION) theory.localMood = MOOD_TENSION;
        else theory.localMood = MOOD_DISCOVERY;
        awareness.boredom /= 2;
        // Forced chord change
        tp.nextChordIdx = (tp.nextChordIdx + random(1, 5)) % 6;
    }

    theory.currentChordIdx = tp.nextChordIdx;

    // Inward-Looking Awareness: Data Quality assessment
    int satelliteQuality = map(constrain(context.satellites, 0, 12), 0, 12, 0, 70);
    int peerStability = (ensemble.peerCount > 0) ? 30 : 0;
    awareness.dataQuality = satelliteQuality + peerStability;

    // Reflective Awareness: Evaluate alignment and update confidence
    if (ensemble.peerCount > 0) {
        int moodSum = 0;
        for (int i = 0; i < 4; i++) if (ensemble.peers[i].active) moodSum += (int)ensemble.peers[i].mood;
        int avgMood = moodSum / ensemble.peerCount;
        awareness.moodAlignment = 100 - abs((int)theory.localMood - avgMood) * 33;
    } else {
        awareness.moodAlignment = 100;
    }

    // Confidence decreases with clashes, poor alignment, and poor data quality
    float dqScale = awareness.dataQuality / 100.0f;
    float targetConfidence = (awareness.moodAlignment / 100.0f) * (1.0f - awareness.peerClashRate / 150.0f) * (0.5f + dqScale * 0.5f);
    awareness.confidence = (awareness.confidence * 0.9f) + (targetConfidence * 0.1f);

    if (ensemble.peerCount > 0) {
        pp.energeticIntensity = (pp.energeticIntensity * 7 + ep.energeticIntensity * 3) / 10;
        pp.perceivedDissonance = (pp.perceivedDissonance * 8 + ep.perceivedDissonance * 2) / 10;
        if (ensemble.callAndResponseTicks > 0) ensemble.callAndResponseTicks--;
        else {
            bool peerListening = false;
            for(int i=0; i<4; i++) if(ensemble.peers[i].active && ensemble.peers[i].listening) peerListening = true;
            int listenProb = 20 + ensemble.peerCount * 15;
            if (peerListening) listenProb += 30;
            if (ep.energeticIntensity > 80 && random(0, 100) < listenProb) {
                ensemble.inCallAndResponse = true;
                ensemble.callAndResponseTicks = 4;
            } else ensemble.inCallAndResponse = false;
        }
    } else {
        ensemble.inCallAndResponse = false;
        ensemble.callAndResponseTicks = 0;
    }

    int transOffset = calculateTransposition(context, baseNote);
    EnsembleContext ensSnapshot = ensemble;
    ENSEMBLE_UNLOCK();
    performMusicalLogic(context, pp, tp, transOffset, ensSnapshot);
}

void CorrelationEngine::performMusicalLogic(const EVContext& context, const PsychoacousticPrediction& pp, const TheoryPrediction& tp, int transpositionOffset, const EnsembleContext& ensSnap) {
    long geoSeed = (long)(context.latitude * 100) + (long)(context.longitude * 100);

    // Pattern Selection refinement: role and mood influence the CSV choice
    int baseRange = 0;
    if (localRole == ROLE_BASS) baseRange = 0; // 0-33
    else if (localRole == ROLE_COMPING) baseRange = 34; // 34-66
    else baseRange = 67; // 67-99

    int moodShift = 0;
    if (theory.localMood == MOOD_TENSION) moodShift = 5;
    else if (theory.localMood == MOOD_DISCOVERY) moodShift = 10;

    int patternIdx = (baseRange + (geoSeed % 20) + moodShift) % 100;
    int jazznetNotes[32], jazznetSize = 0;
    bool useJazznet = false;
    char filename[128];
    const char *typeStr = "chord", *modeStr = "maj7-chord";

    // Orchestral Role Specialization
    if (localRole == ROLE_BASS) typeStr = "arpeggio";
    else if (localRole == ROLE_LEAD) typeStr = (random(0,100) < 50) ? "scale" : "progression";

    if (pp.perceivedDissonance > 75) { modeStr = "locrian"; }
    else if (pp.perceivedDissonance > 50) { modeStr = "min7-arpeggio"; }
    else if (tp.harmonyTension > 60) { modeStr = "seventh-chord"; }

    if (theory.localMood == MOOD_TENSION) { modeStr = "diminished"; }
    else if (theory.localMood == MOOD_DISCOVERY) { modeStr = "lydian"; }

    snprintf(filename, sizeof(filename), "jazznet/%s/%s/C-4-%s-0_0.CSV", typeStr, modeStr, modeStr);
    if (context.satellites >= 3) {
        loadPatternFromSD(filename, jazznetNotes, &jazznetSize, 32);
        useJazznet = (jazznetSize > 0);
    }

    int velocity = map(pp.energeticIntensity, 0, 100, 30, 120);

    // Confidence-Weighted Dynamics: softer if uncertain, slightly louder if confident
    velocity = (int)(velocity * (0.5f + awareness.confidence * 0.5f));

    // Panic Suppression: Mute if confidence is extremely low
    if (awareness.confidence < 0.2f && random(0,100) < 50) velocity = 0;

    if (localRole == ROLE_BASS) velocity = constrain(velocity + 15, 0, 127);
    int tempoOffset = 0;
    if (ensSnap.peerCount > 0) {
        int maxPeerSpeed = 0;
        for(int i=0; i<4; i++) if(ensSnap.peers[i].active && ensSnap.peers[i].speed > maxPeerSpeed) maxPeerSpeed = ensSnap.peers[i].speed;
        tempoOffset = map(maxPeerSpeed, 0, 127, -50, 50);
    }
    int actualDelay = constrain(map(100 - pp.energeticIntensity, 0, 100, 200, 800) - tempoOffset, 100, 1200);

    // Adaptive Timing: slow down if confidence is low
    if (awareness.confidence < 0.5f) actualDelay = (int)(actualDelay * 1.5f);

    int syncopation = map(pp.rhythmicJitter, 0, 100, 0, actualDelay / 3);

    auto playVariedChordLocal = [&](const int* baseChord, int size, int trans, int vel, int novelty) {
        int maxNotesAllowed = MAX_NOTES_PER_CHORD;

        bool peerNeedsSupport = false;
        for(int i=0; i<4; i++) if(ensSnap.peers[i].active && ensSnap.peers[i].clashRate > 50) peerNeedsSupport = true;

        // Altruistic adaptation: if supporting, simplify output further
        if (peerNeedsSupport && awareness.confidence > 0.7f) {
            maxNotesAllowed = 2;
            novelty = 0; // Purest harmony for support
        } else {
            if (ensSnap.peerCount >= 2) maxNotesAllowed = 3;
            if (ensSnap.peerCount >= 4) maxNotesAllowed = 2;
        }

        int variedChord[MAX_NOTES_PER_CHORD], count = 0;
        for (int i = 0; i < size && count < maxNotesAllowed; ++i) {
            int note = baseChord[i];
            if (random(0, 100) < novelty) {
                int choice = random(0, 3);
                if (choice == 0) note += 2; else if (choice == 1) note += 5; else note += 9;
            }
            variedChord[count++] = note;
        }
        if (pp.perceivedDissonance > 80) {
            int arpDelay = actualDelay / (count > 0 ? count : 1);
            for (int i = 0; i < count; ++i) { sendChord(&variedChord[i], 1, trans, vel); delay(arpDelay); }
        } else sendChord(variedChord, count, trans, vel);
    };

    // Reflective Adaptation: Decrease novelty and complexity if confidence is low or clash rate is high
    int novelty = (pp.perceivedDissonance + tp.harmonyTension) / 2;
    if (awareness.confidence < 0.6f || awareness.peerClashRate > 40) {
        novelty /= 2;
        if (awareness.peerClashRate > 60) novelty = 0; // Extreme caution
    }

    if (useJazznet) {
        if (localRole == ROLE_COMPING) novelty /= 2;
        for (int offset = 0; offset < jazznetSize; offset += 4) {
            if (ensSnap.inCallAndResponse && offset > 0 && random(0,100) < 70) continue;
            int currentSegmentSize = (jazznetSize - offset > 4) ? 4 : (jazznetSize - offset);
            int currentVelocity = (offset == 0) ? velocity : constrain(velocity + psycho.trend * 5, 30, 127);
            playVariedChordLocal(jazznetNotes + offset, currentSegmentSize, transpositionOffset, currentVelocity, novelty);
            visualFeedback(offset == 0 ? 255 : 128);
            delay(actualDelay + (offset + 4 >= jazznetSize ? syncopation : 0));
            if (context.brake > 90) break;
        }
    } else {
        playVariedChordLocal(allChords[tp.nextChordIdx], 4, transpositionOffset, velocity, novelty);
        visualFeedback(255); delay(actualDelay);
        if (context.brake < 100) {
            int nextIdx = (tp.nextChordIdx + 1) % 6;
            playVariedChordLocal(allChords[nextIdx], 4, transpositionOffset, velocity, novelty);
            visualFeedback(128); delay(actualDelay + syncopation);
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
    Serial.print("MIDI Note On: "); Serial.print(note); Serial.print(" Velocity: "); Serial.println(velocity);
    #endif
  }
}

void sendMIDINoteOffWrapper(int note) {
  if (note >= 0 && note <= 127) {
    #ifdef MOCK_TESTING
    MIDI.sendNoteOff(note, 0, 1);
    #else
    MIDI.sendNoteOff(note, 0, 1);
    Serial.print("MIDI Note Off: "); Serial.println(note);
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
        std::stringstream ss(line); std::string val;
        while (std::getline(ss, val, ',') && *patternSize < maxNotes) {
            try { patternNotes[*patternSize] = std::stoi(val); (*patternSize)++; } catch (...) {}
        }
    }
    return true;
#else
    char fullpath[128]; snprintf(fullpath, sizeof(fullpath), "/%s", filename);
    File file = SD.open(fullpath); if (!file) return false;
    while (file.available() && *patternSize < maxNotes) {
        int note = file.parseInt(); if (note >= 0 && note <= 127) { patternNotes[*patternSize] = note; (*patternSize)++; }
    }
    file.close(); return *patternSize > 0;
#endif
}

void stopLastPlayedNotes() {
  for (int i = 0; i < lastPlayedNotesCount; ++i) {
    if (lastPlayedNotes[i] != -1) { sendMIDINoteOffWrapper(lastPlayedNotes[i]); lastPlayedNotes[i] = -1; }
  }
  lastPlayedNotesCount = 0;
}

void sendChord(const int* chordDefinition, int chordDefSize, int transpositionOffset, int velocity) {
  stopLastPlayedNotes();
  int currentChordNotes[MAX_NOTES_PER_CHORD] = {-1, -1, -1, -1};
  int currentChordNotesCount = 0;
  int targetCenter = 64 + transpositionOffset;

  // Collect peer notes to avoid "Ensemble Dissonance"
  int ensembleNotes[16]; // 4 peers * 4 notes
  int ensembleNotesCount = 0;
  ENSEMBLE_LOCK();
  for (int i = 0; i < 4; i++) {
    if (engine.ensemble.peers[i].active) {
      int pChordIdx = engine.ensemble.peers[i].currentChordIdx;
      int pTrans = engine.ensemble.peers[i].currentKeyOffset; // simplified peer trans
      for (int j = 0; j < 4; j++) {
        ensembleNotes[ensembleNotesCount++] = (allChords[pChordIdx][j] + pTrans);
      }
    }
  }
  ENSEMBLE_UNLOCK();

  if (lastTargetNotesCount > 0) {
    long sum = 0; for (int i = 0; i < lastTargetNotesCount; ++i) sum += lastTargetNotes[i];
    targetCenter = sum / lastTargetNotesCount;
  }

  int minOct = 3, maxOct = 6;
  if (engine.localRole == ROLE_BASS) { minOct = 2; maxOct = 4; }
  else if (engine.localRole == ROLE_LEAD) { minOct = 5; maxOct = 8; }
  else if (engine.localRole == ROLE_COMPING) { minOct = 4; maxOct = 6; }

    // Virtuosity: Expand registers if very confident and not bored
    if (engine.awareness.confidence > 0.9f && engine.awareness.boredom < 20) {
        minOct--; maxOct++;
    }

  engine.awareness.inAvoidantCorrection = false;

  for (int i = 0; i < chordDefSize && currentChordNotesCount < MAX_NOTES_PER_CHORD; ++i) {
    int noteBase = (chordDefinition[i] + transpositionOffset) % 12;
    int bestNote = -1, minDiff = 128;
    for (int octave = minOct; octave <= maxOct; ++octave) {
      int candidate = noteBase + octave * 12;
      int diff = abs(candidate - targetCenter);
      if (diff < minDiff) { minDiff = diff; bestNote = candidate; }
    }

    if (bestNote >= 0 && bestNote <= 127) {
      bool localDissonant = isDissonant(bestNote, currentChordNotes, currentChordNotesCount);
      bool ensembleClash = isDissonant(bestNote % 12, ensembleNotes, ensembleNotesCount); // compare pitch class

      if (localDissonant || ensembleClash) {
        // Self-Correction: Try shifting by a minor third (3) or fourth (5) to resolve clash
        int corrections[] = {3, 5, 7, -2};
        bool fixed = false;
        for (int c : corrections) {
          int correctedNote = bestNote + c;
          if (!isDissonant(correctedNote, currentChordNotes, currentChordNotesCount) &&
              !isDissonant(correctedNote % 12, ensembleNotes, ensembleNotesCount)) {
            bestNote = correctedNote;
            fixed = true;
            engine.awareness.selfCorrectionCount++;
            engine.awareness.inAvoidantCorrection = true;
            break;
          }
        }
        if (ensembleClash) engine.awareness.peerClashRate = (engine.awareness.peerClashRate * 9 + 100) / 10;

        if (!fixed) {
           // If we can't fix it, skip the note to avoid dissonance (Avoidant behavior)
           continue;
        }
      } else {
        engine.awareness.peerClashRate = (engine.awareness.peerClashRate * 9) / 10;
      }

      sendMIDINoteOnWrapper(bestNote, velocity);
      currentChordNotes[currentChordNotesCount] = bestNote;
      lastPlayedNotes[currentChordNotesCount] = bestNote;
      currentChordNotesCount++; lastPlayedNotesCount++;
    }
  }
  for (int i = 0; i < currentChordNotesCount; ++i) lastTargetNotes[i] = currentChordNotes[i];
  lastTargetNotesCount = currentChordNotesCount;

  // Update awareness state
  engine.awareness.internalDissonance = (currentChordNotesCount < chordDefSize) ? 30 : 0;
}

void CorrelationEngine::updatePeer(const uint8_t* mac, int chordIdx, int intensity, int dissonance, int speed, double lat, double lon, int keyOffset, bool listening, int mood, int clashRate) {
    if (chordIdx < 0 || chordIdx >= 6) return;
    ENSEMBLE_LOCK();
    int slot = -1;
    for (int i = 0; i < 4; i++) {
        if (ensemble.peers[i].active && memcmp(ensemble.peers[i].macAddr, mac, 6) == 0) { slot = i; break; }
    }
    if (slot == -1) {
        for (int i = 0; i < 4; i++) { if (!ensemble.peers[i].active) { slot = i; ensemble.peerCount++; break; } }
        if (slot == -1) {
            long oldestSeen = millis();
            for (int i = 0; i < 4; i++) { if (ensemble.peers[i].lastSeen < oldestSeen) { oldestSeen = ensemble.peers[i].lastSeen; slot = i; } }
        }
    }
    if (slot != -1) {
        if (!ensemble.peers[slot].active) ensemble.peers[slot].firstSeen = millis();
        memcpy(ensemble.peers[slot].macAddr, mac, 6);
        ensemble.peers[slot].currentChordIdx = chordIdx;
        ensemble.peers[slot].intensity = intensity;
        ensemble.peers[slot].dissonance = dissonance;
        ensemble.peers[slot].speed = speed;
        ensemble.peers[slot].latitude = lat; ensemble.peers[slot].longitude = lon;
        ensemble.peers[slot].currentKeyOffset = keyOffset;
        ensemble.peers[slot].listening = listening;
        ensemble.peers[slot].mood = (EnsembleMood)mood;
        ensemble.peers[slot].clashRate = clashRate;
        int macSum = 0; for(int i=0; i<6; i++) macSum += mac[i];
        ensemble.peers[slot].role = (MusicalRole)(macSum % 3);
        ensemble.peers[slot].lastSeen = millis();
        ensemble.peers[slot].active = true;
    }
    ENSEMBLE_UNLOCK();
}

void playChordProgression(const EVContext& context, int currentBaseNote) { engine.process(context, currentBaseNote); }
void playChordProgressionWithEnsemble(const EVContext& context, const EnsembleContext& ensemble, int currentBaseNote) { engine.ensemble = ensemble; engine.process(context, currentBaseNote); }
void updateEnsemblePeer(const uint8_t* mac, int chordIdx, int intensity, int dissonance, int speed, double lat, double lon, int keyOffset, bool listening, int mood, int clashRate) {
    engine.updatePeer(mac, chordIdx, intensity, dissonance, speed, lat, lon, keyOffset, listening, mood, clashRate);
}
void setLocalRole(MusicalRole role) { engine.localRole = role; }
void logEnsembleStatus() {
    ENSEMBLE_LOCK();
    if (engine.ensemble.peerCount > 0) {
        Serial.print("--- Ensemble Status ("); Serial.print(engine.ensemble.peerCount); Serial.println(" peers) ---");
        for (int i = 0; i < 4; i++) {
            if (engine.ensemble.peers[i].active) {
                Serial.print("Peer "); Serial.print(i);
                Serial.print(" Role: "); Serial.print((int)engine.ensemble.peers[i].role);
                Serial.print(" Int: "); Serial.print(engine.ensemble.peers[i].intensity);
                Serial.print(" Chord: "); Serial.println(engine.ensemble.peers[i].currentChordIdx);
            }
        }
    }
    ENSEMBLE_UNLOCK();
}
int getCurrentChordIdx() { return engine.theory.currentChordIdx; }
int getCurrentKeyOffset() { return engine.ensemble.ensembleKeyOffset; }

int getCurrentMood() { return (int)engine.theory.localMood; }
float getLocalConfidence() { return engine.awareness.confidence; }
int getLocalClashRate() { return engine.awareness.peerClashRate; }
int getLocalBoredom() { return engine.awareness.boredom; }
int getLocalDataQuality() { return engine.awareness.dataQuality; }

int predictError(int currentError) {
    static int lastError = 0;
    int prediction = currentError + (currentError - lastError) / 2;
    lastError = currentError;
    return constrain(prediction, 0, 127);
}

bool isLocalListening() { return engine.ensemble.inCallAndResponse; }

void visualFeedback(int intensity) {
#ifndef MOCK_TESTING
    if (intensity > 0) {
        bool isLeader = true;
        for(int i=0; i<4; i++) { if(engine.ensemble.peers[i].active && engine.ensemble.peers[i].intensity > intensity) { isLeader = false; break; } }
        if (engine.ensemble.peerCount > 0) {
            if (isLeader) analogWrite(2, (millis() % 200 < 100) ? 255 : 50);
            else analogWrite(2, intensity / 4);
        } else digitalWrite(2, HIGH);
    } else digitalWrite(2, LOW);
#endif
}
