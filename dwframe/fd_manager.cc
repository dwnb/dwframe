#include "fd_manager.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "hook.h"

namespace dwframe{
    FdCtx::FdCtx(int fd)
    :m_isInit(false)
    ,m_isSocket(false)
    ,m_sysNonblock(false)
    ,m_userNonBlock(false)
    ,m_isClosed(false)
    ,m_fd(fd)
    ,m_recvTimeout(-1)
    ,m_sendTimeout(-1){
        init();
    }
    FdCtx::~FdCtx(){

    }
    bool FdCtx::init(){
        if(m_isInit){
            return true;
        }
        m_recvTimeout=-1;//uint64=-1?
        m_sendTimeout=-1;

        struct stat fd_stat;
        if(-1 == fstat(m_fd,&fd_stat)){
            m_isInit = false;
            m_isSocket = false;
        }else{
            m_isInit =true;
            m_isSocket = S_ISSOCK(fd_stat.st_mode);
        }

        if(m_isSocket){
            int flags = fcntl_f(m_fd,F_GETFL,0);
            if(!(flags&O_NONBLOCK)){
                fcntl_f(m_fd,F_SETFL,flags|O_NONBLOCK);
            }
            m_sysNonblock =true;
        }else{
            m_sysNonblock =false;
        }

        m_userNonBlock = false;
        m_isClosed = false;
        return m_isInit;
    }
    void FdCtx::setTimeout(int type,uint64_t V){
        if(type == SO_RCVTIMEO){
            m_recvTimeout = V;
        }else{
            m_sendTimeout = V;
        }
    }
    uint64_t FdCtx::getTimeout(int type){
        if(type == SO_RCVTIMEO){
            return m_recvTimeout;
        }else{
            return m_sendTimeout;
        }
    }
    FdManager::FdManager(){
        m_datas.resize(64);
    }
    FdCtx::pointer FdManager::get(int fd,bool auto_create){
        RWMutexType::WriteLock lock(m_mutex);
        if((int)m_datas.size()<=fd){
            if(auto_create == false){
                return nullptr;
            }
        }else{
            if(m_datas[fd]||!auto_create){
                return m_datas[fd];
            }

            
        }
        //lock.unlock();

        //RWMutexType::WriteLock lock2(m_mutex);
        FdCtx::pointer ctx(new FdCtx(fd));
        if(fd>=int(m_datas.size())){
            m_datas.resize(fd*1.5);
        }
        m_datas[fd] = ctx;//没有扩容
        return ctx;

    }
    void FdManager::del(int fd){
        RWMutexType::WriteLock lock(m_mutex);
        if((int)m_datas.size()<=fd){
            return ;
        }
        m_datas[fd].reset();
    }

}