#include "web_controller.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>

// 單例實例
static WebController* g_webController = nullptr;

WebController* WebController::instance()
{
    if (g_webController == nullptr)
    {
        g_webController = new WebController();
    }
    return g_webController;
}

WebController::WebController()
{
}

extern crow::SimpleApp app;

void WebController::init()
{
    // 註冊 HTTP 路由
    CROW_ROUTE(app, "/api/login")([this](const crow::request& req){
        if (req.method != crow::HTTPMethod::Post) return crow::response(405);
        crow::response res;
        handleLogin(req, res);
        return res;
    });
    CROW_ROUTE(app, "/api/register")([this](const crow::request& req){
        if (req.method != crow::HTTPMethod::Post) return crow::response(405);
        crow::response res;
        handleRegister(req, res);
        return res;
    });
    CROW_ROUTE(app, "/api/find-user-id")([this](const crow::request& req){
        if (req.method != crow::HTTPMethod::Post) return crow::response(405);
        crow::response res;
        handleFindUserId(req, res);
        return res;
    });
    CROW_ROUTE(app, "/api/friends")([this](const crow::request& req){
        if (req.method != crow::HTTPMethod::Get) return crow::response(405);
        crow::response res;
        handleGetFriends(req, res);
        return res;
    });
    CROW_ROUTE(app, "/api/friends/add")([this](const crow::request& req){
        if (req.method != crow::HTTPMethod::Post) return crow::response(405);
        crow::response res;
        handleAddFriend(req, res);
        return res;
    });
    CROW_ROUTE(app, "/api/debug/users")([this](const crow::request& req){
        if (req.method != crow::HTTPMethod::Get) return crow::response(405);
        crow::response res;
        handleDebugUsers(req, res);
        return res;
    });
    CROW_ROUTE(app, "/api/debug/clear")([this](const crow::request& req){
        if (req.method != crow::HTTPMethod::Post) return crow::response(405);
        crow::response res;
        handleDebugClear(req, res);
        return res;
    });
    // WebSocket 路由
    CROW_ROUTE(app, "/ws").websocket()
        .onopen([this](crow::websocket::connection& conn){ handleWebSocketConnection(conn); })
        .onmessage([this](crow::websocket::connection& conn, const std::string& data, bool is_binary){ handleWebSocketMessage(conn, data, is_binary); })
        .onclose([this](crow::websocket::connection& conn, const std::string& reason){ handleWebSocketClose(conn); });
}

void WebController::handleLogin(const crow::request& req, crow::response& res)
{
    try {
        auto body = crow::json::load(req.body);
        if (!body) {
            res = crow::response(400, "application/json", 
                "{\"success\":false,\"message\":\"無效的JSON格式\"}");
            return;
        }
        
        int id = body["id"].i();
        std::string pwd = body["pwd"].s();
        
        std::cout << "登入請求: id=" << id << ", pwd=" << (pwd.empty() ? "空" : "***") << std::endl;
        
        // 使用現有的ChatService進行登入驗證
        User user = ChatService::instance()->getUserModel().query(id);
        
        if (user.getId() != -1 && user.getPwd() == pwd) {
            // 登入成功
            std::string token = generateToken(id);
            
            json response_json;
            response_json["success"] = true;
            response_json["message"] = "登入成功";
            response_json["token"] = token;
            response_json["user"]["id"] = user.getId();
            response_json["user"]["name"] = user.getName();
            
            res = crow::response(200, "application/json", response_json.dump());
            std::cout << "用戶登入成功: id=" << user.getId() << ", name=" << user.getName() << std::endl;
        } else {
            // 登入失敗
            json response_json;
            response_json["success"] = false;
            response_json["message"] = "用戶ID或密碼錯誤";
            
            res = crow::response(401, "application/json", response_json.dump());
            std::cout << "登入失敗: id=" << id << std::endl;
        }
    } catch (const std::exception& e) {
        json response_json;
        response_json["success"] = false;
        response_json["message"] = "服務器錯誤: " + std::string(e.what());
        res = crow::response(500, "application/json", response_json.dump());
    }
}

void WebController::handleRegister(const crow::request& req, crow::response& res)
{
    try {
        auto body = crow::json::load(req.body);
        if (!body) {
            res = crow::response(400, "application/json", 
                "{\"success\":false,\"message\":\"無效的JSON格式\"}");
            return;
        }
        std::string name = body["name"].s();
        std::string pwd = body["pwd"].s();
        std::cout << "註冊請求: name=" << name << ", pwd=" << (pwd.empty() ? "空" : "***") << std::endl;
        if (name.empty() || pwd.empty()) {
            json response_json;
            response_json["success"] = false;
            response_json["message"] = "請填寫完整資訊";
            res = crow::response(400, "application/json", response_json.dump());
            return;
        }
        // 檢查用戶是否已存在
        User existingUser = ChatService::instance()->getUserModel().queryByName(name);
        if (existingUser.getId() != -1) {
            json response_json;
            response_json["success"] = false;
            response_json["message"] = "用戶名已存在";
            res = crow::response(409, "application/json", response_json.dump());
            return;
        }
        // 創建新用戶
        User newUser;
        newUser.setName(name);
        newUser.setPwd(pwd);
        newUser.setState("online");
        if (ChatService::instance()->getUserModel().insert(newUser)) {
            json response_json;
            response_json["success"] = true;
            response_json["message"] = "註冊成功";
            response_json["userId"] = newUser.getId();
            response_json["userName"] = newUser.getName();
            res = crow::response(200, "application/json", response_json.dump());
            std::cout << "用戶註冊成功: id=" << newUser.getId() << ", name=" << newUser.getName() << std::endl;
        } else {
            json response_json;
            response_json["success"] = false;
            response_json["message"] = "註冊失敗";
            res = crow::response(500, "application/json", response_json.dump());
        }
    } catch (const std::exception& e) {
        json response_json;
        response_json["success"] = false;
        response_json["message"] = "服務器錯誤: " + std::string(e.what());
        res = crow::response(500, "application/json", response_json.dump());
    }
}

void WebController::handleFindUserId(const crow::request& req, crow::response& res)
{
    try {
        auto body = crow::json::load(req.body);
        if (!body) {
            res = crow::response(400, "application/json", 
                "{\"success\":false,\"message\":\"無效的JSON格式\"}");
            return;
        }
        std::string name = body["name"].s();
        std::string pwd = body["pwd"].s();
        std::cout << "找回用戶ID請求: name=" << name << ", pwd=" << (pwd.empty() ? "空" : "***") << std::endl;
        if (name.empty() || pwd.empty()) {
            json response_json;
            response_json["success"] = false;
            response_json["message"] = "請填寫用戶名和密碼";
            res = crow::response(400, "application/json", response_json.dump());
            return;
        }
        // 查找用戶
        User user = ChatService::instance()->getUserModel().queryByName(name);
        if (user.getId() != -1 && user.getPwd() == pwd) {
            json response_json;
            response_json["success"] = true;
            response_json["message"] = "找到用戶";
            response_json["userId"] = user.getId();
            response_json["userName"] = user.getName();
            res = crow::response(200, "application/json", response_json.dump());
            std::cout << "找到用戶: id=" << user.getId() << ", name=" << user.getName() << std::endl;
        } else {
            json response_json;
            response_json["success"] = false;
            response_json["message"] = "用戶名或密碼錯誤，或用戶不存在";
            res = crow::response(404, "application/json", response_json.dump());
            std::cout << "未找到用戶: name=" << name << std::endl;
        }
    } catch (const std::exception& e) {
        json response_json;
        response_json["success"] = false;
        response_json["message"] = "服務器錯誤: " + std::string(e.what());
        res = crow::response(500, "application/json", response_json.dump());
    }
}

void WebController::handleGetFriends(const crow::request& req, crow::response& res)
{
    try {
        // 從Authorization header獲取token
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            json response_json;
            response_json["success"] = false;
            response_json["message"] = "未提供認證令牌";
            res = crow::response(401, "application/json", response_json.dump());
            return;
        }
        
        std::string token = authHeader.substr(7);
        int userId;
        if (!verifyToken(token, userId)) {
            json response_json;
            response_json["success"] = false;
            response_json["message"] = "令牌無效";
            res = crow::response(403, "application/json", response_json.dump());
            return;
        }
        
        // 獲取好友列表
        std::vector<User> friends = ChatService::instance()->getFriendModel().query(userId);
        
        json response_json;
        response_json["success"] = true;
        response_json["friends"] = json::array();
        
        for (const auto& friend_user : friends) {
            json friend_json;
            friend_json["id"] = friend_user.getId();
            friend_json["name"] = friend_user.getName();
            friend_json["status"] = friend_user.getState();
            response_json["friends"].push_back(friend_json);
        }
        
        res = crow::response(200, "application/json", response_json.dump());
    } catch (const std::exception& e) {
        json response_json;
        response_json["success"] = false;
        response_json["message"] = "服務器錯誤: " + std::string(e.what());
        res = crow::response(500, "application/json", response_json.dump());
    }
}

void WebController::handleAddFriend(const crow::request& req, crow::response& res)
{
    try {
        auto body = crow::json::load(req.body);
        if (!body) {
            res = crow::response(400, "application/json", 
                "{\"success\":false,\"message\":\"無效的JSON格式\"}");
            return;
        }
        int friendId = body["friendId"].i();
        // 驗證token
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            json response_json;
            response_json["success"] = false;
            response_json["message"] = "未提供認證令牌";
            res = crow::response(401, "application/json", response_json.dump());
            return;
        }
        std::string token = authHeader.substr(7);
        int userId;
        if (!verifyToken(token, userId)) {
            json response_json;
            response_json["success"] = false;
            response_json["message"] = "令牌無效";
            res = crow::response(403, "application/json", response_json.dump());
            return;
        }
        // 檢查好友是否存在
        User friendUser = ChatService::instance()->getUserModel().query(friendId);
        if (friendUser.getId() == -1) {
            json response_json;
            response_json["success"] = false;
            response_json["message"] = "用戶不存在";
            res = crow::response(404, "application/json", response_json.dump());
            return;
        }
        // 添加好友
        ChatService::instance()->getFriendModel().insert(userId, friendId);
        json response_json;
        response_json["success"] = true;
        response_json["message"] = "好友添加成功";
        res = crow::response(200, "application/json", response_json.dump());
    } catch (const std::exception& e) {
        json response_json;
        response_json["success"] = false;
        response_json["message"] = "服務器錯誤: " + std::string(e.what());
        res = crow::response(500, "application/json", response_json.dump());
    }
}

void WebController::handleDebugUsers(const crow::request& req, crow::response& res)
{
    try {
        // 獲取所有用戶
        std::vector<User> users = ChatService::instance()->getUserModel().queryAll();
        
        json response_json;
        response_json["success"] = true;
        response_json["users"] = json::array();
        
        for (const auto& user : users) {
            json user_json;
            user_json["id"] = user.getId();
            user_json["name"] = user.getName();
            user_json["pwd"] = user.getPwd();
            user_json["status"] = user.getState();
            response_json["users"].push_back(user_json);
        }
        
        res = crow::response(200, "application/json", response_json.dump());
    } catch (const std::exception& e) {
        json response_json;
        response_json["success"] = false;
        response_json["message"] = "服務器錯誤: " + std::string(e.what());
        res = crow::response(500, "application/json", response_json.dump());
    }
}

void WebController::handleDebugClear(const crow::request& req, crow::response& res)
{
    try {
        // 清理所有用戶數據（僅用於調試）
        int userCount = ChatService::instance()->getUserModel().clearAll();
        
        json response_json;
        response_json["success"] = true;
        response_json["message"] = "已清除 " + std::to_string(userCount) + " 個用戶的數據";
        
        res = crow::response(200, "application/json", response_json.dump());
    } catch (const std::exception& e) {
        json response_json;
        response_json["success"] = false;
        response_json["message"] = "服務器錯誤: " + std::string(e.what());
        res = crow::response(500, "application/json", response_json.dump());
    }
}

void WebController::handleWebSocketConnection(crow::websocket::connection& conn)
{
    std::cout << "新的 WebSocket 連接" << std::endl;
    // 只做初始化，不驗證token
}

void WebController::handleWebSocketMessage(crow::websocket::connection& conn, const std::string& data, bool is_binary)
{
    try {
        json message = json::parse(data);
        std::string type = message["type"];
        // 首條消息必須為 AUTH
        if (type == "AUTH") {
            std::string token = message["token"];
            int userId;
            if (!verifyToken(token, userId)) {
                json resp = { {"type", "AUTH_ACK"}, {"success", false}, {"message", "token無效"} };
                conn.send_text(resp.dump());
                conn.close();
                return;
            }
            // 綁定userId
            {
                std::lock_guard<std::mutex> lock(_wsMutex);
                _userWebSocketMap[userId] = &conn;
            }
            // 更新用戶狀態
            User user = ChatService::instance()->getUserModel().query(userId);
            user.setState("online");
            ChatService::instance()->getUserModel().updateState(user);
            json resp = { {"type", "AUTH_ACK"}, {"success", true}, {"message", "認證成功"} };
            conn.send_text(resp.dump());
            std::cout << "用戶 " << user.getName() << " WebSocket認證成功" << std::endl;
            return;
        }
        // 之後的消息必須已經認證
        int userId = -1;
        {
            std::lock_guard<std::mutex> lock(_wsMutex);
            for (const auto& pair : _userWebSocketMap) {
                if (pair.second == &conn) {
                    userId = pair.first;
                    break;
                }
            }
        }
        if (userId == -1) {
            json resp = { {"type", "ERROR"}, {"message", "未認證，請先發送AUTH"} };
            conn.send_text(resp.dump());
            conn.close();
            return;
        }
        // 聊天消息處理（同原邏輯）
        if (type == "ONE_CHAT_MSG") {
            int toId = message["toid"];
            std::string msg = message["msg"];
            std::string time = message["time"];
            json forward_msg = {
                {"type", "ONE_CHAT_MSG"},
                {"fromid", userId},
                {"toid", toId},
                {"msg", msg},
                {"time", time}
            };
            sendMessageToUser(toId, forward_msg);
        } else if (type == "GROUP_CHAT_MSG") {
            int groupId = message["groupid"];
            std::string msg = message["msg"];
            std::string time = message["time"];
            std::vector<GroupUser> groupUsers = ChatService::instance()->getGroupModel().queryGroupUsers(groupId);
            for (const auto& groupUser : groupUsers) {
                if (groupUser.getId() != userId) {
                    json forward_msg = {
                        {"type", "GROUP_CHAT_MSG"},
                        {"groupid", groupId},
                        {"fromid", userId},
                        {"msg", msg},
                        {"time", time}
                    };
                    sendMessageToUser(groupUser.getId(), forward_msg);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cout << "處理WebSocket消息失敗: " << e.what() << std::endl;
    }
}

void WebController::handleWebSocketClose(crow::websocket::connection& conn)
{
    // 從連接映射中找到並移除用戶
    int userId = -1;
    {
        std::lock_guard<std::mutex> lock(_wsMutex);
        for (auto it = _userWebSocketMap.begin(); it != _userWebSocketMap.end(); ++it) {
            if (it->second == &conn) {
                userId = it->first;
                _userWebSocketMap.erase(it);
                break;
            }
        }
    }
    
    if (userId != -1) {
        // 更新用戶狀態
        User user = ChatService::instance()->getUserModel().query(userId);
        if (user.getId() != -1) {
            user.setState("offline");
            ChatService::instance()->getUserModel().updateState(user);
            std::cout << "用戶 " << user.getName() << " WebSocket連接關閉" << std::endl;
        }
    }
}

void WebController::sendMessageToUser(int userId, const json& message)
{
    std::lock_guard<std::mutex> lock(_wsMutex);
    auto it = _userWebSocketMap.find(userId);
    if (it != _userWebSocketMap.end()) {
        try {
            it->second->send_text(message.dump());
        } catch (const std::exception& e) {
            std::cout << "發送消息給用戶 " << userId << " 失敗: " << e.what() << std::endl;
        }
    }
}

std::string WebController::generateToken(int userId)
{
    // 簡單的JWT實現（實際生產環境應使用更安全的庫）
    std::string header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    std::string payload = "{\"userId\":" + std::to_string(userId) + ",\"exp\":" + std::to_string(time(nullptr) + 86400) + "}";
    
    // Base64編碼
    auto encode = [](const std::string& input) -> std::string {
        BIO* bio = BIO_new(BIO_s_mem());
        BIO* b64 = BIO_new(BIO_f_base64());
        bio = BIO_push(b64, bio);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, input.c_str(), input.length());
        BIO_flush(bio);
        
        BUF_MEM* bufferPtr;
        BIO_get_mem_ptr(bio, &bufferPtr);
        
        std::string result(bufferPtr->data, bufferPtr->length);
        BIO_free_all(bio);
        
        return result;
    };
    
    std::string encodedHeader = encode(header);
    std::string encodedPayload = encode(payload);
    
    // 生成簽名
    std::string data = encodedHeader + "." + encodedPayload;
    unsigned char hash[32];
    unsigned int hashLen;
    HMAC(EVP_sha256(), JWT_SECRET.c_str(), JWT_SECRET.length(), 
         (unsigned char*)data.c_str(), data.length(), hash, &hashLen);
    
    std::string signature(reinterpret_cast<char*>(hash), hashLen);
    std::string encodedSignature = encode(signature);
    
    return data + "." + encodedSignature;
}

bool WebController::verifyToken(const std::string& token, int& userId)
{
    try {
        size_t pos1 = token.find('.');
        size_t pos2 = token.find('.', pos1 + 1);
        
        if (pos1 == std::string::npos || pos2 == std::string::npos) {
            return false;
        }
        
        std::string encodedHeader = token.substr(0, pos1);
        std::string encodedPayload = token.substr(pos1 + 1, pos2 - pos1 - 1);
        std::string encodedSignature = token.substr(pos2 + 1);
        
        // 簡單驗證（實際生產環境應進行完整驗證）
        // 這裡只是解析payload獲取userId
        auto decode = [](const std::string& input) -> std::string {
            BIO* bio = BIO_new(BIO_s_mem());
            BIO* b64 = BIO_new(BIO_f_base64());
            bio = BIO_push(b64, bio);
            BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
            BIO_write(bio, input.c_str(), input.length());
            BIO_flush(bio);
            
            BUF_MEM* bufferPtr;
            BIO_get_mem_ptr(bio, &bufferPtr);
            
            std::string result(bufferPtr->data, bufferPtr->length);
            BIO_free_all(bio);
            
            return result;
        };
        
        std::string payload = decode(encodedPayload);
        json payload_json = json::parse(payload);
        
        // 檢查過期時間
        if (payload_json["exp"].get<int64_t>() < time(nullptr)) {
            return false;
        }
        
        userId = payload_json["userId"].get<int>();
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "Token驗證失敗: " << e.what() << std::endl;
        return false;
    }
} 