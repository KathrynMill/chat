#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <chrono>
#include <vector>

#ifdef HAVE_OPENSSL
#include "jwt/JwtValidator.h"
#endif

// 用戶會話信息
struct UserSession {
    std::string userId;
    std::string username;
    std::string token;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point expiresAt;
    std::vector<std::string> permissions;
    std::unordered_map<std::string, std::string> metadata;
    
    bool isExpired() const {
        return std::chrono::system_clock::now() > expiresAt;
    }
    
    bool hasPermission(const std::string& permission) const {
        return std::find(permissions.begin(), permissions.end(), permission) != permissions.end();
    }
};

// 認證結果
struct AuthResult {
    bool success;
    std::string userId;
    std::string username;
    std::string errorMessage;
    std::vector<std::string> permissions;
    std::unordered_map<std::string, std::string> metadata;
    
    AuthResult() : success(false) {}
    AuthResult(bool s, const std::string& uid, const std::string& uname) 
        : success(s), userId(uid), username(uname) {}
};

// 認證管理器
class AuthManager {
public:
    static AuthManager& getInstance();
    
    // 初始化認證管理器
    bool initialize(const std::string& jwtSecret, 
                   int tokenExpirationMinutes = 60,
                   int sessionTimeoutMinutes = 30);
    
    // 用戶登入認證
    AuthResult authenticate(const std::string& username, const std::string& password);
    
    // Token 驗證
    AuthResult validateToken(const std::string& token);
    
    // 刷新 Token
    std::string refreshToken(const std::string& oldToken);
    
    // 登出
    bool logout(const std::string& token);
    
    // 檢查權限
    bool hasPermission(const std::string& token, const std::string& permission);
    
    // 獲取用戶會話
    UserSession* getSession(const std::string& token);
    
    // 創建 JWT Token
    std::string createToken(const std::string& userId, 
                           const std::string& username,
                           const std::vector<std::string>& permissions = {},
                           const std::unordered_map<std::string, std::string>& metadata = {});
    
    // 從 HTTP 頭部提取 Token
    std::string extractTokenFromHeaders(const std::unordered_map<std::string, std::string>& headers);
    
    // 從 gRPC metadata 提取 Token
    std::string extractTokenFromGrpcMetadata(const std::multimap<std::string, std::string>& metadata);
    
    // 清理過期會話
    void cleanupExpiredSessions();
    
    // 獲取會話統計
    struct SessionStats {
        int totalSessions;
        int activeSessions;
        int expiredSessions;
        long lastCleanupTime;
    };
    SessionStats getSessionStats();

private:
    AuthManager() = default;
    ~AuthManager() = default;
    AuthManager(const AuthManager&) = delete;
    AuthManager& operator=(const AuthManager&) = delete;
    
    // 驗證用戶憑證（這裡應該調用 UserService）
    bool validateCredentials(const std::string& username, const std::string& password, 
                           std::string& userId, std::vector<std::string>& permissions);
    
    // 生成隨機 Token
    std::string generateRandomToken();
    
    // 解析 JWT Token
    struct JwtPayload {
        std::string userId;
        std::string username;
        std::vector<std::string> permissions;
        std::unordered_map<std::string, std::string> metadata;
        long exp; // 過期時間
    };
    JwtPayload parseJwtPayload(const std::string& token);
    
#ifdef HAVE_OPENSSL
    std::unique_ptr<JwtValidator> jwtValidator_;
#endif
    
    // 會話管理
    std::unordered_map<std::string, std::unique_ptr<UserSession>> sessions_;
    std::mutex sessionsMutex_;
    
    // 配置
    std::string jwtSecret_;
    int tokenExpirationMinutes_;
    int sessionTimeoutMinutes_;
    
    // 統計信息
    std::atomic<int> totalSessions_;
    std::atomic<int> activeSessions_;
    std::chrono::system_clock::time_point lastCleanupTime_;
};
