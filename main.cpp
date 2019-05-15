#include <iostream>
#include "http_server.hpp"
using namespace xfinal;
class Test {
public:
	bool before(request& req,response& res) {
		std::cout << "pre process aop" << std::endl;
		return true;
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
	http_server server(4);
	server.listen("0.0.0.0", "8080");
	server.router<GET,POST>("/abc", [](request& req,response& res) {
		std::cout << req.query("id") << std::endl;
		std::cout << "hahaha "<<req.url() <<" text :"<<req.query<GBK>("text")<< std::endl;
	}, Test{});

	server.router<GET, POST>("/", [](request& req, response& res) {
		res.write_string(std::string("hello world"),false);
	});

	server.router<POST>("/json", [](request& req, response& res) {
		std::cout << "body: " << req.body()<<std::endl;
	});

	server.router<GET>("/params", [](request& req, response& res) {
		std::cout << "id: " << req.param("id") << std::endl;
		std::cout << "text: " << req.param<GBK>("text") << std::endl;
	});

	server.router<POST>("/upload", [](request& req, response& res) {
		std::cout << "text: " << req.query("text") << std::endl;
		std::cout << req.file("img").size() << std::endl;
		std::cout << req.query<GBK>("text2") << std::endl;
	});

	server.router<GET, POST>("/chunkedstr", [](request& req, response& res) {
		std::ifstream file("./data.txt");
		std::stringstream ss;
		ss << file.rdbuf();
		res.write_string(ss.str(),true);
	});

	server.router<GET, POST>("/chunkedfile", [](request& req, response& res) {

		res.write_file("./data.txt", true);
	});

	server.router<GET, POST>("/video", [](request& req, response& res) {
		res.write_file("./test.mp4", true);
	});

	server.router<GET, POST>("/abc/*", [](request& req, response& res) {
		res.write_string("abc");
	});

	server.run();

	std::cin.get();
	return 0;
}