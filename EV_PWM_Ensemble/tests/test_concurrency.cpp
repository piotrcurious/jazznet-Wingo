#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>

std::atomic<bool> running(true);

void peer_updater() {
    uint8_t mac[6] = {0, 0, 0, 0, 0, 1};
    int i = 0;
    while (running) {
        updateEnsemblePeer(mac, i % 6, 100, 0, 0, 0.0, 0.0, 0, false, 0, 0, 0, false);
        i++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void test_concurrency() {
    std::cout << "Testing Concurrency (Asynchronous Peer Updates)..." << std::endl;
    resetImprovisation();

    std::thread t1(peer_updater);
    std::thread t2(peer_updater);

    EVContext ctx = {0, 0, 0, 0, 0, 0, 10, 0.0, 0.0};
    for(int i=0; i<500; i++) {
        playChordProgression(ctx, 60);
    }

    running = false;
    t1.join();
    t2.join();

    std::cout << "Concurrency test passed without crashing!" << std::endl;
}

int main() {
    test_concurrency();
    return 0;
}
