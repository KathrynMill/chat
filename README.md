# 分布式聊天室 KathrynMill/chatserver

## 專案簡介
本專案是一套基于 C++/Node.js/Redis/MariaDB 的分布式聊天室系统，支持高并发、群组聊天、WebSocket 实时通讯，適用于企業内网、教學與分布式部署学习場景。

## 技术栈
- C++（核心服务端）
- Node.js + Express + WebSocket（前端/中介層）
- Redis（消息隊列/状态同步）
- MariaDB（用户数据库）
- Docker/Kubernetes（容器化與分布式部署）
- NGINX（反向代理，選配）

## 功能特點
- 注册/登录/好友/群组/私聊/群聊
- WebSocket 即時消息推送
- 支援多用户同時在线
- 支援本地化私有部署與 K8s 集群扩展

## 快速开始

### 1. 编译與启动 C++ 服务端
```bash
cd chatserver
mkdir build && cd build
cmake ..
make -j4
# 启动服务（例：6000端口）
./bin/ChatServer 127.0.0.1 6000
```

### 2. 启动 Redis/MariaDB
請確保本機已安裝並启动 redis-server、mariadb。

### 3. 启动 Web 前端
```bash
cd web
npm install
npm start
# 瀏覽器访问 http://127.0.0.1:3000
```

### 4. 测试
- 使用 ChatClient 测试 TCP 聊天協議
- 使用网頁端进行注册、登录、聊天

## 分布式部署（Kubernetes 本地方案）

### 1. 本地 Registry 构建與推送
```bash
# 參考 deploy/registry.sh
```

### 2. K8s YAML 一键部署
```bash
kubectl apply -f deploy/all-in-one.yaml
```

### 3. 内网 hosts 配置
請將 chatserver.local 指向本機或 K8s Ingress IP。

### 4. Redis/Kafka 本地安裝
可用 Helm Chart 离线安裝，詳見 deploy/README.md。

## 常见问题
- 端口佔用：`sudo lsof -i:6000` 查找並 kill
- 依賴缺失：請參考上方依賴安裝命令
- 服务未重启：每次修改後請重启對應服务
- 内存不足：建議 2G+ RAM

---

如需自动化脚本、K8s YAML、Helm Chart、详细部署文档，請參見 `deploy/` 目录。

## 微服务模式（可選）

本專案已提供基于 gRPC/Protobuf 的微服务骨架與 Gateway 骨架，預设不启用。开启方式與构建步骤如下：

### 依賴
- gRPC、Protobuf（编译期）
- 後續可選：muduo、Kafka(cppkafka 或 confluent-kafka-cpp)、redis-plus-plus/hiredis、MariaDB client

Ubuntu 參考安裝（示例）：
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libprotobuf-dev protobuf-compiler protobuf-compiler-grpc libgrpc++-dev
# 其他庫（可選，之後完善時再裝）：muduo、librdkafka、cppkafka、hiredis、redis-plus-plus、libmariadb-dev
```

### 启用與构建
```bash
cd /home/aa/Documents/chat
mkdir build && cd build
cmake -DBUILD_MICROSERVICES=ON ..
make -j4
```

完成後將產生（骨架可执行檔）：
- `microservices/gateway/chat_gateway`
- `microservices/user_service/user_service`
- `microservices/social_service/social_service`
- `microservices/message_service/message_service`

目前為最小骨架：
- 已提供 `.proto`（`microservices/proto/`）
- 後續將在各服务加入 gRPC 服务实作、数据库/Redis/Kafka 连線，以及 Gateway 的 muduo 事件循环與 gRPC 客户端、Kafka 消費邏輯。

### 一键安裝依賴（Ubuntu/Debian）
```bash
chmod +x ./install_micro_deps.sh
./install_micro_deps.sh
```

### 启动基础设施（Kafka/Redis）
```bash
docker compose up -d
# Kafka: localhost:9092, ZK: 2181, Redis: 6379
```

### 启动微服务骨架
```bash
cd /home/aa/Documents/chat/build
./microservices/user_service/user_service &
./microservices/social_service/social_service &
./microservices/message_service/message_service &
./microservices/gateway/chat_gateway &
```

### 服务环境变数
- `REDIS_URL`：預设 `tcp://127.0.0.1:6379`
- `KAFKA_BROKERS`：預设 `127.0.0.1:9092`
- DB（MariaDB/MySQL）：
  - `DB_HOST` 預设 `127.0.0.1`
  - `DB_PORT` 預设 `3306`
  - `DB_USER` 預设 `root`
  - `DB_PASS` 預设空字串
  - `DB_NAME` 預设 `chatdb`
- 微服务多实例（Gateway 负载均衡）：
  - `SERVICE_USER`：預设 `127.0.0.1:60051`，可设多个用逗號分隔
  - `SERVICE_SOCIAL`：預设 `127.0.0.1:60052`，可设多个用逗號分隔
  - `SERVICE_MESSAGE`：預设 `127.0.0.1:60053`，可设多个用逗號分隔

### 初始化数据库（示例）
```sql
CREATE DATABASE IF NOT EXISTS chatdb CHARACTER SET utf8mb4;
USE chatdb;
CREATE TABLE IF NOT EXISTS users (
  id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(64) NOT NULL,
  hashed_pwd VARCHAR(128) NOT NULL,
  state VARCHAR(16) NOT NULL DEFAULT 'offline',
  UNIQUE KEY uk_name(name)
);

-- 新增：社交/讯息最小持久化
CREATE TABLE IF NOT EXISTS friends (
  user_id INT NOT NULL,
  friend_id INT NOT NULL,
  PRIMARY KEY (user_id, friend_id)
);

CREATE TABLE IF NOT EXISTS `groups` (
  id INT AUTO_INCREMENT PRIMARY KEY,
  owner_id INT NOT NULL,
  name VARCHAR(64) NOT NULL,
  `desc` VARCHAR(255) DEFAULT ''
);

CREATE TABLE IF NOT EXISTS group_members (
  group_id INT NOT NULL,
  user_id INT NOT NULL,
  PRIMARY KEY (group_id, user_id)
);

CREATE TABLE IF NOT EXISTS messages (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  from_id INT NOT NULL,
  to_id INT NOT NULL,
  group_id INT NOT NULL,
  content TEXT NOT NULL,
  timestamp_ms BIGINT NOT NULL,
  msg_id VARCHAR(64) DEFAULT ''
);

CREATE TABLE IF NOT EXISTS offline_msgs (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  user_id INT NOT NULL,
  payload TEXT NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### 端到端本機启动與测试（最小）
```bash
# 启动基础设施
docker compose up -d

# 构建微服务
cd /home/aa/Documents/chat
mkdir -p build && cd build
cmake -DBUILD_MICROSERVICES=ON ..
make -j4

# 设定 DB 环境变数（依需求调整）
export DB_HOST=127.0.0.1
export DB_PORT=3306
export DB_USER=root
export DB_PASS=""
export DB_NAME=chatdb

# 启动微服务
./microservices/user_service/user_service &
./microservices/social_service/social_service &
./microservices/message_service/message_service &
./microservices/gateway/chat_gateway &

# 测试：TCP 送 JSON（使用 netcat 或自写工具）
# 1) 登录
printf '{"msgid":1,"id":1,"password":"pwd"}\n' | nc 127.0.0.1 7000
# 2) 加好友
printf '{"msgid":2001,"user_id":1,"friend_id":2}\n' | nc 127.0.0.1 7000
# 3) 建群
printf '{"msgid":2003,"owner_id":1,"name":"team","desc":"demo"}\n' | nc 127.0.0.1 7000
# 4) 拉人入群
printf '{"msgid":2005,"group_id":1,"user_id":2}\n' | nc 127.0.0.1 7000
# 5) 私聊
printf '{"msgid":1001,"from_id":1,"to_id":2,"content":"hi","timestamp_ms":1690000000000}\n' | nc 127.0.0.1 7000
# 6) 群聊
printf '{"msgid":1003,"from_id":1,"group_id":1,"content":"hello group","timestamp_ms":1690000000001}\n' | nc 127.0.0.1 7000
```

### 企业级特性（已实作）
- **服务发现與负载均衡**：Gateway 支援多实例轮询（透过 `SERVICE_*` 环境变数）
- **异步讯息推送**：MessageService 写入 DB 後发送 Kafka，Gateway 消費並推送到在线用户
- **连線管理**：Gateway 維護用户连線映射，支援即時推送與离线消息
- **最小持久化**：Social/Message 服务支援 DB 写入與查询

### 多实例部署示例
```bash
# 启动多个 MessageService 实例
export DB_HOST=127.0.0.1 DB_NAME=chatdb
./microservices/message_service/message_service &
./microservices/message_service/message_service &  # 第二个实例

# Gateway 设定多实例负载均衡
export SERVICE_MESSAGE="127.0.0.1:60053,127.0.0.1:60054"
export KAFKA_BROKERS="127.0.0.1:9092"
./microservices/gateway/chat_gateway &
```

### 企业级特性（已实作）
- **Consul 服务注册與发现**：微服务自动注册到 Consul，Gateway 动态发现健康实例
- **熔断器机制**：gRPC 客户端熔断器，防止级联失败
- **JWT 身份验证**：Gateway 验证 JWT token，透傳用户身份
- **服务发现與负载均衡**：支援多实例轮询與健康检查
- **异步讯息推送**：Kafka 消息隊列與实时推送
- **连線管理**：用户會话綁定與离线消息

### 环境变数（企业级）
- `CONSUL_URL`：Consul 服务地址，預设 `http://127.0.0.1:8500`
- `JWT_SECRET`：JWT 签名密鑰，預设 `your-secret-key`
- `KAFKA_BROKERS`：Kafka 集群地址，預设 `127.0.0.1:9092`
- `SERVICE_*`：微服务多实例配置（逗號分隔）

### 企业级部署示例
```bash
# 启动 Consul
docker run -d --name consul -p 8500:8500 consul:latest

# 设定企业级环境变数
export CONSUL_URL=http://127.0.0.1:8500
export JWT_SECRET=your-production-secret-key
export KAFKA_BROKERS=127.0.0.1:9092
export DB_HOST=127.0.0.1 DB_NAME=chatdb

# 启动微服务（自动注册到 Consul）
./microservices/user_service/user_service &
./microservices/social_service/social_service &
./microservices/message_service/message_service &

# 启动 Gateway（自动发现服务）
./microservices/gateway/chat_gateway &
```

## 运维特性（已实作）

### 分布式追踪（OpenTelemetry）
- 跨服务请求追踪與 span 注入
- gRPC/HTTP 自动追踪
- Jaeger 整合與可视化
- 环境变数：`JAEGER_ENDPOINT=http://localhost:14268/api/traces`

### 指标监控（Prometheus）
- 服务健康状态监控
- gRPC/HTTP 性能指标（QPS、延迟、错误率）
- 业务指标（在线用户、连接数、数据库查询）
- 环境变数：`METRICS_PORT=8080`

### 容器化部署
- Docker 多阶段构建优化
- Docker Compose 一键部署
- Kubernetes 生产级配置
- 自动健康检查與重启

### 监控面板（Grafana）
- 服务健康状态儀表板
- 性能指标可視化
- 业务指标监控
- 访问地址：`http://localhost:3000`（admin/admin）

## 部署指南

### Docker Compose 部署
```bash
# 启动完整企业级环境
docker-compose -f docker-compose.enterprise.yml up -d

# 查看服务状态
docker-compose -f docker-compose.enterprise.yml ps

# 查看日志
docker-compose -f docker-compose.enterprise.yml logs -f gateway
```

### Kubernetes 部署
```bash
# 创建命名空間
kubectl apply -f deploy/k8s/namespace.yaml

# 部署配置
kubectl apply -f deploy/k8s/configmap.yaml

# 部署服务
kubectl apply -f deploy/k8s/services-deployment.yaml
kubectl apply -f deploy/k8s/gateway-deployment.yaml

# 查看部署状态
kubectl get pods -n chat-system
kubectl get services -n chat-system
```

### 监控與故障排查
```bash
# 查看 Prometheus 指标
curl http://localhost:9090/targets

# 查看 Jaeger 追踪
open http://localhost:16686

# 查看 Grafana 儀表板
open http://localhost:3000

# 查看服务健康状态
curl http://localhost:8080/health
curl http://localhost:8081/health
curl http://localhost:8082/health
curl http://localhost:8083/health
```

## 运维工具

### 部署脚本
```bash
# 一键部署（Docker Compose）
./deploy/deploy.sh

# Kubernetes 部署
./deploy/deploy.sh --k8s

# 清理部署
./deploy/deploy.sh --cleanup
```

### 监控工具
```bash
# 系统状态概覽
./deploy/monitor.sh overview

# 健康检查
./deploy/monitor.sh health

# 查看日志
./deploy/monitor.sh logs gateway 100

# 实时日志
./deploy/monitor.sh follow gateway

# 故障排查指南
./deploy/monitor.sh troubleshoot
```

### 测试工具
```bash
# 完整测试套件
./deploy/test.sh full

# 快速测试
./deploy/test.sh quick

# 性能测试
./deploy/test.sh performance

# 健康检查测试
./deploy/test.sh health
```

### 运维文档
- **完整运维指南**: `deploy/OPERATIONS.md`
- **部署脚本**: `deploy/deploy.sh`
- **监控脚本**: `deploy/monitor.sh`
- **测试脚本**: `deploy/test.sh`

### 後續扩展建議
- 日志聚合（ELK Stack）
- 服务网格（Istio）
- 自动扩缩容（HPA）
- 金丝雀部署
- 混沌工程（Chaos Monkey）


