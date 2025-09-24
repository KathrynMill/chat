#include "MetricsCollector.h"
#include <iostream>
#include <sstream>

#ifdef HAVE_PROMETHEUS
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#endif

MetricsCollector& MetricsCollector::getInstance() {
    static MetricsCollector instance;
    return instance;
}

bool MetricsCollector::initialize(const std::string& serviceName, int port) {
    serviceName_ = serviceName;
    port_ = port;
    
#ifdef HAVE_PROMETHEUS
    try {
        registry_ = std::make_shared<prometheus::Registry>();
        
        // 創建指標
        grpc_calls_total_ = prometheus::BuildCounter()
            .Name("grpc_calls_total")
            .Help("Total number of gRPC calls")
            .Register(*registry_);
            
        grpc_call_duration_seconds_ = prometheus::BuildCounter()
            .Name("grpc_call_duration_seconds")
            .Help("Total duration of gRPC calls in seconds")
            .Register(*registry_);
            
        http_requests_total_ = prometheus::BuildCounter()
            .Name("http_requests_total")
            .Help("Total number of HTTP requests")
            .Register(*registry_);
            
        http_request_duration_seconds_ = prometheus::BuildCounter()
            .Name("http_request_duration_seconds")
            .Help("Total duration of HTTP requests in seconds")
            .Register(*registry_);
            
        database_queries_total_ = prometheus::BuildCounter()
            .Name("database_queries_total")
            .Help("Total number of database queries")
            .Register(*registry_);
            
        database_query_duration_seconds_ = prometheus::BuildCounter()
            .Name("database_query_duration_seconds")
            .Help("Total duration of database queries in seconds")
            .Register(*registry_);
            
        kafka_messages_total_ = prometheus::BuildCounter()
            .Name("kafka_messages_total")
            .Help("Total number of Kafka messages")
            .Register(*registry_);
            
        active_connections_ = prometheus::BuildGauge()
            .Name("active_connections")
            .Help("Number of active connections")
            .Register(*registry_);
            
        online_users_ = prometheus::BuildGauge()
            .Name("online_users")
            .Help("Number of online users")
            .Register(*registry_);
        
        // 啟動 HTTP 服務器
        exposer_ = std::make_unique<prometheus::Exposer>("0.0.0.0:" + std::to_string(port));
        exposer_->RegisterCollectable(registry_);
        
        initialized_ = true;
        std::cout << "MetricsCollector initialized for service: " << serviceName 
                  << " on port: " << port << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize MetricsCollector: " << e.what() << "\n";
        return false;
    }
#else
    std::cout << "Prometheus not available, using simple metrics\n";
    initialized_ = true;
    return true;
#endif
}

void MetricsCollector::incrementCounter(const std::string& name, 
                                       const std::unordered_map<std::string, std::string>& labels,
                                       double value) {
#ifdef HAVE_PROMETHEUS
    if (!initialized_) return;
    
    // 簡化版實現，實際應該根據 labels 創建對應的指標
    auto& counter = grpc_calls_total_.Add(labels);
    counter.Increment(value);
#else
    std::lock_guard<std::mutex> lock(metricsMutex_);
    simpleCounters_[name] += value;
#endif
}

void MetricsCollector::setGauge(const std::string& name, 
                               const std::unordered_map<std::string, std::string>& labels,
                               double value) {
#ifdef HAVE_PROMETHEUS
    if (!initialized_) return;
    
    auto& gauge = active_connections_.Add(labels);
    gauge.Set(value);
#else
    std::lock_guard<std::mutex> lock(metricsMutex_);
    simpleGauges_[name] = value;
#endif
}

void MetricsCollector::addGauge(const std::string& name, 
                               const std::unordered_map<std::string, std::string>& labels,
                               double value) {
#ifdef HAVE_PROMETHEUS
    if (!initialized_) return;
    
    auto& gauge = active_connections_.Add(labels);
    gauge.Increment(value);
#else
    std::lock_guard<std::mutex> lock(metricsMutex_);
    simpleGauges_[name] += value;
#endif
}

void MetricsCollector::observeHistogram(const std::string& name, 
                                       const std::unordered_map<std::string, std::string>& labels,
                                       double value) {
#ifdef HAVE_PROMETHEUS
    if (!initialized_) return;
    
    // 簡化版實現
    auto& counter = grpc_call_duration_seconds_.Add(labels);
    counter.Increment(value);
#else
    std::lock_guard<std::mutex> lock(metricsMutex_);
    simpleCounters_[name + "_total"] += value;
#endif
}

void MetricsCollector::recordGrpcCall(const std::string& service, const std::string& method, 
                                     bool success, double durationMs) {
    std::unordered_map<std::string, std::string> labels = {
        {"service", service},
        {"method", method},
        {"status", success ? "success" : "error"}
    };
    
    incrementCounter("grpc_calls_total", labels);
    observeHistogram("grpc_call_duration_seconds", labels, durationMs / 1000.0);
}

void MetricsCollector::recordHttpRequest(const std::string& method, const std::string& path, 
                                        int statusCode, double durationMs) {
    std::unordered_map<std::string, std::string> labels = {
        {"method", method},
        {"path", path},
        {"status", std::to_string(statusCode)}
    };
    
    incrementCounter("http_requests_total", labels);
    observeHistogram("http_request_duration_seconds", labels, durationMs / 1000.0);
}

void MetricsCollector::recordDatabaseQuery(const std::string& operation, bool success, double durationMs) {
    std::unordered_map<std::string, std::string> labels = {
        {"operation", operation},
        {"status", success ? "success" : "error"}
    };
    
    incrementCounter("database_queries_total", labels);
    observeHistogram("database_query_duration_seconds", labels, durationMs / 1000.0);
}

void MetricsCollector::recordKafkaMessage(const std::string& topic, const std::string& operation, bool success) {
    std::unordered_map<std::string, std::string> labels = {
        {"topic", topic},
        {"operation", operation},
        {"status", success ? "success" : "error"}
    };
    
    incrementCounter("kafka_messages_total", labels);
}

void MetricsCollector::recordActiveConnections(int count) {
    std::unordered_map<std::string, std::string> labels = {
        {"service", serviceName_}
    };
    
    setGauge("active_connections", labels, count);
}

void MetricsCollector::recordOnlineUsers(int count) {
    std::unordered_map<std::string, std::string> labels = {
        {"service", serviceName_}
    };
    
    setGauge("online_users", labels, count);
}

std::string MetricsCollector::getMetrics() {
#ifdef HAVE_PROMETHEUS
    if (!initialized_) return "";
    
    // 簡化版實現，實際應該序列化所有指標
    return "# Prometheus metrics endpoint available at :" + std::to_string(port_) + "/metrics\n";
#else
    std::lock_guard<std::mutex> lock(metricsMutex_);
    std::ostringstream oss;
    
    oss << "# Simple metrics for " << serviceName_ << "\n";
    
    for (const auto& counter : simpleCounters_) {
        oss << counter.first << " " << counter.second.load() << "\n";
    }
    
    for (const auto& gauge : simpleGauges_) {
        oss << gauge.first << " " << gauge.second.load() << "\n";
    }
    
    return oss.str();
#endif
}
