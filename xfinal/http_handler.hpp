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
namespace xfinal {

	class connection;
	class request;
	class response;
	class http_router;
	class request :private nocopyable {
		friend class connection;
		friend class response;
		friend class http_router;
	public:
		request(response& res, class connection* connect_) :connecter_(connect_), res_(res) {

		}
	public:
		nonstd::string_view header(std::string const& key) const noexcept {
			auto lkey = to_lower(key);
			if (headers_ != nullptr) {
				if ((*headers_).find(lkey) != (*headers_).end()) {
					return (*headers_).at(lkey);
				}
			}
			return "";
		}
		class connection& connection() {
			return *connecter_;
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
			auto it = url_.find('?');
			if (it != (nonstd::string_view::size_type)nonstd::string_view::npos) {
				return url_.substr(0, it);
			}
			return url_;
		}
		nonstd::string_view param(nonstd::string_view key) noexcept {
			if (decode_url_params_.empty()) {
				auto it = url_.find('?');
				if (it != (nonstd::string_view::size_type)nonstd::string_view::npos) {
					decode_url_params_ = url_decode(view2str(url_.substr(it + 1, url_.size())));
					http_urlform_parser{ decode_url_params_ }.parse_data(url_params_);
				}
			}
			auto vit = url_params_.find(key);
			if (vit != url_params_.end()) {
				return vit->second;
			}
			return "";
		}

		nonstd::string_view param(std::size_t index) noexcept {
			if (is_generic_) {
				auto pos = generic_base_path_.size();
				if (url_.size() >= (pos + 1)) {
					pos += 1;
				}
				auto parms = url_.substr(pos, url_.size() - pos);
				auto vec = split(parms, "/");
				return vec.size() > index ? vec[index] : "";
			}
			return "";
		}

		nonstd::string_view raw_params() const noexcept {
			if (is_generic_) {
				auto pos = generic_base_path_.size();
				if (url_.size() >= (pos + 1)) {
					pos += 1;
				}
				auto parms = url_.substr(pos, url_.size() - pos);
				return parms;
			}
			auto it = url_.find('?');
			if (it != (nonstd::string_view::size_type)nonstd::string_view::npos) {
				return url_.substr(it + 1, url_.size());
			}
			return "";
		}

		std::vector<nonstd::string_view> url_params() noexcept {
			if (is_generic_) {
				auto pos = generic_base_path_.size();
				if (url_.size() >= (pos + 1)) {
					pos += 1;
				}
				auto parms = url_.substr(pos, url_.size() - pos);
				auto vec = split(parms, "/");
				return vec;
			}
			return {};
		}

		std::map<nonstd::string_view, nonstd::string_view> key_params() noexcept {
			if (decode_url_params_.empty()) {
				auto it = url_.find('?');
				if (it != (nonstd::string_view::size_type)nonstd::string_view::npos) {
					decode_url_params_ = url_decode(view2str(url_.substr(it + 1, url_.size())));
					http_urlform_parser{ decode_url_params_ }.parse_data(url_params_);
				}
			}
			return url_params_;
		}



		nonstd::string_view raw_url()  const noexcept {
			return url_;
		}

		template<typename T>
		typename std::enable_if<std::is_same<T, GBK>::value, nonstd::string_view>::type param(nonstd::string_view key) {
			auto view = param(key);
			auto key_str = view2str(key);
			if (!view.empty()) {
				auto it = gbk_decode_params_.find(key_str);
				if (it != gbk_decode_params_.end()) {
					return { it->second.data(),it->second.size() };
				}
				else {
					gbk_decode_params_.insert(std::make_pair(key_str, utf8_to_gbk(view2str(view))));
					auto& v = gbk_decode_params_[key_str];
					return { v.data(),v.size() };
				}
			}
			else {
				return "";
			}
		}

		filewriter const& file(nonstd::string_view filename) const noexcept {
			if (multipart_files_map_ != nullptr) {
				auto it = multipart_files_map_->find(view2str(filename));
				if (it != multipart_files_map_->end()) {
					return it->second;
				}
			}
			return *oct_steam_;
		}

		filewriter const& file() const noexcept {
			return *oct_steam_;
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
			auto it = form_map_.find(key);
			if (it != form_map_.end()) {
				return it->second;
			}
			else {
				if (multipart_form_map_ != nullptr) {
					auto itm = multipart_form_map_->find(view2str(key));
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

		template<typename T>
		typename std::enable_if<std::is_same<T, GBK>::value, nonstd::string_view>::type query(nonstd::string_view key) {
			auto view = query(key);
			auto key_str = view2str(key);
			if (!view.empty()) {
				auto it = gbk_decode_form_map_.find(key_str);
				if (it != gbk_decode_form_map_.end()) {
					return { it->second.data(),it->second.size() };
				}
				else {
					gbk_decode_form_map_.insert(std::make_pair(key_str, utf8_to_gbk(view2str(view))));
					auto& v = gbk_decode_form_map_[key_str];
					return { v.data(),v.size() };
				}
			}
			else {
				return "";
			}
		}

		std::map<nonstd::string_view, nonstd::string_view> const& url_form() const noexcept {
			return form_map_;
		}

		std::map<std::string, std::string> const& multipart_form() const noexcept {
			return *multipart_form_map_;
		}

		nonstd::string_view body() const noexcept {
			return body_;
		}

		bool accept_range(std::uint64_t& pos) {
			auto range = header("range");
			if (range.empty()) {
				return false;
			}
			auto posv = range.substr(range.find('=') + 1);
			pos = std::atoll(posv.data());
			return true;
		}
		bool is_keep_alive() {
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
	public:
		class session& create_session() {
			return create_session("XFINAL");
		}
		class session& create_session(std::string const& name) {
			session_ = std::make_shared<class session>(false);
			session_->set_id(uuids::uuid_system_generator{}().to_short_str());
			//session_->set_expires(600);
			session_->get_cookie().set_name(name);
			session_manager::get().add_session(session_->get_id(), session_);
			return *session_;
		}
		class session& session(std::string const& name) {
			auto cookies_value = header("cookie");
			auto name_view = nonstd::string_view{ name.data(),name.size() };
			auto it = cookies_value.find(name_view);
			if (it == (nonstd::string_view::size_type)nonstd::string_view::npos) {
				session_ = session_manager::get().empty_session();
			}
			else {
				auto pos = it + name.size() + 1;
				auto id = cookies_value.substr(it + name.size() + 1, cookies_value.find('\"', pos) - pos);
				if (id.empty()) {
					session_ = session_manager::get().empty_session();
				}
				else {
					session_manager::get().validata(view2str(id));
					session_ = session_manager::get().get_session(view2str(id));
				}
			}
			return *session_;
		}
		class session& session() {
			return session("XFINAL");
		}
	protected:
		void init_content_type() noexcept {
			auto it = headers_->find("content-type");
			if (it != headers_->end()) {
				auto& value = it->second;
				auto view = nonstd::string_view(value.data(), value.size());
				auto has_op = view.find(';');
				if (has_op == (nonstd::string_view::size_type)nonstd::string_view::npos) {
					auto key = view2str(view);
					if (content_type_str_type_map.count(key) != 0) {
						content_type_ = content_type_str_type_map[key];
					}
					else {
						content_type_ = content_type::string;
					}
				}
				else {
					auto value_view = view.substr(0, has_op);
					auto key = view2str(value_view);
					if (content_type_str_type_map.count(key) != 0) {
						content_type_ = content_type_str_type_map[key];
					}
					else {
						content_type_ = content_type::string;
					}
				}
			}
			else {
				content_type_ = content_type::unknow;
			}
			if (content_type_ == content_type::multipart_form) {
				auto const& v = it->second;
				auto f = v.find("boundary");
				if (f != std::string::npos) {
					auto pos = f + sizeof("boundary");
					boundary_key_ = nonstd::string_view{ &(v[pos]),(v.size() - pos) };
				}
				else {
					boundary_key_ = "";
				}
			}
		}
	private:
		void reset() {
			method_ = "";
			url_ = "";
			version_ = "";
			headers_ = nullptr;
			header_length_ = 0;
			form_map_.clear();
			body_ = "";
			gbk_decode_form_map_.clear();
			decode_url_params_.clear();
			url_params_.clear();
			gbk_decode_params_.clear();
			boundary_key_ = "";
			multipart_form_map_ = nullptr;
			multipart_files_map_ = nullptr;
			oct_steam_ = nullptr;
			is_generic_ = false;
			generic_base_path_ = "";
			session_.reset();
		}
		void set_generic_path(std::string const& path) {
			is_generic_ = true;
			generic_base_path_ = path;
		}
	private:
		nonstd::string_view method_;
		nonstd::string_view url_;
		nonstd::string_view version_;
		std::map<std::string, std::string> const* headers_ = nullptr;
		std::size_t header_length_;
		enum content_type content_type_;
		std::map<nonstd::string_view, nonstd::string_view> form_map_;
		nonstd::string_view body_;
		std::map<std::string, std::string> gbk_decode_form_map_;
		std::string decode_url_params_;
		std::map<nonstd::string_view, nonstd::string_view> url_params_;
		std::map<std::string, std::string> gbk_decode_params_;
		nonstd::string_view boundary_key_;
		std::map<std::string, std::string> const* multipart_form_map_ = nullptr;
		std::map<std::string, xfinal::filewriter> const* multipart_files_map_ = nullptr;
		xfinal::filewriter* oct_steam_ = nullptr;
		class connection* connecter_;
		response& res_;
		std::shared_ptr<class session> session_;
		bool is_generic_ = false;
		std::string generic_base_path_;
	};
	///响应
	class response :private nocopyable {
		friend class connection;
		friend class http_router;
		friend class request;
	protected:
		enum class write_type {
			string,
			file,
			no_body
		};
	public:
		response(request& req, class connection* connect_) :connecter_(connect_), req_(req), view_env_(std::make_unique<inja::Environment>()) {
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
			add_header("Content-Type", conent_type);
			is_chunked_ = is_chunked;
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
			if (!filename.empty()) {
				bool b = file_.open(filename);
				if (!b) {
					write_string("", false, http_status::bad_request);
				}
				else {
					add_header("Content-Type", view2str(file_.content_type()));
					write_type_ = write_type::file;
					is_chunked_ = is_chunked;
					if (!is_chunked) {
						state_ = http_status::ok;
						file_.read_all(body_);
						init_start_pos_ = -1;
					}
					else {
						add_header("Transfer-Encoding", "chunked");
						body_.resize((std::size_t)chunked_size_);
						std::uint64_t pos = 0;
						if (req_.accept_range(pos)) {
							state_ = http_status::partial_content;
							init_start_pos_ = pos;
							auto filesize = file_.size();
							auto rang_value = std::string("bytes ") + std::to_string(init_start_pos_) + std::string("-") + std::to_string(filesize - 1) + "/" + std::to_string(filesize);
							add_header("Content-Range", rang_value);
						}
						else {
							state_ = http_status::ok;
							init_start_pos_ = -1;
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

		void write_view(std::string const& filename, bool is_chunked = false, http_status state = http_status::ok) noexcept {
			std::string extension = "text/plain";
			auto path = fs::path(filename);
			if (path.has_extension()) {
				extension = view2str(get_content_type(path.extension().string()));
			}
			try {
				write_string(view_env_->render_file(filename, view_data_), is_chunked, state, extension);
			}
			catch (std::runtime_error const& ec) {
				write_string(ec.what(), false, http_status::bad_request);
			}
		}

		void write_view(std::string const& filename, json const& json, bool is_chunked = false, http_status state = http_status::ok) noexcept {
			view_data_ = json;
			write_view(filename, is_chunked, state);
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
	public:
		inja::Environment& view_environment() {
			return *view_env_;
		}

		template<typename T, typename U = std::enable_if_t<!std::is_same<std::decay_t<T>, char const*>::value>>
		void set_attr(std::string const& name, T&& value) noexcept {
			view_data_[name] = std::forward<T>(value);
		}

		void set_attr(std::string const& name, std::string const& value) noexcept {
			view_data_[name] = value;
		}

		template<typename T>
		T get_attr(std::string const& name) noexcept {
			return view_data_[name].get<T>();
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
					req_.session_->save(session_manager::get().get_storage()); //保存到存储介质
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

		std::tuple<bool, std::vector<asio::const_buffer>, std::int64_t> chunked_body(std::int64_t startpos) noexcept {
			std::vector<asio::const_buffer> buffers;
			switch (write_type_) {
			case write_type::string: //如果是文本数据
			{
				if ((body_.size() - (std::size_t)startpos) <= chunked_size_) {
					auto size = body_.size() - startpos;
					chunked_write_size_ = to_hex(size);
					buffers.emplace_back(asio::buffer(chunked_write_size_));
					buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
					buffers.emplace_back(asio::buffer(&body_[(std::size_t)startpos], (std::size_t)size));
					buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
					return { true, buffers,0 };
				}
				else {  //还有超过每次chunked传输的数据大小
					auto nstart = startpos + chunked_size_;
					chunked_write_size_ = to_hex(chunked_size_);
					buffers.emplace_back(asio::buffer(chunked_write_size_));
					buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
					buffers.emplace_back(asio::buffer(&body_[(std::size_t)startpos], (std::size_t)chunked_size_));
					buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
					return { false,buffers,nstart };
				}
			}
			break;
			case write_type::file:  //如果是文件数据
			{
				auto read_size = file_.read(startpos, &(body_[0]), chunked_size_);
				chunked_write_size_ = to_hex(read_size);
				buffers.emplace_back(asio::buffer(chunked_write_size_));
				buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
				buffers.emplace_back(asio::buffer(&body_[0], (std::size_t)read_size));
				buffers.emplace_back(asio::buffer(crlf.data(), crlf.size()));
				return { (read_size < chunked_size_),buffers, -1 };
			}
			break;
			default:
				return { true,buffers ,0 };
				break;
			}
		}
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
		}
	private:
		class connection* connecter_;
		request& req_;
		std::unordered_multimap<std::string, std::string> header_map_;
		std::string body_;
		http_status state_;
		bool is_chunked_ = false;
		std::string http_version_;
		write_type write_type_ = write_type::string;
		std::uint64_t chunked_size_;
		std::string chunked_write_size_;
		filereader file_;
		std::int64_t init_start_pos_;
		std::unique_ptr<inja::Environment> view_env_;
		json view_data_;
	};

	class Controller :private nocopyable {
		friend class http_router;
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