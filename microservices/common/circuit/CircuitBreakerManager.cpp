#include "CircuitBreakerManager.h"
#include <iostream>
#include <algorithm>

CircuitBreakerManager& CircuitBreakerManager::getInstance() {
    static CircuitBreakerManager instance;
    return instance;
}

bool CircuitBreakerManager::initialize() {
    // 設置默認配置
    defaultConfig_.failureThreshold = 5;
    defaultConfig_.successThreshold = 3;
    defaultConfig_.timeout = std::chrono::milliseconds(5000);
    defaultConfig_.resetTimeout = std::chrono::milliseconds(30000);
    defaultConfig_.enableFallback = true;
    
    cleanupInterval_ = std::chrono::minutes(10);
    lastCleanupTime_ = std::chrono::system_clock::now();
    
    std::cout << "CircuitBreakerManager initialized\n";
    return true;
}

CircuitBreaker* CircuitBreakerManager::getCircuitBreaker(const std::string& serviceName, 
                                                       const CircuitBreakerConfig& config) {
    std::lock_guard<std::mutex> lock(circuitBreakersMutex_);
    
    auto it = circuitBreakers_.find(serviceName);
    if (it != circuitBreakers_.end()) {
        return it->second.get();
    }
    
    // 創建新的熔斷器
    auto cb = std::make_unique<CircuitBreaker>(
        config.failureThreshold,
        config.successThreshold,
        config.timeout,
        config.resetTimeout
    );
    
    auto* cbPtr = cb.get();
    circuitBreakers_[serviceName] = std::move(cb);
    
    std::cout << "Created circuit breaker for service: " << serviceName << "\n";
    return cbPtr;
}

void CircuitBreakerManager::resetCircuitBreaker(const std::string& serviceName) {
    std::lock_guard<std::mutex> lock(circuitBreakersMutex_);
    
    auto it = circuitBreakers_.find(serviceName);
    if (it != circuitBreakers_.end()) {
        it->second->reset();
        std::cout << "Reset circuit breaker for service: " << serviceName << "\n";
    }
}

CircuitBreaker::State CircuitBreakerManager::getCircuitBreakerState(const std::string& serviceName) {
    std::lock_guard<std::mutex> lock(circuitBreakersMutex_);
    
    auto it = circuitBreakers_.find(serviceName);
    if (it != circuitBreakers_.end()) {
        return it->second->getState();
    }
    
    return CircuitBreaker::State::CLOSED;
}

CircuitBreakerManager::CircuitBreakerStats CircuitBreakerManager::getCircuitBreakerStats(const std::string& serviceName) {
    CircuitBreakerStats stats;
    stats.serviceName = serviceName;
    
    std::lock_guard<std::mutex> lock(circuitBreakersMutex_);
    
    auto it = circuitBreakers_.find(serviceName);
    if (it != circuitBreakers_.end()) {
        stats.state = it->second->getState();
        stats.failureCount = it->second->getFailureCount();
        stats.successCount = it->second->getSuccessCount();
        stats.lastFailureTime = it->second->getLastFailureTime();
        stats.lastSuccessTime = it->second->getLastSuccessTime();
    } else {
        stats.state = CircuitBreaker::State::CLOSED;
        stats.failureCount = 0;
        stats.successCount = 0;
    }
    
    return stats;
}

std::vector<CircuitBreakerManager::CircuitBreakerStats> CircuitBreakerManager::getAllCircuitBreakerStats() {
    std::vector<CircuitBreakerStats> allStats;
    
    std::lock_guard<std::mutex> lock(circuitBreakersMutex_);
    
    for (const auto& pair : circuitBreakers_) {
        CircuitBreakerStats stats;
        stats.serviceName = pair.first;
        stats.state = pair.second->getState();
        stats.failureCount = pair.second->getFailureCount();
        stats.successCount = pair.second->getSuccessCount();
        stats.lastFailureTime = pair.second->getLastFailureTime();
        stats.lastSuccessTime = pair.second->getLastSuccessTime();
        
        allStats.push_back(stats);
    }
    
    return allStats;
}

void CircuitBreakerManager::cleanupInactiveCircuitBreakers() {
    auto now = std::chrono::system_clock::now();
    
    // 檢查是否需要清理
    if (now - lastCleanupTime_ < cleanupInterval_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(circuitBreakersMutex_);
    
    int removedCount = 0;
    auto it = circuitBreakers_.begin();
    
    while (it != circuitBreakers_.end()) {
        // 如果熔斷器處於關閉狀態且長時間沒有活動，則清理
        if (it->second->getState() == CircuitBreaker::State::CLOSED) {
            auto lastActivity = std::max(it->second->getLastFailureTime(), 
                                       it->second->getLastSuccessTime());
            
            if (now - lastActivity > std::chrono::hours(1)) {
                it = circuitBreakers_.erase(it);
                removedCount++;
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
    
    lastCleanupTime_ = now;
    
    if (removedCount > 0) {
        std::cout << "Cleaned up " << removedCount << " inactive circuit breakers\n";
    }
}
