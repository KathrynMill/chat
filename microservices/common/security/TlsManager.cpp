#include "TlsManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

#ifdef HAVE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bn.h>
#endif

TlsManager& TlsManager::getInstance() {
    static TlsManager instance;
    return instance;
}

bool TlsManager::initialize() {
#ifdef HAVE_OPENSSL
    if (initialized_) {
        return true;
    }
    
    // 初始化 OpenSSL
    if (!initializeOpenSSL()) {
        return false;
    }
    
    sslCtx_ = nullptr;
    totalConnections_ = 0;
    activeConnections_ = 0;
    handshakeFailures_ = 0;
    certificateErrors_ = 0;
    initialized_ = true;
    
    std::cout << "TlsManager initialized with OpenSSL\n";
    return true;
#else
    std::cout << "TlsManager initialized without OpenSSL support\n";
    initialized_ = true;
    return true;
#endif
}

bool TlsManager::createSslContext(const TlsConfig& config) {
#ifdef HAVE_OPENSSL
    if (!initialized_) {
        std::cerr << "TlsManager not initialized\n";
        return false;
    }
    
    std::lock_guard<std::mutex> lock(sslCtxMutex_);
    
    // 创建 SSL 上下文
    sslCtx_ = SSL_CTX_new(TLS_server_method());
    if (!sslCtx_) {
        std::cerr << "Failed to create SSL context: " << getSslError() << "\n";
        return false;
    }
    
    // 设置 SSL 選项
    setSslOptions(sslCtx_, config);
    
    // 加载证書
    if (!config.certFile.empty() && !config.keyFile.empty()) {
        if (!loadCertificateToContext(sslCtx_, config.certFile, config.keyFile)) {
            SSL_CTX_free(sslCtx_);
            sslCtx_ = nullptr;
            return false;
        }
    }
    
    // 加载 CA 证書
    if (!config.caFile.empty()) {
        if (!loadCaCertificateToContext(sslCtx_, config.caFile)) {
            SSL_CTX_free(sslCtx_);
            sslCtx_ = nullptr;
            return false;
        }
    }
    
    currentConfig_ = config;
    std::cout << "SSL context created successfully\n";
    return true;
#else
    std::cout << "SSL context creation skipped (OpenSSL not available)\n";
    return true;
#endif
}

std::shared_ptr<void> TlsManager::createSslConnection(int socketFd, bool isServer) {
#ifdef HAVE_OPENSSL
    if (!sslCtx_) {
        std::cerr << "SSL context not created\n";
        return nullptr;
    }
    
    SSL* ssl = SSL_new(sslCtx_);
    if (!ssl) {
        std::cerr << "Failed to create SSL connection: " << getSslError() << "\n";
        return nullptr;
    }
    
    if (SSL_set_fd(ssl, socketFd) != 1) {
        std::cerr << "Failed to set SSL file descriptor: " << getSslError() << "\n";
        SSL_free(ssl);
        return nullptr;
    }
    
    totalConnections_++;
    activeConnections_++;
    
    return std::shared_ptr<void>(ssl, [this](void* ptr) {
        SSL_free(static_cast<SSL*>(ptr));
        activeConnections_--;
    });
#else
    return nullptr;
#endif
}

bool TlsManager::performHandshake(std::shared_ptr<void> ssl) {
#ifdef HAVE_OPENSSL
    if (!ssl) {
        return false;
    }
    
    SSL* sslPtr = static_cast<SSL*>(ssl.get());
    int result = SSL_accept(sslPtr);
    
    if (result != 1) {
        int error = SSL_get_error(sslPtr, result);
        if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
            // 需要重试
            return false;
        } else {
            handshakeFailures_++;
            std::cerr << "SSL handshake failed: " << getSslError() << "\n";
            return false;
        }
    }
    
    return true;
#else
    return true;
#endif
}

int TlsManager::sslRead(std::shared_ptr<void> ssl, void* buffer, int length) {
#ifdef HAVE_OPENSSL
    if (!ssl) {
        return -1;
    }
    
    SSL* sslPtr = static_cast<SSL*>(ssl.get());
    return SSL_read(sslPtr, buffer, length);
#else
    return -1;
#endif
}

int TlsManager::sslWrite(std::shared_ptr<void> ssl, const void* buffer, int length) {
#ifdef HAVE_OPENSSL
    if (!ssl) {
        return -1;
    }
    
    SSL* sslPtr = static_cast<SSL*>(ssl.get());
    return SSL_write(sslPtr, buffer, length);
#else
    return -1;
#endif
}

void TlsManager::closeSslConnection(std::shared_ptr<void> ssl) {
#ifdef HAVE_OPENSSL
    if (!ssl) {
        return;
    }
    
    SSL* sslPtr = static_cast<SSL*>(ssl.get());
    SSL_shutdown(sslPtr);
#endif
}

bool TlsManager::loadCertificate(const std::string& certFile, const std::string& keyFile) {
#ifdef HAVE_OPENSSL
    if (!sslCtx_) {
        std::cerr << "SSL context not created\n";
        return false;
    }
    
    return loadCertificateToContext(sslCtx_, certFile, keyFile);
#else
    return true;
#endif
}

bool TlsManager::loadCaCertificate(const std::string& caFile) {
#ifdef HAVE_OPENSSL
    if (!sslCtx_) {
        std::cerr << "SSL context not created\n";
        return false;
    }
    
    return loadCaCertificateToContext(sslCtx_, caFile);
#else
    return true;
#endif
}

bool TlsManager::verifyCertificate(std::shared_ptr<void> ssl, const std::string& hostname) {
#ifdef HAVE_OPENSSL
    if (!ssl) {
        return false;
    }
    
    SSL* sslPtr = static_cast<SSL*>(ssl.get());
    X509* cert = SSL_get_peer_certificate(sslPtr);
    
    if (!cert) {
        certificateErrors_++;
        return false;
    }
    
    // 验证证書链
    int verifyResult = SSL_get_verify_result(sslPtr);
    if (verifyResult != X509_V_OK) {
        certificateErrors_++;
        X509_free(cert);
        return false;
    }
    
    // 验证主機名
    if (!hostname.empty()) {
        if (X509_check_host(cert, hostname.c_str(), hostname.length(), 0, nullptr) != 1) {
            certificateErrors_++;
            X509_free(cert);
            return false;
        }
    }
    
    X509_free(cert);
    return true;
#else
    return true;
#endif
}

CertificateInfo TlsManager::getCertificateInfo(const std::string& certFile) {
    return parseCertificate(certFile);
}

bool TlsManager::generateSelfSignedCertificate(const std::string& certFile, 
                                             const std::string& keyFile,
                                             const std::string& commonName,
                                             int validityDays) {
#ifdef HAVE_OPENSSL
    try {
        // 生成私鑰
        if (!generateRsaKeyPair(keyFile)) {
            return false;
        }
        
        // 创建证書
        EVP_PKEY* pkey = nullptr;
        X509* x509 = X509_new();
        X509_NAME* name = nullptr;
        
        // 读取私鑰
        FILE* keyFilePtr = fopen(keyFile.c_str(), "r");
        if (!keyFilePtr) {
            std::cerr << "Failed to open key file: " << keyFile << "\n";
            X509_free(x509);
            return false;
        }
        
        pkey = PEM_read_PrivateKey(keyFilePtr, nullptr, nullptr, nullptr);
        fclose(keyFilePtr);
        
        if (!pkey) {
            std::cerr << "Failed to read private key\n";
            X509_free(x509);
            return false;
        }
        
        // 设置证書版本
        X509_set_version(x509, 2);
        
        // 设置序列號
        ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
        
        // 设置有效期
        X509_gmtime_adj(X509_get_notBefore(x509), 0);
        X509_gmtime_adj(X509_get_notAfter(x509), validityDays * 24 * 60 * 60);
        
        // 设置公鑰
        X509_set_pubkey(x509, pkey);
        
        // 设置主體名稱
        name = X509_get_subject_name(x509);
        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char*)"US", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "ST", MBSTRING_ASC, (unsigned char*)"State", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "L", MBSTRING_ASC, (unsigned char*)"City", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char*)"Organization", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)commonName.c_str(), -1, -1, 0);
        
        // 设置頒发者名稱
        X509_set_issuer_name(x509, name);
        
        // 簽名证書
        if (!X509_sign(x509, pkey, EVP_sha256())) {
            std::cerr << "Failed to sign certificate\n";
            X509_free(x509);
            EVP_PKEY_free(pkey);
            return false;
        }
        
        // 保存证書
        FILE* certFilePtr = fopen(certFile.c_str(), "w");
        if (!certFilePtr) {
            std::cerr << "Failed to open certificate file: " << certFile << "\n";
            X509_free(x509);
            EVP_PKEY_free(pkey);
            return false;
        }
        
        PEM_write_X509(certFilePtr, x509);
        fclose(certFilePtr);
        
        // 清理资源
        X509_free(x509);
        EVP_PKEY_free(pkey);
        
        std::cout << "Self-signed certificate generated: " << certFile << "\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to generate self-signed certificate: " << e.what() << "\n";
        return false;
    }
#else
    std::cout << "Self-signed certificate generation skipped (OpenSSL not available)\n";
    return true;
#endif
}

bool TlsManager::isCertificateExpiringSoon(const std::string& certFile, int daysThreshold) {
    auto certInfo = parseCertificate(certFile);
    return certInfo.isExpired || certInfo.daysUntilExpiry <= daysThreshold;
}

std::string TlsManager::getSslError() {
#ifdef HAVE_OPENSSL
    unsigned long error = ERR_get_error();
    if (error == 0) {
        return "No SSL error";
    }
    
    char errorString[256];
    ERR_error_string_n(error, errorString, sizeof(errorString));
    return std::string(errorString);
#else
    return "OpenSSL not available";
#endif
}

void TlsManager::cleanup() {
#ifdef HAVE_OPENSSL
    std::lock_guard<std::mutex> lock(sslCtxMutex_);
    
    if (sslCtx_) {
        SSL_CTX_free(sslCtx_);
        sslCtx_ = nullptr;
    }
    
    initialized_ = false;
    std::cout << "TlsManager cleaned up\n";
#endif
}

TlsManager::TlsStats TlsManager::getTlsStats() {
    TlsStats stats;
    stats.totalConnections = totalConnections_.load();
    stats.activeConnections = activeConnections_.load();
    stats.handshakeFailures = handshakeFailures_.load();
    stats.certificateErrors = certificateErrors_.load();
    stats.currentCipher = "TLS_AES_256_GCM_SHA384"; // 示例
    stats.currentProtocol = "TLSv1.3"; // 示例
    return stats;
}

bool TlsManager::initializeOpenSSL() {
#ifdef HAVE_OPENSSL
    // 初始化 OpenSSL 庫
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    // 初始化隨機数生成器
    if (RAND_status() != 1) {
        std::cerr << "OpenSSL random number generator not seeded\n";
        return false;
    }
    
    return true;
#else
    return true;
#endif
}

std::shared_ptr<void> TlsManager::createSslContextInternal(const TlsConfig& config) {
#ifdef HAVE_OPENSSL
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        return nullptr;
    }
    
    setSslOptions(ctx, config);
    return std::shared_ptr<void>(ctx, [](void* ptr) {
        SSL_CTX_free(static_cast<SSL_CTX*>(ptr));
    });
#else
    return nullptr;
#endif
}

void TlsManager::setSslOptions(std::shared_ptr<void> sslCtx, const TlsConfig& config) {
#ifdef HAVE_OPENSSL
    SSL_CTX* ctx = static_cast<SSL_CTX*>(sslCtx);
    
    // 设置最小和最大 TLS 版本
    SSL_CTX_set_min_proto_version(ctx, config.minVersion);
    SSL_CTX_set_max_proto_version(ctx, config.maxVersion);
    
    // 设置加密套件
    if (!config.cipherSuites.empty()) {
        if (!SSL_CTX_set_cipher_list(ctx, config.cipherSuites.c_str())) {
            std::cerr << "Failed to set cipher suites\n";
        }
    }
    
    // 设置验证選项
    if (config.verifyPeer) {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    } else {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    }
    
    // 设置會话緩存
    if (config.enableSessionResumption) {
        SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_SERVER);
        SSL_CTX_set_timeout(ctx, config.sessionTimeout);
    }
#endif
}

bool TlsManager::loadCertificateToContext(std::shared_ptr<void> sslCtx, 
                                        const std::string& certFile, 
                                        const std::string& keyFile) {
#ifdef HAVE_OPENSSL
    SSL_CTX* ctx = static_cast<SSL_CTX*>(sslCtx);
    
    // 加载证書
    if (SSL_CTX_use_certificate_file(ctx, certFile.c_str(), SSL_FILETYPE_PEM) != 1) {
        std::cerr << "Failed to load certificate file: " << certFile << "\n";
        return false;
    }
    
    // 加载私鑰
    if (SSL_CTX_use_PrivateKey_file(ctx, keyFile.c_str(), SSL_FILETYPE_PEM) != 1) {
        std::cerr << "Failed to load private key file: " << keyFile << "\n";
        return false;
    }
    
    // 验证私鑰和证書匹配
    if (SSL_CTX_check_private_key(ctx) != 1) {
        std::cerr << "Private key and certificate do not match\n";
        return false;
    }
    
    return true;
#else
    return true;
#endif
}

bool TlsManager::loadCaCertificateToContext(std::shared_ptr<void> sslCtx, const std::string& caFile) {
#ifdef HAVE_OPENSSL
    SSL_CTX* ctx = static_cast<SSL_CTX*>(sslCtx);
    
    if (SSL_CTX_load_verify_locations(ctx, caFile.c_str(), nullptr) != 1) {
        std::cerr << "Failed to load CA certificate file: " << caFile << "\n";
        return false;
    }
    
    return true;
#else
    return true;
#endif
}

CertificateInfo TlsManager::parseCertificate(const std::string& certFile) {
    CertificateInfo info;
    
#ifdef HAVE_OPENSSL
    FILE* file = fopen(certFile.c_str(), "r");
    if (!file) {
        info.isValid = false;
        return info;
    }
    
    X509* cert = PEM_read_X509(file, nullptr, nullptr, nullptr);
    fclose(file);
    
    if (!cert) {
        info.isValid = false;
        return info;
    }
    
    // 解析主體名稱
    X509_NAME* subject = X509_get_subject_name(cert);
    if (subject) {
        char buffer[256];
        X509_NAME_oneline(subject, buffer, sizeof(buffer));
        info.subject = buffer;
    }
    
    // 解析頒发者名稱
    X509_NAME* issuer = X509_get_issuer_name(cert);
    if (issuer) {
        char buffer[256];
        X509_NAME_oneline(issuer, buffer, sizeof(buffer));
        info.issuer = buffer;
    }
    
    // 解析序列號
    ASN1_INTEGER* serial = X509_get_serialNumber(cert);
    if (serial) {
        BIGNUM* bn = ASN1_INTEGER_to_BN(serial, nullptr);
        if (bn) {
            char* hex = BN_bn2hex(bn);
            if (hex) {
                info.serialNumber = hex;
                OPENSSL_free(hex);
            }
            BN_free(bn);
        }
    }
    
    // 解析有效期
    ASN1_TIME* notBefore = X509_get_notBefore(cert);
    ASN1_TIME* notAfter = X509_get_notAfter(cert);
    
    if (notBefore) {
        char buffer[64];
        ASN1_TIME_print(buffer, notBefore);
        info.notBefore = buffer;
    }
    
    if (notAfter) {
        char buffer[64];
        ASN1_TIME_print(buffer, notAfter);
        info.notAfter = buffer;
        
        // 检查是否过期
        time_t now = time(nullptr);
        time_t expiry = ASN1_TIME_get(notAfter);
        info.isExpired = (now > expiry);
        info.daysUntilExpiry = (expiry - now) / (24 * 60 * 60);
    }
    
    info.isValid = true;
    X509_free(cert);
#else
    info.isValid = false;
#endif
    
    return info;
}

bool TlsManager::generateRsaKeyPair(const std::string& keyFile, int keySize) {
#ifdef HAVE_OPENSSL
    EVP_PKEY* pkey = EVP_PKEY_new();
    if (!pkey) {
        return false;
    }
    
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return false;
    }
    
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }
    
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, keySize) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }
    
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }
    
    FILE* file = fopen(keyFile.c_str(), "w");
    if (!file) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }
    
    PEM_write_PrivateKey(file, pkey, nullptr, nullptr, 0, nullptr, nullptr);
    fclose(file);
    
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    
    return true;
#else
    return true;
#endif
}

bool TlsManager::createCertificateRequest(const std::string& csrFile, 
                                        const std::string& keyFile,
                                        const std::string& commonName) {
#ifdef HAVE_OPENSSL
    // 读取私鑰
    FILE* keyFilePtr = fopen(keyFile.c_str(), "r");
    if (!keyFilePtr) {
        return false;
    }
    
    EVP_PKEY* pkey = PEM_read_PrivateKey(keyFilePtr, nullptr, nullptr, nullptr);
    fclose(keyFilePtr);
    
    if (!pkey) {
        return false;
    }
    
    // 创建证書請求
    X509_REQ* req = X509_REQ_new();
    if (!req) {
        EVP_PKEY_free(pkey);
        return false;
    }
    
    // 设置版本
    X509_REQ_set_version(req, 0);
    
    // 设置主體名稱
    X509_NAME* name = X509_REQ_get_subject_name(req);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)commonName.c_str(), -1, -1, 0);
    
    // 设置公鑰
    X509_REQ_set_pubkey(req, pkey);
    
    // 簽名請求
    if (!X509_REQ_sign(req, pkey, EVP_sha256())) {
        X509_REQ_free(req);
        EVP_PKEY_free(pkey);
        return false;
    }
    
    // 保存請求
    FILE* csrFilePtr = fopen(csrFile.c_str(), "w");
    if (!csrFilePtr) {
        X509_REQ_free(req);
        EVP_PKEY_free(pkey);
        return false;
    }
    
    PEM_write_X509_REQ(csrFilePtr, req);
    fclose(csrFilePtr);
    
    X509_REQ_free(req);
    EVP_PKEY_free(pkey);
    
    return true;
#else
    return true;
#endif
}
