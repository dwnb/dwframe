#pragma once

#include "../servlet.h"

namespace dwframe {
namespace http {

class ConfigServlet : public Servlet {
public:
    ConfigServlet();
    virtual int32_t handle(dwframe::http::HttpRequest::pointer request
                   , dwframe::http::HttpResponse::pointer response
                   , dwframe::http::HttpSession::pointer session) override;
};

}
}
