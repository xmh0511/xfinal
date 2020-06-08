#pragma once
#include <asio.hpp>
#include <string>
#include "ioservice_pool.hpp"
#include "connection.hpp"
#include "utils.hpp"
#include "http_router.hpp"
#include "session.hpp"
#include "message_handler.hpp"
namespace xfinal {
	constexpr http_method GET = http_method::GET;
	constexpr http_method POST = http_method::POST;
	constexpr http_method HEAD = http_method::HEAD;
	constexpr http_method PUT = http_method::PUT;
	constexpr http_method OPTIONS = http_method::OPTIONS;
	constexpr http_method TRACE_ = http_method::TRACE_;
	constexpr http_method DEL = http_method::DEL;
	constexpr http_method MKCOL = http_method::MKCOL;
	constexpr http_method MOVE = http_method::MOVE;

	class http_server :private nocopyable {
	public:
		http_server(std::size_t thread_size):ioservice_pool_handler_(thread_size), acceptor_(ioservice_pool_handler_.accpetor_io()){
			static_url_ = std::string("/")+ get_root_director(static_path_)+ "/*";
			upload_path_ = static_path_ + "/upload";
		}
	public:
		bool listen(std::string const& ip,std::string const& port) {
			auto ec = port_occupied(atoi(port.c_str()));
			if (ec == asio::error::address_in_use) {
				utils::messageCenter::get().trigger_message(ec.message());
				return false;
			}
			server_query_ = std::unique_ptr<asio::ip::tcp::resolver::query>(new asio::ip::tcp::resolver::query{ ip, port });
			return true;
		}
	public:
		void run() {
			if (!disable_auto_register_static_handler_) {
				register_static_router(); //注册静态文件处理逻辑
			}
			auto session_storager = std::unique_ptr<default_session_storage>(new default_session_storage());
			session_storager->save_dir_ = default_storage_session_path_;
			set_session_storager(std::move(session_storager));
			if (!disable_auto_create_directories_) {
				if (!fs::exists(static_path_)) {
					fs::create_directories(static_path_);
				}
				if (!fs::exists(upload_path_)) {
					fs::create_directories(upload_path_);
				}
			}
			if (server_query_ != nullptr) {
				bool r = listen(*server_query_);
				if (!r) {
					return;
				}
			}
			else {
				utils::messageCenter::get().trigger_message("listen fail");
				return;
			}
			ioservice_pool_handler_.run();
		}
		void stop() {
			ioservice_pool_handler_.stop();
		}
	public:
		///只能设置为相对当前程序运行目录的路径  "./xxx/xxx"
		void set_static_path(std::string const& path) {
			static_path_ = path;
			upload_path_ = static_path_ + "/upload";
			static_url_ = std::string("/") + get_root_director(static_path_) + "/*";
		}
		std::string static_path() {
			return static_path_;
		}

		void set_upload_path(std::string const& path) {
			upload_path_ = path;
		}

		std::string upload_path() {
			return upload_path_;
		}

		void set_chunked_size(std::uint64_t size) {
			if (size > 0) {
				chunked_size_ = size;
			}
		}

		std::uint64_t chunked_size() {
			return chunked_size_;
		}

		///添加视图的处理方法
		void add_view_method(std::string const& name,std::size_t args_number, inja::CallbackFunction const& callback) {
			http_router_.view_method_map_.insert(std::make_pair(name, std::make_pair(args_number,callback)));
		}

		void on_error(std::function<void(std::string const&)>&& event) {
			utils::messageCenter::get().set_handler(event);
		}

		void trigger_error(std::string const& ec) {
			utils::messageCenter::get().trigger_message(ec);
		}

		template<typename T = default_session_storage>
		void set_session_storager(std::unique_ptr<T>&& handler) {
			static_assert(std::is_base_of<session_storage, T>::value, "set storage is not base on session_storage!");
			session_manager<class session>::get().set_storage(std::move(handler));
			session_manager<class session>::get().get_storage().init();  //初始化 
		}

		void set_check_session_rate(std::time_t seconds) {
			http_router_.check_session_time_ = seconds;
		}

		std::time_t check_session_rate() {
			return http_router_.check_session_time_;
		}
		//void set_url_redirect(bool flag) {
		//	http_router_.url_redirect_ = flag;
		//}
		//bool url_redirect() {
		//	return http_router_.url_redirect_;
		//}
		void set_websocket_check_alive_time(std::time_t seconds) {
			http_router_.websokcets().set_check_alive_time(seconds);
		}
		std::time_t websocket_check_alive_time() {
			return http_router_.websokcets().get_check_alive_time();
		}
		void set_websocket_frame_size(std::size_t size) {
			http_router_.websokcets().set_frame_data_size(size);
		}
		std::size_t websocket_frame_size() {
			return http_router_.websokcets().get_frame_data_size();
		}
		void set_not_found_callback(std::function<void(request&,response&)> const& callback) {
			http_router_.not_found_callback_ = callback;
		}
		void remove_not_found_callback() {
			http_router_.not_found_callback_ = nullptr;
		}
		void set_keep_alive_wait_time(std::time_t time) {
			keep_alive_wait_time_ = time;
		}
		std::time_t keep_alive_wait_time() {
			return keep_alive_wait_time_;
		}
		void set_wait_read_time(std::time_t seconds) {
			wait_read_time_ = seconds;
		}
		std::time_t wait_read_time() {
			return wait_read_time_;
		}
		void set_wait_write_time(std::time_t seconds) {
			wait_write_time_ = seconds;
		}
		std::time_t wait_write_time() {
			return wait_write_time_;
		}
		void set_disable_auto_create_directories(bool flag) {
			disable_auto_create_directories_ = flag;
		}
		bool disable_auto_create_directories() {
			return disable_auto_create_directories_;
		}
		void set_default_session_save_path(std::string const& path) {
			default_storage_session_path_ = path;
		}
		std::string default_session_save_path() {
			return default_storage_session_path_;
		}
		void set_disable_register_static_handler(bool flag) {
			disable_auto_register_static_handler_ = flag;
		}
		bool disable_register_static_handler() {
			return disable_auto_register_static_handler_;
		}
	private:
		bool listen(asio::ip::tcp::resolver::query& query) {
			bool result = false;
			asio::ip::tcp::resolver resolver(ioservice_pool_handler_.get_io());
			auto endpoints  = resolver.resolve(query);
			for (; endpoints != asio::ip::tcp::resolver::iterator(); ++endpoints) {
				asio::ip::tcp::endpoint endpoint = *endpoints;
				acceptor_.open(endpoint.protocol());
				acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
				try {
					acceptor_.bind(endpoint);
					acceptor_.listen();
					start_acceptor();
					result = true;
				}
				catch (std::exception const& e) {
					result = false;
					utils::messageCenter::get().trigger_message(e.what());
				}
			}
			return result;
		}
	public:
		template<http_method...Methods,typename Function,typename...Args>
		typename std::enable_if<!std::is_same<typename std::remove_reference<Function>::type,websocket_event>::value>::type router(nonstd::string_view url, Function&& function, Args&&...args) {
			auto method_names = http_method_str<Methods...>::methods_to_name();
			http_router_.router(url, std::move(method_names), std::forward<Function>(function), std::forward<Args>(args)...);
		}
		void router(nonstd::string_view url, websocket_event const& event) {
			http_router_.websockets_.add_event(view2str(url), event);
		}
	private:
		void start_acceptor() {
			auto connector = std::make_shared<connection>(ioservice_pool_handler_.get_io(), http_router_, static_path_, upload_path_);
			connector->set_chunked_size(chunked_size_);
			connector->set_keep_alive_wait_time(keep_alive_wait_time_);
			connector->set_wait_read_time(wait_read_time_);
			connector->set_wait_write_time(wait_write_time_);
			acceptor_.async_accept(connector->get_socket(), [this,connector](std::error_code const& ec) {
				if (!ec) {
					connector->get_socket().set_option(asio::ip::tcp::no_delay(true));
					connector->read_header();
				}
				else {
					utils::messageCenter::get().trigger_message(std::string("accept error: ") + ec.message());
				}
				start_acceptor();
			});
		}
	private:
		void register_static_router() {
			router<GET,HEAD>(nonstd::string_view{ static_url_.data(),static_url_.size() }, [this](request& req,response& res) {  //静态文件处理
				auto url = req.url();
				auto decode_path = url_decode(view2str(url));
				auto p = fs::path("."+ decode_path);
				auto content_type = get_content_type(p.extension());
				auto ab = fs::absolute(p);
				if (ab.parent_path() == fs::current_path()) { //目录到了程序文件的目录 视为不合法请求
					res.write_string("", false, http_status::bad_request, view2str(content_type));
				}
				else {
#ifdef _WIN32
					auto native_path = utf8_to_gbk(p.string());
#else // _WIN32
					auto native_path = p.string();
#endif
					res.write_file(native_path, true); //默认使用chunked方式返回文件数据
				}
			});
		}
		private:
			std::error_code port_occupied(unsigned short port) {
				asio::io_service io_service;
				asio::ip::tcp::acceptor acceptor(io_service);

				std::error_code ec;
				acceptor.open(asio::ip::tcp::v4(), ec);
#ifndef _WIN32
				acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true), ec);
#endif
				acceptor.bind({ asio::ip::tcp::v4(), port }, ec);

				return ec;
		}
	private:
		ioservice_pool ioservice_pool_handler_;
		asio::ip::tcp::acceptor acceptor_;
		http_router http_router_;
		std::string static_path_ = "./static";
		std::string upload_path_;
		std::string static_url_;
		std::uint64_t chunked_size_ = 1*1024*1024;
		std::time_t keep_alive_wait_time_ = 30;
		std::time_t wait_read_time_ = 10;
		std::time_t wait_write_time_ = 10;
		bool disable_auto_create_directories_ = false;
		bool disable_auto_register_static_handler_ = false;
		std::string default_storage_session_path_ = "./session";
		std::unique_ptr<asio::ip::tcp::resolver::query> server_query_ = nullptr;
	};
}
