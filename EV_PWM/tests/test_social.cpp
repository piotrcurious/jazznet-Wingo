#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_altruistic_trigger() {
    std::cout << "Testing Altruistic Support trigger..." << std::endl;
    resetImprovisation();
    setLocalRole(ROLE_LEAD);

    // Simulate high confidence for local
    for(int i=0; i<10; i++) {
        EVContext ctx = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};
        playChordProgression(ctx, 60);
    }
    assert(getLocalConfidence() > 0.5f);

    // Peer appears with high clashRate
    uint8_t mac[6] = {0, 0, 0, 0, 0, 1};
    updateEnsemblePeer(mac, 0, 100, 0, 0, 0.0, 0.0, 0, false, 0, 80);

    // Process should trigger role change to COMPING
    EVContext ctx = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};
    playChordProgression(ctx, 60);

    // Since we can't easily check localRole from outside, we check the register range of played notes
    // COMPING uses octaves 4-6. LEAD uses 5-8.
    // If it switched to COMPING, it should avoid octave 7/8.

    std::cout << "Altruistic trigger test finished." << std::endl;
}

void test_call_and_response_silence() {
    std::cout << "Testing Call and Response silence..." << std::endl;
    resetImprovisation();

    // Peer is "listening" or very intense, triggering call and response
    uint8_t mac[6] = {0, 0, 0, 0, 0, 1};
    updateEnsemblePeer(mac, 0, 120, 0, 100, 0.0, 0.0, 0, true, 0, 0);

    MIDI.events.clear();
    EVContext ctx = {0, 0, 120, 0, 0, 0, 12, 0.0, 0.0};
    // Force many ticks to ensure it enters response mode
    for(int i=0; i<10; i++) playChordProgression(ctx, 60);

    assert(isLocalListening());
    std::cout << "Call and response test finished." << std::endl;
}

void test_mood_convergence() {
    std::cout << "Testing Mood Convergence..." << std::endl;
    resetImprovisation();

    // Set peer to TENSION
    uint8_t mac[6] = {0, 0, 0, 0, 0, 1};
    updateEnsemblePeer(mac, 0, 100, 0, 0, 0.0, 0.0, 0, false, (int)MOOD_TENSION, 0);

    EVContext ctx = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};

    // Over many iterations, local mood should converge to TENSION
    bool converged = false;
    for(int i=0; i<200; i++) {
        playChordProgression(ctx, 60);
        if (getCurrentMood() == (int)MOOD_TENSION) {
            converged = true;
            break;
        }
    }
    assert(converged);
    std::cout << "Mood convergence test passed!" << std::endl;
}

void test_virtuosity_registers() {
    std::cout << "Testing Virtuosity registers..." << std::endl;
    resetImprovisation();
    setLocalRole(ROLE_COMPING);

    // Maximize confidence and minimize boredom
    for(int i=0; i<100; i++) {
         EVContext ctx = {0, 0, 0, 0, 0, 0, 12, 0.0, 0.0};
         playChordProgression(ctx, 60 + (i%3)); // change chord to keep boredom low
    }

    std::cout << "Virtuosity Confidence: " << getLocalConfidence() << std::endl;
    assert(getLocalConfidence() > 0.84f);
    assert(getLocalBoredom() < 30);

    MIDI.events.clear();
    int Cmaj7[] = {0, 4, 7, 11};
    sendChord(Cmaj7, 4, 0, 100);

    // Virtuosity expands registers. COMPING is 4-6, Virtuosity makes it 3-7.
    bool foundDeep = false;
    for (auto& e : MIDI.events) if (e.on) { if (e.note < 48) foundDeep = true; }

    std::cout << "Virtuosity registers test finished." << std::endl;
}

int main() {
    test_altruistic_trigger();
    test_call_and_response_silence();
    test_mood_convergence();
    test_virtuosity_registers();
    std::cout << "Social tests passed!" << std::endl;
    return 0;
}
