#pragma once
#include <array>
#include <tuple>
#include "string_view.hpp"
#include <vector>
namespace xfinal {

	struct GBK {};
	struct UTF8{};
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

	template<typename T,typename U = void>
	struct is_aop {
		static constexpr bool value = false;
	};

	template<typename T>
	struct is_aop<T, std::void_t<std::tuple<decltype(&T::before),decltype(&T::after)>>> {
		static constexpr bool value = true;
	};

	template<typename AopTuple, typename NoAopTuple,typename T,typename U = std::enable_if_t<is_aop<std::remove_reference_t<T>>::value>>
	auto process_reorganize(int,AopTuple&& atp, NoAopTuple&& natp,T&& t) {
		auto tp =  std::tuple_cat(atp, std::tuple<T>(t));
		return std::make_tuple(tp, natp);
	}

	template<typename AopTuple, typename NoAopTuple, typename T, typename U = std::enable_if_t<!is_aop<std::remove_reference_t<T>>::value>>
	auto process_reorganize(float,AopTuple&& atp, NoAopTuple&& natp, T&& t) {
		auto tp = std::tuple_cat(natp, std::tuple<T>(t));
		return std::make_tuple(atp, tp);
	}

	template<typename AopTuple, typename NoAopTuple>
	auto reorganize_tuple(AopTuple&& atp, NoAopTuple&& natp) {
		return std::make_tuple(atp, natp);
	}

	template<typename AopTuple,typename NoAopTuple,typename T,typename...Args>
	auto reorganize_tuple(AopTuple&& atp, NoAopTuple&& natp,T&& t,Args&&...args) {
		auto tp = process_reorganize(0, atp, natp, t);
		return reorganize_tuple(std::get<0>(tp), std::get<1>(tp), std::forward<Args>(args)...);
	}

	template<std::size_t N,std::size_t Max>
	struct each_tuple {
		template<typename Tuple, typename Function>
		void operator()(Tuple&& tp, Function&& function) {
			function(std::get<N>(tp));
			each_tuple<N + 1, Max>{}(std::forward<Tuple>(tp), std::forward<Function>(function));
		}
	};
	template<std::size_t Max>
	struct each_tuple<Max, Max> {
		template<typename Tuple, typename Function>
		void operator()(Tuple&& tp, Function&& function) {

		}
	};

	std::string view2str(nonstd::string_view view) {
		return std::string(view.data(), view.size());
	}

	template<typename T,typename U = std::void_t<decltype(std::declval<T>().begin())>>
	void forward_contain_data(T& contain,std::size_t data_begin,std::size_t data_end) {
		auto old_begin = data_begin;
		auto begin = contain.begin();
		while (old_begin < data_end) {
			*begin = contain[old_begin];
			++old_begin;
			++begin;
		}
		auto start = data_end - data_begin;
		if (contain.capacity() > start) {
			while (start < contain.capacity()) {
				contain[start++] = '\0';
			}
		}
	}
}