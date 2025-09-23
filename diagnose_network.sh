#!/bin/bash

echo "🔍 網路連接診斷工具"
echo "=================="
echo ""

# 獲取IP地址
LOCAL_IP=$(hostname -I | awk '{print $1}')
PUBLIC_IP=$(curl -s ifconfig.me 2>/dev/null || echo "無法獲取")

echo "📍 本機IP: $LOCAL_IP"
echo "🌐 公網IP: $PUBLIC_IP"
echo ""

# 檢查伺服器狀態
echo "🔍 檢查伺服器狀態..."
if netstat -tuln | grep -q ":3000 "; then
    echo "✅ 伺服器正在監聽端口 3000"
else
    echo "❌ 伺服器未在端口 3000 監聽"
    echo "請先啟動伺服器：./start_public.sh"
    exit 1
fi

# 檢查防火牆
echo ""
echo "🔥 檢查防火牆狀態..."
if command -v firewall-cmd &> /dev/null; then
    if firewall-cmd --list-ports | grep -q "3000/tcp"; then
        echo "✅ 防火牆已開放端口 3000"
    else
        echo "❌ 防火牆未開放端口 3000"
        echo "請運行: sudo ./setup_firewall.sh"
    fi
else
    echo "⚠️  未檢測到 firewalld"
fi

# 檢查 iptables
if command -v iptables &> /dev/null; then
    if iptables -L INPUT -n | grep -q "3000"; then
        echo "✅ iptables 已開放端口 3000"
    else
        echo "⚠️  iptables 未明確開放端口 3000"
    fi
fi

# 本地連接測試
echo ""
echo "🧪 本地連接測試..."
if curl -s http://localhost:3000 > /dev/null; then
    echo "✅ 本地連接正常"
else
    echo "❌ 本地連接失敗"
fi

# 本機IP連接測試
if curl -s http://$LOCAL_IP:3000 > /dev/null; then
    echo "✅ 本機IP連接正常"
else
    echo "❌ 本機IP連接失敗"
fi

echo ""
echo "🌐 雲伺服器安全組配置指南"
echo "=========================="
echo ""
echo "如果您使用的是雲伺服器，需要在安全組中開放端口 3000："
echo ""
echo "📋 阿里雲 ECS："
echo "1. 登入阿里雲控制台"
echo "2. 進入 ECS 實例詳情"
echo "3. 點擊「安全組」"
echo "4. 點擊「配置規則」"
echo "5. 添加安全組規則："
echo "   - 授權策略：允許"
echo "   - 端口範圍：3000/3000"
echo "   - 授權對象：0.0.0.0/0"
echo "   - 優先級：1"
echo ""
echo "📋 騰訊雲 CVM："
echo "1. 登入騰訊雲控制台"
echo "2. 進入 CVM 實例詳情"
echo "3. 點擊「安全組」"
echo "4. 點擊「編輯規則」"
echo "5. 添加入站規則："
echo "   - 類型：自定義"
echo "   - 來源：0.0.0.0/0"
echo "   - 協議端口：TCP:3000"
echo "   - 策略：允許"
echo ""
echo "📋 華為雲 ECS："
echo "1. 登入華為雲控制台"
echo "2. 進入 ECS 實例詳情"
echo "3. 點擊「安全組」"
echo "4. 點擊「更改安全組」"
echo "5. 添加規則："
echo "   - 方向：入方向"
echo "   - 協議：TCP"
echo "   - 端口：3000"
echo "   - 源地址：0.0.0.0/0"
echo ""
echo "🔧 測試連接"
echo "=========="
echo "配置完成後，其他人可以通過以下地址訪問："
echo "http://$PUBLIC_IP:3000"
echo ""
echo "您也可以使用以下命令測試連接："
echo "curl -I http://$PUBLIC_IP:3000"
echo ""
echo "💡 如果仍然無法訪問，請檢查："
echo "1. 雲伺服器安全組是否正確配置"
echo "2. 伺服器是否在運行：./start_public.sh"
echo "3. 防火牆是否開放：sudo ./setup_firewall.sh" 