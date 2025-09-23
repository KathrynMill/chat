#include <iostream>

#include "json.hpp"
using json = nlohmann::json;

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

class GatewayServer {
public:
    GatewayServer(EventLoop* loop, const InetAddress& listenAddr)
        : server_(loop, listenAddr, "chat-gateway") {
#ifdef HAVE_GRPC
        auto user_channel = grpc::CreateChannel("127.0.0.1:60051", grpc::InsecureChannelCredentials());
        user_stub_ = chat::user::UserService::NewStub(user_channel);
        auto msg_channel = grpc::CreateChannel("127.0.0.1:60053", grpc::InsecureChannelCredentials());
        message_stub_ = chat::message::MessageService::NewStub(msg_channel);
        auto social_channel = grpc::CreateChannel("127.0.0.1:60052", grpc::InsecureChannelCredentials());
        social_stub_ = chat::social::SocialService::NewStub(social_channel);
#endif
        server_.setConnectionCallback(
            [this](const TcpConnectionPtr& conn) {
                if (conn->connected()) {
                    std::cout << "[Gateway] new connection from " << conn->peerAddress().toIpPort() << "\n";
                } else {
                    std::cout << "[Gateway] connection closed " << conn->peerAddress().toIpPort() << "\n";
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
                        // 構造 gRPC 請求
                        chat::user::LoginRequest req;
                        req.set_id(js.value("id", 0));
                        req.set_password(js.value("password", std::string("")));
                        chat::user::LoginResponse resp;
                        grpc::ClientContext ctx;
                        auto status = user_stub_->Login(&ctx, req, &resp);
                        json out;
                        out["msgid"] = 2; // LOGIN_MSG_ACK
                        if (status.ok()) {
                            out["errno"] = resp.errno();
                            out["errmsg"] = resp.errmsg();
                            out["user"] = { {"id", resp.user().id()}, {"name", resp.user().name()}, {"state", resp.user().state()} };
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
                        auto status2 = message_stub_->OneChat(&ctx2, req, &resp);
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
                        auto status3 = message_stub_->GroupChat(&ctx3, req, &resp);
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
                        auto status4 = social_stub_->AddFriend(&ctx4, req, &resp);
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
                        auto status5 = social_stub_->CreateGroup(&ctx5, req, &resp);
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
                        auto status6 = social_stub_->AddGroup(&ctx6, req, &resp);
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
    std::unique_ptr<chat::user::UserService::Stub> user_stub_;
#endif
};


int main(int argc, char** argv) {
#ifdef HAVE_CPPKAFKA
    std::atomic<bool> running{true};
    std::thread kafkaThread([&running]() {
        cppkafka::Configuration cfg = {
            { "metadata.broker.list", "127.0.0.1:9092" },
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
            // TODO: 解析消息，根據 to_id 推送在線連線
            std::cout << "[Kafka] received: " << payload << "\n";
        }
        consumer.close();
    });
#endif
#ifdef HAVE_MUDUO
    EventLoop loop;
    InetAddress addr(7000);
    GatewayServer server(&loop, addr);
    server.start();
    std::cout << "Chat Gateway (muduo) listening on 0.0.0.0:7000\n";
    loop.loop();
#else
    std::cout << "Chat Gateway built without muduo.\n";
    std::cout << "請安裝 muduo 後重建，或執行 install_micro_deps.sh。\n";
#endif
#ifdef HAVE_CPPKAFKA
    running.store(false);
    if (kafkaThread.joinable()) kafkaThread.join();
#endif
    return 0;
}


