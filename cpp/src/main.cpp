#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include "core/radar_system.h"
#include "interfaces/serial_source.h"
#include "interfaces/replay_source.h"

void print_usage(const char* program_name) {
    std::cout << "TI AWR1843BOOST High-Performance Radar Data Logger (C++)\n\n";
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --live PORT [PORT...]     Live radar serial ports (e.g., /dev/ttyACM0)\n";
    std::cout << "  --replay FILE [FILE...]   Replay from recorded bin files\n";
    std::cout << "  --speed X                 Replay speed multiplier (default: 2.0)\n";
    std::cout << "  --csv FILE               CSV output file (creates _detected.csv and _tracked.csv)\n";
    std::cout << "  --ego-speed X            Ego vehicle speed in m/s for static filtering\n";
    std::cout << "  --activate-radar ID      Activate specific radar (0,1,2...)\n";
    std::cout << "  --deactivate-radar ID    Deactivate specific radar\n";
    std::cout << "  --no-console             Disable console output\n";
    std::cout << "  --help                   Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  Live radar:     " << program_name << " --live /dev/ttyACM0 --csv live_data.csv\n";
    std::cout << "  Dual radars:    " << program_name << " --live /dev/ttyACM0 /dev/ttyACM1\n";
    std::cout << "  Replay data:    " << program_name << " --replay data/recordings/radar_file.bin\n";
    std::cout << "  Mixed mode:     " << program_name << " --live /dev/ttyACM0 --replay data/test.bin\n";
}

int main(int argc, char* argv[]) {
    std::vector<std::string> live_ports;
    std::vector<std::string> replay_files;
    double speed = 2.0;
    std::string csv_file;
    bool console_output = true;
    double ego_speed = 0.0;
    std::vector<int> activate_radars;
    std::vector<int> deactivate_radars;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--live") {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                live_ports.push_back(argv[++i]);
            }
        } else if (arg == "--replay") {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                replay_files.push_back(argv[++i]);
            }
        } else if (arg == "--speed" && i + 1 < argc) {
            speed = std::stod(argv[++i]);
        } else if (arg == "--csv" && i + 1 < argc) {
            csv_file = argv[++i];
        } else if (arg == "--ego-speed" && i + 1 < argc) {
            ego_speed = std::stod(argv[++i]);
        } else if (arg == "--activate-radar" && i + 1 < argc) {
            activate_radars.push_back(std::stoi(argv[++i]));
        } else if (arg == "--deactivate-radar" && i + 1 < argc) {
            deactivate_radars.push_back(std::stoi(argv[++i]));
        } else if (arg == "--no-console") {
            console_output = false;
        }
    }
    
    if (live_ports.empty() && replay_files.empty()) {
        print_usage(argv[0]);
        std::cerr << "\nERROR: Must specify --live or --replay\n";
        return 1;
    }
    
    RadarSystem system;
    
    // Add live radars
    for (size_t i = 0; i < live_ports.size(); ++i) {
        auto source = std::make_unique<SerialSource>(i, live_ports[i]);
        std::cout << "Added live radar " << i << ": " << live_ports[i] << std::endl;
        system.add_source(std::move(source));
    }
    
    // Add replay radars
    size_t offset = live_ports.size();
    for (size_t i = 0; i < replay_files.size(); ++i) {
        if (!std::filesystem::exists(replay_files[i])) {
            std::cerr << "ERROR: File not found: " << replay_files[i] << std::endl;
            return 1;
        }
        auto source = std::make_unique<ReplaySource>(offset + i, replay_files[i], speed);
        std::cout << "Added replay radar " << (offset + i) << ": " << replay_files[i] << std::endl;
        system.add_source(std::move(source));
    }
    
    // Configure system
    if (ego_speed > 0) {
        system.set_ego_vehicle_speed(ego_speed);
    }
    
    for (int radar_id : deactivate_radars) {
        system.activate_radar(radar_id, false);
    }
    for (int radar_id : activate_radars) {
        system.activate_radar(radar_id, true);
    }
    
    std::cout << "\nStarting optimized C++ radar system with " << (live_ports.size() + replay_files.size()) << " sources..." << std::endl;
    if (!csv_file.empty()) {
        std::cout << "Logging to separate CSV files: " << csv_file << "_detected.csv and " << csv_file << "_tracked.csv" << std::endl;
    }
    if (ego_speed > 0) {
        std::cout << "Static object filtering enabled with ego speed: " << ego_speed << " m/s" << std::endl;
    }
    
    system.start(csv_file, console_output);
    
    return 0;
}