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
- Kafka broker 預設 `127.0.0.1:9092`（在程式碼內寫死，後續可改環境變數）
- DB（MariaDB/MySQL）：
  - `DB_HOST` 預設 `127.0.0.1`
  - `DB_PORT` 預設 `3306`
  - `DB_USER` 預設 `root`
  - `DB_PASS` 預設空字串
  - `DB_NAME` 預設 `chatdb`

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


