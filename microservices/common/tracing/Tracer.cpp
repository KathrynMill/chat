#include "Tracer.h"
#include <iostream>

#ifdef HAVE_OPENTELEMETRY
#include <opentelemetry/exporters/jaeger/jaeger_exporter_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/sdk/trace/batch_span_processor_factory.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/resource/resource_detector.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/trace/propagation/grpc_trace_context.h>
#endif

Tracer& Tracer::getInstance() {
    static Tracer instance;
    return instance;
}

bool Tracer::initialize(const std::string& serviceName, const std::string& jaegerEndpoint) {
#ifdef HAVE_OPENTELEMETRY
    try {
        serviceName_ = serviceName;
        
        // 創建 Jaeger exporter
        auto exporter = opentelemetry::exporter::jaeger::JaegerExporterFactory::Create();
        
        // 創建 batch span processor
        auto processor = opentelemetry::sdk::trace::BatchSpanProcessorFactory::Create(
            std::move(exporter),
            opentelemetry::sdk::trace::BatchSpanProcessorOptions{}
        );
        
        // 創建 resource
        auto resource = opentelemetry::sdk::resource::Resource::Create({
            {"service.name", serviceName},
            {"service.version", "1.0.0"}
        });
        
        // 創建 tracer provider
        tracerProvider_ = opentelemetry::sdk::trace::TracerProviderFactory::Create(
            std::move(processor), resource
        );
        
        // 設置全局 tracer provider
        opentelemetry::trace::Provider::SetTracerProvider(tracerProvider_);
        
        // 獲取 tracer
        tracer_ = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer(serviceName);
        
        initialized_ = true;
        std::cout << "Tracer initialized for service: " << serviceName << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize tracer: " << e.what() << "\n";
        return false;
    }
#else
    std::cout << "OpenTelemetry not available, using no-op tracer\n";
    initialized_ = true;
    return true;
#endif
}

std::shared_ptr<void> Tracer::startSpan(const std::string& name, 
                                       const std::unordered_map<std::string, std::string>& attributes) {
#ifdef HAVE_OPENTELEMETRY
    if (!initialized_ || !tracer_) {
        return nullptr;
    }
    
    auto span = tracer_->StartSpan(name);
    
    // 添加屬性
    for (const auto& attr : attributes) {
        span->SetAttribute(attr.first, attr.second);
    }
    
    return std::shared_ptr<void>(span.release(), [](void* ptr) {
        delete static_cast<opentelemetry::trace::Span*>(ptr);
    });
#else
    return nullptr;
#endif
}

std::shared_ptr<void> Tracer::startChildSpan(const std::string& name,
                                            std::shared_ptr<void> parentSpan,
                                            const std::unordered_map<std::string, std::string>& attributes) {
#ifdef HAVE_OPENTELEMETRY
    if (!initialized_ || !tracer_ || !parentSpan) {
        return nullptr;
    }
    
    auto parent = static_cast<opentelemetry::trace::Span*>(parentSpan.get());
    auto span = tracer_->StartSpan(name, parent->GetContext());
    
    // 添加屬性
    for (const auto& attr : attributes) {
        span->SetAttribute(attr.first, attr.second);
    }
    
    return std::shared_ptr<void>(span.release(), [](void* ptr) {
        delete static_cast<opentelemetry::trace::Span*>(ptr);
    });
#else
    return nullptr;
#endif
}

void Tracer::endSpan(std::shared_ptr<void> span, bool success) {
#ifdef HAVE_OPENTELEMETRY
    if (!span) return;
    
    auto opentelemetrySpan = static_cast<opentelemetry::trace::Span*>(span.get());
    
    if (!success) {
        opentelemetrySpan->SetStatus(opentelemetry::trace::StatusCode::kError);
    }
    
    opentelemetrySpan->End();
#endif
}

void Tracer::addAttribute(std::shared_ptr<void> span, const std::string& key, const std::string& value) {
#ifdef HAVE_OPENTELEMETRY
    if (!span) return;
    
    auto opentelemetrySpan = static_cast<opentelemetry::trace::Span*>(span.get());
    opentelemetrySpan->SetAttribute(key, value);
#endif
}

void Tracer::addEvent(std::shared_ptr<void> span, const std::string& name, 
                     const std::unordered_map<std::string, std::string>& attributes) {
#ifdef HAVE_OPENTELEMETRY
    if (!span) return;
    
    auto opentelemetrySpan = static_cast<opentelemetry::trace::Span*>(span.get());
    
    // 簡化版事件添加
    opentelemetrySpan->AddEvent(name);
#endif
}

void Tracer::setError(std::shared_ptr<void> span, const std::string& errorMessage) {
#ifdef HAVE_OPENTELEMETRY
    if (!span) return;
    
    auto opentelemetrySpan = static_cast<opentelemetry::trace::Span*>(span.get());
    opentelemetrySpan->SetStatus(opentelemetry::trace::StatusCode::kError, errorMessage);
#endif
}

std::shared_ptr<void> Tracer::extractFromHeaders(const std::unordered_map<std::string, std::string>& headers) {
#ifdef HAVE_OPENTELEMETRY
    if (!initialized_) return nullptr;
    
    // 簡化版 header 提取
    auto it = headers.find("traceparent");
    if (it != headers.end()) {
        // 這裡應該解析 traceparent header
        // 簡化實現
        return startSpan("extracted-span");
    }
    return nullptr;
#else
    return nullptr;
#endif
}

void Tracer::injectToHeaders(std::shared_ptr<void> span, std::unordered_map<std::string, std::string>& headers) {
#ifdef HAVE_OPENTELEMETRY
    if (!span) return;
    
    // 簡化版 header 注入
    headers["traceparent"] = "00-12345678901234567890123456789012-1234567890123456-01";
#endif
}

std::shared_ptr<void> Tracer::extractFromGrpcMetadata(const std::multimap<std::string, std::string>& metadata) {
#ifdef HAVE_OPENTELEMETRY
    if (!initialized_) return nullptr;
    
    // 簡化版 gRPC metadata 提取
    auto it = metadata.find("traceparent");
    if (it != metadata.end()) {
        return startSpan("grpc-extracted-span");
    }
    return nullptr;
#else
    return nullptr;
#endif
}

void Tracer::injectToGrpcMetadata(std::shared_ptr<void> span, std::multimap<std::string, std::string>& metadata) {
#ifdef HAVE_OPENTELEMETRY
    if (!span) return;
    
    // 簡化版 gRPC metadata 注入
    metadata.insert({"traceparent", "00-12345678901234567890123456789012-1234567890123456-01"});
#endif
}

std::string Tracer::generateTraceId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::string traceId;
    traceId.reserve(32);
    
    for (int i = 0; i < 32; ++i) {
        traceId += "0123456789abcdef"[dis(gen)];
    }
    
    return traceId;
}

std::string Tracer::generateSpanId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::string spanId;
    spanId.reserve(16);
    
    for (int i = 0; i < 16; ++i) {
        spanId += "0123456789abcdef"[dis(gen)];
    }
    
    return spanId;
}

std::string Tracer::getTraceId(std::shared_ptr<void> span) {
    if (!span) return "";
    
    // 簡化實現：返回隨機生成的 trace ID
    return generateTraceId();
}

std::string Tracer::getSpanId(std::shared_ptr<void> span) {
    if (!span) return "";
    
    // 簡化實現：返回隨機生成的 span ID
    return generateSpanId();
}

std::shared_ptr<void> Tracer::startSpanWithContext(const std::string& name,
                                                  const std::string& traceId,
                                                  const std::string& parentSpanId,
                                                  const std::unordered_map<std::string, std::string>& attributes) {
#ifdef HAVE_OPENTELEMETRY
    if (!initialized_ || !tracer_) {
        return nullptr;
    }
    
    // 創建 span context
    auto span = tracer_->StartSpan(name);
    
    // 添加 trace ID 和 span ID 屬性
    span->SetAttribute("trace_id", traceId);
    span->SetAttribute("span_id", parentSpanId);
    
    // 添加其他屬性
    for (const auto& attr : attributes) {
        span->SetAttribute(attr.first, attr.second);
    }
    
    return std::shared_ptr<void>(span.release(), [](void* ptr) {
        delete static_cast<opentelemetry::trace::Span*>(ptr);
    });
#else
    return nullptr;
#endif
}
