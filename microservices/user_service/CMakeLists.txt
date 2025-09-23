cmake_minimum_required(VERSION 3.16)
project(user_service)

add_executable(user_service
    src/main.cpp
    src/UserServiceImpl.cpp
)

target_include_directories(user_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 可選接入 gRPC/Protobuf
find_package(Protobuf QUIET)
find_package(gRPC QUIET)
if(Protobuf_FOUND AND gRPC_FOUND)
    target_compile_definitions(user_service PRIVATE HAVE_GRPC=1)
    target_link_libraries(user_service PRIVATE gRPC::grpc++ protobuf::libprotobuf chat_protos chat_common)
    message(STATUS "UserService: gRPC/Protobuf FOUND - enabling server")
else()
    message(WARNING "UserService: gRPC/Protobuf NOT found - building without gRPC")
endif()

# 可選接入 Redis（redis-plus-plus）
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(REDISPP QUIET redis++)
    if(REDISPP_FOUND)
        target_compile_definitions(user_service PRIVATE HAVE_REDIS=1)
        target_include_directories(user_service PRIVATE ${REDISPP_INCLUDE_DIRS})
        target_link_libraries(user_service PRIVATE ${REDISPP_LIBRARIES})
        message(STATUS "UserService: redis-plus-plus FOUND - enabling Redis cache")
    else()
        message(WARNING "UserService: redis-plus-plus NOT found - Redis disabled")
    endif()
endif()

# 可選接入 gRPC/Protobuf
find_package(Protobuf QUIET)
find_package(gRPC QUIET)
if(Protobuf_FOUND AND gRPC_FOUND)
    target_compile_definitions(user_service PRIVATE HAVE_GRPC=1)
    target_link_libraries(user_service PRIVATE gRPC::grpc++ protobuf::libprotobuf)
    message(STATUS "UserService: gRPC/Protobuf FOUND - enabling gRPC server")
else()
    message(WARNING "UserService: gRPC/Protobuf NOT found - building without gRPC (prints help at runtime)")
endif()


