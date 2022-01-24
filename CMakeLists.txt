cmake_minimum_required(VERSION 3.10)
project(FsdkMinimal)
set(TARGET_NAME "fsdk-minimal")
if (CMAKE_BUILD_TYPE STREQUAL "")
   message(STATUS "CMAKE_BUILD_TYPE not defined, 'Debug' will be used")
#    set(CMAKE_BUILD_TYPE "Release")
   set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}
#    -Wall -Werror -std=c++14 -fstrict-aliasing -pedantic-errors
#    -pedantic -Wno-deprecated-declarations -Wno-unused-variable")

set(CMAKE_CXX_FLAGS "-Wall -Wextra -fPIC -fopenmp  -DBOOST_LOG_DYN_LINK")

find_package(Boost COMPONENTS log system serialization filesystem REQUIRED)
include_directories (${Boost_INCLUDE_DIRS})

find_package(OpenCV  REQUIRED)
include_directories (${OpenCV_INCLUDE_DIRS})


file (GLOB MAIN_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*
)

file (GLOB MAIN_HEADERS
     ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h*
)

source_group("src" FILES ${MAIN_SRC})
source_group("include" FILES ${MAIN_HEADERS})

include_directories(
	/usr/include
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /home/firefly/common/face_sdk/include   # <- Change it to your FaceSDK location
)

add_executable(${TARGET_NAME} ${MAIN_SRC} ${MAIN_HEADERS})

target_link_libraries(${TARGET_NAME}  PRIVATE ${OpenCV_LIBRARIES}  ${Boost_LIBRARIES}
-ldl
-lpthread
)