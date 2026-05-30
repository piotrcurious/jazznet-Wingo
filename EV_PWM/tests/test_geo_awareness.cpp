#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_geo_weighting() {
    std::cout << "Testing Geo-Awareness Weighting..." << std::endl;
    resetImprovisation();

    // High quality: Satellites = 10
    EVContext ctxHigh = {0, 0, 0, 0, 359, 1000, 10, 0.0, 0.0}; // Heading 359, Alt 1000
    playChordProgression(ctxHigh, 60);
    int transHigh = getCurrentKeyOffset(); // Not ideal, we need to observe the actual trans offset

    // Since calculateTransposition is private, we observe via sendMIDINoteOnWrapper if we had a way
    // or we use playChordProgression and check MIDI events.

    auto get_trans_from_midi = [](const EVContext& ctx) {
        MIDI.events.clear();
        playChordProgression(ctx, 60);
        if (MIDI.events.empty()) return 0;
        return MIDI.events[0].note - 60; // relative to base C
    };

    int tHigh = get_trans_from_midi(ctxHigh);
    std::cout << "Trans with High Quality: " << tHigh << std::endl;

    // Low quality: Satellites = 0
    EVContext ctxLow = {0, 0, 0, 0, 359, 1000, 0, 0.0, 0.0};
    int tLow = get_trans_from_midi(ctxLow);
    std::cout << "Trans with Low Quality: " << tLow << std::endl;

    // Low quality should have smaller transposition from heading/altitude
    assert(abs(tLow) < abs(tHigh));

    std::cout << "Geo-awareness weighting test passed!" << std::endl;
}

int main() {
    test_geo_weighting();
    std::cout << "Geo tests passed!" << std::endl;
    return 0;
}
