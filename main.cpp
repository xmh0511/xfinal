#include <iostream>
#include <xfinal.hpp>
using namespace xfinal;
class Test {
public:
	bool before(request& req, response& res) {
		std::cout << "pre process aop 1" << std::endl;
		//res.write_string("aop1 stop");
		return true;
	}

	bool after(request& req, response& res) {
		std::cout << "after process aop 1" << std::endl;
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

struct  init_session {
	bool before(request& req, response& res) {
		auto& session = req.session("test_session");
		if (session.empty()) {
			req.create_session("test_session");
		}
		return true;
	}
	bool after(request& req, response& res) {
		return true;
	}
};

class Base {
private:
	bool b;
};

class Process {
public:
	void test(request& req, response& res) {
		res.write_string("OK test!");
	}
};

class Shop :public Controller {
public:
	void goshop() {
		get_response().write_string(std::to_string(count++) + " go shop");
	}
private:
	int count = 0;
};

struct limitUpload {
	bool prehandle(request& req, response& res) {
		auto fileSize = atoi(req.header("content-length").data());
		if (fileSize > 1024*20) {
			res.write_string("too large", false, http_status::bad_request);
			return false;
		}
		return true;
	}
	bool posthandle(request& req, response& res) {
		return true;
	}
};


int main()
{
	auto const thread_number = std::thread::hardware_concurrency();
	http_server server(thread_number);
	server.on_error([](std::string const& message) {  //提供用户记录错误日志
		std::cout << message << std::endl;
		});
	bool r = server.listen("0.0.0.0", "8080");
	if (!r) {
		return 0;
	}
	server.set_url_redirect(false);
	//server.set_upload_path("./myupload");
	//server.set_not_found_callback([](request& req,response& res) {
	//	res.write_string("custom not found", true, http_status::bad_request);
	//});

	//server.set_websocket_frame_size(1024 * 1024);//设置websocket 帧数据长度

	server.add_view_method("str2int", 1, [](inja::Arguments const& args) {
		auto i = std::atoi(args[0]->get<std::string>().data());
		return std::string("transform:") + std::to_string(i);
		});

	server.router<GET>("/sessionrecord", [](request& req, response& res) {
		auto& session = req.session("test_session0");
		session.set_expires(3600);
		res.write_string("OK");
		}, init_session{});

	server.router<GET, POST>("/abc", [](request& req, response& res) {
		std::cout << req.query("id") << std::endl;
		std::cout << "hahaha " << req.url() << " text :" << req.query<GBK>("text") << std::endl;
		res.write_string("OK");
		}, Test{}, Base{}, Test2{});

	server.router<GET, POST>("/", [](request& req, response& res) {
		res.write_string(std::string("hello world"), false);
		});

	server.router<GET, POST>("/ip", [](request& req, response& res) {
		auto ip = res.connection().remote_ip();
		auto locip = res.connection().local_ip();
		res.write_string(std::move(ip + " " + locip));
		});

	server.router<GET, POST>("/json", [](request& req, response& res) {
		std::cout << "body: " << req.body() << std::endl;
		json root;
		root["hello"] = u8"你好，中文";
		res.write_json(root);
		});

	server.router<GET, POST>("/postjson", [](request& req, response& res) {
		try {
			auto root = json::parse(view2str(req.body()));
			root["url"] = "postjson";
			res.write_json(root);
		}
		catch (std::exception const& ec) {
			res.write_string("error", false, http_status::bad_request);
		}
		});

	server.router<GET>("/params", [](request& req, response& res) {
		auto params = req.key_params();
		std::cout << "id: " << req.param("id") << std::endl;
		std::cout << "all value: " << req.raw_key_params() << std::endl;
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
		data["url"] = file.url_path();
		data["file_path"] = file.path();
		data["success"] = true;
		res.write_json(data);
		});

	server.router<GET, POST>("/chunkedstr", [](request& req, response& res) {
		std::ifstream file("./data.txt");
		std::stringstream ss;
		ss << file.rdbuf();
		res.write_string(ss.str(), true);
		});

	server.router<GET, POST>("/chunkedfile", [](request& req, response& res) {

		res.write_file("./data.txt", true);
		});

	server.router<GET, POST>("/video", [](request& req, response& res) {
		res.write_file("./test.mp4", true);
		});

	server.router<GET, POST>("/pathinfo/*", [](request& req, response& res) {
		auto raw_param = req.raw_url_params();
		auto param = req.param(0);
		auto params = req.url_params();
		res.write_string("abc");
	});

	server.router<GET, POST>("/oct", [](request& req, response& res) {
		res.write_string("abc");
		});

	Process c;
	server.router<GET, POST>("/test", &Process::test, c, Test{});

	server.router<GET, POST>("/test0", &Process::test, nullptr, Test{});

	server.router<GET, POST>("/test1", &Process::test);

	Shop ss;
	server.router<GET, POST>("/controller", &Shop::goshop, ss, Test{});

	server.router<GET, POST>("/controller0", &Shop::goshop, nullptr, Test{});

	server.router<GET, POST>("/controller1", &Shop::goshop, Test{});

	server.router<GET>("/client", [](request& req, response& res) {
		http_client client("www.baidu.com");
		auto con = client.request<GET>("/");
		auto header = con.get_header("Content-Length");
		auto state = con.get_status_code();
		res.write_string(con.get_content());
		});

	server.router<GET>("/asyclient", [](request& req, response& res) {
		http_client client("www.baidu.com");
		client.request<GET>("/", [](http_client::client_response const& res, std::error_code const& ec) {
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
		session.set_expires(30);
		res.write_string("OK");
		});

	server.router<GET>("/changesession", [](request& req, response& res) {
		auto& session = req.session();
		auto t = session.get_data<std::string>("time");
		if (!t.empty()) {
			session.set_id("0123456");
		}
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

	server.router<POST, GET>("/uploadlimt", [](request& req, response& res) {
		std::cout << "command" << std::endl;
		std::cout << req.query("text") << std::endl;
		auto& file = req.file("img");
		json root;
		root["size"] = file.size();
		root["url"] = file.url_path();
		res.write_json(root);
	}, limitUpload{});

	websocket_event event;
	event.on("message", [](websocket& ws) {
		std::cout << view2str(ws.messages()) << std::endl;
		std::cout << (int)ws.message_code() << std::endl;
		std::string message;
		for (auto i = 0; i <= 400; ++i) {
			message.append(std::to_string(i));
		}
		ws.write_string(message);
		}).on("open", [](websocket& ws) {
			std::cout << "open" << std::endl;
			}).on("close", [](websocket& ws) {
				std::cout << "close" << std::endl;
				});
			server.router("/ws", event);



			server.run();
			return 0;
}