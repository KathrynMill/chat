cmake_minimum_required(VERSION 3.16)
project(chat_common)

add_library(chat_common STATIC
    db/Db.cpp
)

target_include_directories(chat_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_library(MARIADB_CLIENT mariadb)
if(NOT MARIADB_CLIENT)
    # 有些系統安裝為 mysqlclient
    find_library(MYSQL_CLIENT mysqlclient)
    if(MYSQL_CLIENT)
        set(MARIADB_CLIENT ${MYSQL_CLIENT})
    endif()
endif()

if(MARIADB_CLIENT)
    target_compile_definitions(chat_common PUBLIC HAVE_MARIADB=1)
    target_link_libraries(chat_common PUBLIC ${MARIADB_CLIENT})
    message(STATUS "chat_common: MariaDB/MySQL client FOUND")
else()
    message(WARNING "chat_common: MariaDB/MySQL client NOT found - building without DB")
endif()


