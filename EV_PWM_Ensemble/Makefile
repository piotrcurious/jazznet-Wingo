CXX = g++
CXXFLAGS = -I. -Itests
LDFLAGS = -ldl

all: tests/runner tests/gui_bridge tests/test_extended

tests/runner: tests/test_main.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_extended: tests/test_extended.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/gui_bridge: tests/gui_bridge.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

clean:
	rm -f tests/runner tests/gui_bridge tests/test_extended progression.mid

.PHONY: all clean
