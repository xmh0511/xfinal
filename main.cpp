#include <iostream>
#include "http_server.hpp"
using namespace xfinal;
class Test {
public:
	bool before(request& req,response& res) {
		std::cout << "pre process aop" << std::endl;
		return false;
	}

	bool after(request& req, response& res) {
		return false;
	}
private:
	int a;
};

class Base {
private:
	bool b;
};
int main()
{
	http_server server(2);
	server.listen("0.0.0.0", "8080");
	server.router<GET>("/abc", [](request& req,response& res) {
		std::cout << "hahaha "<<req.get_url() << std::endl;
	}, Test{});
	server.run();
	std::cin.get();
	return 0;
}