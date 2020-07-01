#include <iostream>
#include <xfinal.hpp>
using namespace xfinal;
class Test {
public:
	bool before(request& req, response& res) {
		std::cout << "pre process aop 1" << std::endl;
		std::shared_ptr<std::string> data{ new std::string("a") };
		req.set_user_data("tag", data);
		//res.write_string("aop1 stop");
		return true;
	}

	bool after(request& req, response& res) {
		std::cout << "after process aop 1" << std::endl;
		auto data = req.get_user_data<std::shared_ptr<std::string>>("tag");
		std::cout << *data << std::endl;
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
		//auto fileSize = atoi(req.header("content-length").data());
		//if (fileSize > 1024*20) {
		//	res.write_string("too large", true, http_status::bad_request);
		//	return false;
		//}
		auto type = req.content_type();
		if (type == content_type::multipart_form) {
			res.write_file_view("./www/test.html", true, http_status::bad_request);
			return false;
		}
		return true;
	}
	bool posthandle(request& req, response& res) {
		return true;
	}
};

struct limitRequest {
	bool prehandle(request& req, response& res) {
		using namespace nonstd::literals;
		auto id = req.param("id");
		if (id == "0"_sv) {
			res.write_file_view("./www/error.html", true, http_status::bad_request);
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
	bool r = server.listen("0.0.0.0", "8080");
	if (!r) {
		return 0;
	}
	server.init_nanolog("./logs/", "server_log", 1);
	server.on_error([](std::string const& message) {  //提供用户记录错误日志
#ifdef _WIN32
		std::cout << utf8_to_gbk(message) << std::endl;
#else 
		std::cout << message << std::endl;
#endif 
	});
	server.set_wait_read_time(30);
	server.set_wait_write_time(30);
	server.set_websocket_check_read_alive_time(200);
	server.set_websocket_check_write_alive_time(200);
	server.set_defer_write_max_wait_time(10);
	//server.set_max_body_size(3*1024*1024);

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
		std::cout <<"id: "<< req.query("id") << std::endl;
		std::cout <<"text: "<< req.query("text") << std::endl;
		auto data = req.get_user_data<std::shared_ptr<std::string>>("tag");
		std::cout << *data << "\n";
		*data = "b";
		res.write_string("OK");
		}, Test{}, Base{}, Test2{});

	server.router<GET, POST>("/", [](request& req, response& res) {
		XFile file;
		file.open("./data.dll",true);
		file.add_data("abc");
		file.add_data("1");
		res.write_string(std::string("hello world"), false);
	});


	server.router<GET, POST>("/readfile", [](request& req, response& res) {
		XFile file;
		file.open("./data.dll");
		char arr[2] = {0};
		auto s1 = file.read(-1, arr, 2);
		char arr2[2] = {0};
		auto s2 = file.read(1, arr2, 2);
		char arr3[2] = { 0 };
		auto s3 = file.read(3, arr3, 2);
		res.write_string(std::string("hello world"), false);
	});

	server.router<GET, POST>("/ip", [](request& req, response& res) {
		auto ip = res.connection().remote_ip();
		auto locip = res.connection().local_ip();
		res.write_string(std::move(ip + " " + locip));
		});

	server.router<GET, POST>("/json", [](request& req, response& res) {
		std::cout << "body: " << view2str(req.body()) << std::endl;
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
			(void)ec;
			res.write_string("error", false, http_status::bad_request);
		}
		});

	server.router<GET>("/params", [](request& req, response& res) {
		auto params = req.key_params();
		std::cout << "id: " << req.param("id") << std::endl;
		std::cout << "text: " << req.param("text") << std::endl;
		std::cout << "all value: " << req.raw_key_params() << std::endl;
		res.write_string("id");
	});

	server.router<POST>("/upload", [](request& req, response& res) {
		json data;
		auto& files = req.files();
		for (auto& file : files) {
			json file_info;
			file_info["name"] = file.second.original_name();
			file_info["size"] = file.second.size();
			file_info["file_path"] = file.second.path();
			file_info["url"] = file.second.url_path();
			file_info["key"] = file.first;
			data["files"].push_back(file_info);
		}
		auto& texts = req.multipart_form();
		for (auto& iter : texts) {
			json text_info;
			text_info["text"] = iter.second;
			text_info["key"] = iter.first;
			data["texts"].push_back(text_info);
		}
		auto& file = req.file("img");
		auto text = req.query("text");
		data["success"] = true;
		res.write_json(data);
		});

	server.router<GET, POST>("/chunkedstr", [](request& req, response& res) {
		std::ifstream file("./data.txt");
		std::stringstream ss;
		ss << file.rdbuf();
		res.write_string(ss.str(), true);
		});

	server.router<GET, POST,HEAD>("/chunkedfile", [](request& req, response& res) {
		std::cout << req.method() << "\n";
		res.write_file("./data.zip", true);
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
		auto& file = req.file();
		json root;
		root["size"] = file.size();
		root["url"] = file.url_path();
		res.write_string(root.dump());
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

	server.router<GET>("/ttt", [](request& req, response& res) {
		auto guarder = res.defer();
		std::shared_ptr<http_client> client(new http_client("www.baidu.com"));
		client->request<GET>("/", [client, guarder,&res](http_client::client_response const& cres, std::error_code const& ec) {
			if (ec) {
				return;
			}
			auto content = cres.get_content();
			res.write_string(std::move(content));
		});
		std::thread t1 = std::thread([client]() {
			client->run();
		});
		t1.detach();
		std::cout << "end" << std::endl;
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
		res.write_file_view("./static/test.html");
	});

	server.router<GET>("/dataview", [](request& req, response& res) {
		res.set_attr("name", "xfinal");
		res.set_attr("language", "c++");
		res.set_attr("str", "1024");
		res.write_data_view("<div>@{name}</div><div>@{language}</div><div>@{str}</div>",true);
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

	server.router<GET, POST>("/performance", [](request& req, response& res) {
		res.write_string("ok");
	}, limitRequest{});


	server.router<GET>("/deferresponse", [](request& req, response& res) {
		auto time = req.param("time");
		auto time_i = std::atoi(time.data());
		auto guard = res.defer();
		std::thread t1 = std::thread([guard,&res, time_i]() {
			std::this_thread::sleep_for(std::chrono::seconds(time_i));
			std::cout << "command\n";
			res.write_string("OK");
		});
		t1.detach();
		}, Test{});


	server.router<GET, POST>("/httppackage", [](request& req, response& res) {
		struct CustomeResult :public response::http_package {
			CustomeResult(std::string&& content) {
				body_souce_ = std::move(content);
				state_ = http_status::ok;
				write_type_ = response::write_type::string;
			}
		};
		res.write_http_package(CustomeResult{ "OK" });
	});

	server.router<GET, POST>("/httppackagefile", [](request& req, response& res) {
		struct CustomeResult :public response::http_package {
			CustomeResult(std::string&& content) {
				body_souce_ = std::move(content);
				state_ = http_status::ok;
				write_type_ = response::write_type::file;
				is_chunked_ = true;
			}
		};
		res.write_http_package(CustomeResult{ "./static/a.mp4" });
	});

	std::shared_ptr<websocket> other_socket;

	websocket_event event;
	event.on("message", [](websocket& ws) {
		std::cout << view2str(ws.messages()) << std::endl;
		std::cout << (int)ws.message_code() << std::endl;
		std::string message;
		for (auto i = 0; i <= 18000; ++i) {
			message.append(std::to_string(i)+",");
		}
		ws.write_string(message);
		}).on("open", [&other_socket](websocket& ws) {
			other_socket = ws.shared_from_this();
			std::cout << ws.uuid() << " open" << std::endl;
		}).on("close", [](websocket& ws) {
				std::cout << ws.uuid()<< " close" << std::endl;
		});
		server.router("/ws", event, limitRequest {});


		websocket_event event2;
		event2.on("message", [](websocket& ws) {

		}).on("open", [&other_socket](websocket& ws) {
			std::cout << ws.uuid() << " open" << std::endl;
			if (other_socket) {
				auto self = ws.shared_from_this();
				std::thread t1([other_socket, self]() {
					while (true) {
						if (!self->is_open()) {
							return;
						}
						other_socket->write_string("1 "+std::to_string(std::time(nullptr)));
						std::this_thread::sleep_for(std::chrono::seconds(1));
					}
				});
				t1.detach();
			}
		}).on("close", [](websocket& ws) {
					std::cout << ws.uuid() << " close" << std::endl;
		 });
		server.router("/other", event2);


		websocket_event event3;
		event3.on("message", [](websocket& ws) {

			}).on("open", [&other_socket](websocket& ws) {
				std::cout << ws.uuid() << " open" << std::endl;
				if (other_socket) {
					auto self = ws.shared_from_this();
					std::thread t1([other_socket, self]() {
						while (true) {
							if (!self->is_open()) {
								return;
							}
							other_socket->write_string("2 " + std::to_string(std::time(nullptr)));
							std::this_thread::sleep_for(std::chrono::seconds(1));
						}
						});
					t1.detach();
				}
				}).on("close", [](websocket& ws) {
					std::cout << ws.uuid() << " close" << std::endl;
					});
				server.router("/other2", event3);

		LOG_INFO << std::string("server start");

		//std::thread endth([&server]() {
		//	std::this_thread::sleep_for(std::chrono::seconds(20));
		//	server.stop();
		//});
		//endth.detach();


		server.run();
		std::cout << "server end" << std::endl;
		return 0;
}