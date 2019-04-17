#include <spdlog/spdlog.h>

#include <msocks/endpoint/server_endpoint.hpp>
#include <msocks/endpoint/client_endpoint.hpp>
#include <msocks/session/pool.hpp>
#include <boost/asio/io_context.hpp>

#include <botan/sha2_32.h>
#include <botan/md5.h>

void log_setup()
{
	spdlog::set_pattern("[%l] %v");
}

std::vector<uint8_t> evpBytesTokey(const std::string& password)
{
	std::vector<std::vector<uint8_t>> m;
	std::vector<uint8_t> password1(password.begin(), password.end());
	std::vector<uint8_t> data;
	Botan::MD5 md5;
	int i = 0;
	while (m.size() < 32 + 8)
	{
		if (i == 0)
		{
			data = password1;
		}
		else
		{
			data = m[i - 1];
			std::copy(password1.begin(), password1.end(), std::back_inserter(data));
		}
		i++;
		auto hash_result = md5.process(data.data(), data.size());
		m.push_back(std::vector<uint8_t>(hash_result.begin(), hash_result.end()));
	}
	std::vector<uint8_t> key;
	for (auto& mh : m)
	{
		std::copy(mh.begin(), mh.end(), std::back_inserter(key));
	}
	return std::vector<uint8_t>(key.begin(), key.begin() + 32);
}

int main(int argc, char* argv[])
{
	try
	{
		io_context ioc;
		std::string password(argv[4]);
		auto key = evpBytesTokey(password);
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
	catch (boost::exception & e)
	{
		spdlog::error("{}",boost::diagnostic_information(e));
	}
	system("pause");
}

