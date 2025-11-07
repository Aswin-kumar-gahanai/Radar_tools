#!/usr/bin/env python3
"""Data processor with TI AWR1843BOOST parser"""
import threading
import queue
import time
import sys
import os
from typing import Optional, Dict, List

# Add parsers to path
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'parsers'))
from awr1843_parser import AWR1843Parser

class DataProcessor(threading.Thread):
    def __init__(self, buffers: List, output_queue: queue.Queue):
        super().__init__(daemon=True)
        self.buffers = buffers
        self.output_queue = output_queue
        self.running = True
        self.parser = AWR1843Parser()
        
    def run(self):
        while self.running:
            for buffer in self.buffers:
                frames = buffer.get_all()
                for frame in frames:
                    # Parse AWR1843 frame - returns list of detected objects
                    detected_objects = self.parser.parse_frame(frame.raw_data, frame.radar_id)
                    
                    # Send each parsed data item as separate frame
                    for data_item in detected_objects:
                        # Create new frame for each data item
                        new_frame = type('RadarFrame', (), {
                            'radar_id': frame.radar_id,
                            'timestamp': frame.timestamp,
                            'raw_data': frame.raw_data,
                            'parsed_data': data_item
                        })()
                        try:
                            self.output_queue.put_nowait(new_frame)
                        except queue.Full:
                            pass
                            
            time.sleep(0.0005)  # 2000Hz processing loop
    
    def stop(self):
        self.running = False