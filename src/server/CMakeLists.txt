# 定义了一个SRC_LIST变量，包含了该目录下所有的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)

# 補充 include 路徑
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)
# Crow 預設 include 路徑
include_directories(${CMAKE_BINARY_DIR}/_deps/crow-src/include)

# 指定生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})
# 指定可执行文件链接时需要依赖的库文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)

# 新增：Web服務器編譯（使用Crow框架）
add_executable(web_server web_server.cpp web_controller.cpp)
target_link_libraries(web_server PRIVATE pthread ssl crypto)

add_executable(web_server_minimal web_server_minimal.cpp)
target_link_libraries(web_server_minimal PRIVATE pthread ssl crypto boost_system)