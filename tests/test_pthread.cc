#include "../dwframe/dwframe.h"
#include "unistd.h"
dwframe::Logger::pointer pLog = dwframe_log_root();

int count = 0;
dwframe::Mutex Mutex;
void fun1(){
    dwframe_LOG_INFO(pLog)<<"name: "<<dwframe::Thread::GetName()
                          <<" this.name: "<<dwframe::Thread::GetThis()->GetThis()->getName()
                          <<" id: "<<dwframe::GetThreadId()
                          <<" this.id: "<<dwframe::Thread::GetThis()->getID();
    //sleep(20);
    dwframe::Mutex::Lock lock(Mutex);
    for (size_t i = 0; i < 10000000; i++)
    {
        //dwframe::RWMutex::WriteLock lock(rwMutex);
        ++count;
    }
    
}
void fun2(){
    while(true){
        dwframe_LOG_INFO(pLog)<<"*******************************";
    }
}

void fun3(){
    while(true){
        dwframe_LOG_INFO(pLog)<<"===============================";
    }
}
int main(){
    YAML::Node root = YAML::LoadFile("/root/server/config/log.yml");
    dwframe::Config::LodeFromYaml(root);
    std::vector<dwframe::Thread::pointer> thrs;
    for(int i=0;i<4;++i){
        dwframe::Thread::pointer thr(new dwframe::Thread(&fun2,"name_"+std::to_string(i*2)));
        thrs.push_back(thr);

        dwframe::Thread::pointer thrd(new dwframe::Thread(&fun3,"name_"+std::to_string(i*2+1)));
        thrs.push_back(thrd);
    }

    for(size_t i=0;i<thrs.size();++i){
        thrs[i]->join();
    }
    dwframe_LOG_INFO(pLog)<<"thread test end";
    dwframe_LOG_INFO(pLog)<<"count ="<<count;
    return 0;
}