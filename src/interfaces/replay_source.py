#!/usr/bin/env python3
"""Bin file replay interface"""
import threading
import time
import os
from core.radar_system import RadarFrame

class ReplayRadarSource(threading.Thread):
    def __init__(self, radar_id: int, binfile: str, speed: float = 1.0):
        super().__init__(daemon=True)
        self.radar_id = radar_id
        self.binfile = binfile
        self.speed = speed
        self.running = True
        self.buffer = None
        self.active_flag = None
        
    def setup(self, buffer, active_flag):
        self.buffer = buffer
        self.active_flag = active_flag
        
    def run(self):
        if not os.path.exists(self.binfile):
            print(f"[ERROR] Bin file not found: {self.binfile}")
            return
            
        try:
            print(f"[REPLAY-{self.radar_id}] Playing {self.binfile} at {self.speed}x")
            
            with open(self.binfile, 'rb') as f:
                chunk_size = 2048
                base_delay = chunk_size / (921600 / 8)
                
                while self.running:
                    if not self.active_flag.is_set():
                        time.sleep(0.01)
                        continue
                    
                    data = f.read(chunk_size)
                    if not data:
                        f.seek(0)  # Loop file
                        continue
                    
                    frame = RadarFrame(
                        radar_id=self.radar_id,
                        timestamp=time.time(),
                        raw_data=data
                    )
                    self.buffer.put(frame)
                    time.sleep(base_delay / self.speed)
                    
        except Exception as e:
            print(f"[REPLAY-{self.radar_id}] Error: {e}")
    
    def stop(self):
        self.running = False