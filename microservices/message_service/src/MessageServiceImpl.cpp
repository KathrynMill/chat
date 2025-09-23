#ifdef HAVE_GRPC
#include "MessageServiceImpl.h"
#include "db/Db.h"
#include <cstdlib>
#include <sstream>

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
    // 簡化：先當對方離線，寫入 offline_msgs
    std::ostringstream off;
    off << "INSERT INTO offline_msgs(user_id, payload) VALUES("
        << m.to_id() << ",'{\\"type\\":\\"ONE_CHAT_MSG\\",\\"from_id\\":" << m.from_id() << ",\\"content\\":\\"" << m.content() << "\\"}')";
    db.execute(off.str());
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
    // 簡化：不展開群成員，先落 DB
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


