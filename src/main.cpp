#include <vector>
#include <string>
#include <endpoint/server.hpp>
#include <endpoint/client.hpp>
#include <config/config.hpp>
#include <botan/sha2_32.h>
#include <spdlog/spdlog.h>

msocks::config parse_config(int argc,char *argv[])
{
  msocks::config config;
  return config;
}

void log_setup()
{
  spdlog::set_pattern("[%l] %v");
}

int main(int argc, char *argv[])
{
  log_setup();
  try
  {
    auto config = parse_config(argc, argv);
    using namespace std::string_literals;
    std::string ip = argv[2];
    unsigned short port = std::stoi(argv[3]);
    ip::tcp::endpoint listen(ip::make_address_v4(ip), port);
    std::string password = argv[4];
    Botan::SHA_256 sha256;
    sha256.update(password);
    std::vector<uint8_t> key;
    sha256.final(key);
    if ( argv[1] == "s"s )
    {
      std::size_t limit = std::stoull(argv[5]);
      msocks::server server(listen, key,limit);
      server.start();
    }
    else if ( argv[1] == "c"s )
    {
      ip::tcp::endpoint local_ep(ip::make_address("127.0.0.1"), 1081);
      msocks::client client(local_ep, listen, key);
      client.start();
    }
  }
  catch (boost::exception &e)
  {
    spdlog::error("main error: {}",boost::diagnostic_information(e,false));
    exit(-1);
  }
  catch (...)
  {
    spdlog::error("main error: {}","unknown exception");
    exit(-2);
  }
  return 0;
}