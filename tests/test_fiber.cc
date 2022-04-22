#include "../dwframe/dwframe.h"

dwframe::Logger::pointer g_logger = dwframe_log_root();
void run_in_fiber(){
    dwframe_LOG_INFO(g_logger)<<"run begin()";
    dwframe::Fiber::GetThis()->YieldToHold();
    dwframe_LOG_INFO(g_logger)<<"run end()";
    dwframe::Fiber::GetThis()->YieldToHold();
}
void test_fiber(){
    dwframe::Fiber::GetThis();
    {
        dwframe_LOG_INFO(g_logger)<<"main begin()";
        dwframe::Fiber::pointer fiber(new dwframe::Fiber(run_in_fiber));
        fiber->swapIn();
        dwframe_LOG_INFO(g_logger)<<"main after swapin";
        fiber->swapIn();
        dwframe_LOG_INFO(g_logger)<<"main after end";
        fiber->swapIn();
    }
}
int main(){
    dwframe::Thread::SetName("main");
    std::vector<dwframe::Thread::pointer> thrs;
    for(int i=0;i<1;++i){
        dwframe::Thread::pointer thr(new dwframe::Thread(&test_fiber,"name_"+std::to_string(i*2)));
        thrs.push_back(thr);
    }

    for(size_t i=0;i<thrs.size();++i){
        thrs[i]->join();
    }
    dwframe_LOG_INFO(g_logger)<<"main after end2";
    
    return 0;
}