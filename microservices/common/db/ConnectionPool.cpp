#include "ConnectionPool.h"
#include <iostream>
#include <algorithm>

ConnectionPool& ConnectionPool::getInstance() {
    static ConnectionPool instance;
    return instance;
}

bool ConnectionPool::initialize(const DbConfig& config, const ConnectionPoolConfig& poolConfig) {
    dbConfig_ = config;
    poolConfig_ = poolConfig;
    
    totalConnections_ = 0;
    activeConnections_ = 0;
    totalRequests_ = 0;
    successfulRequests_ = 0;
    failedRequests_ = 0;
    waitingRequests_ = 0;
    running_ = true;
    
    // 創建初始連接
    for (int i = 0; i < poolConfig_.initialConnections; ++i) {
        auto connection = createConnection();
        if (connection) {
            auto pooledConn = std::make_shared<PooledConnection>(connection);
            availableConnections_.push(pooledConn);
            allConnections_.push_back(pooledConn);
            totalConnections_++;
        }
    }
    
    if (totalConnections_ == 0) {
        std::cerr << "Failed to create any initial connections\n";
        return false;
    }
    
    // 啟動健康檢查線程
    if (poolConfig_.enableHealthCheck) {
        healthCheckThread_ = std::thread(&ConnectionPool::healthCheckThread, this);
    }
    
    // 啟動清理線程
    cleanupThread_ = std::thread(&ConnectionPool::cleanupThread, this);
    
    std::cout << "ConnectionPool initialized with " << totalConnections_ << " connections\n";
    return true;
}

std::shared_ptr<DbConnection> ConnectionPool::getConnection() {
    std::unique_lock<std::mutex> lock(poolMutex_);
    waitingRequests_++;
    totalRequests_++;
    
    // 等待可用連接
    poolCondition_.wait(lock, [this] {
        return !availableConnections_.empty() || !running_;
    });
    
    waitingRequests_--;
    
    if (!running_) {
        return nullptr;
    }
    
    // 獲取連接
    auto pooledConn = availableConnections_.front();
    availableConnections_.pop();
    
    // 檢查連接是否健康
    if (!isConnectionHealthy(pooledConn)) {
        // 創建新連接替換
        auto newConnection = createConnection();
        if (newConnection) {
            pooledConn = std::make_shared<PooledConnection>(newConnection);
            // 替換舊連接
            auto it = std::find(allConnections_.begin(), allConnections_.end(), pooledConn);
            if (it != allConnections_.end()) {
                *it = pooledConn;
            }
        } else {
            // 無法創建新連接，返回 nullptr
            return nullptr;
        }
    }
    
    pooledConn->lastUsedAt = std::chrono::system_clock::now();
    activeConnections_++;
    
    return pooledConn->connection;
}

void ConnectionPool::returnConnection(std::shared_ptr<DbConnection> connection) {
    if (!connection) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    // 找到對應的池化連接
    auto it = std::find_if(allConnections_.begin(), allConnections_.end(),
        [&connection](const std::shared_ptr<PooledConnection>& pooledConn) {
            return pooledConn->connection == connection;
        });
    
    if (it != allConnections_.end()) {
        (*it)->lastUsedAt = std::chrono::system_clock::now();
        availableConnections_.push(*it);
        activeConnections_--;
        successfulRequests_++;
    } else {
        failedRequests_++;
    }
    
    poolCondition_.notify_one();
}

ConnectionPoolStats ConnectionPool::getStats() {
    ConnectionPoolStats stats;
    
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    stats.totalConnections = totalConnections_.load();
    stats.activeConnections = activeConnections_.load();
    stats.idleConnections = availableConnections_.size();
    stats.waitingRequests = waitingRequests_.load();
    stats.totalRequests = totalRequests_.load();
    stats.successfulRequests = successfulRequests_.load();
    stats.failedRequests = failedRequests_.load();
    stats.lastHealthCheck = std::chrono::system_clock::now();
    
    return stats;
}

bool ConnectionPool::healthCheck() {
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    int healthyCount = 0;
    for (const auto& pooledConn : allConnections_) {
        if (isConnectionHealthy(pooledConn)) {
            healthyCount++;
        }
    }
    
    return healthyCount > 0;
}

void ConnectionPool::cleanupExpiredConnections() {
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    auto now = std::chrono::system_clock::now();
    std::vector<std::shared_ptr<PooledConnection>> toRemove;
    
    for (auto it = allConnections_.begin(); it != allConnections_.end();) {
        auto& pooledConn = *it;
        
        // 檢查是否過期
        bool isExpired = false;
        
        // 檢查最大生命週期
        if (now - pooledConn->createdAt > poolConfig_.maxLifetime) {
            isExpired = true;
        }
        
        // 檢查空閒超時
        if (now - pooledConn->lastUsedAt > poolConfig_.idleTimeout) {
            isExpired = true;
        }
        
        if (isExpired) {
            toRemove.push_back(pooledConn);
            it = allConnections_.erase(it);
            totalConnections_--;
        } else {
            ++it;
        }
    }
    
    // 從可用連接隊列中移除過期連接
    std::queue<std::shared_ptr<PooledConnection>> newAvailableConnections;
    while (!availableConnections_.empty()) {
        auto pooledConn = availableConnections_.front();
        availableConnections_.pop();
        
        if (std::find(toRemove.begin(), toRemove.end(), pooledConn) == toRemove.end()) {
            newAvailableConnections.push(pooledConn);
        }
    }
    availableConnections_ = newAvailableConnections;
    
    if (!toRemove.empty()) {
        std::cout << "Cleaned up " << toRemove.size() << " expired connections\n";
    }
}

void ConnectionPool::shutdown() {
    running_ = false;
    
    // 通知所有等待的線程
    {
        std::lock_guard<std::mutex> lock(poolMutex_);
        poolCondition_.notify_all();
    }
    
    // 等待線程結束
    if (healthCheckThread_.joinable()) {
        healthCheckThread_.join();
    }
    
    if (cleanupThread_.joinable()) {
        cleanupThread_.join();
    }
    
    // 清理所有連接
    std::lock_guard<std::mutex> lock(poolMutex_);
    allConnections_.clear();
    
    while (!availableConnections_.empty()) {
        availableConnections_.pop();
    }
    
    std::cout << "ConnectionPool shutdown\n";
}

std::shared_ptr<DbConnection> ConnectionPool::createConnection() {
    try {
        auto connection = std::make_shared<DbConnection>(dbConfig_);
        if (connection->connect()) {
            return connection;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to create database connection: " << e.what() << "\n";
    }
    
    return nullptr;
}

void ConnectionPool::markConnectionAsBad(std::shared_ptr<DbConnection> connection) {
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    // 找到並標記為不健康
    for (auto& pooledConn : allConnections_) {
        if (pooledConn->connection == connection) {
            pooledConn->isHealthy = false;
            break;
        }
    }
    
    failedRequests_++;
}

void ConnectionPool::healthCheckThread() {
    while (running_) {
        try {
            std::this_thread::sleep_for(poolConfig_.healthCheckInterval);
            
            if (!running_) break;
            
            std::lock_guard<std::mutex> lock(poolMutex_);
            
            for (auto& pooledConn : allConnections_) {
                if (!isConnectionHealthy(pooledConn)) {
                    pooledConn->isHealthy = false;
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Health check thread error: " << e.what() << "\n";
        }
    }
}

void ConnectionPool::cleanupThread() {
    while (running_) {
        try {
            std::this_thread::sleep_for(std::chrono::minutes(5));
            
            if (!running_) break;
            
            cleanupExpiredConnections();
            
        } catch (const std::exception& e) {
            std::cerr << "Cleanup thread error: " << e.what() << "\n";
        }
    }
}

bool ConnectionPool::isConnectionHealthy(std::shared_ptr<PooledConnection> pooledConn) {
    if (!pooledConn || !pooledConn->connection) {
        return false;
    }
    
    try {
        // 執行簡單的查詢來檢查連接健康
        auto result = pooledConn->connection->querySingleString("SELECT 1");
        return !result.empty() && result == "1";
    } catch (const std::exception& e) {
        std::cerr << "Connection health check failed: " << e.what() << "\n";
        return false;
    }
}
