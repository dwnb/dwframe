#include "../dwframe/address.h"
#include "../dwframe/log.h"

dwframe::Logger::pointer g_logger = dwframe_log_root();

void test() {
    std::vector<dwframe::Address::pointer> addrs;

    dwframe_LOG_INFO(g_logger) << "begin";
    //bool v = dwframe::Address::Lookup(addrs, "localhost:3080");
    bool v = dwframe::Address::Lookup(addrs, "www.baidu.com:pop3", AF_INET);
    //bool v = dwframe::Address::Lookup(addrs, "www.sylar.top", AF_INET);
    dwframe_LOG_INFO(g_logger) << "end";
    if(!v) {
        dwframe_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i) {
        dwframe_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }

    /*auto addr = dwframe::Address::LookupAny("localhost:4080");
    if(addr) {
        dwframe_LOG_INFO(g_logger) << *addr;
    } else {
        dwframe_LOG_ERROR(g_logger) << "error";
    }*/
}

void test_iface() {
    std::multimap<std::string, std::pair<dwframe::Address::pointer, uint32_t> > results;

    bool v = dwframe::Address::GetInterfaceAddresses(results,AF_UNSPEC);
    if(!v) {
        dwframe_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        dwframe_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

void test_ipv4() {
    //auto addr = sylar::IPAddress::Create("www.sylar.top");
    auto addr = dwframe::IPAddress::Create("127.0.0.8");
    if(addr) {
        dwframe_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    //test_ipv4();
    //test_iface();
    test();
    return 0;
}