#pragma once
#include <fstream>
#include <memory>
#include "string_view.hpp"
#include "utils.hpp"
#include "mime.hpp"
#include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;
namespace xfinal {
	//class filewriter final :private nocopyable {
	//public:
	//	filewriter() = default;
	//	filewriter(filewriter&& f) :file_handle_(std::move(f.file_handle_)), original_name_(std::move(f.original_name_)), size_(f.size_), path_(std::move(f.path_)) {

	//	}
	//	filewriter& operator=(filewriter&& f) {
	//		file_handle_ = std::move(f.file_handle_);
	//		original_name_ = std::move(f.original_name_);
	//		size_ = f.size_;
	//		path_ = std::move(f.path_);
	//		return *this;
	//	}
	//public:
	//	bool open(std::string const& path) {
	//		if (file_handle_ == nullptr) {
	//			file_handle_ = std::move(std::unique_ptr<std::ofstream>(new std::ofstream(path, std::ios::app | std::ios::binary)));
	//			path_ = path;
	//			is_exsit_ = file_handle_->is_open();
	//			return is_exsit_;
	//		}
	//		return false;
	//	}
	//	bool is_exsit() const {
	//		return is_exsit_;
	//	}
	//	void add_data(nonstd::string_view data) {
	//		if (file_handle_ != nullptr) {
	//			file_handle_->write(data.data(), data.size());
	//			size_ += data.size();
	//		}
	//	}
	//	void set_original_name(std::string&& name) {
	//		original_name_ = std::move(name);
	//	}
	//	std::string original_name() const {
	//		return original_name_;
	//	}

	//	std::uint64_t size() const {
	//		return size_;
	//	}
	//	std::string path() const {
	//		return path_;
	//	}

	//	std::string url_path() const {
	//		return path_.substr(path_.find('.') + 1, path_.size());
	//	}

	//	void close() const {
	//		if (file_handle_) {
	//			file_handle_->close();
	//			file_handle_.release();
	//		}
	//	}
	//	bool remove() const {
	//		close();
	//		fs::remove(path_);
	//		return true;
	//	}
	//private:
	//	mutable std::unique_ptr<std::ofstream> file_handle_;
	//	std::string original_name_;
	//	std::uint64_t size_ = 0;
	//	std::string path_;
	//	bool is_exsit_ = false;
	//};

	//class filereader final :private nocopyable {
	//public:
	//	filereader() = default;
	//	filereader(filereader&& f) :file_handle_(std::move(f.file_handle_)), size_(f.size_), path_(std::move(f.path_)), content_type_(f.content_type_) {

	//	}

	//	filereader& operator=(filereader&& f) {
	//		file_handle_ = std::move(f.file_handle_);
	//		size_ = f.size_;
	//		path_ = std::move(f.path_);
	//		content_type_ = std::move(f.content_type_);
	//		return *this;
	//	}
	//public:
	//	bool open(std::string const& filename) {
	//		if (file_handle_ == nullptr) {
	//			file_handle_ = std::move(std::unique_ptr<std::ifstream>(new std::ifstream(filename, std::ios::binary)));
	//		}
	//		bool b = file_handle_->is_open();
	//		is_exsit_ = b;
	//		if (b) {
	//			auto begin = file_handle_->tellg();
	//			file_handle_->seekg(0, std::ios::end);
	//			size_ = file_handle_->tellg();
	//			file_handle_->seekg(begin);
	//			auto pt = fs::path(filename);
	//			if (pt.has_extension()) {
	//				content_type_ = get_content_type(pt.extension());
	//			}
	//			else {
	//				content_type_ = unknow_file;
	//			}
	//		}
	//		return b;
	//	}
	//	bool is_exsit() const {
	//		return is_exsit_;
	//	}
	//	void close() const {
	//		if (file_handle_) {
	//			file_handle_->close();
	//			file_handle_.release();
	//		}
	//	}
	//	std::uint64_t read(std::int64_t start, char* buffer, std::int64_t size) {
	//		if (file_handle_) {
	//			if (start != -1) {
	//				file_handle_->seekg(start, std::ios::beg);
	//			}
	//			file_handle_->read(buffer, size);
	//			return file_handle_->gcount();
	//		}
	//		return 0;
	//	}

	//	void read_all(std::string& content) {
	//		if (file_handle_) {
	//			std::stringstream ss;
	//			ss << file_handle_->rdbuf();
	//			content = ss.str();
	//		}
	//	}

	//	nonstd::string_view content_type() const {
	//		return content_type_;
	//	}

	//	std::uint64_t size() const {
	//		return size_;
	//	}
	//	bool remove() const {
	//		close();
	//		fs::remove(path_);
	//		return true;
	//	}
	//private:
	//	mutable std::unique_ptr<std::ifstream> file_handle_;
	//	std::uint64_t size_ = 0;
	//	std::string path_;
	//	nonstd::string_view content_type_;
	//	bool is_exsit_ = false;
	//};

	class XFile final :private nocopyable {
	public:
		XFile() = default;
		XFile(XFile&& f) noexcept :file_handle_(std::move(f.file_handle_)), size_(f.size_), path_(std::move(f.path_)), content_type_(f.content_type_), original_name_(std::move(f.original_name_)), is_exist_(f.is_exist_){

		}

		XFile& operator=(XFile&& f) noexcept {
			file_handle_ = std::move(f.file_handle_);
			size_ = f.size_;
			path_ = std::move(f.path_);
			content_type_ = std::move(f.content_type_);
			original_name_ = std::move(f.original_name_);
			is_exist_ = f.is_exist_;
			return *this;
		}
	public:
		bool open(std::string const& filename,bool create_file = false) {
			if (file_handle_ == nullptr) {
				if (create_file == true) {
					file_handle_ = std::move(std::unique_ptr<std::fstream>(new std::fstream(filename, std::ios::in | std::ios::out | std::ios::app | std::ios::binary)));
				}
				else {
					file_handle_ = std::move(std::unique_ptr<std::fstream>(new std::fstream(filename, std::ios::in | std::ios::out | std::ios::binary)));
				}
			}
			bool b = file_handle_->is_open();
			is_exist_ = b;
			if (b) {
				path_ = filename;
				auto begin = file_handle_->tellg();
				file_handle_->seekg(0, std::ios::end);
				size_ = file_handle_->tellg();
				file_handle_->seekg(begin);
				auto pt = fs::path(filename);
				if (pt.has_extension()) {
					content_type_ = get_content_type(pt.extension());
				}
				else {
					content_type_ = unknow_file;
				}
			}
			return b;
		}

		bool is_open() const {
			return (file_handle_ && file_handle_->is_open());
		}

		bool is_exist() const {
			return is_exist_;
		}

		std::string extension() const {
			return fs::path(path_).extension().u8string();
		}

		void close() const {
			if (file_handle_) {
				file_handle_->close();
				file_handle_ = nullptr;
			}
		}

		std::uint64_t read(std::int64_t start, char* buffer, std::int64_t size) const {
			if (file_handle_ && is_open()) {
				if (start != -1) {
					file_handle_->seekg(start, std::ios::beg);
				}
				file_handle_->read(buffer, size);
				return file_handle_->gcount();
			}
			return 0;
		}

		void read_all(std::string& content) const {
			if (file_handle_ && is_open()) {
				std::stringstream ss;
				ss << file_handle_->rdbuf();
				content = ss.str();
			}
		}

		nonstd::string_view content_type() const {
			return content_type_;
		}

		std::uint64_t size() const {
			return size_;
		}

		bool remove() const {
			close();
			fs::remove(path_);
			size_ = 0;
			path_.clear();
			content_type_ = "";
			is_exist_ = false;
			original_name_ = "";
			return true;
		}

		XFile move(std::string const& path) const {
			auto r = fs::copy_file(path_, path);
			XFile file;
			if (r) {
				file_handle_ = nullptr;
				fs::remove(path_);
				file.open(path);
				file.original_name_ = std::move(original_name_);
				file.content_type_ = content_type_;
				file.is_exist_ = is_exist_;
				file.size_ = size_;
				size_ = 0;
				path_.clear();
				content_type_ = "";
				is_exist_ = false;
			}
			return file;
		}

		XFile copy(std::string const& path) const {
			auto r = fs::copy_file(path_, path);
			XFile file;
			if (r) {
				file.open(path);
				file.original_name_ = original_name_;
				file.content_type_ = content_type_;
				file.is_exist_ = is_exist_;
				file.size_ = size_;
			}
			return file;
		}

		void add_data(nonstd::string_view data) {
			if (file_handle_ != nullptr && is_open()) {
				file_handle_->write(data.data(), data.size());
				size_ += data.size();
			}
		}

		void set_original_name(std::string&& name) {
			original_name_ = std::move(name);
		}
		std::string original_name() const {
			return original_name_;
		}

		std::string path() const {
			return path_;
		}

		std::string url_path() const {
			return path_.substr(path_.find('.') + 1, path_.size());
		}
	private:
		mutable std::unique_ptr<std::fstream> file_handle_;
		mutable std::uint64_t size_ = 0;
		mutable std::string path_;
		mutable nonstd::string_view content_type_;
		mutable std::string original_name_;
		mutable bool is_exist_ = false;
	};
}