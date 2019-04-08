#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <botan/stream_cipher.h>
#include "session.hpp"
#include <usings.hpp>


namespace msocks
{
class client_session final:
  public session,
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
  
  using Result = async_result<yield_context, void(error_code)>;
  using Handler = Result::completion_handler_type;
  
  void start(yield_context yield);

  void async_handshake(const std::string &host_service, yield_context yield);
  
  void do_async_handshake(Handler handler,const std::string & host_service,yield_context yield);
  
  void fwd_local_remote(yield_context yield);
  void fwd_remote_local(yield_context yield);
  const ip::tcp::endpoint &addr;
  
};
}

