#pragma once
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>

#ifdef HAVE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#endif

// TLS 配置
struct TlsConfig {
    std::string certFile;           // 证書文件路径
    std::string keyFile;            // 私鑰文件路径
    std::string caFile;             // CA 证書文件路径
    std::string cipherSuites;       // 加密套件
    bool verifyPeer = true;         // 是否验证對端证書
    bool verifyHostname = true;     // 是否验证主機名
    int minVersion = TLS1_2_VERSION; // 最小 TLS 版本
    int maxVersion = TLS1_3_VERSION; // 最大 TLS 版本
    bool enableSessionResumption = true; // 启用會话恢复
    int sessionTimeout = 300;       // 會话超時時間（秒）
};

// 证書信息
struct CertificateInfo {
    std::string subject;
    std::string issuer;
    std::string serialNumber;
    std::string notBefore;
    std::string notAfter;
    std::vector<std::string> san;   // Subject Alternative Names
    bool isValid;
    bool isExpired;
    int daysUntilExpiry;
};

// TLS 管理器
class TlsManager {
public:
    static TlsManager& getInstance();
    
    // 初始化 TLS 管理器
    bool initialize();
    
    // 创建 SSL 上下文
    bool createSslContext(const TlsConfig& config);
    
    // 创建 SSL 连接
    std::shared_ptr<void> createSslConnection(int socketFd, bool isServer = true);
    
    // 执行 SSL 握手
    bool performHandshake(std::shared_ptr<void> ssl);
    
    // SSL 读取
    int sslRead(std::shared_ptr<void> ssl, void* buffer, int length);
    
    // SSL 写入
    int sslWrite(std::shared_ptr<void> ssl, const void* buffer, int length);
    
    // 关闭 SSL 连接
    void closeSslConnection(std::shared_ptr<void> ssl);
    
    // 加载证書
    bool loadCertificate(const std::string& certFile, const std::string& keyFile);
    
    // 加载 CA 证書
    bool loadCaCertificate(const std::string& caFile);
    
    // 验证证書
    bool verifyCertificate(std::shared_ptr<void> ssl, const std::string& hostname = "");
    
    // 获取证書信息
    CertificateInfo getCertificateInfo(const std::string& certFile);
    
    // 生成自簽名证書
    bool generateSelfSignedCertificate(const std::string& certFile, 
                                      const std::string& keyFile,
                                      const std::string& commonName,
                                      int validityDays = 365);
    
    // 检查证書是否即將过期
    bool isCertificateExpiringSoon(const std::string& certFile, int daysThreshold = 30);
    
    // 获取 SSL 错误信息
    std::string getSslError();
    
    // 清理资源
    void cleanup();
    
    // 获取 TLS 统計
    struct TlsStats {
        int totalConnections;
        int activeConnections;
        int handshakeFailures;
        int certificateErrors;
        std::string currentCipher;
        std::string currentProtocol;
    };
    TlsStats getTlsStats();

private:
    TlsManager() = default;
    ~TlsManager() = default;
    TlsManager(const TlsManager&) = delete;
    TlsManager& operator=(const TlsManager&) = delete;
    
    // 初始化 OpenSSL
    bool initializeOpenSSL();
    
    // 创建 SSL 上下文
    std::shared_ptr<void> createSslContextInternal(const TlsConfig& config);
    
    // 设置 SSL 選项
    void setSslOptions(std::shared_ptr<void> sslCtx, const TlsConfig& config);
    
    // 加载证書到 SSL 上下文
    bool loadCertificateToContext(std::shared_ptr<void> sslCtx, 
                                 const std::string& certFile, 
                                 const std::string& keyFile);
    
    // 加载 CA 证書到 SSL 上下文
    bool loadCaCertificateToContext(std::shared_ptr<void> sslCtx, const std::string& caFile);
    
    // 解析证書
    CertificateInfo parseCertificate(const std::string& certFile);
    
    // 生成 RSA 密鑰對
    bool generateRsaKeyPair(const std::string& keyFile, int keySize = 2048);
    
    // 创建证書請求
    bool createCertificateRequest(const std::string& csrFile, 
                                 const std::string& keyFile,
                                 const std::string& commonName);
    
#ifdef HAVE_OPENSSL
    SSL_CTX* sslCtx_;
    std::mutex sslCtxMutex_;
#endif
    
    // 统計信息
    std::atomic<int> totalConnections_;
    std::atomic<int> activeConnections_;
    std::atomic<int> handshakeFailures_;
    std::atomic<int> certificateErrors_;
    
    // 配置
    TlsConfig currentConfig_;
    bool initialized_;
};
