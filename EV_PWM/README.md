# PWM_midi
PWM control but using MIDI. Dreamed by chatGPT 4o

Basic sketch of an idea to overlay some pleasant sound f.e. over whine of EV motor PWM.

### Advanced Improvisation Logic
The system uses a combination of:
- **Markov Chain Transitions**: For harmonic variety, influenced by vehicle telemetry.
- **Jazznet Patterns**: Loaded from SD card based on GPS coordinates and vehicle speed. Patterns are generated from high-quality metadata.
- **Variation Engine**: Dynamically varies patterns based on vehicle telemetry (Novelty Factor).
- **Voice Leading**: Smooth transitions between registers.
- **Context Mapping**: Speed, throttle, brake, and GPS data (altitude, heading, satellites) drive the musical performance.

### Generating Patterns
To generate the musical patterns used by the system (Jazznet), ensure you have `mido` and `MIDIUtil` installed:
```bash
pip install mido MIDIUtil
```
Then run:
```bash
cd EV_PWM
make patterns
```
This will populate the `jazznet/` directory with 100 CSV patterns derived from the project's metadata.
