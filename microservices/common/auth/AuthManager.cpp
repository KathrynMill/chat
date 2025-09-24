#include "AuthManager.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

#ifdef HAVE_OPENSSL
#include "jwt/JwtValidator.h"
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#endif

AuthManager& AuthManager::getInstance() {
    static AuthManager instance;
    return instance;
}

bool AuthManager::initialize(const std::string& jwtSecret, 
                           int tokenExpirationMinutes,
                           int sessionTimeoutMinutes) {
    jwtSecret_ = jwtSecret;
    tokenExpirationMinutes_ = tokenExpirationMinutes;
    sessionTimeoutMinutes_ = sessionTimeoutMinutes;
    totalSessions_ = 0;
    activeSessions_ = 0;
    lastCleanupTime_ = std::chrono::system_clock::now();
    
#ifdef HAVE_OPENSSL
    try {
        jwtValidator_ = std::make_unique<JwtValidator>(jwtSecret);
        std::cout << "AuthManager initialized with JWT secret\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize JWT validator: " << e.what() << "\n";
        return false;
    }
#else
    std::cout << "AuthManager initialized without JWT (OpenSSL not available)\n";
    return true;
#endif
}

AuthResult AuthManager::authenticate(const std::string& username, const std::string& password) {
    AuthResult result;
    
    // 驗證用戶憑證
    std::string userId;
    std::vector<std::string> permissions;
    
    if (!validateCredentials(username, password, userId, permissions)) {
        result.success = false;
        result.errorMessage = "Invalid username or password";
        return result;
    }
    
    // 創建會話
    std::string token = createToken(userId, username, permissions);
    
    // 存儲會話
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        auto session = std::make_unique<UserSession>();
        session->userId = userId;
        session->username = username;
        session->token = token;
        session->createdAt = std::chrono::system_clock::now();
        session->expiresAt = session->createdAt + std::chrono::minutes(sessionTimeoutMinutes_);
        session->permissions = permissions;
        
        sessions_[token] = std::move(session);
        totalSessions_++;
        activeSessions_++;
    }
    
    result.success = true;
    result.userId = userId;
    result.username = username;
    result.permissions = permissions;
    
    return result;
}

AuthResult AuthManager::validateToken(const std::string& token) {
    AuthResult result;
    
    if (token.empty()) {
        result.success = false;
        result.errorMessage = "Token is empty";
        return result;
    }
    
    // 檢查會話緩存
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        auto it = sessions_.find(token);
        if (it != sessions_.end()) {
            if (it->second->isExpired()) {
                // 會話過期，清理
                sessions_.erase(it);
                activeSessions_--;
                result.success = false;
                result.errorMessage = "Token expired";
                return result;
            }
            
            result.success = true;
            result.userId = it->second->userId;
            result.username = it->second->username;
            result.permissions = it->second->permissions;
            result.metadata = it->second->metadata;
            return result;
        }
    }
    
    // 如果會話緩存中沒有，嘗試驗證 JWT
#ifdef HAVE_OPENSSL
    if (jwtValidator_) {
        try {
            auto payload = parseJwtPayload(token);
            
            // 檢查過期時間
            auto now = std::chrono::system_clock::now();
            auto expTime = std::chrono::system_clock::from_time_t(payload.exp);
            
            if (now > expTime) {
                result.success = false;
                result.errorMessage = "Token expired";
                return result;
            }
            
            result.success = true;
            result.userId = payload.userId;
            result.username = payload.username;
            result.permissions = payload.permissions;
            result.metadata = payload.metadata;
            
            return result;
        } catch (const std::exception& e) {
            result.success = false;
            result.errorMessage = "Invalid token: " + std::string(e.what());
            return result;
        }
    }
#endif
    
    result.success = false;
    result.errorMessage = "Token validation failed";
    return result;
}

std::string AuthManager::refreshToken(const std::string& oldToken) {
    auto authResult = validateToken(oldToken);
    if (!authResult.success) {
        return "";
    }
    
    // 創建新 token
    std::string newToken = createToken(authResult.userId, authResult.username, 
                                     authResult.permissions, authResult.metadata);
    
    // 更新會話
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        auto it = sessions_.find(oldToken);
        if (it != sessions_.end()) {
            // 更新現有會話
            it->second->token = newToken;
            it->second->expiresAt = std::chrono::system_clock::now() + 
                                   std::chrono::minutes(sessionTimeoutMinutes_);
            
            // 移動到新 token
            sessions_[newToken] = std::move(it->second);
            sessions_.erase(it);
        }
    }
    
    return newToken;
}

bool AuthManager::logout(const std::string& token) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    
    auto it = sessions_.find(token);
    if (it != sessions_.end()) {
        sessions_.erase(it);
        activeSessions_--;
        return true;
    }
    
    return false;
}

bool AuthManager::hasPermission(const std::string& token, const std::string& permission) {
    auto session = getSession(token);
    if (!session) {
        return false;
    }
    
    return session->hasPermission(permission);
}

UserSession* AuthManager::getSession(const std::string& token) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    
    auto it = sessions_.find(token);
    if (it != sessions_.end() && !it->second->isExpired()) {
        return it->second.get();
    }
    
    return nullptr;
}

std::string AuthManager::createToken(const std::string& userId, 
                                   const std::string& username,
                                   const std::vector<std::string>& permissions,
                                   const std::unordered_map<std::string, std::string>& metadata) {
#ifdef HAVE_OPENSSL
    if (jwtValidator_) {
        try {
            // 創建 JWT payload
            auto now = std::chrono::system_clock::now();
            auto exp = now + std::chrono::minutes(tokenExpirationMinutes_);
            
            // 簡化版 JWT 創建（實際應該使用完整的 JWT 庫）
            std::ostringstream payload;
            payload << "{\"sub\":\"" << userId << "\","
                    << "\"username\":\"" << username << "\","
                    << "\"exp\":" << std::chrono::duration_cast<std::chrono::seconds>(exp.time_since_epoch()).count()
                    << "}";
            
            // 這裡應該使用 JWT 庫來創建和簽名 token
            // 簡化實現：返回一個偽造的 token
            return "jwt." + generateRandomToken() + "." + generateRandomToken();
        } catch (const std::exception& e) {
            std::cerr << "Failed to create JWT token: " << e.what() << "\n";
            return "";
        }
    }
#endif
    
    // 如果沒有 JWT 支持，返回簡單的隨機 token
    return generateRandomToken();
}

std::string AuthManager::extractTokenFromHeaders(const std::unordered_map<std::string, std::string>& headers) {
    // 檢查 Authorization 頭
    auto it = headers.find("Authorization");
    if (it != headers.end()) {
        const std::string& auth = it->second;
        if (auth.substr(0, 7) == "Bearer ") {
            return auth.substr(7);
        }
    }
    
    // 檢查 X-Auth-Token 頭
    it = headers.find("X-Auth-Token");
    if (it != headers.end()) {
        return it->second;
    }
    
    return "";
}

std::string AuthManager::extractTokenFromGrpcMetadata(const std::multimap<std::string, std::string>& metadata) {
    // 檢查 authorization metadata
    auto it = metadata.find("authorization");
    if (it != metadata.end()) {
        const std::string& auth = it->second;
        if (auth.substr(0, 7) == "Bearer ") {
            return auth.substr(7);
        }
    }
    
    // 檢查 x-auth-token metadata
    it = metadata.find("x-auth-token");
    if (it != metadata.end()) {
        return it->second;
    }
    
    return "";
}

void AuthManager::cleanupExpiredSessions() {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    
    auto now = std::chrono::system_clock::now();
    int removedCount = 0;
    
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (it->second->isExpired()) {
            it = sessions_.erase(it);
            removedCount++;
            activeSessions_--;
        } else {
            ++it;
        }
    }
    
    lastCleanupTime_ = now;
    
    if (removedCount > 0) {
        std::cout << "Cleaned up " << removedCount << " expired sessions\n";
    }
}

AuthManager::SessionStats AuthManager::getSessionStats() {
    SessionStats stats;
    
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    stats.totalSessions = totalSessions_.load();
    stats.activeSessions = activeSessions_.load();
    stats.expiredSessions = stats.totalSessions - stats.activeSessions;
    stats.lastCleanupTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        lastCleanupTime_.time_since_epoch()).count();
    
    return stats;
}

bool AuthManager::validateCredentials(const std::string& username, const std::string& password, 
                                    std::string& userId, std::vector<std::string>& permissions) {
    // 這裡應該調用 UserService 來驗證憑證
    // 簡化實現：假設驗證成功
    
    if (username.empty() || password.empty()) {
        return false;
    }
    
    // 模擬用戶驗證
    userId = "user_" + username;
    permissions = {"read", "write", "chat"}; // 默認權限
    
    return true;
}

std::string AuthManager::generateRandomToken() {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static const int charsetSize = sizeof(charset) - 1;
    
    std::string token;
    token.reserve(32);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, charsetSize - 1);
    
    for (int i = 0; i < 32; ++i) {
        token += charset[dis(gen)];
    }
    
    return token;
}

AuthManager::JwtPayload AuthManager::parseJwtPayload(const std::string& token) {
    JwtPayload payload;
    
    // 簡化實現：解析 JWT token
    // 實際實現中應該使用完整的 JWT 庫
    
    // 這裡只是示例，實際應該解析 JWT 的 payload 部分
    payload.userId = "parsed_user_id";
    payload.username = "parsed_username";
    payload.exp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() + 3600;
    
    return payload;
}
