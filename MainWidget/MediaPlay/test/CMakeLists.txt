cmake_minimum_required(VERSION 3.5)

project(MediaPlay_test LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets   multimedia multimediawidgets REQUIRED)

#file(GLOB src "src/*")
#message("mediaplay:${MediaplayRes}")
add_executable(MediaPlay_test  main.cpp ${MediaplayRes})

target_link_libraries(MediaPlay_test PRIVATE Qt5::Widgets Qt5::Multimedia Qt5::MultimediaWidgets MediaPlay)

target_compile_definitions(MediaPlay_test PRIVATE MEDIAPLAY_LIBRARY)
