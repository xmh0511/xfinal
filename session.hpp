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
namespace fs = ghc::filesystem;
namespace xfinal {
	class session;
	class session_storage {
	public:
		virtual void save(session& session) = 0;
		virtual void remove(session& session) = 0;
	};
	class cookie final :private nocopyable {
	public:
		cookie(std::string& uuid, std::time_t const& expires):uuid_(&uuid), expires_(&expires){

		}
	public:
		void set_name(std::string const& name) {
			name_ = name;
		}
		std::string get_name() {
			return name_;
		}
	private:
		std::string name_;
		std::string const* uuid_;//point to session uuid;
		std::time_t const* expires_;//point to session expires;
	};

	class session final:private nocopyable {  //session 模块
	public:
		session():expires_(std::time(nullptr)), cookie_(uuid_, expires_){

		}
	public:
		void set_id(std::string const& id) {
			std::unique_lock<std::mutex> lock(mutex_);
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
		void get_data(std::string const& name) {
			std::unique_lock<std::mutex> lock(mutex_);
			return data_[name].get<T>();
		}
		void set_expires(std::time_t seconds) {
			std::unique_lock<std::mutex> lock(mutex_);
			expires_ = std::time(nullptr) + seconds;
		}
		std::time_t get_expires() {
			std::unique_lock<std::mutex> lock(mutex_);
			return expires_;
		}
		void add_expires(std::time_t seconds) {
			std::unique_lock<std::mutex> lock(mutex_);
			expires_ += seconds;
		}
		void sub_expires(std::time_t seconds) {
			std::unique_lock<std::mutex> lock(mutex_);
			expires_ -= seconds;
		}
		cookie& get_cookie() {
			return cookie_;
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
	private:
		std::string uuid_;
		std::time_t expires_;//生命周期 时间戳 精确到秒
		cookie cookie_;
		json data_;
		std::mutex mutex_;
	};

	class session_manager final :private nocopyable {
	private:
		session_manager() = default;
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