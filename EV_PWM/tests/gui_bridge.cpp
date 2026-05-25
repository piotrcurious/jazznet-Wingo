#include "tests/mock_arduino.h"
#include "jazz_logic.h"
#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char** argv) {
    EVContext context = {0, 0, 0, 0};
    int base = 60;

    int dest_client = -1;
    int dest_port = -1;

    // Parse destination port if provided: ./gui_bridge 128 0
    if (argc >= 3) {
        dest_client = std::stoi(argv[1]);
        dest_port = std::stoi(argv[2]);
    }

    ALSAMIDIClient alsa;
    if (alsa.open("PWM MIDI Bridge")) {
        if (dest_client != -1 && dest_port != -1) {
            if (alsa.connect(dest_client, dest_port)) {
                std::cerr << "Connected to ALSA port " << dest_client << ":" << dest_port << std::endl;
            } else {
                std::cerr << "Failed to connect to ALSA port " << dest_client << ":" << dest_port << std::endl;
            }
        }
        MIDI.alsaClient = &alsa;
    }

    set_nonblocking(0);

    char buffer[256];
    std::string line;

    while (true) {
        playChordProgression(context, base);

        while (true) {
            ssize_t n = read(0, buffer, sizeof(buffer)-1);
            if (n <= 0) break;
            buffer[n] = '\0';
            line += buffer;

            size_t pos;
            while ((pos = line.find('\n')) != std::string::npos) {
                std::string cmd = line.substr(0, pos);
                line.erase(0, pos + 1);

                if (cmd.empty()) continue;
                try {
                    if (cmd[0] == 'e') {
                        context.error = std::stoi(cmd.substr(1));
                    } else if (cmd[0] == 's') {
                        context.speed = std::stoi(cmd.substr(1));
                    } else if (cmd[0] == 't') {
                        context.throttle = std::stoi(cmd.substr(1));
                    } else if (cmd[0] == 'B') {
                        context.brake = std::stoi(cmd.substr(1));
                    } else if (cmd[0] == 'H') {
                        context.heading = std::stoi(cmd.substr(1));
                    } else if (cmd[0] == 'A') {
                        context.altitude = std::stoi(cmd.substr(1));
                    } else if (cmd[0] == 'S') {
                        context.satellites = std::stoi(cmd.substr(1));
                    } else if (cmd[0] == 'L') {
                        context.latitude = std::stod(cmd.substr(1));
                    } else if (cmd[0] == 'O') {
                        context.longitude = std::stod(cmd.substr(1));
                    } else if (cmd[0] == 'b') {
                        base = std::stoi(cmd.substr(1));
                    } else if (cmd == "q") {
                        return 0;
                    }
                } catch (...) {}
            }
        }
        usleep(10000);
    }
    return 0;
}
