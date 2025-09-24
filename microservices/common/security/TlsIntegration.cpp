#include "TlsIntegration.h"
#include <iostream>

#ifdef HAVE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

TlsIntegration& TlsIntegration::getInstance() {
    static TlsIntegration instance;
    return instance;
}

bool TlsIntegration::initialize(const TlsIntegrationConfig& config) {
    config_ = config;
    initialized_ = false;
    
    muduoConnections_ = 0;
    grpcConnections_ = 0;
    handshakeFailures_ = 0;
    certificateErrors_ = 0;
    
    tlsManager_ = &TlsManager::getInstance();
    
    if (!config_.enableTls) {
        std::cout << "TLS integration disabled\n";
        initialized_ = true;
        return true;
    }
    
    // 初始化 TLS 管理器
    if (!tlsManager_->initialize()) {
        std::cerr << "Failed to initialize TLS manager\n";
        return false;
    }
    
    // 初始化 OpenSSL
    if (!initializeOpenSSL()) {
        std::cerr << "Failed to initialize OpenSSL\n";
        return false;
    }
    
    // 創建 SSL 上下文
    if (!createSslContext(config_)) {
        std::cerr << "Failed to create SSL context\n";
        return false;
    }
    
    // 加載證書
    if (!loadCertificates(config_)) {
        std::cerr << "Failed to load certificates\n";
        return false;
    }
    
    // 驗證證書
    if (!validateCertificates(config_)) {
        std::cerr << "Failed to validate certificates\n";
        return false;
    }
    
    initialized_ = true;
    std::cout << "TLS integration initialized successfully\n";
    return true;
}

bool TlsIntegration::configureMuduoServer(TcpServer& server, const TlsIntegrationConfig& config) {
    if (!config.enableTls || !initialized_) {
        return true;
    }
    
    // 設置連接回調以處理 TLS 握手
    server.setConnectionCallback([this](const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            configureMuduoConnection(conn, config_);
        }
    });
    
    std::cout << "Muduo server configured with TLS\n";
    return true;
}

bool TlsIntegration::configureMuduoConnection(const TcpConnectionPtr& conn, const TlsIntegrationConfig& config) {
    if (!config.enableTls || !initialized_) {
        return true;
    }
    
    try {
        // 獲取 socket 文件描述符
        int sockfd = conn->socket()->fd();
        
        // 創建 SSL 連接
        auto ssl = tlsManager_->createSslConnection(sockfd, true);
        if (!ssl) {
            std::cerr << "Failed to create SSL connection for muduo\n";
            handshakeFailures_++;
            return false;
        }
        
        // 執行 SSL 握手
        if (!tlsManager_->performHandshake(ssl)) {
            std::cerr << "SSL handshake failed for muduo connection\n";
            handshakeFailures_++;
            return false;
        }
        
        muduoConnections_++;
        std::cout << "Muduo connection configured with TLS\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to configure muduo connection with TLS: " << e.what() << "\n";
        handshakeFailures_++;
        return false;
    }
}

std::shared_ptr<grpc::ServerCredentials> TlsIntegration::createGrpcServerCredentials(const TlsIntegrationConfig& config) {
    if (!config.enableTls || !initialized_) {
        return grpc::InsecureServerCredentials();
    }
    
    try {
        // 創建 SSL 憑證選項
        auto sslOptions = createSslCredentialsOptions(config);
        if (!sslOptions) {
            std::cerr << "Failed to create SSL credentials options\n";
            return grpc::InsecureServerCredentials();
        }
        
        // 創建 gRPC SSL 服務器憑證
        auto credentials = grpc::SslServerCredentials(*sslOptions);
        grpcConnections_++;
        
        std::cout << "gRPC server credentials created with TLS\n";
        return credentials;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to create gRPC server credentials: " << e.what() << "\n";
        return grpc::InsecureServerCredentials();
    }
}

std::shared_ptr<grpc::ChannelCredentials> TlsIntegration::createGrpcClientCredentials(const TlsIntegrationConfig& config) {
    if (!config.enableTls || !initialized_) {
        return grpc::InsecureChannelCredentials();
    }
    
    try {
        // 創建 SSL 憑證選項
        auto sslOptions = createSslCredentialsOptions(config);
        if (!sslOptions) {
            std::cerr << "Failed to create SSL credentials options\n";
            return grpc::InsecureChannelCredentials();
        }
        
        // 創建 gRPC SSL 客戶端憑證
        auto credentials = grpc::SslCredentials(*sslOptions);
        grpcConnections_++;
        
        std::cout << "gRPC client credentials created with TLS\n";
        return credentials;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to create gRPC client credentials: " << e.what() << "\n";
        return grpc::InsecureChannelCredentials();
    }
}

bool TlsIntegration::isTlsEnabled() const {
    return config_.enableTls && initialized_;
}

const TlsIntegrationConfig& TlsIntegration::getConfig() const {
    return config_;
}

void TlsIntegration::updateConfig(const TlsIntegrationConfig& config) {
    config_ = config;
    
    if (config_.enableTls && initialized_) {
        reloadCertificates();
    }
}

bool TlsIntegration::reloadCertificates() {
    if (!initialized_) {
        return false;
    }
    
    // 重新加載證書
    if (!loadCertificates(config_)) {
        std::cerr << "Failed to reload certificates\n";
        return false;
    }
    
    std::cout << "Certificates reloaded successfully\n";
    return true;
}

TlsIntegration::TlsIntegrationStats TlsIntegration::getStats() {
    TlsIntegrationStats stats;
    stats.muduoConnections = muduoConnections_.load();
    stats.grpcConnections = grpcConnections_.load();
    stats.handshakeFailures = handshakeFailures_.load();
    stats.certificateErrors = certificateErrors_.load();
    stats.tlsEnabled = isTlsEnabled();
    stats.currentCipher = "TLS_AES_256_GCM_SHA384"; // 示例
    stats.currentProtocol = "TLSv1.3"; // 示例
    return stats;
}

bool TlsIntegration::initializeOpenSSL() {
#ifdef HAVE_OPENSSL
    // 初始化 OpenSSL 庫
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    // 初始化隨機數生成器
    if (RAND_status() != 1) {
        std::cerr << "OpenSSL random number generator not seeded\n";
        return false;
    }
    
    return true;
#else
    std::cerr << "OpenSSL not available\n";
    return false;
#endif
}

bool TlsIntegration::createSslContext(const TlsIntegrationConfig& config) {
#ifdef HAVE_OPENSSL
    // 創建 SSL 上下文
    sslCtx_ = SSL_CTX_new(TLS_server_method());
    if (!sslCtx_) {
        std::cerr << "Failed to create SSL context: " << tlsManager_->getSslError() << "\n";
        return false;
    }
    
    // 配置 SSL 選項
    configureSslOptions(config);
    
    return true;
#else
    return false;
#endif
}

void TlsIntegration::configureSslOptions(const TlsIntegrationConfig& config) {
#ifdef HAVE_OPENSSL
    if (!sslCtx_) {
        return;
    }
    
    // 設置最小和最大 TLS 版本
    SSL_CTX_set_min_proto_version(sslCtx_, config.minVersion);
    SSL_CTX_set_max_proto_version(sslCtx_, config.maxVersion);
    
    // 設置加密套件
    if (!config.cipherSuites.empty()) {
        if (!SSL_CTX_set_cipher_list(sslCtx_, config.cipherSuites.c_str())) {
            std::cerr << "Failed to set cipher suites\n";
        }
    }
    
    // 設置驗證選項
    if (config.verifyPeer) {
        SSL_CTX_set_verify(sslCtx_, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    } else {
        SSL_CTX_set_verify(sslCtx_, SSL_VERIFY_NONE, nullptr);
    }
    
    // 設置會話緩存
    SSL_CTX_set_session_cache_mode(sslCtx_, SSL_SESS_CACHE_SERVER);
    SSL_CTX_set_timeout(sslCtx_, 300); // 5 分鐘
#endif
}

bool TlsIntegration::loadCertificates(const TlsIntegrationConfig& config) {
    if (!config.enableTls) {
        return true;
    }
    
    // 使用 TlsManager 加載證書
    if (!config.certFile.empty() && !config.keyFile.empty()) {
        if (!tlsManager_->loadCertificate(config.certFile, config.keyFile)) {
            std::cerr << "Failed to load certificate files\n";
            return false;
        }
    }
    
    if (!config.caFile.empty()) {
        if (!tlsManager_->loadCaCertificate(config.caFile)) {
            std::cerr << "Failed to load CA certificate file\n";
            return false;
        }
    }
    
    return true;
}

bool TlsIntegration::validateCertificates(const TlsIntegrationConfig& config) {
    if (!config.enableTls) {
        return true;
    }
    
    // 驗證證書文件是否存在
    if (!config.certFile.empty()) {
        std::ifstream certFile(config.certFile);
        if (!certFile.good()) {
            std::cerr << "Certificate file not found: " << config.certFile << "\n";
            return false;
        }
    }
    
    if (!config.keyFile.empty()) {
        std::ifstream keyFile(config.keyFile);
        if (!keyFile.good()) {
            std::cerr << "Key file not found: " << config.keyFile << "\n";
            return false;
        }
    }
    
    return true;
}

std::shared_ptr<grpc::SslCredentialsOptions> TlsIntegration::createSslCredentialsOptions(const TlsIntegrationConfig& config) {
    auto options = std::make_shared<grpc::SslCredentialsOptions>();
    
    // 設置根證書
    if (!config.caFile.empty()) {
        std::ifstream caFile(config.caFile);
        if (caFile.good()) {
            std::string caContent((std::istreambuf_iterator<char>(caFile)),
                                 std::istreambuf_iterator<char>());
            options->pem_root_certs = caContent;
        }
    }
    
    // 設置客戶端證書和私鑰
    if (!config.certFile.empty() && !config.keyFile.empty()) {
        std::ifstream certFile(config.certFile);
        std::ifstream keyFile(config.keyFile);
        
        if (certFile.good() && keyFile.good()) {
            std::string certContent((std::istreambuf_iterator<char>(certFile)),
                                   std::istreambuf_iterator<char>());
            std::string keyContent((std::istreambuf_iterator<char>(keyFile)),
                                  std::istreambuf_iterator<char>());
            
            options->pem_cert_chain = certContent;
            options->pem_private_key = keyContent;
        }
    }
    
    return options;
}
