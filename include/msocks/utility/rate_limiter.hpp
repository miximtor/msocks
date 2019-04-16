#pragma once

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/spawn.hpp>
#include <queue>
#include <memory>
using namespace boost::asio;
using namespace boost::system;

namespace msocks::utility
{

class rate_limiter : public noncopyable, public std::enable_shared_from_this<rate_limiter>
{
private:
	using storage_pair = std::pair<std::size_t, async_result<yield_context, void()>::completion_handler_type>;
	using unique_pair = std::unique_ptr<storage_pair>;
public:
	rate_limiter(io_context& ioc, const std::size_t limit) :
		ioc_(ioc), timer_(ioc), signal_(ioc), limit_(limit), no_limit_(limit == 0),
		wait_queue_([](const unique_pair& l, const unique_pair& r) { return l->first >= r->first; })
	{}

	async_result<yield_context, void()>::return_type async_get(std::size_t n, yield_context yield);

	void start();

private:
	io_context& ioc_;
	deadline_timer timer_;
	deadline_timer signal_;
	const std::size_t limit_;
	bool no_limit_;
	std::size_t available_ = 0;
	std::priority_queue<
		unique_pair,
		std::vector<unique_pair>,
		std::function<bool(const unique_pair& l, const unique_pair& r)>
	> wait_queue_;
};
}
