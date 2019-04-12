#pragma once

#include <cstdint>

namespace msocks::constant
{

constexpr uint8_t SOCKS5_VERSION = 0x05;
constexpr uint8_t AUTH_NO_AUTH = 0x00;
constexpr uint8_t CONN_TCP = 0x01;
constexpr uint8_t CONN_BND = 0x02;
constexpr uint8_t CONN_UDP = 0x03;
constexpr uint8_t ADDR_IPV4 = 0x01;
constexpr uint8_t ADDR_DOMAIN = 0x03;
constexpr uint8_t ADDR_IPV6 = 0x04;

}

