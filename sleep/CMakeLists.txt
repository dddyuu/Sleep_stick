cmake_minimum_required(VERSION 3.5)

project(SleepResWidget LANGUAGES CXX)

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

#include_directories(D:/ProgramData/qTproject/CognitivePlatform_demo0817/eigen-3.4.0)
include_directories(C:/cognitivePlatform_need/eigen3/eigen-3.3.9)
add_subdirectory(test)
find_package(Qt5 COMPONENTS Core widgets charts REQUIRED)
find_package(Qt5 COMPONENTS Core network  REQUIRED)
file(GLOB src "src/*")

add_library(SleepResWidget  ${src})

target_link_libraries(${PROJECT_NAME} PUBLIC Qt5::Core Qt5::Widgets Qt5::Charts Qt5::Network)

target_compile_definitions(SleepResWidget PRIVATE SLEEPRESWIDGET_LIBRARY)

target_include_directories(SleepResWidget PUBLIC "src")

#add_executable(SleepResWidget_test test/main.cpp)