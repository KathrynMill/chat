#!/bin/bash

# 企業级聊天系统测试腳本
# 提供完整的端到端测试功能

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

# 日誌函数
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

# 发送 JSON 請求
send_request() {
    local json="$1"
    local description="$2"
    
    log_info "测试: $description"
    echo "請求: $json"
    
    local response=$(echo "$json" | nc -w 5 $GATEWAY_HOST $GATEWAY_PORT 2>/dev/null || echo "ERROR")
    
    if [ "$response" = "ERROR" ]; then
        log_error "连接失败或超時"
        return 1
    fi
    
    echo "響應: $response"
    echo ""
    return 0
}

# 健康检查测试
test_health() {
    log_info "执行健康检查测试..."
    
    local services=("gateway" "user-service" "social-service" "message-service")
    local ports=("8080" "8081" "8082" "8083")
    
    for i in "${!services[@]}"; do
        local service="${services[$i]}"
        local port="${ports[$i]}"
        
        echo -n "检查 $service (:$port)... "
        
        if curl -f -s http://localhost:$port/health > /dev/null; then
            log_success "✅ 健康"
        else
            log_error "❌ 不健康"
        fi
    done
    echo ""
}

# 用户註冊测试
test_user_registration() {
    log_info "执行用户註冊测试..."
    
    # 註冊用户1
    send_request '{"msgid":1,"username":"'$TEST_USER1'","password":"'$TEST_PASSWORD'","email":"alice@example.com"}' "註冊用户 $TEST_USER1"
    
    # 註冊用户2
    send_request '{"msgid":1,"username":"'$TEST_USER2'","password":"'$TEST_PASSWORD'","email":"bob@example.com"}' "註冊用户 $TEST_USER2"
}

# 用户登入测试
test_user_login() {
    log_info "执行用户登入测试..."
    
    # 登入用户1
    send_request '{"msgid":2,"username":"'$TEST_USER1'","password":"'$TEST_PASSWORD'"}' "登入用户 $TEST_USER1"
    
    # 登入用户2
    send_request '{"msgid":2,"username":"'$TEST_USER2'","password":"'$TEST_PASSWORD'"}' "登入用户 $TEST_USER2"
}

# 好友功能测试
test_friend_operations() {
    log_info "执行好友功能测试..."
    
    # 添加好友
    send_request '{"msgid":2001,"user_id":1,"friend_id":2}' "添加好友关系"
    
    # 列出好友
    send_request '{"msgid":2002,"user_id":1}' "列出用户1的好友"
}

# 群组功能测试
test_group_operations() {
    log_info "执行群组功能测试..."
    
    # 创建群组
    send_request '{"msgid":2003,"owner_id":1,"name":"Test Group","description":"A test group"}' "创建测试群组"
    
    # 添加群组成員
    send_request '{"msgid":2005,"group_id":1,"user_id":2}' "添加用户2到群组"
    
    # 列出群组
    send_request '{"msgid":2004,"user_id":1}' "列出用户1的群组"
}

# 讯息功能测试
test_message_operations() {
    log_info "执行讯息功能测试..."
    
    # 私聊讯息
    send_request '{"msgid":1001,"from_id":1,"to_id":2,"content":"Hello Bob!","timestamp_ms":'$(date +%s000)'}' "发送私聊讯息"
    
    # 群聊讯息
    send_request '{"msgid":1003,"from_id":1,"group_id":1,"content":"Hello Group!","timestamp_ms":'$(date +%s000)'}' "发送群聊讯息"
    
    # 查询讯息
    send_request '{"msgid":1005,"user_id":1,"scope":"private","since_ms":0,"limit":10}' "查询私聊讯息"
    
    send_request '{"msgid":1005,"user_id":1,"scope":"group","since_ms":0,"limit":10}' "查询群聊讯息"
}

# 性能测试
test_performance() {
    log_info "执行性能测试..."
    
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
    log_info "性能测试結果:"
    echo "  - 總請求数: $total_count"
    echo "  - 成功請求数: $success_count"
    echo "  - 成功率: $((success_count * 100 / total_count))%"
    echo "  - 總耗時: ${duration}s"
    echo "  - QPS: $qps"
    echo ""
}

# 监控指标测试
test_metrics() {
    log_info "检查监控指标..."
    
    local metrics_endpoints=("8080" "8081" "8082" "8083")
    
    for port in "${metrics_endpoints[@]}"; do
        echo -n "检查指标端點 :$port... "
        
        if curl -f -s http://localhost:$port/metrics > /dev/null; then
            log_success "✅ 可用"
        else
            log_error "❌ 不可用"
        fi
    done
    echo ""
}

# 追蹤测试
test_tracing() {
    log_info "检查追蹤功能..."
    
    echo -n "检查 Jaeger UI... "
    if curl -f -s http://localhost:16686 > /dev/null; then
        log_success "✅ 可用"
    else
        log_error "❌ 不可用"
    fi
    
    echo -n "检查 Prometheus... "
    if curl -f -s http://localhost:9090 > /dev/null; then
        log_success "✅ 可用"
    else
        log_error "❌ 不可用"
    fi
    
    echo -n "检查 Grafana... "
    if curl -f -s http://localhost:3000 > /dev/null; then
        log_success "✅ 可用"
    else
        log_error "❌ 不可用"
    fi
    echo ""
}

# 完整测试套件
run_full_test() {
    log_info "执行完整测试套件..."
    echo "=================================="
    
    test_health
    test_user_registration
    test_user_login
    test_friend_operations
    test_group_operations
    test_message_operations
    test_metrics
    test_tracing
    
    log_success "完整测试套件执行完成！"
}

# 快速测试
run_quick_test() {
    log_info "执行快速测试..."
    echo "=================================="
    
    test_health
    test_metrics
    test_tracing
    
    log_success "快速测试执行完成！"
}

# 壓力测试
run_stress_test() {
    log_info "执行壓力测试..."
    echo "=================================="
    
    test_performance
    
    log_success "壓力测试执行完成！"
}

# 主函数
main() {
    echo "🧪 企業级聊天系统测试工具"
    echo "================================"
    
    # 检查依賴
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
            echo "用法: $0 [测试類型]"
            echo ""
            echo "测试類型："
            echo "  health       健康检查测试"
            echo "  user         用户註冊/登入测试"
            echo "  friend       好友功能测试"
            echo "  group        群组功能测试"
            echo "  message      讯息功能测试"
            echo "  performance  性能测试"
            echo "  metrics      监控指标测试"
            echo "  tracing      追蹤功能测试"
            echo "  quick        快速测试"
            echo "  full         完整测试套件（預设）"
            echo "  help         顯示此幫助"
            echo ""
            echo "范例："
            echo "  $0 health"
            echo "  $0 performance"
            echo "  $0 full"
            ;;
        *)
            log_error "未知测试類型: $1"
            echo "使用 '$0 help' 查看可用测试類型"
            exit 1
            ;;
    esac
}

# 执行主函数
main "$@"
