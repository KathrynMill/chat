const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const path = require('path');
const jwt = require('jsonwebtoken');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// 中間件
app.use(express.json());
app.use(express.static(path.join(__dirname)));

// 模擬用戶數據庫
const users = new Map();
const connections = new Map();

// JWT 密鑰
const JWT_SECRET = 'your-secret-key';

// 路由
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'index.html'));
});

// 調試頁面
app.get('/debug', (req, res) => {
    res.sendFile(path.join(__dirname, 'debug.html'));
});

// 簡單測試頁面
app.get('/test', (req, res) => {
    res.sendFile(path.join(__dirname, 'test_simple.html'));
});

// 找回用戶ID頁面
app.get('/find-id', (req, res) => {
    res.sendFile(path.join(__dirname, 'find-id.html'));
});

// 調試API - 獲取所有用戶
app.get('/api/debug/users', (req, res) => {
    const usersList = Array.from(users.values()).map(user => ({
        id: user.id,
        name: user.name,
        pwd: user.pwd,
        status: user.status
    }));
    res.json(usersList);
});

// 調試API - 清理所有用戶數據
app.post('/api/debug/clear', (req, res) => {
    const userCount = users.size;
    users.clear();
    connections.clear();
    res.json({ 
        success: true, 
        message: `已清除 ${userCount} 個用戶的數據` 
    });
});

// 找回用戶ID API
app.post('/api/find-user-id', (req, res) => {
    const { name, pwd } = req.body;
    
    console.log('找回用戶ID請求:', { name, pwd: pwd ? '***' : 'undefined' });
    
    if (!name || !pwd) {
        return res.json({ success: false, message: '請填寫用戶名和密碼' });
    }

    // 查找用戶
    let foundUser = null;
    for (let [id, user] of users) {
        if (user.name === name && user.pwd === pwd) {
            foundUser = { id, ...user };
            break;
        }
    }

    if (foundUser) {
        console.log('找到用戶:', { id: foundUser.id, name: foundUser.name });
        res.json({
            success: true,
            message: '找到用戶',
            userId: foundUser.id,
            userName: foundUser.name
        });
    } else {
        console.log('未找到用戶:', { name });
        res.json({ 
            success: false, 
            message: '用戶名或密碼錯誤，或用戶不存在' 
        });
    }
});

// 註冊 API
app.post('/api/register', (req, res) => {
    const { name, pwd } = req.body;
    
    console.log('註冊請求:', { name, pwd: pwd ? '***' : 'undefined' });
    
    if (!name || !pwd) {
        return res.json({ success: false, message: '請填寫完整資訊' });
    }

    // 檢查用戶是否已存在
    for (let [id, user] of users) {
        if (user.name === name) {
            return res.json({ success: false, message: '用戶名已存在' });
        }
    }

    // 創建新用戶
    const userId = Date.now().toString();
    users.set(userId, {
        id: userId,
        name: name,
        pwd: pwd,
        status: 'offline'
    });

    console.log('用戶註冊成功:', { userId, name });
    console.log('當前所有用戶:', Array.from(users.entries()));

    res.json({ 
        success: true, 
        message: '註冊成功',
        userId: userId,
        userName: name
    });
});

// 登入 API
app.post('/api/login', (req, res) => {
    const { id, pwd } = req.body;
    
    console.log('登入請求:', { id, pwd: pwd ? '***' : 'undefined' });
    console.log('當前所有用戶:', Array.from(users.entries()));
    
    if (!id || !pwd) {
        return res.json({ success: false, message: '請填寫完整資訊' });
    }

    // 查找用戶
    const user = users.get(id);
    console.log('查找用戶結果:', user);
    
    if (!user || user.pwd !== pwd) {
        return res.json({ success: false, message: '用戶ID或密碼錯誤' });
    }

    // 生成 JWT token
    const token = jwt.sign({ userId: user.id }, JWT_SECRET, { expiresIn: '24h' });
    
    // 更新用戶狀態
    user.status = 'online';
    users.set(user.id, user);

    console.log('用戶登入成功:', { id: user.id, name: user.name });

    res.json({
        success: true,
        message: '登入成功',
        token: token,
        user: {
            id: user.id,
            name: user.name
        }
    });
});

// 驗證 JWT token 的中間件
function authenticateToken(req, res, next) {
    const authHeader = req.headers['authorization'];
    const token = authHeader && authHeader.split(' ')[1];

    if (!token) {
        return res.status(401).json({ success: false, message: '未提供認證令牌' });
    }

    jwt.verify(token, JWT_SECRET, (err, user) => {
        if (err) {
            return res.status(403).json({ success: false, message: '令牌無效' });
        }
        req.user = user;
        next();
    });
}

// 獲取好友列表
app.get('/api/friends', authenticateToken, (req, res) => {
    const friends = Array.from(users.values()).map(user => ({
        id: user.id,
        name: user.name,
        status: user.status
    }));
    
    res.json({ success: true, friends: friends });
});

// 添加好友
app.post('/api/friends/add', authenticateToken, (req, res) => {
    const { friendId } = req.body;
    
    if (!users.has(friendId)) {
        return res.json({ success: false, message: '用戶不存在' });
    }

    res.json({ success: true, message: '好友添加成功' });
});

// WebSocket 連接處理
wss.on('connection', (ws, req) => {
    console.log('新的 WebSocket 連接');

    // 解析 token
    const url = new URL(req.url, 'http://localhost');
    const token = url.searchParams.get('token');

    if (!token) {
        ws.close();
        return;
    }

    try {
        const decoded = jwt.verify(token, JWT_SECRET);
        const userId = decoded.userId;
        const user = users.get(userId);

        if (!user) {
            ws.close();
            return;
        }

        // 保存連接
        connections.set(userId, ws);
        user.status = 'online';
        users.set(userId, user);

        // 發送登入成功訊息
        ws.send(JSON.stringify({
            type: 'LOGIN_MSG_ACK',
            success: true,
            message: '登入成功'
        }));

        // 處理訊息
        ws.on('message', (message) => {
            try {
                const data = JSON.parse(message);
                handleMessage(userId, data);
            } catch (error) {
                console.error('解析訊息失敗:', error);
            }
        });

        // 處理連接關閉
        ws.on('close', () => {
            connections.delete(userId);
            user.status = 'offline';
            users.set(userId, user);
            console.log(`用戶 ${user.name} 斷開連接`);
        });

    } catch (error) {
        console.error('Token 驗證失敗:', error);
        ws.close();
    }
});

// 處理 WebSocket 訊息
function handleMessage(userId, data) {
    const user = users.get(userId);
    
    switch (data.type) {
        case 'ONE_CHAT_MSG':
            // 處理私人聊天訊息
            const targetUserId = data.toid;
            const targetWs = connections.get(targetUserId);
            
            if (targetWs) {
                targetWs.send(JSON.stringify({
                    type: 'ONE_CHAT_MSG',
                    fromid: userId,
                    toid: targetUserId,
                    msg: data.msg,
                    time: data.time
                }));
            }
            break;

        case 'GROUP_CHAT_MSG':
            // 處理群組聊天訊息
            const groupId = data.groupid;
            // 這裡可以實現群組訊息廣播邏輯
            break;

        default:
            console.log('未知訊息類型:', data.type);
    }
}

// 啟動伺服器
const PORT = process.env.PORT || 3000;
const HOST = process.env.HOST || '0.0.0.0'; // 監聽所有接口，允許外網訪問

server.listen(PORT, HOST, () => {
    console.log(`🌐 Web 伺服器運行在 http://${HOST}:${PORT}`);
    console.log(`🌍 外網可訪問，請確保防火牆已開放端口 ${PORT}`);
    console.log(`🧪 測試頁面: http://${HOST}:${PORT}/test`);
    console.log(`🔧 調試頁面: http://${HOST}:${PORT}/debug`);
    console.log(`🔍 找回ID頁面: http://${HOST}:${PORT}/find-id`);
    console.log(`📊 當前用戶數量: ${users.size}`);
    console.log('🛑 按 Ctrl+C 停止伺服器');
}); 