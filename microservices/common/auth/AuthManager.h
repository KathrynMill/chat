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

// 用户會话信息
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

// 认证結果
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

// 认证管理器
class AuthManager {
public:
    static AuthManager& getInstance();
    
    // 初始化认证管理器
    bool initialize(const std::string& jwtSecret, 
                   int tokenExpirationMinutes = 60,
                   int sessionTimeoutMinutes = 30);
    
    // 用户登入认证
    AuthResult authenticate(const std::string& username, const std::string& password);
    
    // Token 验证
    AuthResult validateToken(const std::string& token);
    
    // 刷新 Token
    std::string refreshToken(const std::string& oldToken);
    
    // 登出
    bool logout(const std::string& token);
    
    // 检查权限
    bool hasPermission(const std::string& token, const std::string& permission);
    
    // 获取用户會话
    UserSession* getSession(const std::string& token);
    
    // 创建 JWT Token
    std::string createToken(const std::string& userId, 
                           const std::string& username,
                           const std::vector<std::string>& permissions = {},
                           const std::unordered_map<std::string, std::string>& metadata = {});
    
    // 從 HTTP 頭部提取 Token
    std::string extractTokenFromHeaders(const std::unordered_map<std::string, std::string>& headers);
    
    // 從 gRPC metadata 提取 Token
    std::string extractTokenFromGrpcMetadata(const std::multimap<std::string, std::string>& metadata);
    
    // 清理过期會话
    void cleanupExpiredSessions();
    
    // 获取會话统計
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
    
    // 验证用户憑证（這裡應該调用 UserService）
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
        long exp; // 过期時間
    };
    JwtPayload parseJwtPayload(const std::string& token);
    
#ifdef HAVE_OPENSSL
    std::unique_ptr<JwtValidator> jwtValidator_;
#endif
    
    // 會话管理
    std::unordered_map<std::string, std::unique_ptr<UserSession>> sessions_;
    std::mutex sessionsMutex_;
    
    // 配置
    std::string jwtSecret_;
    int tokenExpirationMinutes_;
    int sessionTimeoutMinutes_;
    
    // 统計信息
    std::atomic<int> totalSessions_;
    std::atomic<int> activeSessions_;
    std::chrono::system_clock::time_point lastCleanupTime_;
};
