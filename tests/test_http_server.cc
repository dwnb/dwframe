#include "../dwframe/http/http_server.h"
#include "../dwframe/log.h"


static dwframe::Logger::pointer g_logger = dwframe_log_root();

#define XX(...) #__VA_ARGS__


dwframe::IOManager::pointer worker;
void run() {
    g_logger->setLevel(dwframe::LogLevel::INFO);
    //dwframe::http::HttpServer::pointer server(new dwframe::http::HttpServer(true, worker.get(), dwframe::IOManager::GetThis()));
    dwframe::http::HttpServer::pointer server(new dwframe::http::HttpServer(true));
    dwframe::Address::pointer addr = dwframe::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/dwframe/xx", [](dwframe::http::HttpRequest::pointer req
                ,dwframe::http::HttpResponse::pointer rsp
                ,dwframe::http::HttpSession::pointer session) {
            rsp->setBody(req->toString());
            return 0;
    });

    sd->addGlobServlet("/dwframe/*", [](dwframe::http::HttpRequest::pointer req
                ,dwframe::http::HttpResponse::pointer rsp
                ,dwframe::http::HttpSession::pointer session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
    });

    sd->addGlobServlet("/dwframex/*", [](dwframe::http::HttpRequest::pointer req
                ,dwframe::http::HttpResponse::pointer rsp
                ,dwframe::http::HttpSession::pointer session) {
            rsp->setBody(XX(<html>
<head><title>404 Not Found</title></head>
<body>
<center><h1>404 Not Found</h1></center>
<hr><center>nginx/1.16.0</center>
</body>
</html>
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
));
            return 0;
    });

    server->start();
}

int main(int argc, char** argv) {
    dwframe::IOManager iom(1, true, "main");
    worker.reset(new dwframe::IOManager(3, false, "worker"));
    iom.schedule(run);
    return 0;
}
