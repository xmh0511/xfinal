#pragma once
#include <string>
#include "string_view.hpp"
#include <map>
#include "content_type.hpp"
#include "files.hpp"
#include <unordered_map>
#include <asio.hpp>
#include "response_status.hpp"
#include <vector>
#include <tuple>
#include "inja.hpp"
#include "session.hpp"
#include "any.hpp"
namespace xfinal {

	class connection;
	class request;
	class response;
	class http_router;
	class request :private nocopyable {
		friend class connection;
		friend class response;
		friend class http_router;
	private:
		request(class connection* connect_) :connecter_(connect_) {

		}
	public:
		nonstd::string_view header(std::string const& key) const noexcept {
			auto lkey = to_lower(key);
			if (headers_ != nullptr) {
				for (auto& iter : *headers_) {
					auto key_str = to_lower(view2str(iter.first));
					if (key_str == lkey) {
						return iter.second;
					}
				}
			}
			return "";
		}
		class connection& connection() {
			return *connecter_;
		}
	private:
		bool is_key_params(nonstd::string_view view) const {
			auto str = view2str(view);
			std::map<nonstd::string_view, nonstd::string_view> form;
			http_urlform_parser{ str ,false }.parse_data(form);
			if (!form.empty()) {
				return true;
			}
			return false;
		}
	public:
		bool has_body() const noexcept {
			if (header("content-length").empty()) {
				return false;
			}
			if (std::atoi(header("content-length").data()) > 0) {
				return true;
			}
			return false;
		}
		std::size_t body_length() const noexcept {
			if (header("content-length").empty()) {
				return 0;
			}
			return std::atol(header("content-length").data());
		}

		nonstd::string_view url() const noexcept {
			auto it = url_.rfind('?');
			if (it != (nonstd::string_view::size_type)nonstd::string_view::npos) {
				bool r = is_key_params(url_.substr(it + 1));
				if (r) {
					return url_.substr(0, it);
				}
			}
			return url_;
		}

		nonstd::string_view param(nonstd::string_view key) const noexcept {  //获取问号后的键值对参数值
			auto key_str = view2str(key);
			auto it = decode_url_params_.find(key_str);
			if (it != decode_url_params_.end()) {
				return it->second;
			}
			return  "";
		}

		nonstd::string_view param(std::size_t index) const noexcept {  //获取url参数类型的参数值
			if (is_generic_) {
				auto pos = generic_base_path_.size();
				auto url_fix = url();
				if (url_fix.size() >= (pos + 1)) {
					pos += 1;
				}
				auto parms = url_fix.substr(pos, url_fix.size() - pos);
				auto vec = split(parms, "/");
				return vec.size() > index ? vec[index] : "";
			}
			return "";
		}

		nonstd::string_view raw_url_params() const noexcept {  //获取url参数字符串
			if (is_generic_) {
				auto pos = generic_base_path_.size();
				auto url_fix = url();
				if (url_fix.size() >= (pos + 1)) {
					pos += 1;
				}
				auto parms = url_fix.substr(pos, url_fix.size() - pos);
				return parms;
			}
			return "";
		}

		nonstd::string_view raw_key_params() const noexcept {  //获取问号后的参数字符串
			auto it = url_.rfind('?');
			auto view = url_.substr(it + 1);
			if (it != (nonstd::string_view::size_type)nonstd::string_view::npos) {
				//bool r = is_key_params(view);
				//if (r) {
				//	return view;
				//}
				return view;
			}
			return "";
		}

		std::vector<nonstd::string_view> url_params() const noexcept {  //获取url类型参数的数组
			if (is_generic_) {
				auto pos = generic_base_path_.size();
				auto url_fix = url();
				if (url_fix.size() >= (pos + 1)) {
					pos += 1;
				}
				auto parms = url_fix.substr(pos, url_fix.size() - pos);
				auto vec = split(parms, "/");
				return vec;
			}
			return {};
		}

		std::map<std::string, std::string> key_params() const noexcept { //获取问号键值对的map
			return decode_url_params_;
		}



		nonstd::string_view raw_url()  const noexcept {
			return url_;
		}

		bool is_websocket() {
			return is_websocket_;
		}

		//template<typename T>
		//typename std::enable_if<std::is_same<T, GBK>::value, nonstd::string_view>::type param(nonstd::string_view key) {
		//	auto view = param(key);
		//	auto key_str = view2str(key);
		//	if (!view.empty()) {
		//		auto it = gbk_decode_params_.find(key_str);
		//		if (it != gbk_decode_params_.end()) {
		//			return { it->second.data(),it->second.size() };
		//		}
		//		else {
		//			gbk_decode_params_.insert(std::make_pair(key_str, utf8_to_gbk(view2str(view))));
		//			auto& v = gbk_decode_params_[key_str];
		//			return { v.data(),v.size() };
		//		}
		//	}
		//	else {
		//		return "";
		//	}
		//}

		std::map<std::string, xfinal::XFile> const& files() const noexcept {
			return *multipart_files_map_;
		}

		XFile const& file(nonstd::string_view filename) const noexcept {
			if (multipart_files_map_ != nullptr) {
				auto it = multipart_files_map_->find(view2str(filename));
				if (it != multipart_files_map_->end()) {
					return it->second;
				}
			}
			return *empty_file_;
		}

		XFile const& file() const noexcept {  //获取octstrem 文件
			return *oct_steam_;
		}

		bool remove_all_upload_files() {
			auto& files = *multipart_files_map_;
			for (auto& iter : files) {
				iter.second.remove();
			}
			return true;
		}

		nonstd::string_view method() const noexcept {
			return method_;
		}
		nonstd::string_view http_version() const noexcept {
			return version_;
		}

		enum content_type content_type() const noexcept {
			return content_type_;
		}

		nonstd::string_view query(nonstd::string_view key) const noexcept {
			auto key_str = view2str(key);
			auto it = decode_form_map_.find(key_str);
			if (it != decode_form_map_.end()) {
				return it->second;
			}
			else {
				if (multipart_form_map_ != nullptr) {
					auto itm = multipart_form_map_->find(key_str);
					if (itm != multipart_form_map_->end()) {
						return nonstd::string_view{ itm->second.data(),itm->second.size() };
					}
					else {
						return "";
					}
				}
				return "";
			}
		}

		//template<typename T>
		//typename std::enable_if<std::is_same<T, GBK>::value, nonstd::string_view>::type query(nonstd::string_view key) {
		//	auto view = query(key);
		//	auto key_str = view2str(key);
		//	if (!view.empty()) {
		//		auto it = gbk_decode_form_map_.find(key_str);
		//		if (it != gbk_decode_form_map_.end()) {
		//			return { it->second.data(),it->second.size() };
		//		}
		//		else {
		//			gbk_decode_form_map_.insert(std::make_pair(key_str, utf8_to_gbk(view2str(view))));
		//			auto& v = gbk_decode_form_map_[key_str];
		//			return { v.data(),v.size() };
		//		}
		//	}
		//	else {
		//		return "";
		//	}
		//}

		std::map<std::string, std::string> const& url_form() const noexcept {
			return decode_form_map_;
		}

		std::map<std::string, std::string> const& multipart_form() const noexcept {
			return *multipart_form_map_;
		}

		nonstd::string_view body() const noexcept {
			return body_;
		}

		std::string get_event_index_str() {
			return event_index_str_;
		}

		bool accept_range(std::int64_t& startpos, std::int64_t& endpos) {
			auto range = header("range");
			if (range.empty()) {
				return false;
			}
			auto posv = range.substr(range.find('=') + 1);
			auto line = posv.find('-');
			auto start = posv.substr(0, line);
			startpos = std::atoll(start.data());
			if (line != (nonstd::string_view::size_type)nonstd::string_view::npos) {
				auto potential_end = posv.substr(line + 1);
				if (!potential_end.empty()) {
					if (std::isdigit(potential_end[0])) {
						endpos = std::atoll(potential_end.data());
					}
					else {
						endpos = -1;
					}
				}
				else {
					endpos = -1;
				}
			}
			return true;
		}
		bool is_keep_alive() const {
			auto c = to_lower(view2str(header("connection")));
			using namespace nonstd::string_view_literals;
			if (c.empty()) {
				if (http_version() == "HTTP/1.1"_sv) {
					return true;
				}
				return false;
			}
			if (c == "close") {
				return false;
			}
			if (c == "keep-alive") {
				return true;
			}
			return false;
		}

		template<typename T>
		void set_user_data(std::string const& key,std::shared_ptr<T> const& value) {
			user_data_.emplace(key, value);
		}
		template<typename T>
		T get_user_data(std::string const& key) {
			auto it = user_data_.find(key);
			if (it != user_data_.end()) {
				return nonstd::any_cast<T>(it->second);
			}
			return nullptr;//error
		}
	public:
		class session& create_session() {
			return create_session("XFINAL");
		}
		class session& create_session(std::string const& name) {
			session_ = std::make_shared<class session>(false);
			session_->get_cookie().set_name(name);
			session_->init_id(uuids::uuid_system_generator{}().to_short_str());
			//session_->set_expires(600);
			session_manager<class session>::get().add_session(session_->get_id(), session_);
			return *session_;
		}

		class session& session(std::string const& name) {
			if (session_ != nullptr && !session_->empty()) {
				auto cookie_name = session_->get_cookie().name();
				if (cookie_name == name) {
					return *session_;
				}
			}
			auto cookies_value = header("cookie");
			auto name_view = nonstd::string_view{ name.data(),name.size() };
			auto it = cookies_value.find(name_view);
			if (it == (nonstd::string_view::size_type)nonstd::string_view::npos) {
				session_ = session_manager<class session>::get().empty_session();
			}
			else {
				auto pos = it + name.size() + 1;
				auto dot_pos = cookies_value.find(';', pos);
				auto id = dot_pos != (nonstd::string_view::size_type)nonstd::string_view::npos ? cookies_value.substr(pos, dot_pos - pos) : cookies_value.substr(pos, cookies_value.size() - pos);
				if (id.empty()) {
					session_ = session_manager<class session>::get().empty_session();
				}
				else {
					session_ = session_manager<class session>::get().validata(view2str(id));//验证session的有效性并返回
				}
			}
			return *session_;
		}
		class session& session() {
			return session("XFINAL");
		}

		void set_session(std::weak_ptr<class session> weak) {
			auto handler = weak.lock();
			if (handler != nullptr) {
				session_ = handler;
				session_manager<class session>::get().add_session(handler->get_id(), handler);
			}
		}

		class session& session_by_id(std::string const& sessionId) {
			session_ = session_manager<class session>::get().validata(sessionId);//验证session的有效性并返回
			return *session_;
		}

		class session_manager<class session>& get_session_manager() {
			return session_manager<class session>::get();
		}

	protected:
		void init_content_type() noexcept {
			auto value = header("content-type");
			if (!value.empty()) {
				auto has_op = value.find(';');
				std::string key;
				if (has_op == (nonstd::string_view::size_type)nonstd::string_view::npos) {
					key = to_lower(view2str(value));
				}
				else {
					auto value_view = value.substr(0, has_op);
					key = to_lower(view2str(value_view));
				}
				auto iter = content_type_str_type_map.find(key);
				if (iter != content_type_str_type_map.end()) {
					content_type_ = iter->second;
				}
				else {
					content_type_ = content_type::string;
				}
			}
			else {
				content_type_ = content_type::unknow;
			}
			if (content_type_ == content_type::multipart_form) {
				auto f = value.find("boundary");
				if (f != std::string::npos) {
					auto pos = f + sizeof("boundary");
					boundary_key_ = nonstd::string_view{ &(value[pos]),(value.size() - pos) };
				}
				else {
					boundary_key_ = "";
				}
			}
		}
		void init_url_params() {
			auto it = url_.rfind('?');
			if (it != (nonstd::string_view::size_type)nonstd::string_view::npos) {
				auto url = url_.substr(it + 1);
				http_urlform_parser{ url }.parse_data(url_params_);
				for (auto& iter : url_params_) {
					decode_url_params_.emplace(view2str(iter.first), url_decode(view2str(iter.second)));
				}
			}
		}
	private:
		void reset() {
			event_index_str_.clear();
			method_ = "";
			url_ = "";
			version_ = "";
			header_length_ = 0;
			form_map_.clear();
			body_ = "";
			//gbk_decode_form_map_.clear();
			decode_url_params_.clear();
			decode_form_map_.clear();
			url_params_.clear();
			//gbk_decode_params_.clear();
			boundary_key_ = "";
			is_generic_ = false;
			generic_base_path_ = "";
			session_ = nullptr;
			is_websocket_ = false;
			user_data_.clear();
		}
		void set_generic_path(std::string const& path) {
			is_generic_ = true;
			generic_base_path_ = path;
		}

		std::map<std::string, nonstd::any>&& move_user_data() {
			return std::move(user_data_);
		}
	private:
		nonstd::string_view method_;
		nonstd::string_view url_;
		nonstd::string_view version_;
		std::map<nonstd::string_view, nonstd::string_view> const* headers_ = nullptr;
		std::size_t header_length_ = 0;
		enum content_type content_type_ = content_type::unknow;
		std::map<nonstd::string_view, nonstd::string_view> form_map_;
		std::map<std::string, std::string> decode_form_map_;
		nonstd::string_view body_;
		//std::map<std::string, std::string> gbk_decode_form_map_;
		std::map<std::string, std::string> decode_url_params_;
		std::map<nonstd::string_view, nonstd::string_view> url_params_;
		//std::map<std::string, std::string> gbk_decode_params_;
		nonstd::string_view boundary_key_;
		std::map<std::string, std::string> const* multipart_form_map_ = nullptr;
		std::map<std::string, xfinal::XFile> const* multipart_files_map_ = nullptr;
		xfinal::XFile* oct_steam_ = nullptr;
		xfinal::XFile* empty_file_ = nullptr;
		class connection* connecter_;
		std::shared_ptr<class session> session_;
		bool is_generic_ = false;
		std::string generic_base_path_;
		std::map<std::string, nonstd::any> user_data_;
		bool is_websocket_ = false;
		std::string event_index_str_;
	};
	///响应
	template<typename Connection>
	class defer_guarder {
	public:
		defer_guarder() = default;
		defer_guarder(std::shared_ptr<Connection> const& smarter) :conn_(smarter) {

		}
	public:
		~defer_guarder() {
			conn_->cancel_defer_waiter();
			if (conn_ != nullptr) {
				conn_->do_after();
				if (!conn_->req_.is_websocket()) {
					conn_->write(); //回应请求
				}
				else {
					conn_->forward_write(true); //websocket
				}
			}
		}
		std::shared_ptr<Connection> operator ->() {
			return conn_;
		}
	private:
		std::shared_ptr<Connection> conn_ = nullptr;
	};

	class response :private nocopyable {
		friend class connection;
		friend class http_router;
		friend class request;
		template<typename...Args>
		friend struct auto_params_lambda;
		template<typename Connection>
		friend class defer_guarder;
	public:
		enum class write_type {
			string,
			file,
			no_body
		};
	public:
		class http_package {
			friend class response;
		public:
			http_package() = default;
			virtual ~http_package() = default;
			virtual void dump() = 0;
		protected:
			http_status state_ = http_status::init;
			std::unordered_multimap<std::string, std::string> header_map_;
			bool is_chunked_ = false;
			write_type write_type_ = write_type::string;
			std::string body_souce_;
		};
	private:
		response(request& req, class connection* connect_) :connecter_(connect_), req_(req), view_env_(new inja::Environment()) {
			//初始化view 配置
			view_env_->set_expression("@{", "}");
			view_env_->set_element_notation(inja::ElementNotation::Dot);
		}
	public:
		class connection& connection() {
			return *connecter_;
		}
		void add_header(std::string const& k, std::string const& v) noexcept {
			header_map_.insert(std::make_pair(k, v));
		}
	protected:
		template<typename T, typename U = typename std::enable_if<std::is_same<typename std::remove_reference<T>::type, std::string>::value>::type>
		void write(T&& body, http_status state = http_status::ok, std::string const& conent_type = "text/plain", bool is_chunked = false) noexcept {
			state_ = state;
			body_ = std::move(body);
			if (!is_chunked) {
				add_header("Content-Length", std::to_string(body_.size()));
			}
			else {
				add_header("Transfer-Encoding", "chunked");
			}
			if (!exist_header("Content-Type")) {
				add_header("Content-Type", conent_type);
			}
			is_chunked_ = is_chunked;
		}
	public:
		bool exist_header(std::string const& k) {
			auto key = to_lower(k);
			for (auto& iter : header_map_) {
				if (to_lower(iter.first) == key) {
					return true;
				}
			}
			return false;
		}
		void remove_header(std::string const& k) {
			auto key = to_lower(k);
			for (auto iter = header_map_.begin(); iter != header_map_.end();) {
				if (to_lower(iter->first) == key) {
					header_map_.erase(iter);
					return;
				}
				++iter;
			}
		}
	public:
		void write_state(http_status state = http_status::ok) {
			write_type_ = write_type::no_body;
			init_start_pos_ = 0;
			state_ = state;
			is_chunked_ = false;
		}

		void write_string(std::string&& content, bool is_chunked = false, http_status state = http_status::ok, std::string const& conent_type = "text/plain") noexcept {
			write(std::move(content), state, conent_type, is_chunked);
			write_type_ = write_type::string;
			init_start_pos_ = 0;
		}

		void write_file(std::string const& filename, bool is_chunked = false) noexcept {
			using namespace nonstd::literals;
			if (!filename.empty()) {
				bool b = file_.open(filename);
				if (!b) {
					write_string("", false, http_status::bad_request);
				}
				else {
					if (!exist_header("Content-Type")) { //have not specified content_type
						add_header("Content-Type", view2str(file_.content_type()));
					}
					auto filesize = file_.size();
					if (req_.method() == "HEAD"_sv) {  //询问是否支持range
						add_header("Content-Length", std::to_string(filesize));
						add_header("Accept-Ranges", "bytes");
						write_type_ = write_type::string;
						init_start_pos_ = 0;
						state_ = http_status::ok;
						is_chunked_ = false;
						return;
					}
					write_type_ = write_type::file;
					is_chunked_ = is_chunked;
					init_start_pos_ = -1;
					init_end_pos_ = -1;
					portion_need_size_ = 0;
					if (!is_chunked) {  //非chunk方式
						state_ = http_status::ok;
						add_header("Content-Length", std::to_string(filesize));
						file_.read_all(body_);
						return;
					}
					else {
						add_header("Transfer-Encoding", "chunked");
						body_.resize((std::size_t)chunked_size_);
						if (req_.accept_range(init_start_pos_, init_end_pos_)) {
							state_ = http_status::partial_content;
							std::string rang_value;
							if (init_end_pos_ == -1) {
								rang_value = std::string("bytes ") + std::to_string(init_start_pos_) + std::string("-") + std::to_string(filesize - 1) + "/" + std::to_string(filesize);
								portion_need_size_ = filesize - init_start_pos_;
							}
							else {
								rang_value = std::string("bytes ") + std::to_string(init_start_pos_) + std::string("-") + std::to_string(init_end_pos_) + "/" + std::to_string(filesize);
								portion_need_size_ = init_end_pos_ - init_start_pos_ + 1;
							}
							add_header("Content-Range", rang_value);
						}
						else {  //普通chunk 非range
							state_ = http_status::ok;
							init_start_pos_ = -1;
							init_end_pos_ = -1;
							portion_need_size_ = filesize;
						}
					}
				}
			}
			else {
				write_string("", false, http_status::bad_request);
			}
		}

		void write_json(json const& json, bool is_chunked = false) noexcept {
			try {
				write_string(json.dump(), is_chunked, http_status::ok, "application/json");
			}
			catch (json::type_error const& ec) {
				write_string(ec.what(), false, http_status::bad_request);
			}
		}

		void write_json(std::string&& json, bool is_chunked = false) noexcept {
			try {
				write_string(std::move(json), is_chunked, http_status::ok, "application/json");
			}
			catch (json::type_error const& ec) {
				write_string(ec.what(), false, http_status::bad_request);
			}
		}

		void write_file_view(std::string const& filename, bool is_chunked = false, http_status state = http_status::ok) noexcept {
			std::string extension = "text/plain";
			auto path = fs::path(filename);
			if (path.has_extension()) {
				extension = view2str(get_content_type(path.extension().string()));
			}
			if (!fs::exists(path)) {
				write_string("", false, http_status::bad_request);
				return;
			}
			try {
				write_string(view_env_->render_file(filename, view_data_), is_chunked, state, extension);
			}
			catch (std::runtime_error const& ec) {
				write_string(ec.what(), false, http_status::bad_request);
			}
		}

		void write_file_view(std::string const& filename, json const& json, bool is_chunked = false, http_status state = http_status::ok) noexcept {
			view_data_ = json;
			write_file_view(filename, is_chunked, state);
		}

		void write_data_view(std::string const& data, bool is_chunked = false, http_status state = http_status::ok,std::string const& content_type="text/plain") noexcept {
			try {
				write_string(view_env_->render(data, view_data_), is_chunked, state, content_type);
			}
			catch (std::runtime_error const& ec) {
				write_string(ec.what(), false, http_status::bad_request);
			}
		}

		void write_data_view(std::string const& data, json const& json, bool is_chunked = false, http_status state = http_status::ok, std::string const& content_type = "text/plain") noexcept {
			view_data_ = json;
			write_data_view(data, is_chunked, state, content_type);
		}

		void write_http_package(http_package&& package) {
			package.dump();
			for (auto& iter : package.header_map_) {
				if (!exist_header(iter.first)) {
					add_header(iter.first, iter.second);
				}
			}
			if (package.write_type_ == write_type::string) {
				write_string(std::move(package.body_souce_), package.is_chunked_, package.state_);
			}
			else if (package.write_type_ == write_type::no_body) {
				write_state(package.state_);
			}
			else if (package.write_type_ == write_type::file) {
				write_file(package.body_souce_, package.is_chunked_);
			}
		}

		void redirect(nonstd::string_view url, bool is_temporary = true) noexcept {
			write_type_ = write_type::no_body;
			add_header("Location", view2str(url));
			if (is_temporary) {
				state_ = http_status::moved_temporarily;
			}
			else {
				state_ = http_status::moved_permanently;
			}
		}

		std::shared_ptr<defer_guarder<class connection>> defer() {
			return defer_guarder_.lock();
		}
	public:
		inja::Environment& view_environment() {
			return *view_env_;
		}

		template<typename T, typename U = typename std::enable_if<!std::is_same<typename std::decay<T>::type, char const*>::value>::type>
		void set_attr(std::string const& name, T&& value) noexcept {
			view_data_[name] = std::forward<T>(value);
		}

		void set_attr(std::string const& name, std::string const& value) noexcept {
			view_data_[name] = value;
		}

		template<typename T>
		T get_attr(std::string const& name) noexcept {
			auto it = view_data_.find(name);
			if (it != view_data_.end()) {
				return view_data_[name].get<T>();
			}
			return T{};
		}
	private:
		std::vector<asio::const_buffer> header_to_buffer() noexcept {
			std::vector<asio::const_buffer> buffers_;
			add_header("server", "xfinal");//增加服务器标识
			if (write_type_ == write_type::no_body) {//对于没有body的响应需要明确指定Content-Length=0,否则对于某些客户端会当作chunked处理导致客户端处理出错
				add_header("Content-Length", "0");
			}
			http_version_ = view2str(req_.http_version()) + ' ';//写入回应状态行 
			buffers_.emplace_back(asio::buffer(http_version_));
			buffers_.emplace_back(http_state_to_buffer(state_));
			if ((req_.session_ != nullptr) && !(req_.session_->empty())) {  //是否有session
				if (req_.session_->cookie_update()) {
					add_header("Set-Cookie", req_.session_->cookie_str());
				}
				if (req_.session_->cookie_update() || req_.session_->data_update()) {
					req_.session_->save(session_manager<class session>::get().get_storage()); //保存到存储介质
					req_.session_->set_data_update(false);
				}
				req_.session_->set_cookie_update(false);
			}
			for (auto& iter : header_map_) {  //回写响应头部
				buffers_.emplace_back(asio::buffer(iter.first));
				buffers_.emplace_back(asio::buffer(name_value_separator.data(), name_value_separator.size()));
				buffers_.emplace_back(asio::buffer(iter.second));
				buffers_.emplace_back(asio::buffer(crlf.data(), crlf.size()));
			}
			buffers_.emplace_back(asio::buffer(crlf.data(), crlf.size()));  //头部结束
			return buffers_;
		}
		std::vector<asio::const_buffer> to_buffers() noexcept {  //非chunked 模式 直接返回所有数据
			auto  buffers_ = header_to_buffer();
			//写入body
			if (write_type_ != write_type::no_body) {  //非no_body类型 
				buffers_.emplace_back(asio::buffer(body_.data(), body_.size()));
			}
			return buffers_;
		}

		std::tuple<bool, std::vector<asio::const_buffer>> chunked_body() noexcept {
			std::vector<asio::const_buffer> buffers;
			switch (write_type_) {
			case write_type::string: //如果是文本数据
			{
				auto left_size = body_.size() - (std::size_t)init_start_pos_;
				if (left_size <= chunked_size_) {
					chunked_write_size_ = to_hex(left_size);
					buffers.emplace_back(asio::buffer(chunked_write_size_));
					buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
					buffers.emplace_back(asio::buffer(&body_[(std::size_t)init_start_pos_], left_size));
					buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
					return { true, buffers };
				}
				else {  //还有超过每次chunked传输的数据大小
					chunked_write_size_ = to_hex(chunked_size_);
					buffers.emplace_back(asio::buffer(chunked_write_size_));
					buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
					buffers.emplace_back(asio::buffer(&body_[(std::size_t)init_start_pos_], (std::size_t)chunked_size_));
					buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
					init_start_pos_ = init_start_pos_ + chunked_size_;
					return { false,buffers };
				}
			}
			break;
			case write_type::file:  //如果是文件数据
			{
				bool eof = false;
				std::uint64_t read_size = 0;
				if (portion_need_size_ > chunked_size_) {  //请求的rang大小 大于chunked_size_
					read_size = file_.read(init_start_pos_, &(body_[0]), chunked_size_);
					portion_need_size_ -= chunked_size_;
					eof = !(portion_need_size_ > 0);  //还有剩余
				}
				else {  //一次chunked_size_就写完了range需要的字节数
					read_size = file_.read(init_start_pos_, &(body_[0]), portion_need_size_);
					eof = read_size <= portion_need_size_;
				}
				chunked_write_size_ = to_hex(read_size);
				buffers.emplace_back(asio::buffer(chunked_write_size_));
				buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
				buffers.emplace_back(asio::buffer(&body_[0], (std::size_t)read_size));
				buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
				init_start_pos_ = -1;  //下一次就不需要在重新定位文件
				init_end_pos_ = -1;
				return { eof,buffers };
			}
			break;
			default:
				return { true,buffers };
				break;
			}
		}
	public:
	private:
		void reset() {
			header_map_.clear();
			body_.clear();
			is_chunked_ = false;
			http_version_.clear();
			write_type_ = write_type::string;
			chunked_write_size_.clear();
			file_.close();
			view_data_.clear();
			init_start_pos_ = -1;
			init_end_pos_ = -1;
			portion_need_size_ = 0;
			defer_guarder_.reset();
			after_excutor_ = nullptr;
		}
	private:
		class connection* connecter_;
		request& req_;
		std::unordered_multimap<std::string, std::string> header_map_;
		std::string body_;
		http_status state_ = http_status::init;
		bool is_chunked_ = false;
		std::string http_version_;
		write_type write_type_ = write_type::string;
		std::uint64_t chunked_size_ = 1 * 1024 * 1024;
		std::string chunked_write_size_;
		XFile file_;
		std::int64_t init_start_pos_ = -1;
		std::int64_t init_end_pos_ = -1;
		std::uint64_t portion_need_size_ = 0;
		std::unique_ptr<inja::Environment> view_env_;
		json view_data_;
		std::weak_ptr<defer_guarder<class connection>> defer_guarder_;
		std::function<void(bool&, request&, response&)> after_excutor_ = nullptr;
	};

	class Controller :private nocopyable {
		friend class http_router;
		template<typename...Args>
		friend struct auto_params_lambda;
	public:
		Controller() = default;
	public:
		request& get_request() noexcept {
			return *req_;
		}
		response& get_response() noexcept {
			return *res_;
		}
	private:
		class request* req_;
		class response* res_;
	};

}