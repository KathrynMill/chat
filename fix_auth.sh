#!/bin/bash

echo "🔧 修復註冊登入問題"
echo "=================="
echo ""

# 檢查 Node.js 是否安裝
if ! command -v node &> /dev/null; then
    echo "❌ Node.js 未安裝"
    echo "請先運行: ./install_nodejs.sh"
    exit 1
fi

echo "✅ Node.js 已安裝: $(node --version)"

# 檢查 web 目錄
if [ ! -d "web" ]; then
    echo "❌ web 目錄不存在"
    exit 1
fi

echo "📁 進入 web 目錄..."
cd web

# 檢查依賴是否安裝
if [ ! -d "node_modules" ]; then
    echo "📦 安裝依賴..."
    npm install
fi

echo ""
echo "🚀 啟動伺服器進行測試..."
echo "📍 主頁面: http://localhost:3000"
echo "🔧 調試頁面: http://localhost:3000/debug"
echo ""
echo "💡 使用說明："
echo "1. 在調試頁面中測試註冊功能"
echo "2. 註冊成功後會顯示用戶ID"
echo "3. 使用用戶ID和密碼進行登入"
echo "4. 如果還有問題，可以查看伺服器日誌"
echo ""
echo "🛑 按 Ctrl+C 停止伺服器"
echo ""

# 啟動伺服器
npm start 