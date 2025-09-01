cmake_minimum_required(VERSION 3.5)

project(filter LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core REQUIRED)
##修改MSVC的编码
if(MSVC)
    add_compile_options("/source-charset:utf-8")
    add_compile_options("/execution-charset:utf-8")
endif()

add_subdirectory(test)
enable_testing()

file(GLOB SRC "src/*")
add_library(filter ${SRC})

target_link_libraries(filter PRIVATE Qt5::Core)
target_include_directories(filter PUBLIC "src")

