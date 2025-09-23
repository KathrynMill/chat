const fetch = require('node-fetch');

const BASE_URL = 'http://localhost:3000';

async function testAuth() {
    console.log('🧪 測試註冊和登入功能...\n');

    // 測試註冊
    console.log('1. 測試註冊...');
    const registerData = {
        name: 'testuser',
        pwd: 'testpass123'
    };

    try {
        const registerResponse = await fetch(`${BASE_URL}/api/register`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(registerData)
        });

        const registerResult = await registerResponse.json();
        console.log('註冊結果:', registerResult);

        if (!registerResult.success) {
            console.log('❌ 註冊失敗');
            return;
        }

        const userId = registerResult.userId;
        console.log(`✅ 註冊成功，用戶ID: ${userId}\n`);

        // 測試登入
        console.log('2. 測試登入...');
        const loginData = {
            id: userId,
            pwd: 'testpass123'
        };

        const loginResponse = await fetch(`${BASE_URL}/api/login`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(loginData)
        });

        const loginResult = await loginResponse.json();
        console.log('登入結果:', loginResult);

        if (loginResult.success) {
            console.log('✅ 登入成功！');
            console.log(`用戶: ${loginResult.user.name}`);
            console.log(`Token: ${loginResult.token.substring(0, 20)}...`);
        } else {
            console.log('❌ 登入失敗');
        }

    } catch (error) {
        console.error('❌ 測試失敗:', error.message);
    }
}

// 檢查伺服器是否運行
async function checkServer() {
    try {
        const response = await fetch(BASE_URL);
        if (response.ok) {
            console.log('✅ 伺服器正在運行');
            return true;
        }
    } catch (error) {
        console.log('❌ 伺服器未運行，請先啟動伺服器');
        console.log('運行命令: cd web && npm start');
        return false;
    }
}

async function main() {
    const serverRunning = await checkServer();
    if (serverRunning) {
        await testAuth();
    }
}

main(); 