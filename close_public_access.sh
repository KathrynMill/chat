#!/bin/bash

echo "🔒 关闭外网訪問"
echo "=============="
echo ""

# 停止所有相关进程
echo "🛑 停止聊天伺服器..."
pkill -f "node server.js" 2>/dev/null
sleep 2

# 检查是否還有进程在运行
if pgrep -f "node server.js" > /dev/null; then
    echo "⚠️  强制停止进程..."
    pkill -9 -f "node server.js" 2>/dev/null
fi

# 关闭防火牆端口
echo "🔥 关闭防火牆端口 3000..."
if command -v firewall-cmd &> /dev/null; then
    if sudo firewall-cmd --list-ports | grep -q "3000/tcp"; then
        sudo firewall-cmd --permanent --remove-port=3000/tcp
        sudo firewall-cmd --reload
        echo "✅ 防火牆端口 3000 已关闭"
    else
        echo "✅ 防火牆端口 3000 已經关闭"
    fi
else
    echo "⚠️  未检测到 firewalld"
fi

# 检查 iptables
if command -v iptables &> /dev/null; then
    echo "🔍 检查 iptables 规则..."
    if sudo iptables -L INPUT -n | grep -q "3000"; then
        echo "⚠️  发现 iptables 规则，請手动检查"
        sudo iptables -L INPUT -n | grep 3000
    else
        echo "✅ iptables 中沒有端口 3000 的规则"
    fi
fi

# 检查端口狀態
echo ""
echo "🔍 检查端口狀態..."
if netstat -tuln | grep -q ":3000 "; then
    echo "❌ 端口 3000 仍在监聽"
    echo "請检查是否有其他程序在使用該端口"
    netstat -tuln | grep ":3000 "
else
    echo "✅ 端口 3000 已关闭"
fi

echo ""
echo "🎉 外网訪問已关闭！"
echo ""
echo "📋 當前狀態："
echo "   - 聊天伺服器已停止"
echo "   - 防火牆端口 3000 已关闭"
echo "   - 外网無法訪問您的聊天應用程式"
echo ""
echo "💡 如果需要重新启动本地訪問："
echo "   ./start_local.sh"
echo ""
echo "💡 如果需要重新启动外网訪問："
echo "   ./start_public.sh" 