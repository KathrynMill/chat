#!/bin/bash

echo "🔍 診斷註冊登入問題"
echo "=================="
echo ""

# 检查系统
echo "📋 系统检查："
echo "   Node.js: $(node --version 2>/dev/null || echo '未安裝')"
echo "   npm: $(npm --version 2>/dev/null || echo '未安裝')"
echo "   當前目录: $(pwd)"
echo ""

# 检查文件
echo "📁 文件检查："
if [ -d "web" ]; then
    echo "   ✅ web 目录存在"
    if [ -f "web/server.js" ]; then
        echo "   ✅ server.js 存在"
    else
        echo "   ❌ server.js 不存在"
    fi
    if [ -f "web/package.json" ]; then
        echo "   ✅ package.json 存在"
    else
        echo "   ❌ package.json 不存在"
    fi
    if [ -d "web/node_modules" ]; then
        echo "   ✅ node_modules 存在"
    else
        echo "   ❌ node_modules 不存在"
    fi
else
    echo "   ❌ web 目录不存在"
fi
echo ""

# 检查端口
echo "🔌 端口检查："
if command -v netstat &> /dev/null; then
    if netstat -tuln | grep -q ":3000 "; then
        echo "   ⚠️  端口 3000 已被佔用"
    else
        echo "   ✅ 端口 3000 可用"
    fi
else
    echo "   ℹ️  無法检查端口狀態"
fi
echo ""

# 检查网絡
echo "🌐 网絡检查："
if command -v curl &> /dev/null; then
    if curl -s http://localhost:3000 > /dev/null 2>&1; then
        echo "   ✅ 本地伺服器正在运行"
    else
        echo "   ❌ 本地伺服器未运行"
    fi
else
    echo "   ℹ️  無法检查网絡连接"
fi
echo ""

echo "💡 建議解決方案："
echo "1. 如果 Node.js 未安裝：运行 ./install_nodejs.sh"
echo "2. 如果依賴未安裝：cd web && npm install"
echo "3. 如果端口被佔用：更改 PORT 环境变数"
echo "4. 启动测试：./quick_test.sh"
echo ""

echo "🔧 快速修复："
echo "cd web && npm install && npm start" 