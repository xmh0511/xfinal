#pragma once
#include <asio.hpp>
#include <string>
#include "ioservice_pool.hpp"
#include "connection.hpp"
#include "utils.hpp"
#include "http_router.hpp"
namespace xfinal {
	constexpr http_method GET = http_method::GET;
	constexpr http_method POST = http_method::POST;
	constexpr http_method HEAD = http_method::HEAD;
	constexpr http_method PUT = http_method::PUT;
	constexpr http_method OPTIONS = http_method::OPTIONS;
	constexpr http_method TRACE = http_method::TRACE;
	constexpr http_method DEL = http_method::DEL;
	constexpr http_method MKCOL = http_method::MKCOL;
	constexpr http_method MOVE = http_method::MOVE;

	class http_server :private nocopyable {
	public:
		http_server(std::size_t thread_size):ioservice_pool_handler_(std::make_unique<ioservice_pool>(thread_size)), acceptor_(ioservice_pool_handler_->get_io()){
		}
	public:
		bool listen(std::string const& ip,std::string const& port) {
			asio::ip::tcp::resolver::query query(ip, port);
			return listen(query);
		}
	public:
		void run() {
			ioservice_pool_handler_->run();
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
				}
			}
			return result;
		}
	public:
		template<http_method...Methods,typename Function,typename...Args>
		void router(nonstd::string_view url, Function&& function, Args&&...args) {  
			auto method_names = http_method_str<Methods...>::template methods_to_name();
			http_router_.router(url, std::move(method_names), std::forward<Function>(function), std::forward<Args>(args)...);
		}
	private:
		void start_acceptor() {
			auto connector = std::make_shared<connection>(ioservice_pool_handler_->get_io(), http_router_);
			acceptor_.async_accept(connector->get_socket(), [this,connector](std::error_code const& ec) {
				if (ec) {
					return;
				}
				connector->read_header();
				start_acceptor();
			});
		}
	private:
		std::unique_ptr<ioservice_pool> ioservice_pool_handler_;
		asio::ip::tcp::acceptor acceptor_;
		http_router http_router_;
	};
}