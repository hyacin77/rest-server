#ifndef __HTTPS_SERVER_H
#define __HTTPS_SERVER_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

#include "Debug.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using namespace std;

#define PUT     0
#define POST    1
#define GET     2
#define DELETE  3

#define METHOD_ARRAY_SIZE 4

typedef bool (*handler)(beast::string_view body, vector<string> params, class Session &session);
typedef struct funcinfo_t {
    public:
        handler  fn;
        vector<string> params;
} funcinfo;

class Vertex {
    string     name;
    unordered_map<string, Vertex*> adjacents;
    handler  fnList[METHOD_ARRAY_SIZE];
    public:
    Vertex(string _name): name(_name) {
        for(int i = 0; i < METHOD_ARRAY_SIZE; i++) {
            this->fnList[i] = nullptr;
        }
    }
    string getName() {
        return this->name;
    }
    unordered_map<string, Vertex*> & getAdjcents() {
        return this->adjacents;
    }
    void setFunction(int method, handler fn);
    handler getFunction(int method);
};

class UriRoute {
    unordered_map<string, Vertex*>     vertices;

    public:
    Vertex * getVertex(unordered_map<string, Vertex*> &adjacents, string name);
    Vertex * searchVertex(unordered_map<string, Vertex*> &adjacents, string name);
    vector<string> splitUri(string uri);
    bool addRoute(int method, string uri, handler fn);
    std::shared_ptr<funcinfo> getFunction(int method, string uri);
};

class Session : public std::enable_shared_from_this<Session> {
	private:
	beast::ssl_stream<beast::tcp_stream> 	stream;
	beast::flat_buffer 			            buffer;
	http::request<http::string_body> 	    req;
	std::shared_ptr<void> 			        res;
    UriRoute&                               route;

	public:
	//move: 
	explicit Session(tcp::socket&& socket, ssl::context& ctx, UriRoute& _route) : 
		stream(std::move(socket), ctx), route(_route) {
	};
	
	void getHandler(int method, string uri) {
        this->route.getFunction(method, uri);
	};

	void run();
	void onRun();
	void onHandshake(beast::error_code ec);
	void doRead();
	string getFieldValue(string field);
    int getMethodNum();
	void onRead(beast::error_code ec, std::size_t bytes_transferred);
	void sendErrorResponse(http::status status);
    void sendErrorResponse(http::status status, std::string message);
	void sendResponse(string body);
	http::response<http::string_body> createResponse();
	void doWrite(http::response<http::string_body> res);
	void onWrite(bool close, beast::error_code ec, std::size_t bytes_transferred);
	void doClose();
	void onShutdown(beast::error_code ec);
};

class Listener : public std::enable_shared_from_this<Listener> {
	private:
	net::io_context &ioc;
	ssl::context 	&ctx;
	tcp::acceptor	acceptor;
	tcp::endpoint	endpoint;
    UriRoute       &route;

	public:
	Listener(net::io_context &_ioc, ssl::context &_ctx, tcp::endpoint _endpoint, UriRoute& _route):ioc(_ioc), ctx(_ctx), acceptor(_ioc), endpoint(_endpoint), route(_route) {
	};

	bool initialize();
	void run();
	void doAccept();
	void onAccept(beast::error_code ec, tcp::socket socket);
};

class HttpsServer : public std::enable_shared_from_this<HttpsServer> {
    private:
        net::io_context			    ioc;
        shared_ptr<ssl::context>	ctx;
        shared_ptr<Listener>		listener;
        int				            maxThread;
        vector<thread*>			    *threads;

    public:
        HttpsServer(string address, unsigned short port, int _maxThread, UriRoute& route);
        //의미 없는 코드일 수 있음
        ~HttpsServer();
        bool setInitSSLContext(string certFile, string keyFile, string dhFile);
        bool run();
};

































#endif
