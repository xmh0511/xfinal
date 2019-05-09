#pragma once
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <algorithm>
#include "utils.hpp"
namespace xfinal {
	enum class parse_state :std::uint8_t
	{
		valid,
		invalid,
		unknow
	};

	class request_meta {
	public:
		request_meta() = default;
		request_meta(request_meta const&) = default;
		request_meta(request_meta &&) = default;
		request_meta& operator=(request_meta const&) = default;
		request_meta& operator=(request_meta &&) = default;
		request_meta(std::string&& method, std::string&& url, std::map<std::string, std::string>&& headers):method_(std::move(method)), url_(std::move(url)), headers_(std::move(headers)){

		}
	public:
		std::string method_;
		std::string url_;
		std::map<std::string, std::string> headers_;
	};

	class http_parser {
	public:
		http_parser(std::vector<char>::iterator begin, std::vector<char>::iterator end) :begin_(begin), end_(end) {

		}
		std::pair<parse_state, bool> is_complete_header() {
			auto current = begin_;
			while (current != end_) {
				if (*current == '\r' && *(current + 1) == '\n' && *(current + 2) == '\r' && *(current + 3) == '\n') {
					header_end_ = current - 1;
					return { parse_state::valid,true };
				}
				++current;
			}
			return { parse_state::invalid,false};
		}
		std::pair < parse_state, std::string> get_method() {
			auto start = begin_;
			while (begin_ != end_) {
				//auto c = *begin_;
				//auto c_next = *(begin_ + 1);
				if ((*begin_) == ' ' && (*(begin_ + 1)) !=' ') {
					return { parse_state::valid,std::string(start, begin_++) };
				}
				++begin_;
			}
			return { parse_state::invalid,""};
		}
		std::pair < parse_state, std::string> get_url() {
			auto start = begin_;
			while (begin_ != end_) {
				if ((*begin_) == ' ' && (*(begin_ + 1)) != ' ') {
					return { parse_state::valid,std::string(start, begin_++) };
				}
				++begin_;
			}
			return { parse_state::invalid,"" };
		}
		std::pair < parse_state, std::string> get_version() {
			auto start = begin_;
			while (begin_ != end_) {
				if ((*begin_) == '\r' && (*(begin_ + 1)) == '\n') {
					begin_ += 2;
					return { parse_state::valid ,std::string(start,begin_-2) };
				}
				++begin_;
			}
			return { parse_state::invalid,"" };
		}
		std::pair < parse_state, std::map<std::string, std::string>> get_header() {
			std::map<std::string, std::string> headers;
			auto start = begin_;
			while (begin_ != end_) {
				if ((*begin_) == '\r' && (*(begin_ + 1)) == '\n') {  //maybe a header = > key:value
					auto old_start = start;
					while (start != begin_) {
						if ((*start) == ':') {
							headers.emplace(to_lower(std::string(old_start, start)), std::string(start+1, begin_));
							begin_ += 2;
							start = begin_;
							break;
						}
						++start;
					}
					if (start != begin_) {
						return { parse_state::invalid ,{} };
					}
				}
				++begin_;
			}
			return { parse_state::valid ,headers };
		}
		std::pair<bool, request_meta> parse_request_header() {
			auto method = get_method();
			if (method.first == parse_state::valid) {
				auto url = get_url();
				if (url.first == parse_state::valid) {
					auto version = get_version();
					if (version.first == parse_state::valid) {
						auto headers = get_header();
						if (headers.first == parse_state::valid) {
							return { true,{std::move(method.second),std::move(url.second),std::move(headers.second)} };
						}
						else {
							return { false,request_meta() };
						}
					}
					else {
						return { false,request_meta() };
					}
				}
				else {
					return { false,request_meta() };
				}
			}
			else {
				return { false,request_meta() };
			}
		}
	private:
		std::vector<char>::iterator begin_;
		std::vector<char>::iterator end_;
		std::vector<char>::iterator header_end_;
	};
}