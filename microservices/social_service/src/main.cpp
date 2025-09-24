#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>

#ifdef HAVE_GRPC
#include <grpcpp/grpcpp.h>
using grpc::Server;
using grpc::ServerBuilder;
#include "social_service.grpc.pb.h"
#include "SocialServiceImpl.h"
#endif

#ifdef HAVE_CURL
#include "consul/ConsulClient.h"
#endif

#ifdef HAVE_REDIS
#include <sw/redis++/redis++.h>
#endif

int main(int argc, char** argv) {
#ifdef HAVE_GRPC
    std::string server_address("0.0.0.0:60052");
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    SocialServiceImpl service;
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "SocialService gRPC listening on " << server_address << "\n";

    // 註冊到 Consul
#ifdef HAVE_CURL
    std::string consulUrl = std::getenv("CONSUL_URL") ? std::getenv("CONSUL_URL") : std::string("http://127.0.0.1:8500");
    ConsulClient consul(consulUrl);
    
    std::string serviceId = "social-service-" + std::to_string(getpid());
    std::string serviceName = "chat-social-service";
    std::string address = "127.0.0.1";
    int port = 60052;
    
    if (consul.registerService(serviceName, serviceId, address, port, {"grpc", "social"})) {
        std::cout << "SocialService registered to Consul: " << serviceId << "\n";
    } else {
        std::cout << "Failed to register SocialService to Consul\n";
    }
    
    // 定期健康檢查
    std::thread healthThread([&consul, serviceId]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            if (!consul.checkServiceHealth(serviceId)) {
                std::cout << "SocialService health check failed\n";
            }
        }
    });
    healthThread.detach();
#endif

#ifdef HAVE_REDIS
    try {
        auto url = std::getenv("REDIS_URL");
        std::string redis_url = url ? url : "tcp://127.0.0.1:6379";
        sw::redis::Redis redis(redis_url);
        redis.set("social_service:health", "ok");
        auto v = redis.get("social_service:health");
        std::cout << "SocialService Redis ping: " << (v ? *v : std::string("<nil>")) << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Redis error: " << ex.what() << "\n";
    }
#endif
    server->Wait();
    
    // 服務關閉時註銷
#ifdef HAVE_CURL
    consul.deregisterService(serviceId);
    std::cout << "SocialService deregistered from Consul\n";
#endif
#else
    std::cout << "SocialService built without gRPC. 請安裝依賴或執行 install_micro_deps.sh。\n";
#endif
    return 0;
}


