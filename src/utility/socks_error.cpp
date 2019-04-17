#include  <msocks/utility/socks_erorr.hpp>
namespace msocks
{
boost::system::error_category& socks_category()
{
	static class detail::socks_category category;
	return category;
}
}