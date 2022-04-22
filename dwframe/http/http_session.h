/**
 * @file http_session.h
 * @brief HTTPSession封装
 * @author dwframe.yin
 * @email 564628276@qq.com
 * @date 2019-06-07
 * @copyright Copyright (c) 2019年 dwframe.yin All rights reserved (www.dwframe.top)
 */

#ifndef __dwframe_HTTP_SESSION_H__
#define __dwframe_HTTP_SESSION_H__

#include "../socket_stream.h"
#include "http.h"

namespace dwframe {
namespace http {

/**
 * @brief HTTPSession封装,服务端
 */
class HttpSession : public SocketStream {
public:
    /// 智能指针类型定义
    using pointer = std::shared_ptr<HttpSession>;

    /**
     * @brief 构造函数
     * @param[in] sock Socket类型
     * @param[in] owner 是否托管
     */
    HttpSession(Socket::pointer sock, bool owner = true);

    /**
     * @brief 接收HTTP请求
     */
    HttpRequest::pointer recvRequest();

    /**
     * @brief 发送HTTP响应
     * @param[in] rsp HTTP响应
     * @return >0 发送成功
     *         =0 对方关闭
     *         <0 Socket异常
     */
    int sendResponse(HttpResponse::pointer rsp);
};

}
}

#endif
