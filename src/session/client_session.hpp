#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cryptopp/secblock.h>
#include <cryptopp/salsa.h>
#include <botan/stream_cipher.h>
#include <usings.hpp>


namespace msocks
{
class client_session :
  public noncopyable,
  public std::enable_shared_from_this<client_session>
{
public:
  
  client_session(
    io_context::strand &strand_,
    ip::tcp::socket local_,
    const ip::tcp::endpoint &addr_,
    const std::vector<uint8_t> &key_
  );
  
  void go();

private:
  void start(yield_context yield);
  
  std::pair<bool, std::string> do_local_socks5(yield_context yield);
  
  bool local_socks_auth(yield_context yield);
  
  std::pair<bool, std::string> get_host_service(yield_context yield);
  
  void handshake(yield_context yield, std::string host_service);
  
  void do_forward_local_to_remote(yield_context yield);
  
  void do_forward_remote_to_local(yield_context yield);
  
  io_context::strand &strand;
  ip::tcp::socket local;
  ip::tcp::socket remote;
  const ip::tcp::endpoint &addr;
  const std::vector<uint8_t> &key;
  
  std::unique_ptr<Botan::StreamCipher> send_cipher;
  std::unique_ptr<Botan::StreamCipher> recv_cipher;
  
};
}

