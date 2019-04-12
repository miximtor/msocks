cmake_minimum_required(VERSION 3.9.2)
project(msocks)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

find_package(Threads REQUIRED)
find_package(Botan2 2.9.0 REQUIRED)
find_package(Boost 1.69 COMPONENTS system coroutine random thread REQUIRED)
if (BOTAN2_FOUND)
    message("Use Botan2: ${BOTAN2_LIBRARIES}")
endif()
if (Boost_FOUND)
    message("Use Boost: ${Boost_LIBRARIES}")
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${BOTAN2_INCLUDE_DIRS})
include_directories(third_party/spdlog/include)
include_directories(${PROJECT_SOURCE_DIR}/include)

add_definitions(-DBOOST_ASIO_NO_DEPRECATED)
add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)


if (WIN32)
    set(WIN_EXTRA_LIB ws2_32 wsock32)
else()
    set(WIN_EXTRA_LIB)
endif()

file(GLOB MSOCKS_INCLUDE include/msocks/*hpp)
file(GLOB MSOCKS_INCLUDE_ENDPOINT include/msocks/endpoint/*.hpp)
file(GLOB MSOCKS_INCLUDE_SESSION include/msocks/session/*.hpp)
file(GLOB MSOCKS_INCLUDE_UTILITY include/msocks/utility/*.hpp)
file(GLOB MSOCKS_SRC_SESSION src/session/*.cpp)
file(GLOB MSOCKS_SRC_ENDPOINT src/endpoint/*.cpp)

add_executable(msocks
        ${MSOCKS_INCLUDE}
        ${MSOCKS_INCLUDE_SESSION}
        ${MSOCKS_INCLUDE_ENDPOINT}
        ${MSOCKS_INCLUDE_UTILITY}
        ${MSOCKS_SRC_SESSION}
        ${MSOCKS_SRC_ENDPOINT}
        src/main.cpp)

target_link_options(msocks PRIVATE -fstack-protector)
target_link_libraries(msocks Boost::coroutine Boost::random Boost::system Boost::thread Botan2::Botan2 ${CMAKE_THREAD_LIBS_INIT} ${WIN_EXTRA_LIB})