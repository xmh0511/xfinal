#pragma once
#include <map>
#include <functional>
#include <tuple>
#include "string_view.hpp"
#include "http_handler.hpp"
#include "session.hpp"
#include "ioservice_pool.hpp"
#include "websokcet.hpp"
#include "message_handler.hpp"
namespace xfinal {
	struct c11_auto_lambda_aop_before {
		c11_auto_lambda_aop_before(bool& b, request& req, response& res) :result(b), req_(req), res_(res) {

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
		template<typename Function, typename Tuple>
		constexpr static void apply(bool& b, request& req, response& res, Tuple&& tp) {
			each_tuple<0, tuple_size<typename std::remove_reference<Tuple>::type>::value>{}(std::forward<Tuple>(tp), Function{ b,req,res });
		}
	};
	template<>
	struct router_caller<0> {
		template<typename Function, typename Tuple>
		constexpr static void apply(bool& b, request& req, response& res, Tuple&& tp) {
			b = true;
		}
	};
	template<typename...Args>
	struct auto_params_lambda;

	template<typename Function>
	struct auto_params_lambda<Function> {
		auto_params_lambda(request& req0, response& res0, Function& function0) :func_(function0), req_(req0), res_(res0) {

		}
		template<typename T>
		void operator()(T&& t) {
			bool b = true;
			auto aop_tp = std::get<0>(t);
			router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_before>(b, req_, res_, aop_tp);
			if (!b) {
				return;
			}
			func_(req_, res_);
			router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_after>(b, req_, res_, aop_tp);
		}
		Function& func_;
		request& req_;
		response& res_;
	};

	template<typename Function, typename Object>
	struct auto_params_lambda<Function, Object> {
		auto_params_lambda(request& req0, response& res0, Object* that, Function& function0) :obj_(that), func_(function0), req_(req0), res_(res0) {

		}
		template<typename T>
		void operator()(T&& t) {
			bool b = true;
			auto aop_tp = std::get<0>(t);
			router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_before>(b, req_, res_, aop_tp);
			if (!b) {
				return;
			}
			if (obj_ != nullptr) {
				(obj_->*func_)(req_, res_);
			}
			else {
				(Object{}.*func_)(req_, res_);
			}
			router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_after>(b, req_, res_, aop_tp);
		}
		Object* obj_;
		Function func_;
		request& req_;
		response& res_;
	};

	template<typename Class, typename Object>
	struct auto_params_lambda<void(Class::*)(), Object> {
		auto_params_lambda(request& req0, response& res0, Object* that, void(Class::* function0)()) :obj_(that), func_(function0), req_(req0), res_(res0) {

		}
		template<typename T>
		void operator()(T&& t) {
			bool b = true;
			auto aop_tp = std::get<0>(t);
			router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_before>(b, req_, res_, aop_tp);
			if (!b) {
				return;
			}
			if (obj_ != nullptr) {
				obj_->req_ = &req_;
				obj_->res_ = &res_;
				(obj_->*func_)();
			}
			else {
				Class c;
				c.req_ = &req_;
				c.res_ = &res_;
				(c.*func_)();
			}
			router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_after>(b, req_, res_, aop_tp);
		}
		Object* obj_;
		void(Class::* func_)();
		request& req_;
		response& res_;
	};

	class http_router {
		friend class http_server;
	public:
		using router_function = std::function<void(request&, response&)>;
	public:
		http_router() :timer_(io_) {
			check_session_expires();
			thread_ = std::move(std::unique_ptr<std::thread>(new std::thread([](asio::io_service& io) {
				io.run();
				}, std::ref(io_))));
		}
		~http_router() {
			if (thread_->joinable()) {
				thread_->join();
			}
		}
	private:
		template<typename Array, typename Bind>
		void reg_router(nonstd::string_view url, Array&& methods, Bind&& router) {
			auto generator = url.find('*');
			if (generator == nonstd::string_view::npos) {
				for (auto& iter : methods) {
					std::string key = iter + std::string(url.data(), url.size());
					router_map_.insert(std::make_pair(key, static_cast<router_function>(router)));
				}
			}
			else {
				auto url_ = url.substr(0, generator - 1);
				for (auto& iter : methods) {
					std::string key = iter + std::string(url_.data(), url_.size());
					genera_router_map_.insert(std::make_pair(key, static_cast<router_function>(router)));
				}
			}
		}

		template<typename Array, typename Function, typename...Args>
		void router(nonstd::string_view url, Array&& methods, Function&& lambda, Args&& ...args) { //lambda or regular function
			auto tp = std::tuple<Args...>(std::forward<Args>(args)...);
			auto b = std::bind(&http_router::pre_handler<Function, std::tuple<Args...>>, this, std::placeholders::_1, std::placeholders::_2, std::forward<Function>(lambda), std::move(tp));
			reg_router(url, std::forward<Array>(methods), std::move(b));
		}

		template<typename Array, typename Ret, typename Class, typename...Params, typename Object, typename...Args>
		void router(nonstd::string_view url, Array&& methods, Ret(Class::* memberfunc)(Params...), Object& that, Args&& ...args) {  // member function
			auto tp = std::tuple<Args...>(std::forward<Args>(args)...);
			auto b = std::bind(&http_router::pre_handler_member_function<Ret(Class::*)(Params...), Class, std::tuple<Args...>>, this, std::placeholders::_1, std::placeholders::_2, memberfunc, static_cast<Class*>(&that), std::move(tp));
			reg_router(url, std::forward<Array>(methods), std::move(b));
		}

		template<typename Array, typename Ret, typename Class, typename...Params, typename Object, typename...Args>
		typename std::enable_if<std::is_pointer<typename std::remove_reference<Object>::type>::value || std::is_null_pointer<typename std::remove_reference<Object>::type>::value>::type router(nonstd::string_view url, Array && methods, Ret(Class:: * memberfunc)(Params...), Object && that, Args && ...args) {  // member function
			auto tp = std::tuple<Args...>(std::forward<Args>(args)...);
			auto b = std::bind(&http_router::pre_handler_member_function<Ret(Class::*)(Params...), Class, std::tuple<Args...>>, this, std::placeholders::_1, std::placeholders::_2, memberfunc, static_cast<Class*>(that), std::move(tp));
			reg_router(url, std::forward<Array>(methods), std::move(b));
		}

		template<typename Array, typename Class, typename...Args>
		void router(nonstd::string_view url, Array&& methods, void(Class::* memberfunc)(), Class& that, Args&& ...args) {  //控制器
			auto tp = std::tuple<Args...>(std::forward<Args>(args)...);
			auto b = std::bind(&http_router::pre_handler_controller<Class, std::tuple<Args...>>, this, std::placeholders::_1, std::placeholders::_2, memberfunc, &that, std::move(tp));
			reg_router(url, std::forward<Array>(methods), std::move(b));
		}

		template<typename Array, typename Class, typename...Args>
		void router(nonstd::string_view url, Array&& methods, void(Class::* memberfunc)(), Class* that, Args&& ...args) {  //控制器
			auto tp = std::tuple<Args...>(std::forward<Args>(args)...);
			auto b = std::bind(&http_router::pre_handler_controller<Class, std::tuple<Args...>>, this, std::placeholders::_1, std::placeholders::_2, memberfunc, that, std::move(tp));
			reg_router(url, std::forward<Array>(methods), std::move(b));
		}

		///lambda or regular function  process --start
		template<typename Function, typename Tuple>
		void pre_handler(request& req, response& res, Function& function, Tuple& tp) {  //lambda or regular function
			pre_handler_expand(req, res, function, tp, typename make_index_sequence<std::tuple_size<typename std::remove_reference<Tuple>::type>::value>::type{});
		}

		template<typename Function, typename Tuple, std::size_t...Indexs>
		void pre_handler_expand(request& req, response& res, Function& function, Tuple& tp, index_sequence<Indexs...>) {
			auto_params_lambda<Function> lambda{ req ,res ,function };
			reorganize_tuple(lambda, std::tuple<>{}, std::tuple<>{}, std::get<Indexs>(tp)...);
		}
		///lambda or regular function  process --end

		///class member function process   --start
		template<typename Function, typename Class, typename Tuple>
		void pre_handler_member_function(request& req, response& res, Function& function, Class* that, Tuple& tp) {
			pre_handler_member_function_expand(req, res, function, that, tp, typename make_index_sequence<std::tuple_size<typename std::remove_reference<Tuple>::type>::value>::type{});
		}

		template<typename Function, typename Class, typename Tuple, std::size_t...Indexs>
		void pre_handler_member_function_expand(request& req, response& res, Function& function, Class* that, Tuple& tp, index_sequence<Indexs...>) {
			auto_params_lambda<Function, Class> lambda{ req ,res ,that ,function };
			reorganize_tuple(lambda, std::tuple<>{}, std::tuple<>{}, std::get<Indexs>(tp)...);
		}

		///class member function process   --end;

		///controller process --start
		template<typename Class, typename Tuple>
		void pre_handler_controller(request& req, response& res, void(Class::* memberfunc)(), Class* that, Tuple& tp) {
			handler_controller_expand(req, res, memberfunc, that, tp, typename make_index_sequence<std::tuple_size<typename std::remove_reference<Tuple>::type>::value>::type{});
		}

		template<typename Class, typename Tuple, std::size_t...Indexs>
		void handler_controller_expand(request& req, response& res, void(Class::* memberfunc)(), Class* that, Tuple& tp, index_sequence<Indexs...>) {
			auto_params_lambda<void(Class::*)(), Class> lambda{ req ,res ,that ,memberfunc };
			reorganize_tuple(lambda, std::tuple<>{}, std::tuple<>{}, std::get<Indexs>(tp)...);
		}

		///controller process --end

	public:
		void post_router(request& req, response& res) {
			auto url = req.url();
			if (url.size() > 1) {  //确保这里不是 / 根路由
				std::size_t back = 0;
				while ((url.size() - back - 1) > 0 && url[url.size() - back - 1] == '/') { //去除url 最后有/的干扰 /xxx/xxx/ - > /xxx/xxx
					back++;
				}
				if (back > 0) {  //如果url不是规范的url 则重定向跳转
					if (url_redirect_) {
						auto url_str = view2str(url.substr(0, url.size() - back));
						auto params = req.raw_params();
						if (!params.empty()) {
							url_str += "?";
						}
						url_str += view2str(params);
						res.redirect(url_str);
						return;
					}
					else {
						url = url.substr(0, url.size() - back);
					}
				}
			}
			//auto cookies = cookies_map(req.header("Cookie"));
			//for (auto& iter : cookies) {
			//	session_manager::get().validata(view2str(iter.second));
			//}
			auto key = std::string(req.method()) + std::string(url);
			if (router_map_.find(key) != router_map_.end()) {
				auto& it = router_map_.at(key);
				set_view_method(res);
				it(req, res);
				return;
			}
			else {
				for (auto& iter : genera_router_map_) {
					if (iter.first == (key.substr(0, iter.first.size()))) {
						set_view_method(res);
						auto& url = iter.first;
						auto pos = url.find("/");
						req.set_generic_path(url.substr(pos, url.size() - pos));
						(iter.second)(req, res);
						return;
					}
				}
			}
			if (not_found_callback_ != nullptr) {
				not_found_callback_(req, res);
			}
			else {
				res.write_string(std::string("the url \"") + view2str(req.url()) + "\" is not found", false, http_status::bad_request);
			}
		}
	public:
		void trigger_error(std::exception const& err) {
			utils::messageCenter::get().trigger_message(err.what());
		}
	private:
		void set_view_method(response& res) {
			if (!view_method_map_.empty()) {
				for (auto& iter : view_method_map_) {
					res.view_env_->add_callback(iter.first, (unsigned int)iter.second.first, iter.second.second);
				}
			}
		}
	public:
		void check_session_expires() {
			timer_.expires_from_now(std::chrono::seconds(check_session_time_));
			timer_.async_wait([this](std::error_code const& ec) {
				session_manager::get().check_expires();
				check_session_expires();
				});
		}
		void set_check_session_rate(std::time_t seconds) {
			check_session_time_ = seconds;
		}

		std::time_t check_session_rate() {
			return check_session_time_;
		}
	public:
		class websockets& websokcets() {
			return websockets_;
		}
	private:
		std::map<std::string, router_function> router_map_;
		std::map<std::string, router_function> genera_router_map_;
		std::map<std::string, std::pair<std::size_t, inja::CallbackFunction>> view_method_map_;
		asio::io_service io_;
		asio::steady_timer timer_;
		std::unique_ptr<std::thread> thread_;
		std::time_t check_session_time_ = 10;
		class websockets websockets_;
		bool url_redirect_ = true;
		std::function<void(request&, response&)> not_found_callback_ = nullptr;
	};
}