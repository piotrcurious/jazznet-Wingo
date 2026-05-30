#include "tests/mock_arduino.h"
#include "jazz_logic.h"
#include <iostream>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char** argv) {
    EVContext context = {0, 0, 0, 0};
    int base = 60;
    int dest_client = -1, dest_port = -1;
    if (argc >= 3) { dest_client = std::stoi(argv[1]); dest_port = std::stoi(argv[2]); }
    ALSAMIDIClient alsa;
    if (alsa.open("PWM MIDI Bridge")) {
        if (dest_client != -1 && dest_port != -1) alsa.connect(dest_client, dest_port);
        MIDI.alsaClient = &alsa;
    }
    set_nonblocking(0);
    char buffer[256];
    std::string line;
    initEnsembleMutex();
    while (true) {
        playChordProgression(context, base);
        while (true) {
            ssize_t n = read(0, buffer, sizeof(buffer)-1);
            if (n <= 0) break;
            buffer[n] = '\0'; line += buffer;
            size_t pos;
            while ((pos = line.find('\n')) != std::string::npos) {
                std::string cmd = line.substr(0, pos); line.erase(0, pos + 1);
                if (cmd.empty()) continue;
                try {
                    if (cmd[0] == 'e') context.error = std::stoi(cmd.substr(1));
                    else if (cmd[0] == 's') context.speed = std::stoi(cmd.substr(1));
                    else if (cmd[0] == 't') context.throttle = std::stoi(cmd.substr(1));
                    else if (cmd[0] == 'B') context.brake = std::stoi(cmd.substr(1));
                    else if (cmd[0] == 'H') context.heading = std::stoi(cmd.substr(1));
                    else if (cmd[0] == 'A') context.altitude = std::stoi(cmd.substr(1));
                    else if (cmd[0] == 'S') context.satellites = std::stoi(cmd.substr(1));
                    else if (cmd[0] == 'L') context.latitude = std::stod(cmd.substr(1));
                    else if (cmd[0] == 'O') context.longitude = std::stod(cmd.substr(1));
                    else if (cmd[0] == 'b') base = std::stoi(cmd.substr(1));
                    else if (cmd[0] == 'p') {
                        size_t first_comma = cmd.find(',');
                        if (first_comma != std::string::npos) {
                            int id = std::stoi(cmd.substr(1, first_comma - 1));
                            uint8_t mac[6] = {0, 0, 0, 0, 0, (uint8_t)id};
                            std::vector<std::string> parts;
                            std::string remaining = cmd.substr(first_comma + 1);
                            size_t p;
                            while ((p = remaining.find(',')) != std::string::npos) {
                                parts.push_back(remaining.substr(0, p));
                                remaining.erase(0, p + 1);
                            }
                            parts.push_back(remaining);
                            if (parts.size() >= 8) {
                                int chord = std::stoi(parts[0]), intensity = std::stoi(parts[1]), diss = std::stoi(parts[2]), speed = std::stoi(parts[3]);
                                double lat = std::stod(parts[4]), lon = std::stod(parts[5]);
                                int key = std::stoi(parts[6]);
                                bool list = std::stoi(parts[7]);
                                int mood = (parts.size() >= 9) ? std::stoi(parts[8]) : 0;
                                int clash = (parts.size() >= 10) ? std::stoi(parts[9]) : 0;
                                updateEnsemblePeer(mac, chord, intensity, diss, speed, lat, lon, key, list, mood, clash);
                            }
                        }
                    }
                    else if (cmd == "q") return 0;
                } catch (...) {}
            }
        }
        usleep(10000);
    }
    return 0;
}
