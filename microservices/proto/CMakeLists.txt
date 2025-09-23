cmake_minimum_required(VERSION 3.16)

set(PROTO_FILES
    common.proto
    user_service.proto
    social_service.proto
    message_service.proto
)

if(MS_HAVE_GRPC)
    # 生成 Protobuf / gRPC C++ 程式碼並封裝為單一 target 供其他子專案鏈接
    find_package(Protobuf REQUIRED)
    find_package(gRPC REQUIRED)

    set(GENERATED_PROTO_SRCS)
    set(GENERATED_PROTO_HDRS)
    set(GENERATED_GRPC_SRCS)
    set(GENERATED_GRPC_HDRS)

    foreach(proto ${PROTO_FILES})
        get_filename_component(proto_abspath ${proto} ABSOLUTE)
        get_filename_component(proto_path ${proto_abspath} PATH)
        get_filename_component(proto_name ${proto} NAME_WE)

        protobuf_generate_cpp(${proto_name}_PB_SRCS ${proto_name}_PB_HDRS ${proto_abspath})
        grpc_generate_cpp(${proto_name}_GRPC_SRCS ${proto_name}_GRPC_HDRS ${proto_abspath})

        list(APPEND GENERATED_PROTO_SRCS ${${proto_name}_PB_SRCS})
        list(APPEND GENERATED_PROTO_HDRS ${${proto_name}_PB_HDRS})
        list(APPEND GENERATED_GRPC_SRCS ${${proto_name}_GRPC_SRCS})
        list(APPEND GENERATED_GRPC_HDRS ${${proto_name}_GRPC_HDRS})
    endforeach()

    add_library(chat_protos STATIC
        ${GENERATED_PROTO_SRCS}
        ${GENERATED_GRPC_SRCS}
    )
    target_link_libraries(chat_protos PUBLIC protobuf::libprotobuf gRPC::grpc++)
    target_include_directories(chat_protos PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
endif()


