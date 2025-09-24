# 企業級聊天系統運維指南

## 目錄
- [部署架構](#部署架構)
- [監控與告警](#監控與告警)
- [故障排查](#故障排查)
- [性能優化](#性能優化)
- [備份與恢復](#備份與恢復)
- [安全配置](#安全配置)
- [擴展指南](#擴展指南)

## 部署架構

### 系統組件
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Load Balancer │    │   API Gateway   │    │   Microservices │
│   (NGINX/HAProxy)│    │   (Port 7000)   │    │   (gRPC)        │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
         ┌───────────────────────┼───────────────────────┐
         │                       │                       │
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Service Mesh  │    │   Data Layer    │    │   Monitoring    │
│   (Consul)      │    │   (MariaDB)     │    │   (Prometheus)  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
         ┌───────────────────────┼───────────────────────┐
         │                       │                       │
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Message Queue │    │   Tracing       │    │   Visualization │
│   (Kafka)       │    │   (Jaeger)      │    │   (Grafana)     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### 端口配置
| 服務 | 端口 | 用途 |
|------|------|------|
| Gateway | 7000 | 客戶端連接 |
| Gateway | 8080 | 健康檢查/指標 |
| User Service | 60051 | gRPC |
| User Service | 8081 | 健康檢查/指標 |
| Social Service | 60052 | gRPC |
| Social Service | 8082 | 健康檢查/指標 |
| Message Service | 60053 | gRPC |
| Message Service | 8083 | 健康檢查/指標 |
| MariaDB | 3306 | 資料庫 |
| Kafka | 9092 | 訊息隊列 |
| Consul | 8500 | 服務發現 |
| Prometheus | 9090 | 指標收集 |
| Grafana | 3000 | 監控面板 |
| Jaeger | 16686 | 追蹤視覺化 |

## 監控與告警

### 關鍵指標

#### 業務指標
- **在線用戶數** (`online_users`): 當前在線用戶數量
- **活躍連接數** (`active_connections`): 當前活躍連接數量
- **訊息吞吐量** (`kafka_messages_total`): 每秒處理的訊息數量

#### 技術指標
- **gRPC 調用率** (`grpc_calls_total`): 每秒 gRPC 調用次數
- **gRPC 延遲** (`grpc_call_duration_seconds`): gRPC 調用延遲分佈
- **HTTP 請求率** (`http_requests_total`): 每秒 HTTP 請求次數
- **資料庫查詢率** (`database_queries_total`): 每秒資料庫查詢次數

#### 系統指標
- **CPU 使用率**: 各服務 CPU 使用情況
- **記憶體使用率**: 各服務記憶體使用情況
- **網路 I/O**: 網路流量統計
- **磁碟 I/O**: 磁碟讀寫統計

### 告警規則

#### 關鍵告警
```yaml
# 服務不可用
- alert: ServiceDown
  expr: up{job=~"gateway|user-service|social-service|message-service"} == 0
  for: 1m
  labels:
    severity: critical
  annotations:
    summary: "服務 {{ $labels.job }} 不可用"

# 高錯誤率
- alert: HighErrorRate
  expr: rate(grpc_calls_total{status="error"}[5m]) / rate(grpc_calls_total[5m]) > 0.1
  for: 2m
  labels:
    severity: warning
  annotations:
    summary: "服務 {{ $labels.service }} 錯誤率過高"

# 高延遲
- alert: HighLatency
  expr: histogram_quantile(0.95, rate(grpc_call_duration_seconds_bucket[5m])) > 1
  for: 3m
  labels:
    severity: warning
  annotations:
    summary: "服務 {{ $labels.service }} 延遲過高"
```

### 監控面板

#### Grafana 儀表板
- **系統概覽**: 服務健康狀態、資源使用率
- **性能監控**: gRPC/HTTP 性能指標
- **業務監控**: 用戶活動、訊息統計
- **基礎設施**: 資料庫、Kafka、Consul 狀態

## 故障排查

### 常見問題

#### 1. 服務啟動失敗
**症狀**: 服務無法啟動或立即退出
**排查步驟**:
```bash
# 檢查端口佔用
netstat -tulpn | grep :7000

# 檢查 Docker 容器狀態
docker ps -a

# 查看容器日誌
docker logs <container_id>

# 檢查環境變數
docker exec <container_id> env
```

**解決方案**:
- 釋放被佔用的端口
- 檢查環境變數配置
- 驗證依賴服務狀態

#### 2. 資料庫連接問題
**症狀**: 服務無法連接資料庫
**排查步驟**:
```bash
# 檢查 MariaDB 狀態
docker exec -it chat-mariadb mysql -u root -p

# 測試資料庫連接
telnet $DB_HOST $DB_PORT

# 檢查資料庫配置
echo $DB_HOST $DB_PORT $DB_NAME
```

**解決方案**:
- 檢查資料庫服務狀態
- 驗證連接參數
- 檢查網路連接

#### 3. Kafka 連接問題
**症狀**: 訊息無法發送到 Kafka
**排查步驟**:
```bash
# 檢查 Kafka 狀態
docker exec -it chat-kafka kafka-topics --list --bootstrap-server localhost:9092

# 檢查 Kafka 配置
echo $KAFKA_BROKERS

# 測試 Kafka 連接
docker exec -it chat-kafka kafka-console-producer --bootstrap-server localhost:9092 --topic test
```

**解決方案**:
- 檢查 Kafka 服務狀態
- 驗證 broker 地址
- 檢查主題配置

#### 4. 服務發現問題
**症狀**: 服務無法相互發現
**排查步驟**:
```bash
# 檢查 Consul 狀態
curl http://localhost:8500/v1/status/leader

# 檢查服務註冊
curl http://localhost:8500/v1/agent/services

# 檢查服務目錄
curl http://localhost:8500/v1/catalog/services
```

**解決方案**:
- 檢查 Consul 服務狀態
- 驗證服務註冊配置
- 檢查網路連接

### 性能問題排查

#### 1. 高 CPU 使用率
**排查步驟**:
```bash
# 查看 CPU 使用情況
docker stats

# 分析 CPU 使用熱點
docker exec <container_id> top

# 檢查 Prometheus 指標
curl http://localhost:9090/api/v1/query?query=rate(process_cpu_seconds_total[5m])
```

#### 2. 高記憶體使用率
**排查步驟**:
```bash
# 查看記憶體使用情況
docker stats

# 檢查記憶體洩漏
docker exec <container_id> cat /proc/meminfo

# 分析記憶體使用
curl http://localhost:9090/api/v1/query?query=process_resident_memory_bytes
```

#### 3. 網路延遲問題
**排查步驟**:
```bash
# 測試網路延遲
ping <service_host>

# 檢查網路連接
netstat -an | grep ESTABLISHED

# 分析網路指標
curl http://localhost:9090/api/v1/query?query=rate(network_receive_bytes_total[5m])
```

## 性能優化

### 系統優化

#### 1. 資料庫優化
```sql
-- 添加索引
CREATE INDEX idx_messages_created_at ON messages(created_at);
CREATE INDEX idx_messages_from_to ON messages(from_id, to_id);
CREATE INDEX idx_offline_msgs_user_id ON offline_msgs(user_id);

-- 分區表（大量數據時）
ALTER TABLE messages PARTITION BY RANGE (YEAR(created_at)) (
    PARTITION p2023 VALUES LESS THAN (2024),
    PARTITION p2024 VALUES LESS THAN (2025)
);
```

#### 2. Kafka 優化
```properties
# 增加分區數
num.partitions=3

# 優化批次大小
batch.size=16384
linger.ms=5

# 優化壓縮
compression.type=snappy
```

#### 3. 應用優化
```cpp
// 連接池配置
const int DB_POOL_SIZE = 10;
const int GRPC_POOL_SIZE = 5;

// 快取配置
const int CACHE_SIZE = 1000;
const int CACHE_TTL = 300; // 5 minutes
```

### 擴展策略

#### 1. 水平擴展
```bash
# 擴展 Gateway
kubectl scale deployment gateway-deployment --replicas=3 -n chat-system

# 擴展 Message Service
kubectl scale deployment message-service-deployment --replicas=5 -n chat-system
```

#### 2. 垂直擴展
```yaml
# 增加資源限制
resources:
  requests:
    memory: "512Mi"
    cpu: "500m"
  limits:
    memory: "1Gi"
    cpu: "1000m"
```

## 備份與恢復

### 資料庫備份
```bash
# 全量備份
docker exec chat-mariadb mysqldump -u root -p --all-databases > full_backup_$(date +%Y%m%d_%H%M%S).sql

# 增量備份
docker exec chat-mariadb mysqldump -u root -p --single-transaction --routines --triggers chatdb > incremental_backup_$(date +%Y%m%d_%H%M%S).sql

# 自動備份腳本
#!/bin/bash
BACKUP_DIR="/backup"
DATE=$(date +%Y%m%d_%H%M%S)
docker exec chat-mariadb mysqldump -u root -p chatdb > $BACKUP_DIR/chatdb_$DATE.sql
find $BACKUP_DIR -name "*.sql" -mtime +7 -delete
```

### 配置備份
```bash
# 備份配置
tar -czf config_backup_$(date +%Y%m%d_%H%M%S).tar.gz \
    deploy/ \
    docker-compose.enterprise.yml \
    Dockerfile.*

# 備份 Kubernetes 配置
kubectl get all -n chat-system -o yaml > k8s_backup_$(date +%Y%m%d_%H%M%S).yaml
```

### 恢復流程
```bash
# 停止服務
docker-compose -f docker-compose.enterprise.yml down

# 恢復資料庫
docker exec -i chat-mariadb mysql -u root -p chatdb < backup_file.sql

# 啟動服務
docker-compose -f docker-compose.enterprise.yml up -d

# 驗證恢復
./deploy/monitor.sh health
```

## 安全配置

### 網路安全
```yaml
# 網路策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: chat-network-policy
  namespace: chat-system
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: chat-system
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: chat-system
```

### 認證與授權
```yaml
# JWT 配置
apiVersion: v1
kind: Secret
metadata:
  name: jwt-secret
  namespace: chat-system
type: Opaque
data:
  secret: <base64-encoded-secret>
```

### 資料加密
```bash
# 資料庫加密
docker run -d \
  --name chat-mariadb \
  -e MYSQL_ROOT_PASSWORD=password \
  -e MYSQL_ENCRYPTION_KEY=your-encryption-key \
  mariadb:10.11
```

## 擴展指南

### 添加新服務
1. 創建新的 gRPC 服務定義
2. 實現服務邏輯
3. 添加 Docker 配置
4. 更新 Kubernetes 部署
5. 配置監控和追蹤

### 添加新功能
1. 更新 protobuf 定義
2. 實現業務邏輯
3. 添加資料庫遷移
4. 更新 API 文檔
5. 添加測試用例

### 性能調優
1. 分析性能瓶頸
2. 優化資料庫查詢
3. 調整快取策略
4. 優化網路配置
5. 監控性能指標

---

## 緊急聯繫

### 關鍵命令
```bash
# 緊急重啟
docker-compose -f docker-compose.enterprise.yml restart

# 緊急停止
docker-compose -f docker-compose.enterprise.yml down

# 緊急恢復
docker-compose -f docker-compose.enterprise.yml up -d

# 查看所有日誌
docker-compose -f docker-compose.enterprise.yml logs
```

### 監控工具
- **系統監控**: `./deploy/monitor.sh overview`
- **健康檢查**: `./deploy/monitor.sh health`
- **日誌查看**: `./deploy/monitor.sh logs`
- **故障排查**: `./deploy/monitor.sh troubleshoot`
