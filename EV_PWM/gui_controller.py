import tkinter as tk
from tkinter import ttk, messagebox
import subprocess
import threading
import sys
import os
import re

class MIDIController:
    def __init__(self, root):
        self.root = root
        self.root.title("PWM MIDI Controller")

        # Determine path to bridge
        base_dir = os.path.dirname(os.path.abspath(__file__))
        self.bridge_path = os.path.join(base_dir, 'tests', 'gui_bridge')

        if not os.path.exists(self.bridge_path):
            messagebox.showerror("Error", f"Bridge binary not found at {self.bridge_path}.\nPlease run 'make' first.")
            sys.exit(1)

        self.bridge = None
        self.running = False
        self.available_ports = []

        self.setup_ui()
        self.refresh_ports()

    def setup_ui(self):
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # Controls
        controls_frame = ttk.LabelFrame(main_frame, text="EV & GPS Parameters", padding="10")
        controls_frame.grid(row=0, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=5)

        self.params = [
            ("Error", "e", 0, 127, 0),
            ("Speed", "s", 0, 127, 0),
            ("Throttle", "t", 0, 127, 0),
            ("Brake", "B", 0, 127, 0),
            ("Heading", "H", 0, 359, 0),
            ("Altitude", "A", 0, 2000, 0),
            ("Satellites", "S", 0, 12, 10),
            ("Base Note", "b", 48, 72, 60),
        ]

        self.scales = {}
        self.labels = {}

        for i, (name, cmd, start, end, default) in enumerate(self.params):
            ttk.Label(controls_frame, text=name).grid(row=i, column=0, sticky=tk.W)
            scale = ttk.Scale(controls_frame, from_=start, to=end, orient=tk.HORIZONTAL,
                              command=lambda v, c=cmd, n=name: self.update_param(c, n, v))
            scale.set(default)
            scale.grid(row=i, column=1, sticky=(tk.W, tk.E), padx=5)
            val_label = ttk.Label(controls_frame, text=str(default))
            val_label.grid(row=i, column=2)
            self.scales[cmd] = scale
            self.labels[name] = val_label

        # Geo Controls
        geo_frame = ttk.Frame(controls_frame)
        geo_frame.grid(row=len(self.params), column=0, columnspan=3, sticky=(tk.W, tk.E), pady=5)

        ttk.Label(geo_frame, text="Lat:").pack(side=tk.LEFT)
        self.lat_entry = ttk.Entry(geo_frame, width=10)
        self.lat_entry.insert(0, "0.0")
        self.lat_entry.pack(side=tk.LEFT, padx=5)
        self.lat_entry.bind("<Return>", lambda e: self.update_geo())

        ttk.Label(geo_frame, text="Lon:").pack(side=tk.LEFT)
        self.lon_entry = ttk.Entry(geo_frame, width=10)
        self.lon_entry.insert(0, "0.0")
        self.lon_entry.pack(side=tk.LEFT, padx=5)
        self.lon_entry.bind("<Return>", lambda e: self.update_geo())

        ttk.Button(geo_frame, text="Set Geo", command=self.update_geo).pack(side=tk.LEFT, padx=5)

        # Port Selection
        ttk.Label(main_frame, text="Output Port:").grid(row=2, column=0, sticky=tk.W)
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(main_frame, textvariable=self.port_var, state="readonly")
        self.port_combo.grid(row=2, column=1, sticky=(tk.W, tk.E))

        ttk.Button(main_frame, text="Refresh", command=self.refresh_ports).grid(row=2, column=2)

        # Connection status and Start/Stop
        self.status_var = tk.StringVar(value="Status: Disconnected")
        ttk.Label(main_frame, textvariable=self.status_var).grid(row=3, column=0, columnspan=3, pady=5)

        self.start_btn = ttk.Button(main_frame, text="Start MIDI", command=self.toggle_midi)
        self.start_btn.grid(row=4, column=0, columnspan=3, pady=5)

        ttk.Button(main_frame, text="Quit", command=self.quit).grid(row=5, column=0, columnspan=3, pady=10)

        info_frame = ttk.LabelFrame(main_frame, text="Instructions", padding="5")
        info_frame.grid(row=6, column=0, columnspan=3, pady=10, sticky=(tk.W, tk.E))

        instructions = (
            "1. Run 'make' to build the C++ logic.\n"
            "2. Ensure TiMidity is running ('timidity -iA').\n"
            "3. Select TiMidity port from dropdown.\n"
            "4. Click 'Start MIDI'."
        )
        ttk.Label(info_frame, text=instructions, justify=tk.LEFT).grid(row=0, column=0)

        # Ensemble Simulation
        ensemble_frame = ttk.LabelFrame(main_frame, text="Ensemble Simulator", padding="10")
        ensemble_frame.grid(row=0, column=3, rowspan=7, sticky=(tk.N, tk.S), padx=10)

        self.peer_data = []
        for i in range(2): # Simulate 2 peers
            p_frame = ttk.Frame(ensemble_frame, padding="5")
            p_frame.pack(fill=tk.X)
            ttk.Label(p_frame, text=f"Peer {i+1} Intensity").pack()
            s = ttk.Scale(p_frame, from_=0, to=127, orient=tk.HORIZONTAL,
                          command=lambda v, idx=i: self.update_peer(idx))
            s.set(80)
            s.pack(fill=tk.X)
            self.peer_data.append(s)

    def update_peer(self, idx):
        if not self.running: return
        val = int(float(self.peer_data[idx].get()))
        # Send simulated peer data: p[id],[chord],[int],[diss],[speed],[lat],[lon],[key],[list]
        self.send_cmd(f"p{idx+1},", f"1,{val},0,0,0.0,0.0,0,0")

    def refresh_ports(self):
        self.available_ports = []
        try:
            output = subprocess.check_output(['aconnect', '-o'], text=True)
            current_client = None
            for line in output.split('\n'):
                client_match = re.search(r'client (\d+): \'(.*?)\'', line)
                if client_match:
                    current_client = (client_match.group(1), client_match.group(2))
                    continue

                port_match = re.search(r'^\s+(\d+) \'(.*?)\'', line)
                if port_match and current_client:
                    port_id = port_match.group(1)
                    port_name = port_match.group(2)
                    display_name = f"{current_client[0]}:{port_id} ({current_client[1]} - {port_name})"
                    self.available_ports.append(display_name)
        except:
            pass

        if not self.available_ports:
            self.available_ports = ["128:0 (TiMidity default)"]

        self.port_combo['values'] = self.available_ports
        if self.available_ports:
            idx = 0
            for i, p in enumerate(self.available_ports):
                if "TiMidity" in p:
                    idx = i
                    break
            self.port_combo.current(idx)

    def toggle_midi(self):
        if not self.running:
            self.start_midi()
        else:
            self.stop_midi()

    def start_midi(self):
        try:
            # Extract port if possible
            port_str = self.port_var.get()
            m = re.match(r'(\d+):(\d+)', port_str)
            args = [self.bridge_path]
            if m:
                args.extend([m.group(1), m.group(2)])

            # Start Bridge
            self.bridge = subprocess.Popen(
                args,
                stdin=subprocess.PIPE,
                stdout=sys.stdout,
                stderr=sys.stderr,
                text=True,
                bufsize=1
            )

            self.running = True
            self.start_btn.config(text="Stop MIDI")
            self.status_var.set(f"Status: Running (Port {port_str})")

            # Sync initial values
            for _, cmd, _, _, _ in self.params:
                self.send_cmd(cmd, self.scales[cmd].get())
            self.update_geo()

        except Exception as e:
            messagebox.showerror("Error", f"Failed to start MIDI: {e}")
            self.stop_midi()

    def stop_midi(self):
        self.running = False
        if self.bridge:
            try:
                self.bridge.stdin.write("q\n")
                self.bridge.stdin.flush()
            except: pass
            self.bridge.terminate()

        self.bridge = None
        self.start_btn.config(text="Start MIDI")
        self.status_var.set("Status: Disconnected")

    def update_param(self, cmd, name, val):
        v = int(float(val))
        self.labels[name].config(text=str(v))
        self.send_cmd(cmd, v)

    def update_geo(self):
        try:
            lat = float(self.lat_entry.get())
            lon = float(self.lon_entry.get())
            self.send_cmd("L", lat)
            self.send_cmd("O", lon)
        except ValueError:
            pass

    def send_cmd(self, cmd, val):
        if self.bridge and self.bridge.stdin:
            try:
                self.bridge.stdin.write(f"{cmd}{val}\n")
                self.bridge.stdin.flush()
            except: pass

    def quit(self):
        self.stop_midi()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = MIDIController(root)
    root.protocol("WM_DELETE_WINDOW", app.quit)
    root.mainloop()
