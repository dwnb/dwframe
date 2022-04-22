#ifndef __DWFRAME_UTIL__H__
#define __DWFRAME_UTIL__H__

#include <stdint.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <cxxabi.h>
namespace dwframe{
    pid_t GetThreadId();
    uint64_t GetFiberId();

    //输出栈信息
    void BackTrace(std::vector<std::string>& bt,int size,int skip=1);
    std::string BacktraceToString(int size,int skip=1,const std::string &prefix="");

    uint64_t GetCurrentMS();
    uint64_t GetCurrentUS();

    template<class T>
    const char* TypeToName();
}

#endif // !__DWFRAME_UTIL__H__