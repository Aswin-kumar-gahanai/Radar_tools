#pragma once

#include <vector>
#include <memory>
#include "common/types.h"

class AWR1843Parser {
public:
    AWR1843Parser();
    std::vector<DetectedObject> parse_frame(const std::vector<uint8_t>& raw_data, int radar_id);
    
private:
    static constexpr uint8_t MAGIC_WORD[8] = {0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0x08, 0x07};
    static constexpr uint32_t MMWDEMO_OUTPUT_MSG_DETECTED_POINTS = 1;
    static constexpr uint32_t MMWDEMO_OUTPUT_MSG_NOISE_PROFILE = 3;
    static constexpr uint32_t MMWDEMO_OUTPUT_MSG_AZIMUT_STATIC_HEAT_MAP = 4;
    
    int frame_count_;
    
    std::vector<DetectedObject> parse_single_frame(const uint8_t* data, size_t len, int radar_id);
    std::vector<DetectedObject> parse_detected_objects_tlv(const uint8_t* tlv_data, size_t len, int radar_id, int frame_number, int expected_objects);
};