cmake_minimum_required(VERSION 3.5)

project(SelfAmplifer LANGUAGES CXX C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
##修改MSVC的编码
if(MSVC)
    add_compile_options("/source-charset:utf-8")
    add_compile_options("/execution-charset:utf-8")
endif()
# 查找 Qt5 模块
find_package(Qt5 COMPONENTS Core Widgets SerialPort Network Bluetooth REQUIRED)

#add_subdirectory(test)

file(GLOB src "src/*")
file(GLOB Communication "src/Communication/*")
file(GLOB Protocols "src/Protocols/*")
file(GLOB Bluetooth "src/Bluetooth/*")


#add_library(${PROJECT_NAME} ${src} ${Communication} ${Protocols} ${Bluetooth})
add_library(${PROJECT_NAME}  SHARED ${src} ${Communication} ${Protocols} ${Bluetooth})
# 链接 Qt5 模块
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::SerialPort
    Qt5::Network
    Qt5::Bluetooth  # 添加蓝牙模块
)

target_include_directories(${PROJECT_NAME} PUBLIC "src")

