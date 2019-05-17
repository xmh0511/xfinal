#pragma once
#include <array>
#include <tuple>
#include "string_view.hpp"
#include <vector>
#include <filesystem.hpp>
#include <sstream>
#include "json.hpp"
namespace fs = ghc::filesystem;
namespace xfinal {

	using namespace nlohmann;

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

	enum  class http_method:std::uint8_t
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
	///cpp14 以上的工具类
	template<typename...Args>
	struct void_t {
		using type = void;
	};

	template<typename T>
	struct tuple_size {

	};

	template<typename...Args>
	struct tuple_size<std::tuple<Args...>> {
		static constexpr std::size_t value = sizeof...(Args);
	};

	template<std::size_t...N>
	struct index_sequence {

	};

	template<std::size_t N,std::size_t...Indexs>
	struct index_sequence<N, Indexs...> {
		using type = typename index_sequence<N - 1, N - 1, Indexs...>::type;
	};

	template<std::size_t...Indexs>
	struct index_sequence<0, Indexs...> {
		using type = index_sequence<Indexs...>;
	};
	

	template<std::size_t N>
	struct make_index_sequence {
		using type = typename index_sequence<N>::type;
	};

	///cpp14 以上的工具类



	template<typename T,typename U = void>
	struct is_aop {
		static constexpr bool value = false;
	};

	template<typename T>
	struct is_aop<T, typename void_t<std::tuple<decltype(&T::before),decltype(&T::after)>>::type> {
		static constexpr bool value = true;
	};

	template<typename AopTuple, typename NoAopTuple,typename T,typename U = typename std::enable_if<is_aop<typename std::remove_reference<T>::type>::value>::type>
	auto process_reorganize(int,AopTuple&& atp, NoAopTuple&& natp,T&& t) //->decltype(std::make_tuple(std::tuple_cat(atp, std::tuple<T>(t)), natp)) 
	{
		auto tp =  std::tuple_cat(atp, std::tuple<T>(t));
		return std::make_tuple(tp, natp);
	}

	template<typename AopTuple, typename NoAopTuple, typename T, typename U = typename std::enable_if<!is_aop<typename std::remove_reference<T>::type>::value>::type>
	auto process_reorganize(float,AopTuple&& atp, NoAopTuple&& natp, T&& t) //->decltype(std::make_tuple(atp, std::tuple_cat(natp, std::tuple<T>(t)))) 
	{
		auto tp = std::tuple_cat(natp, std::tuple<T>(t));
		return std::make_tuple(atp, tp);
	}

	template<typename AopTuple, typename NoAopTuple>
	auto reorganize_tuple(AopTuple&& atp, NoAopTuple&& natp) //->decltype(std::make_tuple(atp, natp)) 
	{
		return std::make_tuple(atp, natp);
	}

	template<typename AopTuple,typename NoAopTuple,typename T,typename...Args>
	auto reorganize_tuple(AopTuple&& atp, NoAopTuple&& natp,T&& t,Args&&...args) // ->decltype(reorganize_tuple(std::get<0>(process_reorganize(0, atp, natp, t)), std::get<1>(process_reorganize(0, atp, natp, t)), std::forward<Args>(args)...)) 
	{
		auto tp = process_reorganize(0, atp, natp, t);
		return reorganize_tuple(std::get<0>(tp), std::get<1>(tp), std::forward<Args>(args)...);
	}

	template<std::size_t N,std::size_t Max>
	struct each_tuple {
		template<typename Tuple, typename Function>
		void operator()(Tuple&& tp, Function&& function) {
			bool b = function(std::get<N>(tp));
			if (!b) {
				return;
			}
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

	template<typename T,typename U = typename void_t<decltype(std::declval<T>().begin())>::type>
	void forward_contain_data(T& contain,std::size_t data_begin,std::size_t data_end) {
		auto old_begin = data_begin;
		auto begin = contain.begin();
		while (old_begin < data_end) {
			*begin = contain[old_begin];
			++old_begin;
			++begin;
		}
		//auto start = data_end - data_begin;
		//if (contain.size() > start) {
		//	while (start < contain.size()) {
		//		contain[start++] = '\0';
		//	}
		//}
	}

	std::string get_root_director(fs::path path) {
		auto c = path.parent_path();
		while (c.stem() != ".") {
			path = c;
			c = c.parent_path();
		}
		return path.stem();
	}

	template<typename T>
	std::string to_hex(T number) {
		std::stringstream ss;
		ss << std::hex << number;
		return ss.str();
	}
}