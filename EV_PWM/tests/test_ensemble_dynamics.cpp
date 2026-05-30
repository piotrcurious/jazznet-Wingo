#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_mutual_confidence() {
    std::cout << "Testing Mutual Confidence..." << std::endl;
    resetImprovisation();

    // Simulate high confidence first
    for(int i=0; i<50; i++) {
        EVContext ctx = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};
        playChordProgression(ctx, 60);
    }
    float highConf = getLocalConfidence();
    assert(highConf > 0.8f);

    // Peer appears reporting high clashRate
    uint8_t mac[6] = {0, 0, 0, 0, 0, 1};
    updateEnsemblePeer(mac, 0, 100, 0, 0, 0.0, 0.0, 0, false, 0, 90);

    for(int i=0; i<20; i++) {
        EVContext ctx = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};
        playChordProgression(ctx, 60);
    }

    float lowConf = getLocalConfidence();
    std::cout << "Mutual Low Confidence: " << lowConf << std::endl;
    assert(lowConf < highConf);
    std::cout << "Mutual confidence test passed!" << std::endl;
}

void test_flow_state_trigger() {
    std::cout << "Testing Flow State trigger..." << std::endl;
    resetImprovisation();
    setLocalRole(ROLE_COMPING);

    // Sustain high confidence
    for(int i=0; i<200; i++) {
        EVContext ctx = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};
        playChordProgression(ctx, 60 + (i%3));
    }

    // Flow state should be high
    // We can't check flowState directly without accessor, but we check registers
    std::cout << "Flow Confidence: " << getLocalConfidence() << std::endl;
    MIDI.events.clear();
    int Cmaj7[] = {0, 4, 7, 11};
    sendChord(Cmaj7, 4, 0, 100);

    bool foundDeep = false;
    for (auto& e : MIDI.events) if (e.on) { if (e.note < 48) foundDeep = true; }
    assert(foundDeep);

    std::cout << "Flow state trigger test passed!" << std::endl;
}

int main() {
    test_mutual_confidence();
    test_flow_state_trigger();
    std::cout << "Ensemble dynamics tests passed!" << std::endl;
    return 0;
}
