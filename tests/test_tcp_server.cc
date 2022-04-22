#include "../dwframe/Tcp_server.h"
#include "../dwframe/IoManager.h"
#include "../dwframe/log.h"

dwframe::Logger::pointer g_logger = dwframe_log_root();

void run() {
    auto addr = dwframe::Address::LookupAny("0.0.0.0:8033");
    //auto addr2 = dwframe::UnixAddress::pointer(new dwframe::UnixAddress("/tmp/unix_addr"));
    std::vector<dwframe::Address::pointer> addrs;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    dwframe::TcpServer::pointer tcp_server(new dwframe::TcpServer);
    std::vector<dwframe::Address::pointer> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
    
}
int main(int argc, char** argv) {
    dwframe::IOManager iom(2);
    iom.schedule(run);
    return 0;
}