#pragma once
#include "utils.hpp"
#include <string>
#include "json.hpp"
#include <ctime>
#include <mutex>
#include <unordered_map>
#include "uuid.hpp"
#include <fstream>
#include "ghc/filesystem.hpp"
#include <memory>
#include "code_parser.hpp"
#include "files.hpp"
namespace fs = ghc::filesystem;
namespace xfinal {
	class session;
	class session_storage {
	public:
		virtual bool init() = 0;
		virtual void save(session& session) = 0;
		virtual void remove(session& session) = 0;
	};
	template<typename Session>
	class cookie final :private nocopyable {
	public:
		cookie(std::string& uuid, std::time_t const& expires, Session& session):uuid_(&uuid), expires_(&expires), session_(session){

		}
	public:
		void set_name(std::string const& name) {
			session_.set_cookie_update(true);
			name_ = name;
		}
		std::string name() {
			return name_;
		}
		std::string str() {
			std::string content;
			content = content.append(name_).append("=").append(*uuid_);
			if (!session_.is_temp()) {
				content = content.append("; ").append("Expires=").append(get_gmt_time_str(*expires_));
			}
			if (!domain_.empty()) {
				content = content.append("; ").append("domain=").append(domain_);
			}
			if (!path_.empty()) {
				content = content.append("; ").append("path=").append(path_);
			}
			if (http_only_) {
				content = content.append("; HttpOnly");
			}
			return content;
		}
		void set_http_only(bool is_http_only) {
			session_.set_cookie_update(true);
			http_only_ = is_http_only;
		}
		bool http_only() {
			return http_only_;
		}
		void set_path(std::string const& path) {
			session_.set_cookie_update(true);
			path_ = path;
		}
		std::string path() {
			return path_;
		}
		void set_domain(std::string const& domain) {
			session_.set_cookie_update(true);
			domain_ = domain;
		}
		std::string domain() {
			return domain_;
		}
	private:
		std::string name_;
		std::string const* uuid_;//point to session uuid;
		std::time_t const* expires_;//point to session expires;
		std::string domain_;
		std::string path_;
		bool http_only_ = false;
		Session& session_;
	};

	class session final:private nocopyable {  //session 模块
	public:
		session(bool is_empty = false):is_empty_(is_empty), cookie_(uuid_, expires_,*this), data_(json::object()){
			expires_ = std::time(nullptr) + 600;
		}
	public:
		void set_id(std::string const& id) {
			std::unique_lock<std::mutex> lock(mutex_);
			is_update_ = true;
			uuid_ = id;
		}
		std::string get_id() {
			std::unique_lock<std::mutex> lock(mutex_);
			return uuid_;
		}
		template<typename T>
		void set_data(std::string const& name,T&& value) {
			std::unique_lock<std::mutex> lock(mutex_);
			data_update_ = true;
			data_[name] = std::forward<T>(value);
		}
		template<typename T>
		std::remove_reference_t<T> get_data(std::string const& name) {
			std::unique_lock<std::mutex> lock(mutex_);
			if (data_[name].is_null()) {
				return T{};
			}
			return data_[name].get<T>();
		}
		void set_expires(std::time_t seconds) {
			std::unique_lock<std::mutex> lock(mutex_);
			is_temp_ = false;
			is_update_ = true;
			expires_ = std::time(nullptr) + seconds;
		}
		std::time_t get_expires() {
			std::unique_lock<std::mutex> lock(mutex_);
			return expires_;
		}
		void add_expires(std::time_t seconds) {
			std::unique_lock<std::mutex> lock(mutex_);
			if (is_temp_) {
				expires_ = 0;
			}
			is_temp_ = false;
			is_update_ = true;
			expires_ += seconds;
		}
		void sub_expires(std::time_t seconds) {
			std::unique_lock<std::mutex> lock(mutex_);
			if (is_temp_) {
				expires_ = 0;
			}
			is_temp_ = false;
			is_update_ = true;
			expires_ -= seconds;
		}
		cookie<session>& get_cookie() {
			return cookie_;
		}

		std::string cookie_str() {
			std::unique_lock<std::mutex> lock(mutex_);
			return cookie_.str();
		}

		void save(session_storage& storage) {  //保存session 到存储介质的接口
			storage.save(*this);
		}

		void remove(session_storage& storage) {  //从存储介质删除sesssion 的接口
			storage.remove(*this);
		}
		std::string serialize() {
			json ss;
			std::unique_lock<std::mutex> lock(mutex_);
			ss["expires"] = expires_;
			ss["data"] = data_;
			ss["id"] = uuid_;
			ss["cookie_name"] = cookie_.name();
			ss["http_only"] = cookie_.http_only();
			ss["path"] = cookie_.path();
			ss["domain"] = cookie_.domain();
			return ss.dump();
		}
		bool empty() {
			return is_empty_;
		}
		void set_cookie_update(bool update) {
			std::unique_lock<std::mutex> lock(mutex_);
			is_update_ = update;
		}
		bool cookie_update() {
			std::unique_lock<std::mutex> lock(mutex_);
			return is_update_;
		}
		void set_data_update(bool update) {
			std::unique_lock<std::mutex> lock(mutex_);
			data_update_ = update;
		}
		bool data_update() {
			std::unique_lock<std::mutex> lock(mutex_);
			return data_update_;
		}
		std::mutex& get_mutex() {
			return mutex_;
		}
		void replace_expires(std::time_t time) {
			std::unique_lock<std::mutex> lock(mutex_);
			expires_ = time;
		}
		void replace_data(json data) {
			std::unique_lock<std::mutex> lock(mutex_);
			data_ = data;
		}
		bool is_temp() {
			return is_temp_;
		}
	private:
		std::string uuid_;
		std::time_t expires_;//生命周期 时间戳 精确到秒
		bool is_empty_;
		cookie<session> cookie_;
		json data_;
		std::mutex mutex_;
		bool is_temp_ = true;
		bool is_update_ = true;
		bool data_update_ = true;
	};

	class session_manager final :private nocopyable {
	private:
		session_manager():empty_session_(std::make_shared<session>(true)){
		}
	public:
		static session_manager& get() {
			static session_manager manager_;
			return manager_;
		}
	public:
		void set_storage(std::unique_ptr<session_storage> storage) {
			storage_ = std::move(storage);
		}
		session_storage& get_storage() {
			return *storage_;
		}
	public:
		std::shared_ptr<session> get_session(std::string const& id) {
			std::unique_lock<std::mutex> lock(mutex_);
			auto it = session_map_.find(id);
			if (it != session_map_.end()) {
				return it->second;
			}
			return empty_session_;
		}
		void add_session(std::string const& id, std::weak_ptr<session> session) {
			std::unique_lock<std::mutex> lock(mutex_);
			session_map_[id] = session.lock();
		}
		auto remove_session(std::string const& id) {
			std::unique_lock<std::mutex> lock(mutex_);
			auto it = session_map_.find(id);
			if (it != session_map_.end()) {
				(it->second)->remove(*storage_);
				it = session_map_.erase(it);
			}
			return it;
		}
		std::shared_ptr<session> empty_session() {
			return empty_session_;
		}
		void validata(std::string const& id) {
			auto session = get_session(id);
			if (!session->empty()) {
				auto expires = session->get_expires();
				if (expires <= std::time(nullptr)) {
					remove_session(id);
				}
			}
		}
		void check_expires() {
			std::unique_lock<std::mutex> lock(mutex_);
			for (auto iter = session_map_.begin(); iter != session_map_.end();) {
				auto expires = iter->second->get_expires();
				if (expires <= std::time(nullptr)) {
					iter->second->remove(*storage_);
					iter = session_map_.erase(iter);
				}
				else {
					++iter;
				}
			}
		}
	private:
		std::unordered_map<std::string, std::shared_ptr<session>> session_map_;
		std::mutex mutex_;
		std::shared_ptr<session> empty_session_;
		std::unique_ptr<session_storage> storage_ = nullptr;
	};

	class default_session_storage:public session_storage {
	public:
		default_session_storage() = default;
	public:
		bool init() {
			if (!fs::exists(save_dir_)) {
				fs::create_directory(save_dir_);
			}
			auto iter = fs::directory_iterator(save_dir_);
			for (; iter != fs::directory_iterator(); ++iter) {
				if (!fs::is_directory(iter->path())) {  //session 文件
					filereader reader;
					reader.open(iter->path().string());
					std::string buffer;
					reader.read_all(buffer);
					auto data = json::parse(buffer);
					auto session = std::make_shared<class session>();
					if (!data.is_null() && !data["expires"].is_null() && !data["id"].is_null() && !data["cookie_name"].is_null() && !data["path"].is_null() && !data["domain"].is_null() && !data["http_only"].is_null()) {
						session->set_id(data["id"].get<std::string>());
						session->replace_expires(data["expires"].get<std::time_t>());
						session->get_cookie().set_name(data["cookie_name"].get<std::string>());
						session->get_cookie().set_path(data["path"].get<std::string>());
						session->get_cookie().set_domain(data["domain"].get<std::string>());
						session->get_cookie().set_http_only(data["http_only"].get<bool>());
						session->set_cookie_update(false);
						session->set_data_update(false);
						if (!data["data"].is_null()) {
							session->replace_data(data["data"]);
						}
						session_manager::get().add_session(session->get_id(), session);
					}
				}
			}
			return true;
		}
		void save(session& session) {
			auto context = session.serialize();
			auto filepath = save_dir_ + "/" + session.get_id();
			std::ofstream file(filepath, std::ios::binary);
			if (file.is_open()) {
				file << context;
			}
		}
		void remove(session& session) {
			auto filepath = save_dir_ +"/" + session.get_id();
			fs::remove(filepath);
		}
	public:
		std::string save_dir_ = "./session";
	};
}