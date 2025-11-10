#include "interfaces/can_interface.h"
#include <iostream>
#include <chrono>

CANInterface::CANInterface() : running_(false) {}

CANInterface::~CANInterface() {
    stop();
}

void CANInterface::start(const std::string& can_device) {
    can_device_ = can_device;
    running_ = true;
    reader_thread_ = std::thread(&CANInterface::can_reader_thread, this);
    std::cout << "[CAN] Started CAN interface on " << can_device << std::endl;
}

void CANInterface::stop() {
    running_ = false;
    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }
}

void CANInterface::set_ego_speed_callback(std::function<void(double)> callback) {
    ego_speed_callback_ = callback;
}

void CANInterface::can_reader_thread() {
    // Stub implementation - in production, use SocketCAN
    // This would read CAN frames and extract ego vehicle speed
    while (running_) {
        // Simulate CAN data reception
        if (ego_speed_callback_) {
            // Mock ego speed data - in production parse actual CAN frames
            static double mock_speed = 0.0;
            mock_speed += 0.1;
            if (mock_speed > 30.0) mock_speed = 0.0;
            
            ego_speed_callback_(mock_speed);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}