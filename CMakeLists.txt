cmake_minimum_required(VERSION 3.16)
project(BaseLib)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(SOURCE_FILES
    src/io/json.cpp
    src/io/xml.cpp
    src/io/filesystem.cpp
    src/net/tcp.cpp
    src/net/udp.cpp
    src/net/http.cpp
    src/net/http_server.cpp
    src/net/http_client.cpp
    src/core/logger.cpp
    src/util/config.cpp
    src/util/error.cpp
    src/util/lock.cpp
    src/util/thread.cpp
    src/util/thread_pool.cpp
    src/util/time.cpp
)

add_library(BaseLib STATIC ${SOURCE_FILES})

add_executable(test_all tests/test_all.cpp)
target_link_libraries(test_all PRIVATE BaseLib)

add_executable(test_smart_ptr tests/test_smart_ptr.cpp)
target_link_libraries(test_smart_ptr PRIVATE BaseLib)

# add_executable(test_random tests/test_random.cpp)
# target_link_libraries(test_random PRIVATE BaseLib)

add_executable(test_result tests/test_result.cpp)
target_link_libraries(test_result PRIVATE BaseLib)

add_executable(test_http tests/test_http.cpp)
target_link_libraries(test_http PRIVATE BaseLib)

add_executable(test_http_server tests/test_http_server.cpp)
target_link_libraries(test_http_server PRIVATE BaseLib)

add_executable(test_http_client tests/test_http_client.cpp)
target_link_libraries(test_http_client PRIVATE BaseLib)

add_executable(test_net tests/test_net.cpp)
target_link_libraries(test_net PRIVATE BaseLib)

add_executable(test_thread tests/test_thread.cpp)
target_link_libraries(test_thread PRIVATE BaseLib)

add_executable(test_util tests/test_util.cpp)
target_link_libraries(test_util PRIVATE BaseLib)

add_executable(test_logger tests/test_logger.cpp)
target_link_libraries(test_logger PRIVATE BaseLib)

add_executable(test_lock tests/test_lock.cpp)
target_link_libraries(test_lock PRIVATE BaseLib)

set_target_properties(BaseLib PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    OUTPUT_NAME "BaseLib"
)

install(TARGETS BaseLib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
