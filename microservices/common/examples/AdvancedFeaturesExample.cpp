#include <iostream>
#include <thread>
#include <chrono>
#include <random>

#include "security/TlsIntegration.h"
#include "config/ConfigManager.h"
#include "observability/ObservabilityManager.h"

// 模擬 gRPC 服务调用
class MockGrpcService {
public:
    std::string callUserService(const std::string& method, const std::string& request) {
        // 模擬网絡延遲
        std::this_thread::sleep_for(std::chrono::milliseconds(50 + (rand() % 100)));
        
        // 模擬偶爾的错误
        if (rand() % 10 == 0) {
            throw std::runtime_error("gRPC call failed: network timeout");
        }
        
        return "Response from UserService." + method;
    }
    
    std::string callMessageService(const std::string& method, const std::string& request) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30 + (rand() % 80)));
        
        if (rand() % 15 == 0) {
            throw std::runtime_error("gRPC call failed: service unavailable");
        }
        
        return "Response from MessageService." + method;
    }
};

// 模擬资料庫操作
class MockDatabase {
public:
    bool insertUser(const std::string& userData) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20 + (rand() % 50)));
        
        if (rand() % 20 == 0) {
            throw std::runtime_error("Database error: connection lost");
        }
        
        return true;
    }
    
    std::vector<std::string> queryMessages(const std::string& userId) {
        std::this_thread::sleep_for(std::chrono::milliseconds(40 + (rand() % 60)));
        
        if (rand() % 25 == 0) {
            throw std::runtime_error("Database error: query timeout");
        }
        
        return {"message1", "message2", "message3"};
    }
};

void demonstrateTlsIntegration() {
    std::cout << "\n=== TLS 加密集成演示 ===\n";
    
    // 初始化 TLS 集成
    TlsIntegrationConfig tlsConfig;
    tlsConfig.enableTls = true;
    tlsConfig.certFile = "/tmp/server.crt";
    tlsConfig.keyFile = "/tmp/server.key";
    tlsConfig.caFile = "/tmp/ca.crt";
    tlsConfig.verifyPeer = true;
    tlsConfig.cipherSuites = "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256";
    
    auto& tlsIntegration = TlsIntegration::getInstance();
    if (tlsIntegration.initialize(tlsConfig)) {
        std::cout << "✓ TLS 集成初始化成功\n";
        
        // 获取 TLS 统計信息
        auto stats = tlsIntegration.getStats();
        std::cout << "TLS 统計信息:\n";
        std::cout << "  - TLS 启用: " << (stats.tlsEnabled ? "是" : "否") << "\n";
        std::cout << "  - 當前加密套件: " << stats.currentCipher << "\n";
        std::cout << "  - 當前協議: " << stats.currentProtocol << "\n";
        std::cout << "  - 握手失败次数: " << stats.handshakeFailures << "\n";
        
        // 模擬证書重新加载
        if (tlsIntegration.reloadCertificates()) {
            std::cout << "✓ 证書重新加载成功\n";
        }
    } else {
        std::cout << "✗ TLS 集成初始化失败\n";
    }
}

void demonstrateConfigHotReload() {
    std::cout << "\n=== 配置熱更新演示 ===\n";
    
    // 初始化配置管理器
    auto& configManager = ConfigManager::getInstance();
    configManager.initialize("chat-service", "consul://localhost:8500/v1/kv/chat/", true);
    
    // 设置一些初始配置
    configManager.setString("database.host", "localhost");
    configManager.setInt("database.port", 3306);
    configManager.setBool("service.enable_tls", true);
    configManager.setString("log.level", "info");
    
    std::cout << "初始配置:\n";
    std::cout << "  - 资料庫主機: " << configManager.getString("database.host") << "\n";
    std::cout << "  - 资料庫端口: " << configManager.getInt("database.port") << "\n";
    std::cout << "  - TLS 启用: " << (configManager.getBool("service.enable_tls") ? "是" : "否") << "\n";
    std::cout << "  - 日誌级別: " << configManager.getString("log.level") << "\n";
    
    // 模擬配置变更
    std::cout << "\n模擬配置熱更新...\n";
    configManager.setString("log.level", "debug");
    configManager.setInt("database.port", 3307);
    configManager.setBool("service.enable_tls", false);
    
    std::cout << "更新後配置:\n";
    std::cout << "  - 资料庫主機: " << configManager.getString("database.host") << "\n";
    std::cout << "  - 资料庫端口: " << configManager.getInt("database.port") << "\n";
    std::cout << "  - TLS 启用: " << (configManager.getBool("service.enable_tls") ? "是" : "否") << "\n";
    std::cout << "  - 日誌级別: " << configManager.getString("log.level") << "\n";
    
    // 获取配置统計信息
    auto stats = configManager.getStats();
    std::cout << "\n配置统計信息:\n";
    std::cout << "  - 總配置项数: " << stats.totalConfigs << "\n";
    std::cout << "  - 环境变数配置: " << stats.environmentConfigs << "\n";
    std::cout << "  - Consul 配置: " << stats.consulConfigs << "\n";
    std::cout << "  - 文件配置: " << stats.fileConfigs << "\n";
    std::cout << "  - 熱更新启用: " << (stats.hotReloadEnabled ? "是" : "否") << "\n";
}

void demonstrateObservabilityIntegration() {
    std::cout << "\n=== 日誌與指标集成演示 ===\n";
    
    // 初始化可觀测性管理器
    auto& observability = ObservabilityManager::getInstance();
    if (!observability.initialize("chat-service", "info", "8080", "http://localhost:14268/api/traces")) {
        std::cout << "✗ 可觀测性系统初始化失败\n";
        return;
    }
    
    std::cout << "✓ 可觀测性系统初始化成功\n";
    
    // 创建模擬服务
    MockGrpcService grpcService;
    MockDatabase database;
    
    // 模擬多個 gRPC 调用
    std::cout << "\n模擬 gRPC 服务调用...\n";
    for (int i = 0; i < 10; ++i) {
        try {
            // 使用可觀测性包裝的 gRPC 调用
            auto result = observability.executeWithObservability(
                "user_service_call",
                "UserService",
                "GetUser",
                [&grpcService, i]() {
                    return grpcService.callUserService("GetUser", "user_id_" + std::to_string(i));
                }
            );
            
            std::cout << "  - 调用 " << i << ": " << result << "\n";
            
        } catch (const std::exception& e) {
            std::cout << "  - 调用 " << i << " 失败: " << e.what() << "\n";
        }
        
        // 短暫延遲
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 模擬资料庫操作
    std::cout << "\n模擬资料庫操作...\n";
    for (int i = 0; i < 5; ++i) {
        try {
            // 使用可觀测性包裝的资料庫操作
            auto result = observability.executeDatabaseWithObservability(
                "insert_user",
                "users",
                [&database, i]() {
                    return database.insertUser("user_data_" + std::to_string(i));
                }
            );
            
            std::cout << "  - 插入用户 " << i << ": " << (result ? "成功" : "失败") << "\n";
            
        } catch (const std::exception& e) {
            std::cout << "  - 插入用户 " << i << " 失败: " << e.what() << "\n";
        }
    }
    
    // 模擬業务操作
    std::cout << "\n模擬業务操作...\n";
    for (int i = 0; i < 3; ++i) {
        observability.logBusinessOperation(
            "send_message",
            "user_" + std::to_string(i),
            "success",
            "Message sent to group chat"
        );
        
        observability.recordBusinessMetrics("send_message", "success", 1);
        
        std::cout << "  - 用户 " << i << " 发送消息: 成功\n";
    }
    
    // 模擬分散式追蹤
    std::cout << "\n模擬分散式追蹤...\n";
    std::string traceId = observability.startSpan("user_login", "", {{"user_id", "123"}, {"ip", "192.168.1.100"}});
    std::cout << "  - 开始追蹤 span: " << traceId << "\n";
    
    // 添加子 span
    std::string childSpanId = observability.startSpan("validate_credentials", traceId, {{"method", "jwt"}});
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    observability.finishSpan(childSpanId, "ok");
    std::cout << "  - 完成子 span: " << childSpanId << "\n";
    
    // 添加事件
    observability.addSpanEvent(traceId, "user_authenticated", {{"user_id", "123"}, {"timestamp", "2024-01-01T12:00:00Z"}});
    
    // 完成主 span
    observability.finishSpan(traceId, "ok");
    std::cout << "  - 完成主 span: " << traceId << "\n";
    
    // 获取统計信息
    auto stats = observability.getStats();
    std::cout << "\n可觀测性统計信息:\n";
    std::cout << "  - 服务名稱: " << stats.serviceName << "\n";
    std::cout << "  - gRPC 调用總数: " << stats.grpcCalls.load() << "\n";
    std::cout << "  - gRPC 错误数: " << stats.grpcErrors.load() << "\n";
    std::cout << "  - 资料庫操作数: " << stats.dbOperations.load() << "\n";
    std::cout << "  - 资料庫错误数: " << stats.dbErrors.load() << "\n";
    std::cout << "  - 業务操作数: " << stats.businessOperations.load() << "\n";
    std::cout << "  - 创建 span 数: " << stats.spansCreated.load() << "\n";
    std::cout << "  - 完成 span 数: " << stats.spansFinished.load() << "\n";
    std::cout << "  - 日誌级別: " << stats.logLevel << "\n";
    std::cout << "  - 指标端口: " << stats.metricsPort << "\n";
    std::cout << "  - Jaeger 启用: " << (stats.jaegerEnabled ? "是" : "否") << "\n";
    
    // 关闭可觀测性系统
    observability.shutdown();
    std::cout << "✓ 可觀测性系统已关闭\n";
}

void demonstrateIntegratedWorkflow() {
    std::cout << "\n=== 集成工作流程演示 ===\n";
    
    // 1. 初始化所有系统
    std::cout << "1. 初始化所有系统...\n";
    
    // TLS 集成
    TlsIntegrationConfig tlsConfig;
    tlsConfig.enableTls = true;
    auto& tlsIntegration = TlsIntegration::getInstance();
    tlsIntegration.initialize(tlsConfig);
    
    // 配置管理
    auto& configManager = ConfigManager::getInstance();
    configManager.initialize("chat-service", "consul://localhost:8500/v1/kv/chat/", true);
    
    // 可觀测性
    auto& observability = ObservabilityManager::getInstance();
    observability.initialize("chat-service", "info", "8080", "http://localhost:14268/api/traces");
    
    std::cout << "✓ 所有系统初始化完成\n";
    
    // 2. 模擬完整的用户登录流程
    std::cout << "\n2. 模擬用户登录流程...\n";
    
    // 开始追蹤
    std::string traceId = observability.startSpan("user_login_flow", "", {{"user_id", "123"}});
    
    try {
        // 验证用户憑证
        observability.logBusinessOperation("validate_credentials", "123", "start", "JWT token validation");
        
        // 模擬 gRPC 调用
        auto userResult = observability.executeWithObservability(
            "get_user_info",
            "UserService",
            "GetUser",
            []() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return std::string("User info retrieved");
            }
        );
        
        // 模擬资料庫操作
        auto dbResult = observability.executeDatabaseWithObservability(
            "update_last_login",
            "users",
            []() {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                return true;
            }
        );
        
        // 記录成功
        observability.logBusinessOperation("user_login", "123", "success", "Login completed successfully");
        observability.recordBusinessMetrics("user_login", "success", 1);
        
        std::cout << "✓ 用户登录成功\n";
        
    } catch (const std::exception& e) {
        // 記录失败
        observability.logBusinessOperation("user_login", "123", "error", e.what());
        observability.recordBusinessMetrics("user_login", "error", 1);
        
        std::cout << "✗ 用户登录失败: " << e.what() << "\n";
    }
    
    // 完成追蹤
    observability.finishSpan(traceId, "ok");
    
    // 3. 顯示所有统計信息
    std::cout << "\n3. 系统统計信息:\n";
    
    // TLS 统計
    auto tlsStats = tlsIntegration.getStats();
    std::cout << "TLS 统計:\n";
    std::cout << "  - TLS 启用: " << (tlsStats.tlsEnabled ? "是" : "否") << "\n";
    std::cout << "  - 握手失败: " << tlsStats.handshakeFailures << "\n";
    
    // 配置统計
    auto configStats = configManager.getStats();
    std::cout << "配置统計:\n";
    std::cout << "  - 總配置项: " << configStats.totalConfigs << "\n";
    std::cout << "  - 熱更新启用: " << (configStats.hotReloadEnabled ? "是" : "否") << "\n";
    
    // 可觀测性统計
    auto obsStats = observability.getStats();
    std::cout << "可觀测性统計:\n";
    std::cout << "  - gRPC 调用: " << obsStats.grpcCalls.load() << "\n";
    std::cout << "  - 资料庫操作: " << obsStats.dbOperations.load() << "\n";
    std::cout << "  - 業务操作: " << obsStats.businessOperations.load() << "\n";
    std::cout << "  - 追蹤 span: " << obsStats.spansCreated.load() << "\n";
    
    // 4. 清理资源
    std::cout << "\n4. 清理资源...\n";
    observability.shutdown();
    std::cout << "✓ 资源清理完成\n";
}

int main() {
    std::cout << "=== 企業级微服务框架进阶功能演示 ===\n";
    
    // 设置隨機種子
    srand(time(nullptr));
    
    try {
        // 演示 TLS 加密集成
        demonstrateTlsIntegration();
        
        // 演示配置熱更新
        demonstrateConfigHotReload();
        
        // 演示日誌與指标集成
        demonstrateObservabilityIntegration();
        
        // 演示集成工作流程
        demonstrateIntegratedWorkflow();
        
        std::cout << "\n=== 所有演示完成 ===\n";
        std::cout << "✓ TLS 加密集成 - 完成\n";
        std::cout << "✓ 配置熱更新 - 完成\n";
        std::cout << "✓ 日誌與指标集成 - 完成\n";
        std::cout << "✓ 集成工作流程 - 完成\n";
        
    } catch (const std::exception& e) {
        std::cerr << "演示过程中发生错误: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
