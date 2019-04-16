#pragma once

#include <cstdint>

namespace msocks::socks
{

constexpr uint8_t socks5_version = 0x05;
constexpr uint8_t auth_no_auth = 0x00;
constexpr uint8_t conn_tcp = 0x01;
constexpr uint8_t conn_bind = 0x02;
constexpr uint8_t conn_udp = 0x03;
constexpr uint8_t addr_ipv4 = 0x01;
constexpr uint8_t addr_domain = 0x03;
constexpr uint8_t addr_ipv6 = 0x04;

}

