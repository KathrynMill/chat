#!/bin/bash

echo "🚀 启动网頁版聊天應用程式..."

# 检查 Node.js 是否安裝
if ! command -v node &> /dev/null; then
    echo "❌ 错误: 未找到 Node.js，請先安裝 Node.js"
    echo "請訪問 https://nodejs.org/ 下载並安裝"
    exit 1
fi

# 检查 npm 是否安裝
if ! command -v npm &> /dev/null; then
    echo "❌ 错误: 未找到 npm，請先安裝 npm"
    exit 1
fi

echo "✅ Node.js 版本: $(node --version)"
echo "✅ npm 版本: $(npm --version)"

# 检查是否已安裝依賴
if [ ! -d "node_modules" ]; then
    echo "📦 安裝依賴..."
    npm install
    if [ $? -ne 0 ]; then
        echo "❌ 依賴安裝失败"
        exit 1
    fi
    echo "✅ 依賴安裝完成"
else
    echo "✅ 依賴已存在"
fi

# 启动伺服器
echo "🌐 启动 Web 伺服器..."
echo "📍 訪問地址: http://localhost:3000"
echo "🛑 按 Ctrl+C 停止伺服器"
echo ""

npm start 