# C++ Web 聊天服務 API 文檔

## HTTP API

### 1. 註冊
- 路徑：`POST /api/register`
- 請求：`{ "name": "用戶名", "pwd": "密碼" }`
- 回應：`{ success, message, userId, userName }`

### 2. 登入
- 路徑：`POST /api/login`
- 請求：`{ "id": 用戶ID, "pwd": "密碼" }`
- 回應：`{ success, message, token, user: { id, name } }`

### 3. 找回用戶ID
- 路徑：`POST /api/find-user-id`
- 請求：`{ "name": "用戶名", "pwd": "密碼" }`
- 回應：`{ success, message, userId, userName }`

### 4. 查詢好友列表
- 路徑：`GET /api/friends`
- Header: `Authorization: Bearer <token>`
- 回應：`{ success, friends: [ { id, name, status } ] }`

### 5. 添加好友
- 路徑：`POST /api/friends/add`
- Header: `Authorization: Bearer <token>`
- 請求：`{ "friendId": 好友ID }`
- 回應：`{ success, message }`

## WebSocket
- 路徑：`ws://localhost:3000/ws?token=<token>`
- 連接後可收發JSON消息

### 單聊消息
- 發送：
```json
{
  "type": "ONE_CHAT_MSG",
  "fromid": 發送者ID,
  "toid": 接收者ID,
  "msg": "內容",
  "time": "yyyy-MM-dd HH:mm:ss"
}
```
- 接收：同上

### 群聊消息
- 發送：
```json
{
  "type": "GROUP_CHAT_MSG",
  "groupid": 群組ID,
  "fromid": 發送者ID,
  "msg": "內容",
  "time": "yyyy-MM-dd HH:mm:ss"
}
```
- 接收：同上

---
如需更多接口或細節，請參考源碼或聯繫開發者。 