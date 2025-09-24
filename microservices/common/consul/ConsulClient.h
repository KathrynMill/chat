#pragma once
#include <string>
#include <vector>
#include <functional>

struct ServiceInstance {
    std::string id;
    std::string name;
    std::string address;
    int port;
    std::vector<std::string> tags;
    bool healthy = true;
};

class ConsulClient {
public:
    ConsulClient(const std::string& consulUrl = "http://127.0.0.1:8500");
    ~ConsulClient();

    // 服務註冊
    bool registerService(const std::string& serviceName, 
                        const std::string& serviceId,
                        const std::string& address, 
                        int port,
                        const std::vector<std::string>& tags = {});

    // 服務註銷
    bool deregisterService(const std::string& serviceId);

    // 服務發現
    std::vector<ServiceInstance> getHealthyServiceInstances(const std::string& serviceName);

    // 健康檢查
    bool checkServiceHealth(const std::string& serviceId);

private:
    std::string consulUrl_;
    std::string makeRequest(const std::string& method, const std::string& path, const std::string& body = "");
    std::string httpGet(const std::string& url);
    std::string httpPut(const std::string& url, const std::string& body);
    std::string httpDelete(const std::string& url);
};
