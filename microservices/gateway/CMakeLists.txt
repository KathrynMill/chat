cmake_minimum_required(VERSION 3.16)
project(chat_gateway)

add_executable(chat_gateway
    src/main.cpp
)

target_include_directories(chat_gateway PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(chat_gateway PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/thirdparty)

# 可選接入 muduo（若系統已安裝）
find_library(MUDUO_NET muduo_net)
find_library(MUDUO_BASE muduo_base)
find_package(Threads REQUIRED)

if(MUDUO_NET AND MUDUO_BASE)
    target_compile_definitions(chat_gateway PRIVATE HAVE_MUDUO=1)
    target_link_libraries(chat_gateway PRIVATE ${MUDUO_NET} ${MUDUO_BASE} Threads::Threads)
    message(STATUS "Gateway: muduo FOUND - enabling event loop server")
else()
    message(WARNING "Gateway: muduo NOT found - building without muduo (prints help at runtime)")
endif()

# 可選接入 Kafka（cppkafka）
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(CPPKAFKA QUIET cppkafka)
    if(CPPKAFKA_FOUND)
        target_compile_definitions(chat_gateway PRIVATE HAVE_CPPKAFKA=1)
        target_include_directories(chat_gateway PRIVATE ${CPPKAFKA_INCLUDE_DIRS})
        target_link_libraries(chat_gateway PRIVATE ${CPPKAFKA_LIBRARIES})
        message(STATUS "Gateway: cppkafka FOUND - enabling Kafka consumer")
    else()
        message(WARNING "Gateway: cppkafka NOT found - Kafka consumer disabled")
    endif()
endif()

# 可選接入 gRPC 與 chat_protos
find_package(Protobuf QUIET)
find_package(gRPC QUIET)
if(Protobuf_FOUND AND gRPC_FOUND)
    target_compile_definitions(chat_gateway PRIVATE HAVE_GRPC=1)
    target_link_libraries(chat_gateway PRIVATE gRPC::grpc++ protobuf::libprotobuf chat_protos chat_common)
    message(STATUS "Gateway: gRPC/Protobuf FOUND - enabling gRPC clients")
else()
    message(WARNING "Gateway: gRPC/Protobuf NOT found - building without gRPC client")
endif()



