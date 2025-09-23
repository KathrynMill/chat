#!/bin/bash

echo "🧪 連接測試工具"
echo "=============="
echo ""

# 獲取公網IP
PUBLIC_IP=$(curl -s ifconfig.me 2>/dev/null || echo "無法獲取")

echo "🌐 您的公網IP: $PUBLIC_IP"
echo "📍 測試地址: http://$PUBLIC_IP:3000"
echo ""

# 檢查伺服器是否運行
if ! netstat -tuln | grep -q ":3000 "; then
    echo "❌ 伺服器未運行，請先啟動："
    echo "   ./start_public.sh"
    exit 1
fi

echo "✅ 伺服器正在運行"
echo ""

# 測試本地連接
echo "🔍 測試本地連接..."
if curl -s http://localhost:3000 > /dev/null; then
    echo "✅ 本地連接正常"
else
    echo "❌ 本地連接失敗"
fi

# 測試公網連接
echo ""
echo "🌐 測試公網連接..."
echo "這可能需要幾秒鐘..."

TIMEOUT=10
if timeout $TIMEOUT curl -s http://$PUBLIC_IP:3000 > /dev/null; then
    echo "✅ 公網連接成功！"
    echo ""
    echo "🎉 其他人現在可以訪問您的聊天應用程式了！"
    echo "📍 訪問地址: http://$PUBLIC_IP:3000"
    echo ""
    echo "📋 功能包括："
    echo "   - 用戶註冊和登入"
    echo "   - 即時聊天"
    echo "   - 好友管理"
    echo "   - 找回用戶ID"
else
    echo "❌ 公網連接失敗"
    echo ""
    echo "🔧 這表示雲伺服器安全組未正確配置"
    echo "請按照以下步驟配置："
    echo ""
    echo "1. 查看配置指南："
    echo "   cat cloud_security_guide.md"
    echo ""
    echo "2. 或者運行診斷工具："
    echo "   ./diagnose_network.sh"
    echo ""
    echo "3. 配置完成後再次測試："
    echo "   ./test_connection.sh"
fi

echo ""
echo "💡 提示："
echo "- 如果使用雲伺服器，需要在安全組中開放端口 3000"
echo "- 配置後可能需要等待 1-2 分鐘生效"
echo "- 確保伺服器正在運行：./start_public.sh" 