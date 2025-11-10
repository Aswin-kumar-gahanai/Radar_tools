#include "core/data_processor.h"
#include "parsers/awr1843_parser.h"
#include <chrono>
#include <iostream>

DataProcessor::DataProcessor(std::vector<std::shared_ptr<RingBuffer<RadarFrame>>>& buffers,
                           std::queue<RadarFrame>& output_queue)
    : buffers_(buffers), output_queue_(output_queue), running_(false) {
    parser_ = std::make_unique<AWR1843Parser>();
}

DataProcessor::~DataProcessor() {
    stop();
}

void DataProcessor::start() {
    running_ = true;
    worker_thread_ = std::thread(&DataProcessor::run, this);
    std::cout << "[DataProcessor] Started with optimized AWR1843 parser" << std::endl;
}

void DataProcessor::stop() {
    running_ = false;
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    std::cout << "[DataProcessor] Stopped" << std::endl;
}

void DataProcessor::run() {
    while (running_) {
        for (auto& buffer : buffers_) {
            auto frames = buffer->get_all();
            for (const auto& frame : frames) {
                // Parse AWR1843 frame - returns vector of detected objects
                auto detected_objects = parser_->parse_frame(frame.raw_data, frame.radar_id);
                
                // Create separate frame for each detected object
                for (const auto& obj : detected_objects) {
                    RadarFrame new_frame;
                    new_frame.radar_id = frame.radar_id;
                    new_frame.timestamp = frame.timestamp;
                    new_frame.raw_data = frame.raw_data;
                    new_frame.parsed_data = {obj};
                    
                    // Non-blocking queue insert
                    if (output_queue_.size() < 5000) {
                        output_queue_.push(new_frame);
                    }
                }
            }
        }
        
        // High-frequency processing loop (2000Hz)
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}