// 聊天應用程式主類
class ChatApp {
    constructor() {
        this.currentUser = null;
        this.currentChat = null;
        this.websocket = null;
        this.friends = [];
        this.groups = [];
        this.messages = {};
        
        this.init();
    }

    init() {
        this.bindEvents();
        this.checkAuth();
    }

    bindEvents() {
        // 頁面切換事件
        document.getElementById('showRegister').addEventListener('click', (e) => {
            e.preventDefault();
            this.showPage('registerPage');
        });

        document.getElementById('showLogin').addEventListener('click', (e) => {
            e.preventDefault();
            this.showPage('loginPage');
        });

        document.getElementById('showFindId').addEventListener('click', (e) => {
            e.preventDefault();
            this.showFindIdModal();
        });

        // 表單提交事件
        document.getElementById('loginForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleLogin();
        });

        document.getElementById('registerForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleRegister();
        });

        // 登出事件
        document.getElementById('logoutBtn').addEventListener('click', () => {
            this.handleLogout();
        });

        // 標籤頁切換
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.addEventListener('click', (e) => {
                this.switchTab(e.target.dataset.tab);
            });
        });

        // 添加好友
        document.getElementById('addFriendBtn').addEventListener('click', () => {
            this.showAddFriendModal();
        });

        // 創建群組
        document.getElementById('createGroupBtn').addEventListener('click', () => {
            this.showCreateGroupModal();
        });

        // 發送訊息
        document.getElementById('sendBtn').addEventListener('click', () => {
            this.sendMessage();
        });

        // 按 Enter 發送訊息
        document.getElementById('messageInput').addEventListener('keypress', (e) => {
            if (e.key === 'Enter' && !e.shiftKey) {
                e.preventDefault();
                this.sendMessage();
            }
        });

        // 模態框關閉
        document.querySelector('.close').addEventListener('click', () => {
            this.hideModal();
        });

        window.addEventListener('click', (e) => {
            if (e.target === document.getElementById('modal')) {
                this.hideModal();
            }
        });
    }

    showPage(pageId) {
        document.querySelectorAll('.page').forEach(page => {
            page.classList.remove('active');
        });
        document.getElementById(pageId).classList.add('active');
    }

    switchTab(tabName) {
        // 更新標籤按鈕狀態
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.classList.remove('active');
        });
        document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');

        // 更新標籤內容
        document.querySelectorAll('.tab-content').forEach(content => {
            content.classList.remove('active');
        });
        document.getElementById(`${tabName}Tab`).classList.add('active');
    }

    checkAuth() {
        const token = localStorage.getItem('chatToken');
        const user = localStorage.getItem('chatUser');
        
        // 檢查是否有臨時用戶ID（從找回ID頁面跳轉過來）
        const tempUserId = localStorage.getItem('tempUserId');
        const tempUserPwd = localStorage.getItem('tempUserPwd');
        
        if (tempUserId && tempUserPwd) {
            // 自動填充登入表單
            document.getElementById('loginId').value = tempUserId;
            document.getElementById('loginPwd').value = tempUserPwd;
            
            // 清除臨時數據
            localStorage.removeItem('tempUserId');
            localStorage.removeItem('tempUserPwd');
            
            // 顯示提示
            alert(`已自動填入用戶ID: ${tempUserId}\n請點擊登入按鈕完成登入`);
        }
        
        if (token && user) {
            this.currentUser = JSON.parse(user);
            this.showPage('chatPage');
            this.connectWebSocket();
            this.loadUserData();
        } else {
            this.showPage('loginPage');
        }
    }

    async handleLogin() {
        const id = document.getElementById('loginId').value;
        const pwd = document.getElementById('loginPwd').value;

        try {
            const response = await fetch('/api/login', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ id, pwd })
            });

            const data = await response.json();

            if (data.success) {
                this.currentUser = data.user;
                localStorage.setItem('chatToken', data.token);
                localStorage.setItem('chatUser', JSON.stringify(data.user));
                
                this.showPage('chatPage');
                this.connectWebSocket();
                this.loadUserData();
                
                // 清空表單
                document.getElementById('loginForm').reset();
            } else {
                alert(data.message || '登入失敗');
            }
        } catch (error) {
            console.error('登入錯誤:', error);
            alert('登入失敗，請檢查網路連接');
        }
    }

    async handleRegister() {
        const name = document.getElementById('registerName').value;
        const pwd = document.getElementById('registerPwd').value;
        const pwdConfirm = document.getElementById('registerPwdConfirm').value;

        console.log('註冊請求:', { name, pwd: pwd ? '***' : 'undefined' });

        if (pwd !== pwdConfirm) {
            alert('密碼確認不匹配');
            return;
        }

        try {
            const response = await fetch('/api/register', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ name, pwd })
            });

            const data = await response.json();
            console.log('註冊響應:', data);

            if (data.success) {
                // 顯示註冊成功訊息，包含用戶ID
                const message = `註冊成功！\n\n您的用戶ID是: ${data.userId}\n用戶名: ${data.userName}\n\n請記住您的用戶ID，登入時需要使用。`;
                alert(message);
                
                // 自動填充登入表單
                document.getElementById('loginId').value = data.userId;
                document.getElementById('loginPwd').value = pwd;
                
                // 切換到登入頁面
                this.showPage('loginPage');
                document.getElementById('registerForm').reset();
            } else {
                alert(data.message || '註冊失敗');
            }
        } catch (error) {
            console.error('註冊錯誤:', error);
            alert('註冊失敗，請檢查網路連接');
        }
    }

    handleLogout() {
        if (this.websocket) {
            this.websocket.close();
        }
        
        localStorage.removeItem('chatToken');
        localStorage.removeItem('chatUser');
        
        this.currentUser = null;
        this.currentChat = null;
        this.friends = [];
        this.groups = [];
        this.messages = {};
        
        this.showPage('loginPage');
    }

    connectWebSocket() {
        const token = localStorage.getItem('chatToken');
        const wsUrl = `ws://${window.location.host}/ws?token=${token}`;
        
        this.websocket = new WebSocket(wsUrl);

        this.websocket.onopen = () => {
            console.log('WebSocket 連接已建立');
            this.updateUserStatus('online');
        };

        this.websocket.onmessage = (event) => {
            const data = JSON.parse(event.data);
            this.handleWebSocketMessage(data);
        };

        this.websocket.onclose = () => {
            console.log('WebSocket 連接已關閉');
            this.updateUserStatus('offline');
        };

        this.websocket.onerror = (error) => {
            console.error('WebSocket 錯誤:', error);
        };
    }

    handleWebSocketMessage(data) {
        switch (data.type) {
            case 'LOGIN_MSG_ACK':
                this.handleLoginResponse(data);
                break;
            case 'REG_MSG_ACK':
                this.handleRegisterResponse(data);
                break;
            case 'ONE_CHAT_MSG':
                this.handleChatMessage(data);
                break;
            case 'GROUP_CHAT_MSG':
                this.handleGroupMessage(data);
                break;
            case 'ADD_FRIEND_MSG':
                this.handleAddFriendResponse(data);
                break;
            case 'CREATE_GROUP_MSG':
                this.handleCreateGroupResponse(data);
                break;
            default:
                console.log('未知訊息類型:', data.type);
        }
    }

    handleLoginResponse(data) {
        if (data.success) {
            this.loadUserData();
        } else {
            alert(data.message || '登入失敗');
        }
    }

    handleRegisterResponse(data) {
        if (data.success) {
            alert('註冊成功！');
        } else {
            alert(data.message || '註冊失敗');
        }
    }

    handleChatMessage(data) {
        const message = {
            id: Date.now(),
            from: data.fromid,
            to: data.toid,
            content: data.msg,
            time: new Date().toLocaleTimeString(),
            type: 'private'
        };

        this.addMessageToChat(data.fromid, message);
    }

    handleGroupMessage(data) {
        const message = {
            id: Date.now(),
            from: data.userid,
            groupid: data.groupid,
            content: data.msg,
            time: new Date().toLocaleTimeString(),
            type: 'group'
        };

        this.addMessageToChat(data.groupid, message);
    }

    handleAddFriendResponse(data) {
        if (data.success) {
            this.loadFriends();
            alert('好友添加成功！');
        } else {
            alert(data.message || '添加好友失敗');
        }
    }

    handleCreateGroupResponse(data) {
        if (data.success) {
            this.loadGroups();
            alert('群組創建成功！');
        } else {
            alert(data.message || '創建群組失敗');
        }
    }

    async loadUserData() {
        await Promise.all([
            this.loadFriends(),
            this.loadGroups()
        ]);
        
        this.updateUserInfo();
    }

    async loadFriends() {
        try {
            const response = await fetch('/api/friends', {
                headers: {
                    'Authorization': `Bearer ${localStorage.getItem('chatToken')}`
                }
            });
            
            const data = await response.json();
            
            if (data.success) {
                this.friends = data.friends;
                this.renderFriendsList();
            }
        } catch (error) {
            console.error('載入好友列表失敗:', error);
        }
    }

    async loadGroups() {
        try {
            const response = await fetch('/api/groups', {
                headers: {
                    'Authorization': `Bearer ${localStorage.getItem('chatToken')}`
                }
            });
            
            const data = await response.json();
            
            if (data.success) {
                this.groups = data.groups;
                this.renderGroupsList();
            }
        } catch (error) {
            console.error('載入群組列表失敗:', error);
        }
    }

    renderFriendsList() {
        const container = document.getElementById('friendsList');
        container.innerHTML = '';

        this.friends.forEach(friend => {
            const item = document.createElement('div');
            item.className = 'contact-item';
            item.dataset.id = friend.id;
            item.dataset.type = 'friend';
            
            item.innerHTML = `
                <div class="contact-avatar">
                    <i class="fas fa-user"></i>
                </div>
                <div class="contact-info">
                    <h5>${friend.name}</h5>
                    <p>${friend.status || '離線'}</p>
                </div>
            `;
            
            item.addEventListener('click', () => {
                this.selectChat(friend.id, 'friend', friend.name);
            });
            
            container.appendChild(item);
        });
    }

    renderGroupsList() {
        const container = document.getElementById('groupsList');
        container.innerHTML = '';

        this.groups.forEach(group => {
            const item = document.createElement('div');
            item.className = 'contact-item';
            item.dataset.id = group.id;
            item.dataset.type = 'group';
            
            item.innerHTML = `
                <div class="contact-avatar">
                    <i class="fas fa-layer-group"></i>
                </div>
                <div class="contact-info">
                    <h5>${group.name}</h5>
                    <p>${group.memberCount || 0} 位成員</p>
                </div>
            `;
            
            item.addEventListener('click', () => {
                this.selectChat(group.id, 'group', group.name);
            });
            
            container.appendChild(item);
        });
    }

    selectChat(id, type, name) {
        // 移除之前的選中狀態
        document.querySelectorAll('.contact-item').forEach(item => {
            item.classList.remove('active');
        });

        // 添加新的選中狀態
        const selectedItem = document.querySelector(`[data-id="${id}"][data-type="${type}"]`);
        if (selectedItem) {
            selectedItem.classList.add('active');
        }

        this.currentChat = { id, type, name };
        
        // 更新聊天標題
        document.getElementById('chatTitle').textContent = name;
        document.getElementById('chatStatus').textContent = type === 'friend' ? '私人聊天' : '群組聊天';
        
        // 啟用輸入框
        document.getElementById('messageInput').disabled = false;
        document.getElementById('sendBtn').disabled = false;
        
        // 載入聊天記錄
        this.loadChatHistory(id, type);
    }

    async loadChatHistory(id, type) {
        try {
            const response = await fetch(`/api/messages/${type}/${id}`, {
                headers: {
                    'Authorization': `Bearer ${localStorage.getItem('chatToken')}`
                }
            });
            
            const data = await response.json();
            
            if (data.success) {
                this.messages[`${type}_${id}`] = data.messages;
                this.renderMessages();
            }
        } catch (error) {
            console.error('載入聊天記錄失敗:', error);
        }
    }

    addMessageToChat(chatId, message) {
        const chatKey = this.currentChat && 
            ((this.currentChat.type === 'friend' && (chatId === this.currentChat.id || chatId === this.currentUser.id)) ||
             (this.currentChat.type === 'group' && chatId === this.currentChat.id)) 
            ? `${this.currentChat.type}_${this.currentChat.id}` 
            : null;

        if (chatKey) {
            if (!this.messages[chatKey]) {
                this.messages[chatKey] = [];
            }
            this.messages[chatKey].push(message);
            this.renderMessages();
        }
    }

    renderMessages() {
        if (!this.currentChat) return;

        const chatKey = `${this.currentChat.type}_${this.currentChat.id}`;
        const messages = this.messages[chatKey] || [];
        const container = document.getElementById('messagesList');
        
        container.innerHTML = '';

        messages.forEach(message => {
            const isOwn = message.from === this.currentUser.id;
            const messageDiv = document.createElement('div');
            messageDiv.className = `message ${isOwn ? 'sent' : 'received'}`;
            
            messageDiv.innerHTML = `
                <div class="message-info">
                    <span>${isOwn ? '我' : message.from}</span>
                    <span>${message.time}</span>
                </div>
                <div class="message-content">${message.content}</div>
            `;
            
            container.appendChild(messageDiv);
        });

        // 滾動到底部
        container.scrollTop = container.scrollHeight;
    }

    sendMessage() {
        if (!this.currentChat || !this.websocket) return;

        const input = document.getElementById('messageInput');
        const content = input.value.trim();
        
        if (!content) return;

        const message = {
            type: this.currentChat.type === 'friend' ? 'ONE_CHAT_MSG' : 'GROUP_CHAT_MSG',
            fromid: this.currentUser.id,
            toid: this.currentChat.id,
            msg: content,
            time: new Date().toISOString()
        };

        this.websocket.send(JSON.stringify(message));

        // 清空輸入框
        input.value = '';

        // 立即顯示訊息
        const newMessage = {
            id: Date.now(),
            from: this.currentUser.id,
            content: content,
            time: new Date().toLocaleTimeString(),
            type: this.currentChat.type
        };

        this.addMessageToChat(this.currentChat.id, newMessage);
    }

    showAddFriendModal() {
        const modal = document.getElementById('modal');
        const title = document.getElementById('modalTitle');
        const body = document.getElementById('modalBody');
        
        title.textContent = '添加好友';
        body.innerHTML = `
            <input type="text" id="friendId" placeholder="輸入好友ID">
            <button onclick="chatApp.addFriend()">添加</button>
            <button class="cancel" onclick="chatApp.hideModal()">取消</button>
        `;
        
        modal.style.display = 'block';
    }

    showCreateGroupModal() {
        const modal = document.getElementById('modal');
        const title = document.getElementById('modalTitle');
        const body = document.getElementById('modalBody');
        
        title.textContent = '創建群組';
        body.innerHTML = `
            <input type="text" id="groupName" placeholder="群組名稱">
            <input type="text" id="groupDesc" placeholder="群組描述">
            <button onclick="chatApp.createGroup()">創建</button>
            <button class="cancel" onclick="chatApp.hideModal()">取消</button>
        `;
        
        modal.style.display = 'block';
    }

    hideModal() {
        document.getElementById('modal').style.display = 'none';
    }

    async addFriend() {
        const friendId = document.getElementById('friendId').value;
        
        if (!friendId) {
            alert('請輸入好友ID');
            return;
        }

        try {
            const response = await fetch('/api/friends/add', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Authorization': `Bearer ${localStorage.getItem('chatToken')}`
                },
                body: JSON.stringify({ friendId })
            });

            const data = await response.json();

            if (data.success) {
                this.hideModal();
                this.loadFriends();
                alert('好友添加成功！');
            } else {
                alert(data.message || '添加好友失敗');
            }
        } catch (error) {
            console.error('添加好友失敗:', error);
            alert('添加好友失敗');
        }
    }

    async createGroup() {
        const name = document.getElementById('groupName').value;
        const desc = document.getElementById('groupDesc').value;
        
        if (!name) {
            alert('請輸入群組名稱');
            return;
        }

        try {
            const response = await fetch('/api/groups/create', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Authorization': `Bearer ${localStorage.getItem('chatToken')}`
                },
                body: JSON.stringify({ name, desc })
            });

            const data = await response.json();

            if (data.success) {
                this.hideModal();
                this.loadGroups();
                alert('群組創建成功！');
            } else {
                alert(data.message || '創建群組失敗');
            }
        } catch (error) {
            console.error('創建群組失敗:', error);
            alert('創建群組失敗');
        }
    }

    updateUserInfo() {
        if (this.currentUser) {
            document.getElementById('currentUserName').textContent = this.currentUser.name;
        }
    }

    updateUserStatus(status) {
        const statusElement = document.querySelector('.status');
        statusElement.textContent = status === 'online' ? '在線' : '離線';
        statusElement.className = `status ${status}`;
    }

    // 找回用戶ID功能
    showFindIdModal() {
        document.getElementById('findIdModal').style.display = 'block';
        document.getElementById('findIdResult').innerHTML = '';
        document.getElementById('findIdName').value = '';
        document.getElementById('findIdPwd').value = '';
    }

    closeFindIdModal() {
        document.getElementById('findIdModal').style.display = 'none';
    }

    async findUserId() {
        const name = document.getElementById('findIdName').value;
        const pwd = document.getElementById('findIdPwd').value;
        const resultDiv = document.getElementById('findIdResult');

        if (!name || !pwd) {
            resultDiv.innerHTML = '<div class="result-message error">請填寫用戶名和密碼</div>';
            return;
        }

        try {
            const response = await fetch('/api/find-user-id', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ name, pwd })
            });

            const data = await response.json();

            if (data.success) {
                resultDiv.innerHTML = `
                    <div class="result-message success">
                        <strong>找到用戶！</strong><br>
                        用戶名: ${data.userName}<br>
                        用戶ID: <strong>${data.userId}</strong><br><br>
                        <button onclick="chatApp.useFoundUserId('${data.userId}', '${pwd}')" class="btn-primary">
                            使用此ID登入
                        </button>
                    </div>
                `;
            } else {
                resultDiv.innerHTML = `<div class="result-message error">${data.message}</div>`;
            }
        } catch (error) {
            console.error('找回用戶ID錯誤:', error);
            resultDiv.innerHTML = '<div class="result-message error">網路錯誤，請重試</div>';
        }
    }

    useFoundUserId(userId, pwd) {
        // 關閉找回ID模態框
        this.closeFindIdModal();
        
        // 切換到登入頁面
        this.showPage('loginPage');
        
        // 填充登入表單
        document.getElementById('loginId').value = userId;
        document.getElementById('loginPwd').value = pwd;
        
        // 顯示成功訊息
        alert(`已自動填入用戶ID: ${userId}\n請點擊登入按鈕完成登入`);
    }
}

// 初始化應用程式
const chatApp = new ChatApp();

// 全局函數，供HTML調用
window.findUserId = function() {
    chatApp.findUserId();
};

window.closeFindIdModal = function() {
    chatApp.closeFindIdModal();
}; 