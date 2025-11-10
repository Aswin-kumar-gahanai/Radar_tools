#include "core/radar_system.h"
#include "core/ring_buffer.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <cmath>

RadarSystem::RadarSystem() 
    : running_(false), start_time_(0.0), frame_count_(0), ego_vehicle_speed_(0.0) {
}

RadarSystem::~RadarSystem() {
    stop();
}

void RadarSystem::add_source(std::unique_ptr<RadarSource> source) {
    auto buffer = std::make_shared<RingBuffer<RadarFrame>>();
    source->setup(buffer);
    
    sources_.push_back(std::move(source));
    buffers_.push_back(buffer);
    radar_active_flags_.push_back(true);
}

void RadarSystem::start(const std::string& csv_filename, bool console_output) {
    // Setup separate CSV files for detected and tracked objects
    if (!csv_filename.empty()) {
        std::string detected_file = csv_filename.substr(0, csv_filename.find_last_of('.')) + "_detected.csv";
        std::string tracked_file = csv_filename.substr(0, csv_filename.find_last_of('.')) + "_tracked.csv";
        
        detected_csv_file_.open(detected_file);
        tracked_csv_file_.open(tracked_file);
        
        if (detected_csv_file_.is_open()) {
            detected_csv_file_ << "timestamp,radar_id,frame_number,angle,distance,velocity,x,y,z,peak_val,range_idx,doppler_idx\n";
        }
        if (tracked_csv_file_.is_open()) {
            tracked_csv_file_ << "timestamp,radar_id,frame_number,angle,distance,velocity,x,y,z,peak_val,range_idx,doppler_idx,track_id\n";
        }
    }
    
    // Initialize data processor with buffers and output queue
    processor_ = std::make_unique<DataProcessor>(buffers_, output_queue_);
    processor_->start();
    
    running_ = true;
    
    // Start all active sources
    for (size_t i = 0; i < sources_.size(); ++i) {
        if (radar_active_flags_[i]) {
            sources_[i]->start();
        }
    }
    
    auto now = std::chrono::high_resolution_clock::now();
    start_time_ = std::chrono::duration<double>(now.time_since_epoch()).count();
    
    std::cout << "[SYSTEM] Started with " << sources_.size() << " radar sources (optimized C++ implementation)" << std::endl;
    
    main_loop(console_output);
}

void RadarSystem::stop() {
    running_ = false;
    
    if (processor_) {
        processor_->stop();
    }
    
    for (auto& source : sources_) {
        source->stop();
    }
    
    if (detected_csv_file_.is_open()) {
        detected_csv_file_.close();
    }
    if (tracked_csv_file_.is_open()) {
        tracked_csv_file_.close();
    }
}

void RadarSystem::set_ego_vehicle_speed(double speed_mps) {
    ego_vehicle_speed_ = speed_mps;
    std::cout << "[CAN] Ego vehicle speed updated: " << speed_mps << " m/s" << std::endl;
}

void RadarSystem::activate_radar(int radar_id, bool activate) {
    if (radar_id >= 0 && radar_id < static_cast<int>(radar_active_flags_.size())) {
        radar_active_flags_[radar_id] = activate;
        std::cout << "[FIRMWARE] Radar " << radar_id << (activate ? " activated" : " deactivated") << std::endl;
        
        if (running_) {
            if (activate) {
                sources_[radar_id]->start();
            } else {
                sources_[radar_id]->stop();
            }
        }
    }
}

void RadarSystem::main_loop(bool console_output) {
    auto last_stats = std::chrono::high_resolution_clock::now();
    
    while (running_) {
        bool processed_any = false;
        
        // Process data from output queue
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!output_queue_.empty()) {
            RadarFrame frame = output_queue_.front();
            output_queue_.pop();
            frame_count_++;
            processed_any = true;
            
            if (!frame.parsed_data.empty()) {
                for (const auto& obj : frame.parsed_data) {
                    if (console_output) {
                        display_frame_data(obj);
                    }
                    
                    // Log to appropriate CSV file
                    if (!is_static_object(obj)) {
                        log_detected_objects(frame);
                        // Simple tracking logic - in production use Kalman filter
                        log_tracked_objects(frame);
                    }
                }
                
                if (frame_count_ % 100 == 0) {
                    if (detected_csv_file_.is_open()) detected_csv_file_.flush();
                    if (tracked_csv_file_.is_open()) tracked_csv_file_.flush();
                }
            }
        }
        
        // Performance statistics
        auto now = std::chrono::high_resolution_clock::now();
        auto stats_elapsed = std::chrono::duration<double>(now - last_stats).count();
        
        if (stats_elapsed > 3.0) {
            auto total_elapsed = std::chrono::duration<double>(now.time_since_epoch()).count() - start_time_;
            double hz = frame_count_ / total_elapsed;
            std::cout << "[PERF] " << std::fixed << std::setprecision(1) << hz 
                     << " Hz (" << frame_count_ << " frames)" << std::endl;
            last_stats = now;
        }
        
        if (!processed_any) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}

void RadarSystem::display_frame_data(const DetectedObject& obj) {
    std::cout << "R" << obj.radar_id << ": " 
              << std::fixed << std::setprecision(1)
              << obj.angle << "° " << obj.distance << "cm vel:" 
              << obj.velocity << " peak:" << obj.peak_val << std::endl;
}

void RadarSystem::log_detected_objects(const RadarFrame& frame) {
    if (!detected_csv_file_.is_open()) return;
    
    for (const auto& obj : frame.parsed_data) {
        detected_csv_file_ << frame.timestamp << "," << frame.radar_id << "," 
                          << obj.frame_number << "," << obj.angle << "," << obj.distance << "," 
                          << obj.velocity << "," << obj.x << "," << obj.y << "," << obj.z << "," 
                          << obj.peak_val << "," << obj.range_idx << "," << obj.doppler_idx << "\n";
    }
}

void RadarSystem::log_tracked_objects(const RadarFrame& frame) {
    if (!tracked_csv_file_.is_open()) return;
    
    // Simple tracking - in production use proper tracking algorithm
    static int track_id = 0;
    for (const auto& obj : frame.parsed_data) {
        tracked_csv_file_ << frame.timestamp << "," << frame.radar_id << "," 
                         << obj.frame_number << "," << obj.angle << "," << obj.distance << "," 
                         << obj.velocity << "," << obj.x << "," << obj.y << "," << obj.z << "," 
                         << obj.peak_val << "," << obj.range_idx << "," << obj.doppler_idx << "," 
                         << track_id++ << "\n";
    }
}

bool RadarSystem::is_static_object(const DetectedObject& obj) {
    // Filter static objects using ego vehicle speed
    // Object is static if its relative velocity compensates for ego motion
    double relative_velocity = std::abs(obj.velocity);
    double ego_compensation = ego_vehicle_speed_ * std::cos(obj.angle * M_PI / 180.0);
    
    return std::abs(relative_velocity - ego_compensation) < 0.5; // 0.5 m/s threshold
}