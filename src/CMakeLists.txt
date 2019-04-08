set(SESSION_FILE
        session/client_session.hpp
        session/client_session.cpp
        session/server_session.cpp
        session/server_session.hpp
        session/session.hpp)

set(ENDPOINT_FILE
        endpoint/server.cpp
        endpoint/server.hpp
        endpoint/client.cpp
        endpoint/client.hpp
        endpoint/endpoint.hpp
        )

set(CONFIG_FILE
        config/config.hpp
        )

set(UTILITY_FILE
        utility/socks_constants.hpp
        utility/socket_pair.hpp
        utility/signal_handle.hpp
        utility/limiter.hpp
        utility/local_socks5.hpp utility/socks_erorr.hpp)


add_executable(${PROJECT_NAME}
        ${SESSION_FILE}
        ${UTILITY_FILE}
        ${ENDPOINT_FILE}
        ${CONFIG_FILE}
        main.cpp
        )


IF (WIN32)
    target_link_options(${PROJECT_NAME} PRIVATE -fstack-protector)
    target_link_libraries(${PROJECT_NAME} Botan2::Botan2 Boost::system Boost::random Boost::coroutine ws2_32 wsock32)
ELSE ()
    target_link_libraries(${PROJECT_NAME} Botan2::Botan2 Boost::system Boost::random Boost::coroutine pthread)
ENDIF ()
