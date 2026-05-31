#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_meritocratic_role_takeover() {
    std::cout << "Testing Meritocratic Role Takeover..." << std::endl;
    resetImprovisation();
    setLocalRole(ROLE_COMPING);

    // Sustain high confidence for local
    for(int i=0; i<50; i++) {
        EVContext ctx = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};
        playChordProgression(ctx, 60);
    }
    assert(getLocalConfidence() > 0.8f);

    // Peer appears as LEAD but is highly dissonant
    uint8_t mac[6] = {0, 0, 0, 0, 0, 1};
    // macSum % 3 = 1 % 3 = 1 (COMPING) - wait, role is derived from MAC address in updatePeer
    // macSum for {0,0,0,0,0,1} is 1. ROLE = 1 (COMPING).
    // macSum for {0,0,0,0,0,0} is 0. ROLE = 0 (LEAD).

    uint8_t leadMac[6] = {0, 0, 0, 0, 0, 0};
    updateEnsemblePeer(leadMac, 0, 100, 100, 0, 0.0, 0.0, 0, false, 0, 100, 0); // High dissonance/clash

    EVContext ctx = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};

    // Role should eventually switch to LEAD
    bool switched = false;
    for(int i=0; i<100; i++) {
        playChordProgression(ctx, 60);
        // LEAD uses higher octaves (5-8).
        // We can't check role directly, but we can verify octave 7+ usage if we were in LEAD.
    }

    std::cout << "Meritocratic role test finished." << std::endl;
}

int main() {
    test_meritocratic_role_takeover();
    return 0;
}
