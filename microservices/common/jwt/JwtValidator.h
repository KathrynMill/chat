#pragma once
#include <string>
#include <map>
#include <chrono>

struct JwtPayload {
    std::string sub;        // subject (user_id)
    std::string iss;        // issuer
    std::string aud;        // audience
    std::chrono::system_clock::time_point exp;  // expiration
    std::chrono::system_clock::time_point iat;  // issued at
    std::map<std::string, std::string> claims;
};

class JwtValidator {
public:
    JwtValidator(const std::string& secretKey);
    
    // 驗證 JWT token
    bool validateToken(const std::string& token, JwtPayload& payload);
    
    // 生成 JWT token（用於測試）
    std::string generateToken(const std::string& userId, 
                             const std::string& issuer = "chat-service",
                             int expirationSeconds = 3600);
    
    // 檢查 token 是否過期
    bool isTokenExpired(const JwtPayload& payload);
    
    // 從 token 中提取用戶 ID
    std::string extractUserId(const std::string& token);

private:
    std::string secretKey_;
    
    // Base64 編解碼
    std::string base64Encode(const std::string& input);
    std::string base64Decode(const std::string& input);
    
    // HMAC-SHA256 簽名
    std::string hmacSha256(const std::string& data, const std::string& key);
    
    // JSON 解析（簡化版）
    std::map<std::string, std::string> parseJson(const std::string& json);
    std::string toJson(const std::map<std::string, std::string>& data);
    
    // 時間戳轉換
    std::chrono::system_clock::time_point timestampToTimePoint(int64_t timestamp);
    int64_t timePointToTimestamp(const std::chrono::system_clock::time_point& tp);
};
