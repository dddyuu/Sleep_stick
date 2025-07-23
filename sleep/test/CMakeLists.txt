cmake_minimum_required(VERSION 3.5)

project(SleepResWidget_test LANGUAGES CXX)

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

add_executable(SleepResWidget_test  main.cpp)

target_include_directories(SleepResWidget_test PRIVATE
    $<TARGET_PROPERTY:SleepResWidget,INTERFACE_INCLUDE_DIRECTORIES>
    ${CMAKE_SOURCE_DIR}/src    # 或者写绝对路径
)

target_link_libraries(SleepResWidget_test PRIVATE Qt5::Widgets SleepResWidget)

target_compile_definitions(SleepResWidget_test PRIVATE SLEEPRESWIDGET_LIBRARY)
