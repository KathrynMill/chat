#include <iostream>

#ifdef HAVE_GRPC
#include <grpcpp/grpcpp.h>
using grpc::Server;
using grpc::ServerBuilder;
#include "message_service.grpc.pb.h"
#include "MessageServiceImpl.h"
#endif

#ifdef HAVE_CPPKAFKA
#include <cppkafka/cppkafka.h>
#endif

int main(int argc, char** argv) {
#ifdef HAVE_GRPC
    std::string server_address("0.0.0.0:60053");
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    MessageServiceImpl service;
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "MessageService gRPC listening on " << server_address << "\n";

#ifdef HAVE_CPPKAFKA
    try {
        cppkafka::Configuration cfg = {
            { "metadata.broker.list", "127.0.0.1:9092" }
        };
        cppkafka::Producer producer(cfg);
        std::string payload = "{\"type\":\"ONE_CHAT_MSG\",\"fromid\":1,\"toid\":2,\"msg\":\"hello from MessageService\"}";
        producer.produce(cppkafka::MessageBuilder("chat.private").partition(0).payload(payload));
        producer.flush();
        std::cout << "MessageService sent test message to Kafka topic chat.private\n";
    } catch (const std::exception& ex) {
        std::cerr << "Kafka producer error: " << ex.what() << "\n";
    }
#endif
    server->Wait();
#else
    std::cout << "MessageService built without gRPC. 請安裝依賴或執行 install_micro_deps.sh。\n";
#endif
    return 0;
}

#include <iostream>

int main(int argc, char** argv) {
    std::cout << "MessageService (skeleton) started.\n";
    // 後續加入：gRPC 服務註冊與實作
    return 0;
}


