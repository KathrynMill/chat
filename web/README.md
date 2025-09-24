# 网頁版聊天应用程式

这是一个基于 Web 技术的聊天应用程式，提供即時通讯功能。

## 功能特色

- 🔐 用户注册和登录
- 💬 即時私人聊天
- 👥 群组聊天
- 👤 好友管理
- 📱 响应式设计
- 🔄 即時讯息同步

## 技术架构

- **前端**: HTML5, CSS3, JavaScript (ES6+)
- **後端**: Node.js, Express.js
- **即時通讯**: WebSocket
- **认证**: JWT (JSON Web Token)
- **樣式**: 自定義 CSS + Font Awesome 圖标

## 安裝和运行

### 前置需求

- Node.js (版本 14 或更高)
- npm 或 yarn

### 安裝步骤

1. **进入 web 目录**
   ```bash
   cd web
   ```

2. **安裝依賴**
   ```bash
   npm install
   ```

3. **启动开发伺服器**
   ```bash
   npm run dev
   ```

4. **访问应用程式**
   打开瀏覽器，访问 `http://localhost:3000`

### 生產环境部署

1. **安裝依賴**
   ```bash
   npm install --production
   ```

2. **启动伺服器**
   ```bash
   npm start
   ```

## 使用說明

### 注册新帳號
1. 點擊「立即注册」
2. 填写用户名稱和密码
3. 點擊「注册」按鈕

### 登录
1. 輸入用户ID和密码
2. 點擊「登录」按鈕

### 聊天功能
1. **私人聊天**: 在好友列表中點擊好友开始聊天
2. **群组聊天**: 在群组列表中點擊群组开始聊天
3. **发送讯息**: 在輸入框中輸入讯息，按 Enter 或點擊发送按鈕

### 好友管理
- 點擊「+」按鈕添加好友
- 輸入好友ID进行添加

### 群组管理
- 點擊「+」按鈕创建群组
- 輸入群组名稱和描述

## API 端點

### 认证
- `POST /api/register` - 用户注册
- `POST /api/login` - 用户登录

### 好友管理
- `GET /api/friends` - 获取好友列表
- `POST /api/friends/add` - 添加好友

### WebSocket
- `ws://localhost:3000/ws` - WebSocket 连接端點

## 讯息格式

### 私人聊天讯息
```json
{
  "type": "ONE_CHAT_MSG",
  "fromid": "发送者ID",
  "toid": "接收者ID",
  "msg": "讯息内容",
  "time": "時間戳"
}
```

### 群组聊天讯息
```json
{
  "type": "GROUP_CHAT_MSG",
  "userid": "发送者ID",
  "groupid": "群组ID",
  "msg": "讯息内容",
  "time": "時間戳"
}
```

## 配置

### 环境变数
- `PORT`: 伺服器端口 (預设: 3000)
- `JWT_SECRET`: JWT 密鑰 (預设: 'your-secret-key')

### 自定義配置
在 `server.js` 中修改以下配置：
```javascript
const PORT = process.env.PORT || 3000;
const JWT_SECRET = process.env.JWT_SECRET || 'your-secret-key';
```

## 安全性

- 使用 JWT 进行用户认证
- 密码在内存中存储（生產环境建議使用数据库）
- WebSocket 连接需要有效的 JWT token

## 开发

### 目录結构
```
web/
├── index.html          # 主頁面
├── styles.css          # 樣式文件
├── app.js             # 前端 JavaScript
├── server.js          # 後端伺服器
├── package.json       # 依賴配置
└── README.md          # 說明文件
```

### 添加新功能
1. 在 `app.js` 中添加前端邏輯
2. 在 `server.js` 中添加後端 API
3. 在 `styles.css` 中添加樣式

## 故障排除

### 常见问题

1. **端口被佔用**
   ```bash
   # 更改端口
   PORT=3001 npm start
   ```

2. **WebSocket 连接失败**
   - 检查瀏覽器控制台错误
   - 確认伺服器正在运行
   - 检查防火墙设置

3. **JWT 错误**
   - 清除瀏覽器本地存储
   - 重新登录

## 授权

MIT License

## 貢獻

歡迎提交 Issue 和 Pull Request！ 