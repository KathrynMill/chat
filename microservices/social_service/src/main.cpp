#include <iostream>

#ifdef HAVE_GRPC
#include <grpcpp/grpcpp.h>
using grpc::Server;
using grpc::ServerBuilder;
#include "social_service.grpc.pb.h"
#include "SocialServiceImpl.h"
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
#else
    std::cout << "SocialService built without gRPC. 請安裝依賴或執行 install_micro_deps.sh。\n";
#endif
    return 0;
}


