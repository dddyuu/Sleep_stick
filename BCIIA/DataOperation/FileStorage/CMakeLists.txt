cmake_minimum_required(VERSION 3.5)

project(FileStorage LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(test)
enable_testing()
find_package(Qt5 COMPONENTS Core widgets Network REQUIRED)

##添加MAT读取模块
add_subdirectory(MAT)

file(GLOB SRC "./src/*")
add_library(FileStorage ${SRC}
)

target_link_libraries(FileStorage  Qt5::Core Qt5::Widgets Qt5::Network MAT)
target_include_directories(FileStorage PUBLIC "./src" )

