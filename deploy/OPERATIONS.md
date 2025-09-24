# 企業级聊天系统运維指南

## 目录
- [部署架构](#部署架构)
- [监控與告警](#监控與告警)
- [故障排查](#故障排查)
- [性能优化](#性能优化)
- [备份與恢复](#备份與恢复)
- [安全配置](#安全配置)
- [扩展指南](#扩展指南)

## 部署架构

### 系统组件
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
| 服务 | 端口 | 用途 |
|------|------|------|
| Gateway | 7000 | 客户端连接 |
| Gateway | 8080 | 健康检查/指标 |
| User Service | 60051 | gRPC |
| User Service | 8081 | 健康检查/指标 |
| Social Service | 60052 | gRPC |
| Social Service | 8082 | 健康检查/指标 |
| Message Service | 60053 | gRPC |
| Message Service | 8083 | 健康检查/指标 |
| MariaDB | 3306 | 资料庫 |
| Kafka | 9092 | 讯息隊列 |
| Consul | 8500 | 服务发现 |
| Prometheus | 9090 | 指标收集 |
| Grafana | 3000 | 监控面板 |
| Jaeger | 16686 | 追蹤視覺化 |

## 监控與告警

### 关键指标

#### 業务指标
- **在線用户数** (`online_users`): 當前在線用户数量
- **活躍连接数** (`active_connections`): 當前活躍连接数量
- **讯息吞吐量** (`kafka_messages_total`): 每秒处理的讯息数量

#### 技术指标
- **gRPC 调用率** (`grpc_calls_total`): 每秒 gRPC 调用次数
- **gRPC 延遲** (`grpc_call_duration_seconds`): gRPC 调用延遲分佈
- **HTTP 請求率** (`http_requests_total`): 每秒 HTTP 請求次数
- **资料庫查询率** (`database_queries_total`): 每秒资料庫查询次数

#### 系统指标
- **CPU 使用率**: 各服务 CPU 使用情況
- **記憶體使用率**: 各服务記憶體使用情況
- **网路 I/O**: 网路流量统計
- **磁碟 I/O**: 磁碟读写统計

### 告警规则

#### 关键告警
```yaml
# 服务不可用
- alert: ServiceDown
  expr: up{job=~"gateway|user-service|social-service|message-service"} == 0
  for: 1m
  labels:
    severity: critical
  annotations:
    summary: "服务 {{ $labels.job }} 不可用"

# 高错误率
- alert: HighErrorRate
  expr: rate(grpc_calls_total{status="error"}[5m]) / rate(grpc_calls_total[5m]) > 0.1
  for: 2m
  labels:
    severity: warning
  annotations:
    summary: "服务 {{ $labels.service }} 错误率过高"

# 高延遲
- alert: HighLatency
  expr: histogram_quantile(0.95, rate(grpc_call_duration_seconds_bucket[5m])) > 1
  for: 3m
  labels:
    severity: warning
  annotations:
    summary: "服务 {{ $labels.service }} 延遲过高"
```

### 监控面板

#### Grafana 儀表板
- **系统概覽**: 服务健康狀態、资源使用率
- **性能监控**: gRPC/HTTP 性能指标
- **業务监控**: 用户活动、讯息统計
- **基础设施**: 资料庫、Kafka、Consul 狀態

## 故障排查

### 常見問題

#### 1. 服务启动失败
**症狀**: 服务無法启动或立即退出
**排查步骤**:
```bash
# 检查端口佔用
netstat -tulpn | grep :7000

# 检查 Docker 容器狀態
docker ps -a

# 查看容器日誌
docker logs <container_id>

# 检查环境变数
docker exec <container_id> env
```

**解決方案**:
- 釋放被佔用的端口
- 检查环境变数配置
- 验证依賴服务狀態

#### 2. 资料庫连接問題
**症狀**: 服务無法连接资料庫
**排查步骤**:
```bash
# 检查 MariaDB 狀態
docker exec -it chat-mariadb mysql -u root -p

# 测试资料庫连接
telnet $DB_HOST $DB_PORT

# 检查资料庫配置
echo $DB_HOST $DB_PORT $DB_NAME
```

**解決方案**:
- 检查资料庫服务狀態
- 验证连接參数
- 检查网路连接

#### 3. Kafka 连接問題
**症狀**: 讯息無法发送到 Kafka
**排查步骤**:
```bash
# 检查 Kafka 狀態
docker exec -it chat-kafka kafka-topics --list --bootstrap-server localhost:9092

# 检查 Kafka 配置
echo $KAFKA_BROKERS

# 测试 Kafka 连接
docker exec -it chat-kafka kafka-console-producer --bootstrap-server localhost:9092 --topic test
```

**解決方案**:
- 检查 Kafka 服务狀態
- 验证 broker 地址
- 检查主題配置

#### 4. 服务发现問題
**症狀**: 服务無法相互发现
**排查步骤**:
```bash
# 检查 Consul 狀態
curl http://localhost:8500/v1/status/leader

# 检查服务註冊
curl http://localhost:8500/v1/agent/services

# 检查服务目录
curl http://localhost:8500/v1/catalog/services
```

**解決方案**:
- 检查 Consul 服务狀態
- 验证服务註冊配置
- 检查网路连接

### 性能問題排查

#### 1. 高 CPU 使用率
**排查步骤**:
```bash
# 查看 CPU 使用情況
docker stats

# 分析 CPU 使用熱點
docker exec <container_id> top

# 检查 Prometheus 指标
curl http://localhost:9090/api/v1/query?query=rate(process_cpu_seconds_total[5m])
```

#### 2. 高記憶體使用率
**排查步骤**:
```bash
# 查看記憶體使用情況
docker stats

# 检查記憶體洩漏
docker exec <container_id> cat /proc/meminfo

# 分析記憶體使用
curl http://localhost:9090/api/v1/query?query=process_resident_memory_bytes
```

#### 3. 网路延遲問題
**排查步骤**:
```bash
# 测试网路延遲
ping <service_host>

# 检查网路连接
netstat -an | grep ESTABLISHED

# 分析网路指标
curl http://localhost:9090/api/v1/query?query=rate(network_receive_bytes_total[5m])
```

## 性能优化

### 系统优化

#### 1. 资料庫优化
```sql
-- 添加索引
CREATE INDEX idx_messages_created_at ON messages(created_at);
CREATE INDEX idx_messages_from_to ON messages(from_id, to_id);
CREATE INDEX idx_offline_msgs_user_id ON offline_msgs(user_id);

-- 分區表（大量数据時）
ALTER TABLE messages PARTITION BY RANGE (YEAR(created_at)) (
    PARTITION p2023 VALUES LESS THAN (2024),
    PARTITION p2024 VALUES LESS THAN (2025)
);
```

#### 2. Kafka 优化
```properties
# 增加分區数
num.partitions=3

# 优化批次大小
batch.size=16384
linger.ms=5

# 优化壓缩
compression.type=snappy
```

#### 3. 應用优化
```cpp
// 连接池配置
const int DB_POOL_SIZE = 10;
const int GRPC_POOL_SIZE = 5;

// 快取配置
const int CACHE_SIZE = 1000;
const int CACHE_TTL = 300; // 5 minutes
```

### 扩展策略

#### 1. 水平扩展
```bash
# 扩展 Gateway
kubectl scale deployment gateway-deployment --replicas=3 -n chat-system

# 扩展 Message Service
kubectl scale deployment message-service-deployment --replicas=5 -n chat-system
```

#### 2. 垂直扩展
```yaml
# 增加资源限制
resources:
  requests:
    memory: "512Mi"
    cpu: "500m"
  limits:
    memory: "1Gi"
    cpu: "1000m"
```

## 备份與恢复

### 资料庫备份
```bash
# 全量备份
docker exec chat-mariadb mysqldump -u root -p --all-databases > full_backup_$(date +%Y%m%d_%H%M%S).sql

# 增量备份
docker exec chat-mariadb mysqldump -u root -p --single-transaction --routines --triggers chatdb > incremental_backup_$(date +%Y%m%d_%H%M%S).sql

# 自动备份腳本
#!/bin/bash
BACKUP_DIR="/backup"
DATE=$(date +%Y%m%d_%H%M%S)
docker exec chat-mariadb mysqldump -u root -p chatdb > $BACKUP_DIR/chatdb_$DATE.sql
find $BACKUP_DIR -name "*.sql" -mtime +7 -delete
```

### 配置备份
```bash
# 备份配置
tar -czf config_backup_$(date +%Y%m%d_%H%M%S).tar.gz \
    deploy/ \
    docker-compose.enterprise.yml \
    Dockerfile.*

# 备份 Kubernetes 配置
kubectl get all -n chat-system -o yaml > k8s_backup_$(date +%Y%m%d_%H%M%S).yaml
```

### 恢复流程
```bash
# 停止服务
docker-compose -f docker-compose.enterprise.yml down

# 恢复资料庫
docker exec -i chat-mariadb mysql -u root -p chatdb < backup_file.sql

# 启动服务
docker-compose -f docker-compose.enterprise.yml up -d

# 验证恢复
./deploy/monitor.sh health
```

## 安全配置

### 网路安全
```yaml
# 网路策略
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

### 认证與授权
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

### 资料加密
```bash
# 资料庫加密
docker run -d \
  --name chat-mariadb \
  -e MYSQL_ROOT_PASSWORD=password \
  -e MYSQL_ENCRYPTION_KEY=your-encryption-key \
  mariadb:10.11
```

## 扩展指南

### 添加新服务
1. 创建新的 gRPC 服务定義
2. 实现服务邏輯
3. 添加 Docker 配置
4. 更新 Kubernetes 部署
5. 配置监控和追蹤

### 添加新功能
1. 更新 protobuf 定義
2. 实现業务邏輯
3. 添加资料庫遷移
4. 更新 API 文檔
5. 添加测试用例

### 性能调优
1. 分析性能瓶頸
2. 优化资料庫查询
3. 调整快取策略
4. 优化网路配置
5. 监控性能指标

---

## 緊急联系

### 关键命令
```bash
# 緊急重启
docker-compose -f docker-compose.enterprise.yml restart

# 緊急停止
docker-compose -f docker-compose.enterprise.yml down

# 緊急恢复
docker-compose -f docker-compose.enterprise.yml up -d

# 查看所有日誌
docker-compose -f docker-compose.enterprise.yml logs
```

### 监控工具
- **系统监控**: `./deploy/monitor.sh overview`
- **健康检查**: `./deploy/monitor.sh health`
- **日誌查看**: `./deploy/monitor.sh logs`
- **故障排查**: `./deploy/monitor.sh troubleshoot`
