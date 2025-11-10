#include "interfaces/serial_source.h"
#include "core/ring_buffer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

SerialSource::SerialSource(int radar_id, const std::string& port)
    : RadarSource(radar_id), port_(port), serial_fd_(-1) {
}

SerialSource::~SerialSource() {
    stop();
}

void SerialSource::setup(std::shared_ptr<RingBuffer<RadarFrame>> buffer) {
    buffer_ = buffer;
}

void SerialSource::start() {
    if (running_) return;
    
    running_ = true;
    worker_thread_ = std::thread(&SerialSource::read_loop, this);
}

void SerialSource::stop() {
    running_ = false;
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    close_serial();
}

void SerialSource::read_loop() {
    while (running_) {
        if (!open_serial()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        uint8_t buffer[4096];
        while (running_) {
            ssize_t bytes_read = read(serial_fd_, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                auto now = std::chrono::high_resolution_clock::now();
                double timestamp = std::chrono::duration<double>(now.time_since_epoch()).count();
                
                RadarFrame frame;
                frame.radar_id = radar_id_;
                frame.timestamp = timestamp;
                frame.raw_data.assign(buffer, buffer + bytes_read);
                frame.parsed_data_json = "{\"radar_id\":" + std::to_string(radar_id_) + "}";
                
                buffer_->put(frame);
            } else if (bytes_read < 0) {
                std::cerr << "Serial read error on " << port_ << std::endl;
                close_serial();
                break;
            }
        }
    }
}

bool SerialSource::open_serial() {
    if (serial_fd_ >= 0) return true;
    
    serial_fd_ = open(port_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serial_fd_ < 0) {
        return false;
    }
    
    struct termios tty;
    if (tcgetattr(serial_fd_, &tty) != 0) {
        close_serial();
        return false;
    }
    
    // Configure for 921600 baud, 8N1
    cfsetospeed(&tty, B921600);
    cfsetispeed(&tty, B921600);
    
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
    
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;
    
    if (tcsetattr(serial_fd_, TCSANOW, &tty) != 0) {
        close_serial();
        return false;
    }
    
    return true;
}

void SerialSource::close_serial() {
    if (serial_fd_ >= 0) {
        close(serial_fd_);
        serial_fd_ = -1;
    }
}