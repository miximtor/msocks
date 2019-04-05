#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <usings.hpp>
#include <botan/stream_cipher.h>

namespace msocks
{

class server_session :
  public noncopyable,
  public std::enable_shared_from_this<server_session>
{
public:
  server_session(
    io_context::strand &strand_,
    ip::tcp::socket local_,
    const std::vector<uint8_t> &key_
  );
  
  void go();
private:
  void start(yield_context yield);
  
  void do_forward_local_to_remote(yield_context yield);
  
  void do_forward_remote_to_local(yield_context yield);
  
  std::pair<std::string, std::string> handshake(yield_context yield);
  io_context::strand &strand;
  ip::tcp::socket local;
  ip::tcp::socket remote;
  const std::vector<uint8_t> &key;
  ip::tcp::resolver resolver;
  
  std::unique_ptr<Botan::StreamCipher> send_cipher;
  std::unique_ptr<Botan::StreamCipher> recv_cipher;
  
};

}

