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
    std::string certFile;           // 證書文件路徑
    std::string keyFile;            // 私鑰文件路徑
    std::string caFile;             // CA 證書文件路徑
    std::string cipherSuites;       // 加密套件
    bool verifyPeer = true;         // 是否驗證對端證書
    bool verifyHostname = true;     // 是否驗證主機名
    int minVersion = TLS1_2_VERSION; // 最小 TLS 版本
    int maxVersion = TLS1_3_VERSION; // 最大 TLS 版本
    bool enableSessionResumption = true; // 啟用會話恢復
    int sessionTimeout = 300;       // 會話超時時間（秒）
};

// 證書信息
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
    
    // 創建 SSL 上下文
    bool createSslContext(const TlsConfig& config);
    
    // 創建 SSL 連接
    std::shared_ptr<void> createSslConnection(int socketFd, bool isServer = true);
    
    // 執行 SSL 握手
    bool performHandshake(std::shared_ptr<void> ssl);
    
    // SSL 讀取
    int sslRead(std::shared_ptr<void> ssl, void* buffer, int length);
    
    // SSL 寫入
    int sslWrite(std::shared_ptr<void> ssl, const void* buffer, int length);
    
    // 關閉 SSL 連接
    void closeSslConnection(std::shared_ptr<void> ssl);
    
    // 加載證書
    bool loadCertificate(const std::string& certFile, const std::string& keyFile);
    
    // 加載 CA 證書
    bool loadCaCertificate(const std::string& caFile);
    
    // 驗證證書
    bool verifyCertificate(std::shared_ptr<void> ssl, const std::string& hostname = "");
    
    // 獲取證書信息
    CertificateInfo getCertificateInfo(const std::string& certFile);
    
    // 生成自簽名證書
    bool generateSelfSignedCertificate(const std::string& certFile, 
                                      const std::string& keyFile,
                                      const std::string& commonName,
                                      int validityDays = 365);
    
    // 檢查證書是否即將過期
    bool isCertificateExpiringSoon(const std::string& certFile, int daysThreshold = 30);
    
    // 獲取 SSL 錯誤信息
    std::string getSslError();
    
    // 清理資源
    void cleanup();
    
    // 獲取 TLS 統計
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
    
    // 創建 SSL 上下文
    std::shared_ptr<void> createSslContextInternal(const TlsConfig& config);
    
    // 設置 SSL 選項
    void setSslOptions(std::shared_ptr<void> sslCtx, const TlsConfig& config);
    
    // 加載證書到 SSL 上下文
    bool loadCertificateToContext(std::shared_ptr<void> sslCtx, 
                                 const std::string& certFile, 
                                 const std::string& keyFile);
    
    // 加載 CA 證書到 SSL 上下文
    bool loadCaCertificateToContext(std::shared_ptr<void> sslCtx, const std::string& caFile);
    
    // 解析證書
    CertificateInfo parseCertificate(const std::string& certFile);
    
    // 生成 RSA 密鑰對
    bool generateRsaKeyPair(const std::string& keyFile, int keySize = 2048);
    
    // 創建證書請求
    bool createCertificateRequest(const std::string& csrFile, 
                                 const std::string& keyFile,
                                 const std::string& commonName);
    
#ifdef HAVE_OPENSSL
    SSL_CTX* sslCtx_;
    std::mutex sslCtxMutex_;
#endif
    
    // 統計信息
    std::atomic<int> totalConnections_;
    std::atomic<int> activeConnections_;
    std::atomic<int> handshakeFailures_;
    std::atomic<int> certificateErrors_;
    
    // 配置
    TlsConfig currentConfig_;
    bool initialized_;
};
