#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

enum class CircuitState {
    CLOSED,    // 正常狀態
    OPEN,      // 熔斷狀態
    HALF_OPEN  // 半开狀態
};

class CircuitBreaker {
public:
    CircuitBreaker(int failureThreshold = 5, 
                   std::chrono::milliseconds timeout = std::chrono::milliseconds(60000),
                   std::chrono::milliseconds retryTimeout = std::chrono::milliseconds(30000));
    
    // 执行操作，返回是否允許执行
    bool canExecute();
    
    // 記录成功
    void recordSuccess();
    
    // 記录失败
    void recordFailure();
    
    // 获取當前狀態
    CircuitState getState() const;
    
    // 获取统計信息
    int getFailureCount() const;
    int getSuccessCount() const;
    
    // 重置熔斷器
    void reset();

private:
    const int failureThreshold_;
    const std::chrono::milliseconds timeout_;
    const std::chrono::milliseconds retryTimeout_;
    
    std::atomic<CircuitState> state_;
    std::atomic<int> failureCount_;
    std::atomic<int> successCount_;
    std::chrono::steady_clock::time_point lastFailureTime_;
    std::chrono::steady_clock::time_point lastRetryTime_;
    
    mutable std::mutex mutex_;
    
    void updateState();
};
