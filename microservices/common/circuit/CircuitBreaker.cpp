#include "CircuitBreaker.h"
#include <iostream>

CircuitBreaker::CircuitBreaker(int failureThreshold, 
                               std::chrono::milliseconds timeout,
                               std::chrono::milliseconds retryTimeout)
    : failureThreshold_(failureThreshold)
    , timeout_(timeout)
    , retryTimeout_(retryTimeout)
    , state_(CircuitState::CLOSED)
    , failureCount_(0)
    , successCount_(0)
    , lastFailureTime_(std::chrono::steady_clock::now())
    , lastRetryTime_(std::chrono::steady_clock::now()) {
}

bool CircuitBreaker::canExecute() {
    std::lock_guard<std::mutex> lock(mutex_);
    updateState();
    
    switch (state_.load()) {
        case CircuitState::CLOSED:
            return true;
            
        case CircuitState::OPEN: {
            auto now = std::chrono::steady_clock::now();
            if (now - lastFailureTime_ >= timeout_) {
                state_ = CircuitState::HALF_OPEN;
                lastRetryTime_ = now;
                return true;
            }
            return false;
        }
        
        case CircuitState::HALF_OPEN: {
            auto now = std::chrono::steady_clock::now();
            if (now - lastRetryTime_ >= retryTimeout_) {
                return true;
            }
            return false;
        }
    }
    
    return false;
}

void CircuitBreaker::recordSuccess() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (state_.load() == CircuitState::HALF_OPEN) {
        state_ = CircuitState::CLOSED;
        failureCount_ = 0;
    }
    
    successCount_++;
}

void CircuitBreaker::recordFailure() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    failureCount_++;
    lastFailureTime_ = std::chrono::steady_clock::now();
    
    if (failureCount_ >= failureThreshold_) {
        state_ = CircuitState::OPEN;
    }
}

CircuitState CircuitBreaker::getState() const {
    return state_.load();
}

int CircuitBreaker::getFailureCount() const {
    return failureCount_.load();
}

int CircuitBreaker::getSuccessCount() const {
    return successCount_.load();
}

void CircuitBreaker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = CircuitState::CLOSED;
    failureCount_ = 0;
    successCount_ = 0;
    lastFailureTime_ = std::chrono::steady_clock::now();
    lastRetryTime_ = std::chrono::steady_clock::now();
}

void CircuitBreaker::updateState() {
    // 狀態更新邏輯已在各個方法中处理
}
