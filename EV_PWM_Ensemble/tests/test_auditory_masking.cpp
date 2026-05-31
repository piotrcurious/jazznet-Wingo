#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_masking_velocity_reduction() {
    std::cout << "Testing Auditory Masking Velocity Reduction..." << std::endl;
    resetImprovisation();
    setLocalRole(ROLE_COMPING);

    // High confidence peer in same role (COMPING) with high intensity
    uint8_t mac[6] = {0, 0, 0, 0, 0, 1}; // macSum % 3 = 1 (COMPING)
    updateEnsemblePeer(mac, 0, 120, 0, 0, 0.0, 0.0, 0, false, 0, 0, 0, false);

    // Local: lower intensity, lower confidence
    for(int i=0; i<10; i++) {
        EVContext ctx = {0, 0, 80, 0, 0, 0, 12, 0.0, 0.0};
        playChordProgression(ctx, 60);
    }

    MIDI.events.clear();
    EVContext ctx = {0, 0, 80, 0, 0, 0, 12, 0.0, 0.0};
    playChordProgression(ctx, 60);

    bool foundSoft = false;
    for (auto& e : MIDI.events) if (e.on) { if (e.velocity < 60) foundSoft = true; }

    // Expected to be soft due to masking avoidance
    assert(foundSoft);
    std::cout << "Auditory masking test passed!" << std::endl;
}

int main() {
    test_masking_velocity_reduction();
    return 0;
}
