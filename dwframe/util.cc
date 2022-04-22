#include "util.h"
#include <execinfo.h> //栈信息
#include "fiber.h"
#include "log.h"
#include "sys/time.h"
namespace dwframe{
    dwframe::Logger::pointer sysLog = dwframe_log_name("system");

    pid_t GetThreadId(){
        return syscall(SYS_gettid);
    }
    uint64_t GetFiberId(){
        return dwframe::Fiber::GetFiberId();
    }

    void BackTrace(std::vector<std::string>& bt,int size,int skip){
        void **array = (void**)malloc((sizeof(void*)*size));
        size_t s =::backtrace(array,size);

        char** strings = backtrace_symbols(array,s);//strings返回成功要手动释放
        if(strings ==NULL){
            dwframe_LOG_ERROR(sysLog)<< "backtrace_synbols error";
            free(array);
            return;
        }

        for(size_t i=skip;i<s;++i){
            bt.push_back(strings[i]);//此处隐式类型转换，把c风格字符串转为std::string风格
        }

        free(strings);
        free(array);

    }
    std::string BacktraceToString(int size,int skip,const std::string& prefix){//被const修饰的左值引用可以绑定到右值上
        std::vector<std::string> bt;
        BackTrace(bt,size,skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); i++)
        {
            ss <<prefix<<bt[i] <<std::endl;
        }

        return ss.str();
        
    }

    uint64_t GetCurrentMS(){
        struct timeval tv;
        gettimeofday(&tv,nullptr);
        return tv.tv_sec*1000+tv.tv_usec /1000;
    }
    uint64_t GetCurrentUS(){
        struct timeval tv;
        gettimeofday(&tv,nullptr);
        return tv.tv_sec*1000000UL+tv.tv_usec;
    }

    template<class T>
    const char* TypeToName() {
        static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
        return s_name;
    }
}