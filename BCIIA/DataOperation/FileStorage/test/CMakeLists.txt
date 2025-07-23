cmake_minimum_required(VERSION 3.5)

project(FileStorage_lib LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(COPY ${MATDLL} DESTINATION ".")
find_package(Qt5 COMPONENTS Core widgets REQUIRED)
add_executable(FileStorage_lib main.cpp
)

target_link_libraries(FileStorage_lib  FileStorage  Qt5::Widgets)
