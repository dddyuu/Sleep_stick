cmake_minimum_required(VERSION 3.5)

project(Amplifier_test LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Qt5 COMPONENTS Core Widgets REQUIRED)


add_executable(Amplifier_test main.cpp)

target_link_libraries(Amplifier_test PRIVATE Qt5::Core Qt5::Widgets Amplifier)
