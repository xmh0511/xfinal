//
// Created by xmh on 18-3-16.
//

#ifndef CPPWEBSERVER_URL_ENCODE_DECODE_HPP
#define CPPWEBSERVER_URL_ENCODE_DECODE_HPP
#include "string_view.hpp"
#include <string>
#include <clocale>
#include <cstdlib>
namespace xfinal {
	const char* hex_chars = "0123456789ABCDEF";
	static std::string url_encode(const std::string& value) noexcept {
		std::string result;
		result.reserve(value.size()); // Minimum size of result

		for (auto& chr : value) {
			if (!((chr >= '0' && chr <= '9') || (chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z') || chr == '-' || chr == '.' || chr == '_' || chr == '~'))
				result += std::string("%") + hex_chars[static_cast<unsigned char>(chr) >> 4] + hex_chars[static_cast<unsigned char>(chr) & 15];
			else
				result += chr;
		}

		return result;
	}

	static std::string url_decode(const std::string& value) noexcept {
		std::string result;
		result.reserve(value.size() / 3 + (value.size() % 3)); // Minimum size of result

		for (std::size_t i = 0; i < value.size(); ++i) {
			auto& chr = value[i];
			if (chr == '%' && i + 2 < value.size()) {
				auto hex = value.substr(i + 1, 2);
				auto decoded_chr = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
				result += decoded_chr;
				i += 2;
			}
			else if (chr == '+')
				result += ' ';
			else
				result += chr;
		}

		return result;
	}

	static bool is_url_encode(nonstd::string_view str) {
		return str.find("%") != (nonstd::string_view::size_type)nonstd::string_view::npos || str.find("+") != (nonstd::string_view::size_type)nonstd::string_view::npos;
	}

	static std::string get_string_by_urldecode(nonstd::string_view content){
		return url_decode(std::string(content.data(), content.size()));
	}


	static std::string utf8_to_gbk(std::string const& u8str) {
		std::setlocale(LC_ALL, "en_US.utf8");
		auto size = std::mbstowcs(nullptr, u8str.c_str(), 0);
		std::wstring wstr(size, '\0');
		std::mbstowcs(&wstr[0], u8str.c_str(), size);
#if _MSC_VER
		std::setlocale(LC_ALL, ".936");
#else
		std::setlocale(LC_ALL, "zh_CN.GBK");
#endif 
		auto msize = std::wcstombs(nullptr, wstr.c_str(), 0);
		std::string str(msize, '\0');
		std::wcstombs(&str[0], wstr.c_str(), msize);
		return str;
	}

	static std::string gbk_to_utf8(std::string const& gbkstr) {
#if _MSC_VER
		std::setlocale(LC_ALL, ".936");
#else
		std::setlocale(LC_ALL, "zh_CN.GBK");
#endif 
		auto size = std::mbstowcs(nullptr, gbkstr.c_str(), 0);
		std::wstring wstr(size, '\0');
		std::mbstowcs(&wstr[0], gbkstr.c_str(), size);
		std::setlocale(LC_ALL, "en_US.utf8");
		auto msize = std::wcstombs(nullptr, wstr.c_str(), 0);
		std::string str(msize, '\0');
		std::wcstombs(&str[0], wstr.c_str(), msize);
		return str;
	}
}
#endif //CPPWEBSERVER_URL_ENCODE_DECODE_HPP