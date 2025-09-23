#include <crow.h>

int main() {
    crow::SimpleApp app;

    // 簡單 HTTP 路由
    CROW_ROUTE(app, "/")([](){ return "Hello, Crow minimal demo!"; });

    // WebSocket echo demo
    CROW_ROUTE(app, "/ws").websocket()
        .onopen([](crow::websocket::connection& conn){
            CROW_LOG_INFO << "WebSocket opened";
        })
        .onmessage([](crow::websocket::connection& conn, const std::string& data, bool is_binary){
            conn.send_text("echo: " + data);
        })
        .onclose([](crow::websocket::connection& conn, const std::string& reason){
            CROW_LOG_INFO << "WebSocket closed: " << reason;
        });

    app.port(3000).multithreaded().run();
    return 0;
} 