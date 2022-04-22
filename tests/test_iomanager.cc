#include "../dwframe/dwframe.h"
#include "../dwframe/IoManager.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "fcntl.h"
#include "arpa/inet.h"
#include "unistd.h" 
static dwframe::Logger::pointer g_logger = dwframe_log_root();
void test_fiber(){
    dwframe_LOG_INFO(g_logger)<<"test_fiber";

    int sock = socket(AF_INET,SOCK_STREAM,0);

    fcntl(sock,F_SETFL,O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET,"180.101.49.11",&addr.sin_addr.s_addr);

    
    //dwframe_LOG_INFO(g_logger) << "fd="<<sock;
    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        dwframe_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        dwframe::IOManager::GetThis()->addEvent(sock, dwframe::IOManager::READ, [](){
            dwframe_LOG_INFO(g_logger) << "ERROR read callback";
        });
        dwframe::IOManager::GetThis()->addEvent(sock, dwframe::IOManager::WRITE, [=](){//文件描述符不能按照引用捕获？引用捕获fd变大？
            dwframe_LOG_INFO(g_logger) << "ERROR write callback";
            //close(sock);
            //dwframe_LOG_INFO(g_logger) << "fd="<<sock;
            dwframe::IOManager::GetThis()->cancelEvent(sock, dwframe::IOManager::READ);
            //dwframe_LOG_INFO(g_logger) << "cancel callback";
            close(sock);
        });
    } else {
        dwframe_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
    dwframe_LOG_INFO(g_logger) << "lastfd="<<sock;
}
void test1(){
    dwframe::IOManager iom(2,false);
    iom.schedule(&test_fiber);
}
dwframe::Timer::pointer s_timer;
void test_timer(){
   dwframe::IOManager iom(2);
    s_timer = iom.addtimer(500, [](){
        static int i = 0;
        dwframe_LOG_INFO(g_logger) << "ERROR,hello timer i=" << i;
        if(++i == 3) {
            s_timer->reset(2000,true);
            //s_timer->cancel();
        }
    }, true);

}
int main(int argc, char** argv){
    test1();
    test_timer();
    return 0;
}

/*
1、创建io类型协程调度器、在构造函数start（）
2、添加协程函数test_fiber
3、添加io写事件
4、析构时用stop执行run
5、先从任务队列取任务，就一个，执行完后陷入idle
5、由于idle重写会去循环判断有无epoll事件
6、epoll事件发生，执行后，从idle返回run;
7、由于stopping()重回写，若还有事件未发生会重新陷入idle的epoll等待
*/