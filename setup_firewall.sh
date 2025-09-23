#!/bin/bash

echo "🔥 配置防火牆開放端口 3000"
echo "========================"
echo ""

# 檢查是否為 root 用戶
if [ "$EUID" -ne 0 ]; then
    echo "⚠️  需要 root 權限來配置防火牆"
    echo "請使用 sudo 運行此腳本："
    echo "sudo ./setup_firewall.sh"
    exit 1
fi

# 檢測系統類型
if command -v firewall-cmd &> /dev/null; then
    echo "🐧 檢測到 firewalld，正在配置..."
    
    # 檢查防火牆狀態
    if firewall-cmd --state &> /dev/null; then
        echo "✅ 防火牆正在運行"
        
        # 開放端口 3000
        firewall-cmd --permanent --add-port=3000/tcp
        firewall-cmd --reload
        
        echo "✅ 端口 3000 已開放"
        echo "📋 當前開放的端口："
        firewall-cmd --list-ports
    else
        echo "❌ 防火牆未運行，正在啟動..."
        systemctl start firewalld
        systemctl enable firewalld
        
        # 開放端口 3000
        firewall-cmd --permanent --add-port=3000/tcp
        firewall-cmd --reload
        
        echo "✅ 防火牆已啟動並開放端口 3000"
    fi
    
elif command -v ufw &> /dev/null; then
    echo "🐧 檢測到 ufw，正在配置..."
    
    # 檢查防火牆狀態
    if ufw status | grep -q "Status: active"; then
        echo "✅ 防火牆正在運行"
    else
        echo "❌ 防火牆未運行，正在啟動..."
        ufw enable
    fi
    
    # 開放端口 3000
    ufw allow 3000/tcp
    
    echo "✅ 端口 3000 已開放"
    echo "📋 當前防火牆規則："
    ufw status
    
elif command -v iptables &> /dev/null; then
    echo "🐧 檢測到 iptables，正在配置..."
    
    # 開放端口 3000
    iptables -A INPUT -p tcp --dport 3000 -j ACCEPT
    
    echo "✅ 端口 3000 已開放"
    echo "📋 當前 iptables 規則："
    iptables -L INPUT -n --line-numbers | grep 3000
    
else
    echo "❌ 未檢測到支援的防火牆系統"
    echo "請手動配置防火牆開放端口 3000"
    exit 1
fi

echo ""
echo "🎉 防火牆配置完成！"
echo "📋 下一步："
echo "1. 運行 ./start_public.sh 啟動伺服器"
echo "2. 其他用戶可以通過您的IP地址訪問聊天應用程式"
echo ""
echo "💡 提示："
echo "- 如果使用雲伺服器，還需要在雲服務商的安全組中開放端口 3000"
echo "- 建議在生產環境中使用 HTTPS 和更安全的配置" 