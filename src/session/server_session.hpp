#pragma once

#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <utility/limiter.hpp>
#include <botan/stream_cipher.h>
#include "session.hpp"
#include <usings.hpp>


namespace msocks
{

class server_session final :
  public session,
  public std::enable_shared_from_this<server_session>
{
public:
  server_session(
    io_context::strand &strand_,
    ip::tcp::socket local_,
    const std::vector<uint8_t> &key_,
    std::shared_ptr<utility::limiter> limiter_
  );
  
  void go();
private:
  
  using Result = async_result<yield_context, void(error_code,std::pair<std::string,std::string>)>;
  using Handler = Result::completion_handler_type;
  
  void start(yield_context yield);
  
  void fwd_local_remote(yield_context yield);
  
  void fwd_remote_local(yield_context yield);
  
  Result::return_type async_handshake(yield_context yield);
  
  void do_async_handshake(Handler handler,yield_context yield);
  
  ip::tcp::resolver resolver;
  std::shared_ptr<utility::limiter> limiter;
};

}

