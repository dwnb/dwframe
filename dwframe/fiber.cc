#include "fiber.h"
#include "config.h"
#include "macro.h"
#include <atomic>
#include "log.h"
#include "scheduler.h"
namespace dwframe{
    static Logger::pointer g_logger = dwframe_log_name("system");
    static std::atomic<uint64_t> s_fiber_id{0};
    static std::atomic<uint64_t> s_fiber_count{0};

    static thread_local Fiber* t_fiber = nullptr;
    static thread_local Fiber::pointer t_threadFiber = nullptr;

    static ConfigVar<uint32_t>::pointer g_fiber_stack_size = Config::LookUp
                                                    ("fiber.stack_size",(uint32_t)1024*1024,"fiber stack size");
    class MallocStackAllocator
    {
    public:
        static void* Alloc(size_t size){//size_t为符号类型
            return malloc(size);
        }

        static void  Dealloc(void* vp,size_t size){//size_t为符号类型
            return free(vp);
        }
    };
    
    using StackAllocator = MallocStackAllocator;
    Fiber::Fiber(){
        m_state = EXEC;
        SetThis(this);

        if(getcontext(&m_ctx)){
            dwframe_ASSERT2(false,"getcontext");
        }
        ++s_fiber_count;
        dwframe_LOG_DEBUG(g_logger)<<"Fiber::Fiber";

    }
    Fiber::Fiber(std::function<void()> cb,size_t stackize,bool use_caller)
        :m_id(++s_fiber_id)
        ,m_cb(cb){
            ++s_fiber_count;
            m_stacksize = stackize? stackize: g_fiber_stack_size->getValue();

            m_stack =StackAllocator::Alloc(m_stacksize);
            if(getcontext(&m_ctx)){
                dwframe_ASSERT2(false,"getcontext");
            }

            m_ctx.uc_link =nullptr;
            m_ctx.uc_stack.ss_sp = m_stack;
            m_ctx.uc_stack.ss_size = m_stacksize;

            
             if(!use_caller) {
                makecontext(&m_ctx, &Fiber::MainFunc, 0);
            } else {
                makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
            }
            dwframe_LOG_DEBUG(g_logger)<<"Fiber::Fiber id="<<m_id;

    }
    Fiber::~Fiber(){
        --s_fiber_count;
        if(m_stack) {
            dwframe_ASSERT(m_state == TERM
                    || m_state == EXCEPT
                    || m_state == INIT);

            StackAllocator::Dealloc(m_stack, m_stacksize);
        } else {
            dwframe_ASSERT(!m_cb);
            dwframe_ASSERT(m_state == EXEC);

            Fiber* cur = t_fiber;
            if(cur == this) {
                SetThis(nullptr);
            }
        }
        dwframe_LOG_DEBUG(g_logger)<<"Fiber::~Fiber id="<<m_id;
    }

    //重置协程函数，并重置状态
    //INIT,TEMR
    void Fiber::reset(std::function<void()> cb){
        dwframe_ASSERT(m_stack);
        dwframe_ASSERT(m_state == TERM
                    || m_state == EXCEPT
                    || m_state == INIT);
        m_cb = cb;
        if(getcontext(&m_ctx)){
            dwframe_ASSERT2(false,"getcontext");
        }

        m_ctx.uc_link =nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx,&Fiber::MainFunc,0);
        m_state = INIT;
    }
    //切换到当前协程执行
    void Fiber::swapIn(){
        SetThis(this);
        dwframe_ASSERT(m_state!=EXEC);
        m_state = EXEC;
        if(swapcontext(&(Scheduler::GetMainFiber()->m_ctx),&m_ctx)){
            dwframe_ASSERT2(false,"swapcontext");
        }
    }
    //切换到后台执行
    void Fiber::swapOut(){
        //if(t_fiber!=Scheduler::GetMainFiber()){
        SetThis(Scheduler::GetMainFiber());
        if(swapcontext(&m_ctx,&(Scheduler::GetMainFiber()->m_ctx))){
            dwframe_ASSERT2(false,"swapcontext");
        }
        /*}else{
            SetThis(t_threadFiber.get());
            if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
                dwframe_ASSERT2(false, "swapcontext");
            }
        }*/
        
    }
    //设置当前协程
    void Fiber::SetThis(Fiber* f){
        t_fiber = f;
    }
    //返回当前协程
    Fiber::pointer Fiber::GetThis(){
        if(t_fiber){
            return t_fiber->shared_from_this();
        }

        Fiber::pointer main_fiber(new Fiber);
        dwframe_ASSERT(t_fiber == main_fiber.get());
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }
    //协程切换到后台，并且设置为Ready状态
    void Fiber::YieldToReady(){
        auto cur = GetThis();
        cur->m_state = READY;
        cur->swapOut();
    }
    //协程切换到后台，并且设置为Hold状态
    void Fiber::YieldToHold(){
        auto cur = GetThis();
        //cur->m_state = HOLD;
        cur->swapOut();
    }
    //总协程数
    uint64_t Fiber::TotalFibers(){
        return s_fiber_count;
    }


    void Fiber::MainFunc(){
        //auto cur = GetThis();//智能指针此处会增加当前协程对象的引用计数
        dwframe_ASSERT(t_fiber);
        try{
            t_fiber->m_cb();
            t_fiber->m_cb = nullptr;
            t_fiber->m_state = TERM;
        }catch(std::exception &ex){
            t_fiber->m_state =EXCEPT;
            dwframe_LOG_ERROR(g_logger)<<"Fiber Except:"<<ex.what()
            <<" fiber_id=" <<t_fiber->getId()
            <<std::endl<<dwframe::BacktraceToString(100);
        }catch(...){
            t_fiber->m_state =EXCEPT;
            dwframe_LOG_ERROR(g_logger)<<"Fiber Except"
            <<" fiber_id=" <<t_fiber->getId()
            <<std::endl<<dwframe::BacktraceToString(100);
        }

        //返回主协程
        //auto raw_ptr = cur.get();//智能指针减少当前协程对象的引用计数
        //cur.reset();
        t_fiber->swapOut();
    }

    void Fiber::CallerMainFunc(){
        //auto cur = GetThis();//智能指针此处会增加当前协程对象的引用计数
        dwframe_ASSERT(t_fiber);
        try{
            t_fiber->m_cb();
            t_fiber->m_cb = nullptr;
            t_fiber->m_state = TERM;
        }catch(std::exception &ex){
            t_fiber->m_state =EXCEPT;
            dwframe_LOG_ERROR(g_logger)<<"Fiber Except:"<<ex.what()
            <<" fiber_id=" <<t_fiber->getId()
            <<std::endl<<dwframe::BacktraceToString(100);
        }catch(...){
            t_fiber->m_state =EXCEPT;
            dwframe_LOG_ERROR(g_logger)<<"Fiber Except"
            <<" fiber_id=" <<t_fiber->getId()
            <<std::endl<<dwframe::BacktraceToString(100);
        }

        //返回主协程
        //auto raw_ptr = cur.get();//智能指针减少当前协程对象的引用计数
        //cur.reset();
        t_fiber->back();
    }
    uint64_t Fiber::GetFiberId(){
        if(t_fiber){
            return t_fiber->getId();
        }
        return 0;
    }

    void Fiber::call(){
        SetThis(this);
        m_state = EXEC;
        if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
            dwframe_ASSERT2(false, "swapcontext");
        }
    }

    void Fiber::back() {
        SetThis(t_threadFiber.get());
        if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
            dwframe_ASSERT2(false, "swapcontext");
        }
    }
}