#ifndef __INTERFACE_H
#define __INTERFACE_H

#include <mutex>

#include "HttpsServer.hpp"
#include "Singleton.hpp"

using std::cout;
using std::endl;
using std::string;

class Interface : public Singleton<Interface> {
    UriRoute                            route;
    std::shared_ptr<HttpsServer>	    httpsServer;
    std::mutex  mutex;

    public:

    bool addRestApiFunction(int method, std::string uri, handler fn) {
        //1. split uri and insert into vector
        this->route.addRoute(method, uri, fn);
        return true;
    }

    std::shared_ptr<funcinfo> getRestApiFunction(int method, std::string uri) {
        return this->route.getFunction(method, uri);
    }

    bool startServer(string address, int port, int numWorker, string certFile, string keyFile, string dhFile) {
        this->httpsServer = make_shared<HttpsServer>(address, port, numWorker, this->route);
        httpsServer->setInitSSLContext(certFile, keyFile, dhFile);
        httpsServer->run();
    };
};

#endif
