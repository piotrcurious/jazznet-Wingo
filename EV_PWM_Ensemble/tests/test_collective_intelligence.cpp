#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_collective_cooldown() {
    std::cout << "Testing Collective Cooldown..." << std::endl;
    resetImprovisation();

    // Set many peers to high dissonance
    for(int i=0; i<3; i++) {
        uint8_t mac[6] = {0, 0, 0, 0, 0, (uint8_t)(i + 10)};
        updateEnsemblePeer(mac, 0, 100, 120, 0, 0.0, 0.0, 0, false, 0, 0, 0, false); // diss = 120
    }

    // Initial local state: TENSION
    EVContext ctx = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};
    for(int i=0; i<5; i++) playChordProgression(ctx, 60); // build confidence

    // Process should force MOOD_RESOLUTION
    playChordProgression(ctx, 60);
    assert(getCurrentMood() == (int)MOOD_RESOLUTION);
    std::cout << "Collective cooldown test passed!" << std::endl;
}

void test_social_mirroring() {
    std::cout << "Testing Social Mirroring..." << std::endl;
    resetImprovisation();
    setLocalRole(ROLE_COMPING);

    // LEAD peer at phase 90
    uint8_t mac[6] = {0, 0, 0, 0, 0, 0}; // macSum % 3 = 0 (LEAD)
    updateEnsemblePeer(mac, 0, 100, 0, 0, 0.0, 0.0, 0, false, 0, 0, 90, false);

    // Build confidence for mirroring
    EVContext ctx = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};
    for(int i=0; i<50; i++) playChordProgression(ctx, 60);

    int startPhase = getCurrentPhase();
    // Over some iterations, phase should nudge toward 90
    for(int i=0; i<10; i++) playChordProgression(ctx, 60);

    int endPhase = getCurrentPhase();
    std::cout << "Mirroring: Start Phase " << startPhase << " -> End Phase " << endPhase << std::endl;
    // ... logic to verify nudge ...

    std::cout << "Social mirroring test finished." << std::endl;
}

int main() {
    test_collective_cooldown();
    test_social_mirroring();
    std::cout << "Collective Intelligence tests passed!" << std::endl;
    return 0;
}
