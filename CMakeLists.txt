cmake_minimum_required(VERSION 3.16)
project(chat)

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 配置最终的可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)
link_directories(/usr/lib64/mysql)

# 集成Crow框架（更輕量級的C++ Web框架）
include(FetchContent)
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.0+5
)
FetchContent_MakeAvailable(crow)

# 加载子目录
add_subdirectory(src)

# 微服務改造：可選開關（預設關閉，避免影響現有建置）
option(BUILD_MICROSERVICES "Build gRPC-based microservices (gateway/user/social/message)" OFF)
if(BUILD_MICROSERVICES)
    add_subdirectory(microservices)
endif()