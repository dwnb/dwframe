#include "../dwframe/dwframe.h"

dwframe::Logger::pointer test_logger = dwframe_log_root();
void test_fiber(){
    dwframe_LOG_INFO(test_logger)<<"[ERROR] test in fiber";
    static int x=1000;
    usleep(1000);

    if(x-->0){
        dwframe::Scheduler::GetThis()->schedule(&test_fiber);
    }
    
}
int main(){
    dwframe_LOG_INFO(test_logger)<<"begin";
    dwframe::Scheduler sc(100,false,"test");
    
    sc.start();
    sc.schedule(&test_fiber);
    sc.stop();
    dwframe_LOG_INFO(test_logger)<<"over";
    return 0;
}