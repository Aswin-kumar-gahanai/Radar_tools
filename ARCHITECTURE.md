# TI AWR1843BOOST High-Performance Radar System

## Overview

This system provides high-performance data acquisition and processing for TI AWR1843BOOST mmWave radar sensors, achieving 1000+ Hz logging rates with multi-radar support.

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
├── config/                      # Configuration files (future)
├── tests/                       # Test modules (future)
└── docs/                        # Documentation
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
- **Python**: 3.6+
- **PySerial**: For serial communication
- **Standard Library**: threading, queue, struct, csv

## Current Limitations

### Frame Drop Detection
- Ring buffers track dropped frames but don't actively report to console
- Buffer overflow counter exists but needs user-visible reporting
- No real-time frame drop rate statistics

### Output Control
- No granular control over console output data types
- Cannot selectively choose which TLV types to display (objects, noise, heatmap)
- CSV logging includes all data types without filtering options
- Fixed console output format without customization

### Data Processing
- Only detected objects processing - no object tracking between frames
- No correlation or persistence of objects across time
- Limited velocity estimation (simplified Doppler conversion)

### Speed Parameter
- `--speed` only affects replay mode, not live data processing
- No throttling or rate limiting for live radar data
- Processing speed tied to incoming data rate (921600 baud)

## Enhancement Roadmap

### High Priority
- **Selective Output Filtering**: Add `--console-filter` and `--csv-filter` options
  - Example: `--console-filter objects,noise --csv-filter objects`
  - Allow users to choose which TLV types to display/log
- **Frame Drop Reporting**: Real-time notification of dropped frames
  - Console alerts when buffer overflow occurs
  - Statistics reporting (drop rate, buffer utilization)
- **Enhanced Performance Monitoring**: 
  - Per-radar frame rates and processing statistics
  - Buffer utilization metrics per radar source

### Medium Priority
- **Object Tracking Layer**: Application-level object correlation
  - Track objects across frames using position/velocity
  - Assign unique IDs to persistent objects
  - Configurable tracking parameters (distance threshold, timeout)
- **Improved Velocity Calculation**: 
  - Use actual chirp configuration for accurate Doppler conversion
  - Calibrated velocity measurements in m/s
- **Configuration Management**: 
  - YAML/JSON configuration files for system parameters
  - Radar-specific settings (baud rate, buffer sizes)
  - Output format customization

### Future Extensions
- **CAN Bus Support**: Add CAN interface for automotive applications
- **Shared Memory**: Inter-process communication for distributed systems
- **Real-time Visualization**: Live plotting and monitoring
  - 2D/3D object position plots
  - Heatmap visualization
  - Real-time performance dashboards
- **Advanced Data Analysis**:
  - Statistical analysis of detection patterns
  - Export to common formats (JSON, HDF5)
  - Integration with machine learning pipelines