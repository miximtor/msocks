#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <usings.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/spawn.hpp>
namespace msocks
{
class server : public noncopyable
{
public:
  server(const ip::tcp::endpoint &listen,const std::vector<uint8_t> & key_);
  void start();
private:
  void do_accept(yield_context yield);
  io_context context;
  io_context::strand strand;
  ip::tcp::acceptor acceptor;
  const std::vector<uint8_t> &key;
};
}


