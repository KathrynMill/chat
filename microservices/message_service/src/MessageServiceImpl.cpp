#ifdef HAVE_GRPC
#include "MessageServiceImpl.h"
#include "db/Db.h"
#include <cstdlib>
#include <sstream>
#include "json.hpp"
using json = nlohmann::json;

::grpc::Status MessageServiceImpl::OneChat(::grpc::ServerContext* ctx,
                                           const chat::message::OneChatRequest* req,
                                           chat::message::OneChatResponse* resp) {
    (void)ctx;
    const auto& m = req->msg();
    DbConfig cfg;
    if (auto v = std::getenv("DB_HOST")) cfg.host = v;
    if (auto v = std::getenv("DB_PORT")) cfg.port = std::atoi(v);
    if (auto v = std::getenv("DB_USER")) cfg.user = v;
    if (auto v = std::getenv("DB_PASS")) cfg.password = v;
    if (auto v = std::getenv("DB_NAME")) cfg.database = v;
    DbConnection db;
    if (!db.connect(cfg)) {
        resp->set_errno(1);
        resp->set_errmsg("db connect failed");
        return ::grpc::Status::OK;
    }
    std::ostringstream oss;
    oss << "INSERT INTO messages(from_id, to_id, group_id, content, timestamp_ms, msg_id) VALUES("
        << m.from_id() << "," << m.to_id() << ",0,'" << m.content() << "'," << m.timestamp_ms() << ",'" << m.msg_id() << "')";
    if (!db.execute(oss.str())) {
        resp->set_errno(2);
        resp->set_errmsg("insert message failed");
        return ::grpc::Status::OK;
    }
    // 簡化：先當對方離線，写入 offline_msgs
    std::ostringstream off;
    off << "INSERT INTO offline_msgs(user_id, payload) VALUES("
        << m.to_id() << ",'{\\"type\\":\\"ONE_CHAT_MSG\\",\\"from_id\\":" << m.from_id() << ",\\"content\\":\\"" << m.content() << "\\"}')";
    db.execute(off.str());
    
    // 发送 Kafka 讯息
#ifdef HAVE_CPPKAFKA
    try {
        std::string brokers = std::getenv("KAFKA_BROKERS") ? std::getenv("KAFKA_BROKERS") : std::string("127.0.0.1:9092");
        cppkafka::Configuration cfg = {{"metadata.broker.list", brokers}};
        cppkafka::Producer producer(cfg);
        
        json msg_payload;
        msg_payload["to_id"] = m.to_id();
        msg_payload["from_id"] = m.from_id();
        msg_payload["content"] = m.content();
        msg_payload["timestamp_ms"] = m.timestamp_ms();
        msg_payload["msg_id"] = m.msg_id();
        
        producer.produce(cppkafka::MessageBuilder("chat.private").payload(msg_payload.dump()));
        producer.flush();
    } catch (...) {
        // ignore kafka errors
    }
#endif
    
    resp->set_errno(0);
    resp->set_errmsg("");
    return ::grpc::Status::OK;
}

::grpc::Status MessageServiceImpl::GroupChat(::grpc::ServerContext* ctx,
                                             const chat::message::GroupChatRequest* req,
                                             chat::message::GroupChatResponse* resp) {
    (void)ctx;
    const auto& m = req->msg();
    DbConfig cfg;
    if (auto v = std::getenv("DB_HOST")) cfg.host = v;
    if (auto v = std::getenv("DB_PORT")) cfg.port = std::atoi(v);
    if (auto v = std::getenv("DB_USER")) cfg.user = v;
    if (auto v = std::getenv("DB_PASS")) cfg.password = v;
    if (auto v = std::getenv("DB_NAME")) cfg.database = v;
    DbConnection db;
    if (!db.connect(cfg)) {
        resp->set_errno(1);
        resp->set_errmsg("db connect failed");
        return ::grpc::Status::OK;
    }
    std::ostringstream oss;
    oss << "INSERT INTO messages(from_id, to_id, group_id, content, timestamp_ms, msg_id) VALUES("
        << m.from_id() << ",0," << m.group_id() << ",'" << m.content() << "'," << m.timestamp_ms() << ",'" << m.msg_id() << "')";
    if (!db.execute(oss.str())) {
        resp->set_errno(2);
        resp->set_errmsg("insert group message failed");
        return ::grpc::Status::OK;
    }
    // 发送 Kafka 群组讯息
#ifdef HAVE_CPPKAFKA
    try {
        std::string brokers = std::getenv("KAFKA_BROKERS") ? std::getenv("KAFKA_BROKERS") : std::string("127.0.0.1:9092");
        cppkafka::Configuration cfg = {{"metadata.broker.list", brokers}};
        cppkafka::Producer producer(cfg);
        
        json msg_payload;
        msg_payload["group_id"] = m.group_id();
        msg_payload["from_id"] = m.from_id();
        msg_payload["content"] = m.content();
        msg_payload["timestamp_ms"] = m.timestamp_ms();
        msg_payload["msg_id"] = m.msg_id();
        
        producer.produce(cppkafka::MessageBuilder("chat.group").payload(msg_payload.dump()));
        producer.flush();
    } catch (...) {
        // ignore kafka errors
    }
#endif
    
    resp->set_errno(0);
    resp->set_errmsg("");
    return ::grpc::Status::OK;
}

::grpc::Status MessageServiceImpl::ListMessages(::grpc::ServerContext* ctx,
                                                const chat::message::ListMessagesRequest* req,
                                                chat::message::ListMessagesResponse* resp) {
    (void)ctx;
    DbConfig cfg;
    if (auto v = std::getenv("DB_HOST")) cfg.host = v;
    if (auto v = std::getenv("DB_PORT")) cfg.port = std::atoi(v);
    if (auto v = std::getenv("DB_USER")) cfg.user = v;
    if (auto v = std::getenv("DB_PASS")) cfg.password = v;
    if (auto v = std::getenv("DB_NAME")) cfg.database = v;

    DbConnection db;
    if (!db.connect(cfg)) {
        return ::grpc::Status::OK;
    }
    std::ostringstream q;
    // scope: "private:<peer>" or "group:<gid>"
    std::string scope = req->scope();
    int64_t since = req->since_ms();
    int limit = req->limit() > 0 ? req->limit() : 100;
    if (scope.rfind("private:", 0) == 0) {
        int peer = std::atoi(scope.substr(8).c_str());
        q << "SELECT from_id,to_id,group_id,content,timestamp_ms,IFNULL(msg_id,'') FROM messages WHERE "
          << "((from_id=" << req->user_id() << " AND to_id=" << peer << ") OR (from_id=" << peer << " AND to_id=" << req->user_id() << "))";
    } else if (scope.rfind("group:", 0) == 0) {
        int gid = std::atoi(scope.substr(6).c_str());
        q << "SELECT from_id,to_id,group_id,content,timestamp_ms,IFNULL(msg_id,'') FROM messages WHERE group_id=" << gid;
    } else {
        return ::grpc::Status::OK;
    }
    if (since > 0) {
        q << " AND timestamp_ms>=" << since;
    }
    q << " ORDER BY id DESC LIMIT " << limit;
    db.queryEach(q.str(), [&](const std::vector<std::string>& cols){
        if (cols.size() >= 6) {
            auto* m = resp->add_messages();
            m->set_from_id(std::atoi(cols[0].c_str()));
            m->set_to_id(std::atoi(cols[1].c_str()));
            m->set_group_id(std::atoi(cols[2].c_str()));
            m->set_content(cols[3]);
            m->set_timestamp_ms(std::atoll(cols[4].c_str()));
            m->set_msg_id(cols[5]);
        }
    });
    return ::grpc::Status::OK;
}
#endif


