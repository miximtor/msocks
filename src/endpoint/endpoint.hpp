#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <usings.hpp>

namespace msocks
{

struct endpoint : public noncopyable
{
  endpoint(
    const ip::tcp::endpoint &listen
  ) :
    strand(context),
    acceptor(context, listen)
  {}

protected:
  template <typename SessionFactory>
  void async_accept(yield_context yield,SessionFactory factory)
  {
    while (true)
    {
      ip::tcp::socket socket(context);
      acceptor.async_accept(socket,yield);
      auto session = factory(std::move(socket));
      session->go();
    }
  }
  
  io_context context;
  io_context::strand strand;
  ip::tcp::acceptor acceptor;
  
  
};
}