#pragma once
#include <asio.hpp>
#include "http_handler.hpp"
#include <memory>
#include <functional>
#include <string>
#include "uuid.hpp"
#include <unordered_map>
#include "string_view.hpp"
#include "utils.hpp"
#include <memory>
#include "md5.hpp"
namespace xfinal {
	struct frame_info {
		bool eof;
		int opcode;
		int mask;
		std::uint64_t payload_length;
		unsigned char mask_key[4];
	};

	class websockets;
	class websocket;
	class websocket_event final {
		friend class websockets;
	public:
		websocket_event() = default;
	public:
		template<typename Function, typename...Args>
		websocket_event& on(std::string const& event_name, Function&& function, Args&&...args) {
			event_call_back_.insert(std::make_pair(event_name, on_help<(sizeof...(Args))>(0,std::forward<Function>(function), std::forward<Args>(args)...)));
			return *this;
		}
		void trigger(std::string const& event_name, websocket& ws) {
			auto it = event_call_back_.find(event_name);
			if (it != event_call_back_.end()) {
				(it->second)(ws);
			}
		}
	private:
		template<std::size_t N,typename Function,typename...Args,typename U = std::enable_if_t<std::is_same_v<number_content<N>, number_content<0>>>>
		auto on_help(int,Function&& function, Args&&...args) {
			return std::bind(std::forward<Function>(function), std::placeholders::_1);
		}

		template<std::size_t N, typename Function, typename...Args, typename U = std::enable_if_t<!std::is_same_v<number_content<N>, number_content<0>>>>
		auto on_help(long,Function&& function, Args&&...args) {
			return std::bind(std::forward<Function>(function), std::forward<Args>(args)..., std::placeholders::_1);
		}
	private:
		std::unordered_map<std::string, std::function<void(websocket&)>> event_call_back_;
	};
	class websocket_event_map {
		friend class websockets;
		friend class websocket;
	public:
		void trigger(const std::string& url, std::string const& event_name, websocket& ws) {
			auto it = events_.find(url);
			if (it != events_.end()) {
				it->second.trigger(event_name, ws);
			}
		}
	private:
		std::unordered_map<std::string, websocket_event> events_;
		std::unordered_map<nonstd::string_view, std::shared_ptr<websocket>> websockets_;
	};
	class websocket final:public std::enable_shared_from_this<websocket>{
	public:
		websocket(websocket_event_map& web, std::string const& url) :websocket_event_manager(web), socket_uid_(uuids::uuid_system_generator{}().to_short_str()), url_(url){

		}
	public:
		std::string& uuid() {
			return socket_uid_;
		}
		void move_socket(std::unique_ptr<asio::ip::tcp::socket>&& socket) {
			socket_ = std::move(socket);
			start_read();
		}
	private:
		void start_read() {
			read_pos_ = 0;
			std::memset(frame, 0, sizeof(frame));
			std::memset(&frame_info_, 0, sizeof(frame_info_));
			socket_->async_read_some(asio::buffer(&frame[read_pos_],2), [this](std::error_code const& ec,std::size_t read_size) {
				frame_parser();
			});
		}
	public:
		nonstd::string_view messages() const {
			return message_;
		}
	private:
		void frame_parser() {
			read_pos_ += 2;
			unsigned char c = frame[0];
			frame_info_.eof = c >> 7;
			frame_info_.opcode = c & 15;
			if (frame_info_.opcode == 8) {  //关闭连接
				socket_->close();
				websocket_event_manager.websockets_.erase(nonstd::string_view(socket_uid_.data(), socket_uid_.size()));
				return;
			}
			unsigned char c2 = frame[1];
			frame_info_.mask = c2 >> 7;
			c2 = c2 & 127;
			if (c2 < 126) {  //数据的长度为当前值
				frame_info_.payload_length = c2;
				handle_payload_length(0);
			}
			else if(c2 == 126){  //后续2个字节 unsigned
				socket_->async_read_some(asio::buffer(&frame[read_pos_], 2), [this](std::error_code const& ec, std::size_t read_size) {
					handle_payload_length(2);
				});
			}
			else if(c2 == 127){  //后续8个字节 unsigned
				socket_->async_read_some(asio::buffer(&frame[read_pos_], 8), [this](std::error_code const& ec, std::size_t read_size) {
					handle_payload_length(8);
				});
			}
		}
		void handle_payload_length(std::size_t read_size) {  //if length >126
			if (read_size == 2) {
				unsigned short tmp = 0;
				netendian_to_l(tmp, &(frame[read_pos_]));
				frame_info_.payload_length = tmp;
			}
			else if(read_size==8){
				std::uint64_t tmp = 0;
				netendian_to_l(tmp, &(frame[read_pos_]));
				frame_info_.payload_length = tmp;
			}
			if (frame_info_.mask == 1) {  //应该必须等于1
				socket_->async_read_some(asio::buffer(&frame_info_.mask_key[0], 4), [this](std::error_code const& ec, std::size_t read_size) {
					read_data();
				});
			}
		}
		void read_data() {
			expand_buffer(frame_info_.payload_length);
			socket_->async_read_some(asio::buffer(&buffers_[data_current_pos_], frame_info_.payload_length), [this](std::error_code const& ec, std::size_t read_size) {
				set_current_pos(read_size);
				decode_data(read_size);
			});
		}

		void decode_data(std::size_t use_size) {
			unsigned char* iter = &(buffers_[data_current_pos_ - use_size]);
			for (auto i = 0; i < use_size;++i) {
				auto j = i % 4;
				auto mask_key = frame_info_.mask_key[j];
				*iter = (*iter) ^ mask_key;
				++iter;
			}

			if (frame_info_.eof) {
				message_ = std::string(buffers_.begin(), buffers_.begin()+ data_current_pos_);
				buffers_.resize(0);
				data_current_pos_ = 0;
				//数据帧都处理完整 回调
				websocket_event_manager.trigger(url_, "message", *this);
			}
			start_read();
		}
		void set_current_pos(std::size_t size) {
			data_current_pos_ += size;
		}
		std::size_t left_buffer_size() {
			return buffers_.size() - data_current_pos_;
		}
		void expand_buffer(std::size_t need_size) {
			auto left_size = left_buffer_size();
			if (need_size > left_buffer_size()) {
				auto total_size = buffers_.size() + (need_size - left_size);
				buffers_.resize(total_size);
			}
		}
	private:
		std::unique_ptr<asio::ip::tcp::socket> socket_;
		websocket_event_map& websocket_event_manager;
		std::string socket_uid_;
		std::vector<unsigned char> buffers_;
		unsigned char frame[10];
		frame_info frame_info_;
		std::size_t read_pos_ = 0;
		std::size_t data_current_pos_ = 0;
		std::string message_;
		std::string const url_;
	};


	class websockets final:private nocopyable
	{
		friend class connection;
	public:
		using event_reg_func = std::function<void(websocket&)>;
	public:
		std::shared_ptr<websocket> start_webscoket(std::string const& url) {
			auto ws = std::make_shared<websocket>(websocket_event_map_, url);
			auto& uuid = ws->uuid();
			websocket_event_map_.websockets_.insert(std::make_pair(nonstd::string_view{ uuid.data(),uuid.size() }, ws));
			return ws;
		}
		void add_event(std::string const& url, websocket_event const& websocket_event_) {
			websocket_event_map_.events_.insert(std::make_pair(url, websocket_event_));
		}
	private:
		bool is_websocket(request& req) {
			using namespace  nonstd::string_view_literals;
			if (req.method() != "GET"_sv) {
				return false;
			}
			auto connection = req.header("connection");
			if (connection.empty() && to_lower(view2str(connection)) != "upgrade") {
				return false;
			}
			auto upgrade = req.header("upgrade");
			if (upgrade.empty() && to_lower(view2str(upgrade)) != "websocket") {
				return false;
			}
			auto sec_webSocket_key = req.header("Sec-WebSocket-Key");
			if (sec_webSocket_key.empty()) {
				return false;
			}
			auto sec_websocket_version = req.header("Sec-WebSocket-Version");
			if (sec_websocket_version.empty()) {
				return false;
			}
			sec_webSocket_key_ = sec_webSocket_key;
			sec_websocket_version_ = sec_websocket_version;
			return true;
		}
		void update_to_websocket(response& res) {
			res.add_header("Connection", "Upgrade");
			res.add_header("Upgrade", "websocket");
			auto str = view2str(sec_webSocket_key_) + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
			auto result = to_base64(to_sha1(str));
			res.add_header("Sec-WebSocket-Accept", result);
			res.add_header("Sec-WebSocket-Version", view2str(sec_websocket_version_));
			res.write_state(http_status::switching_protocols);
		}
	private:
		nonstd::string_view sec_webSocket_key_;
		nonstd::string_view sec_websocket_version_;
		websocket_event_map websocket_event_map_;
	};

}