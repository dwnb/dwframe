#pragma once

#include "scheduler.h"
#include "timer.h"
namespace dwframe{
    class IOManager: public Scheduler,public TimerManager{
    public:
        using pointer = std::shared_ptr<IOManager>;
        using RWLock  = RWMutex;

        enum Event{
            NONE = 0x0,
            READ = 0x1,
            WRITE = 0x4
        };
    private:
        struct FdContext
        {
            using MutexLock = Mutex;
            struct EventContext
            {
                /* data */
                Scheduler* scheduler = nullptr;//事件执行scheduler
                Fiber::pointer fiber;//事件协程
                std::function<void()> cb;//事件回调函数
            };

            EventContext& getContext(Event event);
            void resetContext(EventContext& ctx);
            void triggerEvent(Event ctx);
            EventContext read; //读事件
            EventContext write;//写事件
            int fd =0;//事件有关句柄
            Event events = NONE;//已经注册事件
            MutexLock mutex;
        };
    public:
        IOManager(size_t threads = 1,bool use_caller = true,const std::string& name = "");
        ~IOManager();  

        //1 success , 0, retry -1 error
        int addEvent(int fd,Event event,std::function<void()> cb = nullptr);
        bool delEvent(int fd,Event event);
        bool cancelEvent(int fd,Event event);
        bool cancelALL(int fd);
        static IOManager* GetThis();
    protected:
        virtual void tickle() override;
        virtual bool stopping() override;
        bool stopping(uint64_t timerout);
        virtual void idle() override;
	    void contextResize(size_t size);
        virtual void onTimerInsertedAtFront()override;
    private:
        int m_epfd = 0;
        int m_tickleFds[2];

        std::atomic<size_t> m_pendingEventCount ={0};
        RWLock m_mutex;
        std::vector<FdContext*> m_fdContexts;
    };
    
}
