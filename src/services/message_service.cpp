#include <crow.h>
#include <string>

int main(int argc, char** argv) {
    const char* bind_addr = (argc > 1) ? argv[1] : "0.0.0.0";
    const uint16_t port = (argc > 2) ? static_cast<uint16_t>(std::stoi(argv[2])) : 7001;

    crow::SimpleApp app;

    // 健康檢查
    CROW_ROUTE(app, "/healthz").methods(crow::HTTPMethod::GET)([] {
        return crow::response(200, "ok");
    });

    // 最小消息API：一對一/群發占位
    CROW_ROUTE(app, "/api/v1/message/one").methods(crow::HTTPMethod::POST)([](const crow::request& req){
        // TODO: 解析 body，校驗，轉發到 Redis 或內部邏輯
        return crow::response(202, "accepted");
    });

    CROW_ROUTE(app, "/api/v1/message/group").methods(crow::HTTPMethod::POST)([](const crow::request& req){
        return crow::response(202, "accepted");
    });

    app.bindaddr(bind_addr).port(port).multithreaded().run();
    return 0;
}


