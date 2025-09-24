#pragma once
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <thread>
#include <chrono>

#include "Db.h"

// 連接池配置
struct ConnectionPoolConfig {
    int minConnections = 2;           // 最小連接數
    int maxConnections = 10;          // 最大連接數
    int initialConnections = 5;       // 初始連接數
    std::chrono::seconds connectionTimeout = std::chrono::seconds(30);  // 連接超時
    std::chrono::seconds idleTimeout = std::chrono::seconds(300);       // 空閒超時
    std::chrono::seconds maxLifetime = std::chrono::seconds(3600);      // 最大生命週期
    bool enableHealthCheck = true;    // 啟用健康檢查
    std::chrono::seconds healthCheckInterval = std::chrono::seconds(60); // 健康檢查間隔
};

// 連接池統計
struct ConnectionPoolStats {
    int totalConnections;
    int activeConnections;
    int idleConnections;
    int waitingRequests;
    long totalRequests;
    long successfulRequests;
    long failedRequests;
    std::chrono::system_clock::time_point lastHealthCheck;
};

// 資料庫連接池
class ConnectionPool {
public:
    static ConnectionPool& getInstance();
    
    // 初始化連接池
    bool initialize(const DbConfig& config, const ConnectionPoolConfig& poolConfig = ConnectionPoolConfig{});
    
    // 獲取連接
    std::shared_ptr<DbConnection> getConnection();
    
    // 歸還連接
    void returnConnection(std::shared_ptr<DbConnection> connection);
    
    // 執行查詢（自動管理連接）
    template<typename Func>
    auto executeWithConnection(Func&& func) -> decltype(func(std::declval<DbConnection&>())) {
        auto connection = getConnection();
        if (!connection) {
            throw std::runtime_error("Failed to get database connection");
        }
        
        try {
            auto result = func(*connection);
            returnConnection(connection);
            return result;
        } catch (const std::exception& e) {
            // 連接可能已損壞，不歸還到池中
            markConnectionAsBad(connection);
            throw;
        }
    }
    
    // 獲取連接池統計
    ConnectionPoolStats getStats();
    
    // 健康檢查
    bool healthCheck();
    
    // 清理過期連接
    void cleanupExpiredConnections();
    
    // 關閉連接池
    void shutdown();

private:
    ConnectionPool() = default;
    ~ConnectionPool() = default;
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;
    
    // 創建新連接
    std::shared_ptr<DbConnection> createConnection();
    
    // 標記連接為壞連接
    void markConnectionAsBad(std::shared_ptr<DbConnection> connection);
    
    // 健康檢查線程
    void healthCheckThread();
    
    // 清理線程
    void cleanupThread();
    
    // 檢查連接是否健康
    bool isConnectionHealthy(std::shared_ptr<DbConnection> connection);
    
    // 連接包裝器
    struct PooledConnection {
        std::shared_ptr<DbConnection> connection;
        std::chrono::system_clock::time_point createdAt;
        std::chrono::system_clock::time_point lastUsedAt;
        bool isHealthy;
        
        PooledConnection(std::shared_ptr<DbConnection> conn) 
            : connection(conn), createdAt(std::chrono::system_clock::now()), 
              lastUsedAt(createdAt), isHealthy(true) {}
    };
    
    // 配置
    DbConfig dbConfig_;
    ConnectionPoolConfig poolConfig_;
    
    // 連接池
    std::queue<std::shared_ptr<PooledConnection>> availableConnections_;
    std::vector<std::shared_ptr<PooledConnection>> allConnections_;
    std::mutex poolMutex_;
    std::condition_variable poolCondition_;
    
    // 統計
    std::atomic<int> totalConnections_;
    std::atomic<int> activeConnections_;
    std::atomic<long> totalRequests_;
    std::atomic<long> successfulRequests_;
    std::atomic<long> failedRequests_;
    
    // 線程管理
    std::atomic<bool> running_;
    std::thread healthCheckThread_;
    std::thread cleanupThread_;
    
    // 等待請求計數
    std::atomic<int> waitingRequests_;
};
