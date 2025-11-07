#!/usr/bin/env python3
"""Serial interface for live radar data"""
import threading
import time
import serial
from core.radar_system import RadarFrame

class SerialRadarSource(threading.Thread):
    def __init__(self, radar_id: int, port: str, baudrate: int = 921600):
        super().__init__(daemon=True)
        self.radar_id = radar_id
        self.port = port
        self.baudrate = baudrate
        self.running = True
        self.buffer = None
        self.active_flag = None
        
    def setup(self, buffer, active_flag):
        self.buffer = buffer
        self.active_flag = active_flag
        
    def run(self):
        try:
            ser = serial.Serial(self.port, baudrate=self.baudrate, timeout=0.05)
            print(f"[LIVE-{self.radar_id}] Connected to {self.port}")
            
            while self.running:
                if not self.active_flag.is_set():
                    time.sleep(0.01)
                    continue
                
                data = ser.read(2048)
                if data:
                    frame = RadarFrame(
                        radar_id=self.radar_id,
                        timestamp=time.time(),
                        raw_data=data
                    )
                    self.buffer.put(frame)
                    
        except Exception as e:
            print(f"[LIVE-{self.radar_id}] Error: {e}")
    
    def stop(self):
        self.running = False