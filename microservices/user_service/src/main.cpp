#include <iostream>

#ifdef HAVE_GRPC
#include <grpcpp/grpcpp.h>
using grpc::Server;
using grpc::ServerBuilder;
#include "user_service.grpc.pb.h"
#include "UserServiceImpl.h"
#endif

#ifdef HAVE_REDIS
#include <sw/redis++/redis++.h>
#endif

int main(int argc, char** argv) {
#ifdef HAVE_GRPC
    std::string server_address("0.0.0.0:60051");
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    UserServiceImpl service;
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "UserService gRPC listening on " << server_address << "\n";

#ifdef HAVE_REDIS
    try {
        auto url = std::getenv("REDIS_URL");
        std::string redis_url = url ? url : "tcp://127.0.0.1:6379";
        sw::redis::Redis redis(redis_url);
        redis.set("user_service:health", "ok");
        auto v = redis.get("user_service:health");
        std::cout << "UserService Redis ping: " << (v ? *v : std::string("<nil>")) << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Redis error: " << ex.what() << "\n";
    }
#endif
    server->Wait();
#else
    std::cout << "UserService built without gRPC. 請安裝依賴或執行 install_micro_deps.sh。\n";
#endif
    return 0;
}


