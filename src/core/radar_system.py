#!/usr/bin/env python3
"""
High-Performance Multi-Radar System - Core Module
Handles live streaming and bin replay with 50-60Hz+ performance
"""
import threading
import queue
import time
import csv
import sys
import os
from collections import deque
from dataclasses import dataclass
from typing import Optional, Dict, List, Union

@dataclass
class RadarFrame:
    radar_id: int
    timestamp: float
    raw_data: bytes
    parsed_data: Optional[Dict] = None

class RingBuffer:
    def __init__(self, size: int = 30000):
        self.buffer = deque(maxlen=size)
        self.lock = threading.Lock()
        self.dropped_frames = 0
    
    def put(self, item):
        with self.lock:
            if len(self.buffer) == self.buffer.maxlen:
                self.dropped_frames += 1
            self.buffer.append(item)
    
    def get_all(self):
        with self.lock:
            items = list(self.buffer)
            self.buffer.clear()
            return items

class RadarSystem:
    def __init__(self):
        self.sources = []
        self.buffers = []
        self.processor = None
        self.output_queue = queue.Queue(maxsize=5000)
        self.radar_flags = {}
        self.csv_writer = None
        self.csv_file = None
        self.start_time = None
        
    def add_source(self, source):
        buffer = RingBuffer()
        flag = threading.Event()
        flag.set()
        
        source.setup(buffer, flag)
        self.sources.append(source)
        self.buffers.append(buffer)
        self.radar_flags[source.radar_id] = flag
        
    def start(self, csv_filename: Optional[str] = None, console_output: bool = True):
        if csv_filename:
            self.csv_file = open(csv_filename, 'w', newline='')
            self.csv_writer = csv.writer(self.csv_file)
            self.csv_writer.writerow(['timestamp', 'radar_id', 'frame_number', 'angle', 'distance', 'velocity', 'x', 'y', 'z', 'peak_val', 'range_idx', 'doppler_idx'])
        
        from core.data_processor import DataProcessor
        self.processor = DataProcessor(self.buffers, self.output_queue)
        self.processor.start()
        
        for source in self.sources:
            source.start()
        
        self.start_time = time.time()
        print(f"[SYSTEM] Started with {len(self.sources)} radar sources")
        
        self._main_loop(console_output)
    
    def _main_loop(self, console_output: bool):
        frame_count = 0
        last_stats = time.time()
        
        try:
            while True:
                try:
                    frame = self.output_queue.get(timeout=0.1)
                    frame_count += 1
                    
                    if frame.parsed_data:
                        if console_output:
                            self._display_frame_data(frame.parsed_data)
                        
                        if self.csv_writer:
                            self._log_frame_data(frame)
                            if frame_count % 100 == 0:
                                self.csv_file.flush()
                    
                    if time.time() - last_stats > 3.0:
                        elapsed = time.time() - self.start_time
                        hz = frame_count / elapsed if elapsed > 0 else 0
                        print(f"[PERF] {hz:.1f} Hz ({frame_count} frames)")
                        last_stats = time.time()
                        
                except queue.Empty:
                    continue
                    
        except KeyboardInterrupt:
            print("\n[SYSTEM] Stopping...")
            self.stop()
    
    def stop(self):
        if self.processor:
            self.processor.stop()
        for source in self.sources:
            source.stop()
        if self.csv_file:
            self.csv_file.close()
    
    def _display_frame_data(self, data: Dict):
        """Display frame data based on type"""
        data_type = data.get('data_type', 'detected_object')
        
        if data_type == 'detected_object':
            print(f"R{data['radar_id']}: {data['angle']:6.1f}° {data['distance']:6.1f}cm vel:{data.get('velocity', 0):5.1f} peak:{data.get('peak_val', 0)}")
        elif data_type == 'noise_profile':
            print(f"R{data['radar_id']}: NOISE avg={data['avg_noise_level']:.1f} samples={data['samples']}")
        elif data_type == 'azimuth_heatmap':
            print(f"R{data['radar_id']}: HEATMAP max={data['max_intensity']} avg={data['avg_intensity']:.1f}")
    
    def _log_frame_data(self, frame):
        """Log frame data to CSV based on type"""
        data = frame.parsed_data
        data_type = data.get('data_type', 'detected_object')
        
        if data_type == 'detected_object':
            # Log complete object data
            self.csv_writer.writerow([
                frame.timestamp, frame.radar_id, data['frame_number'],
                data['angle'], data['distance'], data.get('velocity', 0),
                data.get('x', 0), data.get('y', 0), data.get('z', 0),
                data.get('peak_val', 0), data.get('range_idx', 0), data.get('doppler_idx', 0)
            ])
        elif data_type == 'noise_profile':
            self.csv_writer.writerow([
                frame.timestamp, frame.radar_id, 0,
                0, 0, data['avg_noise_level'], 0, 0, 0, 0, 0, 0
            ])
        elif data_type == 'azimuth_heatmap':
            self.csv_writer.writerow([
                frame.timestamp, frame.radar_id, 0,
                0, 0, data['max_intensity'], 0, 0, 0, 0, 0, 0
            ])