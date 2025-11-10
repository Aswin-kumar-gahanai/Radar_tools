cmake_minimum_required(VERSION 3.16)
project(RadarSystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/core/radar_system.cpp
    src/core/data_processor.cpp
    src/interfaces/serial_source.cpp
    src/interfaces/replay_source.cpp
    src/parsers/awr1843_parser.cpp
    src/main.cpp
)

# Create executable
add_executable(radar_system ${SOURCES})

# Link libraries
target_link_libraries(radar_system 
    Threads::Threads
)

# Compiler flags for optimization
target_compile_options(radar_system PRIVATE
    -O3
    -Wall
    -Wextra
    -pthread
)