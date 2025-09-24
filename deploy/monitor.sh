#!/bin/bash

# 企業級聊天系統監控腳本
# 提供完整的系統監控、日誌查看、故障排查功能

set -e

# 顏色定義
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

# 配置
NAMESPACE="chat-system"
COMPOSE_FILE="docker-compose.enterprise.yml"

# 日誌函數
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_header() {
    echo -e "${PURPLE}=== $1 ===${NC}"
}

# 檢查部署類型
check_deployment_type() {
    if command -v kubectl &> /dev/null && kubectl get namespace $NAMESPACE &> /dev/null; then
        DEPLOY_TYPE="k8s"
        log_info "檢測到 Kubernetes 部署"
    elif docker-compose -f $COMPOSE_FILE ps &> /dev/null; then
        DEPLOY_TYPE="docker"
        log_info "檢測到 Docker Compose 部署"
    else
        log_error "未檢測到有效的部署"
        exit 1
    fi
}

# 系統狀態概覽
show_system_overview() {
    log_header "系統狀態概覽"
    
    if [ "$DEPLOY_TYPE" = "k8s" ]; then
        echo "📊 Kubernetes 部署狀態："
        kubectl get pods -n $NAMESPACE
        echo ""
        echo "🌐 服務狀態："
        kubectl get services -n $NAMESPACE
        echo ""
        echo "📈 資源使用："
        kubectl top pods -n $NAMESPACE 2>/dev/null || echo "需要安裝 metrics-server"
    else
        echo "📊 Docker Compose 部署狀態："
        docker-compose -f $COMPOSE_FILE ps
        echo ""
        echo "💾 資源使用："
        docker stats --no-stream --format "table {{.Container}}\t{{.CPUPerc}}\t{{.MemUsage}}\t{{.NetIO}}\t{{.BlockIO}}"
    fi
}

# 健康檢查
health_check() {
    log_header "健康檢查"
    
    local services=("gateway" "user-service" "social-service" "message-service")
    local ports=("8080" "8081" "8082" "8083")
    
    for i in "${!services[@]}"; do
        local service="${services[$i]}"
        local port="${ports[$i]}"
        
        echo -n "檢查 $service (:$port)... "
        
        if curl -f -s http://localhost:$port/health > /dev/null; then
            log_success "✅ 健康"
        else
            log_error "❌ 不健康"
        fi
    done
    
    echo ""
    echo "🔍 基礎設施健康檢查："
    
    # Prometheus
    echo -n "Prometheus... "
    if curl -f -s http://localhost:9090/-/healthy > /dev/null; then
        log_success "✅ 健康"
    else
        log_error "❌ 不健康"
    fi
    
    # Grafana
    echo -n "Grafana... "
    if curl -f -s http://localhost:3000/api/health > /dev/null; then
        log_success "✅ 健康"
    else
        log_error "❌ 不健康"
    fi
    
    # Jaeger
    echo -n "Jaeger... "
    if curl -f -s http://localhost:16686/ > /dev/null; then
        log_success "✅ 健康"
    else
        log_error "❌ 不健康"
    fi
    
    # Consul
    echo -n "Consul... "
    if curl -f -s http://localhost:8500/v1/status/leader > /dev/null; then
        log_success "✅ 健康"
    else
        log_error "❌ 不健康"
    fi
}

# 查看日誌
view_logs() {
    local service="$1"
    local lines="${2:-100}"
    
    log_header "查看 $service 日誌（最近 $lines 行）"
    
    if [ "$DEPLOY_TYPE" = "k8s" ]; then
        if [ "$service" = "all" ]; then
            kubectl logs -l app=gateway -n $NAMESPACE --tail=$lines
            kubectl logs -l app=user-service -n $NAMESPACE --tail=$lines
            kubectl logs -l app=social-service -n $NAMESPACE --tail=$lines
            kubectl logs -l app=message-service -n $NAMESPACE --tail=$lines
        else
            kubectl logs -l app=$service -n $NAMESPACE --tail=$lines
        fi
    else
        if [ "$service" = "all" ]; then
            docker-compose -f $COMPOSE_FILE logs --tail=$lines
        else
            docker-compose -f $COMPOSE_FILE logs --tail=$lines $service
        fi
    fi
}

# 實時日誌
follow_logs() {
    local service="$1"
    
    log_header "實時查看 $service 日誌"
    
    if [ "$DEPLOY_TYPE" = "k8s" ]; then
        kubectl logs -l app=$service -n $NAMESPACE -f
    else
        docker-compose -f $COMPOSE_FILE logs -f $service
    fi
}

# 性能指標
show_metrics() {
    log_header "性能指標"
    
    echo "📊 Prometheus 指標端點："
    echo "  - Gateway: http://localhost:8080/metrics"
    echo "  - User Service: http://localhost:8081/metrics"
    echo "  - Social Service: http://localhost:8082/metrics"
    echo "  - Message Service: http://localhost:8083/metrics"
    echo ""
    
    echo "🔍 關鍵指標查詢："
    echo "  - gRPC 調用率: rate(grpc_calls_total[5m])"
    echo "  - 活躍連接數: active_connections"
    echo "  - 在線用戶數: online_users"
    echo "  - 資料庫查詢率: rate(database_queries_total[5m])"
    echo ""
    
    # 嘗試獲取一些關鍵指標
    if command -v curl &> /dev/null; then
        echo "📈 當前指標值："
        
        # 活躍連接數
        local connections=$(curl -s http://localhost:8080/metrics | grep "active_connections" | grep -v "#" | awk '{print $2}' | head -1)
        echo "  - 活躍連接數: ${connections:-N/A}"
        
        # 在線用戶數
        local users=$(curl -s http://localhost:8080/metrics | grep "online_users" | grep -v "#" | awk '{print $2}' | head -1)
        echo "  - 在線用戶數: ${users:-N/A}"
    fi
}

# 故障排查
troubleshoot() {
    log_header "故障排查指南"
    
    echo "🔧 常見問題排查："
    echo ""
    
    echo "1. 服務無法啟動："
    echo "   - 檢查端口是否被佔用: netstat -tulpn | grep :7000"
    echo "   - 檢查 Docker 容器狀態: docker ps -a"
    echo "   - 查看容器日誌: docker logs <container_id>"
    echo ""
    
    echo "2. 資料庫連接問題："
    echo "   - 檢查 MariaDB 狀態: docker exec -it chat-mariadb mysql -u root -p"
    echo "   - 檢查資料庫配置: echo \$DB_HOST \$DB_PORT \$DB_NAME"
    echo "   - 測試資料庫連接: telnet \$DB_HOST \$DB_PORT"
    echo ""
    
    echo "3. Kafka 連接問題："
    echo "   - 檢查 Kafka 狀態: docker exec -it chat-kafka kafka-topics --list --bootstrap-server localhost:9092"
    echo "   - 檢查 Kafka 配置: echo \$KAFKA_BROKERS"
    echo ""
    
    echo "4. Consul 服務發現問題："
    echo "   - 檢查 Consul 狀態: curl http://localhost:8500/v1/agent/services"
    echo "   - 檢查服務註冊: curl http://localhost:8500/v1/catalog/services"
    echo ""
    
    echo "5. 性能問題："
    echo "   - 檢查 CPU/記憶體使用: docker stats"
    echo "   - 檢查網路延遲: ping <service_host>"
    echo "   - 查看 Prometheus 指標: http://localhost:9090"
    echo ""
    
    echo "6. 追蹤問題："
    echo "   - 查看 Jaeger 追蹤: http://localhost:16686"
    echo "   - 檢查追蹤配置: echo \$JAEGER_ENDPOINT"
    echo ""
    
    echo "📞 緊急恢復命令："
    echo "  - 重啟所有服務: docker-compose -f $COMPOSE_FILE restart"
    echo "  - 清理並重建: docker-compose -f $COMPOSE_FILE down && docker-compose -f $COMPOSE_FILE up -d"
    echo "  - 查看所有日誌: docker-compose -f $COMPOSE_FILE logs"
}

# 備份與恢復
backup_restore() {
    log_header "備份與恢復"
    
    echo "💾 資料庫備份："
    echo "  - 備份: docker exec chat-mariadb mysqldump -u root -p chatdb > backup_$(date +%Y%m%d_%H%M%S).sql"
    echo "  - 恢復: docker exec -i chat-mariadb mysql -u root -p chatdb < backup_file.sql"
    echo ""
    
    echo "📁 配置備份："
    echo "  - 備份配置: tar -czf config_backup_$(date +%Y%m%d_%H%M%S).tar.gz deploy/ docker-compose.enterprise.yml"
    echo ""
    
    echo "🔄 快速恢復："
    echo "  - 停止服務: docker-compose -f $COMPOSE_FILE down"
    echo "  - 恢復資料: docker exec -i chat-mariadb mysql -u root -p chatdb < backup_file.sql"
    echo "  - 啟動服務: docker-compose -f $COMPOSE_FILE up -d"
}

# 擴縮容
scale_services() {
    local service="$1"
    local replicas="$2"
    
    if [ -z "$service" ] || [ -z "$replicas" ]; then
        log_error "用法: scale <service> <replicas>"
        echo "可用服務: gateway, user-service, social-service, message-service"
        return 1
    fi
    
    log_header "擴縮容 $service 到 $replicas 個實例"
    
    if [ "$DEPLOY_TYPE" = "k8s" ]; then
        kubectl scale deployment $service-deployment --replicas=$replicas -n $NAMESPACE
        log_success "Kubernetes 擴縮容完成"
    else
        log_warning "Docker Compose 不支援動態擴縮容，請手動修改 docker-compose.enterprise.yml"
    fi
}

# 主函數
main() {
    echo "🔍 企業級聊天系統監控工具"
    echo "================================"
    
    # 檢查部署類型
    check_deployment_type
    
    # 解析命令
    case "${1:-overview}" in
        "overview"|"status")
            show_system_overview
            ;;
        "health")
            health_check
            ;;
        "logs")
            view_logs "${2:-all}" "${3:-100}"
            ;;
        "follow"|"tail")
            follow_logs "${2:-gateway}"
            ;;
        "metrics")
            show_metrics
            ;;
        "troubleshoot"|"debug")
            troubleshoot
            ;;
        "backup")
            backup_restore
            ;;
        "scale")
            scale_services "$2" "$3"
            ;;
        "help")
            echo "用法: $0 [命令] [參數]"
            echo ""
            echo "命令："
            echo "  overview, status    顯示系統狀態概覽"
            echo "  health             執行健康檢查"
            echo "  logs [service] [lines] 查看日誌（預設：所有服務，100行）"
            echo "  follow [service]   實時查看日誌（預設：gateway）"
            echo "  metrics            顯示性能指標"
            echo "  troubleshoot       故障排查指南"
            echo "  backup             備份與恢復指南"
            echo "  scale <service> <replicas> 擴縮容服務"
            echo "  help               顯示此幫助"
            echo ""
            echo "範例："
            echo "  $0 overview"
            echo "  $0 logs gateway 50"
            echo "  $0 follow message-service"
            echo "  $0 scale gateway 3"
            ;;
        *)
            log_error "未知命令: $1"
            echo "使用 '$0 help' 查看可用命令"
            exit 1
            ;;
    esac
}

# 執行主函數
main "$@"
