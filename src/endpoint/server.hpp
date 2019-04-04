#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <cryptopp/secblock.h>
#include <usings.hpp>

namespace msocks
{
class server :
  public noncopyable
{
public:
  server(const ip::tcp::endpoint &listen,const SecByteBlock & key_);
  void start();
  
private:
  io_context context;
  ip::tcp::acceptor acceptor;
  const SecByteBlock &key;
};

}


