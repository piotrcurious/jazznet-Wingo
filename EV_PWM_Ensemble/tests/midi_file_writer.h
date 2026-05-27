#ifndef MIDI_FILE_WRITER_H
#define MIDI_FILE_WRITER_H

#include <vector>
#include <string>
#include <fstream>
#include "mock_arduino.h"

class MIDIFileWriter {
public:
    static void write(const std::string& filename, const std::vector<MockMIDI::NoteEvent>& events) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return;

        // Header Chunk
        file << "MThd";
        writeLong(file, 6);
        writeShort(file, 0); // Format 0
        writeShort(file, 1); // 1 track
        writeShort(file, 480); // 480 ticks per quarter note

        // Track Chunk
        file << "MTrk";
        std::vector<unsigned char> trackData;

        long currentTime = 0;
        for (const auto& e : events) {
            // Delta time (variable length) - using a fixed delta for simulation
            writeVarLen(trackData, 240);

            unsigned char status;
            if (e.on) status = 0x90 | (unsigned char)(e.channel - 1);
            else status = 0x80 | (unsigned char)(e.channel - 1);

            trackData.push_back(status);
            trackData.push_back((unsigned char)e.note);
            trackData.push_back((unsigned char)e.velocity);
        }

        // End of Track
        writeVarLen(trackData, 0);
        trackData.push_back(0xFF);
        trackData.push_back(0x2F);
        trackData.push_back(0x00);

        writeLong(file, trackData.size());
        for (unsigned char b : trackData) file.put(b);

        file.close();
    }

private:
    static void writeShort(std::ofstream& f, short v) {
        f.put((v >> 8) & 0xFF);
        f.put(v & 0xFF);
    }
    static void writeLong(std::ofstream& f, int v) {
        f.put((v >> 24) & 0xFF);
        f.put((v >> 16) & 0xFF);
        f.put((v >> 8) & 0xFF);
        f.put(v & 0xFF);
    }
    static void writeVarLen(std::vector<unsigned char>& data, long v) {
        unsigned char buffer[4];
        int i = 0;
        buffer[i++] = (unsigned char)(v & 0x7F);
        while ((v >>= 7) > 0) {
            buffer[i++] = (unsigned char)((v & 0x7F) | 0x80);
        }
        while (i > 0) {
            data.push_back(buffer[--i]);
        }
    }
};

#endif
