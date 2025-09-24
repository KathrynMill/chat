cmake_minimum_required(VERSION 3.16)
project(chat_common)

add_library(chat_common STATIC
    db/Db.cpp
    db/ConnectionPool.cpp
    consul/ConsulClient.cpp
    circuit/CircuitBreaker.cpp
    circuit/CircuitBreakerManager.cpp
    jwt/JwtValidator.cpp
    auth/AuthManager.cpp
    discovery/ServiceDiscovery.cpp
    retry/RetryManager.cpp
    tracing/Tracer.cpp
    metrics/MetricsCollector.cpp
    config/ConfigManager.cpp
    security/TlsManager.cpp
    security/TlsIntegration.cpp
    logging/Logger.cpp
    observability/ObservabilityManager.cpp
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

# 可選依賴：CURL（用於 Consul HTTP 請求）
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(CURL QUIET libcurl)
    if(CURL_FOUND)
        target_compile_definitions(chat_common PRIVATE HAVE_CURL=1)
        target_include_directories(chat_common PRIVATE ${CURL_INCLUDE_DIRS})
        target_link_libraries(chat_common PRIVATE ${CURL_LIBRARIES})
        message(STATUS "Common: libcurl FOUND - enabling Consul HTTP client")
    else()
        message(WARNING "Common: libcurl NOT found - Consul client disabled")
    endif()
endif()

# 可選依賴：OpenSSL（用於 JWT HMAC-SHA256）
find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
    target_compile_definitions(chat_common PRIVATE HAVE_OPENSSL=1)
    target_link_libraries(chat_common PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    message(STATUS "Common: OpenSSL FOUND - enabling JWT HMAC-SHA256")
else()
    message(WARNING "Common: OpenSSL NOT found - JWT using fallback")
endif()

# JSON 庫（nlohmann/json）
find_package(nlohmann_json QUIET)
if(nlohmann_json_FOUND)
    target_compile_definitions(chat_common PRIVATE HAVE_JSON=1)
    target_link_libraries(chat_common PRIVATE nlohmann_json::nlohmann_json)
    message(STATUS "Common: nlohmann_json FOUND")
else()
    # 使用本地 json.hpp
    target_include_directories(chat_common PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty)
    message(STATUS "Common: using local json.hpp")
endif()

# 可選依賴：OpenTelemetry（用於分散式追蹤）
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(OPENTELEMETRY QUIET opentelemetry-cpp)
    if(OPENTELEMETRY_FOUND)
        target_compile_definitions(chat_common PRIVATE HAVE_OPENTELEMETRY=1)
        target_include_directories(chat_common PRIVATE ${OPENTELEMETRY_INCLUDE_DIRS})
        target_link_libraries(chat_common PRIVATE ${OPENTELEMETRY_LIBRARIES})
        message(STATUS "Common: OpenTelemetry FOUND - enabling distributed tracing")
    else()
        message(WARNING "Common: OpenTelemetry NOT found - tracing disabled")
    endif()
endif()

# 可選依賴：Prometheus（用於指標監控）
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PROMETHEUS QUIET prometheus-cpp)
    if(PROMETHEUS_FOUND)
        target_compile_definitions(chat_common PRIVATE HAVE_PROMETHEUS=1)
        target_include_directories(chat_common PRIVATE ${PROMETHEUS_INCLUDE_DIRS})
        target_link_libraries(chat_common PRIVATE ${PROMETHEUS_LIBRARIES})
        message(STATUS "Common: Prometheus FOUND - enabling metrics collection")
    else()
        message(WARNING "Common: Prometheus NOT found - metrics disabled")
    endif()
endif()

# 可選依賴：spdlog（用於結構化日誌）
find_package(spdlog QUIET)
if(spdlog_FOUND)
    target_compile_definitions(chat_common PRIVATE HAVE_SPDLOG=1)
    target_link_libraries(chat_common PRIVATE spdlog::spdlog)
    message(STATUS "Common: spdlog FOUND - enabling structured logging")
else()
    message(WARNING "Common: spdlog NOT found - using basic logging")
endif()

# 示例程序
add_executable(EnterpriseFeaturesExample examples/EnterpriseFeaturesExample.cpp)
target_link_libraries(EnterpriseFeaturesExample chat_common)

add_executable(AdvancedFeaturesExample examples/AdvancedFeaturesExample.cpp)
target_link_libraries(AdvancedFeaturesExample chat_common)




