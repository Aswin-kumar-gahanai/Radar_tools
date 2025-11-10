#pragma once

#include <memory>
#include <atomic>
#include <thread>
#include "common/types.h"

template<typename T>
class RingBuffer;

class RadarSource {
public:
    RadarSource(int radar_id) : radar_id_(radar_id), running_(false) {}
    virtual ~RadarSource() = default;
    
    virtual void setup(std::shared_ptr<RingBuffer<RadarFrame>> buffer) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    
    int get_radar_id() const { return radar_id_; }
    
protected:
    int radar_id_;
    std::atomic<bool> running_;
    std::shared_ptr<RingBuffer<RadarFrame>> buffer_;
    std::thread worker_thread_;
};