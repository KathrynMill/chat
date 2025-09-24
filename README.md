# 分散式聊天室 KathrynMill/chatserver

## 專案簡介
本專案是一套基於 C++/Node.js/Redis/MariaDB 的分散式聊天室系統，支持高併發、群組聊天、WebSocket 實時通訊，適用於企業內網、教學與分散式部署學習場景。

## 技術棧
- C++（核心服務端）
- Node.js + Express + WebSocket（前端/中介層）
- Redis（消息隊列/狀態同步）
- MariaDB（用戶資料庫）
- Docker/Kubernetes（容器化與分散式部署）
- NGINX（反向代理，選配）

## 功能特點
- 註冊/登入/好友/群組/私聊/群聊
- WebSocket 即時消息推送
- 支援多用戶同時在線
- 支援本地化私有部署與 K8s 集群擴展

## 快速開始

### 1. 編譯與啟動 C++ 服務端
```bash
cd chatserver
mkdir build && cd build
cmake ..
make -j4
# 啟動服務（例：6000端口）
./bin/ChatServer 127.0.0.1 6000
```

### 2. 啟動 Redis/MariaDB
請確保本機已安裝並啟動 redis-server、mariadb。

### 3. 啟動 Web 前端
```bash
cd web
npm install
npm start
# 瀏覽器訪問 http://127.0.0.1:3000
```

### 4. 測試
- 使用 ChatClient 測試 TCP 聊天協議
- 使用網頁端進行註冊、登入、聊天

## 分散式部署（Kubernetes 本地方案）

### 1. 本地 Registry 構建與推送
```bash
# 參考 deploy/registry.sh
```

### 2. K8s YAML 一鍵部署
```bash
kubectl apply -f deploy/all-in-one.yaml
```

### 3. 內網 hosts 配置
請將 chatserver.local 指向本機或 K8s Ingress IP。

### 4. Redis/Kafka 本地安裝
可用 Helm Chart 離線安裝，詳見 deploy/README.md。

## 常見問題
- 端口佔用：`sudo lsof -i:6000` 查找並 kill
- 依賴缺失：請參考上方依賴安裝命令
- 服務未重啟：每次修改後請重啟對應服務
- 記憶體不足：建議 2G+ RAM

---

如需自動化腳本、K8s YAML、Helm Chart、詳細部署文檔，請參見 `deploy/` 目錄。

## 微服務模式（可選）

本專案已提供基於 gRPC/Protobuf 的微服務骨架與 Gateway 骨架，預設不啟用。開啟方式與建置步驟如下：

### 依賴
- gRPC、Protobuf（編譯期）
- 後續可選：muduo、Kafka(cppkafka 或 confluent-kafka-cpp)、redis-plus-plus/hiredis、MariaDB client

Ubuntu 參考安裝（示例）：
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libprotobuf-dev protobuf-compiler protobuf-compiler-grpc libgrpc++-dev
# 其他庫（可選，之後完善時再裝）：muduo、librdkafka、cppkafka、hiredis、redis-plus-plus、libmariadb-dev
```

### 啟用與建置
```bash
cd /home/aa/Documents/chat
mkdir build && cd build
cmake -DBUILD_MICROSERVICES=ON ..
make -j4
```

完成後將產生（骨架可執行檔）：
- `microservices/gateway/chat_gateway`
- `microservices/user_service/user_service`
- `microservices/social_service/social_service`
- `microservices/message_service/message_service`

目前為最小骨架：
- 已提供 `.proto`（`microservices/proto/`）
- 後續將在各服務加入 gRPC 服務實作、資料庫/Redis/Kafka 連線，以及 Gateway 的 muduo 事件循環與 gRPC 客戶端、Kafka 消費邏輯。

### 一鍵安裝依賴（Ubuntu/Debian）
```bash
chmod +x ./install_micro_deps.sh
./install_micro_deps.sh
```

### 啟動基礎設施（Kafka/Redis）
```bash
docker compose up -d
# Kafka: localhost:9092, ZK: 2181, Redis: 6379
```

### 啟動微服務骨架
```bash
cd /home/aa/Documents/chat/build
./microservices/user_service/user_service &
./microservices/social_service/social_service &
./microservices/message_service/message_service &
./microservices/gateway/chat_gateway &
```

### 服務環境變數
- `REDIS_URL`：預設 `tcp://127.0.0.1:6379`
- `KAFKA_BROKERS`：預設 `127.0.0.1:9092`
- DB（MariaDB/MySQL）：
  - `DB_HOST` 預設 `127.0.0.1`
  - `DB_PORT` 預設 `3306`
  - `DB_USER` 預設 `root`
  - `DB_PASS` 預設空字串
  - `DB_NAME` 預設 `chatdb`
- 微服務多實例（Gateway 負載均衡）：
  - `SERVICE_USER`：預設 `127.0.0.1:60051`，可設多個用逗號分隔
  - `SERVICE_SOCIAL`：預設 `127.0.0.1:60052`，可設多個用逗號分隔
  - `SERVICE_MESSAGE`：預設 `127.0.0.1:60053`，可設多個用逗號分隔

### 初始化資料庫（示例）
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

-- 新增：社交/訊息最小持久化
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

### 端到端本機啟動與測試（最小）
```bash
# 啟動基礎設施
docker compose up -d

# 建置微服務
cd /home/aa/Documents/chat
mkdir -p build && cd build
cmake -DBUILD_MICROSERVICES=ON ..
make -j4

# 設定 DB 環境變數（依需求調整）
export DB_HOST=127.0.0.1
export DB_PORT=3306
export DB_USER=root
export DB_PASS=""
export DB_NAME=chatdb

# 啟動微服務
./microservices/user_service/user_service &
./microservices/social_service/social_service &
./microservices/message_service/message_service &
./microservices/gateway/chat_gateway &

# 測試：TCP 送 JSON（使用 netcat 或自寫工具）
# 1) 登入
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

### 企業級特性（已實作）
- **服務發現與負載均衡**：Gateway 支援多實例輪詢（透過 `SERVICE_*` 環境變數）
- **異步訊息推送**：MessageService 寫入 DB 後發送 Kafka，Gateway 消費並推送到在線用戶
- **連線管理**：Gateway 維護用戶連線映射，支援即時推送與離線消息
- **最小持久化**：Social/Message 服務支援 DB 寫入與查詢

### 多實例部署示例
```bash
# 啟動多個 MessageService 實例
export DB_HOST=127.0.0.1 DB_NAME=chatdb
./microservices/message_service/message_service &
./microservices/message_service/message_service &  # 第二個實例

# Gateway 設定多實例負載均衡
export SERVICE_MESSAGE="127.0.0.1:60053,127.0.0.1:60054"
export KAFKA_BROKERS="127.0.0.1:9092"
./microservices/gateway/chat_gateway &
```

### 企業級特性（已實作）
- **Consul 服務註冊與發現**：微服務自動註冊到 Consul，Gateway 動態發現健康實例
- **熔斷器機制**：gRPC 客戶端熔斷器，防止級聯失敗
- **JWT 身份驗證**：Gateway 驗證 JWT token，透傳用戶身份
- **服務發現與負載均衡**：支援多實例輪詢與健康檢查
- **異步訊息推送**：Kafka 消息隊列與實時推送
- **連線管理**：用戶會話綁定與離線消息

### 環境變數（企業級）
- `CONSUL_URL`：Consul 服務地址，預設 `http://127.0.0.1:8500`
- `JWT_SECRET`：JWT 簽名密鑰，預設 `your-secret-key`
- `KAFKA_BROKERS`：Kafka 集群地址，預設 `127.0.0.1:9092`
- `SERVICE_*`：微服務多實例配置（逗號分隔）

### 企業級部署示例
```bash
# 啟動 Consul
docker run -d --name consul -p 8500:8500 consul:latest

# 設定企業級環境變數
export CONSUL_URL=http://127.0.0.1:8500
export JWT_SECRET=your-production-secret-key
export KAFKA_BROKERS=127.0.0.1:9092
export DB_HOST=127.0.0.1 DB_NAME=chatdb

# 啟動微服務（自動註冊到 Consul）
./microservices/user_service/user_service &
./microservices/social_service/social_service &
./microservices/message_service/message_service &

# 啟動 Gateway（自動發現服務）
./microservices/gateway/chat_gateway &
```

## 運維特性（已實作）

### 分散式追蹤（OpenTelemetry）
- 跨服務請求追蹤與 span 注入
- gRPC/HTTP 自動追蹤
- Jaeger 整合與視覺化
- 環境變數：`JAEGER_ENDPOINT=http://localhost:14268/api/traces`

### 指標監控（Prometheus）
- 服務健康狀態監控
- gRPC/HTTP 性能指標（QPS、延遲、錯誤率）
- 業務指標（在線用戶、連接數、資料庫查詢）
- 環境變數：`METRICS_PORT=8080`

### 容器化部署
- Docker 多階段建置優化
- Docker Compose 一鍵部署
- Kubernetes 生產級配置
- 自動健康檢查與重啟

### 監控面板（Grafana）
- 服務健康狀態儀表板
- 性能指標可視化
- 業務指標監控
- 訪問地址：`http://localhost:3000`（admin/admin）

## 部署指南

### Docker Compose 部署
```bash
# 啟動完整企業級環境
docker-compose -f docker-compose.enterprise.yml up -d

# 查看服務狀態
docker-compose -f docker-compose.enterprise.yml ps

# 查看日誌
docker-compose -f docker-compose.enterprise.yml logs -f gateway
```

### Kubernetes 部署
```bash
# 創建命名空間
kubectl apply -f deploy/k8s/namespace.yaml

# 部署配置
kubectl apply -f deploy/k8s/configmap.yaml

# 部署服務
kubectl apply -f deploy/k8s/services-deployment.yaml
kubectl apply -f deploy/k8s/gateway-deployment.yaml

# 查看部署狀態
kubectl get pods -n chat-system
kubectl get services -n chat-system
```

### 監控與故障排查
```bash
# 查看 Prometheus 指標
curl http://localhost:9090/targets

# 查看 Jaeger 追蹤
open http://localhost:16686

# 查看 Grafana 儀表板
open http://localhost:3000

# 查看服務健康狀態
curl http://localhost:8080/health
curl http://localhost:8081/health
curl http://localhost:8082/health
curl http://localhost:8083/health
```

## 運維工具

### 部署腳本
```bash
# 一鍵部署（Docker Compose）
./deploy/deploy.sh

# Kubernetes 部署
./deploy/deploy.sh --k8s

# 清理部署
./deploy/deploy.sh --cleanup
```

### 監控工具
```bash
# 系統狀態概覽
./deploy/monitor.sh overview

# 健康檢查
./deploy/monitor.sh health

# 查看日誌
./deploy/monitor.sh logs gateway 100

# 實時日誌
./deploy/monitor.sh follow gateway

# 故障排查指南
./deploy/monitor.sh troubleshoot
```

### 測試工具
```bash
# 完整測試套件
./deploy/test.sh full

# 快速測試
./deploy/test.sh quick

# 性能測試
./deploy/test.sh performance

# 健康檢查測試
./deploy/test.sh health
```

### 運維文檔
- **完整運維指南**: `deploy/OPERATIONS.md`
- **部署腳本**: `deploy/deploy.sh`
- **監控腳本**: `deploy/monitor.sh`
- **測試腳本**: `deploy/test.sh`

### 後續擴展建議
- 日誌聚合（ELK Stack）
- 服務網格（Istio）
- 自動擴縮容（HPA）
- 金絲雀部署
- 混沌工程（Chaos Monkey）


