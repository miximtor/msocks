#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cryptopp/secblock.h>
#include <cryptopp/salsa.h>

#include <usings.hpp>


namespace msocks
{
class client_session :
  public noncopyable,
  public std::enable_shared_from_this<client_session>
{
public:
  
  client_session(
    ip::tcp::socket local_,
    const ip::tcp::endpoint &addr_,
    const CryptoPP::SecByteBlock& key_
  );
  
  void start(yield_context &yield);

private:
  
  std::pair<bool, std::string> do_local_socks5(yield_context &yield);
  
  bool do_socks5_auth(yield_context &yield);
  
  std::pair<bool,std::string> do_get_proxy_addr(yield_context &yield);
  
  void send_handshake(yield_context &yield,std::string addr_port);
  
  void do_forward_local_to_remote(yield_context &yield);
  
  void do_forward_remote_to_local(yield_context &yield);
  
  ip::tcp::socket local;
  ip::tcp::socket remote;
  const ip::tcp::endpoint &addr;
  SecByteBlock key;
  SecByteBlock iv;
  Salsa20::Encryption encrypt;
  Salsa20::Decryption decrypt;
};
}

