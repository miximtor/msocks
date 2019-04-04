#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cryptopp/secblock.h>
#include <cryptopp/salsa.h>

#include <usings.hpp>

namespace msocks
{

class server_session :
  public noncopyable,
  public std::enable_shared_from_this<server_session>
{
public:
  server_session (
    ip::tcp::socket local_,
    const CryptoPP::SecByteBlock& key_
    );
  
  void start(yield_context &yield);
  
private:
  
  void do_forward_local_to_remote(yield_context &yield);
  
  void do_forward_remote_to_local(yield_context &yield);
  
  std::pair<std::string,std::string> do_get_target(yield_context &yield);
  
  ip::tcp::socket local;
  ip::tcp::socket remote;
  SecByteBlock key;
  SecByteBlock iv;
  Salsa20::Encryption encrypt;
  Salsa20::Decryption decrypt;
  
  
};

}

