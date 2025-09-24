#pragma once
#include <string>
#include <functional>
#include <chrono>
#include <vector>
#include <exception>
#include <memory>

// 重試策略
enum class RetryStrategy {
    FIXED_DELAY,        // 固定延遲
    EXPONENTIAL_BACKOFF, // 指數退避
    LINEAR_BACKOFF      // 線性退避
};

// 重試配置
struct RetryConfig {
    int maxAttempts = 3;                                    // 最大重試次數
    std::chrono::milliseconds initialDelay = std::chrono::milliseconds(100);  // 初始延遲
    std::chrono::milliseconds maxDelay = std::chrono::milliseconds(5000);     // 最大延遲
    double backoffMultiplier = 2.0;                         // 退避倍數
    RetryStrategy strategy = RetryStrategy::EXPONENTIAL_BACKOFF;  // 重試策略
    std::chrono::milliseconds timeout = std::chrono::milliseconds(10000);     // 超時時間
    
    // 可重試的異常類型
    std::function<bool(const std::exception&)> shouldRetry = [](const std::exception& e) {
        // 默認重試所有異常
        return true;
    };
};

// 重試結果
template<typename T>
struct RetryResult {
    bool success;
    T value;
    std::string errorMessage;
    int attempts;
    std::chrono::milliseconds totalDuration;
    
    RetryResult() : success(false), attempts(0), totalDuration(0) {}
    RetryResult(bool s, const T& v, int a, std::chrono::milliseconds d) 
        : success(s), value(v), attempts(a), totalDuration(d) {}
};

// 重試管理器
class RetryManager {
public:
    static RetryManager& getInstance();
    
    // 初始化重試管理器
    bool initialize();
    
    // 執行帶重試的函數
    template<typename Func>
    auto execute(Func&& func, const RetryConfig& config = RetryConfig{}) 
        -> RetryResult<decltype(func())> {
        
        using ReturnType = decltype(func());
        RetryResult<ReturnType> result;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int attempt = 1; attempt <= config.maxAttempts; ++attempt) {
            try {
                // 設置超時
                auto timeoutStart = std::chrono::high_resolution_clock::now();
                
                // 執行函數
                auto funcResult = func();
                
                // 檢查是否超時
                auto timeoutEnd = std::chrono::high_resolution_clock::now();
                auto timeoutDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    timeoutEnd - timeoutStart);
                
                if (timeoutDuration > config.timeout) {
                    throw std::runtime_error("Function execution timeout");
                }
                
                // 成功
                result.success = true;
                result.value = funcResult;
                result.attempts = attempt;
                result.totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    timeoutEnd - startTime);
                
                return result;
                
            } catch (const std::exception& e) {
                result.errorMessage = e.what();
                result.attempts = attempt;
                
                // 檢查是否應該重試
                if (attempt >= config.maxAttempts || !config.shouldRetry(e)) {
                    // 不重試，返回失敗結果
                    auto endTime = std::chrono::high_resolution_clock::now();
                    result.totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        endTime - startTime);
                    return result;
                }
                
                // 計算延遲時間
                auto delay = calculateDelay(attempt, config);
                
                // 等待
                std::this_thread::sleep_for(delay);
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        result.totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime);
        
        return result;
    }
    
    // 執行帶重試的異步函數
    template<typename Func>
    std::future<RetryResult<decltype(std::declval<Func>()())>> executeAsync(
        Func&& func, const RetryConfig& config = RetryConfig{}) {
        
        return std::async(std::launch::async, [this, func = std::forward<Func>(func), config]() {
            return execute(func, config);
        });
    }
    
    // 創建默認重試配置
    static RetryConfig createDefaultConfig() {
        return RetryConfig{};
    }
    
    // 創建快速重試配置
    static RetryConfig createFastRetryConfig() {
        RetryConfig config;
        config.maxAttempts = 2;
        config.initialDelay = std::chrono::milliseconds(50);
        config.maxDelay = std::chrono::milliseconds(200);
        config.timeout = std::chrono::milliseconds(1000);
        return config;
    }
    
    // 創建穩健重試配置
    static RetryConfig createRobustRetryConfig() {
        RetryConfig config;
        config.maxAttempts = 5;
        config.initialDelay = std::chrono::milliseconds(500);
        config.maxDelay = std::chrono::milliseconds(10000);
        config.timeout = std::chrono::milliseconds(30000);
        return config;
    }

private:
    RetryManager() = default;
    ~RetryManager() = default;
    RetryManager(const RetryManager&) = delete;
    RetryManager& operator=(const RetryManager&) = delete;
    
    // 計算延遲時間
    std::chrono::milliseconds calculateDelay(int attempt, const RetryConfig& config) {
        std::chrono::milliseconds delay;
        
        switch (config.strategy) {
            case RetryStrategy::FIXED_DELAY:
                delay = config.initialDelay;
                break;
                
            case RetryStrategy::EXPONENTIAL_BACKOFF: {
                auto exponentialDelay = std::chrono::milliseconds(
                    static_cast<long long>(config.initialDelay.count() * 
                    std::pow(config.backoffMultiplier, attempt - 1)));
                delay = std::min(exponentialDelay, config.maxDelay);
                break;
            }
            
            case RetryStrategy::LINEAR_BACKOFF: {
                auto linearDelay = std::chrono::milliseconds(
                    config.initialDelay.count() * attempt);
                delay = std::min(linearDelay, config.maxDelay);
                break;
            }
        }
        
        return delay;
    }
};
