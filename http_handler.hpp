#pragma once
#include <string>
#include "string_view.hpp"
#include <map>
#include "content_type.hpp"
#include "files.hpp"
#include <unordered_map>
#include <asio.hpp>
#include "response_status.hpp"
#include <vector>
#include <tuple>
namespace xfinal {


	class request;
	class response;
	class request {
		friend class connection;
	public:
		request(response& res) :res_(res) {

		}
	public:
		nonstd::string_view header(std::string const& key) const noexcept {
			auto lkey = to_lower(key);
			if (headers_ != nullptr) {
				if ((*headers_).find(lkey) != (*headers_).end()) {
					return (*headers_).at(lkey);
				}
			}
			return "";
		}
	public:
		bool has_body() const noexcept {
			if (header("content-length") == "") {
				return false;
			}
			if (std::atoi(header("content-length").data()) > 0) {
				return true;
			}
			return false;
		}
		std::size_t body_length() const noexcept {
			if (header("content-length") == "") {
				return 0;
			}
			return std::atol(header("content-length").data());
		}

		nonstd::string_view url() const noexcept {
			auto it = url_.find('?');
			if (it != nonstd::string_view::npos) {
				return url_.substr(0, it);
			}
			return url_;
		}
		nonstd::string_view param(nonstd::string_view key) noexcept {
			if (decode_url_params_.empty()) {
				auto it = url_.find('?');
				if (it != nonstd::string_view::npos) {
					decode_url_params_ = url_decode(view2str(url_.substr(it + 1, url_.size())));
					http_urlform_parser{ decode_url_params_ }.parse_data(url_params_);
				}
			}
			auto vit = url_params_.find(key);
			if (vit != url_params_.end()) {
				return vit->second;
			}
			return "";
		}

		nonstd::string_view params() const noexcept {
			auto it = url_.find('?');
			if (it != nonstd::string_view::npos) {
				return url_.substr(it + 1, url_.size());
			}
			return "";
		}

		nonstd::string_view raw_url()  const noexcept {
			return url_;
		}

		template<typename T>
		typename std::enable_if<std::is_same<T, GBK>::value, nonstd::string_view>::type param(nonstd::string_view key) {
			auto view = param(key);
			auto key_str = view2str(key);
			if (!view.empty()) {
				auto it = gbk_decode_params_.find(key_str);
				if (it != gbk_decode_params_.end()) {
					return { it->second.data(),it->second.size() };
				}
				else {
					gbk_decode_params_.insert(std::make_pair(key_str, utf8_to_gbk(view2str(view))));
					auto& v = gbk_decode_params_[key_str];
					return { v.data(),v.size() };
				}
			}
			else {
				return "";
			}
		}

		filewriter const& file(nonstd::string_view filename) const noexcept {
			if (multipart_files_map_ != nullptr) {
				auto it = multipart_files_map_->find(view2str(filename));
				if (it != multipart_files_map_->end()) {
					return it->second;
				}
			}
			return *oct_steam_;
		}

		filewriter const& file() const noexcept {
			return *oct_steam_;
		}

		nonstd::string_view method() const noexcept {
			return method_;
		}
		nonstd::string_view http_version() const noexcept {
			return version_;
		}

		enum content_type content_type() const noexcept {
			return content_type_;
		}

		nonstd::string_view query(nonstd::string_view key) const noexcept {
			auto it = form_map_.find(key);
			if (it != form_map_.end()) {
				return it->second;
			}
			else {
				if (multipart_form_map_ != nullptr) {
					auto itm = multipart_form_map_->find(view2str(key));
					if (itm != multipart_form_map_->end()) {
						return nonstd::string_view{ itm->second.data(),itm->second.size() };
					}
					else {
						return "";
					}
				}
				return "";
			}
		}

		template<typename T>
		typename std::enable_if<std::is_same<T, GBK>::value, nonstd::string_view>::type query(nonstd::string_view key) {
			auto view = query(key);
			auto key_str = view2str(key);
			if (!view.empty()) {
				auto it = gbk_decode_form_map_.find(key_str);
				if (it != gbk_decode_form_map_.end()) {
					return { it->second.data(),it->second.size() };
				}
				else {
					gbk_decode_form_map_.insert(std::make_pair(key_str, utf8_to_gbk(view2str(view))));
					auto& v = gbk_decode_form_map_[key_str];
					return { v.data(),v.size() };
				}
			}
			else {
				return "";
			}
		}

		std::map<nonstd::string_view, nonstd::string_view> const& url_form() const noexcept {
			return form_map_;
		}

		std::map<std::string, std::string> const& multipart_form() const noexcept {
			return *multipart_form_map_;
		}

		nonstd::string_view body() const noexcept {
			return body_;
		}

		bool accept_range(std::uint64_t& pos) {
			auto range = header("range");
			if (range.empty()) {
				return false;
			}
			auto posv = range.substr(range.find('=')+1);
			pos = std::atoll(posv.data());
			return true;
		}
	protected:
		void init_content_type() noexcept {
			auto it = headers_->find("content-type");
			if (it != headers_->end()) {
				auto& value = it->second;
				auto view = nonstd::string_view(value.data(), value.size());
				auto has_op = view.find(';');
				if (has_op == nonstd::string_view::npos) {
					auto key = view2str(view);
					if (content_type_str_type_map.count(key) != 0) {
						content_type_ = content_type_str_type_map[key];
					}
					else {
						content_type_ = content_type::string;
					}
				}
				else {
					auto value_view = view.substr(0, has_op);
					auto key = view2str(value_view);
					if (content_type_str_type_map.count(key) != 0) {
						content_type_ = content_type_str_type_map[key];
					}
					else {
						content_type_ = content_type::string;
					}
				}
			}
			else {
				content_type_ = content_type::unknow;
			}
			if (content_type_ == content_type::multipart_form) {
				auto const & v = it->second;
				auto f = v.find("boundary");
				if (f != std::string::npos) {
					auto pos = f + sizeof("boundary");
					boundary_key_ = nonstd::string_view{ &(v[pos]),(v.size() - pos) };
				}
				else {
					boundary_key_ = "";
				}
			}
		}
	private:
		nonstd::string_view method_;
		nonstd::string_view url_;
		nonstd::string_view version_;
		std::map<std::string, std::string> const* headers_ = nullptr;
		std::size_t header_length_;
		enum content_type content_type_;
		std::map<nonstd::string_view, nonstd::string_view> form_map_;
		nonstd::string_view body_;
		std::map<std::string, std::string> gbk_decode_form_map_;
		std::string decode_url_params_;
		std::map<nonstd::string_view, nonstd::string_view> url_params_;
		std::map<std::string, std::string> gbk_decode_params_;
		nonstd::string_view boundary_key_;
		std::map<std::string, std::string> const* multipart_form_map_ = nullptr;
		std::map<std::string, xfinal::filewriter> const* multipart_files_map_ = nullptr;
		xfinal::filewriter* oct_steam_;
		response& res_;
	};
	///响应
	class response {
		friend class connection;
	protected:
		enum class write_type {
			string,
			file,
			no_body
		};
	public:
		response(request& req) :req_(req) {
			add_header("server", "xfinal");//增加服务器标识
		}
	public:
		void add_header(std::string const& k, std::string const& v) noexcept {
			header_map_[k] = v;
		}
	protected:
		template<typename T, typename U = typename std::enable_if<std::is_same<typename std::remove_reference<T>::type, std::string>::value>::type>
		void write(T&& body, http_status state = http_status::ok, std::string const& conent_type = "text/plain", bool is_chunked = false) noexcept {
			state_ = state;
			body_ = std::move(body);
			if (!is_chunked) {
				header_map_["Content-Length"] = std::to_string(body_.size());
			}
			else {
				header_map_["Transfer-Encoding"] = "chunked";
			}
			header_map_["Content-Type"] = conent_type;
			is_chunked_ = is_chunked;
		}
	public:
		void write_string(std::string&& content, bool is_chunked = false, http_status state = http_status::ok, std::string const& conent_type = "text/plain") noexcept {
			write(std::move(content), state, conent_type, is_chunked);
			write_type_ = write_type::string;
			init_start_pos_ = 0;
		}

		void write_file(std::string const& filename, bool is_chunked = false) noexcept {
			if (!filename.empty()) {
				bool b = file_.open(filename);
				if (!b) {
					write_string("", false, http_status::bad_request);
				}
				else {
					header_map_["Content-Type"] = view2str(file_.content_type());
					write_type_ = write_type::file;
					is_chunked_ = is_chunked;
					if (!is_chunked) {
						state_ = http_status::ok;
						file_.read_all(body_);
						init_start_pos_ = -1;
					}
					else {
						header_map_["Transfer-Encoding"] = "chunked";
						body_.resize((std::size_t)chunked_size_);
						std::uint64_t pos = 0;
						if (req_.accept_range(pos)) {
							state_ = http_status::partial_content;
							init_start_pos_ = pos;
							auto filesize = file_.size();
							header_map_["Content-Range"] = std::string("bytes ") + std::to_string(init_start_pos_) + std::string("-") + std::to_string(filesize - 1) + "/" + std::to_string(filesize);
						}
						else {
							state_ = http_status::ok;
							init_start_pos_ = -1;
						}
					}
				}
			}
			else {
				write_string("", false, http_status::bad_request);
			}
		}

		void redirect(nonstd::string_view url, bool is_temporary = true) noexcept {
			write_type_ = write_type::no_body;
			header_map_["Location"] = view2str(url);
			if (is_temporary) {
				state_ = http_status::moved_temporarily;
			}
			else {
				state_ = http_status::moved_permanently;
			}
		}
	private:
		std::vector<asio::const_buffer> header_to_buffer() noexcept {
			std::vector<asio::const_buffer> buffers_;
			http_version_ = view2str(req_.http_version()) + ' ';//写入回应状态行 
			buffers_.emplace_back(asio::buffer(http_version_));
			buffers_.emplace_back(http_state_to_buffer(state_));
			for (auto& iter : header_map_) {  //回写响应头部
				buffers_.emplace_back(asio::buffer(iter.first));
				buffers_.emplace_back(asio::buffer(name_value_separator.data(), name_value_separator.size()));
				buffers_.emplace_back(asio::buffer(iter.second));
				buffers_.emplace_back(asio::buffer(crlf.data(), crlf.size()));
			}
			buffers_.emplace_back(asio::buffer(crlf.data(), crlf.size())); //头部结束
			return buffers_;
		}
		std::vector<asio::const_buffer> to_buffers() noexcept {  //非chunked 模式 直接返回所有数据
			auto  buffers_ = header_to_buffer();
			//写入body
			if (write_type_ != write_type::no_body) {  //非no_body类型 
				buffers_.emplace_back(asio::buffer(body_.data(), body_.size()));
			}
			return buffers_;
		}

		std::tuple<bool, std::vector<asio::const_buffer>,std::int64_t> chunked_body(std::int64_t startpos) noexcept {
			std::vector<asio::const_buffer> buffers;
			switch (write_type_) {
			case write_type::string:  //如果是文本数据
			{
				if ((body_.size()- (std::size_t)startpos) <= chunked_size_) {
					auto size = body_.size() - startpos;
					chunked_write_size_ = to_hex(size);
					buffers.emplace_back(asio::buffer(chunked_write_size_));
					buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
					buffers.emplace_back(asio::buffer(&body_[(std::size_t)startpos], (std::size_t)size));
					buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
					return { true, buffers,0};
				}
				else {  //还有超过每次chunked传输的数据大小
					auto nstart = startpos + chunked_size_;
					chunked_write_size_ = to_hex(chunked_size_);
					buffers.emplace_back(asio::buffer(chunked_write_size_));
					buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
					buffers.emplace_back(asio::buffer(&body_[(std::size_t)startpos], (std::size_t)chunked_size_));
					buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
					return { false,buffers,nstart };
				}
			}
			break;
			case write_type::file:  //如果是文件数据
			{
				auto read_size = file_.read(startpos, &(body_[0]), chunked_size_);
				chunked_write_size_ = to_hex(read_size);
				buffers.emplace_back(asio::buffer(chunked_write_size_));
				buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
				buffers.emplace_back(asio::buffer(&body_[0], (std::size_t)read_size));
				buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
				return { (read_size < chunked_size_),buffers, -1 };
			}
				break;
			default:
				return { true,buffers ,0 };
				break;
			}
		}
	private:
		request& req_;
		std::unordered_map<std::string, std::string> header_map_;
		std::string body_;
		http_status state_;
		bool is_chunked_ = false;
		std::string http_version_;
		write_type write_type_ = write_type::string;
		std::uint64_t chunked_size_;
		std::string chunked_write_size_;
		filereader file_;
		std::int64_t init_start_pos_ ;
	};
}