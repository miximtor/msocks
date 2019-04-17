cmake_minimum_required(VERSION 3.9.2)

project(msocks)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

set(Boost_ARCHITECTURE -x64)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})


find_package(Threads REQUIRED)
find_package(Botan2 2.9.0 REQUIRED)
find_package(Boost 1.69 COMPONENTS system coroutine random thread REQUIRED)
add_subdirectory(third_party/spdlog)
include_directories(${BOOST_LIBRARIES})
include_directories(${BOTAN2_INCLUDE_DIRS})
include_directories(third_party/spdlog/include)
include_directories(include)

add_definitions(-DBOOST_ASIO_NO_DEPRECATED)
add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)
if (MSVC)
	add_definitions(-D_WIN32_WINNT=0x0601)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	set(CompilerFlags
			CMAKE_CXX_FLAGS_DEBUG
			CMAKE_CXX_FLAGS_RELEASE
			CMAKE_CXX_FLAGS_MINSIZEREL
			CMAKE_CXX_FLAGS_RELWITHDEBINFO
			CMAKE_C_FLAGS_DEBUG
			CMAKE_C_FLAGS_RELEASE
			CMAKE_C_FLAGS_MINSIZEREL
			CMAKE_C_FLAGS_RELWITHDEBINFO
		)
# REPLACE MD(MultiThread DLL) TO MT (MultiThread) 
	foreach(CompilerFlag ${CompilerFlags})
		string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
endif ()


file(GLOB MSOCKS_INCLUDE include/msocks/*hpp)
file(GLOB MSOCKS_INCLUDE_ENDPOINT include/msocks/endpoint/*.hpp)
file(GLOB MSOCKS_INCLUDE_SESSION include/msocks/session/*.hpp)
file(GLOB MSOCKS_INCLUDE_UTILITY include/msocks/utility/*.hpp)
file(GLOB MSOCKS_SRC_SESSION src/session/*.cpp)
file(GLOB MSOCKS_SRC_ENDPOINT src/endpoint/*.cpp)
file(GLOB MSOCKS_SRC_UTILITY src/utility/*.cpp)

add_executable(msocks
        ${MSOCKS_INCLUDE}
        ${MSOCKS_INCLUDE_SESSION}
        ${MSOCKS_INCLUDE_ENDPOINT}
        ${MSOCKS_INCLUDE_UTILITY}
        ${MSOCKS_SRC_SESSION}
        ${MSOCKS_SRC_ENDPOINT}
        ${MSOCKS_SRC_UTILITY}
        src/main.cpp)

target_link_libraries(msocks ${BOTAN2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_link_libraries(msocks stdc++)
endif ()

 target_link_libraries(msocks Boost::coroutine Boost::system Boost::random Boost::thread)

if (WIN32)
    target_link_libraries(msocks ws2_32 wsock32)
    if (MSVC)
        target_link_libraries(msocks bcrypt)
    endif ()
endif ()


if (UNIX)
    install(TARGETS msocks DESTINATION ${CMAKE_INSTALL_BINDIR}/bin)
    install(FILES daemon/msocksd.service DESTINATION /etc/systemd/system/)
endif ()