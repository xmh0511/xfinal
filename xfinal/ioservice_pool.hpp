#pragma once
#include <asio.hpp>
#include <thread>
#include <vector>
#include <memory>
#include "utils.hpp"
namespace xfinal {
	class ioservice_pool final :private nocopyable, public std::enable_shared_from_this<ioservice_pool> {
		friend class http_server;
	public:
		ioservice_pool(std::size_t size) :io_pool_(size), io_index_(0), thread_counts(size) {
			for (auto& iter : io_pool_) {
				io_workers_.emplace_back(std::unique_ptr<asio::io_service::work>(new asio::io_service::work(iter)));
			}
		}
		asio::io_service& get_io() {
			if (io_index_ >= io_pool_.size()) {
				io_index_ = 0;
			}
			return io_pool_[io_index_++];
		}
	protected:
		void run() {
			for (auto& iter : io_pool_) {
				thread_pool_.emplace_back(std::unique_ptr<std::thread>(new std::thread([](asio::io_service& io) {
					io.run();
				}, std::ref(iter))));
			}
			for (auto&iter : thread_pool_) {
				iter->join();
			}
		}
	private:
		std::vector<asio::io_service> io_pool_;
		std::vector<std::unique_ptr<asio::io_service::work>> io_workers_;
		std::vector<std::unique_ptr<std::thread>> thread_pool_;
		std::size_t io_index_;
		std::size_t thread_counts;
	};
}