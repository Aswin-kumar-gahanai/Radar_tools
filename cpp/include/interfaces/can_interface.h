#pragma once

#include <functional>
#include <thread>
#include <atomic>

class CANInterface {
public:
    CANInterface();
    ~CANInterface();
    
    void start(const std::string& can_device = "can0");
    void stop();
    void set_ego_speed_callback(std::function<void(double)> callback);
    
private:
    void can_reader_thread();
    
    std::atomic<bool> running_;
    std::thread reader_thread_;
    std::function<void(double)> ego_speed_callback_;
    std::string can_device_;
};