#!/bin/bash

echo "🔥 配置防火牆开放端口 3000"
echo "========================"
echo ""

# 检查是否為 root 用户
if [ "$EUID" -ne 0 ]; then
    echo "⚠️  需要 root 权限來配置防火牆"
    echo "請使用 sudo 运行此腳本："
    echo "sudo ./setup_firewall.sh"
    exit 1
fi

# 检测系统類型
if command -v firewall-cmd &> /dev/null; then
    echo "🐧 检测到 firewalld，正在配置..."
    
    # 检查防火牆狀態
    if firewall-cmd --state &> /dev/null; then
        echo "✅ 防火牆正在运行"
        
        # 开放端口 3000
        firewall-cmd --permanent --add-port=3000/tcp
        firewall-cmd --reload
        
        echo "✅ 端口 3000 已开放"
        echo "📋 當前开放的端口："
        firewall-cmd --list-ports
    else
        echo "❌ 防火牆未运行，正在启动..."
        systemctl start firewalld
        systemctl enable firewalld
        
        # 开放端口 3000
        firewall-cmd --permanent --add-port=3000/tcp
        firewall-cmd --reload
        
        echo "✅ 防火牆已启动並开放端口 3000"
    fi
    
elif command -v ufw &> /dev/null; then
    echo "🐧 检测到 ufw，正在配置..."
    
    # 检查防火牆狀態
    if ufw status | grep -q "Status: active"; then
        echo "✅ 防火牆正在运行"
    else
        echo "❌ 防火牆未运行，正在启动..."
        ufw enable
    fi
    
    # 开放端口 3000
    ufw allow 3000/tcp
    
    echo "✅ 端口 3000 已开放"
    echo "📋 當前防火牆规则："
    ufw status
    
elif command -v iptables &> /dev/null; then
    echo "🐧 检测到 iptables，正在配置..."
    
    # 开放端口 3000
    iptables -A INPUT -p tcp --dport 3000 -j ACCEPT
    
    echo "✅ 端口 3000 已开放"
    echo "📋 當前 iptables 规则："
    iptables -L INPUT -n --line-numbers | grep 3000
    
else
    echo "❌ 未检测到支援的防火牆系统"
    echo "請手动配置防火牆开放端口 3000"
    exit 1
fi

echo ""
echo "🎉 防火牆配置完成！"
echo "📋 下一步："
echo "1. 运行 ./start_public.sh 启动伺服器"
echo "2. 其他用户可以通过您的IP地址訪問聊天應用程式"
echo ""
echo "💡 提示："
echo "- 如果使用雲伺服器，還需要在雲服务商的安全组中开放端口 3000"
echo "- 建議在生產环境中使用 HTTPS 和更安全的配置" 