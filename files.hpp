#pragma once
#include <fstream>
#include <memory>
#include "string_view.hpp"
#include "utils.hpp"
namespace xfinal {
	class file final:private nocopyable {
	public:
		file() = default;
	public:
		bool open(std::string const&  path) {
			if (file_handle_ == nullptr) {
				file_handle_ = std::make_unique<std::ofstream>(path, std::ios::app | std::ios::binary);
				path_ = path;
				return file_handle_->is_open();
			}
			return false;
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
		void close() {
			file_handle_->close();
		}
	private:
		std::unique_ptr<std::ofstream> file_handle_;
		std::string original_name_;
		std::uint64_t size_;
		std::string path_;
	};
}