#pragma once

#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <sys/un.h>
#include <arpa/inet.h>
#include <vector>
#include <map>
namespace dwframe{
    //address基类
    class IPAddress;
    class Address{
    public:
        using pointer = std::shared_ptr<Address>;
        virtual ~Address(){}
        static Address::pointer Create(const sockaddr* addr, socklen_t addrlen);
        static bool Lookup(std::vector<Address::pointer>& result, const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);
        static Address::pointer LookupAny(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);
        static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);
        static bool GetInterfaceAddresses(std::multimap<std::string
                ,std::pair<Address::pointer, uint32_t> >& result,
                int family = AF_INET);//返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
        static bool GetInterfaceAddresses(std::vector<std::pair<Address::pointer, uint32_t> >&result
                    ,const std::string& iface, int family = AF_INET);//获取指定网卡的地址和子网掩码位数
        int getFamily() const;//属于那种类型的socket

        virtual const sockaddr* getAddr() const = 0;
        virtual sockaddr* getAddr() = 0;
        virtual socklen_t getAddrlen() const = 0;
        virtual std::ostream& insert(std::ostream& os) const = 0;

        std::string toString() const;

        bool operator<(const Address& rhs) const;
        bool operator==(const Address& rhs) const;
        bool operator!=(const Address& rhs) const;
    };
    //ipaddress基类
    class IPAddress : public Address{
    public:
        using pointer = std::shared_ptr<IPAddress>;
        static IPAddress::pointer Create(const char *address,uint16_t port = 0);
        virtual IPAddress::pointer broadcastAddress(uint32_t prefix_len) = 0; //广播地址
        virtual IPAddress::pointer networkAddress(uint32_t prefix_len) = 0;//网络地址
        virtual IPAddress::pointer subnetMask(uint32_t prefix_len) = 0;//子网掩码

        virtual uint16_t getPort()const=0;//获取端口号
        virtual void setPort(uint16_t port) = 0;//设置端口号
    };
    //ipv4
    class IPv4Address:public IPAddress{
    public:
        using pointer = std::shared_ptr<IPv4Address>;
        static IPv4Address::pointer Create(const char* address,uint16_t port = 0);

        IPv4Address(const sockaddr_in& address);
        IPv4Address(uint32_t address = INADDR_ANY,uint16_t port = 0);//设置地址和端口号

        virtual const sockaddr* getAddr() const override;
        virtual sockaddr* getAddr() override;
        virtual socklen_t getAddrlen() const override;
        virtual std::ostream& insert(std::ostream& os) const override;

        virtual IPAddress::pointer broadcastAddress(uint32_t prefix_len) override; //广播地址
        virtual IPAddress::pointer networkAddress(uint32_t prefix_len) override;//网络地址
        virtual IPAddress::pointer subnetMask(uint32_t prefix_len) override;//子网掩码

        virtual uint16_t getPort()const override;//获取端口号
        virtual void setPort(uint16_t port) override;//设置端口号
    private:
        sockaddr_in m_addr;
    };

    class IPv6Address : public IPAddress{
    public:
        using pointer = std::shared_ptr<IPv6Address>;
        static IPv6Address::pointer Create(const char* address,uint16_t port = 0);
        IPv6Address(const sockaddr_in6& address);
        IPv6Address();
        IPv6Address(const uint8_t Address[16],uint16_t port=0);//设置地址和端口号

        virtual const sockaddr* getAddr() const override;
        virtual sockaddr* getAddr() override;
        virtual socklen_t getAddrlen() const override;
        virtual std::ostream& insert(std::ostream& os) const override;

        virtual IPAddress::pointer broadcastAddress(uint32_t prefix_len) override; //广播地址
        virtual IPAddress::pointer networkAddress(uint32_t prefix_len) override;//网络地址
        virtual IPAddress::pointer subnetMask(uint32_t prefix_len) override;//子网掩码

        virtual uint16_t getPort()const override;//获取端口号
        virtual void setPort(uint16_t port) override;//设置端口号
    private:
        sockaddr_in6 m_addr;    
    };

    class UnixAddress : public Address{
    public:    
        using pointer = std::shared_ptr<UnixAddress>;
        UnixAddress(const std::string& path);
        UnixAddress();

        virtual const sockaddr* getAddr() const override;
        virtual sockaddr* getAddr() override;
        void setAddrlen(uint32_t v);
        virtual socklen_t getAddrlen() const override;
        virtual std::ostream& insert(std::ostream& os) const override;
    private:
        sockaddr_un m_addr;
        socklen_t m_length;
    };

    class UnknowAddress : public Address
    {
    public:
        using pointer = std::shared_ptr<UnknowAddress>;
        UnknowAddress(int family);
        UnknowAddress(const sockaddr& addr);
        virtual const sockaddr* getAddr() const override;
        virtual sockaddr* getAddr() override;
        virtual socklen_t getAddrlen() const override;
        virtual std::ostream& insert(std::ostream& os) const override;
    private:
        sockaddr m_addr; 
    };

    /**
     * @brief 流式输出Address
     */
    std::ostream& operator<<(std::ostream& os, const Address& addr);


    
}