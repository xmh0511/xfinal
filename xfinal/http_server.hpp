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
		http_server(std::size_t thread_size):ioservice_pool_handler_(std::unique_ptr<ioservice_pool>(new ioservice_pool(thread_size))), acceptor_(ioservice_pool_handler_->get_io()){
			static_url_ = std::string("/")+ get_root_director(static_path_)+ "/*";
			upload_path_ = static_path_ + "/upload";
			register_static_router(); //注册静态文件处理逻辑
			set_session_storager();
		}
	public:
		bool listen(std::string const& ip,std::string const& port) {
			asio::ip::tcp::resolver::query query(ip, port);
			return listen(query);
		}
	public:
		void run() {
			if (!fs::exists(upload_path_)) {
				fs::create_directories(upload_path_);
			}
			ioservice_pool_handler_->run();
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

		void trigger_error(std::exception const& ec) {
			utils::messageCenter::get().trigger_message(ec.what());
		}

		template<typename T = default_session_storage>
		void set_session_storager() {
			static_assert(std::is_base_of<session_storage, T>::value, "set storage is not base on session_storage!");
			session_manager::get().set_storage(std::make_unique<T>());
			session_manager::get().get_storage().init();  //初始化 
		}

		void set_check_session_rate(std::time_t seconds) {
			http_router_.check_session_time_ = seconds;
		}

		std::time_t check_session_rate() {
			return http_router_.check_session_time_;
		}
		void set_url_redirect(bool flag) {
			http_router_.url_redirect_ = flag;
		}
		bool url_redirect() {
			return http_router_.url_redirect_;
		}
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
	private:
		bool listen(asio::ip::tcp::resolver::query& query) {
			bool result = false;
			asio::ip::tcp::resolver resolver(ioservice_pool_handler_->get_io());
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
					std::cout << e.what() << std::endl;
					//http_router_.trigger_error(xfinal_exception{ e });
				}
			}
			return result;
		}
	public:
		template<http_method...Methods,typename Function,typename...Args>
		std::enable_if_t<!std::is_same<std::remove_reference_t<Function>,websocket_event>::value> router(nonstd::string_view url, Function&& function, Args&&...args) {
			auto method_names = http_method_str<Methods...>::methods_to_name();
			http_router_.router(url, std::move(method_names), std::forward<Function>(function), std::forward<Args>(args)...);
		}
		void router(nonstd::string_view url, websocket_event const& event) {
			http_router_.websockets_.add_event(view2str(url), event);
		}
	private:
		void start_acceptor() {
			auto connector = std::make_shared<connection>(ioservice_pool_handler_->get_io(), http_router_, static_path_, upload_path_);
			connector->set_chunked_size(chunked_size_);
			acceptor_.async_accept(connector->get_socket(), [this,connector](std::error_code const& ec) {
				if (ec) {
					return;
				}
				//connector->get_socket().set_option(asio::socket_base::linger(true, 30));
				connector->read_header();
				start_acceptor();
			});
		}
	private:
		void register_static_router() {
			router<GET>(nonstd::string_view{ static_url_.data(),static_url_.size() }, [this](request& req,response& res) {  //静态文件处理
				auto url = req.url();
				auto p = fs::path("."+view2str(url));
				auto content_type = get_content_type(p.extension());
				auto ab = fs::absolute(p);
				if (ab.parent_path() == fs::current_path()) { //目录到了程序文件的目录 视为不合法请求
					res.write_string("", false, http_status::bad_request, view2str(content_type));
				}
				else {
					res.write_file(p.string(), true); //默认使用chunked方式返回文件数据
				}
			});
		}
	private:
		std::unique_ptr<ioservice_pool> ioservice_pool_handler_;
		asio::ip::tcp::acceptor acceptor_;
		http_router http_router_;
		std::string static_path_ = "./static";
		std::string upload_path_;
		std::string static_url_;
		std::uint64_t chunked_size_ = 1*1024*1024;
	};
}