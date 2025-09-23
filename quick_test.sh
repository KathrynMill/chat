#!/bin/bash

echo "🚀 快速測試註冊登入功能"
echo "========================"
echo ""

# 檢查 Node.js
if ! command -v node &> /dev/null; then
    echo "❌ Node.js 未安裝，請先運行: ./install_nodejs.sh"
    exit 1
fi

echo "✅ Node.js 版本: $(node --version)"

# 進入 web 目錄
cd web

# 檢查依賴
if [ ! -d "node_modules" ]; then
    echo "📦 安裝依賴..."
    npm install
fi

echo ""
echo "🌐 啟動測試伺服器..."
echo "📍 主頁面: http://localhost:3000"
echo "🧪 簡單測試: http://localhost:3000/test"
echo "🔧 調試頁面: http://localhost:3000/debug"
echo ""
echo "💡 測試步驟："
echo "1. 打開瀏覽器訪問 http://localhost:3000/test"
echo "2. 點擊註冊按鈕"
echo "3. 查看響應結果"
echo "4. 如果註冊成功，用戶ID會自動填入登入表單"
echo "5. 點擊登入按鈕測試登入"
echo ""
echo "🛑 按 Ctrl+C 停止伺服器"
echo ""

# 啟動伺服器
npm start 