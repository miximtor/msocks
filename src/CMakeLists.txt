set(SESSION_FILE
        session/client_session.hpp
        session/client_session.cpp
        session/server_session.cpp
        session/server_session.hpp
        )

set(ENDPOINT_FILE
        endpoint/server.cpp
        endpoint/server.hpp
        endpoint/client.cpp endpoint/client.hpp)

set(CONFIG_FILE
        config/config.hpp
        )

set(UTILITY_FILE
        utility/socks_constants.hpp
        )

include_directories(${PROJECT_SOURCE_DIR}/src/)

add_executable(${PROJECT_NAME}
        main.cpp
        ${SESSION_FILE}
        ${UTILITY_FILE}
        ${ENDPOINT_FILE}
        ${CONFIG_FILE}
        )
target_link_libraries(${PROJECT_NAME} glog  boost_system-mt.dll cryptopp.dll boost_coroutine-mt.dll ws2_32 wsock32)
