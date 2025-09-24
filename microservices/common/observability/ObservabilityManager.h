#pragma once
#include <string>
#include <memory>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <atomic>

#include "logging/Logger.h"
#include "metrics/MetricsCollector.h"
#include "tracing/Tracer.h"

// 可觀測性管理器 - 統一管理日誌、指標和追蹤
class ObservabilityManager {
public:
    static ObservabilityManager& getInstance();
    
    // 初始化可觀測性系統
    bool initialize(const std::string& serviceName, 
                   const std::string& logLevel = "info",
                   const std::string& metricsPort = "8080",
                   const std::string& jaegerEndpoint = "");
    
    // 關閉可觀測性系統
    void shutdown();
    
    // === 日誌記錄 ===
    
    // 記錄 gRPC 調用
    void logGrpcCall(const std::string& service, 
                    const std::string& method,
                    const std::string& status,
                    std::chrono::milliseconds duration,
                    const std::string& error = "");
    
    // 記錄資料庫操作
    void logDatabaseOperation(const std::string& operation,
                             const std::string& table,
                             const std::string& status,
                             std::chrono::milliseconds duration,
                             const std::string& error = "");
    
    // 記錄業務操作
    void logBusinessOperation(const std::string& operation,
                             const std::string& userId,
                             const std::string& status,
                             const std::string& details = "");
    
    // 記錄系統事件
    void logSystemEvent(const std::string& event,
                       const std::string& level,
                       const std::string& message,
                       const std::unordered_map<std::string, std::string>& context = {});
    
    // === 指標收集 ===
    
    // 記錄 gRPC 調用指標
    void recordGrpcMetrics(const std::string& service,
                          const std::string& method,
                          const std::string& status,
                          std::chrono::milliseconds duration);
    
    // 記錄資料庫操作指標
    void recordDatabaseMetrics(const std::string& operation,
                              const std::string& table,
                              const std::string& status,
                              std::chrono::milliseconds duration);
    
    // 記錄業務指標
    void recordBusinessMetrics(const std::string& operation,
                              const std::string& status,
                              int count = 1);
    
    // 記錄系統指標
    void recordSystemMetrics(const std::string& metric,
                            double value,
                            const std::unordered_map<std::string, std::string>& labels = {});
    
    // === 分散式追蹤 ===
    
    // 開始追蹤 span
    std::string startSpan(const std::string& operation,
                         const std::string& parentSpanId = "",
                         const std::unordered_map<std::string, std::string>& tags = {});
    
    // 結束追蹤 span
    void finishSpan(const std::string& spanId,
                   const std::string& status = "ok",
                   const std::string& error = "");
    
    // 添加 span 標籤
    void addSpanTag(const std::string& spanId,
                   const std::string& key,
                   const std::string& value);
    
    // 添加 span 事件
    void addSpanEvent(const std::string& spanId,
                     const std::string& event,
                     const std::unordered_map<std::string, std::string>& attributes = {});
    
    // === 統一操作 ===
    
    // 執行帶完整可觀測性的操作
    template<typename Func>
    auto executeWithObservability(const std::string& operation,
                                 const std::string& service,
                                 const std::string& method,
                                 Func&& func) -> decltype(func()) {
        auto startTime = std::chrono::high_resolution_clock::now();
        std::string spanId = startSpan(operation);
        
        try {
            // 記錄開始
            logSystemEvent("operation_start", "info", 
                          "Starting " + operation + " on " + service + "." + method);
            
            // 執行操作
            auto result = func();
            
            // 記錄成功
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - startTime);
            
            logGrpcCall(service, method, "success", duration);
            recordGrpcMetrics(service, method, "success", duration);
            finishSpan(spanId, "ok");
            
            return result;
            
        } catch (const std::exception& e) {
            // 記錄失敗
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - startTime);
            
            std::string error = e.what();
            logGrpcCall(service, method, "error", duration, error);
            recordGrpcMetrics(service, method, "error", duration);
            finishSpan(spanId, "error", error);
            
            throw;
        }
    }
    
    // 執行資料庫操作帶可觀測性
    template<typename Func>
    auto executeDatabaseWithObservability(const std::string& operation,
                                         const std::string& table,
                                         Func&& func) -> decltype(func()) {
        auto startTime = std::chrono::high_resolution_clock::now();
        std::string spanId = startSpan("db_" + operation);
        
        try {
            // 記錄開始
            logSystemEvent("db_operation_start", "info", 
                          "Starting " + operation + " on table " + table);
            
            // 執行操作
            auto result = func();
            
            // 記錄成功
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - startTime);
            
            logDatabaseOperation(operation, table, "success", duration);
            recordDatabaseMetrics(operation, table, "success", duration);
            finishSpan(spanId, "ok");
            
            return result;
            
        } catch (const std::exception& e) {
            // 記錄失敗
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - startTime);
            
            std::string error = e.what();
            logDatabaseOperation(operation, table, "error", duration, error);
            recordDatabaseMetrics(operation, table, "error", duration);
            finishSpan(spanId, "error", error);
            
            throw;
        }
    }
    
    // === 統計信息 ===
    
    struct ObservabilityStats {
        std::atomic<int> grpcCalls{0};
        std::atomic<int> grpcErrors{0};
        std::atomic<int> dbOperations{0};
        std::atomic<int> dbErrors{0};
        std::atomic<int> businessOperations{0};
        std::atomic<int> spansCreated{0};
        std::atomic<int> spansFinished{0};
        std::string serviceName;
        std::string logLevel;
        std::string metricsPort;
        bool jaegerEnabled;
    };
    
    ObservabilityStats getStats();
    
    // === 配置更新 ===
    
    // 更新日誌級別
    void updateLogLevel(const std::string& level);
    
    // 更新指標配置
    void updateMetricsConfig(const std::string& port);
    
    // 更新追蹤配置
    void updateTracingConfig(const std::string& endpoint);

private:
    ObservabilityManager() = default;
    ~ObservabilityManager() = default;
    ObservabilityManager(const ObservabilityManager&) = delete;
    ObservabilityManager& operator=(const ObservabilityManager&) = delete;
    
    // 組件
    std::unique_ptr<Logger> logger_;
    std::unique_ptr<MetricsCollector> metricsCollector_;
    std::unique_ptr<Tracer> tracer_;
    
    // 統計信息
    ObservabilityStats stats_;
    
    // 配置
    std::string serviceName_;
    bool initialized_;
    
    // 內部方法
    void initializeLogger(const std::string& serviceName, const std::string& logLevel);
    void initializeMetrics(const std::string& serviceName, const std::string& port);
    void initializeTracing(const std::string& serviceName, const std::string& endpoint);
    
    // 生成追蹤 ID
    std::string generateTraceId();
    std::string generateSpanId();
};
