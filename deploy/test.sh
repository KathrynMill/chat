#!/bin/bash

# 企業級聊天系統測試腳本
# 提供完整的端到端測試功能

set -e

# 顏色定義
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 配置
GATEWAY_HOST="localhost"
GATEWAY_PORT="7000"
TEST_USER1="alice"
TEST_USER2="bob"
TEST_PASSWORD="password"

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

# 發送 JSON 請求
send_request() {
    local json="$1"
    local description="$2"
    
    log_info "測試: $description"
    echo "請求: $json"
    
    local response=$(echo "$json" | nc -w 5 $GATEWAY_HOST $GATEWAY_PORT 2>/dev/null || echo "ERROR")
    
    if [ "$response" = "ERROR" ]; then
        log_error "連接失敗或超時"
        return 1
    fi
    
    echo "響應: $response"
    echo ""
    return 0
}

# 健康檢查測試
test_health() {
    log_info "執行健康檢查測試..."
    
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
}

# 用戶註冊測試
test_user_registration() {
    log_info "執行用戶註冊測試..."
    
    # 註冊用戶1
    send_request '{"msgid":1,"username":"'$TEST_USER1'","password":"'$TEST_PASSWORD'","email":"alice@example.com"}' "註冊用戶 $TEST_USER1"
    
    # 註冊用戶2
    send_request '{"msgid":1,"username":"'$TEST_USER2'","password":"'$TEST_PASSWORD'","email":"bob@example.com"}' "註冊用戶 $TEST_USER2"
}

# 用戶登入測試
test_user_login() {
    log_info "執行用戶登入測試..."
    
    # 登入用戶1
    send_request '{"msgid":2,"username":"'$TEST_USER1'","password":"'$TEST_PASSWORD'"}' "登入用戶 $TEST_USER1"
    
    # 登入用戶2
    send_request '{"msgid":2,"username":"'$TEST_USER2'","password":"'$TEST_PASSWORD'"}' "登入用戶 $TEST_USER2"
}

# 好友功能測試
test_friend_operations() {
    log_info "執行好友功能測試..."
    
    # 添加好友
    send_request '{"msgid":2001,"user_id":1,"friend_id":2}' "添加好友關係"
    
    # 列出好友
    send_request '{"msgid":2002,"user_id":1}' "列出用戶1的好友"
}

# 群組功能測試
test_group_operations() {
    log_info "執行群組功能測試..."
    
    # 創建群組
    send_request '{"msgid":2003,"owner_id":1,"name":"Test Group","description":"A test group"}' "創建測試群組"
    
    # 添加群組成員
    send_request '{"msgid":2005,"group_id":1,"user_id":2}' "添加用戶2到群組"
    
    # 列出群組
    send_request '{"msgid":2004,"user_id":1}' "列出用戶1的群組"
}

# 訊息功能測試
test_message_operations() {
    log_info "執行訊息功能測試..."
    
    # 私聊訊息
    send_request '{"msgid":1001,"from_id":1,"to_id":2,"content":"Hello Bob!","timestamp_ms":'$(date +%s000)'}' "發送私聊訊息"
    
    # 群聊訊息
    send_request '{"msgid":1003,"from_id":1,"group_id":1,"content":"Hello Group!","timestamp_ms":'$(date +%s000)'}' "發送群聊訊息"
    
    # 查詢訊息
    send_request '{"msgid":1005,"user_id":1,"scope":"private","since_ms":0,"limit":10}' "查詢私聊訊息"
    
    send_request '{"msgid":1005,"user_id":1,"scope":"group","since_ms":0,"limit":10}' "查詢群聊訊息"
}

# 性能測試
test_performance() {
    log_info "執行性能測試..."
    
    local start_time=$(date +%s)
    local success_count=0
    local total_count=100
    
    for i in $(seq 1 $total_count); do
        local response=$(echo '{"msgid":1001,"from_id":1,"to_id":2,"content":"Test message '$i'","timestamp_ms":'$(date +%s000)'}' | nc -w 1 $GATEWAY_HOST $GATEWAY_PORT 2>/dev/null || echo "ERROR")
        
        if [ "$response" != "ERROR" ]; then
            ((success_count++))
        fi
        
        if [ $((i % 10)) -eq 0 ]; then
            echo -n "."
        fi
    done
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    local qps=$((total_count / duration))
    
    echo ""
    log_info "性能測試結果:"
    echo "  - 總請求數: $total_count"
    echo "  - 成功請求數: $success_count"
    echo "  - 成功率: $((success_count * 100 / total_count))%"
    echo "  - 總耗時: ${duration}s"
    echo "  - QPS: $qps"
    echo ""
}

# 監控指標測試
test_metrics() {
    log_info "檢查監控指標..."
    
    local metrics_endpoints=("8080" "8081" "8082" "8083")
    
    for port in "${metrics_endpoints[@]}"; do
        echo -n "檢查指標端點 :$port... "
        
        if curl -f -s http://localhost:$port/metrics > /dev/null; then
            log_success "✅ 可用"
        else
            log_error "❌ 不可用"
        fi
    done
    echo ""
}

# 追蹤測試
test_tracing() {
    log_info "檢查追蹤功能..."
    
    echo -n "檢查 Jaeger UI... "
    if curl -f -s http://localhost:16686 > /dev/null; then
        log_success "✅ 可用"
    else
        log_error "❌ 不可用"
    fi
    
    echo -n "檢查 Prometheus... "
    if curl -f -s http://localhost:9090 > /dev/null; then
        log_success "✅ 可用"
    else
        log_error "❌ 不可用"
    fi
    
    echo -n "檢查 Grafana... "
    if curl -f -s http://localhost:3000 > /dev/null; then
        log_success "✅ 可用"
    else
        log_error "❌ 不可用"
    fi
    echo ""
}

# 完整測試套件
run_full_test() {
    log_info "執行完整測試套件..."
    echo "=================================="
    
    test_health
    test_user_registration
    test_user_login
    test_friend_operations
    test_group_operations
    test_message_operations
    test_metrics
    test_tracing
    
    log_success "完整測試套件執行完成！"
}

# 快速測試
run_quick_test() {
    log_info "執行快速測試..."
    echo "=================================="
    
    test_health
    test_metrics
    test_tracing
    
    log_success "快速測試執行完成！"
}

# 壓力測試
run_stress_test() {
    log_info "執行壓力測試..."
    echo "=================================="
    
    test_performance
    
    log_success "壓力測試執行完成！"
}

# 主函數
main() {
    echo "🧪 企業級聊天系統測試工具"
    echo "================================"
    
    # 檢查依賴
    if ! command -v nc &> /dev/null; then
        log_error "netcat (nc) 未安裝，請先安裝"
        exit 1
    fi
    
    if ! command -v curl &> /dev/null; then
        log_error "curl 未安裝，請先安裝"
        exit 1
    fi
    
    # 解析命令
    case "${1:-full}" in
        "health")
            test_health
            ;;
        "user")
            test_user_registration
            test_user_login
            ;;
        "friend")
            test_friend_operations
            ;;
        "group")
            test_group_operations
            ;;
        "message")
            test_message_operations
            ;;
        "performance"|"stress")
            run_stress_test
            ;;
        "metrics")
            test_metrics
            ;;
        "tracing")
            test_tracing
            ;;
        "quick")
            run_quick_test
            ;;
        "full")
            run_full_test
            ;;
        "help")
            echo "用法: $0 [測試類型]"
            echo ""
            echo "測試類型："
            echo "  health       健康檢查測試"
            echo "  user         用戶註冊/登入測試"
            echo "  friend       好友功能測試"
            echo "  group        群組功能測試"
            echo "  message      訊息功能測試"
            echo "  performance  性能測試"
            echo "  metrics      監控指標測試"
            echo "  tracing      追蹤功能測試"
            echo "  quick        快速測試"
            echo "  full         完整測試套件（預設）"
            echo "  help         顯示此幫助"
            echo ""
            echo "範例："
            echo "  $0 health"
            echo "  $0 performance"
            echo "  $0 full"
            ;;
        *)
            log_error "未知測試類型: $1"
            echo "使用 '$0 help' 查看可用測試類型"
            exit 1
            ;;
    esac
}

# 執行主函數
main "$@"
