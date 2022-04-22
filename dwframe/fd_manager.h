#pragma once

#include <memory>
#include "thread.h"
#include "IoManager.h"
#include "singleton.h"
#include "sys/stat.h"
#include "fd_manager.h"
namespace dwframe{
    class FdCtx: public std::enable_shared_from_this<FdCtx>{
    private:
        bool m_isInit: 1;
        bool m_isSocket: 1;
        bool m_sysNonblock: 1;
        bool m_userNonBlock: 1;
        bool m_isClosed: 1;
        int m_fd;

        uint64_t m_recvTimeout;
        uint64_t m_sendTimeout;
        dwframe::IOManager* m_iomanager;
    public:
        using pointer = std::shared_ptr<FdCtx>;
        FdCtx(int fd);

        bool init();
        bool isInit() const{return m_isInit;}
        bool isSocket() const {return m_isSocket;}
        bool isClose() const {return m_isClosed;}
        bool close();

        void setUserNonblock(bool v) { m_userNonBlock  = v; }
        bool getUserNonblock() const { return m_userNonBlock; }

        void setSysNonblock(bool v) {m_sysNonblock = v;}
        bool getSysNonblock()const {return m_sysNonblock;}

        void setTimeout(int type,uint64_t v);
        uint64_t getTimeout(int type);
        ~FdCtx();
        

    };
    
    class FdManager{
    public:
        using RWMutexType = RWMutex;
        FdManager();
        FdCtx::pointer get(int fd,bool auto_create = false);
        void del(int fd);
    private:
        RWMutexType m_mutex;
        std::vector<FdCtx::pointer> m_datas;
    };
    typedef Singleton<FdManager> FdMger;
}

