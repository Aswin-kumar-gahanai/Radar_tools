#!/usr/bin/env python3
"""
Correct TI AWR1843BOOST Parser based on actual firmware structures
From: /home/jishnu/workspace_ccstheia/test/drwig-firmware-can-working/
"""
import struct
import math
from typing import Optional, Dict, List

class AWR1843Parser:
    """TI AWR1843BOOST mmWave radar data parser"""
    
    # Magic word from mrr_output.h: {0x0102,0x0304,0x0506,0x0708}
    MAGIC_WORD = b'\x02\x01\x04\x03\x06\x05\x08\x07'
    
    # TLV Types from mrr_output.h
    MMWDEMO_OUTPUT_MSG_DETECTED_POINTS = 1
    MMWDEMO_OUTPUT_MSG_RANGE_PROFILE = 2
    MMWDEMO_OUTPUT_MSG_NOISE_PROFILE = 3
    MMWDEMO_OUTPUT_MSG_AZIMUT_STATIC_HEAT_MAP = 4
    MMWDEMO_OUTPUT_MSG_RANGE_DOPPLER_HEAT_MAP = 5
    MMWDEMO_OUTPUT_MSG_STATS = 6
    
    def __init__(self):
        self.frame_count = 0
        
    def parse_frame(self, data: bytes, radar_id: int) -> List[Dict]:
        """Parse AWR1843BOOST frame using correct firmware structures"""
        results = []
        
        pos = 0
        while pos < len(data) - 40:  # Need at least 40 bytes for header
            # Find magic word
            magic_pos = data.find(self.MAGIC_WORD, pos)
            if magic_pos == -1:
                break
                
            try:
                frame_objects = self._parse_single_frame(data[magic_pos:], radar_id)
                results.extend(frame_objects)
                pos = magic_pos + 40  # Move past header
                
            except Exception:
                pos = magic_pos + 1
                continue
                
        return results
    
    def _parse_single_frame(self, data: bytes, radar_id: int) -> List[Dict]:
        """Parse single frame using MmwDemo_output_message_header structure"""
        if len(data) < 40:  # Header size for XWR18XX with subframe
            return []
            
        try:
            # Parse header: magicWord[4] + version + totalPacketLen + platform + 
            #               frameNumber + timeCpuCycles + numDetectedObj + numTLVs + subFrameNumber
            header_data = struct.unpack('<HHHH IIII III I', data[:40])
            
            magic = header_data[:4]  # Should be [0x0102, 0x0304, 0x0506, 0x0708]
            version = header_data[4]
            total_packet_len = header_data[5]
            platform = header_data[6]
            frame_number = header_data[7]
            time_cpu_cycles = header_data[8]
            num_detected_obj = header_data[9]
            num_tlvs = header_data[10]
            subframe_number = header_data[11]
            
            if num_detected_obj == 0:
                return []
            
            # Parse TLVs starting after header
            tlv_pos = 40
            detected_objects = []
            
            for tlv_idx in range(num_tlvs):
                if tlv_pos + 8 > len(data):
                    break
                    
                # Parse TLV header: type + length
                tlv_type, tlv_length = struct.unpack('<II', data[tlv_pos:tlv_pos+8])
                tlv_data_start = tlv_pos + 8
                tlv_data_end = tlv_data_start + tlv_length
                
                if tlv_data_end > len(data):
                    break
                
                # Parse different TLV types
                if tlv_type == self.MMWDEMO_OUTPUT_MSG_DETECTED_POINTS:
                    objects = self._parse_detected_objects_tlv(
                        data[tlv_data_start:tlv_data_end],
                        radar_id,
                        frame_number,
                        num_detected_obj
                    )
                    detected_objects.extend(objects)
                
                elif tlv_type == self.MMWDEMO_OUTPUT_MSG_NOISE_PROFILE:
                    noise_data = self._parse_noise_profile_tlv(
                        data[tlv_data_start:tlv_data_end],
                        radar_id,
                        frame_number
                    )
                    if noise_data:
                        detected_objects.append(noise_data)
                
                elif tlv_type == self.MMWDEMO_OUTPUT_MSG_AZIMUT_STATIC_HEAT_MAP:
                    heatmap_data = self._parse_azimuth_heatmap_tlv(
                        data[tlv_data_start:tlv_data_end],
                        radar_id,
                        frame_number
                    )
                    if heatmap_data:
                        detected_objects.append(heatmap_data)
                
                tlv_pos = tlv_data_end
            
            return detected_objects
            
        except (struct.error, IndexError):
            return []
    
    def _parse_detected_objects_tlv(self, tlv_data: bytes, radar_id: int, frame_number: int, expected_objects: int) -> List[Dict]:
        """Parse detected objects TLV using MmwDemo_detectedObj structure"""
        objects = []
        
        try:
            # First 4 bytes: MmwDemo_output_message_dataObjDescr
            # numDetetedObj (uint16) + xyzQFormat (uint16)
            if len(tlv_data) < 4:
                return objects
                
            num_detected, xyz_q_format = struct.unpack('<HH', tlv_data[:4])
            
            # Each detected object: MmwDemo_detectedObj structure (12 bytes)
            # rangeIdx(uint16) + dopplerIdx(int16) + peakVal(uint16) + x(int16) + y(int16) + z(int16)
            obj_start = 4
            obj_size = 12
            
            for i in range(min(num_detected, expected_objects)):
                obj_pos = obj_start + i * obj_size
                if obj_pos + obj_size > len(tlv_data):
                    break
                    
                obj_data = tlv_data[obj_pos:obj_pos + obj_size]
                range_idx, doppler_idx, peak_val, x_q, y_q, z_q = struct.unpack('<HhHhhh', obj_data)
                
                # Convert Q-format coordinates to meters
                # Q format is typically Q8 or Q9 for coordinates
                q_scale = 1.0 / (1 << xyz_q_format) if xyz_q_format > 0 else 1.0
                
                x_m = x_q * q_scale
                y_m = y_q * q_scale  
                z_m = z_q * q_scale
                
                # Convert to polar coordinates
                distance = math.sqrt(x_m*x_m + y_m*y_m) * 100  # Convert to cm
                angle = math.degrees(math.atan2(y_m, x_m))
                
                # Estimate velocity from Doppler (simplified)
                # Actual conversion depends on chirp configuration
                velocity_mps = doppler_idx * 0.1  # Approximate scaling
                
                if 5 < distance < 5000:  # Reasonable range 5cm to 50m
                    objects.append({
                        'radar_id': radar_id,
                        'frame_number': frame_number,
                        'angle': angle,
                        'distance': distance,
                        'velocity': velocity_mps,
                        'x': x_m,
                        'y': y_m,
                        'z': z_m,
                        'range_idx': range_idx,
                        'doppler_idx': doppler_idx,
                        'peak_val': peak_val,
                        'xyz_q_format': xyz_q_format
                    })
                    
        except (struct.error, ValueError, ZeroDivisionError):
            pass
            
        return objects
    
    def _parse_noise_profile_tlv(self, tlv_data: bytes, radar_id: int, frame_number: int) -> Optional[Dict]:
        """Parse noise profile TLV"""
        try:
            if len(tlv_data) >= 4:
                # Simple noise level extraction
                noise_samples = len(tlv_data) // 2  # Assume 16-bit samples
                avg_noise = sum(struct.unpack(f'<{noise_samples}H', tlv_data[:noise_samples*2])) / noise_samples
                
                return {
                    'radar_id': radar_id,
                    'frame_number': frame_number,
                    'data_type': 'noise_profile',
                    'avg_noise_level': avg_noise,
                    'samples': noise_samples
                }
        except (struct.error, ValueError):
            pass
        return None
    
    def _parse_azimuth_heatmap_tlv(self, tlv_data: bytes, radar_id: int, frame_number: int) -> Optional[Dict]:
        """Parse azimuth heatmap TLV"""
        try:
            if len(tlv_data) >= 8:
                # Extract basic heatmap info
                heatmap_size = len(tlv_data) // 4  # Assume 32-bit values
                if heatmap_size > 0:
                    values = struct.unpack(f'<{heatmap_size}I', tlv_data[:heatmap_size*4])
                    max_value = max(values)
                    avg_value = sum(values) / len(values)
                    
                    return {
                        'radar_id': radar_id,
                        'frame_number': frame_number,
                        'data_type': 'azimuth_heatmap',
                        'max_intensity': max_value,
                        'avg_intensity': avg_value,
                        'heatmap_points': heatmap_size
                    }
        except (struct.error, ValueError):
            pass
        return None