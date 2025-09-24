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

// 可觀测性管理器 - 统一管理日誌、指标和追蹤
class ObservabilityManager {
public:
    static ObservabilityManager& getInstance();
    
    // 初始化可觀测性系统
    bool initialize(const std::string& serviceName, 
                   const std::string& logLevel = "info",
                   const std::string& metricsPort = "8080",
                   const std::string& jaegerEndpoint = "");
    
    // 关闭可觀测性系统
    void shutdown();
    
    // === 日誌記录 ===
    
    // 記录 gRPC 调用
    void logGrpcCall(const std::string& service, 
                    const std::string& method,
                    const std::string& status,
                    std::chrono::milliseconds duration,
                    const std::string& error = "");
    
    // 記录资料庫操作
    void logDatabaseOperation(const std::string& operation,
                             const std::string& table,
                             const std::string& status,
                             std::chrono::milliseconds duration,
                             const std::string& error = "");
    
    // 記录業务操作
    void logBusinessOperation(const std::string& operation,
                             const std::string& userId,
                             const std::string& status,
                             const std::string& details = "");
    
    // 記录系统事件
    void logSystemEvent(const std::string& event,
                       const std::string& level,
                       const std::string& message,
                       const std::unordered_map<std::string, std::string>& context = {});
    
    // === 指标收集 ===
    
    // 記录 gRPC 调用指标
    void recordGrpcMetrics(const std::string& service,
                          const std::string& method,
                          const std::string& status,
                          std::chrono::milliseconds duration);
    
    // 記录资料庫操作指标
    void recordDatabaseMetrics(const std::string& operation,
                              const std::string& table,
                              const std::string& status,
                              std::chrono::milliseconds duration);
    
    // 記录業务指标
    void recordBusinessMetrics(const std::string& operation,
                              const std::string& status,
                              int count = 1);
    
    // 記录系统指标
    void recordSystemMetrics(const std::string& metric,
                            double value,
                            const std::unordered_map<std::string, std::string>& labels = {});
    
    // === 分散式追蹤 ===
    
    // 开始追蹤 span
    std::string startSpan(const std::string& operation,
                         const std::string& parentSpanId = "",
                         const std::unordered_map<std::string, std::string>& tags = {});
    
    // 結束追蹤 span
    void finishSpan(const std::string& spanId,
                   const std::string& status = "ok",
                   const std::string& error = "");
    
    // 添加 span 标籤
    void addSpanTag(const std::string& spanId,
                   const std::string& key,
                   const std::string& value);
    
    // 添加 span 事件
    void addSpanEvent(const std::string& spanId,
                     const std::string& event,
                     const std::unordered_map<std::string, std::string>& attributes = {});
    
    // === 统一操作 ===
    
    // 执行帶完整可觀测性的操作
    template<typename Func>
    auto executeWithObservability(const std::string& operation,
                                 const std::string& service,
                                 const std::string& method,
                                 Func&& func) -> decltype(func()) {
        auto startTime = std::chrono::high_resolution_clock::now();
        std::string spanId = startSpan(operation);
        
        try {
            // 記录开始
            logSystemEvent("operation_start", "info", 
                          "Starting " + operation + " on " + service + "." + method);
            
            // 执行操作
            auto result = func();
            
            // 記录成功
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - startTime);
            
            logGrpcCall(service, method, "success", duration);
            recordGrpcMetrics(service, method, "success", duration);
            finishSpan(spanId, "ok");
            
            return result;
            
        } catch (const std::exception& e) {
            // 記录失败
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - startTime);
            
            std::string error = e.what();
            logGrpcCall(service, method, "error", duration, error);
            recordGrpcMetrics(service, method, "error", duration);
            finishSpan(spanId, "error", error);
            
            throw;
        }
    }
    
    // 执行资料庫操作帶可觀测性
    template<typename Func>
    auto executeDatabaseWithObservability(const std::string& operation,
                                         const std::string& table,
                                         Func&& func) -> decltype(func()) {
        auto startTime = std::chrono::high_resolution_clock::now();
        std::string spanId = startSpan("db_" + operation);
        
        try {
            // 記录开始
            logSystemEvent("db_operation_start", "info", 
                          "Starting " + operation + " on table " + table);
            
            // 执行操作
            auto result = func();
            
            // 記录成功
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - startTime);
            
            logDatabaseOperation(operation, table, "success", duration);
            recordDatabaseMetrics(operation, table, "success", duration);
            finishSpan(spanId, "ok");
            
            return result;
            
        } catch (const std::exception& e) {
            // 記录失败
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - startTime);
            
            std::string error = e.what();
            logDatabaseOperation(operation, table, "error", duration, error);
            recordDatabaseMetrics(operation, table, "error", duration);
            finishSpan(spanId, "error", error);
            
            throw;
        }
    }
    
    // === 统計信息 ===
    
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
    
    // 更新日誌级別
    void updateLogLevel(const std::string& level);
    
    // 更新指标配置
    void updateMetricsConfig(const std::string& port);
    
    // 更新追蹤配置
    void updateTracingConfig(const std::string& endpoint);

private:
    ObservabilityManager() = default;
    ~ObservabilityManager() = default;
    ObservabilityManager(const ObservabilityManager&) = delete;
    ObservabilityManager& operator=(const ObservabilityManager&) = delete;
    
    // 组件
    std::unique_ptr<Logger> logger_;
    std::unique_ptr<MetricsCollector> metricsCollector_;
    std::unique_ptr<Tracer> tracer_;
    
    // 统計信息
    ObservabilityStats stats_;
    
    // 配置
    std::string serviceName_;
    bool initialized_;
    
    // 内部方法
    void initializeLogger(const std::string& serviceName, const std::string& logLevel);
    void initializeMetrics(const std::string& serviceName, const std::string& port);
    void initializeTracing(const std::string& serviceName, const std::string& endpoint);
    
    // 生成追蹤 ID
    std::string generateTraceId();
    std::string generateSpanId();
};
