#include "../dwframe/http/http_server.h"
#include "../dwframe/log.h"

dwframe::Logger::pointer g_logger = dwframe_log_root();
dwframe::IOManager::pointer worker;
void run() {
    g_logger->setLevel(dwframe::LogLevel::INFO);
    dwframe::Address::pointer addr = dwframe::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        dwframe_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    //dwframe::http::HttpServer::pointer http_server(new dwframe::http::HttpServer(true, worker.get()));
    dwframe::http::HttpServer::pointer http_server(new dwframe::http::HttpServer(true));
    bool ssl = false;
    while(!http_server->bind(addr, ssl)) {
        dwframe_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }

    if(ssl) {
        //http_server->loadCertificates("/home/apps/soft/dwframe/keys/server.crt", "/home/apps/soft/dwframe/keys/server.key");
    }

    http_server->start();
}

int main(int argc, char** argv) {
    dwframe::IOManager iom(2);
    worker.reset(new dwframe::IOManager(3, false));
    iom.schedule(run);
    return 0;
}
