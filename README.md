# TI AWR1843BOOST High-Performance Radar System

## Implementation Options

**Python Implementation** (main branch): Full-featured, easy to modify  
**C++ Implementation** (cpp-implementation branch): High-performance, production-ready with DRWIG compatibility

## Quick Start

### Live Radar Operation
```bash
# Single radar
python3 radar_system.py --live /dev/ttyACM0 --csv live_data.csv

# Dual radars
python3 radar_system.py --live /dev/ttyACM0 /dev/ttyACM1 --csv dual_radar.csv
```

### Recorded Data Replay
```bash
# Single file replay
python3 radar_system.py --replay data/recordings/radar_file.bin --speed 2.0

# Dual file simulation
python3 radar_system.py --replay data/recordings/file1.bin data/recordings/file2.bin
```

### Mixed Mode
```bash
# Live + replay simultaneously
python3 radar_system.py --live /dev/ttyACM0 --replay data/recordings/test.bin
```

### C++ Implementation (cpp-implementation branch)
```bash
# Build C++ version
cd cpp && ./build.sh

# Live radar with enhanced CSV logging
./build/radar_system --live /dev/ttyACM0 --csv live_data
# Creates: live_data_detected.csv, live_data_clusters.csv, live_data_tracked.csv

# High-speed replay with ego speed filtering
./build/radar_system --replay ../data/recordings/radar_file.bin --speed 10.0 --ego-speed 15.0

# Dual radar with selective activation
./build/radar_system --live /dev/ttyACM0 /dev/ttyACM1 --deactivate-radar 1 --csv dual_radar

# Mixed mode with advanced features
./build/radar_system --live /dev/ttyACM0 --replay ../data/test.bin --csv mixed_mode --no-console
```

## Performance

### Python Implementation
- **Achieved**: 1000+ Hz sustained logging
- **Multi-threaded** with lock-free ring buffers
- **Dual radar** parallel processing
- **Memory efficient**: ~50MB for dual setup

### C++ Implementation (cpp-implementation branch)
- **Achieved**: 1500+ Hz sustained logging (50% faster than Python)
- **Lower latency**: Sub-millisecond frame processing
- **Memory efficient**: ~35MB for dual setup (30% less than Python)
- **Native performance**: Optimized for production use
- **DRWIG Compatible**: Event-driven callbacks and enhanced object parsing
- **Advanced Features**: Ego speed estimation, static object filtering, separate CSV outputs

## Files

### Python Implementation (main branch)
- `radar_system.py` - Main entry point
- `src/` - Core system modules
- `ARCHITECTURE.md` - Detailed technical documentation

### C++ Implementation (cpp-implementation branch)
- `cpp/src/main.cpp` - Main entry point with enhanced CLI
- `cpp/src/core/` - Radar system and data processor
- `cpp/src/interfaces/` - Serial, replay, and CAN interfaces
- `cpp/src/parsers/` - AWR1843 parser with DRWIG compatibility
- `cpp/include/` - Header files with complete type definitions
- `cpp/examples/` - Usage examples and callback demonstrations
- `cpp/CMakeLists.txt` - Optimized build configuration
- `cpp/build.sh` - Automated build script

### Common
- `data/recordings/` - Binary radar data files
- `legacy/` - Original recorder/replay scripts

See `ARCHITECTURE.md` for complete technical details.