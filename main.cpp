#include <iostream>
#include "http_server.hpp"
#include "client.hpp"
using namespace xfinal;
class Test {
public:
	bool before(request& req,response& res) {
		std::cout << "pre process aop 1" << std::endl;
		return true;
	}

	bool after(request& req, response& res) {
		return true;
	}
private:
	int a;
};

class Test2 {
public:
	bool before(request& req, response& res) {
		std::cout << "pre process aop 2" << std::endl;
		return true;
	}

	bool after(request& req, response& res) {
		std::cout << "after process aop 2" << std::endl;
		return true;
	}
};

class Base {
private:
	bool b;
};

class Process {
public:
	void test(request& req,response& res) {
		res.write_string("OK test!");
	}
};

class Shop :public Controller {
public:
	void goshop() {
		get_response().write_string(std::to_string(count++)+" go shop");
	}
private:
	int count = 0;
};


int main()
{
	http_server server(4);
	server.listen("0.0.0.0", "8080");

	server.on_error([](std::exception const& ec) {  //提供用户记录错误日志
		std::cout << ec.what() << std::endl;
	});

	server.add_view_method("str2int", 1, [](inja::Arguments const& args) {
		auto i = std::atoi(args[0]->get<std::string>().data());
		return std::string("transform:") + std::to_string(i);
	});

	server.router<GET,POST>("/abc", [](request& req,response& res) {
		std::cout << req.query("id") << std::endl;
		std::cout << "hahaha "<<req.url() <<" text :"<<req.query<GBK>("text")<< std::endl;
	}, Test{}, Base{}, Test2{});

	server.router<GET, POST>("/", [](request& req, response& res) {
		res.write_string(std::string("hello world"),false);
	});

	server.router<POST>("/json", [](request& req, response& res) {
		std::cout << "body: " << req.body()<<std::endl;
	});

	server.router<GET>("/params", [](request& req, response& res) {
		std::cout << "id: " << req.param("id") << std::endl;
		res.write_string("id");
	});

	server.router<POST>("/upload", [](request& req, response& res) {
		std::cout << "text: " << req.query("text") << std::endl;
		auto& file = req.file("img");
		std::cout << file.size() << std::endl;
		std::cout << req.query<GBK>("text2") << std::endl;
		json data;
		data["name"] = file.original_name();
		data["text"] = view2str(req.query("text"));
		data["success"] = true;
		res.write_json(data);
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

	server.router<GET, POST>("/pathinfo/*", [](request& req, response& res) {
		res.write_string("abc");
	});

	server.router<GET, POST>("/oct", [](request& req, response& res) {
		res.write_string("abc");
	});

	Process c;
	server.router<GET, POST>("/test",&Process::test,c);

	Shop ss;
	server.router<GET, POST>("/controller", &Shop::goshop,ss);

	server.router<GET>("/client", [](request& req, response& res) {
		http_client client("www.baidu.com");
		auto con = client.request<GET>("/");
		auto header = con.get_header("Content-Length");
		auto state = con.get_status_code();
		res.write_string(con.get_content());
	});

	server.router<GET>("/asyclient", [](request& req, response& res) {
		http_client client("www.baidu.com");
		client.request<GET>("/", [](http_client::client_response const& res,std::error_code const& ec) {
			if (ec) {
				return;
			 }
			std::cout << res.get_content() << std::endl;
		});
		client.run();
		res.write_string("OK");
	});

	server.router<GET>("/client_post", [](request& req, response& res) {
		try {
			http_client client("127.0.0.1:8020");
			client.add_header("name", "hello");
			client.add_header("Content-Type", "application/x-www-form-urlencoded");
			std::string form = "id=1234";
			auto con = client.request<POST>("/test", form);
			res.write_string(con.get_content());
		}
		catch (...) {
			res.write_string("error");
		}
	});

	server.router<GET>("/client_multipart", [](request& req, response& res) {
		try {
			http_client client("127.0.0.1:8020");
			multipart_form form;
			form.append("img", multipart_file{ "./static/upload/v2.jpg" });
			form.append("text", "hello,world");
			auto con = client.request<POST>("/upload", form);
			res.write_string(con.get_content());
		}
		catch (...) {
			res.write_string("error");
		}
	});

	server.router<GET>("/view", [](request& req, response& res) {
		res.set_attr("name", "xfinal");
		res.set_attr("language", "c++");
		res.set_attr("str", "1024");
		res.write_view("./static/test.html");
	});

	server.router<GET>("/session", [](request& req, response& res) {
		auto& session = req.create_session();
		session.set_data("time", std::to_string(std::time(nullptr)));
		session.set_expires(15);
		res.write_string("OK");
	});

	server.router<GET>("/login", [](request& req, response& res) {
		auto& session = req.session();
		auto t = session.get_data<std::string>("time");
		if (t.empty()) {
			res.write_string("no");
		}
		else {
			res.write_string("yes");
		}
	});

	websocket_event event;
	event.on("message", [](websocket& ws) {
		std::cout << ws.messages() << std::endl;
		if (ws.messages() == nonstd::string_view{ "close" }) {
			//ws.close();
			return;
		}
		std::string message;
		for (auto i = 0; i <= 400; ++i) {
			message.append(std::to_string(i));
		}
		ws.write_string(message);
		//ws.write(std::move(message), 1);
	}).on("open", [](websocket& ws) {
		std::cout << "open" << std::endl;
	});
	server.router("/ws", event);



	server.run();
	return 0;
}