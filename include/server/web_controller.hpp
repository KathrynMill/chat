#ifndef WEB_CONTROLLER_H
#define WEB_CONTROLLER_H

#include <crow.h>
#include <crow/websocket.h>
#include <unordered_map>
#include <mutex>
#include <memory>
#include "chatservice.hpp"
#include "json.hpp"

using json = nlohmann::json;

// Web控制器類，處理HTTP API和WebSocket連接
class WebController
{
public:
    static WebController* instance();
    
    // 初始化Web服務
    void init();
    
    // HTTP API處理函數
    void handleLogin(const crow::request& req, crow::response& res);
    void handleRegister(const crow::request& req, crow::response& res);
    void handleFindUserId(const crow::request& req, crow::response& res);
    void handleGetFriends(const crow::request& req, crow::response& res);
    void handleAddFriend(const crow::request& req, crow::response& res);
    void handleDebugUsers(const crow::request& req, crow::response& res);
    void handleDebugClear(const crow::request& req, crow::response& res);
    
    // WebSocket處理函數
    void handleWebSocketConnection(crow::websocket::connection& conn);
    void handleWebSocketMessage(crow::websocket::connection& conn, const std::string& data, bool is_binary);
    void handleWebSocketClose(crow::websocket::connection& conn);
    
    // 發送消息到WebSocket連接
    void sendMessageToUser(int userId, const json& message);
    
    // 生成JWT token
    std::string generateToken(int userId);
    
    // 驗證JWT token
    bool verifyToken(const std::string& token, int& userId);

private:
    WebController();
    
    // 存儲WebSocket連接
    std::unordered_map<int, crow::websocket::connection*> _userWebSocketMap;
    std::mutex _wsMutex;
    
    // JWT密鑰
    const std::string JWT_SECRET = "your-secret-key-cpp";
    
    // 路由設置
    void setupRoutes();
    
    // 靜態文件服務
    void serveStaticFiles();
};

#endif 