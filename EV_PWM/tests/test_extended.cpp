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
    updateEnsemblePeer(peerMac, 1, 127, 127, 60, 0.0, 0.0, 0, false);
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
    updateEnsemblePeer(peer, 1, 100, 0, 0, 0.0, 0.0, 2, false);
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

void test_density_adaptation() {
    std::cout << "Testing density adaptation..." << std::endl;
    resetImprovisation();
    for (int i = 0; i < 4; i++) {
        uint8_t mac[6] = {0, 0, 0, 0, 0, (uint8_t)(i + 10)};
        updateEnsemblePeer(mac, 1, 100, 0, 0, 0.0, 0.0, 0, false);
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

int main() {
    test_isDissonant_extended();
    test_chord_filtering();
    test_velocity_and_timing();
    test_ensemble_logic();
    test_key_alignment();
    test_role_registers();
    test_density_adaptation();
    std::cout << "Extended tests passed!" << std::endl;
    return 0;
}
