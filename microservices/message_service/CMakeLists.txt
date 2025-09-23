cmake_minimum_required(VERSION 3.16)
project(message_service)

add_executable(message_service
    src/main.cpp
    src/MessageServiceImpl.cpp
    ../common/db/Db.cpp
)

target_include_directories(message_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(message_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)

find_package(Protobuf QUIET)
find_package(gRPC QUIET)
if(Protobuf_FOUND AND gRPC_FOUND)
    target_compile_definitions(message_service PRIVATE HAVE_GRPC=1)
    target_link_libraries(message_service PRIVATE gRPC::grpc++ protobuf::libprotobuf chat_protos)
    message(STATUS "MessageService: gRPC/Protobuf FOUND - enabling server")
else()
    message(WARNING "MessageService: gRPC/Protobuf NOT found - building without gRPC")
endif()

# 可選接入 Kafka（cppkafka）
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(CPPKAFKA QUIET cppkafka)
    if(CPPKAFKA_FOUND)
        target_compile_definitions(message_service PRIVATE HAVE_CPPKAFKA=1)
        target_include_directories(message_service PRIVATE ${CPPKAFKA_INCLUDE_DIRS})
        target_link_libraries(message_service PRIVATE ${CPPKAFKA_LIBRARIES})
        message(STATUS "MessageService: cppkafka FOUND - enabling Kafka producer")
    else()
        message(WARNING "MessageService: cppkafka NOT found - Kafka producer disabled")
    endif()
endif()

# 可選接入 MariaDB/MySQL C API
find_path(MYSQL_INCLUDE_DIR mysql/mysql.h)
find_library(MYSQL_CLIENT_LIB mysqlclient)
if(MYSQL_INCLUDE_DIR AND MYSQL_CLIENT_LIB)
    target_compile_definitions(message_service PRIVATE HAVE_MARIADB=1)
    target_include_directories(message_service PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(message_service PRIVATE ${MYSQL_CLIENT_LIB})
    message(STATUS "MessageService: MySQL/MariaDB client FOUND - enabling DB persistence")
else()
    message(WARNING "MessageService: MySQL/MariaDB client NOT found - DB persistence disabled")
endif()


