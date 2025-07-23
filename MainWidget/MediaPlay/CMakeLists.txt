cmake_minimum_required(VERSION 3.5)

project(MediaPlay LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets   multimedia multimediawidgets REQUIRED)

add_subdirectory(test)

file(GLOB src "src/*")
file(GLOB res "res/*.qrc")
set(MediaplayRes ${res} CACHE PATH "res of mediaplay")
add_library(MediaPlay  ${src} )

target_link_libraries(MediaPlay PRIVATE Qt5::Widgets Qt5::Multimedia Qt5::MultimediaWidgets)

target_compile_definitions(MediaPlay PRIVATE MEDIAPLAY_LIBRARY)

target_include_directories(MediaPlay PUBLIC "src")
