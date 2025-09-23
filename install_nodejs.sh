#!/bin/bash

echo "📦 安裝 Node.js 和 npm..."

# 檢查是否已經安裝
if command -v node &> /dev/null; then
    echo "✅ Node.js 已安裝，版本: $(node --version)"
    echo "✅ npm 已安裝，版本: $(npm --version)"
    exit 0
fi

# 檢測系統類型
if [ -f /etc/redhat-release ]; then
    # CentOS/RHEL 系統
    echo "🐧 檢測到 CentOS/RHEL 系統"
    
    # 安裝 NodeSource 倉庫
    echo "📥 添加 NodeSource 倉庫..."
    curl -fsSL https://rpm.nodesource.com/setup_18.x | sudo bash -
    
    # 安裝 Node.js
    echo "📦 安裝 Node.js..."
    sudo yum install -y nodejs
    
elif [ -f /etc/debian_version ]; then
    # Debian/Ubuntu 系統
    echo "🐧 檢測到 Debian/Ubuntu 系統"
    
    # 安裝 NodeSource 倉庫
    echo "📥 添加 NodeSource 倉庫..."
    curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
    
    # 安裝 Node.js
    echo "📦 安裝 Node.js..."
    sudo apt-get install -y nodejs
    
else
    echo "❌ 不支援的系統，請手動安裝 Node.js"
    echo "請訪問 https://nodejs.org/ 下載並安裝"
    exit 1
fi

# 驗證安裝
if command -v node &> /dev/null; then
    echo "✅ Node.js 安裝成功！"
    echo "📊 Node.js 版本: $(node --version)"
    echo "📊 npm 版本: $(npm --version)"
    
    # 安裝完成後提示
    echo ""
    echo "🎉 安裝完成！現在可以運行網頁版聊天應用程式："
    echo "   cd web"
    echo "   ./start.sh"
    echo ""
    echo "或者使用整合啟動器："
    echo "   ./start_web_chat.sh"
    
else
    echo "❌ Node.js 安裝失敗"
    exit 1
fi 