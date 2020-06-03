#pragma once
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <algorithm>
#include <functional>
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
		void reset() {
			http_header_str_.clear();
			method_ = "";
			url_ = "";
			version_ = "";
			headers_.clear();
			form_map_.clear();
			body_.clear();
			//decode_body_.clear();
			multipart_form_map_.clear();
			multipart_files_map_.clear();
			oct_steam_.close();
			empty_file_.close();
			oct_steam_ = filewriter{};
			empty_file_ = filewriter{};
		}
	public:
		std::string http_header_str_;
		nonstd::string_view method_;
		nonstd::string_view url_;
		nonstd::string_view version_;
		std::map<nonstd::string_view, nonstd::string_view> headers_;
		std::map<nonstd::string_view, nonstd::string_view> form_map_;
		std::string body_;
		//std::string decode_body_;
		std::map<std::string, std::string> multipart_form_map_;
		std::map<std::string, filewriter> multipart_files_map_;
		filewriter oct_steam_;
		filewriter empty_file_;
	};

	template<typename T, typename U>
	bool less_than(T iter0, U iter1, std::size_t number) {
		if (std::size_t(iter1 - iter0) >= number) {
			return true;
		}
		return false;
	}

	template<typename T,typename U>
	T my_search(T first, T last, U s_first, U s_last) {
#ifdef  _MSVC_LANG
#if _MSVC_LANG >= 201703L
		return std::search(first, last, std::boyer_moore_searcher(s_first, s_last));
#else
		return std::search(first, last, s_first, s_last);
#endif 
#else //  _MSVC_LANG
#if __cplusplus >= 201703L
		return std::search(first, last, std::boyer_moore_searcher(s_first, s_last));
#else
		return std::search(first, last, s_first, s_last);
#endif
#endif
	}

	class http_parser_header final {
	public:
		http_parser_header(char const* begin, char const* end):begin_(begin),end_(end) {

		}
		std::pair<parse_state, bool> is_complete_header(request_meta& request_header) {
			nonstd::string_view key = "\r\n\r\n";
			auto it = my_search(begin_, end_, key.begin(), key.end());
			if (it != end_) {
				request_header.http_header_str_ = std::string(begin_, it + 4);
				auto begin = request_header.http_header_str_.data();
				begin_ = begin;
				end_ = begin + request_header.http_header_str_.size();
				return { parse_state::valid,true };
			}
			return { parse_state::valid,false };
			//auto view = nonstd::string_view(begin_, end_ - begin_);
			//auto pos = view.find("\r\n\r\n");
			//auto npos = (nonstd::string_view::size_type)nonstd::string_view::npos;
			//if (pos != npos) {
			//	auto invalid_char = view.rfind('\0', pos);
			//	if (invalid_char != npos) {
			//		return { parse_state::invalid,false };
			//	}
			//	request_header.http_header_str_ = std::string(begin_, pos + 4);
			//	auto begin = request_header.http_header_str_.data();
			//	begin_ = begin;
			//	end_ = begin + request_header.http_header_str_.size();
			//	return { parse_state::valid,true };
			//}
			//return { parse_state::valid,false };
		}
		bool get_method(request_meta& request_header) {
			auto start = begin_;
			while (begin_ != end_) {
				if (less_than(begin_, end_, 2) && (*begin_) == ' ' && (*(begin_ + 1)) != ' ') {
					request_header.method_ = nonstd::string_view(start, begin_ - start);
					++begin_;
					return true;
				}
				++begin_;
			}
			return false;
		}
		bool get_url(request_meta& request_header) {
			auto start = begin_;
			while (begin_ != end_) {
				if (less_than(begin_, end_, 2) && (*begin_) == ' ' && (*(begin_ + 1)) != ' ') {
					request_header.url_ = nonstd::string_view(start, begin_ - start);
					++begin_;
					return true;
				}
				++begin_;
			}
			return false;
		}
		bool get_version(request_meta& request_header) {
			auto start = begin_;
			while (begin_ != end_) {
				if (less_than(begin_, end_, 2) && (*begin_) == '\r' && (*(begin_ + 1)) == '\n') {
					request_header.version_ = nonstd::string_view(start, begin_ - start);
					begin_ += 2;
					return true;
				}
				++begin_;
			}
			return false;
		}
		void skip_value_white_space(const char*& it) {
			while (*it == ' ') {
				++it;
			}
		}
		bool get_header(request_meta& request_header) {
			if (less_than(begin_, end_, 2) && (*begin_) == '\r' && (*(begin_ + 1)) == '\n') {  //没有键值对
				request_header.headers_.clear();
				return true;
			}
			auto view = nonstd::string_view(begin_, end_ - begin_ - 2);
			auto end = end_ - 2;
			char const* start = begin_;
			auto key_lr = "\r\n";
			auto colon = ":";
			auto split = my_search(start, end, key_lr, key_lr + 2);
			while (split < end) {
				auto op_pos = my_search(start, split, colon, colon + 1);
				auto key_v = nonstd::string_view(start, op_pos - start);
				auto value_potential = op_pos + 1;
				skip_value_white_space(value_potential);
				auto value_v = nonstd::string_view(value_potential, split - value_potential);
				request_header.headers_.emplace(key_v, value_v);
				start = split + 2;
				split = my_search(start, end, key_lr, key_lr + 2);
			}
			return true;
		}
		//bool get_header(request_meta& request_header) {
		//	if (less_than(begin_, end_, 2) && (*begin_) == '\r' && (*(begin_ + 1)) == '\n') {  //没有键值对
		//		request_header.headers_.clear();
		//		return true;
		//	}
		//	auto view = nonstd::string_view(begin_, end_ - begin_ - 2);
		//	nonstd::string_view::size_type start = 0;
		//	auto split = view.find("\r\n");
		//	while (split != (nonstd::string_view::size_type)nonstd::string_view::npos) {
		//		auto kv_pair = view.substr(start, split - start);
		//		auto op_pos = kv_pair.find(':');
		//		auto value_pos = skip_value_white_space(kv_pair, op_pos + 1);
		//		auto value = kv_pair.substr(value_pos);
		//		request_header.headers_.emplace(kv_pair.substr(0, op_pos), value);
		//		start = split + 2;
		//		split = view.find("\r\n", start);
		//	}
		//	return true;
		//}
		bool parse_request_header(request_meta& request_header) {
			if (get_method(request_header)) {
				if (get_url(request_header)) {
					if (get_version(request_header)) {
						if (get_header(request_header)) {
							return true;
						}
					}
				}
			}
			return false;
		}
	private:
		char const* begin_;
		char const* end_;
	};

	//class http_parser_header final {
	//public:
	//	http_parser_header(std::vector<char>::iterator begin, std::vector<char>::iterator end) :begin_(begin), end_(end) {

	//	}
	//	std::pair<parse_state, bool> is_complete_header() {
	//		header_begin_ = begin_;
	//		auto view = nonstd::string_view(&(*begin_), end_ - begin_);
	//		auto pos = view.find("\r\n\r\n");
	//		auto npos = (nonstd::string_view::size_type)nonstd::string_view::npos;
	//		if (pos != npos) {
	//			auto invalid_char = view.rfind('\0', pos);
	//			if (invalid_char != npos) {
	//				return { parse_state::invalid,false };
	//			}
	//			header_end_ = begin_ + pos + 4;
	//			return { parse_state::valid,true };
	//		}
	//		return { parse_state::valid,false };
	//		//auto current = begin_;
	//		//header_begin_ = begin_;
	//		//while (current != end_) {
	//		//	if (*current == '\0') {  //不合法的请求
	//		//		return { parse_state::invalid,false };
	//		//	}
	//		//	if (less_than(current, end_,4) && *current == '\r' && *(current + 1) == '\n' && *(current + 2) == '\r' && *(current + 3) == '\n') {
	//		//		header_end_ = current + 4;
	//		//		return { parse_state::valid,true };
	//		//	}
	//		//	++current;
	//		//}
	//		//return { parse_state::valid,false };
	//	}
	//	std::pair < parse_state, std::string> get_white_split_value() {
	//		auto view = nonstd::string_view(&(*begin_), header_end_ - begin_);
	//		auto white_space_pos = view.find(' ');
	//		auto npos = (nonstd::string_view::size_type)nonstd::string_view::npos;
	//		if (white_space_pos != npos) {
	//			auto special_iter = begin_ + white_space_pos + 1;
	//			if (*special_iter != ' ') {
	//				begin_ = special_iter;
	//				return { parse_state::valid ,view2str(view.substr(0,white_space_pos)) };
	//			}
	//		}
	//		return { parse_state::invalid,"" };
	//	}
	//	//std::pair < parse_state, std::string> get_method() {
	//	//	auto start = begin_;
	//	//	while (begin_ != end_) {
	//	//		//auto c = *begin_;
	//	//		//auto c_next = *(begin_ + 1);
	//	//		if (less_than(begin_, end_, 2) && (*begin_) == ' ' && (*(begin_ + 1)) != ' ') {
	//	//			return { parse_state::valid,std::string(start, begin_++) };
	//	//		}
	//	//		++begin_;
	//	//	}
	//	//	return { parse_state::invalid,"" };
	//	//}
	//	//std::pair < parse_state, std::string> get_url() {
	//	//	auto start = begin_;
	//	//	while (begin_ != end_) {
	//	//		if (less_than(begin_, end_, 2) && (*begin_) == ' ' && (*(begin_ + 1)) != ' ') {
	//	//			return { parse_state::valid,std::string(start, begin_++) };
	//	//		}
	//	//		++begin_;
	//	//	}
	//	//	return { parse_state::invalid,"" };
	//	//}
	//	std::pair < parse_state, std::string> get_version() {
	//		auto start = begin_;
	//		while (begin_ != header_end_) {
	//			if (less_than(begin_, header_end_, 2) && (*begin_) == '\r' && (*(begin_ + 1)) == '\n') {
	//				begin_ += 2;
	//				return { parse_state::valid ,std::string(start,begin_ - 2) };
	//			}
	//			++begin_;
	//		}
	//		return { parse_state::invalid,"" };
	//	}
	//	nonstd::string_view::size_type skip_value_white_space(nonstd::string_view view,nonstd::string_view::size_type pos) {
	//		while (*(view.data() + pos) == ' ') {
	//			++pos;
	//		}
	//		return pos;
	//	}
	//	std::pair < parse_state, std::map<std::string, std::string>> get_header() {
	//		if (less_than(begin_, header_end_, 2) && (*begin_) == '\r' && (*(begin_ + 1)) == '\n') {  //没有键值对
	//			return { parse_state::valid ,{} };
	//		}
	//		auto view = nonstd::string_view(&(*begin_), header_end_ - begin_-2);
	//		auto header_vec = split(view, "\r\n");
	//		std::map<std::string, std::string> headers;
	//		for (auto& iter : header_vec) {
	//			auto key_pos = iter.find(':');
	//			auto key = iter.substr(0, key_pos);
	//			auto value_pos = skip_value_white_space(iter, key_pos + 1);
	//			auto value = iter.substr(value_pos);
	//			headers.emplace(to_lower(view2str(key)), view2str(value));
	//		}
	//		return { parse_state::valid ,headers };
	//	}
	//	//std::pair < parse_state, std::map<std::string, std::string>> get_header() {
	//	//	if (less_than(begin_, header_end_, 2) && (*begin_) == '\r' && (*(begin_ + 1)) == '\n') {  //没有键值对
	//	//		return { parse_state::valid ,{} };
	//	//	}
	//	//	std::map<std::string, std::string> headers;
	//	//	char r = ' ';
	//	//	char n = ' ';
	//	//	auto start = begin_;
	//	//	while (begin_ != end_) {
	//	//		if (less_than(begin_, end_, 2) && (*begin_) == '\r' && (*(begin_ + 1)) == '\n') {  //maybe a header = > key:value
	//	//			if ((end_ - begin_) >= 4) {
	//	//				r = *(begin_ + 2);
	//	//				n = *(begin_ + 3);
	//	//			}
	//	//			auto old_start = start;
	//	//			while (start != begin_) {
	//	//				if ((*start) == ':') {
	//	//					auto key = std::string(old_start, start);
	//	//					++start;
	//	//					while ((*start) == ' ') {
	//	//						++start;
	//	//					}
	//	//					headers.emplace(to_lower(std::move(key)), std::string(start, begin_));
	//	//					begin_ += 2;
	//	//					start = begin_;
	//	//					break;
	//	//				}
	//	//				++start;
	//	//			}
	//	//			if (start != begin_) {
	//	//				return { parse_state::invalid ,{} };
	//	//			}
	//	//			if (r == '\r' && n == '\n') {
	//	//				break;
	//	//			}
	//	//		}
	//	//		++begin_;
	//	//	}
	//	//	return { parse_state::valid ,headers };
	//	//}
	//	std::pair<bool, request_meta> parse_request_header() {
	//		auto method = get_white_split_value();
	//		if (method.first == parse_state::valid) {
	//			auto url = get_white_split_value();
	//			if (url.first == parse_state::valid) {
	//				auto version = get_version();
	//				if (version.first == parse_state::valid) {
	//					auto headers = get_header();
	//					if (headers.first == parse_state::valid) {
	//						return std::pair<bool, request_meta>(true, request_meta{ std::move(method.second),std::move(url.second),std::move(version.second),std::move(headers.second) });
	//					}
	//					else {
	//						return { false,request_meta() };
	//					}
	//				}
	//				else {
	//					return { false,request_meta() };
	//				}
	//			}
	//			else {
	//				return { false,request_meta() };
	//			}
	//		}
	//		else {
	//			return { false,request_meta() };
	//		}
	//	}
	//	std::size_t get_header_size() {
	//		return (header_end_ - header_begin_);
	//	}
	//private:
	//	std::vector<char>::iterator begin_;
	//	std::vector<char>::iterator end_;
	//	std::vector<char>::iterator header_end_;
	//	std::vector<char>::iterator header_begin_;
	//};

	class http_urlform_parser final {
	public:
		http_urlform_parser(std::string& body, bool need_url_decode = true) {
			if (need_url_decode) {
				body = xfinal::get_string_by_urldecode(body);
			}
			auto begin = body.data();
			begin_ = begin;
			end_ = begin + body.size();
		}

		http_urlform_parser(nonstd::string_view body) {
			auto begin = body.begin();
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
		void parse_key_value(std::map<nonstd::string_view, nonstd::string_view>& form, char const* old) {
			auto start = old;
			while (old != begin_) {
				if ((*old) == '=') {
					auto key = nonstd::string_view(start, old - start);
					if ((old + 1) != end_) {
						form.emplace(key, nonstd::string_view((old + 1), begin_ - 1 - old));
					}
					else {
						form.emplace(key, "");
					}
					break;
				}
				++old;
			}
		}
	private:
		char const* begin_;
		char const* end_;
	};

	class http_multipart_parser final {
	public:
		http_multipart_parser(std::string const& boundary_start_key_, std::string const& boundary_end_key_) :boundary_start_key_(boundary_start_key_), boundary_end_key_(boundary_end_key_) {

		}
	public:
		bool is_complete_part_header(std::vector<char>::iterator begin_, std::vector<char>::iterator end_) const {
			nonstd::string_view buffer{ &(*begin_) ,std::size_t(end_ - begin_) };
			auto it = buffer.find(boundary_start_key_);
			if (it != (nonstd::string_view::size_type)nonstd::string_view::npos) {
				auto it2 = buffer.find("\r\n\r\n");
				if (it2 != (nonstd::string_view::size_type)nonstd::string_view::npos) {
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
			if (it != (nonstd::string_view::size_type)nonstd::string_view::npos) {
				//std::cout << buffer.substr( it ,boundary_start_key_.size() ) << std::endl;
				return { multipart_data_state::is_end, it - 2 };
			}
			else {
				auto it2 = buffer.find(boundary_end_key_);
				if (it2 != (nonstd::string_view::size_type)nonstd::string_view::npos) {
					return { multipart_data_state::is_end ,it2 - 2 };
				}
				else {
					auto it3 = buffer.rfind("\r");
					if (it3 != (nonstd::string_view::size_type)nonstd::string_view::npos) {
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
			if (it != (nonstd::string_view::size_type)nonstd::string_view::npos) {
				auto parse_begin = it + boundary_start_key_.size() + 2;
				auto parse_end = buffer.find("\r\n\r\n", parse_begin);
				if (parse_end != (nonstd::string_view::size_type)nonstd::string_view::npos) {
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
