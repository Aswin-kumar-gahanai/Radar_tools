#!/usr/bin/env python3
"""Show detailed object data from radar"""
import sys
import os
sys.path.append('src/parsers')

from awr1843_parser import AWR1843Parser

def show_object_details():
    parser = AWR1843Parser()
    binfile = 'data/recordings/radar_2025-11-07_13-05-45.bin'
    
    with open(binfile, 'rb') as f:
        data = f.read(20000)  # First 20KB
    
    objects = parser.parse_frame(data, radar_id=0)
    
    print("=== DETECTED OBJECTS FROM YOUR RADAR ===")
    object_count = 0
    
    for item in objects:
        if item.get('data_type') == 'detected_object' or 'angle' in item:
            object_count += 1
            if object_count <= 10:  # Show first 10 objects
                print(f"\nObject {object_count}:")
                print(f"  Frame: {item.get('frame_number', 'N/A')}")
                print(f"  Angle: {item.get('angle', 0):.2f}°")
                print(f"  Distance: {item.get('distance', 0):.2f} cm")
                print(f"  Velocity: {item.get('velocity', 0):.2f} m/s")
                print(f"  X coordinate: {item.get('x', 0):.3f} m")
                print(f"  Y coordinate: {item.get('y', 0):.3f} m") 
                print(f"  Z coordinate: {item.get('z', 0):.3f} m")
                print(f"  Peak value: {item.get('peak_val', 0)}")
                print(f"  Range index: {item.get('range_idx', 0)}")
                print(f"  Doppler index: {item.get('doppler_idx', 0)}")
    
    print(f"\nTotal detected objects found: {object_count}")
    
    # Show other data types
    noise_count = sum(1 for item in objects if item.get('data_type') == 'noise_profile')
    heatmap_count = sum(1 for item in objects if item.get('data_type') == 'azimuth_heatmap')
    
    print(f"Noise profiles found: {noise_count}")
    print(f"Azimuth heatmaps found: {heatmap_count}")

if __name__ == "__main__":
    show_object_details()