#include "ServiceDiscovery.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <sstream>

#ifdef HAVE_CURL
#include "consul/ConsulClient.h"
#endif

ServiceDiscovery& ServiceDiscovery::getInstance() {
    static ServiceDiscovery instance;
    return instance;
}

bool ServiceDiscovery::initialize(const std::string& consulUrl) {
    consulUrl_ = consulUrl;
    discoveryInterval_ = std::chrono::seconds(30);
    healthCheckInterval_ = std::chrono::seconds(10);
    healthCheckTimeout_ = std::chrono::seconds(5);
    running_ = false;
    discoveryEnabled_ = true;
    
#ifdef HAVE_CURL
    try {
        consulClient_ = std::make_unique<ConsulClient>(consulUrl);
        std::cout << "ServiceDiscovery initialized with Consul: " << consulUrl << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Consul client: " << e.what() << "\n";
        return false;
    }
#else
    std::cout << "ServiceDiscovery initialized without Consul (CURL not available)\n";
    return true;
#endif
}

bool ServiceDiscovery::registerService(const std::string& serviceName, 
                                      const std::string& serviceId,
                                      const std::string& address, 
                                      int port,
                                      const std::unordered_map<std::string, std::string>& tags,
                                      const std::unordered_map<std::string, std::string>& meta) {
#ifdef HAVE_CURL
    if (!consulClient_) {
        std::cerr << "Consul client not initialized\n";
        return false;
    }
    
    try {
        ServiceRegistration registration;
        registration.id = serviceId;
        registration.name = serviceName;
        registration.address = address;
        registration.port = port;
        registration.tags = tags;
        registration.meta = meta;
        
        // 添加健康检查
        Check healthCheck;
        healthCheck.http = "http://" + address + ":" + std::to_string(port + 1000) + "/health";
        healthCheck.interval = "10s";
        healthCheck.timeout = "5s";
        registration.checks.push_back(healthCheck);
        
        bool success = consulClient_->registerService(registration);
        if (success) {
            std::cout << "Service registered: " << serviceName << " (" << serviceId << ") at " 
                      << address << ":" << port << "\n";
        }
        return success;
    } catch (const std::exception& e) {
        std::cerr << "Failed to register service: " << e.what() << "\n";
        return false;
    }
#else
    std::cout << "Service registration skipped (CURL not available): " << serviceName << "\n";
    return true;
#endif
}

bool ServiceDiscovery::deregisterService(const std::string& serviceId) {
#ifdef HAVE_CURL
    if (!consulClient_) {
        std::cerr << "Consul client not initialized\n";
        return false;
    }
    
    try {
        bool success = consulClient_->deregisterService(serviceId);
        if (success) {
            std::cout << "Service deregistered: " << serviceId << "\n";
        }
        return success;
    } catch (const std::exception& e) {
        std::cerr << "Failed to deregister service: " << e.what() << "\n";
        return false;
    }
#else
    std::cout << "Service deregistration skipped (CURL not available): " << serviceId << "\n";
    return true;
#endif
}

std::vector<ServiceInstance> ServiceDiscovery::getHealthyInstances(const std::string& serviceName) {
    std::lock_guard<std::mutex> lock(instancesMutex_);
    
    auto it = serviceInstances_.find(serviceName);
    if (it == serviceInstances_.end()) {
        return {};
    }
    
    std::vector<ServiceInstance> healthyInstances;
    for (const auto& instance : it->second) {
        if (instance.healthy) {
            healthyInstances.push_back(instance);
        }
    }
    
    return healthyInstances;
}

ServiceInstance ServiceDiscovery::getInstance(const std::string& serviceName, LoadBalanceStrategy strategy) {
    auto healthyInstances = getHealthyInstances(serviceName);
    
    if (healthyInstances.empty()) {
        // 返回空实例
        return ServiceInstance{};
    }
    
    if (healthyInstances.size() == 1) {
        return healthyInstances[0];
    }
    
    switch (strategy) {
        case LoadBalanceStrategy::ROUND_ROBIN:
            return roundRobinSelect(serviceName);
        case LoadBalanceStrategy::RANDOM:
            return randomSelect(serviceName);
        case LoadBalanceStrategy::LEAST_CONN:
            return leastConnSelect(serviceName);
        case LoadBalanceStrategy::WEIGHTED:
            return weightedSelect(serviceName);
        default:
            return roundRobinSelect(serviceName);
    }
}

void ServiceDiscovery::setLoadBalanceStrategy(const std::string& serviceName, LoadBalanceStrategy strategy) {
    std::lock_guard<std::mutex> lock(strategyMutex_);
    loadBalanceStrategies_[serviceName] = strategy;
}

void ServiceDiscovery::updateServiceHealth(const std::string& serviceName, const std::string& instanceId, bool healthy) {
    std::lock_guard<std::mutex> lock(instancesMutex_);
    
    auto it = serviceInstances_.find(serviceName);
    if (it != serviceInstances_.end()) {
        for (auto& instance : it->second) {
            if (instance.id == instanceId) {
                instance.healthy = healthy;
                instance.lastCheckTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                break;
            }
        }
    }
}

void ServiceDiscovery::startDiscovery() {
    if (running_) {
        return;
    }
    
    running_ = true;
    discoveryThread_ = std::thread(&ServiceDiscovery::discoveryThread, this);
    std::cout << "Service discovery started\n";
}

void ServiceDiscovery::stopDiscovery() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    if (discoveryThread_.joinable()) {
        discoveryThread_.join();
    }
    std::cout << "Service discovery stopped\n";
}

void ServiceDiscovery::refreshServices() {
    if (!discoveryEnabled_) {
        return;
    }
    
#ifdef HAVE_CURL
    if (!consulClient_) {
        return;
    }
    
    try {
        // 获取所有服务
        std::vector<std::string> serviceNames = {"user-service", "social-service", "message-service"};
        
        for (const auto& serviceName : serviceNames) {
            auto instances = fetchServicesFromConsul(serviceName);
            
            std::lock_guard<std::mutex> lock(instancesMutex_);
            serviceInstances_[serviceName] = instances;
        }
        
        std::cout << "Services refreshed from Consul\n";
    } catch (const std::exception& e) {
        std::cerr << "Failed to refresh services: " << e.what() << "\n";
    }
#endif
}

ServiceDiscovery::ServiceStats ServiceDiscovery::getServiceStats(const std::string& serviceName) {
    std::lock_guard<std::mutex> lock(instancesMutex_);
    
    ServiceStats stats{};
    auto it = serviceInstances_.find(serviceName);
    if (it != serviceInstances_.end()) {
        stats.totalInstances = it->second.size();
        for (const auto& instance : it->second) {
            if (instance.healthy) {
                stats.healthyInstances++;
            } else {
                stats.unhealthyInstances++;
            }
        }
        stats.lastUpdateTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    return stats;
}

void ServiceDiscovery::discoveryThread() {
    while (running_) {
        try {
            refreshServices();
            std::this_thread::sleep_for(discoveryInterval_);
        } catch (const std::exception& e) {
            std::cerr << "Service discovery thread error: " << e.what() << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

ServiceInstance ServiceDiscovery::roundRobinSelect(const std::string& serviceName) {
    auto healthyInstances = getHealthyInstances(serviceName);
    if (healthyInstances.empty()) {
        return ServiceInstance{};
    }
    
    int& counter = roundRobinCounters_[serviceName];
    int index = counter % healthyInstances.size();
    counter++;
    
    return healthyInstances[index];
}

ServiceInstance ServiceDiscovery::randomSelect(const std::string& serviceName) {
    auto healthyInstances = getHealthyInstances(serviceName);
    if (healthyInstances.empty()) {
        return ServiceInstance{};
    }
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, healthyInstances.size() - 1);
    
    return healthyInstances[dis(gen)];
}

ServiceInstance ServiceDiscovery::leastConnSelect(const std::string& serviceName) {
    auto healthyInstances = getHealthyInstances(serviceName);
    if (healthyInstances.empty()) {
        return ServiceInstance{};
    }
    
    // 簡化实现：返回第一個实例
    // 实際实现中需要維護连接計数
    return healthyInstances[0];
}

ServiceInstance ServiceDiscovery::weightedSelect(const std::string& serviceName) {
    auto healthyInstances = getHealthyInstances(serviceName);
    if (healthyInstances.empty()) {
        return ServiceInstance{};
    }
    
    // 簡化实现：返回第一個实例
    // 实際实现中需要根据权重選擇
    return healthyInstances[0];
}

std::vector<ServiceInstance> ServiceDiscovery::fetchServicesFromConsul(const std::string& serviceName) {
    std::vector<ServiceInstance> instances;
    
#ifdef HAVE_CURL
    if (!consulClient_) {
        return instances;
    }
    
    try {
        // 這裡應該调用 ConsulClient 的方法來获取服务列表
        // 由於 ConsulClient 的实现可能不同，這裡提供一個框架
        
        // 示例：假设 ConsulClient 有 getServiceInstances 方法
        // auto consulInstances = consulClient_->getServiceInstances(serviceName);
        
        // 转换為 ServiceInstance 格式
        // for (const auto& consulInstance : consulInstances) {
        //     ServiceInstance instance;
        //     instance.id = consulInstance.id;
        //     instance.name = consulInstance.name;
        //     instance.address = consulInstance.address;
        //     instance.port = consulInstance.port;
        //     instance.healthy = consulInstance.healthy;
        //     instances.push_back(instance);
        // }
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to fetch services from Consul: " << e.what() << "\n";
    }
#endif
    
    return instances;
}
