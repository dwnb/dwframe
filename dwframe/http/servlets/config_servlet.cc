#include "config_servlet.h"
#include "../../config.h"

namespace dwframe {
namespace http {

ConfigServlet::ConfigServlet()
    :Servlet("ConfigServlet") {
}

int32_t ConfigServlet::handle(dwframe::http::HttpRequest::pointer request
                              ,dwframe::http::HttpResponse::pointer response
                              ,dwframe::http::HttpSession::pointer session) {
    std::string type = request->getParam("type");
    if(type == "json") {
        response->setHeader("Content-Type", "text/json charset=utf-8");
    } else {
        response->setHeader("Content-Type", "text/yaml charset=utf-8");
    }
    YAML::Node node;
    dwframe::Config::Visit([&node](ConfigVarBase::pointer base) {
        YAML::Node n;
        try {
            n = YAML::Load(base->toString());
        } catch(...) {
            return;
        }
        node[base->getName()] = n;
        node[base->getName() + "$description"] = base->getDescription();
    });
    if(type == "json") {
        Json::Value jvalue;
        if(YamlToJson(node, jvalue)) {
            response->setBody(JsonUtil::ToString(jvalue));
            return 0;
        }
    }
    std::stringstream ss;
    ss << node;
    response->setBody(ss.str());
    return 0;
}

}
}
