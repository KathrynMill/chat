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
    int failureThreshold = 5;           // 失敗閾值
    int successThreshold = 3;           // 成功閾值（半開狀態）
    std::chrono::milliseconds timeout = std::chrono::milliseconds(60000);  // 超時時間
    std::chrono::milliseconds resetTimeout = std::chrono::milliseconds(30000);  // 重置超時
    bool enableFallback = true;         // 是否啟用降級
};

// 熔斷器管理器
class CircuitBreakerManager {
public:
    static CircuitBreakerManager& getInstance();
    
    // 初始化熔斷器管理器
    bool initialize();
    
    // 獲取或創建熔斷器
    CircuitBreaker* getCircuitBreaker(const std::string& serviceName, 
                                    const CircuitBreakerConfig& config = CircuitBreakerConfig{});
    
    // 執行受保護的調用
    template<typename Func, typename FallbackFunc>
    auto execute(const std::string& serviceName, 
                Func&& func, 
                FallbackFunc&& fallback,
                const CircuitBreakerConfig& config = CircuitBreakerConfig{}) 
                -> decltype(func()) {
        
        auto* cb = getCircuitBreaker(serviceName, config);
        if (!cb) {
            // 如果熔斷器不可用，直接執行原函數
            return func();
        }
        
        // 檢查熔斷器狀態
        if (cb->getState() == CircuitBreaker::State::OPEN) {
            // 熔斷器開啟，執行降級函數
            if (config.enableFallback) {
                return fallback();
            } else {
                throw std::runtime_error("Circuit breaker is OPEN for service: " + serviceName);
            }
        }
        
        try {
            // 執行原函數
            auto result = func();
            
            // 記錄成功
            cb->recordSuccess();
            return result;
            
        } catch (const std::exception& e) {
            // 記錄失敗
            cb->recordFailure();
            throw;
        }
    }
    
    // 手動重置熔斷器
    void resetCircuitBreaker(const std::string& serviceName);
    
    // 獲取熔斷器狀態
    CircuitBreaker::State getCircuitBreakerState(const std::string& serviceName);
    
    // 獲取熔斷器統計
    struct CircuitBreakerStats {
        std::string serviceName;
        CircuitBreaker::State state;
        int failureCount;
        int successCount;
        std::chrono::system_clock::time_point lastFailureTime;
        std::chrono::system_clock::time_point lastSuccessTime;
    };
    CircuitBreakerStats getCircuitBreakerStats(const std::string& serviceName);
    
    // 獲取所有熔斷器統計
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
    
    // 默認配置
    CircuitBreakerConfig defaultConfig_;
    
    // 清理配置
    std::chrono::minutes cleanupInterval_;
    std::chrono::system_clock::time_point lastCleanupTime_;
};
