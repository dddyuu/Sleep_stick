cmake_minimum_required(VERSION 3.5)

project(BCIIA LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
#set(Qt5_DIR "D:\\Qt\\5.14.2\\msvc2017_64\\lib\\cmake\\Qt5")
set(Qt5_DIR "D:/Qt/Qt5.14.2/5.14.2/msvc2017_64/lib/cmake/Qt5")
find_package(Qt5 COMPONENTS Widgets REQUIRED)
##ÐÞ¸ÄMSVCµÄ±àÂë
if(MSVC)
    add_compile_options("/source-charset:utf-8")
    add_compile_options("/execution-charset:utf-8")
endif()
add_subdirectory(Calculate)
add_subdirectory(DataOperation)
add_subdirectory(UI)
file(GLOB src "src/*")
add_library(BCIIA ${src})
target_link_libraries(BCIIA PUBLIC Qt5::Widgets Calculate DataOperation UI)
target_include_directories(BCIIA PUBLIC "src")

#²âÊÔ
add_executable(BCIIA_test "test/main.cpp" ${bcimonitor_res} ${AmpliferRes})
file(COPY ${MATDLL} DESTINATION ".")
target_link_libraries(BCIIA_test PUBLIC BCIIA)