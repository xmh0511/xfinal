#pragma once
#include <map>
#include <functional>
#include <tuple>
#include "string_view.hpp"
#include "http_handler.hpp"

namespace xfinal {
	class http_router {
		using router_function = std::function<void(request&, response&)>;
	public:
		template<typename Array,typename Function,typename...Args>
		void router(nonstd::string_view url, Array&& methods, Function&& lambda, Args&&...args) { //lambda
			auto tp = std::tuple<Args...>(std::forward<Args>(args)...);
			auto b = std::bind(&http_router::pre_handler<Function, std::tuple<Args...>>, this, std::placeholders::_1, std::placeholders::_2, std::forward<Function>(lambda), std::move(tp));
			for (auto& iter : methods) {
				std::string key = iter + std::string(url.data(), url.size());
				router_map_.insert(std::make_pair(key, static_cast<router_function>(b)));
			}
		}
		template<typename Function,typename Tuple>
		void pre_handler(request& req, response& res, Function& function, Tuple& tp) {  //lambda

		}
	public:
		void post_router(request& req, response& res) {
			auto key = std::string(req.get_method()) + std::string(req.get_url());
			if (router_map_.find(key) != router_map_.end()) {
				auto& it = router_map_.at(key);
				it(req, res);
			}
			else {
				std::cout << "not found" << std::endl;
			}
		}
	private:
		std::map<std::string, router_function> router_map_;
	};
}