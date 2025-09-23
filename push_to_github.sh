#!/bin/bash

echo "🚀 推送代碼到 GitHub"
echo "=================="
echo ""

# 檢查是否已經設置了遠端倉庫
if ! git remote get-url origin > /dev/null 2>&1; then
    echo "❌ 未設置遠端倉庫"
    echo "請先運行：git remote add origin https://github.com/KathrynMill/chatserver.git"
    exit 1
fi

echo "✅ 遠端倉庫已設置：$(git remote get-url origin)"
echo ""

# 檢查是否有未提交的更改
if [ -n "$(git status --porcelain)" ]; then
    echo "📝 發現未提交的更改，正在提交..."
    git add .
    git commit -m "更新聊天伺服器專案"
    echo "✅ 更改已提交"
    echo ""
fi

echo "🔐 需要 GitHub 身份驗證"
echo "請按照以下步驟操作："
echo ""
echo "1. 訪問 https://github.com/settings/tokens"
echo "2. 點擊 'Generate new token (classic)'"
echo "3. 選擇 'repo' 權限"
echo "4. 複製生成的令牌"
echo ""
echo "然後運行以下命令（替換 YOUR_TOKEN 為您的令牌）："
echo ""
echo "git push https://YOUR_TOKEN@github.com/KathrynMill/chatserver.git master"
echo ""
echo "或者，如果您已經設置了 SSH 密鑰，可以使用："
echo "git remote set-url origin git@github.com:KathrynMill/chatserver.git"
echo "git push -u origin master"
echo ""

# 檢查是否提供了令牌作為參數
if [ $# -eq 1 ]; then
    TOKEN=$1
    echo "🔑 使用提供的令牌進行推送..."
    git push https://${TOKEN}@github.com/KathrynMill/chatserver.git master
    if [ $? -eq 0 ]; then
        echo ""
        echo "🎉 推送成功！"
        echo "您的代碼已經上傳到：https://github.com/KathrynMill/chatserver"
        echo ""
        echo "📋 專案包含："
        echo "   - 原始 C++ 聊天伺服器"
        echo "   - 現代化 Web 聊天應用程式"
        echo "   - 本地/外網訪問切換功能"
        echo "   - 完整的部署腳本"
    else
        echo "❌ 推送失敗，請檢查令牌是否正確"
    fi
else
    echo "💡 提示：您也可以直接運行："
    echo "   ./push_to_github.sh YOUR_TOKEN"
fi 