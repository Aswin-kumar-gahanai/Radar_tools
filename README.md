# TI AWR1843BOOST High-Performance Radar System

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

## Performance
- **Achieved**: 1000+ Hz sustained logging
- **Multi-threaded** with lock-free ring buffers
- **Dual radar** parallel processing
- **Memory efficient**: ~50MB for dual setup

## Files
- `radar_system.py` - Main entry point
- `ARCHITECTURE.md` - Detailed technical documentation
- `src/` - Core system modules
- `data/recordings/` - Binary radar data files
- `legacy/` - Original recorder/replay scripts

See `ARCHITECTURE.md` for complete technical details.