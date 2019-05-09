#pragma once
#include <string>
#include "string_view.hpp"
#include <map>
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
			if (std::atoi(get_header("content-length").data()) > 0) {
				return true;
			}
			return false;
		}
		nonstd::string_view get_url() const {
			return url_;
		}
		nonstd::string_view get_method() const {
			return method_;
		}
		nonstd::string_view get_version() const {
			return version_;
		}
	private:
		nonstd::string_view method_;
		nonstd::string_view url_;
		nonstd::string_view version_;
		std::map<std::string, std::string> const* headers_ = nullptr;
	};
	class response {

	};
}