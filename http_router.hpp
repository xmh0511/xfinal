#pragma once
#include <map>
#include <functional>
#include <tuple>
#include "string_view.hpp"
#include "http_handler.hpp"

namespace xfinal {
	template<std::size_t N>
	struct router_caller {
		template<typename Tuple>
		constexpr static void apply_before(bool& b, request& req,response& res,Tuple&& tp) {
			each_tuple<0, std::tuple_size_v<std::remove_reference_t<Tuple>>>{}(std::forward<Tuple>(tp), [&b,&req,&res](auto& aop) {
				auto r = aop.before(req, res);
				b = b && r;
			});
		}
	};
	template<>
	struct router_caller<0> {
		template<typename Tuple>
		constexpr static void apply_before(bool& b, request& req, response& res, Tuple&& tp) {
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
			pre_handler_expand(req, res, function, tp, std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
		}

		template<typename Function, typename Tuple,std::size_t...Indexs>
		void pre_handler_expand(request& req, response& res, Function& function, Tuple& tp, std::index_sequence<Indexs...>) {
			auto rtp = reorganize_tuple(std::tuple<>{}, std::tuple<>{}, std::get<Indexs>(tp)...);
			bool b = true;
			auto aop_tp = std::get<0>(rtp);
			router_caller<std::tuple_size_v<std::remove_reference_t<decltype(aop_tp)>>>::template apply_before(b, req, res, aop_tp);
			if (!b) {
				return;
			}
			function(req, res);
		}
	public:
		void post_router(request& req, response& res) {
			auto key = std::string(req.method()) + std::string(req.url());
			while (key.back() != '/') { //去除url 最后有/的干扰 /xxx/xxx/ - > /xxx/xxx
				key = key.substr(0, key.size() - 1);
			}
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