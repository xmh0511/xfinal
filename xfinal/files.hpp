#pragma once
#include <fstream>
#include <memory>
#include "string_view.hpp"
#include "utils.hpp"
#include "mime.hpp"
#include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;
namespace xfinal {
	class filewriter final :private nocopyable {
	public:
		filewriter() = default;
		filewriter(filewriter&& f) :file_handle_(std::move(f.file_handle_)), original_name_(std::move(f.original_name_)), size_(f.size_), path_(std::move(f.path_)) {

		}
		filewriter& operator=(filewriter&& f) {
			file_handle_ = std::move(f.file_handle_);
			original_name_ = std::move(f.original_name_);
			size_ = f.size_;
			path_ = std::move(f.path_);
			return *this;
		}
	public:
		bool open(std::string const& path) {
			if (file_handle_ == nullptr) {
				file_handle_ = std::move(std::unique_ptr<std::ofstream>(new std::ofstream(path, std::ios::app | std::ios::binary)));
				path_ = path;
				is_exsit_ = file_handle_->is_open();
				return is_exsit_;
			}
			return false;
		}
		bool is_exsit() {
			return is_exsit_;
		}
		void add_data(nonstd::string_view data) {
			if (file_handle_ != nullptr) {
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

		std::uint64_t size() const {
			return size_;
		}
		std::string path() const {
			return path_;
		}

		std::string url_path() const {
			return path_.substr(path_.find('.') + 1, path_.size());
		}

		void close() {
			if (file_handle_) {
				file_handle_->close();
				file_handle_.release();
			}
		}
	private:
		std::unique_ptr<std::ofstream> file_handle_;
		std::string original_name_;
		std::uint64_t size_ = 0;
		std::string path_;
		bool is_exsit_ = false;
	};

	class filereader final :private nocopyable {
	public:
		filereader() = default;
		filereader(filereader&& f) :file_handle_(std::move(f.file_handle_)), size_(f.size_), path_(std::move(f.path_)), content_type_(f.content_type_) {

		}

		filereader& operator=(filereader&& f) {
			file_handle_ = std::move(f.file_handle_);
			size_ = f.size_;
			path_ = std::move(f.path_);
			content_type_ = std::move(f.content_type_);
			return *this;
		}
	public:
		bool open(std::string const& filename) {
			if (file_handle_ == nullptr) {
				file_handle_ = std::move(std::unique_ptr<std::ifstream>(new std::ifstream(filename, std::ios::binary)));
			}
			bool b = file_handle_->is_open();
			is_exsit_ = b;
			if (b) {
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
		bool is_exsit() {
			return is_exsit_;
		}
		void close() {
			if (file_handle_) {
				file_handle_->close();
				file_handle_.release();
			}
		}
		std::uint64_t read(std::int64_t start, char* buffer, std::int64_t size) {
			if (file_handle_) {
				if (start != -1) {
					file_handle_->seekg(start, std::ios::beg);
				}
				file_handle_->read(buffer, size);
				return file_handle_->gcount();
			}
			return 0;
		}

		void read_all(std::string& content) {
			if (file_handle_) {
				std::stringstream ss;
				ss << file_handle_->rdbuf();
				content = ss.str();
			}
		}

		nonstd::string_view content_type() {
			return content_type_;
		}

		std::uint64_t size() {
			return size_;
		}
	private:
		std::unique_ptr<std::ifstream> file_handle_;
		std::uint64_t size_ = 0;
		std::string path_;
		nonstd::string_view content_type_;
		bool is_exsit_ = false;
	};
}