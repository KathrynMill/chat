#!/bin/bash

echo "🌍 啟動公開訪問的聊天伺服器"
echo "=========================="
echo ""

# 檢查 Node.js
if ! command -v node &> /dev/null; then
    echo "❌ Node.js 未安裝，請先運行: ./install_nodejs.sh"
    exit 1
fi

echo "✅ Node.js 版本: $(node --version)"

# 獲取本機IP地址
echo "🔍 獲取本機IP地址..."
LOCAL_IP=$(hostname -I | awk '{print $1}')
PUBLIC_IP=$(curl -s ifconfig.me 2>/dev/null || echo "無法獲取公網IP")

echo "📍 本機IP: $LOCAL_IP"
echo "🌐 公網IP: $PUBLIC_IP"
echo ""

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
echo "🚀 啟動公開訪問伺服器..."
echo "📍 本地訪問: http://localhost:3000"
echo "📍 本機訪問: http://$LOCAL_IP:3000"
if [ "$PUBLIC_IP" != "無法獲取公網IP" ]; then
    echo "🌐 公網訪問: http://$PUBLIC_IP:3000"
fi
echo ""
echo "📋 重要提醒："
echo "1. 確保防火牆已開放端口 3000"
echo "2. 如果使用雲伺服器，請在安全組中開放端口 3000"
echo "3. 其他用戶可以通過上述地址訪問您的聊天應用程式"
echo ""
echo "🛑 按 Ctrl+C 停止伺服器"
echo ""

# 設置環境變數並啟動伺服器
export HOST=0.0.0.0
export PORT=3000
npm start 