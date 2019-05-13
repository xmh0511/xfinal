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
			auto total_size = buffers_.capacity() + expand_buffer_size;
			if (total_size > max_buffer_size_) {
				left_buffer_size_ = 0;
				return false;
			}
			buffers_.resize(total_size);
			left_buffer_size_ = buffers_.capacity() - current_use_pos_;
			return true;
		}
		void set_current_pos(std::size_t addpos) {
			current_use_pos_ += addpos;
		}
		void post_router() {
			router_.post_router(req_, res_);
			write();
		}
		void parse_header(http_parser_header& parser) {  //解析请求头
			auto request = parser.parse_request_header();
			if (request.first) {
				request_info = std::move(request.second);
				req_.method_ = request_info.method_;
				req_.url_ = request_info.url_;
				req_.headers_ = &(request_info.headers_);
				req_.header_length_ = parser.get_header_size();
				handle_read();
				return;
			}
			//http request error;
		}
		void handle_read() {  //读取请求头完毕后 开始下一步处理
			req_.init_content_type();
			if (req_.has_body()) {  //has request body
				auto type = req_.content_type();
				switch (type)
				{
				case xfinal::content_type::url_encode_form:
				case xfinal::content_type::string:
				{
					pre_process_body(type);
				}
					break;
				case xfinal::content_type::multipart_form:
					break;
				case xfinal::content_type::unknow:
				{
					return;
				}
					break;
				default:
				{
					return;
				}
					break;
				}
			}
			else {
				post_router();  //如果没有body部分
			}
		}

		void handle_body(content_type type,std::size_t body_length) {  //读取完整的body后 更具content_type 进行处理
			switch (type)
			{
			case content_type::url_encode_form:
			{
				process_url_form(body_length);
			}
				break;
			case content_type::multipart_form:
				break;
			case content_type::octet_stream:
				break;
			case content_type::string: 
			{
				process_string();
			}
				break;
			default:
				break;
			}
		}
        /// 处理有content-length的body  读取完整body部分  ---start
		void pre_process_body(content_type type) {  //读body 的预处理
			auto header_size = req_.header_length_;
			if (current_use_pos_ > header_size) {  //是不是读头的时候 把body也读取进来了 
				forward_contain_data(buffers_, header_size, current_use_pos_);
				current_use_pos_ -= header_size;
				left_buffer_size_ = buffers_.capacity()- current_use_pos_;
				auto body_length = req_.body_length();
				if (body_length > current_use_pos_) {    //body没有读完整
					continue_read_data([handler = this->shared_from_this(), type]() {
						handler->process_body(type);
					});
				}
				else {  //开始更具content_type 解析数据 并执行路由
					request_info.body_ = std::string(buffers_.data(), body_length);
					handle_body(type, body_length);
					post_router();  //处理完毕 执行路由
				}
			}
		}
		void process_body(content_type type) {  //如果预读取body的时候没有读完整body 
			auto body_length = req_.body_length();
			if (body_length > current_use_pos_) {  //body没有读完整
				continue_read_data([handler = this->shared_from_this(), type]() {
					handler->process_body(type);
				});
			}
			else { //开始更具content_type 解析数据 并执行路由
				request_info.body_ = std::string(buffers_.data(), body_length);
				handle_body(type, body_length);
				post_router(); //处理完毕 执行路由
			}
		}
		/// 处理有content-length的body  读取完整body部分 ---end

		void process_url_form(std::size_t body_length) { // 处理application/x-www-form-urlencoded 类型请求
			request_info.decode_body_ = request_info.body_;
			req_.body_ = request_info.body_;
			http_urlform_parser parser{ request_info .decode_body_ };
			parser.parse_data(request_info.form_map_);
			req_.form_map_ = request_info.form_map_;
		}

		void process_string() {  //处理诸如 json text 等纯文本类型的body
			req_.body_ = nonstd::string_view(request_info.body_);
		}

	public:
		template<typename Function>
		void continue_read_data(Function&& callback) { //用来继续读取body剩余数据的
			bool b = expand_size();
			if (!b) {
				return; //超过最大可接受字节数 等待处理;
			}
			socket_->async_read_some(asio::buffer(&buffers_[current_use_pos_], left_buffer_size_), [handler = this->shared_from_this(),function = std::move(callback)](std::error_code const& ec, std::size_t read_size) {
				if (ec) {
					return;
				}
				handler->set_current_pos(read_size);
				function();
			});
		}
		void read_header() {  //有连接请求后 开始读取请求头
			socket_->async_read_some(asio::buffer(&buffers_[current_use_pos_], left_buffer_size_), [handler = this->shared_from_this()](std::error_code const& ec, std::size_t read_size){
				if (ec) {
					return;
				}
				handler->set_current_pos(read_size);
				auto& buffer = handler->get_buffers();
				//std::cout << handler->get_buffers().data() << std::endl;
				http_parser_header parser{ buffer.begin(),buffer.end() };
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
			});
		}
		void write() {
			socket_->async_write_some(asio::buffer("HTTP/1.1 200 OK\r\nServer: Apache-Coyote/1.1"), [handler = this->shared_from_this()](std::error_code const& ec, std::size_t write_size) {
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
		xfinal::request_meta request_info;
		request req_;
		response res_;
		http_router& router_;
	};
}