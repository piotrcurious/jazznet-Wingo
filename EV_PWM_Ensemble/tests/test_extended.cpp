#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_isDissonant_extended() {
    std::cout << "Testing isDissonant extended..." << std::endl;
    int context[] = {60, -1, -1, -1};
    assert(isDissonant(61, context, 1) == true);
    assert(isDissonant(62, context, 1) == false);
    std::cout << "isDissonant extended tests passed!" << std::endl;
}

void test_chord_filtering() {
    std::cout << "Testing chord filtering..." << std::endl;
    MIDI.events.clear();
    int G7[] = {7, 11, 2, 5};
    sendChord(G7, 4, 60, 100);
    int notesOn = 0;
    for (const auto& e : MIDI.events) if (e.on) notesOn++;
    assert(notesOn == 4);
    std::cout << "Chord filtering tests passed!" << std::endl;
}

void test_velocity_and_timing() {
    std::cout << "Testing velocity and timing..." << std::endl;
    MIDI.events.clear();
    EVContext ctxLow = {10, 0, 0, 0, 0, 0, 10, 0.0, 0.0};
    playChordProgression(ctxLow, 60);
    int lowVelocity = 0;
    for (auto it = MIDI.events.rbegin(); it != MIDI.events.rend(); ++it) if (it->on) { lowVelocity = it->velocity; break; }
    MIDI.events.clear();
    EVContext ctxHigh = {110, 127, 127, 0, 0, 0, 10, 0.0, 0.0};
    playChordProgression(ctxHigh, 60);
    int highVelocity = 0;
    for (auto it = MIDI.events.rbegin(); it != MIDI.events.rend(); ++it) if (it->on) { highVelocity = it->velocity; break; }
    assert(highVelocity > lowVelocity);
    std::cout << "Velocity and timing tests passed!" << std::endl;
}

void test_ensemble_logic() {
    std::cout << "Testing ensemble logic..." << std::endl;
    resetImprovisation();
    MIDI.events.clear();
    EVContext ctxSolo = {50, 0, 0, 0, 0, 0, 0, 0.0, 0.0};
    playChordProgression(ctxSolo, 60);
    int soloVelocity = 0;
    for (auto& e : MIDI.events) if (e.on) { soloVelocity = e.velocity; break; }
    const uint8_t peerMac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    updateEnsemblePeer(peerMac, 1, 127, 127, 60, 0.0, 0.0, 0, false, 0, 0);
    MIDI.events.clear();
    playChordProgression(ctxSolo, 60);
    int ensembleVelocity = 0;
    for (auto& e : MIDI.events) if (e.on) { ensembleVelocity = e.velocity; break; }
    assert(ensembleVelocity > soloVelocity);
    std::cout << "Ensemble logic tests passed!" << std::endl;
}

void test_key_alignment() {
    std::cout << "Testing key alignment..." << std::endl;
    resetImprovisation();
    const uint8_t peer[6] = {0x3, 0x3, 0x3, 0x3, 0x3, 0x3};
    updateEnsemblePeer(peer, 1, 100, 0, 0, 0.0, 0.0, 2, false, 0, 0);
    EVContext ctx = {50, 0, 0, 0, 0, 0, 0, 0.0, 0.0};
    for(int i=0; i<1000; i++) playChordProgression(ctx, 60);
    int finalKey = getCurrentKeyOffset();
    assert(finalKey > 0);
    std::cout << "Key alignment test passed!" << std::endl;
}

void test_role_registers() {
    std::cout << "Testing role registers..." << std::endl;
    setLocalRole(ROLE_BASS);
    MIDI.events.clear();
    int Cmaj7[] = {0, 4, 7, 11};
    sendChord(Cmaj7, 4, 0, 100);
    for (auto& e : MIDI.events) if (e.on) assert(e.note < 72);
    setLocalRole(ROLE_LEAD);
    MIDI.events.clear();
    sendChord(Cmaj7, 4, 0, 100);
    for (auto& e : MIDI.events) if (e.on) assert(e.note > 48);
    std::cout << "Role registers test passed!" << std::endl;
}

void test_dissonance_correction() {
    std::cout << "Testing dissonance correction..." << std::endl;
    resetImprovisation();
    setLocalRole(ROLE_COMPING);
    // Peer playing 61 (C#)
    const uint8_t peer[6] = {0x1, 0x1, 0x1, 0x1, 0x1, 0x1};
    // Chord idx 0 (Dm7: 62, 65, 69, 72) with trans -1 to make it play 61
    updateEnsemblePeer(peer, 0, 100, 0, 0, 0.0, 0.0, -1, false, 0, 0);

    MIDI.events.clear();
    // We try to play C (60), which clashes with peer 61 (pitch class 1)
    int Cmajor[] = {0};
    sendChord(Cmajor, 1, 60, 100);

    for (auto& e : MIDI.events) {
        if (e.on) {
            // Should not be 60 or 61 (which are clashing with peer 61)
            assert(e.note != 60);
            assert(e.note != 61);
        }
    }
    assert(getLocalClashRate() > 0);
    std::cout << "Dissonance correction test passed!" << std::endl;
}

void test_density_adaptation() {
    std::cout << "Testing density adaptation..." << std::endl;
    resetImprovisation();
    for (int i = 0; i < 4; i++) {
        uint8_t mac[6] = {0, 0, 0, 0, 0, (uint8_t)(i + 10)};
        updateEnsemblePeer(mac, 1, 100, 0, 0, 0.0, 0.0, 0, false, 0, 0);
    }
    EVContext ctx = {50, 0, 0, 0, 0, 0, 0, 0.0, 0.0};
    MIDI.events.clear();
    playChordProgression(ctx, 60);
    int notesOn = 0;
    for (auto& e : MIDI.events) if (e.on) notesOn++;
    // With 4 peers, maxNotesAllowed is 2
    assert(notesOn <= 4);
    std::cout << "Density adaptation test passed!" << std::endl;
}

void test_awareness_adaptation() {
    std::cout << "Testing awareness adaptation..." << std::endl;
    resetImprovisation();
    // Simulate high clash rate
    for(int i=0; i<20; i++) {
        const uint8_t peer[6] = {0x1, 0x1, 0x1, 0x1, 0x1, 0x1};
        updateEnsemblePeer(peer, 0, 100, 0, 0, 0.0, 0.0, -1, false, 0, 80);
        int Cmajor[] = {0};
        sendChord(Cmajor, 1, 60, 100);
    }
    assert(getLocalConfidence() < 0.8f);
    std::cout << "Awareness adaptation test passed!" << std::endl;
}

void test_boredom_trigger() {
    std::cout << "Testing boredom trigger..." << std::endl;
    resetImprovisation();
    EVContext ctx = {0, 0, 0, 0, 0, 0, 10, 0.0, 0.0};
    int startMood = getCurrentMood();
    // Force many repetitions of the same context
    for(int i=0; i<50; i++) {
        playChordProgression(ctx, 60);
    }
    // Boredom should have triggered at least once, potentially changing mood
    assert(getLocalBoredom() < 100);
    std::cout << "Boredom trigger test passed!" << std::endl;
}

void test_data_quality_adaptation() {
    std::cout << "Testing data quality adaptation..." << std::endl;
    resetImprovisation();
    // Poor satellites
    EVContext ctxPoor = {0, 0, 0, 0, 0, 0, 0, 0.0, 0.0};
    playChordProgression(ctxPoor, 60);
    assert(getLocalDataQuality() < 50);
    float confPoor = getLocalConfidence();

    // Good satellites
    EVContext ctxGood = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};
    playChordProgression(ctxGood, 60);
    assert(getLocalDataQuality() > 50);

    std::cout << "Data quality adaptation test passed!" << std::endl;
}

void test_confidence_dynamics() {
    std::cout << "Testing confidence dynamics..." << std::endl;
    resetImprovisation();
    // Simulate low confidence by forcing clashes
    for(int i=0; i<100; i++) {
        const uint8_t peer[6] = {0x1, 0x1, 0x1, 0x1, 0x1, 0x1};
        updateEnsemblePeer(peer, 0, 100, 0, 0, 0.0, 0.0, -1, false, 0, 100);
        int Cmajor[] = {0};
        sendChord(Cmajor, 1, 60, 100);
    }

    float confidence = getLocalConfidence();
    std::cout << "Final Confidence: " << confidence << std::endl;

    MIDI.events.clear();
    EVContext ctx = {0, 0, 100, 0, 0, 0, 12, 0.0, 0.0};
    playChordProgression(ctx, 60);

    bool foundNotes = false;
    bool foundSoft = false;
    for (auto& e : MIDI.events) if (e.on) {
        foundNotes = true;
        if (e.velocity < 100) foundSoft = true;
    }

    // If confidence is low, we expect either scaling down or muting
    if (confidence < 0.8f) {
        assert(!foundNotes || foundSoft);
    }
    std::cout << "Confidence dynamics test passed!" << std::endl;
}

int main() {
    test_isDissonant_extended();
    test_chord_filtering();
    test_velocity_and_timing();
    test_ensemble_logic();
    test_key_alignment();
    test_role_registers();
    test_density_adaptation();
    test_dissonance_correction();
    test_awareness_adaptation();
    test_boredom_trigger();
    test_data_quality_adaptation();
    test_confidence_dynamics();
    std::cout << "Extended tests passed!" << std::endl;
    return 0;
}
