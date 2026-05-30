#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <map>

void test_harmonic_entrainment() {
    std::cout << "Testing Harmonic Entrainment (ii-V to I)..." << std::endl;
    resetImprovisation();

    // Add two peers playing ii (idx 0) and V (idx 1)
    uint8_t mac1[6] = {0x1, 0, 0, 0, 0, 1};
    uint8_t mac2[6] = {0x2, 0, 0, 0, 0, 2};
    updateEnsemblePeer(mac1, 0, 100, 0, 0, 0.0, 0.0, 0, false, 0, 0); // ii
    updateEnsemblePeer(mac2, 1, 100, 0, 0, 0.0, 0.0, 0, false, 0, 0); // V

    EVContext ctx = {0, 0, 0, 0, 0, 0, 10, 0.0, 0.0};

    std::map<int, int> chordCounts;
    for(int i=0; i<1000; i++) {
        // Mock process to get next chord without playing
        TheoryPredictor theory;
        theory.currentChordIdx = 1; // Currently on V
        EnsembleContext ens;
        // Manual sync for test
        updateEnsemblePeer(mac1, 0, 100, 0, 0, 0.0, 0.0, 0, false, 0, 0);
        updateEnsemblePeer(mac2, 1, 100, 0, 0, 0.0, 0.0, 0, false, 0, 0);
        // We can't easily call theory.predict directly with engine state,
        // so we use the engine's internal theory predictor via process or similar.
        // For this test, we'll observe the engine's choice.
        playChordProgression(ctx, 60);
        chordCounts[getCurrentChordIdx()]++;
    }

    std::cout << "Chord I (idx 2) Count: " << chordCounts[2] << " / 1000" << std::endl;
    // With ii and V peers, idx 2 (I) should be highly favored
    assert(chordCounts[2] > 400);
    std::cout << "Harmonic entrainment test passed!" << std::endl;
}

void test_leadership_weighting() {
    std::cout << "Testing Leadership Weighting..." << std::endl;
    resetImprovisation();

    // Peer 1: Low intensity, wants chord IV (idx 3)
    uint8_t mac1[6] = {0x1, 0, 0, 0, 0, 1};
    updateEnsemblePeer(mac1, 3, 30, 0, 0, 0.0, 0.0, 0, false, 0, 0);

    // Peer 2: High intensity (LEADER), wants chord vi (idx 4)
    uint8_t mac2[6] = {0x2, 0, 0, 0, 0, 2};
    updateEnsemblePeer(mac2, 4, 120, 0, 0, 0.0, 0.0, 0, false, 0, 0);

    EVContext ctx = {0, 0, 0, 0, 0, 0, 10, 0.0, 0.0};

    std::map<int, int> chordCounts;
    for(int i=0; i<1000; i++) {
        playChordProgression(ctx, 60);
        chordCounts[getCurrentChordIdx()]++;
    }

    std::cout << "Leader's Chord (idx 4) Count: " << chordCounts[4] << std::endl;
    std::cout << "Follower's Chord (idx 3) Count: " << chordCounts[3] << std::endl;

    assert(chordCounts[4] > chordCounts[3]);
    std::cout << "Leadership weighting test passed!" << std::endl;
}

int main() {
    test_harmonic_entrainment();
    test_leadership_weighting();
    std::cout << "Harmonic tests passed!" << std::endl;
    return 0;
}
