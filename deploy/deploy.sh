#!/bin/bash

# 企業级聊天系统部署腳本
# 支援 Docker Compose 和 Kubernetes 部署

set -e

# 顏色定義
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# 检查依賴
check_dependencies() {
    log_info "检查部署依賴..."
    
    # 检查 Docker
    if ! command -v docker &> /dev/null; then
        log_error "Docker 未安裝，請先安裝 Docker"
        exit 1
    fi
    
    # 检查 Docker Compose
    if ! command -v docker-compose &> /dev/null; then
        log_error "Docker Compose 未安裝，請先安裝 Docker Compose"
        exit 1
    fi
    
    # 检查 kubectl（可選）
    if command -v kubectl &> /dev/null; then
        log_success "kubectl 已安裝，支援 Kubernetes 部署"
    else
        log_warning "kubectl 未安裝，僅支援 Docker Compose 部署"
    fi
    
    log_success "依賴检查完成"
}

# 建置 Docker 映像
build_images() {
    log_info "建置 Docker 映像..."
    
    # 建置微服务映像
    docker build -f Dockerfile.services -t chat-services:latest .
    log_success "微服务映像建置完成"
    
    # 建置 Gateway 映像
    docker build -f Dockerfile.gateway -t chat-gateway:latest .
    log_success "Gateway 映像建置完成"
}

# Docker Compose 部署
deploy_docker_compose() {
    log_info "使用 Docker Compose 部署..."
    
    # 停止现有服务
    docker-compose -f docker-compose.enterprise.yml down 2>/dev/null || true
    
    # 启动服务
    docker-compose -f docker-compose.enterprise.yml up -d
    
    # 等待服务启动
    log_info "等待服务启动..."
    sleep 30
    
    # 检查服务狀態
    docker-compose -f docker-compose.enterprise.yml ps
    
    log_success "Docker Compose 部署完成"
    show_access_info
}

# Kubernetes 部署
deploy_kubernetes() {
    log_info "使用 Kubernetes 部署..."
    
    # 检查 kubectl
    if ! command -v kubectl &> /dev/null; then
        log_error "kubectl 未安裝，無法进行 Kubernetes 部署"
        exit 1
    fi
    
    # 检查集群连接
    if ! kubectl cluster-info &> /dev/null; then
        log_error "無法连接到 Kubernetes 集群"
        exit 1
    fi
    
    # 部署配置
    kubectl apply -f deploy/k8s/namespace.yaml
    kubectl apply -f deploy/k8s/configmap.yaml
    kubectl apply -f deploy/k8s/services-deployment.yaml
    kubectl apply -f deploy/k8s/gateway-deployment.yaml
    
    # 等待部署完成
    log_info "等待部署完成..."
    kubectl wait --for=condition=available --timeout=300s deployment/gateway-deployment -n chat-system
    kubectl wait --for=condition=available --timeout=300s deployment/user-service-deployment -n chat-system
    kubectl wait --for=condition=available --timeout=300s deployment/social-service-deployment -n chat-system
    kubectl wait --for=condition=available --timeout=300s deployment/message-service-deployment -n chat-system
    
    # 顯示部署狀態
    kubectl get pods -n chat-system
    kubectl get services -n chat-system
    
    log_success "Kubernetes 部署完成"
    show_k8s_access_info
}

# 顯示訪問信息
show_access_info() {
    log_success "部署完成！訪問信息："
    echo ""
    echo "🌐 服务訪問："
    echo "  - Gateway: http://localhost:7000"
    echo "  - 健康检查: http://localhost:8080/health"
    echo ""
    echo "📊 监控面板："
    echo "  - Grafana: http://localhost:3000 (admin/admin)"
    echo "  - Prometheus: http://localhost:9090"
    echo "  - Jaeger: http://localhost:16686"
    echo "  - Consul: http://localhost:8500"
    echo ""
    echo "🗄️ 资料庫："
    echo "  - MariaDB: localhost:3306 (root/password)"
    echo ""
    echo "📝 测试命令："
    echo "  - 查看日誌: docker-compose -f docker-compose.enterprise.yml logs -f"
    echo "  - 停止服务: docker-compose -f docker-compose.enterprise.yml down"
    echo "  - 重启服务: docker-compose -f docker-compose.enterprise.yml restart"
}

# 顯示 Kubernetes 訪問信息
show_k8s_access_info() {
    log_success "Kubernetes 部署完成！訪問信息："
    echo ""
    echo "🌐 服务訪問："
    echo "  - Gateway: kubectl port-forward svc/gateway-service 7000:7000 -n chat-system"
    echo "  - 健康检查: kubectl port-forward svc/gateway-service 8080:8080 -n chat-system"
    echo ""
    echo "📊 监控面板："
    echo "  - Grafana: kubectl port-forward svc/grafana 3000:3000 -n chat-system"
    echo "  - Prometheus: kubectl port-forward svc/prometheus 9090:9090 -n chat-system"
    echo "  - Jaeger: kubectl port-forward svc/jaeger 16686:16686 -n chat-system"
    echo ""
    echo "📝 管理命令："
    echo "  - 查看 Pod: kubectl get pods -n chat-system"
    echo "  - 查看日誌: kubectl logs -f deployment/gateway-deployment -n chat-system"
    echo "  - 扩缩容: kubectl scale deployment gateway-deployment --replicas=3 -n chat-system"
}

# 清理部署
cleanup() {
    log_info "清理部署..."
    
    if [ "$DEPLOY_TYPE" = "k8s" ]; then
        kubectl delete -f deploy/k8s/ --ignore-not-found=true
        log_success "Kubernetes 部署已清理"
    else
        docker-compose -f docker-compose.enterprise.yml down -v
        docker system prune -f
        log_success "Docker Compose 部署已清理"
    fi
}

# 健康检查
health_check() {
    log_info "执行健康检查..."
    
    if [ "$DEPLOY_TYPE" = "k8s" ]; then
        # Kubernetes 健康检查
        kubectl get pods -n chat-system
        kubectl get services -n chat-system
    else
        # Docker Compose 健康检查
        docker-compose -f docker-compose.enterprise.yml ps
        curl -f http://localhost:8080/health || log_warning "Gateway 健康检查失败"
        curl -f http://localhost:9090/-/healthy || log_warning "Prometheus 健康检查失败"
    fi
    
    log_success "健康检查完成"
}

# 主函数
main() {
    echo "🚀 企業级聊天系统部署腳本"
    echo "================================"
    
    # 解析參数
    DEPLOY_TYPE="docker"
    BUILD_IMAGES=true
    CLEANUP=false
    HEALTH_CHECK=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --k8s)
                DEPLOY_TYPE="k8s"
                shift
                ;;
            --no-build)
                BUILD_IMAGES=false
                shift
                ;;
            --cleanup)
                CLEANUP=true
                shift
                ;;
            --health-check)
                HEALTH_CHECK=true
                shift
                ;;
            --help)
                echo "用法: $0 [選项]"
                echo "選项:"
                echo "  --k8s           使用 Kubernetes 部署（預设：Docker Compose）"
                echo "  --no-build      跳过映像建置"
                echo "  --cleanup       清理部署"
                echo "  --health-check  执行健康检查"
                echo "  --help          顯示此幫助信息"
                exit 0
                ;;
            *)
                log_error "未知參数: $1"
                exit 1
                ;;
        esac
    done
    
    # 执行清理
    if [ "$CLEANUP" = true ]; then
        cleanup
        exit 0
    fi
    
    # 执行健康检查
    if [ "$HEALTH_CHECK" = true ]; then
        health_check
        exit 0
    fi
    
    # 检查依賴
    check_dependencies
    
    # 建置映像
    if [ "$BUILD_IMAGES" = true ]; then
        build_images
    fi
    
    # 部署
    if [ "$DEPLOY_TYPE" = "k8s" ]; then
        deploy_kubernetes
    else
        deploy_docker_compose
    fi
    
    log_success "部署完成！"
}

# 执行主函数
main "$@"
