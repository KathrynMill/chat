# 網頁版聊天應用程式

這是一個基於 Web 技術的聊天應用程式，提供即時通訊功能。

## 功能特色

- 🔐 用戶註冊和登入
- 💬 即時私人聊天
- 👥 群組聊天
- 👤 好友管理
- 📱 響應式設計
- 🔄 即時訊息同步

## 技術架構

- **前端**: HTML5, CSS3, JavaScript (ES6+)
- **後端**: Node.js, Express.js
- **即時通訊**: WebSocket
- **認證**: JWT (JSON Web Token)
- **樣式**: 自定義 CSS + Font Awesome 圖標

## 安裝和運行

### 前置需求

- Node.js (版本 14 或更高)
- npm 或 yarn

### 安裝步驟

1. **進入 web 目錄**
   ```bash
   cd web
   ```

2. **安裝依賴**
   ```bash
   npm install
   ```

3. **啟動開發伺服器**
   ```bash
   npm run dev
   ```

4. **訪問應用程式**
   打開瀏覽器，訪問 `http://localhost:3000`

### 生產環境部署

1. **安裝依賴**
   ```bash
   npm install --production
   ```

2. **啟動伺服器**
   ```bash
   npm start
   ```

## 使用說明

### 註冊新帳號
1. 點擊「立即註冊」
2. 填寫用戶名稱和密碼
3. 點擊「註冊」按鈕

### 登入
1. 輸入用戶ID和密碼
2. 點擊「登入」按鈕

### 聊天功能
1. **私人聊天**: 在好友列表中點擊好友開始聊天
2. **群組聊天**: 在群組列表中點擊群組開始聊天
3. **發送訊息**: 在輸入框中輸入訊息，按 Enter 或點擊發送按鈕

### 好友管理
- 點擊「+」按鈕添加好友
- 輸入好友ID進行添加

### 群組管理
- 點擊「+」按鈕創建群組
- 輸入群組名稱和描述

## API 端點

### 認證
- `POST /api/register` - 用戶註冊
- `POST /api/login` - 用戶登入

### 好友管理
- `GET /api/friends` - 獲取好友列表
- `POST /api/friends/add` - 添加好友

### WebSocket
- `ws://localhost:3000/ws` - WebSocket 連接端點

## 訊息格式

### 私人聊天訊息
```json
{
  "type": "ONE_CHAT_MSG",
  "fromid": "發送者ID",
  "toid": "接收者ID",
  "msg": "訊息內容",
  "time": "時間戳"
}
```

### 群組聊天訊息
```json
{
  "type": "GROUP_CHAT_MSG",
  "userid": "發送者ID",
  "groupid": "群組ID",
  "msg": "訊息內容",
  "time": "時間戳"
}
```

## 配置

### 環境變數
- `PORT`: 伺服器端口 (預設: 3000)
- `JWT_SECRET`: JWT 密鑰 (預設: 'your-secret-key')

### 自定義配置
在 `server.js` 中修改以下配置：
```javascript
const PORT = process.env.PORT || 3000;
const JWT_SECRET = process.env.JWT_SECRET || 'your-secret-key';
```

## 安全性

- 使用 JWT 進行用戶認證
- 密碼在記憶體中存儲（生產環境建議使用資料庫）
- WebSocket 連接需要有效的 JWT token

## 開發

### 目錄結構
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

### 常見問題

1. **端口被佔用**
   ```bash
   # 更改端口
   PORT=3001 npm start
   ```

2. **WebSocket 連接失敗**
   - 檢查瀏覽器控制台錯誤
   - 確認伺服器正在運行
   - 檢查防火牆設置

3. **JWT 錯誤**
   - 清除瀏覽器本地存儲
   - 重新登入

## 授權

MIT License

## 貢獻

歡迎提交 Issue 和 Pull Request！ 