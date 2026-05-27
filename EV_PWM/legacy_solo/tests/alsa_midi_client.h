#ifndef ALSA_MIDI_CLIENT_H
#define ALSA_MIDI_CLIENT_H

#include <string>
#include <vector>

// Forward declarations to avoid needing <alsa/asoundlib.h> during main compilation
// We will use dlopen/dlsym to load ALSA at runtime for maximum portability
class ALSAMIDIClient {
public:
    ALSAMIDIClient();
    ~ALSAMIDIClient();

    bool open(const std::string& clientName);
    void close();
    bool connect(int destClient, int destPort);

    void sendNoteOn(int channel, int note, int velocity);
    void sendNoteOff(int channel, int note, int velocity);

    struct PortInfo {
        int client;
        int port;
        std::string name;
    };
    std::vector<PortInfo> listPorts();

private:
    void* handle;
    void* seq;
    int port;
};

#endif
