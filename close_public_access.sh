#!/bin/bash

echo "🔒 關閉外網訪問"
echo "=============="
echo ""

# 停止所有相關進程
echo "🛑 停止聊天伺服器..."
pkill -f "node server.js" 2>/dev/null
sleep 2

# 檢查是否還有進程在運行
if pgrep -f "node server.js" > /dev/null; then
    echo "⚠️  強制停止進程..."
    pkill -9 -f "node server.js" 2>/dev/null
fi

# 關閉防火牆端口
echo "🔥 關閉防火牆端口 3000..."
if command -v firewall-cmd &> /dev/null; then
    if sudo firewall-cmd --list-ports | grep -q "3000/tcp"; then
        sudo firewall-cmd --permanent --remove-port=3000/tcp
        sudo firewall-cmd --reload
        echo "✅ 防火牆端口 3000 已關閉"
    else
        echo "✅ 防火牆端口 3000 已經關閉"
    fi
else
    echo "⚠️  未檢測到 firewalld"
fi

# 檢查 iptables
if command -v iptables &> /dev/null; then
    echo "🔍 檢查 iptables 規則..."
    if sudo iptables -L INPUT -n | grep -q "3000"; then
        echo "⚠️  發現 iptables 規則，請手動檢查"
        sudo iptables -L INPUT -n | grep 3000
    else
        echo "✅ iptables 中沒有端口 3000 的規則"
    fi
fi

# 檢查端口狀態
echo ""
echo "🔍 檢查端口狀態..."
if netstat -tuln | grep -q ":3000 "; then
    echo "❌ 端口 3000 仍在監聽"
    echo "請檢查是否有其他程序在使用該端口"
    netstat -tuln | grep ":3000 "
else
    echo "✅ 端口 3000 已關閉"
fi

echo ""
echo "🎉 外網訪問已關閉！"
echo ""
echo "📋 當前狀態："
echo "   - 聊天伺服器已停止"
echo "   - 防火牆端口 3000 已關閉"
echo "   - 外網無法訪問您的聊天應用程式"
echo ""
echo "💡 如果需要重新啟動本地訪問："
echo "   ./start_local.sh"
echo ""
echo "💡 如果需要重新啟動外網訪問："
echo "   ./start_public.sh" 