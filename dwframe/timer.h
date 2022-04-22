#pragma once

#include <memory>
#include "thread.h"
#include <set>
#include <vector>
namespace dwframe{
    //enable_shared_from_this可以详查
    class TimerManager;
    class Timer:public std::enable_shared_from_this<Timer>{//enable_shared_from_this表示这个类只能以智能指针的形式存在
    friend class TimerManager;
    public:
        using pointer = std::shared_ptr<Timer>;
        bool cancel();
        bool refresh();
        bool reset(uint64_t ms,bool form_now);
    private:
        Timer(uint64_t ms,std::function<void()> cb,
            bool recurring,TimerManager* manager);
        Timer(uint64_t next);
        
        struct Comparator
        {
            bool operator()(const Timer::pointer& lhs,const Timer::pointer& rhs)const;
        };
             
    private:
        bool m_recurring = false;//是否循环定时器
        uint64_t m_ms=0;//如果是循环定时器，执行周期
        uint32_t m_next=0;//精确的执行时间
        std::function<void()> m_cb;
        TimerManager* m_manager = nullptr;
    };

    class TimerManager
    {
    friend class Timer;
    

    public:
        using RWLock = dwframe::RWMutex;
        TimerManager(/* args */);
        virtual ~TimerManager();
        Timer::pointer addtimer(uint64_t ms,std::function<void()>cb,
                                bool recurring = false);//添加定时器
        Timer::pointer addConditionTimer(uint64_t ms,std::function<void()>cb,
                                std::weak_ptr<void> weak_cond,
                                bool recurring = false);
        uint64_t getNextTimer();
        void listExpiredCb(std::vector<std::function<void()>>& cbs);//返回已经需要执行的回调
        bool hasTimer();
    protected:
        virtual void onTimerInsertedAtFront() = 0;
        inline void addTimer(Timer::pointer val,RWMutex::WriteLock& lock);
    private:
        bool detectClockRollover(uint64_t now_ms);
    private:
        RWLock m_mutex;
        std::set<Timer::pointer,Timer::Comparator> m_timers;
        bool m_tickled = false;
        uint64_t m_previousTime = 0;
    };
    
    
}
