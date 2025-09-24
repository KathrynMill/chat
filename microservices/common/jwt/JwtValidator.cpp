#include "JwtValidator.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>

#ifdef HAVE_OPENSSL
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#endif

JwtValidator::JwtValidator(const std::string& secretKey) : secretKey_(secretKey) {
}

bool JwtValidator::validateToken(const std::string& token, JwtPayload& payload) {
    // JWT 格式：header.payload.signature
    size_t firstDot = token.find('.');
    size_t secondDot = token.find('.', firstDot + 1);
    
    if (firstDot == std::string::npos || secondDot == std::string::npos) {
        return false;
    }
    
    std::string header = token.substr(0, firstDot);
    std::string payloadStr = token.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string signature = token.substr(secondDot + 1);
    
    // 驗證簽名
    std::string expectedSignature = hmacSha256(header + "." + payloadStr, secretKey_);
    std::string expectedSignatureB64 = base64Encode(expectedSignature);
    
    if (signature != expectedSignatureB64) {
        return false;
    }
    
    // 解析 payload
    std::string decodedPayload = base64Decode(payloadStr);
    auto claims = parseJson(decodedPayload);
    
    // 提取標準字段
    payload.sub = claims["sub"];
    payload.iss = claims["iss"];
    payload.aud = claims["aud"];
    
    if (claims.find("exp") != claims.end()) {
        payload.exp = timestampToTimePoint(std::stoll(claims["exp"]));
    }
    if (claims.find("iat") != claims.end()) {
        payload.iat = timestampToTimePoint(std::stoll(claims["iat"]));
    }
    
    payload.claims = claims;
    
    // 檢查過期時間
    return !isTokenExpired(payload);
}

std::string JwtValidator::generateToken(const std::string& userId, 
                                       const std::string& issuer,
                                       int expirationSeconds) {
    // Header
    std::map<std::string, std::string> header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";
    std::string headerJson = toJson(header);
    std::string headerB64 = base64Encode(headerJson);
    
    // Payload
    std::map<std::string, std::string> payload;
    payload["sub"] = userId;
    payload["iss"] = issuer;
    payload["aud"] = "chat-service";
    
    auto now = std::chrono::system_clock::now();
    payload["iat"] = std::to_string(timePointToTimestamp(now));
    payload["exp"] = std::to_string(timePointToTimestamp(now + std::chrono::seconds(expirationSeconds)));
    
    std::string payloadJson = toJson(payload);
    std::string payloadB64 = base64Encode(payloadJson);
    
    // Signature
    std::string signature = hmacSha256(headerB64 + "." + payloadB64, secretKey_);
    std::string signatureB64 = base64Encode(signature);
    
    return headerB64 + "." + payloadB64 + "." + signatureB64;
}

bool JwtValidator::isTokenExpired(const JwtPayload& payload) {
    auto now = std::chrono::system_clock::now();
    return now > payload.exp;
}

std::string JwtValidator::extractUserId(const std::string& token) {
    size_t firstDot = token.find('.');
    size_t secondDot = token.find('.', firstDot + 1);
    
    if (firstDot == std::string::npos || secondDot == std::string::npos) {
        return "";
    }
    
    std::string payloadStr = token.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string decodedPayload = base64Decode(payloadStr);
    auto claims = parseJson(decodedPayload);
    
    return claims["sub"];
}

std::string JwtValidator::base64Encode(const std::string& input) {
#ifdef HAVE_OPENSSL
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    
    BIO_write(bio, input.c_str(), input.length());
    BIO_flush(bio);
    
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    
    return result;
#else
    // 簡化版 Base64 編碼
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0, valb = -6;
    
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (result.size() % 4) {
        result.push_back('=');
    }
    
    return result;
#endif
}

std::string JwtValidator::base64Decode(const std::string& input) {
#ifdef HAVE_OPENSSL
    BIO* bio = BIO_new_mem_buf(input.c_str(), input.length());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    
    char* buffer = new char[input.length()];
    int decodedLength = BIO_read(bio, buffer, input.length());
    BIO_free_all(bio);
    
    std::string result(buffer, decodedLength);
    delete[] buffer;
    
    return result;
#else
    // 簡化版 Base64 解碼
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0, valb = -8;
    
    for (char c : input) {
        if (c == '=') break;
        size_t pos = chars.find(c);
        if (pos == std::string::npos) continue;
        
        val = (val << 6) + pos;
        valb += 6;
        if (valb >= 0) {
            result.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return result;
#endif
}

std::string JwtValidator::hmacSha256(const std::string& data, const std::string& key) {
#ifdef HAVE_OPENSSL
    unsigned char* result;
    unsigned int len = 32;
    
    result = HMAC(EVP_sha256(), key.c_str(), key.length(),
                  (unsigned char*)data.c_str(), data.length(), NULL, &len);
    
    return std::string((char*)result, len);
#else
    // 簡化版 HMAC-SHA256（僅用於測試）
    std::cerr << "JwtValidator: OpenSSL not available, using fallback\n";
    return "fallback_signature_" + data + "_" + key;
#endif
}

std::map<std::string, std::string> JwtValidator::parseJson(const std::string& json) {
    std::map<std::string, std::string> result;
    
    // 簡化版 JSON 解析
    size_t pos = 0;
    while (pos < json.length()) {
        // 跳過空白字符
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
            pos++;
        }
        
        if (pos >= json.length() || json[pos] != '"') break;
        
        // 提取 key
        size_t keyStart = pos + 1;
        size_t keyEnd = json.find('"', keyStart);
        if (keyEnd == std::string::npos) break;
        
        std::string key = json.substr(keyStart, keyEnd - keyStart);
        pos = keyEnd + 1;
        
        // 跳過冒號
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == ':')) {
            pos++;
        }
        
        // 提取 value
        if (pos >= json.length()) break;
        
        std::string value;
        if (json[pos] == '"') {
            // 字符串值
            size_t valueStart = pos + 1;
            size_t valueEnd = json.find('"', valueStart);
            if (valueEnd == std::string::npos) break;
            value = json.substr(valueStart, valueEnd - valueStart);
            pos = valueEnd + 1;
        } else {
            // 數字值
            size_t valueStart = pos;
            while (pos < json.length() && json[pos] != ',' && json[pos] != '}') {
                pos++;
            }
            value = json.substr(valueStart, pos - valueStart);
        }
        
        result[key] = value;
        
        // 跳過逗號
        while (pos < json.length() && (json[pos] == ',' || json[pos] == ' ')) {
            pos++;
        }
    }
    
    return result;
}

std::string JwtValidator::toJson(const std::map<std::string, std::string>& data) {
    std::ostringstream oss;
    oss << "{";
    
    bool first = true;
    for (const auto& pair : data) {
        if (!first) oss << ",";
        oss << "\"" << pair.first << "\":\"" << pair.second << "\"";
        first = false;
    }
    
    oss << "}";
    return oss.str();
}

std::chrono::system_clock::time_point JwtValidator::timestampToTimePoint(int64_t timestamp) {
    return std::chrono::system_clock::from_time_t(timestamp);
}

int64_t JwtValidator::timePointToTimestamp(const std::chrono::system_clock::time_point& tp) {
    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
}
