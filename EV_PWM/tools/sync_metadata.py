import os
import subprocess
import mido
import random
import shutil
import csv
import sys

# Configuration
# All paths relative to this script's directory for robustness
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
METADATA_CSV = os.path.join(SCRIPT_DIR, "../../metadata/full.csv")
OUTPUT_DIR = os.path.join(SCRIPT_DIR, "../jazznet")
PG_DIR = os.path.join(SCRIPT_DIR, "../../PatternGenerator")
MAX_PATTERNS = 100

def check_dependencies():
    try:
        import mido
        from midiutil import MIDIFile
    except ImportError as e:
        print(f"Error: Missing dependency: {e.name}")
        print("Please install required packages: pip install mido MIDIUtil")
        sys.exit(1)

def get_metadata_samples():
    if not os.path.exists(METADATA_CSV):
        print(f"Error: Metadata file {METADATA_CSV} not found.")
        return []

    samples = []
    with open(METADATA_CSV, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            samples.append(row)

    random.shuffle(samples)
    return samples

def midi_to_csv(midi_path):
    try:
        mid = mido.MidiFile(midi_path)
        notes = []
        for msg in mid:
            if msg.type == 'note_on' and msg.velocity > 0:
                notes.append(str(msg.note))
        return ",".join(notes)
    except Exception:
        return None

def generate_pattern_from_metadata(sample, temp_cwd):
    p_type = sample['type']
    p_mode = sample['mode']
    p_name = sample['name']

    cmd = ["python3"]
    if p_type == "chord" or p_type == "arpeggio":
        script = os.path.join(PG_DIR, "generateChords.py")
        style = "chords" if p_type == "chord" else "arpeggios"
        offsets = "4 3 4" if "7" in p_mode else "4 3"
        if "min" in p_mode: offsets = "3 4 3" if "7" in p_mode else "3 4"
        cmd += [script, "-t", style, "-l", "tetrad" if "7" in p_mode else "triad", "-o", offsets, "-n", p_name]
    elif p_type == "progression":
        script = os.path.join(PG_DIR, "generateProgressions.py")
        prog = "ii-V-I" if "ii-V-I" in p_mode else "I-VI-ii-V"
        cmd += [script, "-p", prog, "-n", p_name]
    else: # scale
        script = os.path.join(PG_DIR, "generateScales.py")
        offsets = "2 2 1 2 2 2"
        if p_mode == "dorian": offsets = "2 1 2 2 2 1"
        elif p_mode == "aeolian": offsets = "2 1 2 2 1 2 2"
        cmd += [script, "-o", offsets, "-n", p_name]

    try:
        # Run in the temporary directory to avoid polluting the repo
        subprocess.run(cmd, check=True, capture_output=True, cwd=temp_cwd)
        return True
    except subprocess.CalledProcessError:
        return False

def sync():
    check_dependencies()
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    samples = get_metadata_samples()

    print(f"Syncing up to {MAX_PATTERNS} patterns from full metadata...")

    temp_dir = os.path.join(SCRIPT_DIR, "temp_gen")
    if os.path.exists(temp_dir):
        shutil.rmtree(temp_dir)
    os.makedirs(temp_dir)

    count = 0
    for sample in samples:
        if count >= MAX_PATTERNS:
            break

        if generate_pattern_from_metadata(sample, temp_dir):
            patterns_path = os.path.join(temp_dir, "patterns")
            if os.path.exists(patterns_path):
                for root, _, files in os.walk(patterns_path):
                    for f in files:
                        if f.endswith(".mid"):
                            midi_path = os.path.join(root, f)
                            csv_content = midi_to_csv(midi_path)
                            if csv_content:
                                with open(os.path.join(OUTPUT_DIR, f"P{count}.CSV"), "w") as out:
                                    out.write(csv_content + "\n")
                                count += 1
                                break
                    if count >= MAX_PATTERNS: break
                shutil.rmtree(patterns_path)

    shutil.rmtree(temp_dir)
    print(f"Successfully synced {count} patterns to {OUTPUT_DIR}")

if __name__ == "__main__":
    sync()
