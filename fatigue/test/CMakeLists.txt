cmake_minimum_required(VERSION 3.5)

project(FatigueResWidget_test LANGUAGES CXX)

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

find_package(Qt5 COMPONENTS Core widgets  REQUIRED)

add_executable(FatigueResWidget_test  main.cpp)

target_link_libraries(FatigueResWidget_test PRIVATE Qt5::Widgets FatigueResWidget)

target_compile_definitions(FatigueResWidget_test PRIVATE FATIGUERESWIDGET_LIBRARY)
