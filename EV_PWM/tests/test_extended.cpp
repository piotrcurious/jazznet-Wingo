#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_isDissonant_extended() {
    std::cout << "Testing isDissonant extended..." << std::endl;

    // Test basic intervals
    int context[] = {60, -1, -1, -1};
    assert(isDissonant(61, context, 1) == true);  // Minor 2nd
    assert(isDissonant(62, context, 1) == false); // Major 2nd - OK now
    assert(isDissonant(63, context, 1) == false); // Minor 3rd
    assert(isDissonant(64, context, 1) == false); // Major 3rd
    assert(isDissonant(65, context, 1) == false); // Perfect 4th
    assert(isDissonant(66, context, 1) == false); // Tritone - OK now
    assert(isDissonant(67, context, 1) == false); // Perfect 5th
    assert(isDissonant(68, context, 1) == false); // Minor 6th
    assert(isDissonant(69, context, 1) == false); // Major 6th
    assert(isDissonant(70, context, 1) == false); // Minor 7th - OK now
    assert(isDissonant(71, context, 1) == false); // Major 7th

    std::cout << "isDissonant extended tests passed!" << std::endl;
}

void test_chord_filtering() {
    std::cout << "Testing chord filtering..." << std::endl;

    // Mock MIDI should clear events
    MIDI.events.clear();

    // G7 chord: G, B, D, F
    int G7[] = {7, 11, 2, 5}; // Using base notes to test voice leading
    sendChord(G7, 4, 60); // Base at 60 (Middle C)

    // Check how many notes were played
    int notesOn = 0;
    for (const auto& e : MIDI.events) {
        if (e.on) notesOn++;
    }

    std::cout << "Notes played for G7: " << notesOn << std::endl;
    // All 4 notes should be played because we relaxed isDissonant
    assert(notesOn == 4);

    std::cout << "Chord filtering tests passed!" << std::endl;
}

void test_velocity_and_timing() {
    std::cout << "Testing velocity and timing..." << std::endl;

    MIDI.events.clear();
    // Low error, no throttle, no brake = Low velocity
    EVContext ctxLow = {10, 0, 0, 0, 0, 0, 10, 0.0, 0.0};
    playChordProgression(ctxLow, 60);
    int lowVelocity = 0;
    for (auto it = MIDI.events.rbegin(); it != MIDI.events.rend(); ++it) {
        if (it->on) {
            lowVelocity = it->velocity;
            break;
        }
    }

    MIDI.events.clear();
    // High error, high throttle, no brake = High velocity
    EVContext ctxHigh = {110, 127, 127, 0, 0, 0, 10, 0.0, 0.0};
    playChordProgression(ctxHigh, 60);
    int highVelocity = 0;
    for (auto it = MIDI.events.rbegin(); it != MIDI.events.rend(); ++it) {
        if (it->on) {
            highVelocity = it->velocity;
            break;
        }
    }

    std::cout << "Low Context Velocity: " << lowVelocity << ", High Context Velocity: " << highVelocity << std::endl;
    assert(highVelocity > lowVelocity);

    MIDI.events.clear();
    // High error, high throttle, HIGH BRAKE = Lowered velocity
    EVContext ctxBrake = {110, 127, 127, 127, 0, 0, 10, 0.0, 0.0};
    playChordProgression(ctxBrake, 60);
    int brakeVelocity = 0;
    for (auto it = MIDI.events.rbegin(); it != MIDI.events.rend(); ++it) {
        if (it->on) {
            brakeVelocity = it->velocity;
            break;
        }
    }
    std::cout << "High Context Velocity: " << highVelocity << ", Brake Velocity: " << brakeVelocity << std::endl;
    assert(brakeVelocity < highVelocity);

    std::cout << "Velocity and timing tests passed!" << std::endl;
}

void test_voice_leading() {
    std::cout << "Testing voice leading..." << std::endl;

    MIDI.events.clear();
    int CMaj7[] = {0, 4, 7, 11};
    sendChord(CMaj7, 4, 48, 100); // Low octave

    int firstChordAvg = 0;
    int notesCount = 0;
    for (const auto& e : MIDI.events) {
        if (e.on) {
            firstChordAvg += e.note;
            notesCount++;
        }
    }
    firstChordAvg /= notesCount;

    MIDI.events.clear();
    int FMaj7[] = {5, 9, 0, 4};
    sendChord(FMaj7, 4, 72, 100); // High octave transposition

    int secondChordAvg = 0;
    notesCount = 0;
    for (const auto& e : MIDI.events) {
        if (e.on) {
            secondChordAvg += e.note;
            notesCount++;
        }
    }
    secondChordAvg /= notesCount;

    std::cout << "First chord avg: " << firstChordAvg << ", Second chord avg: " << secondChordAvg << std::endl;
    // Even with high transposition, the second chord should stay close to the first one due to voice leading octave selection
    assert(abs(secondChordAvg - firstChordAvg) < 12);

    std::cout << "Voice leading tests passed!" << std::endl;
}

void test_gps_influence() {
    std::cout << "Testing GPS influence..." << std::endl;

    // Altitude test
    resetImprovisation();
    MIDI.events.clear();
    // Use fixed error to avoid random Markov transitions during altitude test
    // Satellites < 6 to avoid Jazznet loading during GPS influence tests
    EVContext ctxSeaLevel = {0, 0, 0, 0, 0, 0, 0, 0.0, 0.0};
    playChordProgression(ctxSeaLevel, 60);
    int seaLevelNote = 0;
    for (const auto& e : MIDI.events) if (e.on) { seaLevelNote = e.note; break; }

    resetImprovisation(); // Reset to ensure same chord choice
    MIDI.events.clear();
    EVContext ctxMountain = {0, 0, 0, 0, 0, 1000, 0, 0.0, 0.0}; // 1000m high
    playChordProgression(ctxMountain, 60);
    int mountainNote = 0;
    for (const auto& e : MIDI.events) if (e.on) { mountainNote = e.note; break; }

    std::cout << "Sea Level Note: " << seaLevelNote << ", Mountain Note: " << mountainNote << std::endl;
    assert(mountainNote > seaLevelNote);

    // Heading test
    resetImprovisation();
    MIDI.events.clear();
    EVContext ctxNorth = {50, 0, 0, 0, 0, 0, 0, 0.0, 0.0};
    playChordProgression(ctxNorth, 60);
    int northNote = MIDI.events.front().note;

    resetImprovisation();
    MIDI.events.clear();
    EVContext ctxEast = {50, 0, 0, 0, 90, 0, 0, 0.0, 0.0};
    playChordProgression(ctxEast, 60);
    int eastNote = MIDI.events.front().note;

    std::cout << "North Note: " << northNote << ", East Note: " << eastNote << std::endl;
    assert(northNote != eastNote);

    std::cout << "GPS influence tests passed!" << std::endl;
}

void test_novelty_variation() {
    std::cout << "Testing Novelty Variation..." << std::endl;

    // Low error = Low novelty (familiarity)
    EVContext ctxLow = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};
    MIDI.events.clear();
    resetImprovisation();
    playChordProgression(ctxLow, 60);

    std::vector<int> lowNotes;
    for (const auto& e : MIDI.events) if (e.on) lowNotes.push_back(e.note % 12);

    // High error = High novelty (variation)
    EVContext ctxHigh = {127, 0, 0, 0, 0, 0, 12, 0.0, 0.0};
    MIDI.events.clear();
    resetImprovisation();
    playChordProgression(ctxHigh, 60);

    std::vector<int> highNotes;
    for (const auto& e : MIDI.events) if (e.on) highNotes.push_back(e.note % 12);

    std::cout << "Low Novelty Notes: "; for(int n : lowNotes) std::cout << n << " "; std::cout << std::endl;
    std::cout << "High Novelty Notes: "; for(int n : highNotes) std::cout << n << " "; std::cout << std::endl;

    // In high novelty, we expect some notes that are NOT in the standard chords (0,2,4,5,7,9,11 ...)
    // Standard I chord: 0, 4, 7, 11. Novelty might add 2, 5, 9.
    // This is hard to assert deterministically due to random(), but we can check if they differ.
    assert(lowNotes != highNotes);

    std::cout << "Novelty variation tests passed!" << std::endl;
}

void test_jazznet_loading() {
    std::cout << "Testing Jazznet loading..." << std::endl;

    for (int i = 0; i < 100; ++i) {
        EVContext ctx = {50, 0, 0, 0, 0, 0, 10, (double)i / 100.0, 0.0}; // GeoSeed i
        MIDI.events.clear();
        resetImprovisation();
        playChordProgression(ctx, 60);

        assert(MIDI.events.size() > 0);
    }

    std::cout << "Jazznet loading 100 files tests passed!" << std::endl;
}

void test_arpeggiation_trigger() {
    std::cout << "Testing arpeggiation trigger..." << std::endl;

    // Normal error = Block chord (multiple NoteOn in same tick/sequence)
    EVContext ctxLow = {30, 0, 0, 0, 0, 0, 0, 0.0, 0.0}; // No satellites to avoid jazznet
    MIDI.events.clear();
    playChordProgression(ctxLow, 60);

    // Low error should result in block chords (many NoteOn before NoteOff)
    int concurrentMax = 0;
    int current = 0;
    for(auto& e : MIDI.events) {
        if(e.on) current++;
        else current--;
        if(current > concurrentMax) concurrentMax = current;
    }
    std::cout << "Max concurrent notes (Low Error): " << concurrentMax << std::endl;
    assert(concurrentMax > 1);

    // High error = Arpeggio (NoteOn, delay, NoteOff, NoteOn ...)
    EVContext ctxHigh = {120, 0, 0, 0, 0, 0, 0, 0.0, 0.0};
    MIDI.events.clear();
    playChordProgression(ctxHigh, 60);

    // In arpeggio mode, concurrent notes should be 1
    concurrentMax = 0;
    current = 0;
    for(auto& e : MIDI.events) {
        if(e.on) current++;
        else current--;
        if(current > concurrentMax) concurrentMax = current;
    }
    std::cout << "Max concurrent notes (High Error): " << concurrentMax << std::endl;
    assert(concurrentMax == 1);

    std::cout << "Arpeggiation trigger tests passed!" << std::endl;
}

void test_markov_transitions() {
    std::cout << "Testing Markov transitions..." << std::endl;

    // We want to see if chords actually change over multiple calls
    EVContext ctx = {50, 0, 0, 0, 0, 0, 10, 0.0, 0.0};

    int transitionCount = 0;
    int lastNote = -1;

    for (int i = 0; i < 20; ++i) {
        MIDI.events.clear();
        // Satellites 0 to avoid Jazznet overriding Markov transitions
        EVContext ctxMarkov = {50, 0, 0, 0, 0, 0, 0, 0.0, 0.0};
        playChordProgression(ctxMarkov, 60);
        int currentNote = MIDI.events.front().note;
        if (lastNote != -1 && currentNote != lastNote) {
            transitionCount++;
        }
        lastNote = currentNote;
    }

    std::cout << "Unique transitions in 20 steps: " << transitionCount << std::endl;
    // With Markov, we expect at least some transitions
    assert(transitionCount > 0);

    std::cout << "Markov transition tests passed!" << std::endl;
}

int main() {
    test_isDissonant_extended();
    test_chord_filtering();
    test_velocity_and_timing();
    test_voice_leading();
    test_gps_influence();
    test_markov_transitions();
    test_jazznet_loading();
    test_arpeggiation_trigger();
    test_novelty_variation();
    std::cout << "Extended tests passed!" << std::endl;
    return 0;
}
