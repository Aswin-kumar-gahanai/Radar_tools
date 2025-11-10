#!/usr/bin/env python3
import os
import subprocess
import sys

def test_all_bins():
    bin_dir = "/home/jishnu/work/radar_scripts/Radar_tools/data/recordings"
    build_dir = "/home/jishnu/work/radar_scripts/Radar_tools/cpp/build"
    
    if not os.path.exists(bin_dir):
        print(f"Directory not found: {bin_dir}")
        return
    
    bin_files = [f for f in os.listdir(bin_dir) if f.endswith('.bin')]
    print(f"Found {len(bin_files)} bin files")
    
    for i, bin_file in enumerate(bin_files):
        print(f"\nTesting {bin_file}...")
        bin_path = os.path.join(bin_dir, bin_file)
        csv_prefix = f"scan_{i+1}"
        
        # Run radar system
        cmd = [
            "./radar_system", 
            "--replay", bin_path,
            "--csv", csv_prefix,
            "--speed", "50.0",
            "--no-console"
        ]
        
        try:
            result = subprocess.run(cmd, cwd=build_dir, timeout=10, 
                                  capture_output=True, text=True)
            
            # Check results
            tracked_file = os.path.join(build_dir, f"{csv_prefix}_tracked.csv")
            if os.path.exists(tracked_file):
                with open(tracked_file, 'r') as f:
                    lines = f.readlines()
                    tracked_count = len(lines) - 1  # Subtract header
                    if tracked_count > 0:
                        print(f"  ✅ Found {tracked_count} tracked objects!")
                        return bin_file, csv_prefix
                    else:
                        print(f"  ❌ No tracked objects")
            else:
                print(f"  ❌ No tracked file generated")
                
        except subprocess.TimeoutExpired:
            print(f"  ⏰ Timeout")
        except Exception as e:
            print(f"  ❌ Error: {e}")
    
    print("\nNo tracked objects found in any file")
    return None, None

if __name__ == "__main__":
    test_all_bins()
