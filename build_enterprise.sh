#!/bin/bash

# 企業级微服务框架构建腳本
# 支援完整的企業级功能构建

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
    log_info "检查构建依賴..."
    
    # 检查 CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake 未安裝，請先运行: ./install_micro_deps.sh"
        exit 1
    fi
    
    # 检查編譯器
    if ! command -v g++ &> /dev/null; then
        log_error "g++ 編譯器未安裝，請先运行: ./install_micro_deps.sh"
        exit 1
    fi
    
    log_success "构建依賴检查完成"
}

# 清理构建目录
clean_build() {
    log_info "清理构建目录..."
    rm -rf build/*
    log_success "构建目录已清理"
}

# 配置构建
configure_build() {
    log_info "配置企業级构建..."
    
    cd build
    
    # 配置 CMake，启用所有企業级功能
    cmake .. \
        -DBUILD_MICROSERVICES=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_CXX_STANDARD_REQUIRED=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    
    if [ $? -eq 0 ]; then
        log_success "CMake 配置成功"
    else
        log_error "CMake 配置失败"
        exit 1
    fi
    
    cd ..
}

# 編譯项目
build_project() {
    log_info "开始編譯企業级微服务框架..."
    
    cd build
    
    # 使用所有可用核心进行並行編譯
    CORES=$(nproc)
    log_info "使用 $CORES 個核心进行並行編譯"
    
    make -j$CORES
    
    if [ $? -eq 0 ]; then
        log_success "編譯成功完成"
    else
        log_error "編譯失败"
        exit 1
    fi
    
    cd ..
}

# 运行测试
run_tests() {
    log_info "运行企業级功能测试..."
    
    # 检查是否有测试可执行文件
    if [ -f "build/microservices/common/examples/EnterpriseFeaturesExample" ]; then
        log_info "运行企業级功能示例..."
        ./build/microservices/common/examples/EnterpriseFeaturesExample
    fi
    
    if [ -f "build/microservices/common/examples/AdvancedFeaturesExample" ]; then
        log_info "运行进阶功能示例..."
        ./build/microservices/common/examples/AdvancedFeaturesExample
    fi
    
    log_success "测试完成"
}

# 顯示构建結果
show_results() {
    log_info "构建結果："
    echo ""
    
    # 顯示生成的可执行文件
    if [ -d "build/microservices" ]; then
        echo "📁 微服务可执行文件："
        find build/microservices -name "*.so" -o -name "*Service" -o -name "*Gateway" | while read file; do
            echo "  ✅ $(basename $file)"
        done
    fi
    
    echo ""
    echo "📁 示例程序："
    if [ -f "build/microservices/common/examples/EnterpriseFeaturesExample" ]; then
        echo "  ✅ EnterpriseFeaturesExample"
    fi
    if [ -f "build/microservices/common/examples/AdvancedFeaturesExample" ]; then
        echo "  ✅ AdvancedFeaturesExample"
    fi
    
    echo ""
    log_success "企業级微服务框架构建完成！"
    echo ""
    echo "🚀 下一步："
    echo "  1. 运行企業级部署: ./deploy/deploy.sh"
    echo "  2. 运行系统监控: ./deploy/monitor.sh"
    echo "  3. 运行端到端测试: ./deploy/test.sh"
    echo "  4. 查看完整文檔: cat ADVANCED_ENTERPRISE_FEATURES.md"
}

# 主函数
main() {
    echo "🏗️  企業级微服务框架构建腳本"
    echo "=============================="
    echo ""
    
    # 检查是否在正確的目录
    if [ ! -f "CMakeLists.txt" ]; then
        log_error "請在项目根目录运行此腳本"
        exit 1
    fi
    
    # 创建构建目录
    mkdir -p build
    
    # 执行构建步骤
    check_dependencies
    clean_build
    configure_build
    build_project
    run_tests
    show_results
}

# 运行主函数
main "$@"
