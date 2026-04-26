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
    src/util/config.cpp
    src/util/thread_pool.cpp
    src/util/time.cpp
)

add_library(BaseLib STATIC ${SOURCE_FILES})

add_executable(test_all tests/test_all.cpp)
target_link_libraries(test_all PRIVATE BaseLib)

add_executable(test_random tests/test_random.cpp)
target_link_libraries(test_random PRIVATE BaseLib)

add_executable(test_result tests/test_result.cpp)
target_link_libraries(test_result PRIVATE BaseLib)

add_executable(test_http tests/test_http.cpp)
target_link_libraries(test_http PRIVATE BaseLib)

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
