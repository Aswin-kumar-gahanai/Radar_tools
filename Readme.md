# Radar Data Recording & Replay: Expanded & Explained
# for Ubuntu Users (Robotics & Sensor Integration Focus)

# Step 1: Install Python Serial Library (PySerial)
# PySerial allows Python to communicate over serial ports with radar modules or microcontrollers.
pip3 install pyserial

# Step 2: Save your 'radar_io.py' script in your working directory.
# This script includes functions to record raw radar data from a real serial port or replay data via a virtual PTY port.

# 3. Recording Radar Data
# Use this command during test or deployment to capture live radar data.
# Example: if your radar is connected to /dev/ttyACM1
python3 radar_recorder_replay.py record --port /dev/ttyACM1 --out data

# - `--port /dev/ttyACM1`: Specify the serial port your radar module is connected to.
# - `--out data`: Directory where timestamped `.bin` files will be saved.
# - The program will run until manually stopped with Ctrl+C, capturing continuous data stream.

# 4. Replaying Recorded Data
# Use this to simulate radar output by replaying recorded data, useful for testing or visualization.
# Example: playback with normal speed
python3 radar_recorder_replay.py replay --binfile data/radar_2025-11-07_13-56-27.bin --speed 1.0

# - `--binfile`: Path to the pre-recorded `.bin` file.
# - `--speed`: Playback speed, where 1.0 is real-time, >1.0 is faster, <1.0 slower.

# 5. Connecting Your Parser or ROS2 Node
# The replay creates a virtual serial port like `/dev/pts/6` which appears as a normal serial port.
# For example, to connect a ROS2 radar parser:
#   ros2 launch radar_parser radar_parser.launch.py serial_port:=/dev/pts/6

# 6. Notes for Compatibility & Usage
# - Ensure your radar device outputs data in the expected comma-separated format during recording.
#   Example printout after successful serial connection:
#   "0,120" indicating angle=0 degrees, distance=120cm.
# - When replaying, data streams directly into the PTY, which your parser reads as a serial device.
# - Adjust `baudrate` and port names as per your hardware configuration.

# Optional: You may want to wrap these commands into a single shell script for convenience, i.e.,
# (then you can copy and execute steps as needed in your development workflow)
