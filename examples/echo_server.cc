#include "../dwframe/Tcp_server.h"
#include "../dwframe/log.h"
#include "../dwframe/IoManager.h"
#include "../dwframe/bytearray.h"
#include "../dwframe/address.h"
#include <unistd.h>
#include <sys/types.h>

static dwframe::Logger::pointer g_logger = dwframe_log_root();

class EchoServer : public dwframe::TcpServer {
public:
    EchoServer(int type);
    void handleClient(dwframe::Socket::pointer client);
    dwframe::Socket::pointer getCtSock(){return c_sock;}
private:
    int m_type = 0;
    dwframe::Socket::pointer c_sock=nullptr;
};

EchoServer::EchoServer(int type)
    :m_type(type) {
}

void EchoServer::handleClient(dwframe::Socket::pointer client) {
    
    dwframe_LOG_INFO(g_logger) << "handleClient " << *client;   
    dwframe::ByteArray::pointer ba(new dwframe::ByteArray);
    while(true) {
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);

        int rt = client->recv(&iovs[0], iovs.size());
        if(rt == 0) {
            dwframe_LOG_INFO(g_logger) << "client close: " << *client;
            break;
        } else if(rt < 0) {
            dwframe_LOG_INFO(g_logger) << "client error rt=" << rt
                << " errno=" << errno << " errstr=" << strerror(errno);
            break;
        }
        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        //dwframe_LOG_INFO(g_logger) << "recv rt=" << rt << " data=" << std::string((char*)iovs[0].iov_base, rt);
        if(m_type == 1) {//text 
            if(ba->toString()=="exit\r\n"){
                client->close();
                std::cout <<"close_ct"<<std::endl;
                return;
            } 
            std::cout << ba->toString()<<std::endl;// << std::endl;
            
        } else {
            std::cout << ba->toHexString()<<std::endl;// << std::endl;
        }
        std::cout.flush();
    }
}

int type = 1;

void run() {
    dwframe_LOG_INFO(g_logger) << "server type=" << type;
    EchoServer::pointer es(new EchoServer(type));
    
    auto addr = dwframe::Address::LookupAny("0.0.0.0:8020");
    while(!es->bind(addr)) {
        sleep(2);
    }
    
    es->start();
}

int main(int argc, char** argv) {

    int rt =fork();
    if(rt==0){
        if(argc < 2) {
            dwframe_LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
            return 0;
        }

        if(!strcmp(argv[1], "-b")) {
            type = 2;
        }

        dwframe::IOManager iom(2);
        iom.schedule(run);
    }

    
    return 0;
}
