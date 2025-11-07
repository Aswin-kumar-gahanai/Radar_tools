#!/usr/bin/env python3
"""
TI AWR1843BOOST High-Performance Radar Data Logger
Main entry point for radar data acquisition and processing
"""
import sys
import os
import argparse

# Add src to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'src'))

from core.radar_system import RadarSystem
from interfaces.serial_source import SerialRadarSource
from interfaces.replay_source import ReplayRadarSource

def main():
    parser = argparse.ArgumentParser(
        description="TI AWR1843BOOST High-Performance Radar Data Logger",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  Live radar:     python3 radar_system.py --live /dev/ttyACM0 --csv live_data.csv
  Dual radars:    python3 radar_system.py --live /dev/ttyACM0 /dev/ttyACM1
  Replay data:    python3 radar_system.py --replay data/recordings/radar_file.bin
  Mixed mode:     python3 radar_system.py --live /dev/ttyACM0 --replay data/test.bin
        """
    )
    
    parser.add_argument("--live", nargs='+', metavar="PORT", 
                       help="Live radar serial ports (e.g., /dev/ttyACM0 /dev/ttyACM1)")
    parser.add_argument("--replay", nargs='+', metavar="FILE",
                       help="Replay from recorded bin files")
    parser.add_argument("--speed", type=float, default=2.0, metavar="X",
                       help="Replay speed multiplier (default: 2.0)")
    parser.add_argument("--csv", metavar="FILE",
                       help="CSV output file for logging detected objects")
    parser.add_argument("--no-console", action='store_true',
                       help="Disable console output (CSV only)")
    
    args = parser.parse_args()
    
    if not (args.live or args.replay):
        parser.print_help()
        print("\nERROR: Must specify --live or --replay")
        sys.exit(1)
    
    system = RadarSystem()
    
    # Add live radars
    if args.live:
        for i, port in enumerate(args.live):
            source = SerialRadarSource(i, port)
            system.add_source(source)
            print(f"Added live radar {i}: {port}")
    
    # Add replay radars
    if args.replay:
        offset = len(args.live) if args.live else 0
        for i, binfile in enumerate(args.replay):
            if not os.path.exists(binfile):
                print(f"ERROR: File not found: {binfile}")
                sys.exit(1)
            source = ReplayRadarSource(offset + i, binfile, args.speed)
            system.add_source(source)
            print(f"Added replay radar {offset + i}: {binfile}")
    
    print(f"\nStarting radar system with {len(system.sources)} sources...")
    if args.csv:
        print(f"Logging to CSV: {args.csv}")
    
    system.start(args.csv, not args.no_console)

if __name__ == "__main__":
    main()