#!/bin/bash

echo "🎬 聊天應用程式演示"
echo "=================="
echo ""

# 檢查 Node.js 是否安裝
if ! command -v node &> /dev/null; then
    echo "❌ Node.js 未安裝"
    echo "請先運行: ./install_nodejs.sh"
    exit 1
fi

echo "✅ Node.js 已安裝: $(node --version)"
echo "✅ npm 已安裝: $(npm --version)"
echo ""

# 檢查 web 目錄
if [ ! -d "web" ]; then
    echo "❌ web 目錄不存在"
    exit 1
fi

echo "📁 檢查 web 目錄結構..."
ls -la web/
echo ""

echo "🚀 啟動網頁版聊天應用程式..."
echo "📍 訪問地址: http://localhost:3000"
echo "🛑 按 Ctrl+C 停止演示"
echo ""

# 進入 web 目錄並啟動
cd web
./start.sh 