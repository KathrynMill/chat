#include <iostream>
#include <thread>
#include <chrono>

// 包含所有企業级功能
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

// 企業级功能使用示例
class EnterpriseFeaturesExample {
public:
    void runExample() {
        std::cout << "🚀 企業级微服务功能示例\n";
        std::cout << "========================\n\n";
        
        // 1. 配置管理
        demonstrateConfigManagement();
        
        // 2. 結构化日誌
        demonstrateStructuredLogging();
        
        // 3. TLS 加密
        demonstrateTlsEncryption();
        
        // 4. 服务发现
        demonstrateServiceDiscovery();
        
        // 5. 认证授权
        demonstrateAuthentication();
        
        // 6. 熔斷器
        demonstrateCircuitBreaker();
        
        // 7. 重试機制
        demonstrateRetryMechanism();
        
        // 8. 分散式追蹤
        demonstrateDistributedTracing();
        
        // 9. 指标监控
        demonstrateMetricsCollection();
        
        // 10. 资料庫连接池
        demonstrateConnectionPool();
        
        std::cout << "\n✅ 所有企業级功能示例完成！\n";
    }

private:
    void demonstrateConfigManagement() {
        std::cout << "📋 1. 统一配置管理示例\n";
        std::cout << "------------------------\n";
        
        // 初始化配置管理器
        auto& configManager = ConfigManager::getInstance();
        configManager.initialize("http://127.0.0.1:8500", "chat/", true);
        
        // 從环境变数加载配置
        configManager.loadFromEnvironment();
        
        // 设置配置值
        configManager.setString("service.name", "chat-service");
        configManager.setInt("service.port", 7000);
        configManager.setBool("service.enable_tls", true);
        
        // 获取配置值
        std::string serviceName = configManager.getString("service.name", "default-service");
        int port = configManager.getInt("service.port", 8080);
        bool enableTls = configManager.getBool("service.enable_tls", false);
        
        std::cout << "服务名稱: " << serviceName << "\n";
        std::cout << "服务端口: " << port << "\n";
        std::cout << "启用 TLS: " << (enableTls ? "是" : "否") << "\n";
        
        // 註冊配置变更回调
        configManager.registerChangeCallback("service.port", 
            [](const std::string& key, const std::string& oldValue, const std::string& newValue) {
                std::cout << "配置变更: " << key << " 從 " << oldValue << " 变更為 " << newValue << "\n";
            });
        
        // 获取配置统計
        auto stats = configManager.getConfigStats();
        std::cout << "配置统計: 總数=" << stats.totalConfigs 
                  << ", 必填=" << stats.requiredConfigs 
                  << ", 环境变数=" << stats.environmentConfigs << "\n\n";
    }
    
    void demonstrateStructuredLogging() {
        std::cout << "📝 2. 結构化日誌系统示例\n";
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
        
        // 基本日誌記录
        LOG_INFO("服务启动成功");
        LOG_WARN("配置文件中缺少某些可選參数");
        LOG_ERROR("资料庫连接失败");
        
        // 帶字段的結构化日誌
        std::unordered_map<std::string, std::string> fields = {
            {"user_id", "12345"},
            {"action", "login"},
            {"ip", "192.168.1.100"},
            {"duration_ms", "150"}
        };
        
        LOG_INFO_FIELDS("用户登入成功", fields);
        
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
        
        LOG_DEBUG_FIELDS("资料庫查询完成", perfFields);
        
        // 获取日誌统計
        auto logStats = logger.getStats();
        std::cout << "日誌统計: 總数=" << logStats.totalLogs.load() 
                  << ", INFO=" << logStats.infoLogs.load() 
                  << ", ERROR=" << logStats.errorLogs.load() << "\n\n";
    }
    
    void demonstrateTlsEncryption() {
        std::cout << "🔒 3. TLS 加密通信示例\n";
        std::cout << "----------------------\n";
        
        // 初始化 TLS 管理器
        auto& tlsManager = TlsManager::getInstance();
        tlsManager.initialize();
        
        // 生成自簽名证書
        std::string certFile = "/tmp/chat.crt";
        std::string keyFile = "/tmp/chat.key";
        
        bool certGenerated = tlsManager.generateSelfSignedCertificate(
            certFile, keyFile, "chat.example.com", 365);
        
        if (certGenerated) {
            std::cout << "自簽名证書生成成功\n";
            
            // 获取证書信息
            auto certInfo = tlsManager.getCertificateInfo(certFile);
            std::cout << "证書主體: " << certInfo.subject << "\n";
            std::cout << "证書頒发者: " << certInfo.issuer << "\n";
            std::cout << "有效期至: " << certInfo.notAfter << "\n";
            std::cout << "是否过期: " << (certInfo.isExpired ? "是" : "否") << "\n";
            std::cout << "剩餘天数: " << certInfo.daysUntilExpiry << "\n";
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
            std::cout << "SSL 上下文创建成功\n";
        }
        
        // 获取 TLS 统計
        auto tlsStats = tlsManager.getTlsStats();
        std::cout << "TLS 统計: 總连接=" << tlsStats.totalConnections 
                  << ", 活躍连接=" << tlsStats.activeConnections 
                  << ", 握手失败=" << tlsStats.handshakeFailures << "\n\n";
    }
    
    void demonstrateServiceDiscovery() {
        std::cout << "🔍 4. 服务发现與負载均衡示例\n";
        std::cout << "----------------------------\n";
        
        // 初始化服务发现
        auto& serviceDiscovery = ServiceDiscovery::getInstance();
        serviceDiscovery.initialize("http://127.0.0.1:8500");
        
        // 註冊服务
        std::unordered_map<std::string, std::string> tags = {
            {"version", "1.0.0"},
            {"environment", "production"}
        };
        
        bool registered = serviceDiscovery.registerService(
            "user-service", "user-1", "127.0.0.1", 60051, tags);
        
        if (registered) {
            std::cout << "服务註冊成功\n";
        }
        
        // 设置負载均衡策略
        serviceDiscovery.setLoadBalanceStrategy("user-service", LoadBalanceStrategy::ROUND_ROBIN);
        
        // 获取服务实例
        auto instance = serviceDiscovery.getInstance("user-service", LoadBalanceStrategy::ROUND_ROBIN);
        if (!instance.id.empty()) {
            std::cout << "获取服务实例: " << instance.name 
                      << " (" << instance.getEndpoint() << ")\n";
        }
        
        // 获取健康实例
        auto healthyInstances = serviceDiscovery.getHealthyInstances("user-service");
        std::cout << "健康实例数量: " << healthyInstances.size() << "\n";
        
        // 获取服务统計
        auto stats = serviceDiscovery.getServiceStats("user-service");
        std::cout << "服务统計: 總实例=" << stats.totalInstances 
                  << ", 健康实例=" << stats.healthyInstances 
                  << ", 不健康实例=" << stats.unhealthyInstances << "\n\n";
    }
    
    void demonstrateAuthentication() {
        std::cout << "🔐 5. 认证與授权示例\n";
        std::cout << "-------------------\n";
        
        // 初始化认证管理器
        auto& authManager = AuthManager::getInstance();
        authManager.initialize("your-jwt-secret-key", 60, 30);
        
        // 用户认证
        auto authResult = authManager.authenticate("alice", "password123");
        if (authResult.success) {
            std::cout << "用户认证成功: " << authResult.username 
                      << " (ID: " << authResult.userId << ")\n";
            
            // 创建 JWT Token
            std::string token = authManager.createToken(
                authResult.userId, authResult.username, authResult.permissions);
            std::cout << "JWT Token 创建成功\n";
            
            // 验证 Token
            auto validation = authManager.validateToken(token);
            if (validation.success) {
                std::cout << "Token 验证成功: " << validation.username << "\n";
                
                // 检查权限
                bool canChat = authManager.hasPermission(token, "chat");
                std::cout << "聊天权限: " << (canChat ? "有" : "無") << "\n";
            }
        }
        
        // 获取會话统計
        auto sessionStats = authManager.getSessionStats();
        std::cout << "會话统計: 總會话=" << sessionStats.totalSessions 
                  << ", 活躍會话=" << sessionStats.activeSessions 
                  << ", 过期會话=" << sessionStats.expiredSessions << "\n\n";
    }
    
    void demonstrateCircuitBreaker() {
        std::cout << "⚡ 6. 熔斷器示例\n";
        std::cout << "---------------\n";
        
        // 初始化熔斷器管理器
        auto& circuitBreakerManager = CircuitBreakerManager::getInstance();
        circuitBreakerManager.initialize();
        
        // 模擬服务调用
        auto result = circuitBreakerManager.execute(
            "user-service",
            []() -> std::string {
                // 模擬服务调用
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return "服务调用成功";
            },
            []() -> std::string {
                // 降级策略
                return "服务降级響應";
            }
        );
        
        std::cout << "服务调用結果: " << result << "\n";
        
        // 获取熔斷器狀態
        auto state = circuitBreakerManager.getCircuitBreakerState("user-service");
        std::string stateStr;
        switch (state) {
            case CircuitBreaker::State::CLOSED: stateStr = "关闭"; break;
            case CircuitBreaker::State::OPEN: stateStr = "开启"; break;
            case CircuitBreaker::State::HALF_OPEN: stateStr = "半开"; break;
        }
        std::cout << "熔斷器狀態: " << stateStr << "\n";
        
        // 获取熔斷器统計
        auto stats = circuitBreakerManager.getCircuitBreakerStats("user-service");
        std::cout << "熔斷器统計: 失败次数=" << stats.failureCount 
                  << ", 成功次数=" << stats.successCount << "\n\n";
    }
    
    void demonstrateRetryMechanism() {
        std::cout << "🔄 7. 重试機制示例\n";
        std::cout << "-----------------\n";
        
        // 初始化重试管理器
        auto& retryManager = RetryManager::getInstance();
        retryManager.initialize();
        
        // 配置重试策略
        RetryConfig retryConfig;
        retryConfig.maxAttempts = 3;
        retryConfig.strategy = RetryStrategy::EXPONENTIAL_BACKOFF;
        retryConfig.initialDelay = std::chrono::milliseconds(100);
        retryConfig.maxDelay = std::chrono::milliseconds(1000);
        retryConfig.timeout = std::chrono::milliseconds(5000);
        
        // 执行重试
        auto result = retryManager.execute(
            [](int attempt) -> std::string {
                if (attempt < 2) {
                    throw std::runtime_error("模擬失败");
                }
                return "重试成功";
            },
            retryConfig
        );
        
        std::cout << "重试結果: " << (result.success ? result.value : "失败") << "\n";
        std::cout << "重试次数: " << result.attempts << "\n";
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
        
        // 创建根 span
        std::unordered_map<std::string, std::string> attributes = {
            {"service.name", "chat-service"},
            {"operation", "user_login"},
            {"user.id", "12345"}
        };
        
        auto rootSpan = tracer.startSpanWithContext("user-login", traceId, "", attributes);
        if (rootSpan) {
            std::cout << "根 Span 创建成功\n";
            
            // 创建子 span
            auto childSpan = tracer.startChildSpan("database-query", rootSpan, {
                {"query", "SELECT * FROM users WHERE id = ?"},
                {"duration_ms", "50"}
            });
            
            if (childSpan) {
                std::cout << "子 Span 创建成功\n";
                
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
        std::cout << "📊 9. 指标监控示例\n";
        std::cout << "-----------------\n";
        
        // 初始化指标收集器
        auto& metricsCollector = MetricsCollector::getInstance();
        metricsCollector.initialize("chat-service", 8080);
        
        // 記录業务指标
        metricsCollector.recordOnlineUsers(150);
        metricsCollector.recordActiveConnections(75);
        
        // 記录技术指标
        metricsCollector.recordGrpcCall("user-service", "GetUser", true, 25.5);
        metricsCollector.recordHttpRequest("POST", "/api/login", 200, 100.0);
        metricsCollector.recordDatabaseQuery("SELECT", true, 15.0);
        metricsCollector.recordKafkaMessage("user-events", "produce", true);
        
        // 获取指标数据
        std::string metrics = metricsCollector.getMetrics();
        std::cout << "指标数据:\n" << metrics << "\n";
        
        std::cout << "指标监控完成\n\n";
    }
    
    void demonstrateConnectionPool() {
        std::cout << "🗄️ 10. 资料庫连接池示例\n";
        std::cout << "----------------------\n";
        
        // 初始化连接池
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
            std::cout << "连接池初始化成功\n";
            
            // 执行资料庫操作
            try {
                auto result = connectionPool.executeWithConnection(
                    [](DbConnection& conn) -> std::string {
                        // 模擬资料庫查询
                        return conn.querySingleString("SELECT 'Hello from database'");
                    }
                );
                std::cout << "资料庫查询結果: " << result << "\n";
            } catch (const std::exception& e) {
                std::cout << "资料庫查询失败: " << e.what() << "\n";
            }
            
            // 获取连接池统計
            auto stats = connectionPool.getStats();
            std::cout << "连接池统計: 總连接=" << stats.totalConnections 
                      << ", 活躍连接=" << stats.activeConnections 
                      << ", 空閒连接=" << stats.idleConnections 
                      << ", 等待請求=" << stats.waitingRequests << "\n";
        }
        
        std::cout << "资料庫连接池示例完成\n\n";
    }
};

int main() {
    EnterpriseFeaturesExample example;
    example.runExample();
    return 0;
}
