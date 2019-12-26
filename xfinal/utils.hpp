#pragma once
#include <array>
#include <tuple>
#include "string_view.hpp"
#include <vector>
#include <filesystem.hpp>
#include <sstream>
#include "json.hpp"
#include <ctime>
#include <unordered_map>
#include "code_parser.hpp"
#include "sha1.hpp"
#include <string>
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

	inline std::string to_lower(std::string str) {
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
		TRACE_,  //与windows宏重名
		DEL,
		MKCOL,
		MOVE
	};
	inline std::string http_method_to_name(http_method m) {
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
		case http_method::TRACE_:
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

	inline std::string get_root_director(fs::path path) {
		auto c = path.parent_path();
		while (c.stem().string() != ".") {
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

	class xfinal_exception final :public std::exception {
	public:
		xfinal_exception(std::string&& msg) noexcept :msg_(std::move(msg)) {

		}
		xfinal_exception(std::string const& msg) noexcept:msg_(msg) {

		}
		xfinal_exception(std::exception const& ec) noexcept:msg_(ec.what()) {

		}
	public:
		char const* what() const noexcept{
			return msg_.c_str();
		}
	private:
		std::string msg_;
	};

	inline std::vector<nonstd::string_view> split(nonstd::string_view s, nonstd::string_view delimiter) {
		std::vector<nonstd::string_view> output;
		std::size_t b = 0;
		auto it = s.find(delimiter);
		while (it != (nonstd::string_view::size_type)nonstd::string_view::npos) {
			output.push_back(nonstd::string_view{ s.data()+b,it - b });
			b = it + delimiter.size();
			it = s.find(delimiter, b);
		}
		if (b != s.size()) {
			output.push_back(nonstd::string_view{ s.data() + b ,s.size()-b });
		}
		return output;
	}

	inline std::string get_gmt_time_str(std::time_t t)
	{
		struct tm* GMTime = gmtime(&t);
		char buff[512] = { 0 };
		strftime(buff, sizeof(buff), "%a, %d %b %Y %H:%M:%S GMT", GMTime);
		return buff;
	}

	inline std::unordered_map<nonstd::string_view, nonstd::string_view> cookies_map(nonstd::string_view cookies) {
		auto v = split(cookies, "; ");
		std::unordered_map<nonstd::string_view, nonstd::string_view> out;
		for (auto iter : v) {
			auto v2 = split(iter, "=");
			out.insert(std::make_pair(v2[0], v2[1]));
		}
		return out;
	}

	inline std::string to_sha1(std::string const& src) {
		sha1_context ctx;
		init(ctx);
		update(ctx, reinterpret_cast<unsigned char const*>(&src[0]), src.size());
		std::string buff(20, '\0');
		finish(ctx, &buff[0]);
		return buff;
	}

	inline size_t base64_encode(char *_dst, const void *_src, size_t len, int url_encoded) {
		char *dst = _dst;
		const uint8_t *src = reinterpret_cast<const uint8_t *>(_src);
		static const char *MAP_URL_ENCODED = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz"
			"0123456789-_";
		static const char *MAP = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz"
			"0123456789+/";
		const char *map = url_encoded ? MAP_URL_ENCODED : MAP;
		uint32_t quad;

		for (; len >= 3; src += 3, len -= 3) {
			quad = ((uint32_t)src[0] << 16) | ((uint32_t)src[1] << 8) | src[2];
			*dst++ = map[quad >> 18];
			*dst++ = map[(quad >> 12) & 63];
			*dst++ = map[(quad >> 6) & 63];
			*dst++ = map[quad & 63];
		}
		if (len != 0) {
			quad = (uint32_t)src[0] << 16;
			*dst++ = map[quad >> 18];
			if (len == 2) {
				quad |= (uint32_t)src[1] << 8;
				*dst++ = map[(quad >> 12) & 63];
				*dst++ = map[(quad >> 6) & 63];
				if (!url_encoded)
					*dst++ = '=';
			}
			else {
				*dst++ = map[(quad >> 12) & 63];
				if (!url_encoded) {
					*dst++ = '=';
					*dst++ = '=';
				}
			}
		}

		*dst = '\0';
		return dst - _dst;
	}

	inline std::string to_base64(std::string const& src) {
		char buff[1024];
		auto size = base64_encode(buff,src.data(), src.size(), 0);
		return std::string(buff, size);
	}

	inline bool is_bigendian(){
	  int a = 1;
	  unsigned char& c = (unsigned char&)a;
	  if (c == '\0') {
		  return true;
	  }
	  return false;
	}


	template<typename T>
	void netendian_to_l(T& t, unsigned char const* ptr) {
		auto size = sizeof(T);
		//unsigned char* iter = (unsigned char*)&t;
		//if (is_bigendian()) {  //大端
		//	for (int i = 0; i < size; ++i) {
		//		iter[i] = *ptr;
		//		++ptr;
		//	}
		//}
		//else {  //小端
		//	for (int i = size - 1; i >= 0; --i) {
		//		iter[i] = *ptr;
		//		++ptr;
		//	}
		//}
		t = 0;
		for (int i = 0; i < (int)size; ++i) {
			T p = (T)ptr[i];
			p = p << (size - i - 1)*8;
			t = t |  p;
		}
	}

	template<typename T>
	std::string l_to_netendian(T t) {
		auto size = sizeof(T);
		std::string result;
		for (std::size_t i = 0; i < size; i++) {
			unsigned char c = (t >> ((size-i-1)*8)) & 255;
			result.push_back(c);
		}
		return result;
	}

	template<std::size_t N>
	class number_content{};
}