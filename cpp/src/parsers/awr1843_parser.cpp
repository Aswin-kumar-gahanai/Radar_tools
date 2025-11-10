#include "parsers/awr1843_parser.h"
#include <cstring>
#include <cmath>
#include <algorithm>

AWR1843Parser::AWR1843Parser() : frame_count_(0) {}

std::vector<DetectedObject> AWR1843Parser::parse_frame(const std::vector<uint8_t>& raw_data, int radar_id) {
    std::vector<DetectedObject> results;
    
    const uint8_t* data = raw_data.data();
    size_t len = raw_data.size();
    size_t pos = 0;
    
    while (pos < len - 40) {
        // Find magic word
        const uint8_t* magic_pos = static_cast<const uint8_t*>(
            std::search(data + pos, data + len, MAGIC_WORD, MAGIC_WORD + 8));
        
        if (magic_pos == data + len) break;
        
        size_t frame_pos = magic_pos - data;
        auto frame_objects = parse_single_frame(data + frame_pos, len - frame_pos, radar_id);
        results.insert(results.end(), frame_objects.begin(), frame_objects.end());
        
        pos = frame_pos + 40;
    }
    
    return results;
}

std::vector<DetectedObject> AWR1843Parser::parse_single_frame(const uint8_t* data, size_t len, int radar_id) {
    if (len < 40) return {};
    
    // Parse header (40 bytes for XWR18XX with subframe)
    uint32_t total_packet_len = *reinterpret_cast<const uint32_t*>(data + 12);
    uint32_t frame_number = *reinterpret_cast<const uint32_t*>(data + 20);
    uint32_t num_detected_obj = *reinterpret_cast<const uint32_t*>(data + 28);
    uint32_t num_tlvs = *reinterpret_cast<const uint32_t*>(data + 32);
    
    if (num_detected_obj == 0 || len < total_packet_len) return {};
    
    std::vector<DetectedObject> detected_objects;
    size_t tlv_pos = 40;
    
    for (uint32_t tlv_idx = 0; tlv_idx < num_tlvs && tlv_pos + 8 <= len; ++tlv_idx) {
        uint32_t tlv_type = *reinterpret_cast<const uint32_t*>(data + tlv_pos);
        uint32_t tlv_length = *reinterpret_cast<const uint32_t*>(data + tlv_pos + 4);
        
        size_t tlv_data_start = tlv_pos + 8;
        size_t tlv_data_end = tlv_data_start + tlv_length;
        
        if (tlv_data_end > len) break;
        
        if (tlv_type == MMWDEMO_OUTPUT_MSG_DETECTED_POINTS) {
            auto objects = parse_detected_objects_tlv(
                data + tlv_data_start, tlv_length, radar_id, frame_number, num_detected_obj);
            detected_objects.insert(detected_objects.end(), objects.begin(), objects.end());
        }
        
        tlv_pos = tlv_data_end;
    }
    
    return detected_objects;
}

std::vector<DetectedObject> AWR1843Parser::parse_detected_objects_tlv(
    const uint8_t* tlv_data, size_t len, int radar_id, int frame_number, int expected_objects) {
    
    std::vector<DetectedObject> objects;
    if (len < 4) return objects;
    
    uint16_t num_detected = *reinterpret_cast<const uint16_t*>(tlv_data);
    uint16_t xyz_q_format = *reinterpret_cast<const uint16_t*>(tlv_data + 2);
    
    size_t obj_start = 4;
    size_t obj_size = 12;
    
    for (int i = 0; i < std::min(static_cast<int>(num_detected), expected_objects); ++i) {
        size_t obj_pos = obj_start + i * obj_size;
        if (obj_pos + obj_size > len) break;
        
        const uint8_t* obj_data = tlv_data + obj_pos;
        uint16_t range_idx = *reinterpret_cast<const uint16_t*>(obj_data);
        int16_t doppler_idx = *reinterpret_cast<const int16_t*>(obj_data + 2);
        uint16_t peak_val = *reinterpret_cast<const uint16_t*>(obj_data + 4);
        int16_t x_q = *reinterpret_cast<const int16_t*>(obj_data + 6);
        int16_t y_q = *reinterpret_cast<const int16_t*>(obj_data + 8);
        int16_t z_q = *reinterpret_cast<const int16_t*>(obj_data + 10);
        
        double q_scale = xyz_q_format > 0 ? 1.0 / (1 << xyz_q_format) : 1.0;
        double x_m = x_q * q_scale;
        double y_m = y_q * q_scale;
        double z_m = z_q * q_scale;
        
        double distance = std::sqrt(x_m*x_m + y_m*y_m) * 100.0; // Convert to cm
        double angle = std::atan2(y_m, x_m) * 180.0 / M_PI;
        double velocity_mps = doppler_idx * 0.1;
        
        if (distance > 5.0 && distance < 5000.0) {
            DetectedObject obj;
            obj.radar_id = radar_id;
            obj.frame_number = frame_number;
            obj.angle = angle;
            obj.distance = distance;
            obj.velocity = velocity_mps;
            obj.x = x_m;
            obj.y = y_m;
            obj.z = z_m;
            obj.peak_val = peak_val;
            obj.range_idx = range_idx;
            obj.doppler_idx = doppler_idx;
            objects.push_back(obj);
        }
    }
    
    return objects;
}