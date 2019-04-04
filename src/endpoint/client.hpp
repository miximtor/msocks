#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <cryptopp/secblock.h>
#include <usings.hpp>

namespace msocks
{

class client
{
public:
  client(const ip::tcp::endpoint &listen,const ip::tcp::endpoint &remote_,const SecByteBlock & key_);
  void start();
private:
  io_context context;
  ip::tcp::acceptor acceptor;
  const ip::tcp::endpoint &remote;
  const SecByteBlock &key;
};

}

