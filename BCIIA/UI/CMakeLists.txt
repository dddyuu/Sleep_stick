cmake_minimum_required(VERSION 3.5)

project(UI LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(curvegroup)
file(GLOB src "src/*")

file(GLOB res "res/*.qrc")
set(bcimonitor_res ${res} CACHE PATH "res of bcimonitor")
find_package(Qt5 COMPONENTS core REQUIRED)

add_library(UI ${src})

target_link_libraries(UI PUBLIC Qt5::Core curvegroup)

target_include_directories(UI PUBLIC "src")

# ui≤‚ ‘
add_executable(UI_test "test/main.cpp" ${res})
target_link_libraries(UI_test PUBLIC UI)

