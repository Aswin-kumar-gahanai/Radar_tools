#pragma once

#include "radar_source.h"
#include <string>
#include <fstream>

class ReplaySource : public RadarSource {
public:
    ReplaySource(int radar_id, const std::string& filename, double speed = 2.0);
    ~ReplaySource() override;
    
    void setup(std::shared_ptr<RingBuffer<RadarFrame>> buffer) override;
    void start() override;
    void stop() override;
    
private:
    void replay_loop();
    
    std::string filename_;
    double speed_;
    std::ifstream file_;
};