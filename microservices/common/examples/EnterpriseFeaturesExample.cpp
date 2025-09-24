#include <iostream>
#include <thread>
#include <chrono>

// 包含所有企業級功能
#include "config/ConfigManager.h"
#include "security/TlsManager.h"
#include "logging/Logger.h"
#include "discovery/ServiceDiscovery.h"
#include "auth/AuthManager.h"
#include "circuit/CircuitBreakerManager.h"
#include "retry/RetryManager.h"
#include "tracing/Tracer.h"
#include "metrics/MetricsCollector.h"
#include "db/ConnectionPool.h"

// 企業級功能使用示例
class EnterpriseFeaturesExample {
public:
    void runExample() {
        std::cout << "🚀 企業級微服務功能示例\n";
        std::cout << "========================\n\n";
        
        // 1. 配置管理
        demonstrateConfigManagement();
        
        // 2. 結構化日誌
        demonstrateStructuredLogging();
        
        // 3. TLS 加密
        demonstrateTlsEncryption();
        
        // 4. 服務發現
        demonstrateServiceDiscovery();
        
        // 5. 認證授權
        demonstrateAuthentication();
        
        // 6. 熔斷器
        demonstrateCircuitBreaker();
        
        // 7. 重試機制
        demonstrateRetryMechanism();
        
        // 8. 分散式追蹤
        demonstrateDistributedTracing();
        
        // 9. 指標監控
        demonstrateMetricsCollection();
        
        // 10. 資料庫連接池
        demonstrateConnectionPool();
        
        std::cout << "\n✅ 所有企業級功能示例完成！\n";
    }

private:
    void demonstrateConfigManagement() {
        std::cout << "📋 1. 統一配置管理示例\n";
        std::cout << "------------------------\n";
        
        // 初始化配置管理器
        auto& configManager = ConfigManager::getInstance();
        configManager.initialize("http://127.0.0.1:8500", "chat/", true);
        
        // 從環境變數加載配置
        configManager.loadFromEnvironment();
        
        // 設置配置值
        configManager.setString("service.name", "chat-service");
        configManager.setInt("service.port", 7000);
        configManager.setBool("service.enable_tls", true);
        
        // 獲取配置值
        std::string serviceName = configManager.getString("service.name", "default-service");
        int port = configManager.getInt("service.port", 8080);
        bool enableTls = configManager.getBool("service.enable_tls", false);
        
        std::cout << "服務名稱: " << serviceName << "\n";
        std::cout << "服務端口: " << port << "\n";
        std::cout << "啟用 TLS: " << (enableTls ? "是" : "否") << "\n";
        
        // 註冊配置變更回調
        configManager.registerChangeCallback("service.port", 
            [](const std::string& key, const std::string& oldValue, const std::string& newValue) {
                std::cout << "配置變更: " << key << " 從 " << oldValue << " 變更為 " << newValue << "\n";
            });
        
        // 獲取配置統計
        auto stats = configManager.getConfigStats();
        std::cout << "配置統計: 總數=" << stats.totalConfigs 
                  << ", 必填=" << stats.requiredConfigs 
                  << ", 環境變數=" << stats.environmentConfigs << "\n\n";
    }
    
    void demonstrateStructuredLogging() {
        std::cout << "📝 2. 結構化日誌系統示例\n";
        std::cout << "------------------------\n";
        
        // 初始化日誌器
        auto& logger = Logger::getInstance();
        
        LogConfig logConfig;
        logConfig.level = LogLevel::DEBUG;
        logConfig.format = LogFormat::JSON;
        logConfig.output = LogOutput::CONSOLE;
        logConfig.enableAsync = true;
        logConfig.enableColor = true;
        
        logger.initialize(logConfig);
        
        // 基本日誌記錄
        LOG_INFO("服務啟動成功");
        LOG_WARN("配置文件中缺少某些可選參數");
        LOG_ERROR("資料庫連接失敗");
        
        // 帶字段的結構化日誌
        std::unordered_map<std::string, std::string> fields = {
            {"user_id", "12345"},
            {"action", "login"},
            {"ip", "192.168.1.100"},
            {"duration_ms", "150"}
        };
        
        LOG_INFO_FIELDS("用戶登入成功", fields);
        
        // 性能日誌
        auto start = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::unordered_map<std::string, std::string> perfFields = {
            {"operation", "database_query"},
            {"duration_ms", std::to_string(duration.count())},
            {"rows_affected", "5"}
        };
        
        LOG_DEBUG_FIELDS("資料庫查詢完成", perfFields);
        
        // 獲取日誌統計
        auto logStats = logger.getStats();
        std::cout << "日誌統計: 總數=" << logStats.totalLogs.load() 
                  << ", INFO=" << logStats.infoLogs.load() 
                  << ", ERROR=" << logStats.errorLogs.load() << "\n\n";
    }
    
    void demonstrateTlsEncryption() {
        std::cout << "🔒 3. TLS 加密通信示例\n";
        std::cout << "----------------------\n";
        
        // 初始化 TLS 管理器
        auto& tlsManager = TlsManager::getInstance();
        tlsManager.initialize();
        
        // 生成自簽名證書
        std::string certFile = "/tmp/chat.crt";
        std::string keyFile = "/tmp/chat.key";
        
        bool certGenerated = tlsManager.generateSelfSignedCertificate(
            certFile, keyFile, "chat.example.com", 365);
        
        if (certGenerated) {
            std::cout << "自簽名證書生成成功\n";
            
            // 獲取證書信息
            auto certInfo = tlsManager.getCertificateInfo(certFile);
            std::cout << "證書主體: " << certInfo.subject << "\n";
            std::cout << "證書頒發者: " << certInfo.issuer << "\n";
            std::cout << "有效期至: " << certInfo.notAfter << "\n";
            std::cout << "是否過期: " << (certInfo.isExpired ? "是" : "否") << "\n";
            std::cout << "剩餘天數: " << certInfo.daysUntilExpiry << "\n";
        }
        
        // 配置 TLS
        TlsConfig tlsConfig;
        tlsConfig.certFile = certFile;
        tlsConfig.keyFile = keyFile;
        tlsConfig.verifyPeer = true;
        tlsConfig.minVersion = TLS1_2_VERSION;
        tlsConfig.maxVersion = TLS1_3_VERSION;
        
        bool contextCreated = tlsManager.createSslContext(tlsConfig);
        if (contextCreated) {
            std::cout << "SSL 上下文創建成功\n";
        }
        
        // 獲取 TLS 統計
        auto tlsStats = tlsManager.getTlsStats();
        std::cout << "TLS 統計: 總連接=" << tlsStats.totalConnections 
                  << ", 活躍連接=" << tlsStats.activeConnections 
                  << ", 握手失敗=" << tlsStats.handshakeFailures << "\n\n";
    }
    
    void demonstrateServiceDiscovery() {
        std::cout << "🔍 4. 服務發現與負載均衡示例\n";
        std::cout << "----------------------------\n";
        
        // 初始化服務發現
        auto& serviceDiscovery = ServiceDiscovery::getInstance();
        serviceDiscovery.initialize("http://127.0.0.1:8500");
        
        // 註冊服務
        std::unordered_map<std::string, std::string> tags = {
            {"version", "1.0.0"},
            {"environment", "production"}
        };
        
        bool registered = serviceDiscovery.registerService(
            "user-service", "user-1", "127.0.0.1", 60051, tags);
        
        if (registered) {
            std::cout << "服務註冊成功\n";
        }
        
        // 設置負載均衡策略
        serviceDiscovery.setLoadBalanceStrategy("user-service", LoadBalanceStrategy::ROUND_ROBIN);
        
        // 獲取服務實例
        auto instance = serviceDiscovery.getInstance("user-service", LoadBalanceStrategy::ROUND_ROBIN);
        if (!instance.id.empty()) {
            std::cout << "獲取服務實例: " << instance.name 
                      << " (" << instance.getEndpoint() << ")\n";
        }
        
        // 獲取健康實例
        auto healthyInstances = serviceDiscovery.getHealthyInstances("user-service");
        std::cout << "健康實例數量: " << healthyInstances.size() << "\n";
        
        // 獲取服務統計
        auto stats = serviceDiscovery.getServiceStats("user-service");
        std::cout << "服務統計: 總實例=" << stats.totalInstances 
                  << ", 健康實例=" << stats.healthyInstances 
                  << ", 不健康實例=" << stats.unhealthyInstances << "\n\n";
    }
    
    void demonstrateAuthentication() {
        std::cout << "🔐 5. 認證與授權示例\n";
        std::cout << "-------------------\n";
        
        // 初始化認證管理器
        auto& authManager = AuthManager::getInstance();
        authManager.initialize("your-jwt-secret-key", 60, 30);
        
        // 用戶認證
        auto authResult = authManager.authenticate("alice", "password123");
        if (authResult.success) {
            std::cout << "用戶認證成功: " << authResult.username 
                      << " (ID: " << authResult.userId << ")\n";
            
            // 創建 JWT Token
            std::string token = authManager.createToken(
                authResult.userId, authResult.username, authResult.permissions);
            std::cout << "JWT Token 創建成功\n";
            
            // 驗證 Token
            auto validation = authManager.validateToken(token);
            if (validation.success) {
                std::cout << "Token 驗證成功: " << validation.username << "\n";
                
                // 檢查權限
                bool canChat = authManager.hasPermission(token, "chat");
                std::cout << "聊天權限: " << (canChat ? "有" : "無") << "\n";
            }
        }
        
        // 獲取會話統計
        auto sessionStats = authManager.getSessionStats();
        std::cout << "會話統計: 總會話=" << sessionStats.totalSessions 
                  << ", 活躍會話=" << sessionStats.activeSessions 
                  << ", 過期會話=" << sessionStats.expiredSessions << "\n\n";
    }
    
    void demonstrateCircuitBreaker() {
        std::cout << "⚡ 6. 熔斷器示例\n";
        std::cout << "---------------\n";
        
        // 初始化熔斷器管理器
        auto& circuitBreakerManager = CircuitBreakerManager::getInstance();
        circuitBreakerManager.initialize();
        
        // 模擬服務調用
        auto result = circuitBreakerManager.execute(
            "user-service",
            []() -> std::string {
                // 模擬服務調用
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return "服務調用成功";
            },
            []() -> std::string {
                // 降級策略
                return "服務降級響應";
            }
        );
        
        std::cout << "服務調用結果: " << result << "\n";
        
        // 獲取熔斷器狀態
        auto state = circuitBreakerManager.getCircuitBreakerState("user-service");
        std::string stateStr;
        switch (state) {
            case CircuitBreaker::State::CLOSED: stateStr = "關閉"; break;
            case CircuitBreaker::State::OPEN: stateStr = "開啟"; break;
            case CircuitBreaker::State::HALF_OPEN: stateStr = "半開"; break;
        }
        std::cout << "熔斷器狀態: " << stateStr << "\n";
        
        // 獲取熔斷器統計
        auto stats = circuitBreakerManager.getCircuitBreakerStats("user-service");
        std::cout << "熔斷器統計: 失敗次數=" << stats.failureCount 
                  << ", 成功次數=" << stats.successCount << "\n\n";
    }
    
    void demonstrateRetryMechanism() {
        std::cout << "🔄 7. 重試機制示例\n";
        std::cout << "-----------------\n";
        
        // 初始化重試管理器
        auto& retryManager = RetryManager::getInstance();
        retryManager.initialize();
        
        // 配置重試策略
        RetryConfig retryConfig;
        retryConfig.maxAttempts = 3;
        retryConfig.strategy = RetryStrategy::EXPONENTIAL_BACKOFF;
        retryConfig.initialDelay = std::chrono::milliseconds(100);
        retryConfig.maxDelay = std::chrono::milliseconds(1000);
        retryConfig.timeout = std::chrono::milliseconds(5000);
        
        // 執行重試
        auto result = retryManager.execute(
            [](int attempt) -> std::string {
                if (attempt < 2) {
                    throw std::runtime_error("模擬失敗");
                }
                return "重試成功";
            },
            retryConfig
        );
        
        std::cout << "重試結果: " << (result.success ? result.value : "失敗") << "\n";
        std::cout << "重試次數: " << result.attempts << "\n";
        std::cout << "總耗時: " << result.totalDuration.count() << "ms\n\n";
    }
    
    void demonstrateDistributedTracing() {
        std::cout << "🔍 8. 分散式追蹤示例\n";
        std::cout << "-------------------\n";
        
        // 初始化追蹤器
        auto& tracer = Tracer::getInstance();
        tracer.initialize("chat-service", "http://localhost:14268/api/traces");
        
        // 生成追蹤 ID
        std::string traceId = tracer.generateTraceId();
        std::string spanId = tracer.generateSpanId();
        
        std::cout << "追蹤 ID: " << traceId << "\n";
        std::cout << "Span ID: " << spanId << "\n";
        
        // 創建根 span
        std::unordered_map<std::string, std::string> attributes = {
            {"service.name", "chat-service"},
            {"operation", "user_login"},
            {"user.id", "12345"}
        };
        
        auto rootSpan = tracer.startSpanWithContext("user-login", traceId, "", attributes);
        if (rootSpan) {
            std::cout << "根 Span 創建成功\n";
            
            // 創建子 span
            auto childSpan = tracer.startChildSpan("database-query", rootSpan, {
                {"query", "SELECT * FROM users WHERE id = ?"},
                {"duration_ms", "50"}
            });
            
            if (childSpan) {
                std::cout << "子 Span 創建成功\n";
                
                // 添加事件
                tracer.addEvent(childSpan, "query-executed", {
                    {"rows_affected", "1"}
                });
                
                // 結束子 span
                tracer.endSpan(childSpan, true);
            }
            
            // 結束根 span
            tracer.endSpan(rootSpan, true);
        }
        
        std::cout << "分散式追蹤完成\n\n";
    }
    
    void demonstrateMetricsCollection() {
        std::cout << "📊 9. 指標監控示例\n";
        std::cout << "-----------------\n";
        
        // 初始化指標收集器
        auto& metricsCollector = MetricsCollector::getInstance();
        metricsCollector.initialize("chat-service", 8080);
        
        // 記錄業務指標
        metricsCollector.recordOnlineUsers(150);
        metricsCollector.recordActiveConnections(75);
        
        // 記錄技術指標
        metricsCollector.recordGrpcCall("user-service", "GetUser", true, 25.5);
        metricsCollector.recordHttpRequest("POST", "/api/login", 200, 100.0);
        metricsCollector.recordDatabaseQuery("SELECT", true, 15.0);
        metricsCollector.recordKafkaMessage("user-events", "produce", true);
        
        // 獲取指標數據
        std::string metrics = metricsCollector.getMetrics();
        std::cout << "指標數據:\n" << metrics << "\n";
        
        std::cout << "指標監控完成\n\n";
    }
    
    void demonstrateConnectionPool() {
        std::cout << "🗄️ 10. 資料庫連接池示例\n";
        std::cout << "----------------------\n";
        
        // 初始化連接池
        auto& connectionPool = ConnectionPool::getInstance();
        
        DbConfig dbConfig;
        dbConfig.host = "127.0.0.1";
        dbConfig.port = 3306;
        dbConfig.user = "root";
        dbConfig.password = "";
        dbConfig.database = "chatdb";
        
        ConnectionPoolConfig poolConfig;
        poolConfig.minConnections = 2;
        poolConfig.maxConnections = 10;
        poolConfig.initialConnections = 5;
        
        bool initialized = connectionPool.initialize(dbConfig, poolConfig);
        if (initialized) {
            std::cout << "連接池初始化成功\n";
            
            // 執行資料庫操作
            try {
                auto result = connectionPool.executeWithConnection(
                    [](DbConnection& conn) -> std::string {
                        // 模擬資料庫查詢
                        return conn.querySingleString("SELECT 'Hello from database'");
                    }
                );
                std::cout << "資料庫查詢結果: " << result << "\n";
            } catch (const std::exception& e) {
                std::cout << "資料庫查詢失敗: " << e.what() << "\n";
            }
            
            // 獲取連接池統計
            auto stats = connectionPool.getStats();
            std::cout << "連接池統計: 總連接=" << stats.totalConnections 
                      << ", 活躍連接=" << stats.activeConnections 
                      << ", 空閒連接=" << stats.idleConnections 
                      << ", 等待請求=" << stats.waitingRequests << "\n";
        }
        
        std::cout << "資料庫連接池示例完成\n\n";
    }
};

int main() {
    EnterpriseFeaturesExample example;
    example.runExample();
    return 0;
}
