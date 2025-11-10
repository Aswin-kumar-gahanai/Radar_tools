#pragma once

#include <vector>
#include <string>
#include <cstdint>

struct DetectedObject {
    int radar_id;
    int frame_number;
    double angle;
    double distance;
    double velocity;
    double x, y, z;
    uint16_t peak_val;
    uint16_t range_idx;
    int16_t doppler_idx;
    std::string data_type = "detected_object";
};

struct RadarFrame {
    int radar_id;
    double timestamp;
    std::vector<uint8_t> raw_data;
    std::string parsed_data_json;
    std::vector<DetectedObject> parsed_data;
};