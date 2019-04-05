#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <usings.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/spawn.hpp>
namespace msocks
{

class client : public noncopyable
{
public:
  client(
    const ip::tcp::endpoint &listen,
    const ip::tcp::endpoint &remote_,
    const std::vector<uint8_t> & key_);
  void start();
private:
  void do_accept(yield_context yield);
  io_context context;
  io_context::strand strand;
  ip::tcp::acceptor acceptor;
  const ip::tcp::endpoint &remote;
  const std::vector<uint8_t> &key;
};

}

