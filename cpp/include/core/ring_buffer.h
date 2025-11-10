#pragma once

#include <deque>
#include <mutex>
#include <atomic>
#include <vector>

template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t max_size = 30000) 
        : max_size_(max_size), dropped_frames_(0) {}
    
    void put(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (buffer_.size() >= max_size_) {
            buffer_.pop_front();
            dropped_frames_++;
        }
        buffer_.push_back(item);
    }
    
    std::vector<T> get_all() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<T> items(buffer_.begin(), buffer_.end());
        buffer_.clear();
        return items;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return buffer_.size();
    }
    
    uint64_t get_dropped_frames() const {
        return dropped_frames_.load();
    }
    
private:
    mutable std::mutex mutex_;
    std::deque<T> buffer_;
    size_t max_size_;
    std::atomic<uint64_t> dropped_frames_;
};