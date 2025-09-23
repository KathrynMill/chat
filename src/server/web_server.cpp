#include <crow.h>
#include "web_controller.hpp"

int main()
{
    crow::SimpleApp app;
    // 初始化 Web 控制器（註冊路由和靜態文件服務）
    WebController::instance()->init();

    // 監聽 3000 端口
    app.port(3000).multithreaded().run();
    return 0;
} 