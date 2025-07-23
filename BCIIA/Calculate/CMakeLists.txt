cmake_minimum_required(VERSION 3.5)

project(Calculate)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#设置模型相对路径

#onnxruntime的库
#set(onnxruntime_DIR "D:\\ProgramData\\fatigue911\\onnxruntime\\lib\\cmake\\onnxruntime")
#set(onnxruntime_DLL_DIR "D:\\ProgramData\\fatigue911\\onnxruntime\\bin")

#file(GLOB onnxruntime_DLL ${onnxruntime_DLL_DIR}/*.dll)
#set(onnxruntime_DLL ${onnxruntime_DLL} CACHE PATH "dll of onnxruntime")
#file(COPY ${onnxruntime_DLL} DESTINATION .)
#find_package(onnxruntime REQUIRED)

file(GLOB src "src/*")
add_library(Calculate  ${src})



target_compile_definitions(Calculate PRIVATE CALCULATE_LIBRARY)
target_include_directories(Calculate PUBLIC "src" )
#target_link_libraries(Calculate onnxruntime::onnxruntime)

target_include_directories(Calculate PUBLIC "C:/cognitivePlatform_need/onnxruntime/onnxruntime1_9/include")
target_link_libraries(Calculate PUBLIC "C:/cognitivePlatform_need/onnxruntime/onnxruntime1_9/lib/onnxruntime.lib")


#功能测试
add_executable(deeplearn_test test/deeplearn.cpp)
target_link_libraries(deeplearn_test Calculate)

add_executable(brainrecognition_test test/brainrecognition.cpp)
target_link_libraries(brainrecognition_test Calculate)

add_executable(tskfatigue_test test/tskfatigue.cpp)
target_link_libraries(tskfatigue_test Calculate)
