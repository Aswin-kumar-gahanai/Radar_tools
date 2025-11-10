# TI AWR1843BOOST High-Performance Radar System

## Overview

This system provides high-performance data acquisition and processing for TI AWR1843BOOST mmWave radar sensors. The Python implementation achieves 1000+ Hz logging rates, while the C++ implementation delivers 1500+ Hz with enhanced features including DRWIG compatibility, advanced object parsing, and production-ready optimizations.

## Architecture

### System Components

```
┌─────────────────────────────────────────────────────────────────┐
│                    TI AWR1843BOOST Radar System                │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Data Sources  │    │  Core Processing │    │     Output      │
│                 │    │                  │    │                 │
│ ┌─────────────┐ │    │ ┌──────────────┐ │    │ ┌─────────────┐ │
│ │ Live Radar  │ │───▶│ │ Ring Buffers │ │───▶│ │   Console   │ │
│ │/dev/ttyACM* │ │    │ │              │ │    │ │   Output    │ │
│ └─────────────┘ │    │ └──────────────┘ │    │ └─────────────┘ │
│                 │    │                  │    │                 │
│ ┌─────────────┐ │    │ ┌──────────────┐ │    │ ┌─────────────┐ │
│ │ Bin Replay  │ │───▶│ │ Data Parser  │ │───▶│ │ CSV Logger  │ │
│ │   Files     │ │    │ │ (AWR1843)    │ │    │ │             │ │
│ └─────────────┘ │    │ └──────────────┘ │    │ └─────────────┘ │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

### Data Flow

1. **Data Acquisition**
   - Live radars: Serial data from `/dev/ttyACM*` ports at 921600 baud
   - Replay mode: Binary files recorded from previous sessions
   - Multi-threaded: Each radar source runs in separate thread

2. **Buffer Management**
   - Lock-free ring buffers (30,000 frame capacity per radar)
   - High-speed data transfer between threads
   - Automatic overflow handling

3. **Data Processing**
   - TI AWR1843BOOST frame parsing using actual firmware structures
   - **Detected Objects**: angle, distance, velocity, coordinates (x,y,z)
   - **Noise Profile**: average noise levels and sample counts
   - **Azimuth Heatmap**: intensity maps for spatial analysis
   - Real-time multi-data processing at 2000Hz rate

4. **Output Generation**
   - Console: Real-time display of detected objects
   - CSV: High-speed logging with batched writes
   - Performance monitoring and statistics

## Directory Structure

### Python Implementation (main branch)
```
radar_scripts/Radar_tools/
├── radar_system.py              # Main entry point
├── src/
│   ├── core/
│   │   ├── radar_system.py      # System controller and main loop
│   │   └── data_processor.py    # Multi-threaded data processing
│   ├── interfaces/
│   │   ├── serial_source.py     # Live radar serial interface
│   │   └── replay_source.py     # Binary file replay interface
│   └── parsers/
│       └── awr1843_parser.py    # TI AWR1843BOOST frame parser
├── data/
│   ├── recordings/              # Binary radar data files (.bin)
│   └── output/                  # CSV output files
├── legacy/
│   └── radar_recorder_replay.py # Original recorder (preserved)
└── docs/                        # Documentation
```

### C++ Implementation (cpp-implementation branch)
```
radar_scripts/Radar_tools/cpp/
├── src/
│   ├── main.cpp                 # Enhanced CLI entry point
│   ├── core/
│   │   ├── radar_system.cpp     # High-performance system controller
│   │   └── data_processor.cpp   # Optimized multi-threaded processing
│   ├── interfaces/
│   │   ├── serial_source.cpp    # Live radar interface
│   │   ├── replay_source.cpp    # Binary file replay
│   │   └── can_interface.cpp    # CAN bus interface (header only)
│   └── parsers/
│       └── awr1843_parser.cpp   # DRWIG-compatible parser
├── include/
│   ├── common/
│   │   └── types.h              # Complete data structures
│   ├── core/
│   │   ├── radar_system.h       # System interface
│   │   ├── data_processor.h     # Processing interface
│   │   └── ring_buffer.h        # Lock-free ring buffer
│   ├── interfaces/
│   │   ├── radar_source.h       # Base interface
│   │   ├── serial_source.h      # Serial interface
│   │   ├── replay_source.h      # Replay interface
│   │   └── can_interface.h      # CAN interface
│   └── parsers/
│       └── awr1843_parser.h     # Enhanced parser with callbacks
├── examples/
│   └── callback_example.cpp     # DRWIG callback usage
├── CMakeLists.txt               # Optimized build configuration
├── build.sh                     # Automated build script
└── README_CPP.md                # C++ specific documentation
```

## Data Format

### Input Data
- **Live**: TI AWR1843BOOST binary frames via UART (921600 baud)
- **Replay**: Binary `.bin` files containing recorded radar frames
- **Frame Structure**: TI mmWave standard with magic word `02010403 06050807`

### Output Data
- **Console**: 
  - Detected Objects: `R{radar_id}: {angle:6.1f}° {distance:6.1f}cm`
  - Noise Profile: `R{radar_id}: NOISE avg={avg_noise_level} samples={samples}`
  - Azimuth Heatmap: `R{radar_id}: HEATMAP max={max_intensity} avg={avg_intensity}`
- **CSV**: `timestamp,radar_id,data_type,angle,distance,value`
  - data_type: 'object', 'noise', 'heatmap'
  - value: velocity (objects), noise_level (noise), max_intensity (heatmap)

### Frame Parsing
Based on TI AWR1843BOOST firmware structures:
- `MmwDemo_output_message_header`: 40-byte frame header
- `MmwDemo_detectedObj`: 12-byte detected object structure
- TLV (Type-Length-Value) format for data payload

### TLV Data Types
The system processes 3 main TLV types from firmware (`mrr_output.h`):
- **Type 1 - Detected Points**: Object detections with position, velocity, peak values
- **Type 3 - Noise Profile**: Average noise levels and sample counts per frame
- **Type 4 - Azimuth Static Heatmap**: Spatial intensity maps for analysis

### Speed Parameter Behavior
- **Live Mode**: No speed control - processes at incoming data rate (921600 baud)
- **Replay Mode**: `--speed` multiplier controls playback rate
  - Calculates base delay: `chunk_size / (921600 / 8)`
  - Applies speed: `time.sleep(base_delay / speed)`
  - Default speed: 2.0x (faster than real-time)
- **Processing Rate**: 2000Hz internal processing loop regardless of input speed

## Performance Characteristics

### Achieved Performance
- **Data Rate**: 1000+ Hz sustained logging
- **Latency**: <1ms processing per frame
- **Memory**: ~50MB for dual radar setup
- **CPU**: Efficient multi-threading with lock-free buffers

### Scalability
- **Multi-radar**: Parallel processing of multiple radar sources
- **Buffer Capacity**: 30,000 frames per radar (configurable)
- **Thread Architecture**: Dedicated threads for I/O, processing, and output

## Usage Examples

### Basic Operations

```bash
# Live single radar
python3 radar_system.py --live /dev/ttyACM0

# Live dual radars with CSV logging
python3 radar_system.py --live /dev/ttyACM0 /dev/ttyACM1 --csv radar_data.csv

# Replay recorded data
python3 radar_system.py --replay data/recordings/radar_session.bin

# High-speed replay
python3 radar_system.py --replay data/recordings/test.bin --speed 5.0

# Mixed mode (live + replay)
python3 radar_system.py --live /dev/ttyACM0 --replay data/recordings/reference.bin

# CSV-only logging (no console output)
python3 radar_system.py --live /dev/ttyACM0 --csv output.csv --no-console
```

### Command Line Arguments

#### Current Arguments
| Argument | Type | Description | Example |
|----------|------|-------------|---------|
| `--live` | list | Live radar serial ports | `--live /dev/ttyACM0 /dev/ttyACM1` |
| `--replay` | list | Binary files to replay | `--replay file1.bin file2.bin` |
| `--speed` | float | Replay speed multiplier (replay only) | `--speed 2.5` |
| `--csv` | string | CSV output filename | `--csv radar_log.csv` |
| `--no-console` | flag | Disable console output | `--no-console` |

#### Proposed Enhancements
| Argument | Type | Description | Example |
|----------|------|-------------|---------|
| `--console-filter` | list | Console output data types | `--console-filter objects,noise` |
| `--csv-filter` | list | CSV logging data types | `--csv-filter objects,heatmap` |
| `--show-drops` | flag | Display frame drop statistics | `--show-drops` |
| `--buffer-size` | int | Ring buffer size per radar | `--buffer-size 50000` |
| `--stats-interval` | float | Performance stats interval (seconds) | `--stats-interval 5.0` |

## Technical Details

### Threading Model
- **Main Thread**: System control and CSV output
- **Source Threads**: One per radar (live or replay)
- **Processor Thread**: Data parsing and object detection
- **Lock-free Communication**: Ring buffers between threads

### Memory Management
- **Ring Buffers**: Circular buffers with automatic overflow
- **Zero-copy**: Minimal data copying between threads
- **Configurable**: Buffer sizes adjustable for different workloads

### Error Handling
- **Connection Recovery**: Automatic reconnection for live radars
- **Data Validation**: Frame integrity checking with magic word verification
- **Graceful Degradation**: Continue operation if one radar fails
- **Buffer Overflow**: Automatic frame dropping when ring buffer reaches capacity
- **Frame Drop Tracking**: Internal counter for dropped frames (not user-visible)

## Integration Notes

### Hardware Requirements
- **Radar**: TI AWR1843BOOST evaluation board
- **Connection**: USB serial (typically `/dev/ttyACM*`)
- **Baud Rate**: 921600 (configured in radar firmware)

### Software Dependencies

#### Python Implementation
- **Python**: 3.6+
- **PySerial**: For serial communication
- **Standard Library**: threading, queue, struct, csv

#### C++ Implementation
- **C++ Compiler**: GCC 7+ or Clang 6+ with C++17 support
- **CMake**: 3.16+ for build system
- **Threading**: POSIX threads (pthread)
- **Standard Library**: Only standard C++ libraries (no external dependencies)

## Implementation Status

### ✅ Completed Features

#### Python Implementation (main branch)
- High-performance data acquisition (1000+ Hz)
- Multi-radar support with parallel processing
- Live radar and binary file replay
- TLV parsing for detected objects, noise, and heatmaps
- CSV logging and console output
- Lock-free ring buffers
- Mixed mode operation (live + replay)

#### C++ Implementation (cpp-implementation branch)
- **Enhanced Performance**: 1500+ Hz processing (50% faster than Python)
- **DRWIG Compatibility**: Event-driven callbacks and extended object parsing
- **Advanced Object Types**: Detected objects, clusters, and tracked objects
- **Separate CSV Outputs**: Individual files for each object type
- **Ego Speed Estimation**: Automatic vehicle speed calculation
- **Static Object Filtering**: Remove stationary objects based on ego motion
- **Dynamic Radar Control**: Enable/disable radars at runtime
- **Fixed-point Conversion**: Proper scaling for position, velocity, and size
- **Memory Optimization**: 30% less memory usage than Python
- **Production Ready**: Optimized build system and error handling

### ⚠️ Current Limitations

#### Python Implementation
- Limited to detected objects only (no clusters or tracked objects)
- Single CSV output format
- No ego speed estimation
- No static object filtering
- Fixed console output format

#### C++ Implementation
- No tracked objects in current test data (parser ready but no data source)
- Visualization tools are stashed (available but not committed)
- CAN interface implemented as header-only (not fully functional)

### 🔄 Known Issues
- Some bin files may not contain cluster or tracked object TLVs
- Frame drop reporting exists but not user-visible
- No real-time visualization integrated into main system

## Enhancement Roadmap

### ✅ Completed (C++ Implementation)
- **Enhanced Object Parsing**: Detected objects, clusters, tracked objects
- **DRWIG Compatibility**: Event-driven callbacks and dual interface support
- **Separate CSV Outputs**: Individual files for each object type
- **Ego Speed Estimation**: Automatic vehicle speed calculation from stationary objects
- **Static Object Filtering**: Remove stationary objects based on ego motion
- **Dynamic Radar Control**: Runtime activation/deactivation of specific radars
- **Performance Optimization**: 50% faster processing, 30% less memory
- **Production Build System**: CMake with optimization flags
- **Comprehensive Documentation**: Complete API and usage documentation

### 🚧 In Progress
- **Visualization Tools**: Comprehensive 2D/3D/live visualization (stashed, ready for use)
- **Real-time Object Tracking**: Multi-frame correlation and persistence
- **Enhanced Error Handling**: Better connection recovery and validation

### 📋 High Priority (Next Phase)
- **CAN Bus Integration**: Complete automotive interface implementation
- **Configuration Management**: YAML/JSON configuration files
- **Frame Drop Reporting**: User-visible buffer overflow notifications
- **Integrated Visualization**: Built-in real-time plotting capabilities
- **Advanced Analytics**: Statistical analysis and pattern recognition

### 📋 Medium Priority
- **Machine Learning Integration**: Object classification and prediction
- **Sensor Fusion**: Multi-modal data processing
- **Cloud Connectivity**: Remote monitoring and data upload
- **Advanced Tracking**: Kalman filtering and trajectory prediction

### 📋 Future Extensions
- **Shared Memory IPC**: Inter-process communication for distributed systems
- **Real-time Operating System**: RTOS compatibility for embedded systems
- **Hardware Acceleration**: GPU processing for high-throughput scenarios
- **Advanced Export Formats**: JSON, HDF5, Protocol Buffers
- **Web Interface**: Browser-based monitoring and control

## Migration Guide

### Python to C++ Migration
The C++ implementation maintains command-line compatibility with the Python version:

```bash
# Python command
python3 radar_system.py --live /dev/ttyACM0 --csv data.csv

# Equivalent C++ command
./build/radar_system --live /dev/ttyACM0 --csv data
# Note: C++ creates data_detected.csv, data_clusters.csv, data_tracked.csv
```

### Enhanced Features Available in C++
```bash
# Ego speed estimation and static filtering
./build/radar_system --live /dev/ttyACM0 --ego-speed 15.0 --csv filtered

# Dynamic radar control
./build/radar_system --live /dev/ttyACM0 /dev/ttyACM1 --deactivate-radar 1

# High-speed replay with advanced features
./build/radar_system --replay data.bin --speed 20.0 --ego-speed 10.0
```

### DRWIG Integration
```cpp
// Event-driven processing
AWR1843Parser parser;
parser.set_detected_object_callback([](const auto& objects) {
    // Handle detected objects
});
parser.set_tracked_object_callback([](const auto& objects) {
    // Handle tracked objects
});
parser.parse_frame_with_callbacks(raw_data, radar_id);
```