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
			name_ = name;
		}
		std::string name() {
			session_.set_cookie_update(true);
			return name_;
		}
		std::string str() {
			std::string content;
			content = content.append(name_).append("=").append(*uuid_);
			if ((*expires_) >= 0) {
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
		std::mutex& get_mutex() {
			return mutex_;
		}
	private:
		std::string uuid_;
		std::time_t expires_ = -1;//生命周期 时间戳 精确到秒
		bool is_empty_;
		cookie<session> cookie_;
		json data_;
		std::mutex mutex_;
		bool is_temp_ = true;
		bool is_update_ = true;
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
		void remove_session(std::string const& id) {
			std::unique_lock<std::mutex> lock(mutex_);
			auto it = session_map_.find(id);
			if (it != session_map_.end()) {
				(it->second)->remove(*storage_);
				session_map_.erase(it);
			}
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