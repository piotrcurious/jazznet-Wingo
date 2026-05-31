#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_dialogue_alternation() {
    std::cout << "Testing Dialogue Alternation..." << std::endl;
    resetImprovisation();

    // Exactly 1 peer
    uint8_t mac[6] = {0, 0, 0, 0, 0, 1};
    updateEnsemblePeer(mac, 0, 100, 0, 0, 0.0, 0.0, 0, false, 0, 0, 0, false);

    // Build confidence
    EVContext ctx = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};
    for(int i=0; i<10; i++) playChordProgression(ctx, 60);

    bool initialSolo = isLocalSoloing();
    // Run many cycles to see if soloing status flips
    bool flipped = false;
    for(int i=0; i<1000; i++) {
        playChordProgression(ctx, 60);
        if (isLocalSoloing() != initialSolo) {
            flipped = true;
            break;
        }
    }

    assert(flipped);
    std::cout << "Dialogue alternation test passed!" << std::endl;
}

int main() {
    test_dialogue_alternation();
    return 0;
}
