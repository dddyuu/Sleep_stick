cmake_minimum_required(VERSION 3.10.0)
project(SleepSystem LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



add_definitions(-pthread) # 使能线程支持
add_definitions(-exceptions) # 使能异常支持
add_definitions(-frtti) # 使能RTTI支持

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(Qt5_DIR "D:/Qt/Qt5.14.2/5.14.2/msvc2017_64/lib/cmake/Qt5")

find_package(Qt5 COMPONENTS Core Widgets Concurrent REQUIRED)
set(Eigen_Dir "${CMAKE_SOURCE_DIR}/Eigen")
include_directories(Eigen_Dir)


#find_package(Qt5 COMPONENTS Widgets Core REQUIRED)
if(MSVC)
    add_compile_options("/source-charset:utf-8")
    add_compile_options("/execution-charset:utf-8")
endif()
add_subdirectory(MainWidget)
add_subdirectory(BCIIA)
add_subdirectory(fatigue)
add_subdirectory(sleep)
add_subdirectory(SelfAmplifer)
file(GLOB src "src/*")
file(GLOB res "res/*.qrc")
file(COPY ${MATDLL} DESTINATION ".")
file(COPY ${onnxruntime_DLL} DESTINATION .)
file(GLOB ui_files 
    "src/*.ui"  # 包含所有的UI文件
)
add_executable(SleepSystem   main.cpp ${src} ${res}
    ${bcimonitor_res} ${AmpliferRes} 
    ${MediaplayRes} ${MIPlayRes} ${ui_files})

target_link_libraries(SleepSystem PRIVATE Qt5::Widgets Qt5::Core
                        MainWidget Qt5::Concurrent  BCIIA  FatigueResWidget SleepResWidget)



#target_link_libraries(FatigueSystem PUBLIC "${CMAKE_SOURCE_DIR}\\qBreakpad\\lib\\release\\qBreakpad.lib")
#target_include_directories(FatigueSystem PUBLIC "${CMAKE_SOURCE_DIR}\\qBreakpad\\include")

target_include_directories(SleepSystem PUBLIC "src" ${Eigen_Dir})

#add_executable(preprocessing_test "test.cpp"  ${src})

#target_include_directories(preprocessing_test PUBLIC "src" ${Eigen_Dir})
#target_link_libraries(preprocessing_test PRIVATE Qt5::Widgets
#                        MainWidget  BCIIA  FatigueResWidget)
