cmake_minimum_required(VERSION 3.16)

project(chat_microservices)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 依賴（可選）：未安裝時僅建立可執行骨架，以便逐步遷移
find_package(Protobuf QUIET)
find_package(gRPC QUIET)

if(Protobuf_FOUND AND gRPC_FOUND)
    set(MS_HAVE_GRPC ON)
    message(STATUS "Microservices: gRPC/Protobuf FOUND - full build enabled")
else()
    set(MS_HAVE_GRPC OFF)
    message(WARNING "Microservices: gRPC/Protobuf NOT found - building stubs only")
endif()

add_subdirectory(proto)
add_subdirectory(common)
add_subdirectory(gateway)
add_subdirectory(user_service)
add_subdirectory(social_service)
add_subdirectory(message_service)


