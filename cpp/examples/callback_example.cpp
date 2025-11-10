#include "../include/parsers/awr1843_parser.h"
#include <iostream>
#include <vector>

// Example DRWIG-compatible callback usage
int main() {
    AWR1843Parser parser;
    
    // Set up callbacks for different object types
    parser.set_detected_object_callback([](const std::vector<DetectedObject>& objects) {
        std::cout << "Detected " << objects.size() << " objects" << std::endl;
        for (const auto& obj : objects) {
            std::cout << "  Object at " << obj.distance << "cm, " << obj.angle << "°" << std::endl;
        }
    });
    
    parser.set_tracked_object_callback([](const std::vector<TrackedObject>& objects) {
        std::cout << "Tracking " << objects.size() << " objects" << std::endl;
        for (const auto& obj : objects) {
            std::cout << "  Track ID " << obj.track_id << " at (" << obj.x << "," << obj.y << ")" << std::endl;
        }
    });
    
    parser.set_frame_callback([](const RadarFrame& frame) {
        std::cout << "Frame from radar " << frame.radar_id << " with " 
                  << frame.detected_objects.size() << " detected, "
                  << frame.cluster_objects.size() << " clusters, "
                  << frame.tracked_objects.size() << " tracked" << std::endl;
    });
    
    // Example usage with raw radar data
    std::vector<uint8_t> raw_data; // Would contain actual radar data
    
    // Event-driven parsing with callbacks
    parser.parse_frame_with_callbacks(raw_data, 0);
    
    // Or traditional queue-based parsing
    RadarFrame frame = parser.parse_frame(raw_data, 0);
    
    return 0;
}