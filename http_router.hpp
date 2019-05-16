#pragma once
#include <map>
#include <functional>
#include <tuple>
#include "string_view.hpp"
#include "http_handler.hpp"

namespace xfinal {
	struct c11_auto_lambda_aop_before{
		c11_auto_lambda_aop_before(bool& b, request& req, response& res):result(b),req_(req),res_(res){

		}
		template<typename T>
		bool operator()(T&& aop) {
			result = result && (aop.before(req_, res_));
			return result;
		}
		bool& result;
		request& req_;
		response& res_;
	};

	struct c11_auto_lambda_aop_after {
		c11_auto_lambda_aop_after(bool& b, request& req, response& res) :result(b), req_(req), res_(res) {

		}
		template<typename T>
		bool operator()(T&& aop) {
			result = result && (aop.after(req_, res_));
			return result;
		}
		bool& result;
		request& req_;
		response& res_;
	};
	//template<std::size_t N>
	//struct router_caller {
	//	template<typename Tuple>
	//	constexpr static void apply_before(bool& b, request& req,response& res,Tuple&& tp) {
	//		each_tuple<0, tuple_size<typename std::remove_reference<Tuple>::type>::value>{}(std::forward<Tuple>(tp), [&b,&req,&res](auto& aop) {
	//			auto r = aop.before(req, res);
	//			b = b && r;
	//		});
	//	}
	//};
	template<std::size_t N>
	struct router_caller {
		template<typename Function,typename Tuple>
		constexpr static void apply(bool& b, request& req, response& res, Tuple&& tp) {
			each_tuple<0, tuple_size<typename std::remove_reference<Tuple>::type>::value>{}(std::forward<Tuple>(tp), Function{b,req,res});
		}
	};
	template<>
	struct router_caller<0> {
		template<typename Function, typename Tuple>
		constexpr static void apply(bool& b, request& req, response& res, Tuple&& tp) {
			b = true;
		}
	};
	class http_router {
		using router_function = std::function<void(request&, response&)>;
	public:
		template<typename Array,typename Bind>
		void reg_router(nonstd::string_view url, Array&& methods, Bind&& router) {
			auto generator = url.find('*');
			if (generator == nonstd::string_view::npos) {
				for (auto& iter : methods) {
					std::string key = iter + std::string(url.data(), url.size());
					router_map_.insert(std::make_pair(key, static_cast<router_function>(router)));
				}
			}
			else {
				auto url_ = url.substr(0, generator-1);
				for (auto& iter : methods) {
					std::string key = iter + std::string(url_.data(), url_.size());
					genera_router_map_.insert(std::make_pair(key, static_cast<router_function>(router)));
				}
			}
		}

		template<typename Array,typename Function,typename...Args>
		void router(nonstd::string_view url, Array&& methods, Function&& lambda, Args&&...args) { //lambda
			auto tp = std::tuple<Args...>(std::forward<Args>(args)...);
			auto b = std::bind(&http_router::pre_handler<Function, std::tuple<Args...>>, this, std::placeholders::_1, std::placeholders::_2, std::forward<Function>(lambda), std::move(tp));
			reg_router(url, std::forward<Array>(methods), std::move(b));
		}
		template<typename Function,typename Tuple>
		void pre_handler(request& req, response& res, Function& function, Tuple& tp) {  //lambda
			pre_handler_expand(req, res, function, tp, typename make_index_sequence<std::tuple_size<typename std::remove_reference<Tuple>::type>::value>::type{});
		}

		template<typename Function, typename Tuple,std::size_t...Indexs>
		void pre_handler_expand(request& req, response& res, Function& function, Tuple& tp, index_sequence<Indexs...>) {
			auto rtp = reorganize_tuple(std::tuple<>{}, std::tuple<>{}, std::get<Indexs>(tp)...);
			bool b = true;
			auto aop_tp = std::get<0>(rtp);
			router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_before>(b, req, res, aop_tp);
			if (!b) {
				return;
			}
			function(req, res);
			router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_after>(b, req, res, aop_tp);
		}
	public:
		void post_router(request& req, response& res) {
			auto url = req.url();
			if (url.size() > 1) {  //确保这里不是 / 根路由
				std::size_t back = 0;
				while ((url.size() - back - 1)>0 && url[url.size() - back - 1] == '/') { //去除url 最后有/的干扰 /xxx/xxx/ - > /xxx/xxx
					back++;
				}
				if (back > 0) {  //如果url不是规范的url 则重定向跳转
					auto url_str = view2str(url.substr(0, url.size() - back));
					auto params = req.params();
					if (!params.empty()) {
						url_str += "?";
					}
					url_str += view2str(params);
					res.redirect(url_str);
					return;
				}
			}
			auto key = std::string(req.method()) + std::string(url);
			if (router_map_.find(key) != router_map_.end()) {
				auto& it = router_map_.at(key);
				it(req, res);
			}
			else {
				for (auto& iter : genera_router_map_) {
					if (iter.first == (key.substr(0, iter.first.size()))) {
						(iter.second)(req, res);
						return;
					}
				}
				res.write_string(std::string("the url \"") + view2str(req.url()) + "\" is not found", false,http_status::bad_request);
			}
		}
	private:
		std::map<std::string, router_function> router_map_;
		std::map<std::string, router_function> genera_router_map_;
	};
}