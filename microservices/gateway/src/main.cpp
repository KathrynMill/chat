#include <iostream>

#include "json.hpp"
using json = nlohmann::json;
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <vector>
#include <cstdlib>
#include <thread>
#include <chrono>

#ifdef HAVE_CPPKAFKA
#include <cppkafka/cppkafka.h>
#include <thread>
#include <atomic>
#endif

#ifdef HAVE_MUDUO
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
using namespace muduo;
using namespace muduo::net;
#endif

#ifdef HAVE_GRPC
#include <grpcpp/grpcpp.h>
#include "user_service.grpc.pb.h"
#include "message_service.grpc.pb.h"
#include "social_service.grpc.pb.h"
#endif

#ifdef HAVE_CURL
#include "consul/ConsulClient.h"
#endif

#ifdef HAVE_OPENSSL
#include "circuit/CircuitBreaker.h"
#include "jwt/JwtValidator.h"
#endif

static inline std::vector<std::string> parseEndpoints(const char* envName, const char* fallback) {
    std::vector<std::string> out;
    const char* v = std::getenv(envName);
    std::string s = v ? v : std::string(fallback ? fallback : "");
    if (s.empty()) return out;
    size_t start = 0;
    while (start < s.size()) {
        size_t pos = s.find(',', start);
        std::string item = s.substr(start, pos == std::string::npos ? std::string::npos : pos - start);
        if (!item.empty()) out.push_back(item);
        if (pos == std::string::npos) break;
        start = pos + 1;
    }
    return out;
}

static GatewayServer* g_gateway = nullptr; // for Kafka consumer access

#ifdef HAVE_CURL
static ConsulClient* g_consul = nullptr;
#endif

#ifdef HAVE_OPENSSL
static JwtValidator* g_jwt = nullptr;
#endif

class GatewayServer {
public:
    GatewayServer(EventLoop* loop, const InetAddress& listenAddr)
        : server_(loop, listenAddr, "chat-gateway") {
#ifdef HAVE_GRPC
        user_eps_ = parseEndpoints("SERVICE_USER", "127.0.0.1:60051");
        msg_eps_ = parseEndpoints("SERVICE_MESSAGE", "127.0.0.1:60053");
        social_eps_ = parseEndpoints("SERVICE_SOCIAL", "127.0.0.1:60052");
#endif
        server_.setConnectionCallback(
            [this](const TcpConnectionPtr& conn) {
                if (conn->connected()) {
                    std::cout << "[Gateway] new connection from " << conn->peerAddress().toIpPort() << "\n";
                } else {
                    std::cout << "[Gateway] connection closed " << conn->peerAddress().toIpPort() << "\n";
                    unbindConn(conn);
                }
            });
        server_.setMessageCallback(
            [this](const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
                std::string s = buf->retrieveAllAsString();
                try {
                    auto js = json::parse(s);
                    int msgid = js.value("msgid", 0);
                    if (msgid == 1) { // LOGIN_MSG
#ifdef HAVE_GRPC
                        // 构造 gRPC 請求
                        chat::user::LoginRequest req;
                        req.set_id(js.value("id", 0));
                        req.set_password(js.value("password", std::string("")));
                        chat::user::LoginResponse resp;
                        grpc::ClientContext ctx;
                        auto stub = getUserStub();
                        auto status = stub->Login(&ctx, req, &resp);
                        json out;
                        out["msgid"] = 2; // LOGIN_MSG_ACK
                        if (status.ok()) {
                            out["errno"] = resp.errno();
                            out["errmsg"] = resp.errmsg();
                            out["user"] = { {"id", resp.user().id()}, {"name", resp.user().name()}, {"state", resp.user().state()} };
                            if (resp.errno() == 0) {
                                bindUser(resp.user().id(), conn);
                            }
                        } else {
                            out = { {"msgid", 2}, {"errno", 1}, {"errmsg", status.error_message()} };
                        }
                        conn->send(out.dump());
#else
                        conn->send(s); // 無 gRPC 時暫時回顯
#endif
                    } else if (msgid == 1001) { // ONE_CHAT_MSG
#ifdef HAVE_GRPC
                        chat::message::OneChatRequest req;
                        auto* m = req.mutable_msg();
                        m->set_from_id(js.value("from_id", 0));
                        m->set_to_id(js.value("to_id", 0));
                        m->set_content(js.value("content", std::string("")));
                        m->set_timestamp_ms(js.value("timestamp_ms", 0LL));
                        m->set_msg_id(js.value("msg_id", std::string("")));
                        chat::message::OneChatResponse resp;
                        grpc::ClientContext ctx2;
                        auto stub = getMsgStub();
                        auto status2 = stub->OneChat(&ctx2, req, &resp);
                        json out = { {"msgid", 1002}, {"errno", status2.ok() ? resp.errno() : 1}, {"errmsg", status2.ok() ? resp.errmsg() : status2.error_message()} };
                        conn->send(out.dump());
#else
                        conn->send(s);
#endif
                    } else if (msgid == 1003) { // GROUP_CHAT_MSG
#ifdef HAVE_GRPC
                        chat::message::GroupChatRequest req;
                        auto* m = req.mutable_msg();
                        m->set_from_id(js.value("from_id", 0));
                        m->set_group_id(js.value("group_id", 0));
                        m->set_content(js.value("content", std::string("")));
                        m->set_timestamp_ms(js.value("timestamp_ms", 0LL));
                        m->set_msg_id(js.value("msg_id", std::string("")));
                        chat::message::GroupChatResponse resp;
                        grpc::ClientContext ctx3;
                        auto stub = getMsgStub();
                        auto status3 = stub->GroupChat(&ctx3, req, &resp);
                        json out = { {"msgid", 1004}, {"errno", status3.ok() ? resp.errno() : 1}, {"errmsg", status3.ok() ? resp.errmsg() : status3.error_message()} };
                        conn->send(out.dump());
#else
                        conn->send(s);
#endif
                    } else if (msgid == 2001) { // ADD_FRIEND
#ifdef HAVE_GRPC
                        chat::social::AddFriendRequest req;
                        req.set_user_id(js.value("user_id", 0));
                        req.set_friend_id(js.value("friend_id", 0));
                        chat::social::AddFriendResponse resp;
                        grpc::ClientContext ctx4;
                        auto stub = getSocialStub();
                        auto status4 = stub->AddFriend(&ctx4, req, &resp);
                        json out = { {"msgid", 2002}, {"errno", status4.ok() ? resp.errno() : 1}, {"errmsg", status4.ok() ? resp.errmsg() : status4.error_message()} };
                        conn->send(out.dump());
#else
                        conn->send(s);
#endif
                    } else if (msgid == 2003) { // CREATE_GROUP
#ifdef HAVE_GRPC
                        chat::social::CreateGroupRequest req;
                        req.set_owner_id(js.value("owner_id", 0));
                        req.set_name(js.value("name", std::string("")));
                        req.set_desc(js.value("desc", std::string("")));
                        chat::social::CreateGroupResponse resp;
                        grpc::ClientContext ctx5;
                        auto stub = getSocialStub();
                        auto status5 = stub->CreateGroup(&ctx5, req, &resp);
                        json out = { {"msgid", 2004}, {"errno", status5.ok() ? resp.errno() : 1}, {"errmsg", status5.ok() ? resp.errmsg() : status5.error_message()}, {"group_id", resp.group_id()} };
                        conn->send(out.dump());
#else
                        conn->send(s);
#endif
                    } else if (msgid == 2005) { // ADD_GROUP_MEMBER
#ifdef HAVE_GRPC
                        chat::social::AddGroupRequest req;
                        req.set_user_id(js.value("user_id", 0));
                        req.set_group_id(js.value("group_id", 0));
                        chat::social::AddGroupResponse resp;
                        grpc::ClientContext ctx6;
                        auto stub = getSocialStub();
                        auto status6 = stub->AddGroup(&ctx6, req, &resp);
                        json out = { {"msgid", 2006}, {"errno", status6.ok() ? resp.errno() : 1}, {"errmsg", status6.ok() ? resp.errmsg() : status6.error_message()} };
                        conn->send(out.dump());
#else
                        conn->send(s);
#endif
                    } else {
                        conn->send(s);
                    }
                } catch (...) {
                    conn->send(s);
                }
            });
    }

    void start() { server_.start(); }

private:
    TcpServer server_;
#ifdef HAVE_GRPC
    std::vector<std::string> user_eps_, msg_eps_, social_eps_;
    std::atomic<size_t> user_rr_{0}, msg_rr_{0}, social_rr_{0};
    
#ifdef HAVE_OPENSSL
    std::unordered_map<std::string, CircuitBreaker> circuitBreakers_;
#endif

    std::unique_ptr<chat::user::UserService::Stub> getUserStub() {
#ifdef HAVE_CURL
        // 從 Consul 获取服务实例
        if (g_consul) {
            auto instances = g_consul->getHealthyServiceInstances("chat-user-service");
            if (!instances.empty()) {
                static std::atomic<size_t> rr{0};
                auto& instance = instances[rr++ % instances.size()];
                std::string ep = instance.address + ":" + std::to_string(instance.port);
                auto ch = grpc::CreateChannel(ep, grpc::InsecureChannelCredentials());
                return chat::user::UserService::NewStub(ch);
            }
        }
#endif
        // 回退到环境变数或預设值
        std::string ep = user_eps_.empty() ? std::string("127.0.0.1:60051") : user_eps_[(user_rr_++) % user_eps_.size()];
        auto ch = grpc::CreateChannel(ep, grpc::InsecureChannelCredentials());
        return chat::user::UserService::NewStub(ch);
    }
    std::unique_ptr<chat::message::MessageService::Stub> getMsgStub() {
#ifdef HAVE_CURL
        if (g_consul) {
            auto instances = g_consul->getHealthyServiceInstances("chat-message-service");
            if (!instances.empty()) {
                static std::atomic<size_t> rr{0};
                auto& instance = instances[rr++ % instances.size()];
                std::string ep = instance.address + ":" + std::to_string(instance.port);
                auto ch = grpc::CreateChannel(ep, grpc::InsecureChannelCredentials());
                return chat::message::MessageService::NewStub(ch);
            }
        }
#endif
        std::string ep = msg_eps_.empty() ? std::string("127.0.0.1:60053") : msg_eps_[(msg_rr_++) % msg_eps_.size()];
        auto ch = grpc::CreateChannel(ep, grpc::InsecureChannelCredentials());
        return chat::message::MessageService::NewStub(ch);
    }
    std::unique_ptr<chat::social::SocialService::Stub> getSocialStub() {
#ifdef HAVE_CURL
        if (g_consul) {
            auto instances = g_consul->getHealthyServiceInstances("chat-social-service");
            if (!instances.empty()) {
                static std::atomic<size_t> rr{0};
                auto& instance = instances[rr++ % instances.size()];
                std::string ep = instance.address + ":" + std::to_string(instance.port);
                auto ch = grpc::CreateChannel(ep, grpc::InsecureChannelCredentials());
                return chat::social::SocialService::NewStub(ch);
            }
        }
#endif
        std::string ep = social_eps_.empty() ? std::string("127.0.0.1:60052") : social_eps_[(social_rr_++) % social_eps_.size()];
        auto ch = grpc::CreateChannel(ep, grpc::InsecureChannelCredentials());
        return chat::social::SocialService::NewStub(ch);
    }
#endif

    std::unordered_map<int, TcpConnectionPtr> online_;
    std::mutex mu_;

    void bindUser(int userId, const TcpConnectionPtr& conn) {
        std::lock_guard<std::mutex> lk(mu_);
        online_[userId] = conn;
    }
    void unbindConn(const TcpConnectionPtr& conn) {
        std::lock_guard<std::mutex> lk(mu_);
        for (auto it = online_.begin(); it != online_.end(); ) {
            if (it->second == conn) it = online_.erase(it); else ++it;
        }
    }
public:
    bool sendToUser(int userId, const std::string& payload) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = online_.find(userId);
        if (it == online_.end()) return false;
        it->second->send(payload);
        return true;
    }
};


int main(int argc, char** argv) {
    // 初始化 Consul 客户端
#ifdef HAVE_CURL
    std::string consulUrl = std::getenv("CONSUL_URL") ? std::getenv("CONSUL_URL") : std::string("http://127.0.0.1:8500");
    g_consul = new ConsulClient(consulUrl);
    std::cout << "Gateway: Consul client initialized: " << consulUrl << "\n";
#endif

    // 初始化 JWT 验证器
#ifdef HAVE_OPENSSL
    std::string jwtSecret = std::getenv("JWT_SECRET") ? std::getenv("JWT_SECRET") : std::string("your-secret-key");
    g_jwt = new JwtValidator(jwtSecret);
    std::cout << "Gateway: JWT validator initialized\n";
#endif

#ifdef HAVE_CPPKAFKA
    std::atomic<bool> running{true};
    std::thread kafkaThread([&running]() {
        std::string brokers = std::getenv("KAFKA_BROKERS") ? std::getenv("KAFKA_BROKERS") : std::string("127.0.0.1:9092");
        cppkafka::Configuration cfg = {
            { "metadata.broker.list", brokers },
            { "group.id", "chat-gateway-group" },
            { "enable.partition.eof", true },
        };
        cppkafka::Consumer consumer(cfg);
        consumer.subscribe({"chat.private"});
        std::cout << "Gateway Kafka consumer subscribed to topic chat.private\n";
        while (running.load()) {
            auto msg = consumer.poll(std::chrono::milliseconds(200));
            if (!msg) continue;
            if (msg.get_error()) {
                if (!msg.is_eof()) std::cerr << "Kafka error: " << msg.get_error() << "\n";
                continue;
            }
            std::string payload = msg.get_payload();
            try {
                auto js = json::parse(payload);
                int to_id = js.value("to_id", 0);
                if (to_id != 0 && g_gateway) {
                    if (!g_gateway->sendToUser(to_id, payload)) {
                        // offline: 忽略（已由 MessageService 落庫 offline_msgs）
                    }
                }
            } catch (...) {
                // ignore
            }
        }
        consumer.close();
    });
#endif
#ifdef HAVE_MUDUO
    EventLoop loop;
    InetAddress addr(7000);
    GatewayServer server(&loop, addr);
    g_gateway = &server;
    server.start();
    std::cout << "Chat Gateway (muduo) listening on 0.0.0.0:7000\n";
    loop.loop();
#else
    std::cout << "Chat Gateway built without muduo.\n";
    std::cout << "請安裝 muduo 後重建，或执行 install_micro_deps.sh。\n";
#endif
#ifdef HAVE_CPPKAFKA
    running.store(false);
    if (kafkaThread.joinable()) kafkaThread.join();
#endif

    // 清理资源
#ifdef HAVE_CURL
    delete g_consul;
#endif
#ifdef HAVE_OPENSSL
    delete g_jwt;
#endif

    return 0;
}


