cmake_minimum_required(VERSION 3.5)

project(Amplifier LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options("/source-charset:utf-8")
    add_compile_options("/execution-charset:utf-8")
    add_compile_options("/validate-charset:utf-8")
endif()

add_subdirectory(test)
enable_testing()

add_subdirectory(filter)

find_package(Qt5 COMPONENTS Core Widgets quickwidgets REQUIRED)
file(GLOB src "src/*")
file(GLOB res "res/*.qrc")
set(AmpliferRes ${res} CACHE PATH "res of amplifier")
add_library(Amplifier ${src} ${res})

target_link_libraries(Amplifier PRIVATE Qt5::Core Qt5::Widgets Qt5::QuickWidgets )
target_link_libraries(Amplifier PUBLIC filter)
target_include_directories(Amplifier PUBLIC src)
