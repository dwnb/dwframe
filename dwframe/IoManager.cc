#include "IoManager.h"
#include "sys/epoll.h"
#include "macro.h"
#include "log.h"
#include "fcntl.h"
#include "unistd.h"
#include "string"
#include "errno.h"
namespace dwframe{
    static dwframe::Logger::pointer g_logger = dwframe_log_name("system");
    IOManager::FdContext::EventContext& IOManager::FdContext::getContext(Event event){
        switch(event){
            case IOManager::READ:
                return read;
            case IOManager::WRITE:
                return write;
            default:
                dwframe_ASSERT2(false,"getContext");
        }
    }

    void IOManager::FdContext::resetContext(EventContext& ctx){
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }
    void IOManager::FdContext::triggerEvent(Event event){
        dwframe_ASSERT(events& event);

        events = (Event)(events & ~event);
        EventContext& ctx = getContext(event);
        if(ctx.cb){
            ctx.scheduler->schedule(&ctx.cb);
        }else {
            ctx.scheduler->schedule(&ctx.fiber);
        }

        ctx.scheduler = nullptr;

        return;
    }
    IOManager::IOManager(size_t threads ,bool use_caller ,const std::string& name)
    : Scheduler(threads,use_caller,name){
        m_epfd = epoll_create(5000);
        dwframe_ASSERT(m_epfd>0);

        int rt = pipe(m_tickleFds);
        dwframe_ASSERT(!rt);

        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_tickleFds[0];

        rt = fcntl(m_tickleFds[0],F_SETFL,O_NONBLOCK);
        dwframe_ASSERT(!rt);

        rt = epoll_ctl(m_epfd,EPOLL_CTL_ADD,m_tickleFds[0],&event);
        dwframe_ASSERT(!rt);

        contextResize(32);
        start();
    }

    IOManager::~IOManager(){
        stop();
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        for(size_t i=0;i<m_fdContexts.size();++i){
            if(m_fdContexts[i]){
                delete m_fdContexts[i];
            }
        }
    }

    void IOManager::contextResize(size_t size){
        m_fdContexts.resize(size);
        for(size_t i =0 ;i<m_fdContexts.size();++i){
            if(!m_fdContexts[i]){
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }
    int IOManager::addEvent(int fd,Event event,std::function<void()>cb){
        //dwframe_LOG_INFO(g_logger) << "fd="<<fd;
        FdContext* fd_ctx = nullptr;
        RWMutex::WriteLock lock(m_mutex);
        if((int)m_fdContexts.size()>fd){
            fd_ctx = m_fdContexts[fd];
            //lock.unlock();
        }else {
            //lock.unlock();
            //RWMutex::WriteLock lock2(m_mutex);

            contextResize(fd * 1.5);//fd*1.5?是不是更好一点,fd可能很大
            //dwframe_LOG_INFO(g_logger) << "m_fdContexts.size="<<m_fdContexts.size();
            fd_ctx = m_fdContexts[fd];
        }
        //dwframe_LOG_INFO(g_logger) << "fd="<<fd;
        //dwframe_LOG_INFO(g_logger) << "m_fdContexts.size="<<m_fdContexts.size();
        FdContext::MutexLock::Lock lock2(fd_ctx->mutex);
        if(fd_ctx->events&event){//判断事件是否已经存在
            dwframe_LOG_ERROR(g_logger)<<"addEvent asssert="<<fd
                                        << " event="<<event
                                        <<" fd_ctx.event="<<fd_ctx->events;
            dwframe_ASSERT(!(fd_ctx->events&event));
        }

        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;//添加还是修改
        epoll_event epevent;
        epevent.events = EPOLLET | fd_ctx->events| event;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd,op,fd,&epevent);
        if(rt){
            dwframe_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","<<op<<","<<fd<<","<<epevent.events<<"):"
            <<rt<<" ("<<errno<<") ("<<strerror(errno)<<")";
            return -1;
        }

        m_pendingEventCount++;

        fd_ctx->events = (Event)(fd_ctx->events | event);//用或既可以表示都也可以表示写，也可以表示读写
        FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
        dwframe_ASSERT(!event_ctx.scheduler&&!event_ctx.fiber&&!event_ctx.cb);

        event_ctx.scheduler = Scheduler::GetThis();
        if(cb){
            event_ctx.cb.swap(cb);
        }else {
            event_ctx.fiber = Fiber::GetThis();
            dwframe_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
        }
        return 0;
    }
    bool IOManager::delEvent(int fd, Event event){
        RWMutex::ReadLock lock(m_mutex);
        if((int)m_fdContexts.size()<=fd){
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        //lock.unlock();

        FdContext::MutexLock::Lock lock2(fd_ctx->mutex);
        if(!(fd_ctx->events&event)){//如果event事件不存在
            return false;
        }
        Event new_events = (Event)(fd_ctx->events & ~event);//删除事件
        int  op = new_events ? EPOLL_CTL_MOD:EPOLL_CTL_DEL;

        epoll_event epevent;
        epevent.events = EPOLLET |new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd,op,fd,&epevent);
        if(rt){
            dwframe_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","<<op<<","<<fd<<","<<epevent.events<<"):"
            <<rt<<" ("<<errno<<") ("<<strerror(errno)<<")";
            return false;
        }

        --m_pendingEventCount;
        fd_ctx->events = new_events;
        FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return true;
    }

    bool IOManager::cancelEvent(int fd , Event event){//取消事件不是删除事件，取消事件是指强制触发事件
        //dwframe_LOG_INFO(g_logger)<<"m_pendingEventCount1="<<m_pendingEventCount;
        RWMutex::ReadLock lock(m_mutex);
        if((int)m_fdContexts.size()<=fd){
            dwframe_LOG_INFO(g_logger)<<"fd="<<fd;
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        //lock.unlock();
        //dwframe_LOG_INFO(g_logger)<<"m_pendingEventCount2="<<m_pendingEventCount;

        FdContext::MutexLock::Lock lock2(fd_ctx->mutex);
        if(!(fd_ctx->events&event)){//如果event事件不存在
            return false;
        }
        Event new_events = (Event)(fd_ctx->events & ~event);//删除事件
        int  op = new_events ? EPOLL_CTL_MOD:EPOLL_CTL_DEL;
        //dwframe_LOG_INFO(g_logger)<<"m_pendingEventCount3="<<m_pendingEventCount;

        epoll_event epevent;
        epevent.events = EPOLLET |new_events;
        epevent.data.ptr = fd_ctx;
        //dwframe_LOG_INFO(g_logger)<<"m_pendingEventCount4="<<m_pendingEventCount;
        int rt = epoll_ctl(m_epfd,op,fd,&epevent);
        if(rt){
            dwframe_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","<<op<<","<<fd<<","<<epevent.events<<"):"
            <<rt<<" ("<<errno<<") ("<<strerror(errno)<<")";
            return false;
        }
        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;
        //dwframe_LOG_INFO(g_logger)<<"m_pendingEventCount5="<<m_pendingEventCount;
        return true;
    }
    bool IOManager::cancelALL(int fd){
        RWMutex::ReadLock lock(m_mutex);
        if((int)m_fdContexts.size()<=fd){
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        //lock.unlock();

        FdContext::MutexLock::Lock lock2(fd_ctx->mutex);
        if(!(fd_ctx->events)){
            return false;
        }

        int  op = EPOLL_CTL_DEL;

        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd,op,fd,&epevent);
        if(rt){
            dwframe_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","<<op<<","<<fd<<","<<epevent.events<<"):"
            <<rt<<" ("<<errno<<") ("<<strerror(errno)<<")";
            return false;
        }
        if(fd_ctx->events&READ){
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        }

        if(fd_ctx->events&WRITE){
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }
        
        dwframe_ASSERT(fd_ctx->events==0);
        return true;
    }
    IOManager* IOManager::GetThis(){
        return dynamic_cast<IOManager*>(Scheduler::GetThis());
    }

    void IOManager::tickle(){//父类抽象为虚接口子类实现差异
        if(hasIdleThreads()){
            int rt = write(m_tickleFds[1],"T",1);
            dwframe_ASSERT(rt == 1);
        }
        
    }
    bool IOManager::stopping(uint64_t timerout){
        timerout = getNextTimer();
        return Scheduler::stopping()&&
                m_pendingEventCount == 0&&
                timerout == ~0ull;
    }
    bool IOManager::stopping(){
        uint64_t timerout = 0;
        return stopping(timerout);
    }
    void IOManager::idle(){
        epoll_event* events = new epoll_event[64]();
        //智能指针不支持数组，传递析构器
        std::shared_ptr<epoll_event> shared_events(events,[](epoll_event* ptr){
            delete [] ptr;
            //return;
        });

        
        while(true){
            uint64_t next_timeout = 0;
            if(stopping(next_timeout)){
                dwframe_LOG_INFO(g_logger)<<"name="<<getName()<<" idle stopping exit";
                break;
            }

            
            int rt = 0;
            do{
                static const int MAX_TIMEOUT = 1000;//epoll时间只支持毫秒级
                //dwframe_LOG_INFO(g_logger)<<"errno";
                if(next_timeout != ~0ull){
                    next_timeout = (int)next_timeout> MAX_TIMEOUT?
                                    MAX_TIMEOUT:next_timeout;
                }else{
                    next_timeout = MAX_TIMEOUT;
                }
                rt = epoll_wait(m_epfd,events,64,(int)next_timeout);
                if(rt<0&&errno == EINTR){
                    //dwframe_LOG_INFO(g_logger)<<"errno";
                }else{
                    break;
                }
            }while(true);

            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            if(!cbs.empty()){
                schedule(cbs.begin(),cbs.end());
                cbs.clear();
            }
            for(int i = 0;i<rt;++i){
                epoll_event& event = events[i];
                if(event.data.fd == m_tickleFds[0]){
                    uint8_t dummy;
                    while(read(m_tickleFds[0],&dummy,1)==1);
                    continue;
                }
                FdContext* fd_ctx = (FdContext*)event.data.ptr;
                FdContext::MutexLock::Lock lock(fd_ctx->mutex);
                
                if(fd_ctx->events&(EPOLLERR|EPOLLHUP)){//错误或者中断
                    event.events|=EPOLLIN|EPOLLOUT;
                }

                int real_events = NONE;
                if(event.events & EPOLLIN){
                    real_events |=READ;
                }
                if(event.events & EPOLLOUT){
                    real_events |=WRITE;
                }

                if((fd_ctx->events&real_events)==NONE){
                    continue;
                }

                int left_events = (fd_ctx->events&~real_events);//剔除处理已经触发的事件；

                int op =left_events?EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET |left_events;

                int rt2 = epoll_ctl(m_epfd,op,fd_ctx->fd,&event);
                if(rt2){
                    dwframe_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","<<op<<","<<fd_ctx->fd<<","<<event.events<<"):"
                    <<rt<<" ("<<errno<<") ("<<strerror(errno)<<")";
                    continue;
                }

                if(real_events&READ){
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }

                if(real_events&WRITE){
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }

            Fiber::pointer cur = Fiber::GetThis();
            auto raw_ptr = cur.get();
            cur.reset();
            //tickle();
            raw_ptr->swapOut();

        }

        
            
    }

    void IOManager::onTimerInsertedAtFront(){
        tickle();
    }
}

