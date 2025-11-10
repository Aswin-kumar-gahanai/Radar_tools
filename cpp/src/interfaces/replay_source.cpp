#include "interfaces/replay_source.h"
#include "core/ring_buffer.h"
#include <iostream>
#include <chrono>
#include <thread>

ReplaySource::ReplaySource(int radar_id, const std::string& filename, double speed)
    : RadarSource(radar_id), filename_(filename), speed_(speed) {
}

ReplaySource::~ReplaySource() {
    stop();
}

void ReplaySource::setup(std::shared_ptr<RingBuffer<RadarFrame>> buffer) {
    buffer_ = buffer;
}

void ReplaySource::start() {
    if (running_) return;
    
    running_ = true;
    worker_thread_ = std::thread(&ReplaySource::replay_loop, this);
}

void ReplaySource::stop() {
    running_ = false;
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    if (file_.is_open()) {
        file_.close();
    }
}

void ReplaySource::replay_loop() {
    file_.open(filename_, std::ios::binary);
    if (!file_.is_open()) {
        std::cerr << "Failed to open replay file: " << filename_ << std::endl;
        return;
    }
    
    const size_t chunk_size = 4096;
    const double base_delay = chunk_size / (921600.0 / 8.0); // Base delay for 921600 baud
    const double actual_delay = base_delay / speed_;
    
    char buffer[chunk_size];
    
    while (running_ && file_.good()) {
        file_.read(buffer, chunk_size);
        std::streamsize bytes_read = file_.gcount();
        
        if (bytes_read > 0) {
            auto now = std::chrono::high_resolution_clock::now();
            double timestamp = std::chrono::duration<double>(now.time_since_epoch()).count();
            
            RadarFrame frame;
            frame.radar_id = radar_id_;
            frame.timestamp = timestamp;
            frame.raw_data.assign(reinterpret_cast<uint8_t*>(buffer), 
                                 reinterpret_cast<uint8_t*>(buffer) + bytes_read);
            frame.parsed_data_json = "{\"radar_id\":" + std::to_string(radar_id_) + "}";
            
            buffer_->put(frame);
            
            // Simulate timing based on speed parameter
            std::this_thread::sleep_for(
                std::chrono::duration<double>(actual_delay)
            );
        }
    }
    
    file_.close();
    std::cout << "Replay finished for radar " << radar_id_ << std::endl;
}