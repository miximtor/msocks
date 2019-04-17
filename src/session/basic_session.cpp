//
// Created by maxtorm on 2019/4/14.
//
#include <msocks/session/basic_session.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
using namespace boost::uuids;
namespace msocks
{

const std::string &basic_session::uuid() const noexcept
{
	return uuid_;
}

basic_session::basic_session(io_context &ioc, ip::tcp::socket local) :
	ioc_(ioc), 
	local_(std::move(local)), 
	buffer_local_(), 
	remote_(ioc), 
	buffer_remote_(),
	uuid_(to_string(random_generator_mt19937()()))
{
}

void basic_session::cipher_setup(
	std::unique_ptr<Botan::StreamCipher> &cipher,
	const std::string &method,
	const std::vector<uint8_t> &key,
	const std::vector<uint8_t> &iv)
{
	if (cipher == nullptr || cipher->name() != method)
		cipher = Botan::StreamCipher::create_or_throw(method);
	cipher->set_key(key.data(), key.size());
	cipher->set_iv(iv.data(), iv.size());
}

}
