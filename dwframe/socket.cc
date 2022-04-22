#include "socket.h"
#include "fd_manager.h"
#include "log.h"
#include "macro.h"
#include "hook.h"
#include "IoManager.h"
#include "stdint.h"
#include "netinet/tcp.h"
namespace dwframe
{
    static dwframe::Logger::pointer g_logger = dwframe_log_name("system");

    Socket::pointer Socket::CreateTCP(dwframe::Address::pointer address) {
        Socket::pointer sock(new Socket(address->getFamily(), TCP, 0));
        return sock;
    }

    Socket::pointer Socket::CreateUDP(dwframe::Address::pointer address) {
        Socket::pointer sock(new Socket(address->getFamily(), UDP, 0));
        sock->newSock();
        sock->m_isConnected = true;
        return sock;
    }

    Socket::pointer Socket::CreateTCPSocket() {
        Socket::pointer sock(new Socket(IPv4, TCP, 0));
        return sock;
    }

    Socket::pointer Socket::CreateUDPSocket() {
        Socket::pointer sock(new Socket(IPv4, UDP, 0));
        sock->newSock();
        sock->m_isConnected = true;
        return sock;
    }

    Socket::pointer Socket::CreateTCPSocket6() {
        Socket::pointer sock(new Socket(IPv6, TCP, 0));
        return sock;
    }

    Socket::pointer Socket::CreateUDPSocket6() {
        Socket::pointer sock(new Socket(IPv6, UDP, 0));
        sock->newSock();
        sock->m_isConnected = true;
        return sock;
    }

    Socket::pointer Socket::CreateUnixTCPSocket() {
        Socket::pointer sock(new Socket(UNIX, TCP, 0));
        return sock;
    }

    Socket::pointer Socket::CreateUnixUDPSocket() {
        Socket::pointer sock(new Socket(UNIX, UDP, 0));
        return sock;
    }

    Socket::Socket(int family,int type,int protocol)
    :m_sock(-1)
    ,m_family(family)
    ,m_type(type)
    ,m_protocol(protocol)
    ,m_isConnected(false){

    }
    Socket::~Socket(){
        close();
    }

    int64_t Socket::getSendTimeout(){//获取超时时间
        FdCtx::pointer ctx  = FdMger::Getinstance()->get(m_sock);
        if(ctx){
            return ctx->getTimeout(SO_SNDTIMEO);
        }
        return -1;
    }
    void Socket::setSendTimeout(int64_t v){
        timeval tv{int(v/1000),int(v%1000*1000)};
        setOption(SOL_SOCKET,SO_SNDTIMEO,tv);
    }

    int64_t Socket::getRecvTimeout(){
        FdCtx::pointer ctx  = FdMger::Getinstance()->get(m_sock);
        if(ctx){
            return ctx->getTimeout(SO_RCVTIMEO);
        }
        return -1;
    }
    void Socket::setRecvTimeout(int64_t v){
        timeval tv{int(v/1000),int(v%1000*1000)};
        setOption(SOL_SOCKET,SO_RCVTIMEO,tv);
    }
    bool Socket::getOption(int level, int option, void* result, socklen_t* len){
        int rt = getsockopt(m_sock,level,option,result,len);
        if(rt){
            dwframe_LOG_DEBUG(g_logger)<<"getOption sock=" <<m_sock
            <<"level="<<level<<"option="<<option
            <<"errno="<<errno<<"errstr="<<strerror(errno);
            return false;
        }
        return true;
    }
    bool Socket::setOption(int level,int option,const void* result,socklen_t len){
        if(setsockopt(m_sock,level,option,result,len)){
            dwframe_LOG_DEBUG(g_logger)<<"getOption sock=" <<m_sock
            <<"level="<<level<<"option="<<option
            <<"errno="<<errno<<"errstr="<<strerror(errno);
            return false;
        }
        return true;
    }
    Socket::pointer Socket::accept(){
        Socket::pointer sock(new Socket(m_family,m_type,m_protocol));
        int newsock = ::accept(m_sock,nullptr,nullptr);//不需要传远端地址

        if(newsock == -1){
            dwframe_LOG_ERROR(g_logger)<<"accept("<<m_sock<<") errno="
            <<errno<<"errstr="<<strerror(errno);
            return nullptr;
        }

        if(sock->init(newsock)){
            return sock;
        }

        return nullptr;
    }
    bool Socket::init(int sock ){
        FdCtx::pointer ctx = FdMger::Getinstance()->get(sock);
        if(ctx &&ctx->isSocket()&&!ctx->isClose()){
            m_sock = sock;
            m_isConnected = true;
            initSock();
            getLocalAddress();
            getRemoteAddress();
            return true;
        }
        return false;
    }
    bool Socket::bind(const Address::pointer addr){
        if(!isVaild()){
            newSock();
            if(dwframe_UNLIKELY(!isVaild())) {
                return false;
            }
        }

        if(dwframe_UNLIKELY(addr->getFamily() != m_family)) {
            dwframe_LOG_ERROR(g_logger) << "bind sock.family("
                << m_family << ") addr.family(" << addr->getFamily()
                << ") not equal, addr=" << addr->toString();
            return false;
        }

        if(::bind(m_sock,addr->getAddr(),addr->getAddrlen())){
            dwframe_LOG_ERROR(g_logger)<<"bind error";
            return false;
        }

        getLocalAddress();
        return true;
    }
    bool Socket::connect(const Address::pointer addr, uint64_t timeout_ms){
        if(!isVaild()){
            newSock();
            if(dwframe_UNLIKELY(!isVaild())) {
                return false;
            }
        }

        if(dwframe_UNLIKELY(addr->getFamily() != m_family)) {
            dwframe_LOG_ERROR(g_logger) << "connect sock.family("
                << m_family << ") addr.family(" << addr->getFamily()
                << ") not equal, addr=" << addr->toString();
            return false;
        }

        if(timeout_ms == (uint64_t)-1) {
            if(::connect(m_sock, addr->getAddr(), addr->getAddrlen())) {
                dwframe_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                    << ") error errno=" << errno << " errstr=" << strerror(errno);
                close();
                return false;
            }
        } else {
            if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrlen(), timeout_ms)) {
                dwframe_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                    << ") timeout=" << timeout_ms << " error errno="
                    << errno << " errstr=" << strerror(errno);
                close();
                return false;
            }
        }
        m_isConnected = true;
        getRemoteAddress();
        getLocalAddress();
        return true;

    }
    bool Socket::listen(int backlog){
        if(!isVaild()) {
            dwframe_LOG_ERROR(g_logger) << "listen error sock=-1";
            return false;
        }
        if(::listen(m_sock, backlog)) {
            dwframe_LOG_ERROR(g_logger) << "listen error errno=" << errno
                << " errstr=" << strerror(errno);
            return false;
        }
        return true;
        }
    bool Socket::close(){
        if(!m_isConnected && m_sock == -1) {
            return true;
        }
        m_isConnected = false;
        if(m_sock != -1) {
            ::close(m_sock);
            m_sock = -1;
        }
        return false;
    }

    //Tcp
    int Socket::send(const void* buffer,size_t length,int flags){
        if(isConnected()) {
            return ::send(m_sock, buffer, length, flags);
        }
        return -1;
    }
    int Socket::send(const iovec* buffers,size_t length,int flags){
        if(isConnected()) {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffers;
            msg.msg_iovlen = length;
            return ::sendmsg(m_sock, &msg, flags);
        }
        return -1;
    }
    //udp
    int Socket::sendTo(const void* buffer,size_t length,const Address::pointer to,int flags){
        if(isConnected()) {
            return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrlen());
        }
        return -1;
    }
    int Socket::sendTo(const iovec* buffers,size_t length,const Address::pointer to,int flags){
        if(isConnected()) {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffers;
            msg.msg_iovlen = length;
            msg.msg_name = to->getAddr();
            msg.msg_namelen = to->getAddrlen();
            return ::sendmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    int Socket::recv(void* buffer,size_t length,int flags){
        if(isConnected()) {
            return ::recv(m_sock, buffer, length, flags);
        }
        return -1;
    }
    int Socket::recv(iovec* buffers,size_t length,int flags){
        if(isConnected()) {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffers;
            msg.msg_iovlen = length;
            return ::recvmsg(m_sock, &msg, flags);
        }
        return -1;
    }
    //udp
    int Socket::recvFrom(void* buffer,size_t length,Address::pointer from,int flags){
        if(isConnected()) {
            socklen_t len = from->getAddrlen();
            return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
        }
        return -1;
    }
    int Socket::recvFrom(iovec* buffers,size_t length,Address::pointer from,int flags){
        if(isConnected()) {
            msghdr msg;
            memset(&msg, 0, sizeof(msg));
            msg.msg_iov = (iovec*)buffers;
            msg.msg_iovlen = length;
            msg.msg_name = from->getAddr();
            msg.msg_namelen = from->getAddrlen();
            return ::recvmsg(m_sock, &msg, flags);
        }
        return -1;
    }

    Address::pointer Socket::getRemoteAddress(){
        if(m_remoteAddress) {
            return m_remoteAddress;
        }

        Address::pointer result;
        switch(m_family) {
            case AF_INET:
                result.reset(new IPv4Address());
                break;
            case AF_INET6:
                result.reset(new IPv6Address());
                break;
            case AF_UNIX:
                result.reset(new UnixAddress());//纯虚函数必须有实例，是的，子类不实例还是抽象类！！！
                break;
            default:
                result.reset(new UnknowAddress(m_family));
                break;
        }
        socklen_t addrlen = result->getAddrlen();
        if(getpeername(m_sock, result->getAddr(), &addrlen)) {
            //dwframe_LOG_ERROR(g_logger) << "getpeername error sock=" << m_sock
            //    << " errno=" << errno << " errstr=" << strerror(errno);
            return Address::pointer(new UnknowAddress(m_family));
        }
        if(m_family == AF_UNIX) {
            UnixAddress::pointer addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrlen(addrlen);
        }
        m_remoteAddress = result;
        return m_remoteAddress;
    }
    Address::pointer Socket::getLocalAddress(){
        if(m_localAddress) {
            return m_localAddress;
        }

        Address::pointer result;
        switch(m_family) {
            case AF_INET:
                result.reset(new IPv4Address());
                break;
            case AF_INET6:
                result.reset(new IPv6Address());
                break;
            case AF_UNIX:
                result.reset(new UnixAddress());
                break;
            default:
                result.reset(new UnknowAddress(m_family));
                break;
        }
        socklen_t addrlen = result->getAddrlen();
        if(getsockname(m_sock, result->getAddr(), &addrlen)) {
            dwframe_LOG_ERROR(g_logger) << "getsockname error sock=" << m_sock
                << " errno=" << errno << " errstr=" << strerror(errno);
            return Address::pointer(new UnknowAddress(m_family));
        }
        if(m_family == AF_UNIX) {
            UnixAddress::pointer addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrlen(addrlen);
        }
        m_localAddress = result;
        return m_localAddress;
    }

    bool Socket::isVaild() const{
        return m_sock != -1;
    }
    int Socket::getError(){
        int error = 0;
        socklen_t len = sizeof(error);
        if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
            error = errno;
        }
        return error;
    }

    std::ostream& Socket::dump(std::ostream& os) const{
        os << "[Socket sock=" << m_sock
        << " is_connected=" << m_isConnected
        << " family=" << m_family
        << " type=" << m_type
        << " protocol=" << m_protocol;
        if(m_localAddress) {
            os << " local_address=" << m_localAddress->toString();
        }
        if(m_remoteAddress) {
            os << " remote_address=" << m_remoteAddress->toString();
        }
        os << "]";
        return os;
    }

    bool Socket::cancelRead(){
        return IOManager::GetThis()->cancelEvent(m_sock, dwframe::IOManager::READ);
    }
    bool Socket::cancelWrite(){
        return IOManager::GetThis()->cancelEvent(m_sock, dwframe::IOManager::WRITE);
    }
    bool Socket::cancelAccept(){
        return IOManager::GetThis()->cancelEvent(m_sock, dwframe::IOManager::READ);
    }
    bool Socket::cancelAll(){
        return IOManager::GetThis()->cancelALL(m_sock);
    }
    void Socket::initSock(){
        int val = 1;
        setOption(SOL_SOCKET,SO_REUSEADDR,val);
        if(m_type == SOCK_STREAM){
            setOption(IPPROTO_TCP,TCP_NODELAY,val);//解决tcp延迟，不要积累一定量再发送，降低延迟
        }
        
    }
    void Socket::newSock(){
        m_sock = socket(m_family,m_type,m_protocol);
        if(dwframe_LIKELY(m_sock!=-1)){
            initSock();
        }else{
            dwframe_LOG_ERROR(g_logger)<<"socket("<<m_family<<
            ","<<m_type<<", "<<m_protocol<<") errno="<<
            errno<<" errstr="<<strerror(errno);
        }
    }

    std::ostream& operator<<(std::ostream& os, const Socket& sock) {
        return sock.dump(os);
    }
}