#include "../dwframe/hook.h"
#include "../dwframe/IoManager.h"
#include "../dwframe/log.h"
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
static dwframe::Logger::pointer g_logger = dwframe_log_root();
void test_sleep(){
    dwframe::IOManager iom(1);
    iom.schedule([](){
        sleep(2);
        dwframe_LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([](){
        sleep(3);
        dwframe_LOG_INFO(g_logger) << "sleep 3";
    });

    dwframe_LOG_INFO(g_logger)<<"test_sleep";
}
void test_sock(){
    int sock = socket(AF_INET,SOCK_STREAM,0);

    sockaddr_in addr;
    memset(&addr, 0 ,sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET,"180.97.34.94",&addr.sin_addr.s_addr);

    int rt = connect(sock,(const sockaddr*)&addr,sizeof(addr));
    dwframe_LOG_INFO(g_logger)<<"connect rt="<<rt<<"errnp="<<errno;

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    dwframe_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], buff.size(), 0);
    dwframe_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    buff.resize(rt);
    dwframe_LOG_INFO(g_logger) << buff;
}
int main(){
    //test_sleep();
    dwframe::IOManager iom(1);
    iom.schedule(test_sock);

    //int a=6;此例体现了shared_from_this的功能
    //std::shared_ptr<int> A(&a);
   //std::shared_ptr<int> B = A;

    //std::shared_ptr<int> C(&a);
    //std::cout << "A.use_count() = " << A.use_count() << std::endl;
    //std::cout << "B.use_count() = " << B.use_count() << std::endl;
    //std::cout << "C.use_count() = " << C.use_count() << std::endl;

    return 0;
}