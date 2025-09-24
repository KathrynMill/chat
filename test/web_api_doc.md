# C++ Web 聊天服务 API 文檔

## HTTP API

### 1. 註冊
- 路径：`POST /api/register`
- 請求：`{ "name": "用户名", "pwd": "密碼" }`
- 回應：`{ success, message, userId, userName }`

### 2. 登入
- 路径：`POST /api/login`
- 請求：`{ "id": 用户ID, "pwd": "密碼" }`
- 回應：`{ success, message, token, user: { id, name } }`

### 3. 找回用户ID
- 路径：`POST /api/find-user-id`
- 請求：`{ "name": "用户名", "pwd": "密碼" }`
- 回應：`{ success, message, userId, userName }`

### 4. 查询好友列表
- 路径：`GET /api/friends`
- Header: `Authorization: Bearer <token>`
- 回應：`{ success, friends: [ { id, name, status } ] }`

### 5. 添加好友
- 路径：`POST /api/friends/add`
- Header: `Authorization: Bearer <token>`
- 請求：`{ "friendId": 好友ID }`
- 回應：`{ success, message }`

## WebSocket
- 路径：`ws://localhost:3000/ws?token=<token>`
- 连接後可收发JSON消息

### 单聊消息
- 发送：
```json
{
  "type": "ONE_CHAT_MSG",
  "fromid": 发送者ID,
  "toid": 接收者ID,
  "msg": "内容",
  "time": "yyyy-MM-dd HH:mm:ss"
}
```
- 接收：同上

### 群聊消息
- 发送：
```json
{
  "type": "GROUP_CHAT_MSG",
  "groupid": 群组ID,
  "fromid": 发送者ID,
  "msg": "内容",
  "time": "yyyy-MM-dd HH:mm:ss"
}
```
- 接收：同上

---
如需更多接口或細节，請參考源碼或联系开发者。 