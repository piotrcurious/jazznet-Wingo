CXX = g++
CXXFLAGS = -I. -Itests
LDFLAGS = -ldl

all: tests/runner tests/gui_bridge tests/test_extended tests/test_stress tests/test_social

tests/runner: tests/test_main.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_extended: tests/test_extended.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/gui_bridge: tests/gui_bridge.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_stress: tests/test_stress.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_social: tests/test_social.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

clean:
	rm -f tests/runner tests/gui_bridge tests/test_extended tests/test_stress tests/test_social progression.mid

.PHONY: all clean
