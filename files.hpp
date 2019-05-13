#pragma once
#include <fstream>
#include <memory>
namespace xfinal {
	class file {
	public:
		file() = default;
	public:
		bool open(std::string const&  path) {
			if (file_handle_ == nullptr) {
				file_handle_ = std::make_shared<std::ofstream>(path, std::ios::app | std::ios::binary);
				return true;
			}
			return false;
		}
		void add_data(std::string&& data) {
			if (file_handle_ != nullptr) {
				file_handle_->write(data.data(), data.size());
			}
		}
	private:
		std::shared_ptr<std::ofstream> file_handle_;
	};
}