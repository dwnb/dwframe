#include "../dwframe/socket.h"
#include "../dwframe/dwframe.h"
#include "../dwframe/IoManager.h"

static dwframe::Logger::pointer g_looger = dwframe_log_root();

void test_socket() {
    //std::vector<sylar::Address::ptr> addrs;
    //sylar::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    //sylar::IPAddress::ptr addr;
    //for(auto& i : addrs) {
    //    SYLAR_LOG_INFO(g_looger) << i->toString();
    //    addr = std::dynamic_pointer_cast<sylar::IPAddress>(i);
    //    if(addr) {
    //        break;
    //    }
    //}
    dwframe::IPAddress::pointer addr = dwframe::Address::LookupAnyIPAddress("www.baidu.com");
    if(addr) {
        dwframe_LOG_INFO(g_looger) << "get address: " << addr->toString();
    } else {
        dwframe_LOG_ERROR(g_looger) << "get address fail";
        return;
    }

    dwframe::Socket::pointer sock = dwframe::Socket::CreateTCP(addr);
    addr->setPort(80);
    dwframe_LOG_INFO(g_looger) << "addr=" << addr->toString();
    if(!sock->connect(addr)) {
        dwframe_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        dwframe_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        dwframe_LOG_INFO(g_looger) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(1000000);
    rt = sock->recv(&buffs[0], buffs.size());

    if(rt <= 0) {
        dwframe_LOG_INFO(g_looger) << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(rt);
    dwframe_LOG_INFO(g_looger) << buffs;
    //dwframe_LOG_INFO(g_looger)<<"1";
}

void test2() {
    dwframe::IPAddress::pointer addr = dwframe::Address::LookupAnyIPAddress("www.baidu.com:80");
    if(addr) {
        dwframe_LOG_INFO(g_looger) << "get address: " << addr->toString();
    } else {
        dwframe_LOG_ERROR(g_looger) << "get address fail";
        return;
    }

    dwframe::Socket::pointer sock = dwframe::Socket::CreateTCP(addr);
    if(!sock->connect(addr)) {
        dwframe_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        dwframe_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
    }

    uint64_t ts = dwframe::GetCurrentUS();
    for(size_t i = 0; i < 100000000ul; ++i) {
        if(int err = sock->getError()) {
            dwframe_LOG_INFO(g_looger) << "err=" << err << " errstr=" << strerror(err);
            break;
        }

        //struct tcp_info tcp_info;
        //if(!sock->getOption(IPPROTO_TCP, TCP_INFO, tcp_info)) {
        //    dwframe_LOG_INFO(g_looger) << "err";
        //    break;
        //}
        //if(tcp_info.tcpi_state != TCP_ESTABLISHED) {
        //    dwframe_LOG_INFO(g_looger)
        //            << " state=" << (int)tcp_info.tcpi_state;
        //    break;
        //}
        static int batch = 10000000;
        if(i && (i % batch) == 0) {
            uint64_t ts2 = dwframe::GetCurrentUS();
            dwframe_LOG_INFO(g_looger) << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
            ts = ts2;
        }
    }
}

int main(int argc, char** argv) {
    dwframe::IOManager iom;
    //iom.schedule(&test_socket);
    iom.schedule(&test2);

    dwframe_LOG_INFO(g_looger)<<"1";
    return 0;
}
