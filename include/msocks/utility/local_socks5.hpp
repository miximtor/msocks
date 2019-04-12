#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <msocks/utility/socks_constants.hpp>
#include <msocks/utility/socks_erorr.hpp>
#include <msocks/usings.hpp>
#include <spdlog/spdlog.h>

#define MSOCKS_PACK(DEC) DEC __attribute__((__packed__))

namespace utility
{
namespace detail
{
using Result = async_result<yield_context, void(error_code, std::string)>;
using Handler = Result::completion_handler_type;

void do_local_socks5(
  io_context::strand &strand,
  ip::tcp::socket &local,
  Handler handler,
  yield_context yield
);
}

detail::Result::return_type
async_local_socks5(
  io_context::strand &strand,
  ip::tcp::socket &local,
  yield_context yield)
{
  detail::Handler handler(yield);
  detail::Result result(handler);
  spawn(
    strand,
    [&strand, &local, handler](yield_context yield)
    { detail::do_local_socks5(strand, local, handler, yield); }
  );
  return result.get();
}

namespace detail
{

void do_local_socks5(
  io_context::strand &strand,
  ip::tcp::socket &local,
  Handler handler,
  yield_context yield)
{
  namespace ms = msocks;
  namespace msc = ms::constant;
  std::array<uint8_t, 256 + 2> temp{};
  MSOCKS_PACK(
    struct
    {
      uint8_t version;
      uint8_t n_method;
    })
    auth_method{};
  MSOCKS_PACK(
    struct
    {
      uint8_t version;
      uint8_t method;
    }
  ) auth_reply{msc::SOCKS5_VERSION, msc::AUTH_NO_AUTH};
  
  MSOCKS_PACK (
    struct
    {
      uint8_t version;
      uint8_t cmd;
      uint8_t reserve;
      uint8_t addr_type;
    })
    request{};
  
  MSOCKS_PACK (
    struct
    {
      uint8_t version;
      uint8_t rep;
      uint8_t rsv;
      uint8_t atyp;
      uint32_t bnd_addr;
      uint16_t bnd_port;
    })
    reply{msc::SOCKS5_VERSION, 0x00, 0x00, msc::ADDR_IPV4, 0x00000000, 0x0000};
  
  std::string result;
  error_code ec;
  try
  {
    async_read(local, buffer(&auth_method, sizeof(auth_method)), yield);
    
    if ( auth_method.version != msc::SOCKS5_VERSION )
      throw system_error(ms::errc::version_not_supported, ms::socks_category());
    
    async_read(local, buffer(temp, auth_method.n_method), yield);
    bool find_auth = false;
    for ( int i = 0; i < auth_method.n_method && !find_auth; i++ )
      find_auth = temp[i] == msc::AUTH_NO_AUTH;
    
    if ( !find_auth )
      throw system_error(ms::errc::auth_not_found, ms::socks_category());
    
    async_write(local, buffer(&auth_reply, sizeof(auth_reply)), yield);
    
    async_read(local, buffer(&request, sizeof(request)), yield);
    
    if ( request.version != msc::SOCKS5_VERSION )
      throw system_error(ms::errc::version_not_supported, ms::socks_category());
    
    switch ( request.cmd )
    {
      case msc::CONN_TCP:
        break;
      case msc::CONN_BND:
      case msc::CONN_UDP:
      default:
        throw system_error(ms::errc::cmd_not_supported, ms::socks_category());
    }
    
    if ( request.addr_type == msc::ADDR_IPV4 )
    {
      async_read(local, buffer(temp, 32 / 8 + 2), yield);
      MSOCKS_PACK(
        struct
        {
          uint32_t addr;
          uint16_t port;
        }
      )
        address_port{};
      
      buffer_copy(buffer(&address_port, sizeof(address_port)), buffer(temp),
                  sizeof(address_port));
      result =
        ip::make_address_v4(big_to_native(address_port.addr)).to_string() +
        ":" +
        std::to_string(big_to_native(address_port.port));
    }
    else if ( request.addr_type == msc::ADDR_IPV6 )
    {
      async_read(local, buffer(temp, 128 / 8 + 2), yield);
      MSOCKS_PACK(
        struct
        {
          ip::address_v6::bytes_type addr;
          uint16_t port;
        })
        address_port{};
      buffer_copy(buffer(&address_port, sizeof(address_port)), buffer(temp),
                  sizeof(address_port));
      std::reverse(address_port.addr.begin(), address_port.addr.end());
      result =
        ip::make_address_v6(address_port.addr).to_string() +
        ":" +
        std::to_string(big_to_native(address_port.port));
    }
    else if ( request.addr_type == msc::ADDR_DOMAIN )
    {
      uint8_t domain_length;
      async_read(local, buffer(&domain_length, sizeof(domain_length)), yield);
      async_read(local, dynamic_buffer(result), transfer_exactly(domain_length),
                 yield);
      uint16_t port = 0;
      async_read(local, buffer(&port, sizeof(port)), yield);
      big_to_native_inplace(port);
      result += ":" + std::to_string(port);
    }
    else
    {
      throw system_error(ms::errc::address_not_supported, ms::socks_category());
    }
    async_write(local, buffer(&reply, sizeof(reply)), yield);
  }
  catch ( system_error &e )
  {
    ec = e.code();
  }
  auto cb = [ec,result,handler]() mutable {handler(ec,result);};
  post(strand,std::move(cb));
}
}
}
