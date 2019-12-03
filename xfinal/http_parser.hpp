#pragma once
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <algorithm>
#include "utils.hpp"
#include "code_parser.hpp"
#include "string_view.hpp"
#include "files.hpp"
namespace xfinal {
	enum class parse_state :std::uint8_t
	{
		valid,
		invalid,
		unknow
	};

	enum class multipart_data_state :std::uint8_t {
		is_end,
		all_part_data,
		maybe_end
	};

	class request_meta :private nocopyable {
	public:
		request_meta() = default;
		//request_meta(request_meta const&) = default;
		//request_meta(request_meta &&) = default;
		//request_meta& operator=(request_meta const&) = default;
		//request_meta& operator=(request_meta &&) = default;
		request_meta(std::string&& method, std::string&& url, std::string&& version, std::map<std::string, std::string>&& headers) :method_(std::move(method)), url_(std::move(url)), version_(std::move(version)), headers_(std::move(headers)) {

		}
		request_meta(request_meta&& r) :method_(std::move(r.method_)), url_(std::move(r.url_)), version_(std::move(r.version_)), headers_(std::move(r.headers_)), form_map_(std::move(r.form_map_)), body_(std::move(r.body_)), decode_body_(std::move(r.decode_body_)), multipart_form_map_(std::move(r.multipart_form_map_)), multipart_files_map_(std::move(r.multipart_files_map_)), oct_steam_(std::move(r.oct_steam_)) {

		}
		request_meta& operator=(request_meta&& r) {
			method_ = std::move(r.method_);
			url_ = std::move(r.url_);
			version_ = std::move(r.version_);
			headers_ = std::move(r.headers_);
			form_map_ = std::move(r.form_map_);
			body_ = std::move(r.body_);
			decode_body_ = std::move(r.body_);
			multipart_form_map_ = std::move(r.multipart_form_map_);
			multipart_files_map_ = std::move(r.multipart_files_map_);
			oct_steam_ = std::move(r.oct_steam_);
			return *this;
		}
		void reset() {
			method_.clear();
			url_.clear();
			version_.clear();
			headers_.clear();
			form_map_.clear();
			body_.clear();
			decode_body_.clear();
			multipart_form_map_.clear();
			multipart_files_map_.clear();
			oct_steam_.close();
		}
	public:
		std::string method_;
		std::string url_;
		std::string version_;
		std::map<std::string, std::string> headers_;
		std::map<nonstd::string_view, nonstd::string_view> form_map_;
		std::string body_;
		std::string decode_body_;
		std::map<std::string, std::string> multipart_form_map_;
		std::map<std::string, filewriter> multipart_files_map_;
		filewriter oct_steam_;
	};

	class http_parser_header final {
	public:
		http_parser_header(std::vector<char>::iterator begin, std::vector<char>::iterator end) :begin_(begin), end_(end) {

		}
		std::pair<parse_state, bool> is_complete_header() {
			auto current = begin_;
			header_begin_ = begin_;
			while (current != end_) {
				if (*current == '\r' && *(current + 1) == '\n' && *(current + 2) == '\r' && *(current + 3) == '\n') {
					header_end_ = current + 4;
					return { parse_state::valid,true };
				}
				++current;
			}
			return { parse_state::valid,false };
		}
		std::pair < parse_state, std::string> get_method() {
			auto start = begin_;
			while (begin_ != end_) {
				//auto c = *begin_;
				//auto c_next = *(begin_ + 1);
				if ((*begin_) == ' ' && (*(begin_ + 1)) != ' ') {
					return { parse_state::valid,std::string(start, begin_++) };
				}
				++begin_;
			}
			return { parse_state::invalid,"" };
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
					return { parse_state::valid ,std::string(start,begin_ - 2) };
				}
				++begin_;
			}
			return { parse_state::invalid,"" };
		}
		std::pair < parse_state, std::map<std::string, std::string>> get_header() {
			if ((*begin_) == '\r' && (*(begin_ + 1)) == '\n') {  //没有键值对
				return { parse_state::valid ,{} };
			}
			std::map<std::string, std::string> headers;
			char r = ' ';
			char n = ' ';
			auto start = begin_;
			while (begin_ != end_) {
				if ((*begin_) == '\r' && (*(begin_ + 1)) == '\n') {  //maybe a header = > key:value
					if ((end_ - begin_) >= 4) {
						r = *(begin_ + 2);
						n = *(begin_ + 3);
					}
					auto old_start = start;
					while (start != begin_) {
						if ((*start) == ':') {
							auto key = std::string(old_start, start);
							++start;
							while ((*start) == ' ') {
								++start;
							}
							headers.emplace(to_lower(std::move(key)), std::string(start, begin_));
							begin_ += 2;
							start = begin_;
							break;
						}
						++start;
					}
					if (start != begin_) {
						return { parse_state::invalid ,{} };
					}
					if (r == '\r'&& n == '\n') {
						break;
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
							return std::pair<bool, request_meta>(true, request_meta{ std::move(method.second),std::move(url.second),std::move(version.second),std::move(headers.second) });
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
		std::size_t get_header_size() {
			return (header_end_ - header_begin_);
		}
	private:
		std::vector<char>::iterator begin_;
		std::vector<char>::iterator end_;
		std::vector<char>::iterator header_end_;
		std::vector<char>::iterator header_begin_;
	};

	class http_urlform_parser final {
	public:
		http_urlform_parser(std::string& body, bool need_url_decode = true) {
			if (need_url_decode) {
				body = xfinal::get_string_by_urldecode(body);
			}
			begin_ = body.begin();
			end_ = body.end();
		}
		void parse_data(std::map<nonstd::string_view, nonstd::string_view>& form) {
			auto old = begin_;
			while (begin_ != end_) {
				if ((begin_ + 1) == end_) {
					begin_++;
					parse_key_value(form, old);
					break;
				}
				if ((*begin_) == '&') {
					parse_key_value(form, old);
					++begin_;
					old = begin_;
				}
				++begin_;
			}
		}
	protected:
		void parse_key_value(std::map<nonstd::string_view, nonstd::string_view>& form, std::string::iterator old) {
			auto start = old;
			while (old != begin_) {
				if ((*old) == '=') {
					auto key = nonstd::string_view(&(*start), old - start);
					if ((old + 1) != end_) {
						form.insert(std::make_pair(key, nonstd::string_view(&(*(old + 1)), begin_ - 1 - old)));
					}
					else {
						form.insert(std::make_pair(key, ""));
					}
					break;
				}
				++old;
			}
		}
	private:
		std::string::iterator begin_;
		std::string::iterator end_;
	};

	class http_multipart_parser final {
	public:
		http_multipart_parser(std::string const & boundary_start_key_, std::string const & boundary_end_key_) :boundary_start_key_(boundary_start_key_), boundary_end_key_(boundary_end_key_) {

		}
	public:
		bool is_complete_part_header(std::vector<char>::iterator begin_, std::vector<char>::iterator end_) const {
			nonstd::string_view buffer{ &(*begin_) ,std::size_t(end_ - begin_) };
			auto it = buffer.find(boundary_start_key_);
			if (it != nonstd::string_view::npos) {
				auto it2 = buffer.find("\r\n\r\n");
				if (it2 != nonstd::string_view::npos) {
					return true;
				}
				else {
					return false;
				}
			}
			else {
				return false;
			}
		}
		std::pair<multipart_data_state, std::size_t> is_end_part_data(char const* buffers, std::size_t size) const {
			nonstd::string_view buffer{ buffers ,size };
			auto it = buffer.find(boundary_start_key_);
			if (it != nonstd::string_view::npos) {
				//std::cout << buffer.substr( it ,boundary_start_key_.size() ) << std::endl;
				return { multipart_data_state::is_end, it - 2 };
			}
			else {
				auto it2 = buffer.find(boundary_end_key_);
				if (it2 != nonstd::string_view::npos) {
					return { multipart_data_state::is_end ,it2 - 2 };
				}
				else {
					auto it3 = buffer.rfind("\r");
					if (it3 != nonstd::string_view::npos) {
						auto padding_start = it3 + boundary_start_key_.size() + 1;
						auto padding_end = it3 + boundary_end_key_.size() + 1;
						if ((padding_start <= size) || (padding_end <= size)) {
							return { multipart_data_state::all_part_data,0 };
						}
						return { multipart_data_state::maybe_end ,it3 };
					}
					return { multipart_data_state::all_part_data,0 };
				}
			}
		}

		std::pair<std::size_t, std::map<std::string, std::string>> parser_part_head(std::vector<char>::iterator begin_, std::vector<char>::iterator end_) const {
			nonstd::string_view buffer{ &(*begin_) ,std::size_t(end_ - begin_) };
			auto it = buffer.find(boundary_start_key_);
			if (it != nonstd::string_view::npos) {
				auto parse_begin = it + boundary_start_key_.size() + 2;
				auto parse_end = buffer.find("\r\n\r\n", parse_begin);
				if (parse_end != nonstd::string_view::npos) {
					auto head_size = parse_end + 4;
					auto multipart_head = get_header(buffer.begin() + parse_begin, buffer.begin() + head_size);
					return { head_size,multipart_head };
				}
				else {
					return { 0,{} };
				}
			}
			else {
				return { 0,{} };
			}
		}
		bool is_end(std::vector<char>::iterator begin_, std::vector<char>::iterator end_) const {
			nonstd::string_view buffer{ &(*begin_) ,std::size_t(end_ - begin_) };
			auto e = std::string(&(*begin_), boundary_end_key_.size());
			if (e == boundary_end_key_) {
				return true;
			}
			return false;
		}
	protected:
		std::map<std::string, std::string> get_header(nonstd::string_view::iterator start, nonstd::string_view::iterator stop) const {
			std::map<std::string, std::string> headers;
			auto start_ = start;
			while (start != stop) {
				//auto c = *start;
				if ((start + 1) >= stop) {
					break;
				}
				//auto c_next = *(start + 1);
				if (((*start) == '\r') && ((*(start + 1)) == '\n')) {  //maybe a header = > key:value
					auto old_start = start_;
					while (start_ != start) {
						if ((*start_) == ':') {
							auto key = std::string(old_start, start_);
							++start_;
							while ((*start_) == ' ') {
								++start_;
							}
							headers.emplace(to_lower(std::move(key)), std::string(start_, start));
							start += 2;
							start_ = start;
							break;
						}
						++start_;
					}
					if (start_ != start) {
						return {};
					}
				}
				++start;
			}
			return headers;
		}
	private:
		std::string boundary_start_key_;
		std::string boundary_end_key_;
	};
}
