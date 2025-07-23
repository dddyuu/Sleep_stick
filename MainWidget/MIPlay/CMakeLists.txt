cmake_minimum_required(VERSION 3.5)

project(MIPlay LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_subdirectory(test)

file(GLOB src "src/*")
file(GLOB res "res/*.qrc")

set(MIPlayRes ${res} CACHE PATH "res of miplay")
add_library(${PROJECT_NAME} ${src})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets)

target_compile_definitions(${PROJECT_NAME} PRIVATE MIPlay_LIBRARY)

# 暴露头文件
target_include_directories(${PROJECT_NAME} PUBLIC "src")
