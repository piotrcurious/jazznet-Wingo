CXX = g++
CXXFLAGS = -I. -Itests
LDFLAGS = -ldl

all: patterns tests/runner tests/gui_bridge tests/test_extended

patterns:
	python3 tools/sync_metadata.py

tests/runner: tests/test_main.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_extended: tests/test_extended.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/gui_bridge: tests/gui_bridge.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

clean:
	rm -f tests/runner tests/gui_bridge tests/test_extended progression.mid
	rm -rf jazznet/*.CSV

.PHONY: all clean patterns
