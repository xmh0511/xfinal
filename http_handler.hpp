#pragma once
#include <string>
#include "string_view.hpp"
#include <map>
#include "content_type.hpp"
namespace xfinal {


	class request;
	class response;
	class request {
		friend class connection;
	public:
		request() = default;
	public:
		nonstd::string_view get_header(std::string const& key) const {
			if (headers_ != nullptr) {
				if ((*headers_).find(key) != (*headers_).end()) {
					return (*headers_).at(key);
				}
			}
			return "";
		}
	public:
		bool has_body() const {
			if (get_header("content-length") == "") {
				return false;
			}
			if (std::atoi(get_header("content-length").data()) > 0) {
				return true;
			}
			return false;
		}
		std::size_t body_length() const {
			if (get_header("content-length") == "") {
				return 0;
			}
			return std::atol(get_header("content-length").data());
		}

		nonstd::string_view url() const {
			auto it = url_.find('?');
			if (it != nonstd::string_view::npos) {
				return url_.substr(0, it);
			}
			return url_;
		}
		nonstd::string_view param(nonstd::string_view key) {
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
				auto it = gbk_decode_url_params_.find(key_str);
				if (it != gbk_decode_url_params_.end()) {
					return { it->second.data(),it->second.size() };
				}
				else {
					gbk_decode_url_params_.insert(std::make_pair(key_str, utf8_to_gbk(view2str(view))));
					auto& v = gbk_decode_url_params_[key_str];
					return { v.data(),v.size() };
				}
			}
			else {
				return "";
			}
		}

		nonstd::string_view method() const {
			return method_;
		}
		nonstd::string_view http_version() const {
			return version_;
		}

		content_type content_type() const {
			return content_type_;
		}

		nonstd::string_view query(nonstd::string_view key) const {
			auto it = form_map_.find(key);
			if (it != form_map_.end()) {
				return it->second;
			}
			else {
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

		nonstd::string_view body() const {
			return body_;
		}
		protected:
		void init_content_type() {
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
		std::map<std::string, std::string> gbk_decode_url_params_;
		nonstd::string_view boundary_key_;
	};
	class response {

	};
}