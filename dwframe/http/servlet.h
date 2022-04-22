#pragma once

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include "http.h"
#include "http_session.h"
#include "../thread.h"
#include "../util.h"

namespace dwframe {
namespace http {

/**
 * @brief Servlet封装
 */
class Servlet {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<Servlet> pointer;

    /**
     * @brief 构造函数
     * @param[in] name 名称
     */
    Servlet(const std::string& name)
        :m_name(name) {}

    /**
     * @brief 析构函数
     */
    virtual ~Servlet() {}

    /**
     * @brief 处理请求
     * @param[in] request HTTP请求
     * @param[in] response HTTP响应
     * @param[in] session HTTP连接
     * @return 是否处理成功
     */
    virtual int32_t handle(dwframe::http::HttpRequest::pointer request
                   , dwframe::http::HttpResponse::pointer response
                   , dwframe::http::HttpSession::pointer session) = 0;
                   
    /**
     * @brief 返回Servlet名称
     */
    const std::string& getName() const { return m_name;}
protected:
    /// 名称
    std::string m_name;
};

/**
 * @brief 函数式Servlet
 */
class FunctionServlet : public Servlet {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<FunctionServlet> pointer;
    /// 函数回调类型定义
    typedef std::function<int32_t (dwframe::http::HttpRequest::pointer request
                   , dwframe::http::HttpResponse::pointer response
                   , dwframe::http::HttpSession::pointer session)> callback;


    /**
     * @brief 构造函数
     * @param[in] cb 回调函数
     */
    FunctionServlet(callback cb);
    virtual int32_t handle(dwframe::http::HttpRequest::pointer request
                   , dwframe::http::HttpResponse::pointer response
                   , dwframe::http::HttpSession::pointer session) override;
private:
    /// 回调函数
    callback m_cb;
};

class IServletCreator {
public:
    typedef std::shared_ptr<IServletCreator> pointer;
    virtual ~IServletCreator() {}
    virtual Servlet::pointer get() const = 0;
    virtual std::string getName() const = 0;
};

class HoldServletCreator : public IServletCreator {
public:
    typedef std::shared_ptr<HoldServletCreator> pointer;
    HoldServletCreator(Servlet::pointer slt)
        :m_servlet(slt) {
    }

    Servlet::pointer get() const override {
        return m_servlet;
    }

    std::string getName() const override {
        return m_servlet->getName();
    }
private:
    Servlet::pointer m_servlet;
};

template<class T>
class ServletCreator : public IServletCreator {
public:
    typedef std::shared_ptr<ServletCreator> pointer;

    ServletCreator() {
    }

    Servlet::pointer get() const override {
        return Servlet::pointer(new T);
    }

    std::string getName() const override {
        return TypeToName<T>();
    }
};

/**
 * @brief Servlet分发器
 */
class ServletDispatch : public Servlet {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<ServletDispatch> pointer;
    /// 读写锁类型定义
    typedef RWMutex RWMutexType;

    /**
     * @brief 构造函数
     */
    ServletDispatch();
    virtual int32_t handle(dwframe::http::HttpRequest::pointer request
                   , dwframe::http::HttpResponse::pointer response
                   , dwframe::http::HttpSession::pointer session) override;

    /**
     * @brief 添加servlet
     * @param[in] uri uri
     * @param[in] slt serlvet
     */
    void addServlet(const std::string& uri, Servlet::pointer slt);

    /**
     * @brief 添加servlet
     * @param[in] uri uri
     * @param[in] cb FunctionServlet回调函数
     */
    void addServlet(const std::string& uri, FunctionServlet::callback cb);

    /**
     * @brief 添加模糊匹配servlet
     * @param[in] uri uri 模糊匹配 /dwframe_*
     * @param[in] slt servlet
     */
    void addGlobServlet(const std::string& uri, Servlet::pointer slt);

    /**
     * @brief 添加模糊匹配servlet
     * @param[in] uri uri 模糊匹配 /dwframe_*
     * @param[in] cb FunctionServlet回调函数
     */
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    void addServletCreator(const std::string& uri, IServletCreator::pointer creator);
    void addGlobServletCreator(const std::string& uri, IServletCreator::pointer creator);

    template<class T>
    void addServletCreator(const std::string& uri) {
        addServletCreator(uri, std::make_shared<ServletCreator<T> >());
    }

    template<class T>
    void addGlobServletCreator(const std::string& uri) {
        addGlobServletCreator(uri, std::make_shared<ServletCreator<T> >());
    }

    /**
     * @brief 删除servlet
     * @param[in] uri uri
     */
    void delServlet(const std::string& uri);

    /**
     * @brief 删除模糊匹配servlet
     * @param[in] uri uri
     */
    void delGlobServlet(const std::string& uri);

    /**
     * @brief 返回默认servlet
     */
    Servlet::pointer getDefault() const { return m_default;}

    /**
     * @brief 设置默认servlet
     * @param[in] v servlet
     */
    void setDefault(Servlet::pointer v) { m_default = v;}


    /**
     * @brief 通过uri获取servlet
     * @param[in] uri uri
     * @return 返回对应的servlet
     */
    Servlet::pointer getServlet(const std::string& uri);

    /**
     * @brief 通过uri获取模糊匹配servlet
     * @param[in] uri uri
     * @return 返回对应的servlet
     */
    Servlet::pointer getGlobServlet(const std::string& uri);

    /**
     * @brief 通过uri获取servlet
     * @param[in] uri uri
     * @return 优先精准匹配,其次模糊匹配,最后返回默认
     */
    Servlet::pointer getMatchedServlet(const std::string& uri);

    void listAllServletCreator(std::map<std::string, IServletCreator::pointer>& infos);
    void listAllGlobServletCreator(std::map<std::string, IServletCreator::pointer>& infos);
private:
    /// 读写互斥量
    RWMutexType m_mutex;
    /// 精准匹配servlet MAP
    /// uri(/dwframe/xxx) -> servlet
    std::unordered_map<std::string, IServletCreator::pointer> m_datas;
    /// 模糊匹配servlet 数组
    /// uri(/dwframe/*) -> servlet
    std::vector<std::pair<std::string, IServletCreator::pointer> > m_globs;
    /// 默认servlet，所有路径都没匹配到时使用
    Servlet::pointer m_default;
};

/**
 * @brief NotFoundServlet(默认返回404)
 */
class NotFoundServlet : public Servlet {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<NotFoundServlet> pointer;
    /**
     * @brief 构造函数
     */
    NotFoundServlet(const std::string& name);
    virtual int32_t handle(dwframe::http::HttpRequest::pointer request
                   , dwframe::http::HttpResponse::pointer response
                   , dwframe::http::HttpSession::pointer session) override;

private:
    std::string m_name;
    std::string m_content;
};

}
}


