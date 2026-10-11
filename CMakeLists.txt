cmake_minimum_required(VERSION 3.10)
project(webserver CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -pthread")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_executable(webserver
    src/main.cpp
    src/http_server.cpp
    src/thread_pool.cpp
)

target_link_libraries(webserver pthread)
