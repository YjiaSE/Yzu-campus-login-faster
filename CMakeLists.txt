cmake_minimum_required(VERSION 3.16)

project(YzuCampusnetLogin VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)

add_executable(netool
    src/campus_net.cpp
    src/config.cpp
    src/http_client.cpp
    src/main.cpp
    src/self_service.cpp
    src/utils.cpp
)

target_include_directories(netool PRIVATE src)
target_link_libraries(netool PRIVATE CURL::libcurl)

if(MSVC)
    target_compile_options(netool PRIVATE /W4)
else()
    target_compile_options(netool PRIVATE -Wall -Wextra -Wpedantic)
endif()
