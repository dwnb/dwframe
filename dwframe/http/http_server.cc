#include "http_server.h"
#include "../log.h"
//#include "servlets/config_servlet.h"
//#include "dwframe/http/servlets/status_servlet.h"

namespace dwframe {
namespace http {

static dwframe::Logger::pointer g_logger = dwframe_log_name("system");

HttpServer::HttpServer(bool keepalive
               ,dwframe::IOManager* worker
               ,dwframe::IOManager* io_worker
               ,dwframe::IOManager* accept_worker)
    :TcpServer(worker, io_worker, accept_worker)
    ,m_isKeepalive(keepalive) {
    m_dispatch.reset(new ServletDispatch);

    m_type = "http";
    //m_dispatch->addServlet("/_/status", Servlet::pointer(new StatusServlet));
    //m_dispatch->addServlet("/_/config", Servlet::pointer(new ConfigServlet));
}

void HttpServer::setName(const std::string& v) {
    TcpServer::setName(v);
    m_dispatch->setDefault(std::make_shared<NotFoundServlet>(v));
}

void HttpServer::handleClient(Socket::pointer client) {
    dwframe_LOG_DEBUG(g_logger) << "handleClient " << *client;
    HttpSession::pointer session(new HttpSession(client));
    do {
        auto req = session->recvRequest();
        if(!req) {
            dwframe_LOG_DEBUG(g_logger) << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " cliet:" << *client << " keep_alive=" << m_isKeepalive;
            break;
        }

        HttpResponse::pointer rsp(new HttpResponse(req->getVersion()
                            ,req->isClose() || !m_isKeepalive));
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);//AOP?
        session->sendResponse(rsp);

        if(!m_isKeepalive || req->isClose()) {
            break;
        }
    } while(true);
    session->close();
}

}
}
