#pragma once

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>
#include <queue>
#include <usings.hpp>

namespace utility
{

class limiter :
  public noncopyable,
  public std::enable_shared_from_this<limiter>
{
private:
  using Result = async_result<yield_context, void(void)>;
  using Handler = typename Result::completion_handler_type;
public:
  limiter(io_context::strand &strand_, std::size_t limit_per_sec) :
    strand(strand_),
    timer(strand.context()),
    signal(strand.context()),
    increment(limit_per_sec),
    available(0)
  {
  }
  
  auto async_get(std::size_t n, yield_context yield)
  {
    if (available >= n)
    {
      available-=n;
      return;
    }
    if ( queue.empty())
    {
      signal.cancel();
    }
    Handler handler(yield);
    Result result(handler);
    queue.push(std::make_pair(n, handler));
    return result.get();
  }
  
  void start()
  {
    spawn(strand, [this, p = shared_from_this()](yield_context yield)
    {
      while ( true )
      {
        if ( queue.empty())
        {
          signal.expires_from_now(boost::posix_time::pos_infin);
          error_code ignored_ec;
          signal.async_wait(yield[ignored_ec]);
        }
        timer.expires_from_now(boost::posix_time::seconds(1));
        timer.async_wait(yield);
        available += increment;
        while ( !queue.empty() && queue.front().first <= available )
        {
          auto handler = queue.front().second;
          available -= queue.front().first;
          queue.pop();
          post(strand, handler);
        }
      }
    });
  }

private:
  io_context::strand &strand;
  deadline_timer timer;
  deadline_timer signal;
  std::size_t increment;
  std::size_t available;
  std::queue<std::pair<std::size_t, Handler>> queue;
  
};

}