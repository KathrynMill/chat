#!/bin/bash

# 企業級微服務框架構建腳本
# 支援完整的企業級功能構建

set -e

# 顏色定義
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# 檢查依賴
check_dependencies() {
    log_info "檢查構建依賴..."
    
    # 檢查 CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake 未安裝，請先運行: ./install_micro_deps.sh"
        exit 1
    fi
    
    # 檢查編譯器
    if ! command -v g++ &> /dev/null; then
        log_error "g++ 編譯器未安裝，請先運行: ./install_micro_deps.sh"
        exit 1
    fi
    
    log_success "構建依賴檢查完成"
}

# 清理構建目錄
clean_build() {
    log_info "清理構建目錄..."
    rm -rf build/*
    log_success "構建目錄已清理"
}

# 配置構建
configure_build() {
    log_info "配置企業級構建..."
    
    cd build
    
    # 配置 CMake，啟用所有企業級功能
    cmake .. \
        -DBUILD_MICROSERVICES=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_CXX_STANDARD_REQUIRED=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    
    if [ $? -eq 0 ]; then
        log_success "CMake 配置成功"
    else
        log_error "CMake 配置失敗"
        exit 1
    fi
    
    cd ..
}

# 編譯項目
build_project() {
    log_info "開始編譯企業級微服務框架..."
    
    cd build
    
    # 使用所有可用核心進行並行編譯
    CORES=$(nproc)
    log_info "使用 $CORES 個核心進行並行編譯"
    
    make -j$CORES
    
    if [ $? -eq 0 ]; then
        log_success "編譯成功完成"
    else
        log_error "編譯失敗"
        exit 1
    fi
    
    cd ..
}

# 運行測試
run_tests() {
    log_info "運行企業級功能測試..."
    
    # 檢查是否有測試可執行文件
    if [ -f "build/microservices/common/examples/EnterpriseFeaturesExample" ]; then
        log_info "運行企業級功能示例..."
        ./build/microservices/common/examples/EnterpriseFeaturesExample
    fi
    
    if [ -f "build/microservices/common/examples/AdvancedFeaturesExample" ]; then
        log_info "運行進階功能示例..."
        ./build/microservices/common/examples/AdvancedFeaturesExample
    fi
    
    log_success "測試完成"
}

# 顯示構建結果
show_results() {
    log_info "構建結果："
    echo ""
    
    # 顯示生成的可執行文件
    if [ -d "build/microservices" ]; then
        echo "📁 微服務可執行文件："
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
    log_success "企業級微服務框架構建完成！"
    echo ""
    echo "🚀 下一步："
    echo "  1. 運行企業級部署: ./deploy/deploy.sh"
    echo "  2. 運行系統監控: ./deploy/monitor.sh"
    echo "  3. 運行端到端測試: ./deploy/test.sh"
    echo "  4. 查看完整文檔: cat ADVANCED_ENTERPRISE_FEATURES.md"
}

# 主函數
main() {
    echo "🏗️  企業級微服務框架構建腳本"
    echo "=============================="
    echo ""
    
    # 檢查是否在正確的目錄
    if [ ! -f "CMakeLists.txt" ]; then
        log_error "請在項目根目錄運行此腳本"
        exit 1
    fi
    
    # 創建構建目錄
    mkdir -p build
    
    # 執行構建步驟
    check_dependencies
    clean_build
    configure_build
    build_project
    run_tests
    show_results
}

# 運行主函數
main "$@"
