#pragma once

#include <memory>
#include "address.h"
#include "Noncopyable.h"
namespace dwframe{

    class Socket: public std::enable_shared_from_this<Socket>,Noncopyable{
    public:
        using pointer = std::shared_ptr<Socket>;
        using pointer_weak = std::weak_ptr<Socket>;
        /**
         * @brief Socket类型
         */
        enum Type {
            /// TCP类型
            TCP = SOCK_STREAM,
            /// UDP类型
            UDP = SOCK_DGRAM
        };

        /**
         * @brief Socket协议簇
         */
        enum Family {
            /// IPv4 socket
            IPv4 = AF_INET,
            /// IPv6 socket
            IPv6 = AF_INET6,
            /// Unix socket
            UNIX = AF_UNIX
        };

        /**
         * @brief 创建TCP Socket(满足地址类型)
         * @param[in] address 地址
         */
        static Socket::pointer CreateTCP(dwframe::Address::pointer address);

        /**
         * @brief 创建UDP Socket(满足地址类型)
         * @param[in] address 地址
         */
        static Socket::pointer CreateUDP(dwframe::Address::pointer address);

        /**
         * @brief 创建IPv4的TCP Socket
         */
        static Socket::pointer CreateTCPSocket();

        /**
         * @brief 创建IPv4的UDP Socket
         */
        static Socket::pointer CreateUDPSocket();

        /**
         * @brief 创建IPv6的TCP Socket
         */
        static Socket::pointer CreateTCPSocket6();

        /**
         * @brief 创建IPv6的UDP Socket
         */
        static Socket::pointer CreateUDPSocket6();

        /**
         * @brief 创建Unix的TCP Socket
         */
        static Socket::pointer CreateUnixTCPSocket();

        /**
         * @brief 创建Unix的UDP Socket
         */
        static Socket::pointer CreateUnixUDPSocket();

        Socket(int family,int type,int protocol = 0);
        ~Socket();

        int64_t getSendTimeout();
        void setSendTimeout(int64_t v);

        int64_t getRecvTimeout();
        void setRecvTimeout(int64_t v);

        bool getOption(int level, int option, void* result, socklen_t* len); //获取sockopt @see getsockopt 获取socket句柄上的一些信息
        template<typename T>
        bool getOption(int level, int option, T& result){
            size_t length = sizeof(T);
            return getOption(level,option,&result,&length);
        }

        bool setOption(int level,int option,const void* result,socklen_t len);
        template<typename T>
        bool setOption(int level, int option, T& result){
            return setOption(level,option,&result,sizeof(T));
        }

        virtual Socket::pointer accept();
        bool init(int sock );
        virtual bool bind(const Address::pointer addr);
        virtual bool connect(const Address::pointer addr, uint64_t timeout_ms = -1);
        virtual bool listen(int backlog = SOMAXCONN);
        virtual bool close();

        //Tcp
        int send(const void* buffer,size_t length,int flags = 0);
        int send(const iovec* buffers,size_t length,int flags = 0); 
        //udp
        int sendTo(const void* buffer,size_t length,const Address::pointer to,int flags = 0);
        int sendTo(const iovec* buffers,size_t length,const Address::pointer to,int flags = 0); 

        int recv(void* buffer,size_t length,int flags = 0);
        int recv(iovec* buffers,size_t length,int flags = 0);
        //udp
        int recvFrom(void* buffer,size_t length,Address::pointer from,int flags = 0);
        int recvFrom(iovec* buffers,size_t length,Address::pointer from,int flags = 0);

        Address::pointer getRemoteAddress();
        Address::pointer getLocalAddress();

        int getFamily() const{return m_family;};
        int getType() const{ return m_type;};
        int getProtocol() const{return m_protocol;};

        bool isConnected() const{return m_isConnected;};
        bool isVaild() const;
        int getError();

        std::ostream& dump(std::ostream& os) const;
        int getSocket() const{return m_sock;};

        bool cancelRead();
        bool cancelWrite();
        bool cancelAccept();
        bool cancelAll();
    private:
        void initSock();
        void newSock();
    private:
        int m_sock;//sock_fd
        int m_family;//协议族,ipv4,ipv6
        int m_type;//服务类型，流式还是数据报,TCP UDP
        int m_protocol;//网络数据交换规则，在因特网上，通用TCP/IP协议，它由以下部分组成:一.传输控制协议（TCP），
                      //使用一组规则与其他的因特网节点在数据包水平上交换信息。二.因特网协议（IP），
                        //使用一套规则来在因特网地址水平上发送和接收消息。
                        //另外还包括超文本传输协议（HTTP）和文件传输协议（FTP），以及边界网关协议（BGP）
                        //和动态主机配置协议（DHCP），简易邮件传输通讯协议（SMTP），邮局通讯协定（POP3）,网络通讯协议（TCP/IP）,用户数据报协议（UDP）
        bool m_isConnected;

        Address::pointer m_localAddress;
        Address::pointer m_remoteAddress;

    };
    /**
     * @brief 流式输出socket
     * @param[in, out] os 输出流
     * @param[in] sock Socket类
     */
    std::ostream& operator<<(std::ostream& os, const Socket& sock);
}