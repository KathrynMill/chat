#pragma once
#include <string>
#include <memory>
#include <functional>

#ifdef HAVE_MUDUO
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/InetAddress.h>
using namespace muduo;
using namespace muduo::net;
#endif

#ifdef HAVE_GRPC
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/security/credentials.h>
#endif

#include "TlsManager.h"

// TLS 配置
struct TlsIntegrationConfig {
    bool enableTls = false;
    std::string certFile;
    std::string keyFile;
    std::string caFile;
    bool verifyPeer = true;
    bool verifyHostname = true;
    std::string cipherSuites = "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256";
    int minVersion = TLS1_2_VERSION;
    int maxVersion = TLS1_3_VERSION;
};

// TLS 集成管理器
class TlsIntegration {
public:
    static TlsIntegration& getInstance();
    
    // 初始化 TLS 集成
    bool initialize(const TlsIntegrationConfig& config);
    
    // 為 muduo TcpServer 配置 TLS
    bool configureMuduoServer(TcpServer& server, const TlsIntegrationConfig& config);
    
    // 為 muduo TcpConnection 配置 TLS
    bool configureMuduoConnection(const TcpConnectionPtr& conn, const TlsIntegrationConfig& config);
    
    // 為 gRPC Server 配置 TLS
    std::shared_ptr<grpc::ServerCredentials> createGrpcServerCredentials(const TlsIntegrationConfig& config);
    
    // 為 gRPC Client 配置 TLS
    std::shared_ptr<grpc::ChannelCredentials> createGrpcClientCredentials(const TlsIntegrationConfig& config);
    
    // 检查 TLS 是否启用
    bool isTlsEnabled() const;
    
    // 获取 TLS 配置
    const TlsIntegrationConfig& getConfig() const;
    
    // 更新 TLS 配置
    void updateConfig(const TlsIntegrationConfig& config);
    
    // 重新加载证書
    bool reloadCertificates();
    
    // 获取 TLS 统計
    struct TlsIntegrationStats {
        int muduoConnections;
        int grpcConnections;
        int handshakeFailures;
        int certificateErrors;
        bool tlsEnabled;
        std::string currentCipher;
        std::string currentProtocol;
    };
    TlsIntegrationStats getStats();

private:
    TlsIntegration() = default;
    ~TlsIntegration() = default;
    TlsIntegration(const TlsIntegration&) = delete;
    TlsIntegration& operator=(const TlsIntegration&) = delete;
    
    // 初始化 OpenSSL
    bool initializeOpenSSL();
    
    // 创建 SSL 上下文
    bool createSslContext(const TlsIntegrationConfig& config);
    
    // 配置 SSL 選项
    void configureSslOptions(const TlsIntegrationConfig& config);
    
    // 加载证書
    bool loadCertificates(const TlsIntegrationConfig& config);
    
    // 验证证書
    bool validateCertificates(const TlsIntegrationConfig& config);
    
    // 创建 gRPC SSL 憑证
    std::shared_ptr<grpc::SslCredentialsOptions> createSslCredentialsOptions(const TlsIntegrationConfig& config);
    
    // 统計信息
    std::atomic<int> muduoConnections_;
    std::atomic<int> grpcConnections_;
    std::atomic<int> handshakeFailures_;
    std::atomic<int> certificateErrors_;
    
    // 配置
    TlsIntegrationConfig config_;
    bool initialized_;
    
    // TLS 管理器
    TlsManager* tlsManager_;
    
#ifdef HAVE_OPENSSL
    SSL_CTX* sslCtx_;
#endif
};
