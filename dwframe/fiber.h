#pragma once

#include <memory>
#include <ucontext.h>

#include "thread.h"

namespace dwframe{
    class Fiber : public std::enable_shared_from_this<Fiber>{//继承它，this指针就能变成shared_ptr。
    friend class Scheduler;
    public:
        using  pointer=std::shared_ptr<Fiber>;

        enum State{
            INIT,//初始化状态
            HOLD,//暂停状态
            EXEC,//执行中状态
            TERM,//结束状态
            READY,//可执行状态
            EXCEPT// 异常状态
        };
    private:
        Fiber();
    public:
        Fiber(std::function<void()> cb,size_t stackize =0,bool use_caller = false);
        ~Fiber();

        //重置协程函数，并重置状态
        //INIT,TEMR
        void reset(std::function<void()> cb);
        //切换到当前协程执行
        void swapIn();
        //切换到后台执行
        void swapOut();
        void call();
        void back();
        Fiber::State getState() const {return m_state;}
        void setState(Fiber::State state) {m_state = state;}
        uint64_t getId() const {return m_id;}
    public:
        //设置当前协程
        static void SetThis(Fiber* f);
        //返回当前协程
        static pointer GetThis();
        //协程切换到后台，并且设置为Ready状态
        static void YieldToReady();
        //协程切换到后台，并且设置为Hold状态
        static void YieldToHold();
        //总协程数
        static uint64_t TotalFibers();

        static uint64_t GetFiberId();

        static void MainFunc();
        static void CallerMainFunc();
    private:
        uint64_t m_id = 0;
        uint32_t m_stacksize = 0;
        State m_state = INIT;

        ucontext_t m_ctx;
        void* m_stack = nullptr;
        std::function<void()> m_cb;
    };
}