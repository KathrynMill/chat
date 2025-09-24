#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <chrono>
#include <functional>

#include "CircuitBreaker.h"

// 熔斷器配置
struct CircuitBreakerConfig {
    int failureThreshold = 5;           // 失败閾值
    int successThreshold = 3;           // 成功閾值（半开狀態）
    std::chrono::milliseconds timeout = std::chrono::milliseconds(60000);  // 超時時間
    std::chrono::milliseconds resetTimeout = std::chrono::milliseconds(30000);  // 重置超時
    bool enableFallback = true;         // 是否启用降级
};

// 熔斷器管理器
class CircuitBreakerManager {
public:
    static CircuitBreakerManager& getInstance();
    
    // 初始化熔斷器管理器
    bool initialize();
    
    // 获取或创建熔斷器
    CircuitBreaker* getCircuitBreaker(const std::string& serviceName, 
                                    const CircuitBreakerConfig& config = CircuitBreakerConfig{});
    
    // 执行受保護的调用
    template<typename Func, typename FallbackFunc>
    auto execute(const std::string& serviceName, 
                Func&& func, 
                FallbackFunc&& fallback,
                const CircuitBreakerConfig& config = CircuitBreakerConfig{}) 
                -> decltype(func()) {
        
        auto* cb = getCircuitBreaker(serviceName, config);
        if (!cb) {
            // 如果熔斷器不可用，直接执行原函数
            return func();
        }
        
        // 检查熔斷器狀態
        if (cb->getState() == CircuitBreaker::State::OPEN) {
            // 熔斷器开启，执行降级函数
            if (config.enableFallback) {
                return fallback();
            } else {
                throw std::runtime_error("Circuit breaker is OPEN for service: " + serviceName);
            }
        }
        
        try {
            // 执行原函数
            auto result = func();
            
            // 記录成功
            cb->recordSuccess();
            return result;
            
        } catch (const std::exception& e) {
            // 記录失败
            cb->recordFailure();
            throw;
        }
    }
    
    // 手动重置熔斷器
    void resetCircuitBreaker(const std::string& serviceName);
    
    // 获取熔斷器狀態
    CircuitBreaker::State getCircuitBreakerState(const std::string& serviceName);
    
    // 获取熔斷器统計
    struct CircuitBreakerStats {
        std::string serviceName;
        CircuitBreaker::State state;
        int failureCount;
        int successCount;
        std::chrono::system_clock::time_point lastFailureTime;
        std::chrono::system_clock::time_point lastSuccessTime;
    };
    CircuitBreakerStats getCircuitBreakerStats(const std::string& serviceName);
    
    // 获取所有熔斷器统計
    std::vector<CircuitBreakerStats> getAllCircuitBreakerStats();
    
    // 清理不活躍的熔斷器
    void cleanupInactiveCircuitBreakers();

private:
    CircuitBreakerManager() = default;
    ~CircuitBreakerManager() = default;
    CircuitBreakerManager(const CircuitBreakerManager&) = delete;
    CircuitBreakerManager& operator=(const CircuitBreakerManager&) = delete;
    
    // 熔斷器緩存
    std::unordered_map<std::string, std::unique_ptr<CircuitBreaker>> circuitBreakers_;
    std::mutex circuitBreakersMutex_;
    
    // 默认配置
    CircuitBreakerConfig defaultConfig_;
    
    // 清理配置
    std::chrono::minutes cleanupInterval_;
    std::chrono::system_clock::time_point lastCleanupTime_;
};
