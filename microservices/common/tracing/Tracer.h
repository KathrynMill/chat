#pragma once
#include <string>
#include <memory>
#include <chrono>
#include <unordered_map>

#ifdef HAVE_OPENTELEMETRY
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/span_context.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/exporters/jaeger/jaeger_exporter_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/batch_span_processor_factory.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/resource/resource_detector.h>
#endif

class Tracer {
public:
    static Tracer& getInstance();
    
    // 初始化追蹤器
    bool initialize(const std::string& serviceName, 
                   const std::string& jaegerEndpoint = "http://localhost:14268/api/traces");
    
    // 創建根 span
    std::shared_ptr<void> startSpan(const std::string& name, 
                                   const std::unordered_map<std::string, std::string>& attributes = {});
    
    // 創建子 span
    std::shared_ptr<void> startChildSpan(const std::string& name,
                                        std::shared_ptr<void> parentSpan,
                                        const std::unordered_map<std::string, std::string>& attributes = {});
    
    // 結束 span
    void endSpan(std::shared_ptr<void> span, bool success = true);
    
    // 添加屬性
    void addAttribute(std::shared_ptr<void> span, const std::string& key, const std::string& value);
    
    // 添加事件
    void addEvent(std::shared_ptr<void> span, const std::string& name, 
                 const std::unordered_map<std::string, std::string>& attributes = {});
    
    // 設置錯誤狀態
    void setError(std::shared_ptr<void> span, const std::string& errorMessage);
    
    // 從 HTTP 頭部提取 trace context
    std::shared_ptr<void> extractFromHeaders(const std::unordered_map<std::string, std::string>& headers);
    
    // 注入 trace context 到 HTTP 頭部
    void injectToHeaders(std::shared_ptr<void> span, std::unordered_map<std::string, std::string>& headers);
    
    // 從 gRPC metadata 提取 trace context
    std::shared_ptr<void> extractFromGrpcMetadata(const std::multimap<std::string, std::string>& metadata);
    
    // 注入 trace context 到 gRPC metadata
    void injectToGrpcMetadata(std::shared_ptr<void> span, std::multimap<std::string, std::string>& metadata);
    
    // 生成唯一的 trace ID
    std::string generateTraceId();
    
    // 生成唯一的 span ID
    std::string generateSpanId();
    
    // 從 span 中提取 trace ID
    std::string getTraceId(std::shared_ptr<void> span);
    
    // 從 span 中提取 span ID
    std::string getSpanId(std::shared_ptr<void> span);
    
    // 創建帶有 trace context 的 span
    std::shared_ptr<void> startSpanWithContext(const std::string& name,
                                              const std::string& traceId,
                                              const std::string& parentSpanId,
                                              const std::unordered_map<std::string, std::string>& attributes = {});

private:
    Tracer() = default;
    ~Tracer() = default;
    Tracer(const Tracer&) = delete;
    Tracer& operator=(const Tracer&) = delete;
    
#ifdef HAVE_OPENTELEMETRY
    std::shared_ptr<opentelemetry::trace::Tracer> tracer_;
    std::shared_ptr<opentelemetry::trace::TracerProvider> tracerProvider_;
    opentelemetry::trace::propagation::HttpTraceContext propagator_;
#endif
    
    bool initialized_ = false;
    std::string serviceName_;
};
