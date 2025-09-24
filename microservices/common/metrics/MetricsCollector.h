#pragma once
#include <string>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <chrono>

#ifdef HAVE_PROMETHEUS
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#endif

class MetricsCollector {
public:
    static MetricsCollector& getInstance();
    
    // 初始化指标收集器
    bool initialize(const std::string& serviceName, int port = 8080);
    
    // 計数器指标
    void incrementCounter(const std::string& name, 
                         const std::unordered_map<std::string, std::string>& labels = {},
                         double value = 1.0);
    
    // 計量器指标
    void setGauge(const std::string& name, 
                  const std::unordered_map<std::string, std::string>& labels = {},
                  double value = 0.0);
    
    void addGauge(const std::string& name, 
                  const std::unordered_map<std::string, std::string>& labels = {},
                  double value = 1.0);
    
    // 直方圖指标
    void observeHistogram(const std::string& name, 
                         const std::unordered_map<std::string, std::string>& labels = {},
                         double value = 0.0);
    
    // 業务指标
    void recordGrpcCall(const std::string& service, const std::string& method, 
                       bool success, double durationMs);
    
    void recordHttpRequest(const std::string& method, const std::string& path, 
                          int statusCode, double durationMs);
    
    void recordDatabaseQuery(const std::string& operation, bool success, double durationMs);
    
    void recordKafkaMessage(const std::string& topic, const std::string& operation, bool success);
    
    void recordActiveConnections(int count);
    
    void recordOnlineUsers(int count);
    
    // 获取指标数据（用於测试或自定義端點）
    std::string getMetrics();

private:
    MetricsCollector() = default;
    ~MetricsCollector() = default;
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;
    
#ifdef HAVE_PROMETHEUS
    std::shared_ptr<prometheus::Registry> registry_;
    std::unique_ptr<prometheus::Exposer> exposer_;
    
    // 預定義指标
    prometheus::Family<prometheus::Counter>& grpc_calls_total_;
    prometheus::Family<prometheus::Counter>& grpc_call_duration_seconds_;
    prometheus::Family<prometheus::Counter>& http_requests_total_;
    prometheus::Family<prometheus::Counter>& http_request_duration_seconds_;
    prometheus::Family<prometheus::Counter>& database_queries_total_;
    prometheus::Family<prometheus::Counter>& database_query_duration_seconds_;
    prometheus::Family<prometheus::Counter>& kafka_messages_total_;
    prometheus::Family<prometheus::Gauge>& active_connections_;
    prometheus::Family<prometheus::Gauge>& online_users_;
#endif
    
    bool initialized_ = false;
    std::string serviceName_;
    int port_;
    
    // 簡化版指标（當 Prometheus 不可用時）
    std::unordered_map<std::string, std::atomic<double>> simpleCounters_;
    std::unordered_map<std::string, std::atomic<double>> simpleGauges_;
    std::mutex metricsMutex_;
};
