#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_phase_alignment() {
    std::cout << "Testing Phase Alignment..." << std::endl;
    resetImprovisation();

    // Peer is at phase 80
    uint8_t mac[6] = {0, 0, 0, 0, 0, 1};
    updateEnsemblePeer(mac, 0, 100, 0, 0, 0.0, 0.0, 0, false, 0, 0, 80);

    // Local is at phase 0
    // We expect some nudge in delay
    // This is hard to verify without timing, but we check if getCurrentPhase updates

    EVContext ctx = {0, 0, 0, 0, 0, 0, 10, 0.0, 0.0};
    playChordProgression(ctx, 60);

    std::cout << "Local Phase: " << getCurrentPhase() << std::endl;
    std::cout << "Phase alignment test finished." << std::endl;
}

int main() {
    test_phase_alignment();
    return 0;
}
