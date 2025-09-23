#ifdef HAVE_GRPC
#include "SocialServiceImpl.h"
#include "db/Db.h"
#include <cstdlib>
#include <sstream>

::grpc::Status SocialServiceImpl::AddFriend(::grpc::ServerContext* ctx,
                                            const chat::social::AddFriendRequest* req,
                                            chat::social::AddFriendResponse* resp) {
    (void)ctx;
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
    oss << "INSERT IGNORE INTO friends(user_id, friend_id) VALUES("
        << req->user_id() << "," << req->friend_id() << ")";
    bool ok1 = db.execute(oss.str());
    std::ostringstream oss2;
    oss2 << "INSERT IGNORE INTO friends(user_id, friend_id) VALUES("
        << req->friend_id() << "," << req->user_id() << ")";
    bool ok2 = db.execute(oss2.str());
    if (ok1 && ok2) {
        resp->set_errno(0);
        resp->set_errmsg("");
    } else {
        resp->set_errno(2);
        resp->set_errmsg("insert failed");
    }
    return ::grpc::Status::OK;
}

::grpc::Status SocialServiceImpl::ListFriends(::grpc::ServerContext* ctx,
                                              const chat::social::ListFriendsRequest* req,
                                              chat::social::ListFriendsResponse* resp) {
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
    q << "SELECT u.id,u.name,u.state FROM friends f JOIN users u ON u.id=f.friend_id WHERE f.user_id="
      << req->user_id();
    db.queryEach(q.str(), [&](const std::vector<std::string>& cols){
        if (cols.size() >= 3) {
            auto* u = resp->add_friends();
            u->set_id(std::atoi(cols[0].c_str()));
            u->set_name(cols[1]);
            u->set_state(cols[2]);
        }
    });
    return ::grpc::Status::OK;
}

::grpc::Status SocialServiceImpl::CreateGroup(::grpc::ServerContext* ctx,
                                              const chat::social::CreateGroupRequest* req,
                                              chat::social::CreateGroupResponse* resp) {
    (void)ctx;
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
    oss << "INSERT INTO groups(owner_id, name, `desc`) VALUES("
        << req->owner_id() << ",'" << req->name() << "','" << req->desc() << "')";
    if (!db.execute(oss.str())) {
        resp->set_errno(2);
        resp->set_errmsg("insert group failed");
        return ::grpc::Status::OK;
    }
    // 取回剛建立 group 的 id（簡化：以名稱+owner 唯一查一次）
    std::ostringstream q;
    q << "SELECT id FROM groups WHERE owner_id=" << req->owner_id()
      << " AND name='" << req->name() << "' ORDER BY id DESC LIMIT 1";
    std::string gid;
    if (db.querySingleString(q.str(), gid)) {
        resp->set_group_id(std::atoi(gid.c_str()));
    } else {
        resp->set_group_id(0);
    }
    // 把 owner 加入 group_members
    if (resp->group_id() > 0) {
        std::ostringstream gmem;
        gmem << "INSERT IGNORE INTO group_members(group_id, user_id) VALUES("
             << resp->group_id() << "," << req->owner_id() << ")";
        db.execute(gmem.str());
    }
    resp->set_errno(0);
    resp->set_errmsg("");
    return ::grpc::Status::OK;
}

::grpc::Status SocialServiceImpl::AddGroup(::grpc::ServerContext* ctx,
                                           const chat::social::AddGroupRequest* req,
                                           chat::social::AddGroupResponse* resp) {
    (void)ctx;
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
    oss << "INSERT IGNORE INTO group_members(group_id, user_id) VALUES("
        << req->group_id() << "," << req->user_id() << ")";
    if (db.execute(oss.str())) {
        resp->set_errno(0);
        resp->set_errmsg("");
    } else {
        resp->set_errno(2);
        resp->set_errmsg("insert group member failed");
    }
    return ::grpc::Status::OK;
}

::grpc::Status SocialServiceImpl::ListGroups(::grpc::ServerContext* ctx,
                                             const chat::social::ListGroupsRequest* req,
                                             chat::social::ListGroupsResponse* resp) {
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
    q << "SELECT g.id,g.name,COUNT(m2.user_id) AS mc FROM groups g "
      << "JOIN group_members m ON m.group_id=g.id AND m.user_id=" << req->user_id() << " "
      << "LEFT JOIN group_members m2 ON m2.group_id=g.id "
      << "GROUP BY g.id,g.name";
    db.queryEach(q.str(), [&](const std::vector<std::string>& cols){
        if (cols.size() >= 3) {
            auto* g = resp->add_groups();
            g->set_id(std::atoi(cols[0].c_str()));
            g->set_name(cols[1]);
            g->set_member_count(std::atoi(cols[2].c_str()));
        }
    });
    return ::grpc::Status::OK;
}
#endif


