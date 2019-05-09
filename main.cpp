#include <iostream>
#include "http_server.hpp"
using namespace xfinal;
class Test {
public:
	void before() {

	}

	bool after() {

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
	});
	server.run();
	std::cin.get();
	return 0;
}