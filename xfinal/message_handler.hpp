#pragma once
#include <functional>
#include <string>
namespace utils {
	class messageCenter {
	public:
		static messageCenter& get() {
			static messageCenter instance{};
			return instance;
		}
		void set_handler(std::function<void(std::string const& message)> const& func) {
			handler = func;
		}
		void trigger_message(std::string const& message) {
			if (handler) {
				handler(message);
			}
		}
	private:
		~messageCenter() {

		}
	private:
		std::function<void(std::string const& message)> handler;
	};
}