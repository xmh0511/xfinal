#pragma once
#include "string_view.hpp"
#include <asio.hpp>
namespace xfinal {
	enum class http_status {
		init,
		switching_protocols = 101,
		ok = 200,
		created = 201,
		accepted = 202,
		no_content = 204,
		partial_content = 206,
		multiple_choices = 300,
		moved_permanently = 301,
		moved_temporarily = 302,
		not_modified = 304,
		bad_request = 400,
		unauthorized = 401,
		forbidden = 403,
		not_found = 404,
		internal_server_error = 500,
		not_implemented = 501,
		bad_gateway = 502,
		service_unavailable = 503
	};
	static const nonstd::string_view name_value_separator = ": ";
	static const nonstd::string_view crlf = "\r\n";

	static const nonstd::string_view switching_protocols = "101 Switching Protocals\r\n";
	static const nonstd::string_view rep_ok = "200 OK\r\n";
	static const nonstd::string_view rep_created = "201 Created\r\n";
	static const nonstd::string_view rep_accepted = "202 Accepted\r\n";
	static const nonstd::string_view rep_no_content = "204 No Content\r\n";
	static const nonstd::string_view rep_partial_content = "206 Partial Content\r\n";
	static const nonstd::string_view rep_multiple_choices = "300 Multiple Choices\r\n";
	static const nonstd::string_view rep_moved_permanently = "301 Moved Permanently\r\n";
	static const nonstd::string_view rep_moved_temporarily = "302 Moved Temporarily\r\n";
	static const nonstd::string_view rep_not_modified = "304 Not Modified\r\n";
	static const nonstd::string_view rep_bad_request = "400 Bad Request\r\n";
	static const nonstd::string_view rep_unauthorized = "401 Unauthorized\r\n";
	static const nonstd::string_view rep_forbidden = "403 Forbidden\r\n";
	static const nonstd::string_view rep_not_found = "404 Not Found\r\n";
	static const nonstd::string_view rep_internal_server_error = "500 Internal Server Error\r\n";
	static const nonstd::string_view rep_not_implemented = "501 Not Implemented\r\n";
	static const nonstd::string_view rep_bad_gateway = "502 Bad Gateway\r\n";
	static const nonstd::string_view rep_service_unavailable = "503 Service Unavailable\r\n";

	inline asio::const_buffer http_state_to_buffer(http_status status) {
		switch (status) {
		case http_status::switching_protocols:
			return asio::buffer(switching_protocols.data(), switching_protocols.length());
		case http_status::ok:
			return asio::buffer(rep_ok.data(), rep_ok.length());
		case http_status::created:
			return asio::buffer(rep_created.data(), rep_created.length());
		case http_status::accepted:
			return asio::buffer(rep_accepted.data(), rep_created.length());
		case http_status::no_content:
			return asio::buffer(rep_no_content.data(), rep_no_content.length());
		case http_status::partial_content:
			return asio::buffer(rep_partial_content.data(), rep_partial_content.length());
		case http_status::multiple_choices:
			return asio::buffer(rep_multiple_choices.data(), rep_multiple_choices.length());
		case http_status::moved_permanently:
			return asio::buffer(rep_moved_permanently.data(), rep_moved_permanently.length());
		case http_status::moved_temporarily:
			return asio::buffer(rep_moved_temporarily.data(), rep_moved_temporarily.length());
		case http_status::not_modified:
			return asio::buffer(rep_not_modified.data(), rep_not_modified.length());
		case http_status::bad_request:
			return asio::buffer(rep_bad_request.data(), rep_bad_request.length());
		case http_status::unauthorized:
			return asio::buffer(rep_unauthorized.data(), rep_unauthorized.length());
		case http_status::forbidden:
			return asio::buffer(rep_forbidden.data(), rep_forbidden.length());
		case http_status::not_found:
			return asio::buffer(rep_not_found.data(), rep_not_found.length());
		case http_status::internal_server_error:
			return asio::buffer(rep_internal_server_error.data(), rep_internal_server_error.length());
		case http_status::not_implemented:
			return asio::buffer(rep_not_implemented.data(), rep_not_implemented.length());
		case http_status::bad_gateway:
			return asio::buffer(rep_bad_gateway.data(), rep_bad_gateway.length());
		case http_status::service_unavailable:
			return asio::buffer(rep_service_unavailable.data(), rep_service_unavailable.length());
		default:
			return asio::buffer(rep_internal_server_error.data(), rep_internal_server_error.length());
		}
	}
}
