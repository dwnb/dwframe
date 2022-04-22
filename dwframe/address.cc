#include "address.h"
#include <sstream>
#include <string.h>
#include <netdb.h>
#include "endian.h"
#include <ifaddrs.h>//返回网卡信息
#include <stddef.h>
#include "log.h"
namespace dwframe{
    static dwframe::Logger::pointer g_logger = dwframe_log_name("system");
    template<typename T>
    static T CreateMask(uint32_t bits){//创建掩码，具体看up怎么用
        return (1<<(sizeof(T)*8-bits))-1;//
    }
    template<class T>
    static uint32_t CountBytes(T value) {//统计1的数量
        uint32_t result = 0;
        for(; value; ++result) {
            value &= value - 1;//统计1的技巧
        }
        return result;
    }

    Address::pointer Address::Create(const sockaddr* addr, socklen_t addrlen){
        if(addr == nullptr) {
            return nullptr;
        }

        Address::pointer result;
        switch(addr->sa_family) {
            case AF_INET:
                result.reset(new IPv4Address(*(const sockaddr_in*)addr));
                break;
            case AF_INET6:
                result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
                break;
            default:
                result.reset(new UnknowAddress(*addr));
                break;
        }
        return result;
    }
    //返回满足要求的任意一个地址
    Address::pointer Address::LookupAny(const std::string& host,
                                    int family, int type, int protocol) {
        std::vector<Address::pointer> result;
        if(Lookup(result, host, family, type, protocol)) {
            return result[0];
        }
        return nullptr;
    }
    //返回满足要求的任意一个IpAddress地址
    IPAddress::pointer Address::LookupAnyIPAddress(const std::string& host,
                                    int family, int type, int protocol) {
        std::vector<Address::pointer> result;
        if(Lookup(result, host, family, type, protocol)) {
            //for(auto& i : result) {
            //    std::cout << i->toString() << std::endl;
            //}
            for(auto& i : result) {
                IPAddress::pointer v = std::dynamic_pointer_cast<IPAddress>(i);
                if(v) {
                    return v;
                }
            }
        }
        return nullptr;
    }
    //通过主机名获取所有地址
    bool Address::Lookup(std::vector<Address::pointer>& result, const std::string& host,
            int family, int type, int protocol){
        addrinfo hints, *results, *next;//这里要搞懂
        hints.ai_flags = 0;
        hints.ai_family = family;
        hints.ai_socktype = type;
        hints.ai_protocol = protocol;
        hints.ai_addrlen = 0;
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;

        std::string node;
        const char* service = NULL;

        //检查 ipv6address serivce//检查有没有越界
        if(!host.empty() && host[0] == '[') {//memchr字符比较
            const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
            if(endipv6) {
                //TODO check out of range（没有检查越界问题）
                if(*(endipv6 + 1) == ':') {
                    service = endipv6 + 2;
                }
                node = host.substr(1, endipv6 - host.c_str() - 1);
            }
        }

        //检查 node serivce
        if(node.empty()) {//越界检查
            service = (const char*)memchr(host.c_str(), ':', host.size());
            if(service) {
                if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                    node = host.substr(0, service - host.c_str());
                    ++service;
                }
            }
        }

        if(node.empty()) {
            node = host;
        }
        int error = getaddrinfo(node.c_str(), service, &hints, &results);
        if(error) {
            dwframe_LOG_DEBUG(g_logger) << "Address::Lookup getaddress(" << host << ", "
                << family << ", " << type << ") err=" << error << " errstr="
                << gai_strerror(error);
            return false;
        }

        next = results;
        while(next) {
            result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
            //dwframe_LOG_INFO(g_logger) << ((sockaddr_in*)next->ai_addr)->sin_addr.s_addr;
            next = next->ai_next;
        }

        freeaddrinfo(results);
        return !result.empty();        
    }

    bool Address::GetInterfaceAddresses(std::multimap<std::string
                        ,std::pair<Address::pointer, uint32_t> >& result,
                        int family) {
        struct ifaddrs *next, *results;
        if(getifaddrs(&results) != 0) {
            dwframe_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
                " err=" << errno << " errstr=" << strerror(errno);
            return false;
        }

        try {
            for(next = results; next; next = next->ifa_next) {
                Address::pointer addr;
                uint32_t prefix_len = ~0u;
                if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                    continue;
                }
                switch(next->ifa_addr->sa_family) {
                    case AF_INET:
                        {
                            addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                            uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                            prefix_len = CountBytes(netmask);
                        }
                        break;
                    case AF_INET6:
                        {
                            addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                            in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                            prefix_len = 0;
                            for(int i = 0; i < 16; ++i) {
                                prefix_len += CountBytes(netmask.s6_addr[i]);
                            }
                        }
                        break;
                    default:
                        break;
                }

                if(addr) {
                    result.insert(std::make_pair(next->ifa_name,
                                std::make_pair(addr, prefix_len)));
                }
            }
        } catch (...) {
            dwframe_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
            freeifaddrs(results);
            return false;
        }
        freeifaddrs(results);
        return !result.empty();
    }

    bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::pointer, uint32_t> >&result
                        ,const std::string& iface, int family) {
        if(iface.empty() || iface == "*") {
            if(family == AF_INET || family == AF_UNSPEC) {
                result.push_back(std::make_pair(Address::pointer(new IPv4Address()), 0u));
            }
            if(family == AF_INET6 || family == AF_UNSPEC) {
                result.push_back(std::make_pair(Address::pointer(new IPv6Address()), 0u));
            }
            return true;
        }

        std::multimap<std::string
            ,std::pair<Address::pointer, uint32_t> > results;

        if(!GetInterfaceAddresses(results, family)) {
            return false;
        }

        auto its = results.equal_range(iface);//返回迭代器区间
        for(; its.first != its.second; ++its.first) {
            result.push_back(its.first->second);
        }
        return !result.empty();
    }
    int Address::getFamily() const{
        return getAddr()->sa_family;
    }

    std::string Address::toString() const {
        std::stringstream ss;
        insert(ss);
        return ss.str();
    }

    bool Address::operator<(const Address& rhs) const {
        socklen_t minlen = std::min(getAddrlen(), rhs.getAddrlen());
        int result = memcmp(getAddr(), rhs.getAddr(), minlen);
        if(result < 0) {
            return true;
        } else if(result > 0) {
            return false;
        } else if(getAddrlen() < rhs.getAddrlen()) {
            return true;
        }
        return false;
    }

    bool Address::operator==(const Address& rhs) const {
        return getAddrlen() == rhs.getAddrlen()
            && memcmp(getAddr(), rhs.getAddr(), getAddrlen()) == 0;
    }

    bool Address::operator!=(const Address& rhs) const {
        return !(*this == rhs);
    }
    IPAddress::pointer IPAddress::Create(const char *address,uint16_t port){//域名转ip
        addrinfo hints, *results;//此处用法？
        memset(&hints, 0, sizeof(addrinfo));

        hints.ai_flags = AI_NUMERICHOST;
        hints.ai_family = AF_UNSPEC;

        int error = getaddrinfo(address, NULL, &hints, &results);
        if(error) {
            dwframe_LOG_DEBUG(g_logger) << "IPAddress::Create(" << address
                << ", " << port << ") error=" << error
                << " errno=" << errno << " errstr=" << strerror(errno);
            return nullptr;
        }

        try {
            IPAddress::pointer result = std::dynamic_pointer_cast<IPAddress>(
                    Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
            if(result) {
                result->setPort(port);
            }
            freeaddrinfo(results);
            return result;
        } catch (...) {
            freeaddrinfo(results);
            return nullptr;
        }

    }
    IPv4Address::pointer IPv4Address::Create(const char* address,uint16_t port ){
        IPv4Address::pointer rt(new IPv4Address);
        rt->m_addr.sin_port = byteswapOnLittleEndian(port);
        int result = inet_pton(AF_INET,address,&rt->m_addr.sin_addr);
        if(result<=0){
            dwframe_LOG_DEBUG(g_logger) << "IPv4Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
            return nullptr;
        }
        return rt;
    }
    IPv4Address::IPv4Address(const sockaddr_in& address){
        m_addr = address;
    }
    IPv4Address::IPv4Address(uint32_t address,uint16_t port){//端口号应当是16位
        memset(&m_addr,0,sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = byteswapOnLittleEndian(port);//32位不会一堆0填充？
        m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
    }
    sockaddr* IPv4Address::getAddr() {
        return (sockaddr*)&m_addr;
    }
    const sockaddr* IPv4Address::getAddr() const{
        return (sockaddr*)&m_addr;
    }
    socklen_t IPv4Address::getAddrlen() const {
        return sizeof(m_addr);
    }
    std::ostream& IPv4Address::insert(std::ostream& os) const{
        uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
        os << ((addr >> 24) & 0xff) << "."
            << ((addr >> 16) & 0xff) << "."
            << ((addr >> 8) & 0xff) << "."
            << (addr & 0xff);
        os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
        return os;
    }

    IPAddress::pointer IPv4Address::broadcastAddress(uint32_t prefix_len) {
        if(prefix_len>32){
            return nullptr;
        }
        sockaddr_in baddr(m_addr);
        baddr.sin_addr.s_addr |= byteswapOnLittleEndian(
                CreateMask<uint32_t>(prefix_len));
        return IPv4Address::pointer(new IPv4Address(baddr)); 
    }
    IPAddress::pointer IPv4Address::networkAddress(uint32_t prefix_len) {
        if(prefix_len>32){
            return nullptr;
        }
        sockaddr_in baddr(m_addr);
        baddr.sin_addr.s_addr &= byteswapOnLittleEndian(
                CreateMask<uint32_t>(prefix_len));
        return IPv4Address::pointer(new IPv4Address(baddr)); 
    }//这里有问题？网络号应该是和子网掩码的与运算，这里“&=”后面需要取反
    IPAddress::pointer IPv4Address::subnetMask(uint32_t prefix_len){
        sockaddr_in subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin_family = AF_INET;
        subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
        return IPv4Address::pointer(new IPv4Address(subnet));
    }

    uint16_t IPv4Address::getPort()const {
        return byteswapOnLittleEndian(m_addr.sin_port);
    }
    void IPv4Address::setPort(uint16_t port) {
        m_addr.sin_port = byteswapOnLittleEndian(port);
    }

    //ipv6
    IPv6Address::pointer IPv6Address::Create(const char* address,uint16_t port ){
        IPv6Address::pointer rt(new IPv6Address);
        rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
        int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
        if(result <= 0) {
            dwframe_LOG_DEBUG(g_logger) << "IPv6Address::Create(" << address << ", "
                    << port << ") rt=" << result << " errno=" << errno
                    << " errstr=" << strerror(errno);
            return nullptr;
        }
        return rt;
    }
    IPv6Address::IPv6Address(){
        memset(&m_addr,0,sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        
    }
    IPv6Address::IPv6Address(const sockaddr_in6& address){
        m_addr = address;
    }
    IPv6Address::IPv6Address(const uint8_t Address[16],uint16_t port){
        memset(&m_addr,0,sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        m_addr.sin6_port = byteswapOnLittleEndian(port);//32位不会一堆0填充？
        memcpy(&(m_addr.sin6_addr.s6_addr),Address,16);//可以观察下sockaddr_in6的结构体
    }
    sockaddr* IPv6Address::getAddr() {
        return (sockaddr*)&m_addr;
    }
    const sockaddr* IPv6Address::getAddr() const{
        return (sockaddr*)&m_addr;
    }
    socklen_t IPv6Address::getAddrlen() const {
        return sizeof(m_addr);
    }
    std::ostream& IPv6Address::insert(std::ostream& os) const{
        os << "[";
        uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
        bool used_zeros = false;//这里代表前导0
        for(size_t i = 0; i < 8; ++i) {//写法挺好
            if(addr[i] == 0 && !used_zeros) {
                continue;
            }
            if(i && addr[i - 1] == 0 && !used_zeros) {
                os << ":";
                used_zeros = true;
            }
            if(i) {
                os << ":";
            }
            os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
        }

        if(!used_zeros && addr[7] == 0) {
            os << "::";
        }

        os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
        return os;

    }
    //ipv6没有广播地址，需要注意
    IPAddress::pointer IPv6Address::broadcastAddress(uint32_t prefix_len) {
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] |=
            CreateMask<uint8_t>(prefix_len % 8);
        for(int i = prefix_len / 8 + 1; i < 16; ++i) {
            baddr.sin6_addr.s6_addr[i] = 0xff;
        }
        return IPv6Address::pointer(new IPv6Address(baddr));
    }
    IPAddress::pointer IPv6Address::networkAddress(uint32_t prefix_len) {
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] &=
            CreateMask<uint8_t>(prefix_len % 8);
        for(int i = prefix_len / 8 + 1; i < 16; ++i) {
            baddr.sin6_addr.s6_addr[i] = 0x00;
        }
        return IPv6Address::pointer(new IPv6Address(baddr));
    }
    IPAddress::pointer IPv6Address::subnetMask(uint32_t prefix_len){
        sockaddr_in6 subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin6_family = AF_INET6;
        subnet.sin6_addr.s6_addr[prefix_len /8] =
            ~CreateMask<uint8_t>(prefix_len % 8);

        for(uint32_t i = 0; i < prefix_len / 8; ++i) {
            subnet.sin6_addr.s6_addr[i] = 0xff;
        }
        return IPv6Address::pointer(new IPv6Address(subnet));
    }

    uint16_t IPv6Address::getPort()const {
        return byteswapOnLittleEndian(m_addr.sin6_port);
    }
    void IPv6Address::setPort(uint16_t port) {
        m_addr.sin6_port = byteswapOnLittleEndian(port);
    }
    static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;
    UnixAddress::UnixAddress(){
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;//牛逼哦//偏移写法
    }
    UnixAddress::UnixAddress(const std::string& path){
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = path.size()+1;

        if(!path.empty() && path[0] == '\0') {
            --m_length;
        }

        if(m_length > sizeof(m_addr.sun_path)) {
            throw std::logic_error("path too long");
        }
        memcpy(m_addr.sun_path, path.c_str(), m_length);
        m_length += offsetof(sockaddr_un, sun_path);
    }
    void UnixAddress::setAddrlen(uint32_t v) {
        m_length = v;
    }

    sockaddr* UnixAddress::getAddr() {
        return (sockaddr*)&m_addr;
    }
    const sockaddr* UnixAddress::getAddr() const{
        return (sockaddr*)&m_addr;
    }
    socklen_t UnixAddress::getAddrlen() const{
        return m_length;
    }
    std::ostream& UnixAddress::insert(std::ostream& os) const{
        if(m_length > offsetof(sockaddr_un, sun_path)
                && m_addr.sun_path[0] == '\0') {
            return os << "\\0" << std::string(m_addr.sun_path + 1,
                    m_length - offsetof(sockaddr_un, sun_path) - 1);
        }
        return os << m_addr.sun_path;
    }

    UnknowAddress::UnknowAddress(int family){
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sa_family = family;

    }
    UnknowAddress::UnknowAddress(const sockaddr& addr){
        m_addr = addr;
    }
    sockaddr* UnknowAddress::getAddr() {
        return (sockaddr*)&m_addr;
    }
    const sockaddr* UnknowAddress::getAddr() const{
        return &m_addr;
    }
    socklen_t UnknowAddress::getAddrlen() const {
        return sizeof(m_addr);
    }
    std::ostream& UnknowAddress::insert(std::ostream& os) const {
        os << "[UnknownAddress family=" << m_addr.sa_family << "]";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Address& addr) {
        return addr.insert(os);
    }
}