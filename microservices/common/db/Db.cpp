#include "db/Db.h"
#include <cstdlib>
#include <iostream>

#ifdef HAVE_MARIADB
#include <mysql/mysql.h>
#endif

DbConnection::DbConnection() {
}

DbConnection::~DbConnection() {
#ifdef HAVE_MARIADB
    if (conn_) {
        MYSQL* c = static_cast<MYSQL*>(conn_);
        mysql_close(c);
        conn_ = nullptr;
    }
#endif
}

bool DbConnection::connect(const DbConfig& cfg) {
#ifdef HAVE_MARIADB
    MYSQL* c = mysql_init(nullptr);
    if (!c) return false;
    if (!mysql_real_connect(c, cfg.host.c_str(), cfg.user.c_str(), cfg.password.c_str(),
                            cfg.database.c_str(), cfg.port, nullptr, 0)) {
        std::cerr << "DB connect error: " << mysql_error(c) << "\n";
        mysql_close(c);
        return false;
    }
    conn_ = c;
    return true;
#else
    (void)cfg;
    std::cerr << "DB client not available, skipped connect.\n";
    return false;
#endif
}

bool DbConnection::execute(const std::string& sql) {
#ifdef HAVE_MARIADB
    if (!conn_) return false;
    MYSQL* c = static_cast<MYSQL*>(conn_);
    if (mysql_query(c, sql.c_str()) != 0) {
        std::cerr << "DB exec error: " << mysql_error(c) << "\n";
        return false;
    }
    return true;
#else
    (void)sql;
    return false;
#endif
}

bool DbConnection::querySingleString(const std::string& sql, std::string& out) {
#ifdef HAVE_MARIADB
    if (!conn_) return false;
    MYSQL* c = static_cast<MYSQL*>(conn_);
    if (mysql_query(c, sql.c_str()) != 0) {
        std::cerr << "DB query error: " << mysql_error(c) << "\n";
        return false;
    }
    MYSQL_RES* res = mysql_store_result(c);
    if (!res) return false;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row && row[0]) {
        out = row[0];
        mysql_free_result(res);
        return true;
    }
    mysql_free_result(res);
    return false;
#else
    (void)sql; (void)out;
    return false;
#endif
}

bool DbConnection::queryEach(const std::string& sql,
                   const std::function<void(const std::vector<std::string>&)>& on_row) {
#ifdef HAVE_MARIADB
    if (!conn_) return false;
    MYSQL* c = static_cast<MYSQL*>(conn_);
    if (mysql_query(c, sql.c_str()) != 0) {
        std::cerr << "DB query error: " << mysql_error(c) << "\n";
        return false;
    }
    MYSQL_RES* res = mysql_store_result(c);
    if (!res) return false;
    int n = mysql_num_fields(res);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::vector<std::string> cols;
        cols.reserve(n);
        for (int i = 0; i < n; ++i) cols.emplace_back(row[i] ? row[i] : "");
        on_row(cols);
    }
    mysql_free_result(res);
    return true;
#else
    (void)sql; (void)on_row;
    return false;
#endif
}


