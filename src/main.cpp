#include <spdlog/spdlog.h>

#include <msocks/endpoint/server_endpoint.hpp>
#include <msocks/endpoint/client_endpoint.hpp>
#include <msocks/session/pool.hpp>
#include <boost/asio/io_context.hpp>

#include <botan/sha2_32.h>

void log_setup()
{
	spdlog::set_pattern("[%l] %v");
}

int main(int argc, char* argv[])
{
	io_context ioc;
	Botan::SHA_256 sha256;
	sha256.update(std::string(argv[4]));
	std::vector<uint8_t> key;
	sha256.final(key);
	if (!strcmp(argv[1], "s"))
	{
		msocks::pool<msocks::server_session> pool(ioc);
		msocks::server_endpoint_config config;
		config.no_delay = true;
		config.server_address = argv[2];
		config.server_port = std::stoi(argv[3]);
		config.key = key;
		config.speed_limit = std::stoi(argv[5]);
		config.method = "ChaCha(20)";
		config.timeout = boost::posix_time::seconds(2);
		msocks::server_endpoint server(ioc, pool, std::move(config));
		server.start();
		ioc.run();
	}
	else
	{
		msocks::client_config config;
		config.local_address = "127.0.0.1";
		config.local_port = 1081;
		config.key = key;
		config.remote_address = argv[2];
		config.remote_port = std::stoi(argv[3]);
		config.method = "ChaCha(20)";
		config.timeout = boost::posix_time::seconds(2);
		msocks::client_endpoint client(ioc, std::move(config));
		client.start();
		ioc.run();
	}
}

