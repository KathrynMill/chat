#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <memory>

#ifdef HAVE_CURL
#include "consul/ConsulClient.h"
#endif

#ifdef HAVE_GRPC
#include <grpcpp/grpcpp.h>
#endif

// 服务实例信息
struct ServiceInstance {
    std::string id;
    std::string name;
    std::string address;
    int port;
    std::unordered_map<std::string, std::string> tags;
    std::unordered_map<std::string, std::string> meta;
    bool healthy;
    long lastCheckTime;
    
    std::string getEndpoint() const {
        return address + ":" + std::to_string(port);
    }
};

// 負载均衡策略
enum class LoadBalanceStrategy {
    ROUND_ROBIN,    // 輪询
    RANDOM,         // 隨機
    LEAST_CONN,     // 最少连接
    WEIGHTED        // 权重
};

// 服务发现和負载均衡器
class ServiceDiscovery {
public:
    static ServiceDiscovery& getInstance();
    
    // 初始化服务发现
    bool initialize(const std::string& consulUrl = "http://127.0.0.1:8500");
    
    // 註冊服务
    bool registerService(const std::string& serviceName, 
                        const std::string& serviceId,
                        const std::string& address, 
                        int port,
                        const std::unordered_map<std::string, std::string>& tags = {},
                        const std::unordered_map<std::string, std::string>& meta = {});
    
    // 註销服务
    bool deregisterService(const std::string& serviceId);
    
    // 获取健康服务实例
    std::vector<ServiceInstance> getHealthyInstances(const std::string& serviceName);
    
    // 获取单個服务实例（負载均衡）
    ServiceInstance getInstance(const std::string& serviceName, 
                               LoadBalanceStrategy strategy = LoadBalanceStrategy::ROUND_ROBIN);
    
    // 设置負载均衡策略
    void setLoadBalanceStrategy(const std::string& serviceName, LoadBalanceStrategy strategy);
    
    // 更新服务健康狀態
    void updateServiceHealth(const std::string& serviceName, const std::string& instanceId, bool healthy);
    
    // 启动服务发现線程
    void startDiscovery();
    
    // 停止服务发现
    void stopDiscovery();
    
    // 手动刷新服务列表
    void refreshServices();
    
    // 获取服务统計信息
    struct ServiceStats {
        int totalInstances;
        int healthyInstances;
        int unhealthyInstances;
        long lastUpdateTime;
    };
    ServiceStats getServiceStats(const std::string& serviceName);

private:
    ServiceDiscovery() = default;
    ~ServiceDiscovery() = default;
    ServiceDiscovery(const ServiceDiscovery&) = delete;
    ServiceDiscovery& operator=(const ServiceDiscovery&) = delete;
    
    // 服务发现線程
    void discoveryThread();
    
    // 負载均衡算法
    ServiceInstance roundRobinSelect(const std::string& serviceName);
    ServiceInstance randomSelect(const std::string& serviceName);
    ServiceInstance leastConnSelect(const std::string& serviceName);
    ServiceInstance weightedSelect(const std::string& serviceName);
    
    // 從 Consul 获取服务列表
    std::vector<ServiceInstance> fetchServicesFromConsul(const std::string& serviceName);
    
#ifdef HAVE_CURL
    std::unique_ptr<ConsulClient> consulClient_;
#endif
    
    // 服务实例緩存
    std::unordered_map<std::string, std::vector<ServiceInstance>> serviceInstances_;
    std::mutex instancesMutex_;
    
    // 負载均衡狀態
    std::unordered_map<std::string, LoadBalanceStrategy> loadBalanceStrategies_;
    std::unordered_map<std::string, std::atomic<int>> roundRobinCounters_;
    std::unordered_map<std::string, std::atomic<int>> connectionCounters_;
    std::mutex strategyMutex_;
    
    // 服务发现配置
    std::string consulUrl_;
    std::atomic<bool> running_;
    std::atomic<bool> discoveryEnabled_;
    std::thread discoveryThread_;
    std::chrono::seconds discoveryInterval_;
    
    // 健康检查配置
    std::chrono::seconds healthCheckInterval_;
    std::chrono::seconds healthCheckTimeout_;
};
