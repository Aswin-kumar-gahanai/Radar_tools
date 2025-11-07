# Radar Data Recording & Replay: Expanded & Explained
_For Ubuntu Users_

---

## 1. Install Python Serial Library (PySerial)

PySerial allows Python to communicate over serial ports with radar modules or microcontrollers.

```bash
pip3 install pyserial
```

---

## 2. Save Your `radar_recorder_replay.py` Script

Place the script `radar_recorder_replay.py` in your working directory.
This script includes functions to record raw radar data from a real serial port or replay data via a virtual PTY port.

---

## 3. Recording Radar Data

Use this command during test or deployment to capture live radar data.

**Example**: If your radar is connected to `/dev/ttyACM1`:

```bash
python3 radar_recorder_replay.py record --port /dev/ttyACM1 --out data
```

- `--port /dev/ttyACM1`: Specify the serial port your radar module is connected to.
- `--out data`: Directory where timestamped `.bin` files will be saved.
- The program will run until manually stopped with <kbd>Ctrl</kbd>+<kbd>C</kbd>, capturing a continuous data stream.

---

## 4. Replaying Recorded Data

Use this to simulate radar output by replaying recorded data, useful for testing or visualization.

**Example**: Playback with normal speed

```bash
python3 radar_recorder_replay.py replay --binfile data/radar_2025-11-07_13-56-27.bin --speed 1.0
```

- `--binfile`: Path to the pre-recorded `.bin` file.
- `--speed`: Playback speed (`1.0` = real-time, `>1.0` = faster, `<1.0` = slower).

---

## 5. Connecting Your Parser or ROS2 Node

The replay creates a virtual serial port (e.g., `/dev/pts/6`) which appears as a normal serial port.

**Example**: To connect a ROS2 radar parser:

```bash
ros2 launch radar_parser radar_parser.launch.py serial_port:=/dev/pts/6
```

---

## 6. Compatibility & Usage Notes

- Ensure your radar device outputs data in the expected comma-separated format during recording.
  - Example printout after a successful serial connection: `"0,120"` (angle = 0°, distance = 120 cm).
- When replaying, data streams directly into the PTY, which your parser reads as a serial device.
- Adjust `baudrate` and port names as per your hardware configuration.

---

## Optional: Wrapping Commands in a Shell Script

You may want to wrap these commands into a shell script for convenience, allowing you to execute steps as needed in your development workflow.
