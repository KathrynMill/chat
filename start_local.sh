#!/bin/bash

echo "🔒 啟動本地訪問的聊天伺服器"
echo "=========================="
echo ""

# 檢查 Node.js
if ! command -v node &> /dev/null; then
    echo "❌ Node.js 未安裝，請先運行: ./install_nodejs.sh"
    exit 1
fi

echo "✅ Node.js 版本: $(node --version)"

# 檢查端口是否被佔用
if netstat -tuln | grep -q ":3000 "; then
    echo "⚠️  端口 3000 已被佔用，正在停止現有進程..."
    pkill -f "node server.js" 2>/dev/null
    sleep 2
fi

# 進入 web 目錄
cd web

# 檢查依賴
if [ ! -d "node_modules" ]; then
    echo "📦 安裝依賴..."
    npm install
fi

echo ""
echo "🚀 啟動本地訪問伺服器..."
echo "📍 訪問地址: http://localhost:3000"
echo "🔒 僅限本機訪問，外網無法連接"
echo ""
echo "📋 功能包括："
echo "   - 用戶註冊和登入"
echo "   - 即時聊天"
echo "   - 好友管理"
echo "   - 找回用戶ID"
echo ""
echo "🛑 按 Ctrl+C 停止伺服器"
echo ""

# 設置環境變數並啟動伺服器
export HOST=127.0.0.1
export PORT=3000
npm start 