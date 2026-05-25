#include "jazz_logic.h"
#include "midi_file_writer.h"
#include <cassert>
#include <iostream>
#include <cstring>

void test_isDissonant() {
    std::cout << "Testing isDissonant..." << std::endl;
    int context[] = {60, -1, -1, -1}; // C
    assert(isDissonant(61, context, 1) == true);
    assert(isDissonant(62, context, 1) == false); // Now allowed
    assert(isDissonant(64, context, 1) == false);
    assert(isDissonant(66, context, 1) == false); // Now allowed
    assert(isDissonant(70, context, 1) == false); // Now allowed
    std::cout << "isDissonant tests passed!" << std::endl;
}

void test_predictError() {
    std::cout << "Testing predictError..." << std::endl;
    predictError(50);
    predictError(60);
    int pred = predictError(70);
    assert(pred >= 70 && pred <= 75);
    std::cout << "predictError tests passed!" << std::endl;
}

void run_progression_recording(const std::string& filename) {
    std::cout << "Recording progression to " << filename << "..." << std::endl;
    MIDI.events.clear();
    for(int e=0; e<128; e += 20) {
        EVContext ctx = {e, 0, 0, 0, 0, 0, 10, 0.0, 0.0};
        playChordProgression(ctx, 60); // C
        playChordProgression(ctx, 65); // F
        playChordProgression(ctx, 67); // G
    }
    MIDIFileWriter::write(filename, MIDI.events);
    std::cout << "Recording complete." << std::endl;
}

int main(int argc, char** argv) {
    if (argc > 1 && strcmp(argv[1], "--live") == 0) {
        MIDI.liveMode = true;
        for(int i=0; i<100; i++) {
            EVContext ctx = {rand()%128, rand()%128, rand()%128, rand()%128, rand()%360, rand()%2000, 10, 0.0, 0.0};
            playChordProgression(ctx, 60 + (rand()%12));
        }
        return 0;
    }

    test_isDissonant();
    test_predictError();
    run_progression_recording("progression.mid");

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
