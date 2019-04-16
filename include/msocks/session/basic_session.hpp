//
// Created by maxtorm on 2019/4/14.
//

#pragma once


#include <boost/asio/ip/tcp.hpp>
#include <botan/stream_cipher.h>

using namespace boost::asio;
using namespace boost::system;
namespace msocks
{

class basic_session : public noncopyable
{
public:

	const std::string& uuid() const noexcept;

protected:
	basic_session(io_context& ioc, ip::tcp::socket local);

	static void cipher_setup(
		std::unique_ptr<Botan::StreamCipher>& cipher,
		const std::string& method,
		const std::vector<uint8_t>& key,
		const std::vector<uint8_t>& iv
	);

	io_context& ioc_;
	std::string uuid_;
	ip::tcp::socket local_;
	std::array<uint8_t, MSOCKS_BUFFER_SIZE> buffer_local_;
	ip::tcp::socket remote_;
	std::array<uint8_t, MSOCKS_BUFFER_SIZE> buffer_remote_;

	std::unique_ptr<Botan::StreamCipher> send_cipher_;
	std::unique_ptr<Botan::StreamCipher> recv_cipher_;

};

}
