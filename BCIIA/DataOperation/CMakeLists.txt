cmake_minimum_required(VERSION 3.5)

project(DataOperation LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(Amplifier)
add_subdirectory(FileStorage)
file(GLOB src "src/*")

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_library(DataOperation ${src})
target_link_libraries(DataOperation PUBLIC Qt5::Widgets Amplifier FileStorage)

target_include_directories(DataOperation PUBLIC "src")

add_executable(DataOperation_test main.cpp)
target_link_libraries(DataOperation_test PRIVATE DataOperation)