#ifndef __HTTPS_CLIENT_H
#define __HTTPS_CLIENT_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "Debug.hpp"
#include "Singleton.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using std::string;
using std::cout;
using std::endl;

class ClientInfo : public Singleton<ClientInfo> {
    std::string     certFile;
    std::string     keyFile;
    std::string     pemFile;
    public:
    void setCertFile(std::string certFile) {
        this->certFile = certFile;
    }
    std::string getCertFile() {
        return this->certFile;
    }
    void setKeyFile(std::string keyFile) {
        this->keyFile = keyFile;
    }
    std::string getKeyFile() {
        return this->keyFile;
    }
    void setPemFile(std::string pemFile) {
        this->pemFile = pemFile;
    }
    std::string getPemFile() {
        return this->pemFile;
    }
};

// Report a failure
// Performs an HTTP GET and prints the response
class ClientSession : public std::enable_shared_from_this<ClientSession> {
	tcp::resolver resolver;
	beast::ssl_stream<beast::tcp_stream> stream;
	beast::flat_buffer buffer; // (Must persist between reads)
	http::request<http::string_body> req;
	http::response<http::string_body> res;

    bool isSuccess;

	public:
	// Objects are constructed with a strand to
	// ensure that handlers do not execute concurrently.
	explicit
		ClientSession(net::io_context& ioc, ssl::context& ctx) : resolver(net::make_strand(ioc))
			, stream(net::make_strand(ioc), ctx) { this->isSuccess = true; }

    string getResponseBody();

    void fail(beast::error_code ec, char const* what) {
        LOG_ERROR(std::string(what) + ":" + ec.message());
        this->isSuccess = false;
    }

    // Start the asynchronous operation
    void run(char const* host, char const* port, http::verb method, char const* target, int version, string& body);
    void onResolve(beast::error_code ec, tcp::resolver::results_type results);
    void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type);
    void onHandshake(beast::error_code ec);
    void onWrite(beast::error_code ec, std::size_t bytes_transferred);
    void onRead(beast::error_code ec, std::size_t bytes_transferred);
    void onShutdown(beast::error_code ec);
    bool getIsSuccess() { return this->isSuccess; }
};


class HttpsClient {
    private:
        net::io_context			    ioc;
        ssl::context            	ctx;

    public:
        HttpsClient():ctx(ssl::context::tls) {}
        bool setInitSSLContext(string certFile, string keyFile, string dhFile);
        bool sendRequest(string & result, string address, string port, http::verb method, string uri, string& body);
};






























#endif
