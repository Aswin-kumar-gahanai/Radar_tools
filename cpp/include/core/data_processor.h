#pragma once

#include <thread>
#include <atomic>
#include <queue>
#include <memory>
#include "ring_buffer.h"
#include "common/types.h"

class AWR1843Parser;

class DataProcessor {
public:
    DataProcessor(std::vector<std::shared_ptr<RingBuffer<RadarFrame>>>& buffers, 
                  std::queue<RadarFrame>& output_queue);
    ~DataProcessor();
    
    void start();
    void stop();
    
private:
    void run();
    
    std::vector<std::shared_ptr<RingBuffer<RadarFrame>>>& buffers_;
    std::queue<RadarFrame>& output_queue_;
    std::atomic<bool> running_;
    std::thread worker_thread_;
    std::unique_ptr<AWR1843Parser> parser_;
};