#!/bin/bash

echo "🎯 聊天伺服器啟動器"
echo "=================="
echo "1. 運行原始 C++ 聊天伺服器"
echo "2. 運行網頁版聊天應用程式"
echo "3. 同時運行兩個伺服器"
echo "4. 退出"
echo ""

read -p "請選擇選項 (1-4): " choice

case $choice in
    1)
        echo "🚀 啟動 C++ 聊天伺服器..."
        if [ -f "bin/ChatServer" ]; then
            echo "請輸入伺服器 IP 和端口:"
            read -p "IP (預設: 127.0.0.1): " ip
            read -p "端口 (預設: 6000): " port
            
            ip=${ip:-127.0.0.1}
            port=${port:-6000}
            
            echo "📍 伺服器地址: $ip:$port"
            echo "🛑 按 Ctrl+C 停止伺服器"
            echo ""
            
            ./bin/ChatServer $ip $port
        else
            echo "❌ 錯誤: 未找到 ChatServer 執行檔"
            echo "請先編譯專案: ./autobuild.sh"
        fi
        ;;
    2)
        echo "🌐 啟動網頁版聊天應用程式..."
        if [ -d "web" ]; then
            cd web
            ./start.sh
        else
            echo "❌ 錯誤: 未找到 web 目錄"
        fi
        ;;
    3)
        echo "🔄 同時啟動兩個伺服器..."
        
        # 檢查 C++ 伺服器
        if [ ! -f "bin/ChatServer" ]; then
            echo "❌ 錯誤: 未找到 ChatServer 執行檔"
            echo "請先編譯專案: ./autobuild.sh"
            exit 1
        fi
        
        # 檢查 web 目錄
        if [ ! -d "web" ]; then
            echo "❌ 錯誤: 未找到 web 目錄"
            exit 1
        fi
        
        echo "請輸入 C++ 伺服器 IP 和端口:"
        read -p "IP (預設: 127.0.0.1): " ip
        read -p "端口 (預設: 6000): " port
        
        ip=${ip:-127.0.0.1}
        port=${port:-6000}
        
        echo "🚀 啟動 C++ 伺服器在 $ip:$port"
        echo "🌐 啟動 Web 伺服器在 http://localhost:3000"
        echo "🛑 按 Ctrl+C 停止所有伺服器"
        echo ""
        
        # 在背景啟動 C++ 伺服器
        ./bin/ChatServer $ip $port &
        cpp_pid=$!
        
        # 啟動 Web 伺服器
        cd web
        ./start.sh &
        web_pid=$!
        
        # 等待用戶中斷
        trap "echo '正在停止伺服器...'; kill $cpp_pid $web_pid 2>/dev/null; exit" INT
        
        wait
        ;;
    4)
        echo "👋 再見！"
        exit 0
        ;;
    *)
        echo "❌ 無效選項，請重新運行腳本"
        exit 1
        ;;
esac 