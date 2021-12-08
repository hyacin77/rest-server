#include "HttpsClient.hpp"

string ClientSession::getResponseBody() {
    return res.body();
}

// Start the asynchronous operation
void ClientSession::run(char const* host, char const* port, http::verb method, char const* target, int version, string& body) {
    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(! SSL_set_tlsext_host_name(stream.native_handle(), host)) {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        LOG_ERROR("Client Session run fail, err:" + ec.message());
        return;
    }

    // Set up an HTTP GET request message
    req.version(version);
    req.method(method);
    req.target(target);
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.set(http::field::content_length, body.length());

    // Look up the domain name
    resolver.async_resolve(host, port, 
            beast::bind_front_handler(&ClientSession::onResolve, shared_from_this()));
}

void ClientSession::onResolve(beast::error_code ec, tcp::resolver::results_type results) {
    if(ec) return fail(ec, "resolve");

    // Set a timeout on the operation
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(stream).async_connect(results,
            beast::bind_front_handler(&ClientSession::onConnect, shared_from_this()));
}

void ClientSession::onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
    if(ec) return fail(ec, "connect");

    // Perform the SSL handshake
    stream.async_handshake(ssl::stream_base::client, 
            beast::bind_front_handler(&ClientSession::onHandshake, shared_from_this()));
}

void ClientSession::onHandshake(beast::error_code ec) {
    if(ec) return fail(ec, "handshake");
    // Set a timeout on the operation
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
    // Send the HTTP request to the remote host
    http::async_write(stream, req,
            beast::bind_front_handler( &ClientSession::onWrite, shared_from_this()));
}

void ClientSession::onWrite(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if(ec) return fail(ec, "write");

    // Receive the HTTP response
    http::async_read(stream, buffer, res,
            beast::bind_front_handler(&ClientSession::onRead, shared_from_this()));
}

void ClientSession::onRead(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if(ec) return fail(ec, "read");

    // Set a timeout on the operation
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Gracefully close the stream
    stream.async_shutdown(beast::bind_front_handler(&ClientSession::onShutdown, shared_from_this()));
}

void ClientSession::onShutdown(beast::error_code ec) {
    if(ec == net::error::eof) {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
    }
    if(ec) return fail(ec, "shutdown");

    // If we get here then the connection is closed gracefully
}

bool HttpsClient::setInitSSLContext(string certFile, string keyFile, string dhFile) {
    this->ctx.use_certificate_chain_file(certFile); 
    this->ctx.use_private_key_file(keyFile, boost::asio::ssl::context::pem);
    this->ctx.use_tmp_dh_file(dhFile);
    return true;
}

bool HttpsClient::sendRequest(string & result, string address, string port, http::verb method, string uri, string& body) {
    auto s = std::make_shared<ClientSession>(this->ioc, this->ctx);
    s->run(address.c_str(), port.c_str(), method, uri.c_str(), 10, body);
    ioc.run();
    result = s->getResponseBody();
    return s->getIsSuccess();
}

