cmake_minimum_required(VERSION 3.13)
project(msocks)

set(CMAKE_CXX_STANDARD 17)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -Wall")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/modules)

find_package(Boost 1.67 COMPONENTS system coroutine)
if (NOT Boost_FOUND)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost)
endif()
find_package(Botan2 2.8.0 REQUIRED)

add_definitions(-DBOOST_ASIO_NO_DEPRECATED)
add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)
add_definitions(-DGLOG_NO_ABBREVIATED_SEVERITIES)
add_subdirectory(${PROJECT_SOURCE_DIR}/src)
