cmake_minimum_required(VERSION 3.10.0)
project(curvewnd)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
add_compile_definitions(QCUSTOMPLOT_USE_OPENGL)
#添加测试
enable_testing()
add_subdirectory(test)
add_subdirectory(maxwidget)
#获取源文件
file(GLOB SRC ./src/*)

find_package(Qt5 COMPONENTS core widgets printsupport REQUIRED)
add_library(curvewnd STATIC ${SRC})
target_include_directories(curvewnd PUBLIC "./src/")
target_link_libraries(curvewnd Qt5::Core Qt5::Widgets Qt5::PrintSupport maxwidget opengl32.dll)

install(TARGETS ${PROJECT_NAME} ARCHIVE)
