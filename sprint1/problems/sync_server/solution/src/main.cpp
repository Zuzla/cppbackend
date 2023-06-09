// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <thread>
#include <optional>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;

struct ContentType
{
	ContentType() = delete;
	constexpr static std::string_view TEXT_HTML = "text/html"sv;
};

struct Allow
{
	Allow() = delete;
	constexpr static std::string_view TEXT_HTML = "GET, HEAD"sv;
};

void DumpRequest(const StringRequest& req)
{
	std::cout << req.method_string() << ' ' << req.target() << std::endl;

	for(const auto& header : req)
	{
		std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
	}
}

std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer)
{
	beast::error_code ec;
	StringRequest req;

	http::read(socket, buffer, req, ec);	

	if (ec == http::error::end_of_stream)
	{
		return std::nullopt;
	}

	if (ec)
	{
		throw std::runtime_error("Failed o read reques:"s.append(ec.message()));
	}

	return req;
    

}

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive,
								  std::string_view allow = Allow::TEXT_HTML, std::string_view content_type = ContentType::TEXT_HTML) 
{
	StringResponse response(status, http_version);
	response.set(http::field::content_type, content_type);

	if (status == http::status::method_not_allowed)
		response.set(http::field::allow, allow);
	
	response.body() = body;
	response.content_length(body.size());
	response.keep_alive(keep_alive);
	return response;
}

StringResponse HandleRequest(StringRequest&& req)
{
	const auto text_response = [&req](http::status status, std::string_view text)
	{
		return MakeStringResponse(status, text, req.version(), req.keep_alive());
	};
    
    switch (req.method())
    {
    case http::verb::get:
	{
		auto target = std::move(req.target());
		target.remove_prefix(1);
        return text_response(http::status::ok, "Hello, "s.append(target));
	}
        break;
    case http::verb::head:
        return text_response(http::status::ok, ""sv);
        break;
    default:
        return text_response(http::status::method_not_allowed, "Invalid method"sv);
        break;
    }
}

template <typename RequestHandle>
void HandleConnection(tcp::socket& socket, RequestHandle&& handle_request)
{
	try{
		beast::flat_buffer buffer;

		while (auto request = ReadRequest(socket, buffer))
		{
			// DumpRequest(*request);

			StringResponse response = handle_request(*std::move(request));

			http::write(socket, response);

			if (response.need_eof())
				break;

		}
	}catch(std::exception& ex)
	{
		std::cout << "Exception - " << ex.what() << std::endl;
	}

	beast::error_code ec;

	socket.shutdown(tcp::socket::shutdown_send, ec);
}

int main() {
    net::io_context io;

    const auto ip = net::ip::make_address("0.0.0.0");
    const auto port = 8080;

    tcp::acceptor acceptor(io, {ip, port});

    std::cout << "Server has started..."sv << std::endl;

    while(1)
    {
        tcp::socket socket{io};
        acceptor.accept(socket);

        HandleConnection(socket, HandleRequest);
    }
}
