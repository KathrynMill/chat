#pragma once
#include <string>
#include <vector>
#include <functional>

class DbConfig {
public:
    std::string host = "127.0.0.1";
    int port = 3306;
    std::string user = "root";
    std::string password = "";
    std::string database = "chatdb";
};

class DbConnection {
public:
    DbConnection();
    ~DbConnection();

    bool connect(const DbConfig& cfg);
    bool execute(const std::string& sql);
    bool querySingleString(const std::string& sql, std::string& out);
    bool queryEach(const std::string& sql,
                   const std::function<void(const std::vector<std::string>&)>& on_row);

private:
    void* conn_ = nullptr; // Opaque pointer to MYSQL*
};


