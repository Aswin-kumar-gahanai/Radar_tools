#pragma once

#include <vector>
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <fstream>
#include <queue>
#include <mutex>
#include "interfaces/radar_source.h"
#include "data_processor.h"
#include "common/types.h"

template<typename T>
class RingBuffer;

class RadarSystem {
public:
    RadarSystem();
    ~RadarSystem();
    
    void add_source(std::unique_ptr<RadarSource> source);
    void start(const std::string& csv_filename = "", bool console_output = true);
    void stop();
    void set_ego_vehicle_speed(double speed_mps); // CAN input
    void activate_radar(int radar_id, bool activate); // Firmware control
    
private:
    void main_loop(bool console_output);
    void display_frame_data(const DetectedObject& obj);
    void log_detected_objects(const RadarFrame& frame);
    void log_tracked_objects(const RadarFrame& frame);
    bool is_static_object(const DetectedObject& obj);
    
    std::vector<std::unique_ptr<RadarSource>> sources_;
    std::vector<std::shared_ptr<RingBuffer<RadarFrame>>> buffers_;
    std::unique_ptr<DataProcessor> processor_;
    std::queue<RadarFrame> output_queue_;
    std::mutex queue_mutex_;
    
    std::atomic<bool> running_;
    std::ofstream detected_csv_file_;
    std::ofstream tracked_csv_file_;
    double start_time_;
    int frame_count_;
    double ego_vehicle_speed_;
    std::vector<bool> radar_active_flags_;
};