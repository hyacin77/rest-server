#include <iostream>

#include "HttpsServer.hpp"


void printError(beast::error_code ec, char const* what) {
	if(ec == net::ssl::error::stream_truncated) return;
    LOG_ERROR(std::string(what) + ":" + ec.message());
}

string getMethodString(int method) {
    switch(method) {
        case 0: return "PUT";
        case 1: return "POST";
        case 2: return "GET";
        case 3: return "DELETE";
    }
    return "";
}

void Vertex::setFunction(int method, handler fn) {
    if(method < 0 || method > 3) {
        LOG_WARN("not supported method number[" + std::to_string(method) + "]");
        return;
    }
    LOG_DEBUG("add method number[" + std::to_string(method) + "] array size[" + std::to_string(sizeof(fnList)) + "]");

    this->fnList[method] = fn;
}

handler Vertex::getFunction(int method) {
    if(method < 0 || method > 3) {
        LOG_WARN("not supported method number[" + std::to_string(method) + "]");
        return nullptr;
    }
    LOG_DEBUG("get method number[" + std::to_string(method) + "] array size[" + std::to_string(sizeof(fnList)) + "]");
    return this->fnList[method];
}

Vertex * UriRoute::getVertex(unordered_map<string, Vertex*> &adjacents, string name) {
    Vertex *v = nullptr;
    auto it = adjacents.find(name);
    if(it == adjacents.end()) {
        v = new Vertex(name);
        adjacents.insert(std::make_pair(name, v));
    } else {
        v = it->second;
    }
    return v;
}

Vertex * UriRoute::searchVertex(unordered_map<string, Vertex*> &adjacents, string name) {
    auto it = adjacents.find(name);
    if(it != adjacents.end()) return it->second;
    return nullptr;
}

vector<string> UriRoute::splitUri(string uri) {
    vector<string> uriVec;
    std::stringstream ss(uri);
    string temp;
    while(getline(ss, temp, '/')) {
        uriVec.push_back(temp);
    }
    return uriVec;
}

bool UriRoute::addRoute(int method, string uri, handler fn) {
    vector<string> uriVec = this->splitUri(uri);

    if(uriVec.size() < 2 || uriVec[1].size() < 1) {
        return false;
    }
    Vertex *vertex = this->getVertex(this->vertices, uriVec[1]);
    string newRoute = vertex->getName();
    for(int i = 2; i < uriVec.size(); i++) {
        vertex = this->getVertex(vertex->getAdjcents(), uriVec[i]);
        newRoute += ("-" + vertex->getName());
    }
    vertex->setFunction(method, fn);
    LOG_INFO("RestAPI " + getMethodString(method) + " route[" + newRoute + "] has been added!!");
    return true;
}

std::shared_ptr<funcinfo> UriRoute::getFunction(int method, string uri) {
    vector<string> uriVec = this->splitUri(uri);

    int i = 1;
    if(uriVec.size() < 2 || uriVec[i].size() < 1) {
        return nullptr;
    }
    Vertex *temp = this->searchVertex(this->vertices, uriVec[i++]);
    Vertex *vertex = nullptr;
    if(!temp) return nullptr;
    for(; i < uriVec.size(); i++) {
        temp = this->searchVertex(temp->getAdjcents(), uriVec[i]);
        if(temp) vertex = temp; else break;
    }

    if(!vertex) return nullptr;
    std::shared_ptr<funcinfo> fi = std::make_shared<funcinfo>();
    fi->fn = vertex->getFunction(method);
    for( ; i < uriVec.size(); i++) {
        fi->params.push_back(uriVec[i]);
    }

    return fi;
}


void Session::run() {
	// We need to be executing within a strand to perform async operations
	// on the I/O objects in this session. Although not strictly necessary
	// for single-threaded contexts, this example code is written to be
	// thread-safe by default.
	net::dispatch(this->stream.get_executor(), 
			beast::bind_front_handler(&Session::onRun, shared_from_this()));
}

void Session::onRun() {
	LOG_DEBUG("Session onRun() called!!");
	//1. Set the timeout. ? 왜 타임아웃 거나?
	beast::get_lowest_layer(this->stream).expires_after(
			std::chrono::seconds(30));

	//2. Perform the SSL handshake
	this->stream.async_handshake(ssl::stream_base::server, 
			beast::bind_front_handler(&Session::onHandshake, shared_from_this()));
}

void Session::onHandshake(beast::error_code ec) {
	LOG_DEBUG("[Session onHandshake() called!!");
	if(ec) return printError(ec, "handshake");

	this->doRead();
}

void Session::doRead() {
	// Make the request empty before reading,
	// otherwise the operation behavior is undefined.
	LOG_DEBUG("[Session doRead() called!!");
	this->req = {};

	// Set the timeout.
	beast::get_lowest_layer(this->stream).expires_after(std::chrono::seconds(30));

	// Read a request
	http::async_read(this->stream, this->buffer, this->req,
			beast::bind_front_handler(&Session::onRead, shared_from_this()));
}

string Session::getFieldValue(string field) {
	auto it = req.find(field);
	if(it != req.end()) {
		return string(it->value());
	}
	return "";
}

int Session::getMethodNum() {
	switch(this->req.method()) {
		case http::verb::get :
            return GET;
		case http::verb::post :
            return POST;
		case http::verb::put :
            return PUT;
		case http::verb::delete_ :
            return DELETE;
	}
    return -1;
}

void Session::onRead(beast::error_code ec, std::size_t bytes_transferred) {
	boost::ignore_unused(bytes_transferred);

	LOG_DEBUG("Session onRead() called!!");
	// This means they closed the connection
	if(ec == http::error::end_of_stream)
		return doClose();

	if(ec) return printError(ec, "read");

	string uri = string(this->req.target());
    int method = this->getMethodNum();
    auto funcinfo = this->route.getFunction(method, uri);
	if(funcinfo == nullptr || funcinfo->fn == nullptr) {
        LOG_ERROR("fail get function-info method[" + std::to_string(method) + "] uri[" + uri + "]");
		return this->sendErrorResponse(http::status::not_found);
	}

    funcinfo->fn(this->req.body(), funcinfo->params, *this);
}

void Session::sendErrorResponse(http::status status) {
	auto sp = make_shared<http::response<http::string_body>>(status, req.version());
	sp->set(http::field::server, BOOST_BEAST_VERSION_STRING);
	sp->set(http::field::content_type, "application/json");
	sp->keep_alive(req.keep_alive());
	switch(status) {
		case http::status::not_found:
            LOG_ERROR("The resource '" + std::string(this->req.target()) + "' was not found.");
			sp->body() = "The resource '" + std::string(this->req.target()) + "' was not found.";
			break;
		case http::status::method_not_allowed:
            LOG_ERROR("Not allowed method");
			sp->body() = "Not allowed method";
			break;
		case http::status::unauthorized:
            LOG_ERROR("Unauthorized device");
			sp->body() = "Unauthorized device";
			break;
	}

	sp->prepare_payload();

	this->res = sp;
	http::async_write(this->stream, *sp, beast::bind_front_handler(
				&Session::onWrite, shared_from_this(), sp->need_eof()));
}

void Session::sendErrorResponse(http::status status, std::string message) {
	auto sp = make_shared<http::response<http::string_body>>(status, req.version());
	sp->set(http::field::server, BOOST_BEAST_VERSION_STRING);
	sp->set(http::field::content_type, "application/json");
	sp->keep_alive(req.keep_alive());
    sp->body() = message;

	sp->prepare_payload();

	this->res = sp;
	http::async_write(this->stream, *sp, beast::bind_front_handler(
				&Session::onWrite, shared_from_this(), sp->need_eof()));
}




void Session::sendResponse(string body) {
	// The lifetime of the message has to extend
	// for the duration of the async operation so
	// we use a shared_ptr to manage it.
	auto sp = make_shared<http::response<http::string_body>>(http::status::ok, req.version());
	sp->set(http::field::server, BOOST_BEAST_VERSION_STRING);
	sp->set(http::field::content_type, "application/json");
	sp->keep_alive(req.keep_alive());
	sp->body() = body;
	sp->prepare_payload();
	// Store a type-erased version of the shared
	// pointer in the class to keep it alive.
	this->res = sp;
	// Write the response
	http::async_write(this->stream, *sp, beast::bind_front_handler(
				&Session::onWrite, shared_from_this(), sp->need_eof()));
}

void Session::onWrite(bool close, beast::error_code ec, std::size_t bytes_transferred) {
	boost::ignore_unused(bytes_transferred);

	if(ec) return printError(ec, "write");

	// This means we should close the connection, usually because
	// the response indicated the "Connection: close" semantic.
	if(close) return doClose();

	// We're done with the response so delete it
	this->res = nullptr;

	// Read another request
	this->doRead();
}

void Session::doClose() {
	LOG_DEBUG("Session doClose() called!!");
	// Set the timeout.
	beast::get_lowest_layer(this->stream).expires_after(std::chrono::seconds(30));

	// Perform the SSL shutdown
	this->stream.async_shutdown(beast::bind_front_handler(&Session::onShutdown,
				shared_from_this()));
}

void Session::onShutdown(beast::error_code ec) {
	LOG_DEBUG("Session onShutdown() called!!");
	if(ec) return printError(ec, "shutdown");
	// At this point the connection is closed gracefully
}

bool Listener::initialize() {
	beast::error_code ec;

	//1. Open the acceptor
	this->acceptor.open(this->endpoint.protocol(), ec);
	if(ec) {
		printError(ec, "open");
		return false;
	}

	//2. Allow address reuse
	this->acceptor.set_option(net::socket_base::reuse_address(true), ec);
	if(ec) {
		printError(ec, "set_option");
		return false;
	}

	//3. Bind to the server address
	this->acceptor.bind(endpoint, ec);
	if(ec) {
		printError(ec, "bind");
		return false;
	}

	//4. Start listening for connections
	this->acceptor.listen(net::socket_base::max_listen_connections, ec);
	if(ec) {
		printError(ec, "listen");
		return false;
	}

	return true;
}

void Listener::run() {
	this->doAccept();
}

void Listener::doAccept() {
	// The new connection gets its own strand
    LOG_DEBUG("Listener doAccept() called!!");
	this->acceptor.async_accept(net::make_strand(this->ioc), 
			beast::bind_front_handler(&Listener::onAccept,	shared_from_this()));
}

void Listener::onAccept(beast::error_code ec, tcp::socket socket) {
	if(ec) {
		printError(ec, "accept");
	} else {
		// Create the session and run it
        LOG_DEBUG("Listener onAccept() called!!");

		try {
			auto session = std::make_shared<Session>(std::move(socket), this->ctx, this->route);
			session->run();
		} catch (std::exception &e) {
            LOG_ERROR("fail to create session. err:" + std::string(e.what()));
		}
	}

	// Accept another connection
	doAccept();
}

bool HttpsServer::run() {
	if(this->listener->initialize() == false) {
        LOG_ERROR("fail to init Listener!!");
		return false;
	}

	this->listener->run();
	//함수 return시 thread객체 메모리가 사라지지 않도록 new로 할당
	this->threads = new vector<thread*>();
	for(int i = 0; i < this->maxThread; i++) {
		threads->push_back(new thread(boost::bind(&net::io_context::run, &this->ioc)));
	}

	return true;
}

HttpsServer::HttpsServer(string address, unsigned short port, int _maxThread, UriRoute& route) : ioc(_maxThread), maxThread(_maxThread) {
    this->ctx = make_shared<ssl::context>(ssl::context::tls);
    this->listener = make_shared<Listener>(ioc, *this->ctx, 
            tcp::endpoint{net::ip::make_address(address), port}, route);
}

        //의미 없는 코드일 수 있음
HttpsServer::~HttpsServer() {
    if(this->threads) {
        for(int i = 0; i < this->threads->size(); i++) {
            delete this->threads->at(i);
        }
        delete this->threads;
    }
}

bool HttpsServer::setInitSSLContext(string certFile, string keyFile, string dhFile) {
    this->ctx->use_certificate_chain_file(certFile); 
    this->ctx->use_private_key_file(keyFile, boost::asio::ssl::context::pem);
    this->ctx->use_tmp_dh_file(dhFile);
    return true;
}


















