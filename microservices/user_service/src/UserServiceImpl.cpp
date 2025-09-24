#ifdef HAVE_GRPC
#include "UserServiceImpl.h"
#include "db/Db.h"

::grpc::Status UserServiceImpl::Reg(::grpc::ServerContext* ctx,
                                    const chat::user::RegRequest* req,
                                    chat::user::RegResponse* resp) {
    DbConfig cfg;
    if (const char* v = std::getenv("DB_HOST")) cfg.host = v;
    if (const char* v = std::getenv("DB_USER")) cfg.user = v;
    if (const char* v = std::getenv("DB_PASS")) cfg.password = v;
    if (const char* v = std::getenv("DB_NAME")) cfg.database = v;
    if (const char* v = std::getenv("DB_PORT")) cfg.port = std::atoi(v);
    DbConnection db;
    if (db.connect(cfg)) {
        std::string name = req->name();
        std::string pwd = req->password();
        std::string sql = "INSERT INTO users(name, hashed_pwd, state) VALUES('" + name + "','" + pwd + "','offline')";
        if (!db.execute(sql)) {
            resp->set_errno(1);
            resp->set_errmsg("db insert failed");
            return ::grpc::Status::OK;
        }
        resp->set_errno(0);
        resp->set_errmsg("");
        resp->set_user_id(1);
    } else {
        resp->set_errno(1);
        resp->set_errmsg("db connect failed");
    }
    return ::grpc::Status::OK;
}

::grpc::Status UserServiceImpl::Login(::grpc::ServerContext* ctx,
                                      const chat::user::LoginRequest* req,
                                      chat::user::LoginResponse* resp) {
    DbConfig cfg;
    if (const char* v = std::getenv("DB_HOST")) cfg.host = v;
    if (const char* v = std::getenv("DB_USER")) cfg.user = v;
    if (const char* v = std::getenv("DB_PASS")) cfg.password = v;
    if (const char* v = std::getenv("DB_NAME")) cfg.database = v;
    if (const char* v = std::getenv("DB_PORT")) cfg.port = std::atoi(v);
    DbConnection db;
    if (!db.connect(cfg)) {
        resp->set_errno(1);
        resp->set_errmsg("db connect failed");
        return ::grpc::Status::OK;
    }
    // 簡化：以 name 當作 id 或查询演示
    std::string out;
    std::string sql = "SELECT name FROM users WHERE id=" + std::to_string(req->id());
    if (db.querySingleString(sql, out)) {
        auto* u = resp->mutable_user();
        u->set_id(req->id());
        u->set_name(out);
        u->set_state("online");
        resp->set_errno(0);
        resp->set_errmsg("");
    } else {
        resp->set_errno(1);
        resp->set_errmsg("user not found");
    }
    return ::grpc::Status::OK;
}

::grpc::Status UserServiceImpl::Logout(::grpc::ServerContext* ctx,
                                       const chat::user::LogoutRequest* req,
                                       chat::user::LogoutResponse* resp) {
    resp->set_errno(0);
    resp->set_errmsg("");
    return ::grpc::Status::OK;
}
#endif


