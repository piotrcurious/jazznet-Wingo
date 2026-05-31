CXX = g++
CXXFLAGS = -I. -Itests
LDFLAGS = -ldl

all: tests/runner tests/gui_bridge tests/test_extended tests/test_stress tests/test_social tests/test_harmony tests/test_ensemble_dynamics tests/test_geo_awareness tests/test_concurrency tests/test_synchronization tests/test_role_dynamics tests/test_collective_intelligence tests/test_communication_awareness tests/test_dialogue_mode tests/test_auditory_masking tests/test_inhibitory_awareness

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

tests/test_harmony: tests/test_harmony.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_ensemble_dynamics: tests/test_ensemble_dynamics.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_geo_awareness: tests/test_geo_awareness.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_concurrency: tests/test_concurrency.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_synchronization: tests/test_synchronization.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_role_dynamics: tests/test_role_dynamics.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_collective_intelligence: tests/test_collective_intelligence.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_communication_awareness: tests/test_communication_awareness.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_dialogue_mode: tests/test_dialogue_mode.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_auditory_masking: tests/test_auditory_masking.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

tests/test_inhibitory_awareness: tests/test_inhibitory_awareness.cpp jazz_logic.cpp tests/alsa_midi_client.cpp
	$(CXX) $(CXXFLAGS) -DMOCK_TESTING -o $@ $^ $(LDFLAGS)

test: all
	./tests/run_all_tests.sh

clean:
	rm -f tests/runner tests/gui_bridge tests/test_extended tests/test_stress tests/test_social tests/test_harmony tests/test_ensemble_dynamics tests/test_geo_awareness tests/test_concurrency tests/test_synchronization tests/test_role_dynamics tests/test_collective_intelligence tests/test_communication_awareness tests/test_dialogue_mode tests/test_auditory_masking tests/test_inhibitory_awareness progression.mid

.PHONY: all clean test
