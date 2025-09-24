#include <iostream>
#include <thread>
#include <chrono>
#include <random>

#include "security/TlsIntegration.h"
#include "config/ConfigManager.h"
#include "observability/ObservabilityManager.h"

// 模擬 gRPC 服務調用
class MockGrpcService {
public:
    std::string callUserService(const std::string& method, const std::string& request) {
        // 模擬網絡延遲
        std::this_thread::sleep_for(std::chrono::milliseconds(50 + (rand() % 100)));
        
        // 模擬偶爾的錯誤
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

// 模擬資料庫操作
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
        
        // 獲取 TLS 統計信息
        auto stats = tlsIntegration.getStats();
        std::cout << "TLS 統計信息:\n";
        std::cout << "  - TLS 啟用: " << (stats.tlsEnabled ? "是" : "否") << "\n";
        std::cout << "  - 當前加密套件: " << stats.currentCipher << "\n";
        std::cout << "  - 當前協議: " << stats.currentProtocol << "\n";
        std::cout << "  - 握手失敗次數: " << stats.handshakeFailures << "\n";
        
        // 模擬證書重新加載
        if (tlsIntegration.reloadCertificates()) {
            std::cout << "✓ 證書重新加載成功\n";
        }
    } else {
        std::cout << "✗ TLS 集成初始化失敗\n";
    }
}

void demonstrateConfigHotReload() {
    std::cout << "\n=== 配置熱更新演示 ===\n";
    
    // 初始化配置管理器
    auto& configManager = ConfigManager::getInstance();
    configManager.initialize("chat-service", "consul://localhost:8500/v1/kv/chat/", true);
    
    // 設置一些初始配置
    configManager.setString("database.host", "localhost");
    configManager.setInt("database.port", 3306);
    configManager.setBool("service.enable_tls", true);
    configManager.setString("log.level", "info");
    
    std::cout << "初始配置:\n";
    std::cout << "  - 資料庫主機: " << configManager.getString("database.host") << "\n";
    std::cout << "  - 資料庫端口: " << configManager.getInt("database.port") << "\n";
    std::cout << "  - TLS 啟用: " << (configManager.getBool("service.enable_tls") ? "是" : "否") << "\n";
    std::cout << "  - 日誌級別: " << configManager.getString("log.level") << "\n";
    
    // 模擬配置變更
    std::cout << "\n模擬配置熱更新...\n";
    configManager.setString("log.level", "debug");
    configManager.setInt("database.port", 3307);
    configManager.setBool("service.enable_tls", false);
    
    std::cout << "更新後配置:\n";
    std::cout << "  - 資料庫主機: " << configManager.getString("database.host") << "\n";
    std::cout << "  - 資料庫端口: " << configManager.getInt("database.port") << "\n";
    std::cout << "  - TLS 啟用: " << (configManager.getBool("service.enable_tls") ? "是" : "否") << "\n";
    std::cout << "  - 日誌級別: " << configManager.getString("log.level") << "\n";
    
    // 獲取配置統計信息
    auto stats = configManager.getStats();
    std::cout << "\n配置統計信息:\n";
    std::cout << "  - 總配置項數: " << stats.totalConfigs << "\n";
    std::cout << "  - 環境變數配置: " << stats.environmentConfigs << "\n";
    std::cout << "  - Consul 配置: " << stats.consulConfigs << "\n";
    std::cout << "  - 文件配置: " << stats.fileConfigs << "\n";
    std::cout << "  - 熱更新啟用: " << (stats.hotReloadEnabled ? "是" : "否") << "\n";
}

void demonstrateObservabilityIntegration() {
    std::cout << "\n=== 日誌與指標集成演示 ===\n";
    
    // 初始化可觀測性管理器
    auto& observability = ObservabilityManager::getInstance();
    if (!observability.initialize("chat-service", "info", "8080", "http://localhost:14268/api/traces")) {
        std::cout << "✗ 可觀測性系統初始化失敗\n";
        return;
    }
    
    std::cout << "✓ 可觀測性系統初始化成功\n";
    
    // 創建模擬服務
    MockGrpcService grpcService;
    MockDatabase database;
    
    // 模擬多個 gRPC 調用
    std::cout << "\n模擬 gRPC 服務調用...\n";
    for (int i = 0; i < 10; ++i) {
        try {
            // 使用可觀測性包裝的 gRPC 調用
            auto result = observability.executeWithObservability(
                "user_service_call",
                "UserService",
                "GetUser",
                [&grpcService, i]() {
                    return grpcService.callUserService("GetUser", "user_id_" + std::to_string(i));
                }
            );
            
            std::cout << "  - 調用 " << i << ": " << result << "\n";
            
        } catch (const std::exception& e) {
            std::cout << "  - 調用 " << i << " 失敗: " << e.what() << "\n";
        }
        
        // 短暫延遲
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 模擬資料庫操作
    std::cout << "\n模擬資料庫操作...\n";
    for (int i = 0; i < 5; ++i) {
        try {
            // 使用可觀測性包裝的資料庫操作
            auto result = observability.executeDatabaseWithObservability(
                "insert_user",
                "users",
                [&database, i]() {
                    return database.insertUser("user_data_" + std::to_string(i));
                }
            );
            
            std::cout << "  - 插入用戶 " << i << ": " << (result ? "成功" : "失敗") << "\n";
            
        } catch (const std::exception& e) {
            std::cout << "  - 插入用戶 " << i << " 失敗: " << e.what() << "\n";
        }
    }
    
    // 模擬業務操作
    std::cout << "\n模擬業務操作...\n";
    for (int i = 0; i < 3; ++i) {
        observability.logBusinessOperation(
            "send_message",
            "user_" + std::to_string(i),
            "success",
            "Message sent to group chat"
        );
        
        observability.recordBusinessMetrics("send_message", "success", 1);
        
        std::cout << "  - 用戶 " << i << " 發送消息: 成功\n";
    }
    
    // 模擬分散式追蹤
    std::cout << "\n模擬分散式追蹤...\n";
    std::string traceId = observability.startSpan("user_login", "", {{"user_id", "123"}, {"ip", "192.168.1.100"}});
    std::cout << "  - 開始追蹤 span: " << traceId << "\n";
    
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
    
    // 獲取統計信息
    auto stats = observability.getStats();
    std::cout << "\n可觀測性統計信息:\n";
    std::cout << "  - 服務名稱: " << stats.serviceName << "\n";
    std::cout << "  - gRPC 調用總數: " << stats.grpcCalls.load() << "\n";
    std::cout << "  - gRPC 錯誤數: " << stats.grpcErrors.load() << "\n";
    std::cout << "  - 資料庫操作數: " << stats.dbOperations.load() << "\n";
    std::cout << "  - 資料庫錯誤數: " << stats.dbErrors.load() << "\n";
    std::cout << "  - 業務操作數: " << stats.businessOperations.load() << "\n";
    std::cout << "  - 創建 span 數: " << stats.spansCreated.load() << "\n";
    std::cout << "  - 完成 span 數: " << stats.spansFinished.load() << "\n";
    std::cout << "  - 日誌級別: " << stats.logLevel << "\n";
    std::cout << "  - 指標端口: " << stats.metricsPort << "\n";
    std::cout << "  - Jaeger 啟用: " << (stats.jaegerEnabled ? "是" : "否") << "\n";
    
    // 關閉可觀測性系統
    observability.shutdown();
    std::cout << "✓ 可觀測性系統已關閉\n";
}

void demonstrateIntegratedWorkflow() {
    std::cout << "\n=== 集成工作流程演示 ===\n";
    
    // 1. 初始化所有系統
    std::cout << "1. 初始化所有系統...\n";
    
    // TLS 集成
    TlsIntegrationConfig tlsConfig;
    tlsConfig.enableTls = true;
    auto& tlsIntegration = TlsIntegration::getInstance();
    tlsIntegration.initialize(tlsConfig);
    
    // 配置管理
    auto& configManager = ConfigManager::getInstance();
    configManager.initialize("chat-service", "consul://localhost:8500/v1/kv/chat/", true);
    
    // 可觀測性
    auto& observability = ObservabilityManager::getInstance();
    observability.initialize("chat-service", "info", "8080", "http://localhost:14268/api/traces");
    
    std::cout << "✓ 所有系統初始化完成\n";
    
    // 2. 模擬完整的用戶登錄流程
    std::cout << "\n2. 模擬用戶登錄流程...\n";
    
    // 開始追蹤
    std::string traceId = observability.startSpan("user_login_flow", "", {{"user_id", "123"}});
    
    try {
        // 驗證用戶憑證
        observability.logBusinessOperation("validate_credentials", "123", "start", "JWT token validation");
        
        // 模擬 gRPC 調用
        auto userResult = observability.executeWithObservability(
            "get_user_info",
            "UserService",
            "GetUser",
            []() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return std::string("User info retrieved");
            }
        );
        
        // 模擬資料庫操作
        auto dbResult = observability.executeDatabaseWithObservability(
            "update_last_login",
            "users",
            []() {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                return true;
            }
        );
        
        // 記錄成功
        observability.logBusinessOperation("user_login", "123", "success", "Login completed successfully");
        observability.recordBusinessMetrics("user_login", "success", 1);
        
        std::cout << "✓ 用戶登錄成功\n";
        
    } catch (const std::exception& e) {
        // 記錄失敗
        observability.logBusinessOperation("user_login", "123", "error", e.what());
        observability.recordBusinessMetrics("user_login", "error", 1);
        
        std::cout << "✗ 用戶登錄失敗: " << e.what() << "\n";
    }
    
    // 完成追蹤
    observability.finishSpan(traceId, "ok");
    
    // 3. 顯示所有統計信息
    std::cout << "\n3. 系統統計信息:\n";
    
    // TLS 統計
    auto tlsStats = tlsIntegration.getStats();
    std::cout << "TLS 統計:\n";
    std::cout << "  - TLS 啟用: " << (tlsStats.tlsEnabled ? "是" : "否") << "\n";
    std::cout << "  - 握手失敗: " << tlsStats.handshakeFailures << "\n";
    
    // 配置統計
    auto configStats = configManager.getStats();
    std::cout << "配置統計:\n";
    std::cout << "  - 總配置項: " << configStats.totalConfigs << "\n";
    std::cout << "  - 熱更新啟用: " << (configStats.hotReloadEnabled ? "是" : "否") << "\n";
    
    // 可觀測性統計
    auto obsStats = observability.getStats();
    std::cout << "可觀測性統計:\n";
    std::cout << "  - gRPC 調用: " << obsStats.grpcCalls.load() << "\n";
    std::cout << "  - 資料庫操作: " << obsStats.dbOperations.load() << "\n";
    std::cout << "  - 業務操作: " << obsStats.businessOperations.load() << "\n";
    std::cout << "  - 追蹤 span: " << obsStats.spansCreated.load() << "\n";
    
    // 4. 清理資源
    std::cout << "\n4. 清理資源...\n";
    observability.shutdown();
    std::cout << "✓ 資源清理完成\n";
}

int main() {
    std::cout << "=== 企業級微服務框架進階功能演示 ===\n";
    
    // 設置隨機種子
    srand(time(nullptr));
    
    try {
        // 演示 TLS 加密集成
        demonstrateTlsIntegration();
        
        // 演示配置熱更新
        demonstrateConfigHotReload();
        
        // 演示日誌與指標集成
        demonstrateObservabilityIntegration();
        
        // 演示集成工作流程
        demonstrateIntegratedWorkflow();
        
        std::cout << "\n=== 所有演示完成 ===\n";
        std::cout << "✓ TLS 加密集成 - 完成\n";
        std::cout << "✓ 配置熱更新 - 完成\n";
        std::cout << "✓ 日誌與指標集成 - 完成\n";
        std::cout << "✓ 集成工作流程 - 完成\n";
        
    } catch (const std::exception& e) {
        std::cerr << "演示過程中發生錯誤: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
