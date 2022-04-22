/**
 * @file http_server.h
 * @brief HTTP服务器封装
 * @author dwframe.yin
 * @email 564628276@qq.com
 * @date 2019-06-09
 * @copyright Copyright (c) 2019年 dwframe.yin All rights reserved (www.dwframe.top)
 */

#pragma once

#include "../Tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace dwframe {
namespace http {

/**
 * @brief HTTP服务器类
 */
class HttpServer : public TcpServer {
public:
    /// 智能指针类型
    typedef std::shared_ptr<HttpServer> pointer;

    /**
     * @brief 构造函数
     * @param[in] keepalive 是否长连接
     * @param[in] worker 工作调度器
     * @param[in] accept_worker 接收连接调度器
     */
    HttpServer(bool keepalive = false
               ,dwframe::IOManager* worker = dwframe::IOManager::GetThis()
               ,dwframe::IOManager* io_worker = dwframe::IOManager::GetThis()
               ,dwframe::IOManager* accept_worker = dwframe::IOManager::GetThis());

    /**
     * @brief 获取ServletDispatch
     */
    ServletDispatch::pointer getServletDispatch() const { return m_dispatch;}

    /**
     * @brief 设置ServletDispatch
     */
    void setServletDispatch(ServletDispatch::pointer v) { m_dispatch = v;}

    virtual void setName(const std::string& v) override;
protected:
    virtual void handleClient(Socket::pointer client) override;
private:
    /// 是否支持长连接
    bool m_isKeepalive;
    /// Servlet分发器
    ServletDispatch::pointer m_dispatch;
};

}
}

