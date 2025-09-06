cmake_minimum_required(VERSION 3.5)

project(SelfAmplifer_test LANGUAGES CXX)

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
find_package(Qt5 COMPONENTS Core widgets Network Bluetooth REQUIRED)

add_executable(${PROJECT_NAME}   main.cpp )

target_link_libraries(${PROJECT_NAME} PRIVATE  Qt5::Widgets Qt5::SerialPort Qt5::Network Qt5::Bluetooth SelfAmplifer)

target_compile_definitions(${PROJECT_NAME} PRIVATE BPAMPLIFIER_LIBRARY)

target_include_directories(${PROJECT_NAME} PUBLIC "src")
