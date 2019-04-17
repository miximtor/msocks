//
// Created by maxtorm on 2019/4/14.
//

#include <boost/asio/post.hpp>

#include <msocks/utility/rate_limiter.hpp>

namespace msocks::utility
{

async_result<yield_context, void()>::return_type rate_limiter::async_get(std::size_t n, yield_context yield)
{
	async_result<yield_context, void()>::completion_handler_type handler(yield);
	async_result<yield_context, void()> result(handler);
	if (no_limit_)
	{
		post(ioc_, std::bind(handler));
		return result.get();
	}

	if (available_ >= n)
	{
		available_ -= n;
		post(ioc_, std::bind(handler));
		return result.get();
	}

	if (wait_queue_.empty())
	{
		signal_.expires_from_now(boost::posix_time::pos_infin);
	}

	unique_pair pair(new storage_pair(n, handler));
	wait_queue_.emplace(std::move(pair));
	return result.get();
}


void rate_limiter::start()
{
	if (no_limit_)
	{
		return;
	}

	spawn(
		ioc_,
		[this, p = shared_from_this()](yield_context yield)
	{
		while (true)
		{
			if (wait_queue_.empty())
			{
				error_code ignored_ec;
				signal_.async_wait(yield[ignored_ec]);
			}
			for (auto i = 0; i < 10; i++)
			{
				timer_.expires_from_now(boost::posix_time::milliseconds(100));
				timer_.async_wait(yield);
				available_ += limit_ / 10;
				while (!wait_queue_.empty() && wait_queue_.top()->first <= available_)
				{
					auto handler = wait_queue_.top()->second;
					available_ -= wait_queue_.top()->first;
					wait_queue_.pop();
					post(ioc_, std::bind(handler));
				}
			}
		}
	});
}

}