#pragma once
#include <iostream>
#include <asio.hpp>
#include <memory>
#include <vector>
#include "http_parser.hpp"
#include "string_view.hpp"
#include "http_handler.hpp"
#include "http_router.hpp"

namespace xfinal {
	class connection :public std::enable_shared_from_this<connection> {
	public:
		connection(asio::io_service& io, http_router& router) :socket_(std::make_unique<asio::ip::tcp::socket>(io)), io_service_(io), buffers_(expand_buffer_size),router_(router) {
			left_buffer_size_ = buffers_.size();
		}
	public:
		asio::ip::tcp::socket& get_socket() {
			return *socket_;
		}
	public:
		std::vector<char>& get_buffers() {
			return buffers_;
		}
		bool expand_size() {
			auto total_size = buffers_.size() + expand_buffer_size;
			if (total_size > max_buffer_size_) {
				left_buffer_size_ = 0;
				return false;
			}
			buffers_.reserve(total_size);
			left_buffer_size_ = buffers_.size() - current_use_pos_;
			return true;
		}
		void set_current_pos(std::size_t addpos) {
			current_use_pos_ += addpos;
		}
		void parse_header(http_parser& parser) {
			auto request = parser.parse_request_header();
			if (request.first) {
				request_info = std::move(request.second);
				req_.method_ = request_info.method_;
				req_.url_ = request_info.url_;
				req_.headers_ = &(request_info.headers_);
				handler_read();
				return;
			}
			//http request error;
		}
		void handler_read() {
			if (req_.has_body()) {  //has request body

			}
			else {  //has no body ,just simple request
				router_.post_router(req_, res_);
			}
		}
	public:
		void read_header() {
			socket_->async_read_some(asio::buffer(&buffers_[current_use_pos_], left_buffer_size_), [handler = this->shared_from_this()](std::error_code const& ec, std::size_t read_size){
				if (ec) {
					return;
				}
				handler->set_current_pos(read_size);
				auto& buffer = handler->get_buffers();
				//std::cout << handler->get_buffers().data() << std::endl;
				http_parser parser{ buffer.begin(),buffer.end() };
				if (parser.is_complete_header().first == parse_state::valid) {
					if (parser.is_complete_header().second) {
						//std::cout << "Method " << parser.get_method().second << std::endl;
						//std::cout << "Url " << parser.get_url().second << std::endl;
						//std::cout << "Version " << parser.get_version().second << std::endl;
						handler->parse_header(parser);
					}
					else {
						handler->expand_size();
						handler->read_header();
					}
				}
				handler->write();
			});
		}
		void write() {
			socket_->async_write_some(asio::buffer("HTTP/1.1  200  OK\r\nServer: Apache-Coyote/1.1"), [handler = this->shared_from_this()](std::error_code const& ec, std::size_t write_size) {
				handler->close();
			});
		}
	public:
		void close() {
			socket_->close();
		}
	private:
		std::unique_ptr<asio::ip::tcp::socket> socket_;
		asio::io_service& io_service_;
		std::size_t expand_buffer_size = 1024;
		std::size_t max_buffer_size_ = 3 * 1024 * 1024;
		std::vector<char> buffers_;
		std::size_t current_use_pos_ = 0;
		std::size_t left_buffer_size_;
		request_meta request_info;
		request req_;
		response res_;
		http_router& router_;
	};
}