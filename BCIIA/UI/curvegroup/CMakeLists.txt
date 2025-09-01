cmake_minimum_required(VERSION 3.5)

project(curvegroup)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)

if(MSVC)
    add_compile_options("/source-charset:utf-8")
    add_compile_options("/execution-charset:utf-8")
    add_compile_options("/validate-charset:utf-8")
endif()
#添加测试
enable_testing()
add_subdirectory(curvewnd)
add_subdirectory(test)
#获取源文件
file(GLOB SRC ./src/*)
file(GLOB RES ./res/*.png)
set(CURVE_RES ${RES} CACHE PATH "res of curve")
find_package(Qt5 COMPONENTS core widgets charts REQUIRED)
add_library(curvegroup STATIC ${SRC})
target_include_directories(curvegroup PUBLIC "./src/")
target_link_libraries(curvegroup Qt5::Core Qt5::Widgets Qt5::Charts curvewnd)

install(TARGETS ${PROJECT_NAME} ARCHIVE)
