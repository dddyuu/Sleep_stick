cmake_minimum_required(VERSION 3.5)

project(MainWidget LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    add_compile_options("/source-charset:utf-8")
    add_compile_options("/execution-charset:utf-8")
endif()
add_subdirectory(test)
add_subdirectory(MediaPlay)
add_subdirectory(MIPlay)

find_package(Qt5 COMPONENTS Core widgets charts REQUIRED)

file(GLOB src "src/*")
add_library(MainWidget  ${src})

target_link_libraries(MainWidget PRIVATE Qt5::Core Qt5::Widgets Qt5::Charts)
target_link_libraries(MainWidget PUBLIC MediaPlay MIPlay)
target_compile_definitions(MainWidget PRIVATE RESULTWIDGET_LIBRARY)

target_include_directories(MainWidget PUBLIC "src")
