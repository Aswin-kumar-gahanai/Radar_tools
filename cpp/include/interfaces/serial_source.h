#pragma once

#include "radar_source.h"
#include <string>

class SerialSource : public RadarSource {
public:
    SerialSource(int radar_id, const std::string& port);
    ~SerialSource() override;
    
    void setup(std::shared_ptr<RingBuffer<RadarFrame>> buffer) override;
    void start() override;
    void stop() override;
    
private:
    void read_loop();
    bool open_serial();
    void close_serial();
    
    std::string port_;
    int serial_fd_;
};