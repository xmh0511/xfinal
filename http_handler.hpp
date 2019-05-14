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
namespace xfinal {


	class request;
	class response;
	class request {
		friend class connection;
	public:
		request(response& res):res_(res){

		}
	public:
		nonstd::string_view get_header(std::string const& key) const noexcept {
			if (headers_ != nullptr) {
				if ((*headers_).find(key) != (*headers_).end()) {
					return (*headers_).at(key);
				}
			}
			return "";
		}
	public:
		bool has_body() const noexcept {
			if (get_header("content-length") == "") {
				return false;
			}
			if (std::atoi(get_header("content-length").data()) > 0) {
				return true;
			}
			return false;
		}
		std::size_t body_length() const noexcept {
			if (get_header("content-length") == "") {
				return 0;
			}
			return std::atol(get_header("content-length").data());
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
					decode_url_params_ = url_decode(view2str(url_.substr(it+1, url_.size())));
					http_urlform_parser{ decode_url_params_ }.parse_data(url_params_);
				}
			}
			auto vit = url_params_.find(key);
			if (vit != url_params_.end()) {
				return vit->second;
			}
			return "";
		}

		template<typename T>
		std::enable_if_t<std::is_same_v<T, GBK>, nonstd::string_view> param(nonstd::string_view key) {
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

		file const& file(nonstd::string_view filename) const noexcept {
			if (multipart_files_map_ != nullptr) {
				auto it = multipart_files_map_->find(view2str(filename));
				if (it != multipart_files_map_->end()) {
					return it->second;
				}
			}
			return nullfile_;
		}

		nonstd::string_view method() const noexcept {
			return method_;
		}
		nonstd::string_view http_version() const noexcept {
			return version_;
		}

		content_type content_type() const noexcept {
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
		std::enable_if_t<std::is_same_v<T, GBK>, nonstd::string_view> query(nonstd::string_view key) {
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
					boundary_key_ = nonstd::string_view{ &(v[pos]),(v.size()- pos) };
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
		std::map<std::string, xfinal::file> const* multipart_files_map_ = nullptr;
		xfinal::file nullfile_;
		response& res_;
	};

	class response {
		friend class connection;
	public:
		response(request& req):req_(req){
			add_header("server", "xfinal");//增加服务器标识
		}
	public:
		void add_header(std::string const& k,std::string const& v) {
			header_map_[k] = v;
		}
		template<typename T,typename U = std::enable_if_t<std::is_same_v<std::remove_reference_t<T>,std::string>>>
		void write(T&& body, http_status state = http_status::ok,std::string const& conent_type = "text/plain",bool is_chunked = false) {
			state_ = state;
			body_ = std::move(body);
			if (!is_chunked) {
				header_map_["content-length"] = std::to_string(body_.size());
			}
			header_map_["content-type"] = conent_type;
			is_chunked_ = is_chunked;
		}
	public:
		void write_string(std::string&& content, http_status state = http_status::ok, bool is_chunked = false) {
			write(std::move(content), state,"text/plain", is_chunked);
		}
	private:
		std::vector<asio::const_buffer> to_buffers() {
			std::vector<asio::const_buffer> buffers_;
			http_version_ = view2str(req_.http_version())+' ';//写入回应状态行 
			buffers_.emplace_back(asio::buffer(http_version_));
			buffers_.emplace_back(http_state_to_buffer(state_));
			for (auto& iter : header_map_) {  //回写响应头部
				buffers_.emplace_back(asio::buffer(iter.first));
				buffers_.emplace_back(asio::buffer(name_value_separator.data(), name_value_separator.size()));
				buffers_.emplace_back(asio::buffer(iter.second));
				buffers_.emplace_back(asio::buffer(crlf.data(),crlf.size()));
			}
			buffers_.emplace_back(asio::buffer(crlf.data(), crlf.size())); //头部结束
			//写入body
			buffers_.emplace_back(asio::buffer(body_.data(), body_.size()));
			return buffers_;
		}
	private:
		request& req_;
		std::unordered_map<std::string, std::string> header_map_;
		std::string body_;
		http_status state_;
		bool is_chunked_ = false;
		std::string http_version_;
	};
}