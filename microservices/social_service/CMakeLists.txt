cmake_minimum_required(VERSION 3.16)
project(social_service)

add_executable(social_service
    src/main.cpp
    src/SocialServiceImpl.cpp
    ../common/db/Db.cpp
)

target_include_directories(social_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(social_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)

find_package(Protobuf QUIET)
find_package(gRPC QUIET)
if(Protobuf_FOUND AND gRPC_FOUND)
    target_compile_definitions(social_service PRIVATE HAVE_GRPC=1)
    target_link_libraries(social_service PRIVATE gRPC::grpc++ protobuf::libprotobuf chat_protos)
    message(STATUS "SocialService: gRPC/Protobuf FOUND - enabling server")
else()
    message(WARNING "SocialService: gRPC/Protobuf NOT found - building without gRPC")
endif()

# 可選接入 Redis（redis-plus-plus）
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(REDISPP QUIET redis++)
    if(REDISPP_FOUND)
        target_compile_definitions(social_service PRIVATE HAVE_REDIS=1)
        target_include_directories(social_service PRIVATE ${REDISPP_INCLUDE_DIRS})
        target_link_libraries(social_service PRIVATE ${REDISPP_LIBRARIES})
        message(STATUS "SocialService: redis-plus-plus FOUND - enabling Redis cache")
    else()
        message(WARNING "SocialService: redis-plus-plus NOT found - Redis disabled")
    endif()
endif()

# 可選接入 MariaDB/MySQL C API
find_path(MYSQL_INCLUDE_DIR mysql/mysql.h)
find_library(MYSQL_CLIENT_LIB mysqlclient)
if(MYSQL_INCLUDE_DIR AND MYSQL_CLIENT_LIB)
    target_compile_definitions(social_service PRIVATE HAVE_MARIADB=1)
    target_include_directories(social_service PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(social_service PRIVATE ${MYSQL_CLIENT_LIB})
    message(STATUS "SocialService: MySQL/MariaDB client FOUND - enabling DB persistence")
else()
    message(WARNING "SocialService: MySQL/MariaDB client NOT found - DB persistence disabled")
endif()


