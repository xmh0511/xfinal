#pragma once
#include <map>
#include <functional>
#include <tuple>
#include "string_view.hpp"
#include "http_handler.hpp"
#include "session.hpp"
#include "ioservice_pool.hpp"
#include "websocket.hpp"
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

	struct c11_auto_lambda_interceptor_pre {
		c11_auto_lambda_interceptor_pre(bool& b, request& req, response& res) :result(b), req_(req), res_(res) {

		}
		template<typename T>
		bool operator()(T&& interceptor) {
			result = result && (interceptor.prehandle(req_, res_));
			return result;
		}
		bool& result;
		request& req_;
		response& res_;
	};

	template<std::size_t N>
	struct router_caller {
		template<typename Function, typename Tuple>
		static void apply(bool& b, request& req, response& res, Tuple&& tp) {
			each_tuple<0, tuple_size<typename std::remove_reference<Tuple>::type>::value>{}(std::forward<Tuple>(tp), Function{ b,req,res });
		}
	};
	template<>
	struct router_caller<0> {
		template<typename Function, typename Tuple>
		static void apply(bool& b, request& req, response& res, Tuple&& tp) {
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
			auto&& aop_tp = std::get<0>(t);
			router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_before>(b, req_, res_, aop_tp);
			if (!b) {
				return;
			}
			using aop_type = typename std::remove_reference<decltype(aop_tp)>::type;
			auto excutor = std::bind(&router_caller<std::tuple_size<aop_type>::value>::template apply<c11_auto_lambda_aop_after, aop_type&>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::ref(aop_tp));
			res_.after_excutor_ = excutor;
			func_(req_, res_);
			//router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_after>(b, req_, res_, aop_tp);
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
			auto&& aop_tp = std::get<0>(t);
			router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_before>(b, req_, res_, aop_tp);
			if (!b) {
				return;
			}
			using aop_type = typename std::remove_reference<decltype(aop_tp)>::type;
			auto excutor = std::bind(&router_caller<std::tuple_size<aop_type>::value>::template apply<c11_auto_lambda_aop_after, aop_type&>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::ref(aop_tp));
			res_.after_excutor_ = excutor;
			if (obj_ != nullptr) {
				(obj_->*func_)(req_, res_);
			}
			else {
				(Object{}.*func_)(req_, res_);
			}
			//router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_after>(b, req_, res_, aop_tp);
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
			auto&& aop_tp = std::get<0>(t);
			router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_before>(b, req_, res_, aop_tp);
			if (!b) {
				return;
			}
			using aop_type = typename std::remove_reference<decltype(aop_tp)>::type;
			auto excutor = std::bind(&router_caller<std::tuple_size<aop_type>::value>::template apply<c11_auto_lambda_aop_after, aop_type&>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::ref(aop_tp));
			res_.after_excutor_ = excutor;
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
			//router_caller<std::tuple_size<typename std::remove_reference<decltype(aop_tp)>::type>::value>::template apply<c11_auto_lambda_aop_after>(b, req_, res_, aop_tp);
		}
		Object* obj_;
		void(Class::* func_)();
		request& req_;
		response& res_;
	};

	template<typename Methods, typename Function, typename MemberClass>
	struct fiction_auto_lambda_for_ws {
		fiction_auto_lambda_for_ws(nonstd::string_view url, Methods methods, Function function, MemberClass* that, websocket_event const& Event):url_(url), methods_(methods), function_(function), that_(that), event_(Event){

		}
		template<typename T>
		void operator()(T&& tp) {
			auto b = std::bind(&MemberClass::template pre_handler<Function, typename std::remove_reference<T>::type>, that_, std::placeholders::_1, std::placeholders::_2, function_, tp);
			auto str = that_->reg_router(url_, methods_, std::move(b), std::get<1>(tp));
			that_->websockets_.add_event(methods_[0]+ str, event_);
		}
		nonstd::string_view url_;
		Methods methods_;
		Function function_;
		MemberClass* that_;
		websocket_event const& event_;
	};

	template<typename Methods,typename Function,typename MemberClass>
	struct fiction_auto_lambda_in_router0 {
		fiction_auto_lambda_in_router0(nonstd::string_view url,Methods methods, Function function, MemberClass* that):url_(url),methods_(methods), function_(function), that_(that){

		}
		template<typename T>
		void operator()(T&& tp) {
			auto b = std::bind(&MemberClass::template pre_handler<Function,typename std::remove_reference<T>::type>, that_, std::placeholders::_1, std::placeholders::_2, function_, tp);
			that_->reg_router(url_, methods_, std::move(b), std::get<1>(tp));
		}
		nonstd::string_view url_;
		Methods methods_;
		Function function_;
		MemberClass* that_;
	};

	template<typename Methods, typename Function,typename ClassType, typename MemberClass>
	struct fiction_auto_lambda_in_router1 {
		fiction_auto_lambda_in_router1(nonstd::string_view url, Methods methods, Function function, ClassType* class_ptr,MemberClass* that) :url_(url), methods_(methods), function_(function), class_(class_ptr), that_(that){

		}
		template<typename T>
		void operator()(T&& tp) {
			auto b = std::bind(&MemberClass::template pre_handler_member_function<Function, ClassType, typename std::remove_reference<T>::type>, that_, std::placeholders::_1, std::placeholders::_2, function_, class_, tp);
			that_->reg_router(url_, methods_, std::move(b), std::get<1>(tp));
		}
		nonstd::string_view url_;
		Methods methods_;
		Function function_;
		ClassType* class_;
		MemberClass* that_;
	};

	template<typename Methods, typename Function, typename ClassType, typename MemberClass>
	struct fiction_auto_lambda_in_router2 {
		fiction_auto_lambda_in_router2(nonstd::string_view url, Methods methods, Function function, ClassType* class_ptr, MemberClass* that) :url_(url), methods_(methods), function_(function), class_(class_ptr), that_(that) {

		}
		template<typename T>
		void operator()(T&& tp) {
			auto b = std::bind(&MemberClass::template pre_handler_controller<ClassType, typename std::remove_reference<T>::type>, that_, std::placeholders::_1, std::placeholders::_2, function_, class_, tp);
			that_->reg_router(url_, methods_, std::move(b),std::get<1>(tp));
		}
		nonstd::string_view url_;
		Methods methods_;
		Function function_;
		ClassType* class_;
		MemberClass* that_;
	};

	enum class router_type:std::uint16_t {
		specify,
		general,
		ws,
		unknow
	};

	class http_router {
		friend class http_server;
		template<typename Methods, typename Function, typename MemberClass>
		friend struct fiction_auto_lambda_in_router0;
		template<typename Methods, typename Function, typename ClassType, typename MemberClass>
		friend struct fiction_auto_lambda_in_router1;
		template<typename Methods, typename Function, typename ClassType, typename MemberClass>
		friend struct fiction_auto_lambda_in_router2;
		template<typename Methods, typename Function, typename MemberClass>
		friend struct fiction_auto_lambda_for_ws;
	public:
		using router_function = std::function<void(request&, response&)>;
		using interceptor_function = std::function<bool(request&, response&)>;
	public:
		http_router() :timer_(io_), websocket_hub_instance_(websocket_hub::get()){
		}
		~http_router() {
			std::error_code ignore_err;
			timer_.cancel(ignore_err);
			io_.stop();
			if (thread_!=nullptr && thread_->joinable()) {
				thread_->join();
			}
			websocket_hub_instance_.end_thread_ = true;
			websocket_hub_instance_.add({ nullptr,nullptr });
			websocket_hub_instance_.condtion_.notify_all();
			if (websocket_hub_thread_!=nullptr && websocket_hub_thread_->joinable()) {
				websocket_hub_thread_->join();
			}
		}
	private:
		void run() {
			check_session_expires();
			thread_ = std::move(std::unique_ptr<std::thread>(new std::thread([](asio::io_service& io) {
				io.run();
			}, std::ref(io_))));
			websocket_hub_thread_ = std::move(std::unique_ptr<std::thread>(new std::thread([this]() {
				run_hub_send();
			})));
		}
		void run_hub_send() {
			websocket_hub_instance_.send();
		}
	private:
		template<typename Array, typename Bind,typename Interceptor>
		std::string reg_router(nonstd::string_view url, Array&& methods, Bind&& router, Interceptor&& interceptors) {
			auto interceptor_pre_process = [interceptors](request& req,response& res) mutable ->bool  {
				using tupleType = typename std::remove_reference<decltype(interceptors)>::type;
				bool result = true;
				each_tuple<0, tuple_size<tupleType>::value>{}(interceptors, c11_auto_lambda_interceptor_pre{ result,req,res });
				return result;
			};
			auto generator = url.find('*');
			if (generator == nonstd::string_view::npos) {
				auto url_str = std::string(url.data(), url.size());
				for (auto& iter : methods) {
					std::string key = iter + url_str;
					router_map_.insert(std::make_pair(key, static_cast<router_function>(router)));
					interceptor_map_.insert(std::make_pair(key, static_cast<interceptor_function>(interceptor_pre_process)));
				}
				return url_str;
			}
			else {
				auto url_ = url.substr(0, generator - 1);
				auto url_str = std::string(url_.data(), url_.size());
				for (auto& iter : methods) {
					std::string key = iter + url_str;
					genera_router_map_.insert(std::make_pair(key, static_cast<router_function>(router)));
					genera_interceptor_map_.insert(std::make_pair(key, static_cast<interceptor_function>(interceptor_pre_process)));
				}
				return url_str;
			}
		}

		template<typename Array, typename Function, typename...Args>
		void router(nonstd::string_view url, Array&& methods, Function&& lambda, Args&& ...args) { //lambda or regular function
			fiction_auto_lambda_in_router0<Array, Function, http_router> callable(url, std::forward<Array>(methods), std::forward<Function>(lambda),this);
			reorganize_tuple_v1(callable, std::tuple<>{}, std::tuple<>{}, std::tuple<>{}, std::forward<Args>(args)...);
		}

		template<typename Array, typename Ret, typename Class, typename...Params, typename Object, typename...Args>
		void router(nonstd::string_view url, Array&& methods, Ret(Class::* memberfunc)(Params...), Object& that, Args&& ...args) {  // member function with lvalue object argument
			fiction_auto_lambda_in_router1< Array, Ret(Class::*)(Params...), Object, http_router> callable(url, std::forward<Array>(methods), memberfunc, static_cast<Object*>(&that), this);
			reorganize_tuple_v1(callable, std::tuple<>{}, std::tuple<>{}, std::tuple<>{}, std::forward<Args>(args)...);
		}

		template<typename Array, typename Ret, typename Class, typename...Params, typename...Args>
		void router(nonstd::string_view url, Array&& methods, Ret(Class::* memberfunc)(Params...), Args&& ...args) {  // member function elision object argument
			fiction_auto_lambda_in_router1< Array, Ret(Class::*)(Params...), Class, http_router> callable(url, std::forward<Array>(methods), memberfunc, static_cast<Class*>(nullptr), this);
			reorganize_tuple_v1(callable, std::tuple<>{}, std::tuple<>{}, std::tuple<>{}, std::forward<Args>(args)...);
		}

		template<typename Array, typename Ret, typename Class, typename...Params,typename Object, typename...Args>
		void router(nonstd::string_view url, Array&& methods, Ret(Class::* memberfunc)(Params...), Object* that, Args&& ...args) {  // member function with object pointer
			fiction_auto_lambda_in_router1< Array, Ret(Class::*)(Params...), Class, http_router> callable(url, std::forward<Array>(methods), memberfunc, static_cast<Object*>(that), this);
			reorganize_tuple_v1(callable, std::tuple<>{}, std::tuple<>{}, std::tuple<>{}, std::forward<Args>(args)...);
		}

		template<typename Array, typename Class, typename...Args>
		typename std::enable_if<std::is_base_of<Controller, typename std::remove_cv<Class>::type>::value>::type router(nonstd::string_view url, Array&& methods, void(Class::* memberfunc)(), Class& that, Args&& ...args) {  //controller with lvalue object argument
			fiction_auto_lambda_in_router2< Array, void(Class::*)(), Class, http_router> callable(url, std::forward<Array>(methods), memberfunc, static_cast<Class*>(&that), this);
			reorganize_tuple_v1(callable, std::tuple<>{}, std::tuple<>{}, std::tuple<>{}, std::forward<Args>(args)...);
		}

		template<typename Array, typename Class, typename...Args>
		typename std::enable_if<std::is_base_of<Controller, typename std::remove_cv<Class>::type>::value>::type router(nonstd::string_view url, Array&& methods, void(Class::* memberfunc)(), Args&& ...args) {  //controller elision object argument
			fiction_auto_lambda_in_router2< Array, void(Class::*)(), Class, http_router> callable(url, std::forward<Array>(methods), memberfunc, static_cast<Class*>(nullptr), this);
			reorganize_tuple_v1(callable, std::tuple<>{}, std::tuple<>{}, std::tuple<>{}, std::forward<Args>(args)...);
		}

		template<typename Array, typename Class, typename Object, typename...Args>
		typename std::enable_if<std::is_base_of<Controller, Class>::value>::type router(nonstd::string_view url, Array&& methods, void(Class::* memberfunc)(), Object* that, Args&& ...args) {  //controller with object pointer
			fiction_auto_lambda_in_router2< Array, void(Class::*)(), Class, http_router> callable(url, std::forward<Array>(methods), memberfunc, static_cast<Object*>(that), this);
			reorganize_tuple_v1(callable, std::tuple<>{}, std::tuple<>{}, std::tuple<>{}, std::forward<Args>(args)...);
		}

		// ws router
		template<typename Function,typename...Args>
		void router_ws(nonstd::string_view url, websocket_event const& Event, Function&& lambda, Args&&...args) {
			std::array<std::string, 1> method = { "WEBSOSCKET" };
			fiction_auto_lambda_for_ws<std::array<std::string, 1>, Function, http_router> callable(url, std::forward<std::array<std::string, 1>>(method), std::forward<Function>(lambda), this, Event);
			reorganize_tuple_v1(callable, std::tuple<>{}, std::tuple<>{}, std::tuple<>{}, std::forward<Args>(args)...);
		}

		///lambda or regular function  process --start
		template<typename Function, typename Tuple>
		void pre_handler(request& req, response& res, Function& function, Tuple& tp) const {  //lambda or regular function
			auto_params_lambda<Function> lambda{ req ,res ,function };
			lambda(tp);
		}
		///lambda or regular function  process --end

		///class member function process   --start
		template<typename Function, typename Class, typename Tuple>
		void pre_handler_member_function(request& req, response& res, Function& function, Class* that, Tuple& tp) const {
			auto_params_lambda<Function, Class> lambda{ req ,res ,that ,function };
			lambda(tp);
		}

		///class member function process   --end;

		///controller process --start
		template<typename Class, typename Tuple>
		void pre_handler_controller(request& req, response& res, void(Class::* memberfunc)(), Class* that, Tuple& tp) const {
			auto_params_lambda<void(Class::*)(), Class> lambda{ req ,res ,that ,memberfunc };
			lambda(tp);
		}


		///controller process --end

	public:
		//void post_router(request& req, response& res) {
		//	auto url = req.url();
		//	if (url.size() > 1) {  //确保这里不是 / 根路由
		//		std::size_t back = 0;
		//		while ((url.size() - back - 1) > 0 && url[url.size() - back - 1] == '/') { //去除url 最后有/的干扰 /xxx/xxx/ - > /xxx/xxx
		//			back++;
		//		}
		//		if (back > 0) {  //如果url不是规范的url 则重定向跳转
		//			if (url_redirect_) {
		//				auto url_str = view2str(url.substr(0, url.size() - back));
		//				auto params = req.raw_key_params();
		//				if (!params.empty()) {
		//					url_str += "?";
		//				}
		//				url_str += view2str(params);
		//				res.redirect(url_str);
		//				return;
		//			}
		//			else {
		//				url = url.substr(0, url.size() - back);
		//			}
		//		}
		//	}
		//	auto key = view2str(req.method()) + view2str(url);
		//	if (router_map_.find(key) != router_map_.end()) {
		//		auto& it = router_map_.at(key);
		//		set_view_method(res);
		//		it(req, res);
		//		return;
		//	}
		//	else {
		//		using value_type = std::pair<std::string, router_function>;
		//		std::map<std::size_t, value_type> overload_set;
		//		for (auto& iter : genera_router_map_) {
		//			auto pos = key.find("/", iter.first.size());
		//			auto gurl = key.substr(0, pos - 0);
		//			if (iter.first == gurl) {
		//				overload_set.insert(std::make_pair(iter.first.size(), iter));
		//			}
		//		}
		//		if (!overload_set.empty()) {  //重载执行最佳匹配的url
		//			auto endIter = overload_set.end();
		//			auto& iter = (*(--endIter)).second;
		//			set_view_method(res);
		//			auto& url = iter.first;
		//			auto pos = url.find("/");
		//			if (pos == std::string::npos) {
		//				req.set_generic_path("");
		//			}
		//			else {
		//				req.set_generic_path(url.substr(pos, url.size() - pos));
		//			}
		//			(iter.second)(req, res);
		//			return;
		//		}
		//	}
		//	if (not_found_callback_ != nullptr) {
		//		not_found_callback_(req, res);
		//	}
		//	else {
		//		std::stringstream ss;
		//		ss << "the request " << view2str(req.method()) << " \"" << view2str(req.url()) << "\" is not found";
		//		res.write_string(ss.str(), false, http_status::bad_request);
		//	}
		//	if (req.content_type() == content_type::multipart_form) {
		//		auto& files = req.files();
		//		for (auto& iter : files) {
		//			iter.second.remove();
		//		}
		//	}
		//	if (req.content_type() == content_type::octet_stream) {
		//		auto& file = req.file();
		//		file.remove();
		//	}
		//}
		std::pair<std::string,router_function> pre_router(request& req, response& res, router_type& rtype,bool is_websocket = false) const {
			auto url = req.url();
			if (url.size() > 1) {  //确保这里不是 / 根路由
				std::size_t back = 0;
				while ((url.size() - back - 1) > 0 && url[url.size() - back - 1] == '/') { //去除url 最后有/的干扰 /xxx/xxx/ - > /xxx/xxx
					back++;
				}
				if (back > 0) {  //如果url不是规范的url 则重定向跳转
					url = url.substr(0, url.size() - back);
				}
			}
			auto url_str = view2str(url);
			auto key = is_websocket==false?view2str(req.method()) + url_str:"WEBSOSCKET"+ url_str;
			auto&& common_iter = router_map_.find(key);
			if (common_iter != router_map_.end()) {
				//auto& it = router_map_.at(key);
				set_view_method(res);
				rtype = router_type::specify;
				return *common_iter;
			}
			else {
				using value_type = std::pair<std::string, router_function>;
				std::map<std::size_t, value_type> overload_set;
				for (auto& iter : genera_router_map_) {
					auto pos = key.find("/", iter.first.size());
					auto gurl = key.substr(0, pos - 0);
					if (iter.first == gurl) {
						overload_set.insert(std::make_pair(iter.first.size(), iter));
					}
				}
				if (!overload_set.empty()) {  //重载执行最佳匹配的url
					auto endIter = overload_set.end();
					auto& iter = (*(--endIter)).second;
					set_view_method(res);
					auto& url = iter.first;
					auto pos = url.find("/");
					if (pos == std::string::npos) {
						req.set_generic_path("");
					}
					else {
						req.set_generic_path(url.substr(pos, url.size() - pos));
					}
					rtype = router_type::general;
					return iter ;
				}
				auto wb_iter = websocket_router_map_.find(url_str);
				if (wb_iter != websocket_router_map_.end()) {
					rtype = router_type::ws;
					return std::pair<std::string,router_function>{ url_str ,nullptr };
				}
			}
			//if (req.content_type() == content_type::multipart_form) {
			//	auto& files = req.files();
			//	for (auto& iter : files) {
			//		iter.second.remove();
			//	}
			//}
			//if (req.content_type() == content_type::octet_stream) {
			//	auto& file = req.file();
			//	file.remove();
			//}
			rtype = router_type::unknow;
			if (not_found_callback_ != nullptr) {
				return std::pair<std::string,router_function>{ "",not_found_callback_ };
			}
			else {
				return std::pair<std::string,router_function>{ "",[](request& req, response& res) {
					std::stringstream ss;
					ss << "the request " << view2str(req.method()) << " \"" << view2str(req.url()) << "\" is not found";
					res.write_string(ss.str(), false, http_status::not_found);
				} };
			}
		}
	public:
		interceptor_function get_interceptors_process(std::string const& key, router_type type) const {
			if (type == router_type::specify) {
				auto iter = interceptor_map_.find(key);
				if (iter != interceptor_map_.end()) {
					return iter->second;
				}
			}
			else if(type == router_type::general){
				auto iter = genera_interceptor_map_.find(key);
				if (iter != genera_interceptor_map_.end()) {
					return  iter->second;
				}
			}
			else if (type == router_type::ws) {
				auto iter = websocket_interceptor_map_.find(key);
				if (iter != websocket_interceptor_map_.end()) {
					return  iter->second;
				}
			}
			return nullptr;
		}
	public:
		void trigger_error(std::string const& err) {
			utils::messageCenter::get().trigger_message(err);
		}
	private:
		void set_view_method(response& res) const {
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
				if (ec) {
					return;
				}
				session_manager<class session>::get().check_expires();
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
		std::unordered_map<std::string, router_function> router_map_;
		std::unordered_map<std::string, router_function> genera_router_map_;
		std::unordered_map<std::string, interceptor_function> interceptor_map_;
		std::unordered_map<std::string, interceptor_function> genera_interceptor_map_;
		std::unordered_map<std::string, router_function> websocket_router_map_;
		std::unordered_map<std::string, interceptor_function> websocket_interceptor_map_;
		std::unordered_map<std::string, std::pair<std::size_t, inja::CallbackFunction>> view_method_map_;
		asio::io_service io_;
		asio::steady_timer timer_;
		std::unique_ptr<std::thread> thread_;
		std::unique_ptr<std::thread> websocket_hub_thread_;
		std::time_t check_session_time_ = 10;
		class websockets websockets_;
		bool url_redirect_ = true;
		std::function<void(request&, response&)> not_found_callback_ = nullptr;
		websocket_hub& websocket_hub_instance_;
	};
}