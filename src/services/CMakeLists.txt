cmake_minimum_required(VERSION 3.16)

# MessageService 可執行檔
add_executable(message_service message_service.cpp)

target_include_directories(message_service PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/thirdparty
    ${CMAKE_BINARY_DIR}/_deps/crow-src/include
)

# Crow 依賴 Boost.Asio（需要 boost_system），以及 pthread
target_link_libraries(message_service PRIVATE pthread boost_system)


