#pragma once
#include <array>
namespace xfinal {
	class nocopyable {
	public:
		nocopyable() = default;
	private:
		nocopyable(nocopyable const&) = delete;
		nocopyable& operator=(nocopyable const&) = delete;
	};

	std::string to_lower(std::string str) {
		std::for_each(str.begin(), str.end(), [](char& c) {
			c = std::tolower(c);
		});
		return str;
	}

	enum  class http_method
	{
		GET,
		POST,
		HEAD,
		PUT,
		OPTIONS,
		TRACE,
		DEL,
		MKCOL,
		MOVE
	};
	std::string http_method_to_name(http_method m) {
		switch (m) {
		case http_method::GET:
		{
			return "GET";
		}
		break;
		case http_method::POST:
		{
			return "POST";
		}
		break;
		case http_method::HEAD:
		{
			return "HEAD";
		}
		break;
		case http_method::PUT:
		{
			return "PUT";
		}
		break;
		case http_method::OPTIONS:
		{
			return "OPTIONS";
		}
		break;
		case http_method::TRACE:
		{
			return "TRACE";
		}
		break;
		case http_method::DEL:
		{
			return "DEL";
		}
		break;
		case http_method::MKCOL:
		{
			return "MKCOL";
		}
		break;
		case http_method::MOVE:
		{
			return "MOVE";
		}
		break;
		default:
			return "NULL";
			break;
		}
	}
	template<std::size_t Index, typename Array, typename Method>
	void each_http_method_args_help(Array&& arr, Method&& m){
		arr[Index] = http_method_to_name(m);
	}

	template<std::size_t Index, typename Array, typename Method, typename...Args>
	void each_http_method_args_help(Array&& arr, Method&& m, Args&&...args) {
		arr[Index] = http_method_to_name(m);
		each_http_method_args_help<Index+1>(std::forward<Array>(arr), std::forward<Args>(args)...);
	}

	template<http_method...Is>
	struct http_method_str {
		static auto methods_to_name()->std::array<std::string,sizeof...(Is)> {
			std::array<std::string, sizeof...(Is)> arr;
			each_http_method_args_help<0>(arr, Is...);
			return arr;
		}
	};

	template<>
	struct http_method_str<> {
		static auto methods_to_name() ->std::array<std::string, 0>{
			return std::array<std::string, 0>{};
		}
	};
}