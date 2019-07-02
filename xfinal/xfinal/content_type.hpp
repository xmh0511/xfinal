#pragma once
#include <map>
#include <string>
namespace xfinal {
	enum class content_type {
		url_encode_form,
		multipart_form,
		octet_stream,
		string,
		unknow
	};
	static std::map<std::string, enum xfinal::content_type> content_type_str_type_map = {
		{"application/x-www-form-urlencoded",content_type::url_encode_form},
	    {"application/octet-stream",content_type::octet_stream},
	    {"multipart/form-data",content_type::multipart_form }
	};
}