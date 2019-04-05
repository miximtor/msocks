set(SESSION_FILE
        session/client_session.hpp
        session/client_session.cpp
        session/server_session.cpp
        session/server_session.hpp
        )

set(ENDPOINT_FILE
        endpoint/server.cpp
        endpoint/server.hpp
        endpoint/client.cpp endpoint/client.hpp endpoint/endpoint.hpp)

set(CONFIG_FILE
        config/config.hpp
        )

set(UTILITY_FILE
        utility/socks_constants.hpp
        utility/socket_pair.hpp
        utility/signal_handle.hpp)

include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${BOTAN2_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}
        ${SESSION_FILE}
        ${UTILITY_FILE}
        ${ENDPOINT_FILE}
        ${CONFIG_FILE}
        main.cpp
        )

IF(WIN32)
    target_link_libraries(${PROJECT_NAME} ${GLOG_LIBRARIES} ${BOTAN2_LIBRARIES} boost_system-mt boost_coroutine-mt ws2_32 wsock32 )
ELSE(WIN32)
    target_link_libraries(${PROJECT_NAME} ${GLOG_LIBRARIES} ${BOTAN2_LIBRARIES} boost_system boost_coroutine pthread)
ENDIF(WIN32)
