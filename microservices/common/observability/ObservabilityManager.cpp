#include "ObservabilityManager.h"
#include <random>
#include <sstream>
#include <iomanip>

ObservabilityManager& ObservabilityManager::getInstance() {
    static ObservabilityManager instance;
    return instance;
}

bool ObservabilityManager::initialize(const std::string& serviceName, 
                                     const std::string& logLevel,
                                     const std::string& metricsPort,
                                     const std::string& jaegerEndpoint) {
    serviceName_ = serviceName;
    initialized_ = false;
    
    // 重置统計信息
    stats_ = ObservabilityStats{};
    stats_.serviceName = serviceName;
    stats_.logLevel = logLevel;
    stats_.metricsPort = metricsPort;
    stats_.jaegerEnabled = !jaegerEndpoint.empty();
    
    try {
        // 初始化日誌系统
        initializeLogger(serviceName, logLevel);
        
        // 初始化指标收集
        initializeMetrics(serviceName, metricsPort);
        
        // 初始化分散式追蹤
        initializeTracing(serviceName, jaegerEndpoint);
        
        initialized_ = true;
        
        // 記录初始化成功
        logSystemEvent("observability_init", "info", 
                      "Observability system initialized for service: " + serviceName);
        
        std::cout << "Observability system initialized for service: " << serviceName << "\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize observability system: " << e.what() << "\n";
        return false;
    }
}

void ObservabilityManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // 記录关闭事件
    logSystemEvent("observability_shutdown", "info", 
                  "Observability system shutting down for service: " + serviceName_);
    
    // 关闭组件
    if (logger_) {
        logger_->shutdown();
    }
    
    if (metricsCollector_) {
        metricsCollector_->shutdown();
    }
    
    if (tracer_) {
        tracer_->shutdown();
    }
    
    initialized_ = false;
    std::cout << "Observability system shutdown for service: " << serviceName_ << "\n";
}

// === 日誌記录实现 ===

void ObservabilityManager::logGrpcCall(const std::string& service, 
                                      const std::string& method,
                                      const std::string& status,
                                      std::chrono::milliseconds duration,
                                      const std::string& error) {
    if (!logger_) return;
    
    std::unordered_map<std::string, std::string> context = {
        {"service", service},
        {"method", method},
        {"status", status},
        {"duration_ms", std::to_string(duration.count())}
    };
    
    if (!error.empty()) {
        context["error"] = error;
    }
    
    std::string message = "gRPC call: " + service + "." + method + " - " + status;
    if (!error.empty()) {
        message += " - " + error;
    }
    
    logger_->log("grpc_call", "info", message, context);
    stats_.grpcCalls++;
    
    if (status == "error") {
        stats_.grpcErrors++;
    }
}

void ObservabilityManager::logDatabaseOperation(const std::string& operation,
                                               const std::string& table,
                                               const std::string& status,
                                               std::chrono::milliseconds duration,
                                               const std::string& error) {
    if (!logger_) return;
    
    std::unordered_map<std::string, std::string> context = {
        {"operation", operation},
        {"table", table},
        {"status", status},
        {"duration_ms", std::to_string(duration.count())}
    };
    
    if (!error.empty()) {
        context["error"] = error;
    }
    
    std::string message = "Database operation: " + operation + " on " + table + " - " + status;
    if (!error.empty()) {
        message += " - " + error;
    }
    
    logger_->log("db_operation", "info", message, context);
    stats_.dbOperations++;
    
    if (status == "error") {
        stats_.dbErrors++;
    }
}

void ObservabilityManager::logBusinessOperation(const std::string& operation,
                                               const std::string& userId,
                                               const std::string& status,
                                               const std::string& details) {
    if (!logger_) return;
    
    std::unordered_map<std::string, std::string> context = {
        {"operation", operation},
        {"user_id", userId},
        {"status", status}
    };
    
    if (!details.empty()) {
        context["details"] = details;
    }
    
    std::string message = "Business operation: " + operation + " by user " + userId + " - " + status;
    if (!details.empty()) {
        message += " - " + details;
    }
    
    logger_->log("business_operation", "info", message, context);
    stats_.businessOperations++;
}

void ObservabilityManager::logSystemEvent(const std::string& event,
                                         const std::string& level,
                                         const std::string& message,
                                         const std::unordered_map<std::string, std::string>& context) {
    if (!logger_) return;
    
    logger_->log(event, level, message, context);
}

// === 指标收集实现 ===

void ObservabilityManager::recordGrpcMetrics(const std::string& service,
                                            const std::string& method,
                                            const std::string& status,
                                            std::chrono::milliseconds duration) {
    if (!metricsCollector_) return;
    
    std::unordered_map<std::string, std::string> labels = {
        {"service", service},
        {"method", method},
        {"status", status}
    };
    
    metricsCollector_->recordCounter("grpc_calls_total", 1, labels);
    metricsCollector_->recordHistogram("grpc_call_duration_ms", duration.count(), labels);
    
    if (status == "error") {
        metricsCollector_->recordCounter("grpc_errors_total", 1, labels);
    }
}

void ObservabilityManager::recordDatabaseMetrics(const std::string& operation,
                                                const std::string& table,
                                                const std::string& status,
                                                std::chrono::milliseconds duration) {
    if (!metricsCollector_) return;
    
    std::unordered_map<std::string, std::string> labels = {
        {"operation", operation},
        {"table", table},
        {"status", status}
    };
    
    metricsCollector_->recordCounter("db_operations_total", 1, labels);
    metricsCollector_->recordHistogram("db_operation_duration_ms", duration.count(), labels);
    
    if (status == "error") {
        metricsCollector_->recordCounter("db_errors_total", 1, labels);
    }
}

void ObservabilityManager::recordBusinessMetrics(const std::string& operation,
                                                const std::string& status,
                                                int count) {
    if (!metricsCollector_) return;
    
    std::unordered_map<std::string, std::string> labels = {
        {"operation", operation},
        {"status", status}
    };
    
    metricsCollector_->recordCounter("business_operations_total", count, labels);
}

void ObservabilityManager::recordSystemMetrics(const std::string& metric,
                                              double value,
                                              const std::unordered_map<std::string, std::string>& labels) {
    if (!metricsCollector_) return;
    
    metricsCollector_->recordGauge(metric, value, labels);
}

// === 分散式追蹤实现 ===

std::string ObservabilityManager::startSpan(const std::string& operation,
                                           const std::string& parentSpanId,
                                           const std::unordered_map<std::string, std::string>& tags) {
    if (!tracer_) {
        return generateSpanId();
    }
    
    std::string spanId = tracer_->startSpan(operation, parentSpanId, tags);
    stats_.spansCreated++;
    
    return spanId;
}

void ObservabilityManager::finishSpan(const std::string& spanId,
                                     const std::string& status,
                                     const std::string& error) {
    if (!tracer_) {
        return;
    }
    
    tracer_->finishSpan(spanId, status, error);
    stats_.spansFinished++;
}

void ObservabilityManager::addSpanTag(const std::string& spanId,
                                     const std::string& key,
                                     const std::string& value) {
    if (!tracer_) {
        return;
    }
    
    tracer_->addTag(spanId, key, value);
}

void ObservabilityManager::addSpanEvent(const std::string& spanId,
                                       const std::string& event,
                                       const std::unordered_map<std::string, std::string>& attributes) {
    if (!tracer_) {
        return;
    }
    
    tracer_->addEvent(spanId, event, attributes);
}

// === 统計信息 ===

ObservabilityManager::ObservabilityStats ObservabilityManager::getStats() {
    return stats_;
}

// === 配置更新 ===

void ObservabilityManager::updateLogLevel(const std::string& level) {
    if (logger_) {
        logger_->setLevel(level);
        stats_.logLevel = level;
    }
}

void ObservabilityManager::updateMetricsConfig(const std::string& port) {
    if (metricsCollector_) {
        metricsCollector_->updatePort(port);
        stats_.metricsPort = port;
    }
}

void ObservabilityManager::updateTracingConfig(const std::string& endpoint) {
    if (tracer_) {
        tracer_->updateEndpoint(endpoint);
        stats_.jaegerEnabled = !endpoint.empty();
    }
}

// === 私有方法实现 ===

void ObservabilityManager::initializeLogger(const std::string& serviceName, const std::string& logLevel) {
    logger_ = std::make_unique<Logger>();
    
    LoggerConfig config;
    config.serviceName = serviceName;
    config.level = logLevel;
    config.format = "json";
    config.output = "console";
    config.enableAsync = true;
    config.maxFileSize = 100 * 1024 * 1024; // 100MB
    config.maxFiles = 10;
    
    if (!logger_->initialize(config)) {
        throw std::runtime_error("Failed to initialize logger");
    }
}

void ObservabilityManager::initializeMetrics(const std::string& serviceName, const std::string& port) {
    metricsCollector_ = std::make_unique<MetricsCollector>();
    
    MetricsConfig config;
    config.serviceName = serviceName;
    config.port = port;
    config.enableDefaultMetrics = true;
    
    if (!metricsCollector_->initialize(config)) {
        throw std::runtime_error("Failed to initialize metrics collector");
    }
}

void ObservabilityManager::initializeTracing(const std::string& serviceName, const std::string& endpoint) {
    if (endpoint.empty()) {
        return; // 追蹤是可選的
    }
    
    tracer_ = std::make_unique<Tracer>();
    
    TracingConfig config;
    config.serviceName = serviceName;
    config.endpoint = endpoint;
    config.samplingRate = 1.0;
    
    if (!tracer_->initialize(config)) {
        std::cerr << "Warning: Failed to initialize tracer, continuing without tracing\n";
        tracer_.reset();
    }
}

std::string ObservabilityManager::generateTraceId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 16; ++i) {
        ss << dis(gen);
    }
    return ss.str();
}

std::string ObservabilityManager::generateSpanId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; ++i) {
        ss << dis(gen);
    }
    return ss.str();
}
