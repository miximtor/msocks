cmake_minimum_required(VERSION 3.9.4)
project(msocks)

set(CMAKE_CXX_STANDARD 17)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_ARCHITECTURE -x64)
set(Boost_USE_STATIC_RUNTIME ON)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(Botan2 2.10.0 REQUIRED)
find_package(Boost 1.69 COMPONENTS system coroutine random REQUIRED)

include_directories(${GLOG_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${BOTAN2_INCLUDE_DIRS})
include_directories(third_party/spdlog/include)
include_directories(src)

add_definitions(-DBOOST_ASIO_NO_DEPRECATED)
add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)

if (MSVC)
    add_definitions(-D_WIN32_WINNT=0x0A00)
    add_definitions(-DMSOCKS_MSVC)
elseif(MINGW)
    add_definitions(-DMSOCKS_MINGW)
elseif(UNIX)
    add_definitions(-DMSOCKS_UNIX)
endif()


add_subdirectory(${PROJECT_SOURCE_DIR}/src)

