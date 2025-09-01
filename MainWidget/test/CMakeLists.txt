cmake_minimum_required(VERSION 3.5)

project(MainWidget_test LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets   multimedia multimediawidgets REQUIRED)

#file(GLOB src "src/*")

add_executable(MainWidget_test  main.cpp)

target_link_libraries(MainWidget_test PRIVATE Qt5::Widgets Qt5::Multimedia Qt5::MultimediaWidgets MainWidget)

target_compile_definitions(MainWidget_test PRIVATE MEDIAPLAY_LIBRARY)
