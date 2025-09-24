#!/bin/bash

echo "🔍 网路连接診斷工具"
echo "=================="
echo ""

# 获取IP地址
LOCAL_IP=$(hostname -I | awk '{print $1}')
PUBLIC_IP=$(curl -s ifconfig.me 2>/dev/null || echo "無法获取")

echo "📍 本機IP: $LOCAL_IP"
echo "🌐 公网IP: $PUBLIC_IP"
echo ""

# 检查伺服器狀態
echo "🔍 检查伺服器狀態..."
if netstat -tuln | grep -q ":3000 "; then
    echo "✅ 伺服器正在监聽端口 3000"
else
    echo "❌ 伺服器未在端口 3000 监聽"
    echo "請先启动伺服器：./start_public.sh"
    exit 1
fi

# 检查防火牆
echo ""
echo "🔥 检查防火牆狀態..."
if command -v firewall-cmd &> /dev/null; then
    if firewall-cmd --list-ports | grep -q "3000/tcp"; then
        echo "✅ 防火牆已开放端口 3000"
    else
        echo "❌ 防火牆未开放端口 3000"
        echo "請运行: sudo ./setup_firewall.sh"
    fi
else
    echo "⚠️  未检测到 firewalld"
fi

# 检查 iptables
if command -v iptables &> /dev/null; then
    if iptables -L INPUT -n | grep -q "3000"; then
        echo "✅ iptables 已开放端口 3000"
    else
        echo "⚠️  iptables 未明確开放端口 3000"
    fi
fi

# 本地连接测试
echo ""
echo "🧪 本地连接测试..."
if curl -s http://localhost:3000 > /dev/null; then
    echo "✅ 本地连接正常"
else
    echo "❌ 本地连接失败"
fi

# 本機IP连接测试
if curl -s http://$LOCAL_IP:3000 > /dev/null; then
    echo "✅ 本機IP连接正常"
else
    echo "❌ 本機IP连接失败"
fi

echo ""
echo "🌐 雲伺服器安全组配置指南"
echo "=========================="
echo ""
echo "如果您使用的是雲伺服器，需要在安全组中开放端口 3000："
echo ""
echo "📋 阿里雲 ECS："
echo "1. 登入阿里雲控制台"
echo "2. 进入 ECS 实例詳情"
echo "3. 點擊「安全组」"
echo "4. 點擊「配置规则」"
echo "5. 添加安全组规则："
echo "   - 授权策略：允許"
echo "   - 端口范圍：3000/3000"
echo "   - 授权對象：0.0.0.0/0"
echo "   - 优先级：1"
echo ""
echo "📋 騰讯雲 CVM："
echo "1. 登入騰讯雲控制台"
echo "2. 进入 CVM 实例詳情"
echo "3. 點擊「安全组」"
echo "4. 點擊「編輯规则」"
echo "5. 添加入站规则："
echo "   - 類型：自定義"
echo "   - 來源：0.0.0.0/0"
echo "   - 協議端口：TCP:3000"
echo "   - 策略：允許"
echo ""
echo "📋 華為雲 ECS："
echo "1. 登入華為雲控制台"
echo "2. 进入 ECS 实例詳情"
echo "3. 點擊「安全组」"
echo "4. 點擊「更改安全组」"
echo "5. 添加规则："
echo "   - 方向：入方向"
echo "   - 協議：TCP"
echo "   - 端口：3000"
echo "   - 源地址：0.0.0.0/0"
echo ""
echo "🔧 测试连接"
echo "=========="
echo "配置完成後，其他人可以通过以下地址訪問："
echo "http://$PUBLIC_IP:3000"
echo ""
echo "您也可以使用以下命令测试连接："
echo "curl -I http://$PUBLIC_IP:3000"
echo ""
echo "💡 如果仍然無法訪問，請检查："
echo "1. 雲伺服器安全组是否正確配置"
echo "2. 伺服器是否在运行：./start_public.sh"
echo "3. 防火牆是否开放：sudo ./setup_firewall.sh" 