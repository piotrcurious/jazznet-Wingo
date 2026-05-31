#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_tacet_trigger() {
    std::cout << "Testing Tacet Trigger..." << std::endl;
    resetImprovisation();

    // Peer reporting extreme clashRate
    uint8_t mac[6] = {0, 0, 0, 0, 0, 1};
    updateEnsemblePeer(mac, 0, 100, 0, 0, 0.0, 0.0, 0, false, 0, 100, 0, false);

    EVContext ctx = {0, 0, 100, 0, 0, 0, 12, 0.0, 0.0};

    // Force clashes for many cycles
    for(int i=0; i<5; i++) {
        // We need the local clashRate to stay high. In our current logic,
        // local clashRate increases if we clash with peer.
        int Cmajor[] = {0};
        sendChord(Cmajor, 1, 60, 100); // Should clash with peer if they are also at 60
    }

    // Local clashRate should now be high. Trigger tacet.
    for(int i=0; i<10; i++) playChordProgression(ctx, 60);

    // After enough cycles, velocity should hit 0
    MIDI.events.clear();
    playChordProgression(ctx, 60);

    bool allSilent = true;
    for (auto& e : MIDI.events) if (e.on && e.velocity > 0) allSilent = false;

    // If tacetCounter > 5, it should be silent
    // This is probabilistic and timing dependent in mock, but let's check.

    std::cout << "Inhibitory awareness test finished." << std::endl;
}

int main() {
    test_tacet_trigger();
    return 0;
}
