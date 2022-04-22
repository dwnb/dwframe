#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"
namespace dwframe{
    static dwframe::Logger::pointer g_logger = dwframe_log_name("system");

    static thread_local Scheduler* t_scheduler = nullptr;
    static thread_local Fiber* t_Fiber = nullptr;
    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    :m_name(name){
        dwframe_ASSERT(threads>0);

        if(use_caller){
            dwframe::Fiber::GetThis();
            --threads;

            dwframe_ASSERT(GetThis() == nullptr);
            t_scheduler = this;

            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run,this),0,true));
            dwframe::Thread::SetName(m_name);

            t_Fiber = m_rootFiber.get();
            m_rootThread = dwframe::GetThreadId();
            m_threadIds.push_back(m_rootThread);
        }else{
            m_rootThread = -1;
        }
        m_threadCount  = threads;
    }
    Scheduler::~Scheduler(){
        dwframe_ASSERT(m_stoping);
        if(GetThis() == this){
            t_scheduler = nullptr;
        }
    }

    Scheduler* Scheduler::GetThis(){
        return t_scheduler;
    }
    Fiber* Scheduler::GetMainFiber(){
        return t_Fiber;
    }

    void Scheduler::start(){
        MutexType::Lock lock(m_mutex);
        if(!m_stoping){
            return ;
        }

        m_stoping = false;
        dwframe_ASSERT(m_threads.empty());

        m_threads.resize(m_threadCount);
        dwframe_LOG_INFO(g_logger)<<"threads: "<<m_threads.size();
        for(size_t i =0;i<m_threadCount;++i){
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run,this)
                                    ,m_name+"_"+std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getID());
        }

        lock.unlock();
        //dwframe_LOG_INFO(g_logger)<<"call";
        //dwframe_LOG_INFO(g_logger)<<"m_rootFiber";
        //if(m_rootFiber){
        //    dwframe_LOG_INFO(g_logger)<<"call";
        //    m_rootFiber->call();
        //}
    }
    void Scheduler::stop(){
        dwframe_LOG_INFO(g_logger)<<this<<"stopp start";
        m_autoStop = true;
        if(m_rootFiber&&m_threadCount==0
            && (m_rootFiber->getState()==Fiber::TERM
            ||m_rootFiber->getState()==Fiber::INIT)){
                dwframe_LOG_INFO(g_logger)<<this<<"stopped";
            
            m_stoping = true;
            if(stopping()){
                return;
            }
        }

        //bool exit_on_this_fiber = false;
        if(m_rootThread != -1) {
            dwframe_ASSERT(GetThis() == this);
        } else {
            dwframe_ASSERT(GetThis() != this);
        }

        m_stoping = true;
        for(size_t i =0;i<m_threadCount;++i){
            tickle();
        }

        if(m_rootFiber){
            tickle();
        }
        if(m_rootFiber){
           /* while(!stopping()){
                if(m_rootFiber->getState() == Fiber::TERM
                    || m_rootFiber->getState() == Fiber::EXCEPT){
                        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run,this),0,true));
                        t_Fiber = m_rootFiber.get();
                        dwframe_LOG_INFO(g_logger)<<"root fiber is term";
                    }
                m_rootFiber->call();
            }*/
            if(!stopping())m_rootFiber->call();
            
        }
        if(stopping()){
            return;
        }

        std::vector<Thread::pointer> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }

        for(auto &i:thrs){
            i->join();
        }
        
    }
    
    void Scheduler::setThis(){
        t_scheduler = this;
    }
    void Scheduler::run(){
        /*setThis();
        //return;
        if(dwframe::GetThreadId() != m_rootThread){
            dwframe_LOG_INFO(g_logger)<<"t_Fiber";
            t_Fiber = Fiber::GetThis().get();
        }
        Fiber::pointer idle_fiber(new Fiber(std::bind(&dwframe::Scheduler::idle,this)));

        Fiber::pointer cb_fiber;

        FiberAndThread ft;
        while(true){
           
            ft.reset();
            bool tickle_me  = false;
            {
                //从消息队列取出一个我们想要执行的任务
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                while(it!=m_fibers.end()){
                    if(it->thread != -1&&it->thread!=dwframe::GetThreadId()){
                        ++it;
                        tickle_me = true;
                        continue;
                    }
                    //dwframe_LOG_ERROR(dwframe_log_root())<<"m_fibers.size 0:"<<m_fibers.size();
                    dwframe_ASSERT(it->fiber||it->cb);
                    if(it->fiber&&it->fiber->getState() == Fiber::EXEC){
                        ++it;
                        continue;
                    }
                    ft=*it;
                    m_fibers.erase(it++);
                    //dwframe_LOG_ERROR(dwframe_log_root())<<"m_fibers.size 1:"<<m_fibers.size();
                    break;
                }
            }
            
            //dwframe_LOG_INFO(g_logger)<<"unLock";
            if(tickle_me){
                tickle();
            }
            if(ft.fiber && (ft.fiber->getState()!=Fiber::TERM
                    &&ft.fiber->getState()!=Fiber::EXCEPT)){
                ++m_idleThreadConunt;
                ft.fiber->swapIn();
                --m_idleThreadConunt;
                if(ft.fiber->getState()!=Fiber::TERM
                    &&ft.fiber->getState()!=Fiber::EXCEPT){
                        ft.fiber->setState(Fiber::HOLD);
                    }
            }else if(ft.cb){
                if(cb_fiber){
                    cb_fiber->reset(ft.cb);
                }else{
                    cb_fiber.reset(new Fiber(ft.cb));
                }

                ft.reset();
                cb_fiber->swapIn();
                if(cb_fiber->getState()==Fiber::READY){
                    schedule(cb_fiber);
                    cb_fiber.reset();
                }else if(cb_fiber->getState()==Fiber::EXCEPT
                ||cb_fiber->getState()==Fiber::TERM){
                    cb_fiber->reset(nullptr);
                }else {
                    cb_fiber->setState(Fiber::HOLD);
                    cb_fiber.reset();
                }
            }else {
                if(idle_fiber->getState() == Fiber::TERM){
                    dwframe_LOG_INFO(g_logger)<<"idle fiber term";
                    break;
                }
                ++m_idleThreadConunt;
                idle_fiber->swapIn();
                if(idle_fiber->getState()!=Fiber::TERM
                    &&idle_fiber->getState()!=Fiber::EXCEPT){
                        idle_fiber->setState(Fiber::HOLD);
                    }
                --m_idleThreadConunt;
            }
        }*/

        dwframe_LOG_DEBUG(g_logger) << m_name << " run";
        set_hook_enable(true);
        setThis();
        if(dwframe::GetThreadId() != m_rootThread) {
            t_Fiber = Fiber::GetThis().get();
        }

        Fiber::pointer idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        Fiber::pointer cb_fiber;

        FiberAndThread ft;
        while(true) {
            ft.reset();
            bool tickle_me = false;
            bool is_active = false;
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                while(it != m_fibers.end()) {
                    if(it->thread != -1 && it->thread != dwframe::GetThreadId()) {
                        ++it;
                        tickle_me = true;
                        continue;
                    }

                    dwframe_ASSERT(it->fiber || it->cb);
                    if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                        ++it;
                        continue;
                    }

                    ft = *it;
                    m_fibers.erase(it++);
                    ++m_activeThreadCount;
                    is_active = true;
                    break;
                }
                tickle_me |= it != m_fibers.end();
            }

            if(tickle_me) {
                tickle();
            }

            if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                            && ft.fiber->getState() != Fiber::EXCEPT)) {
                ft.fiber->swapIn();
                --m_activeThreadCount;

                if(ft.fiber->getState() == Fiber::READY) {
                    schedule(ft.fiber);
                } else if(ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT) {
                    ft.fiber->m_state = Fiber::HOLD;
                }
                ft.reset();
            } else if(ft.cb) {
                if(cb_fiber) {
                    cb_fiber->reset(ft.cb);
                } else {
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                ft.reset();
                cb_fiber->swapIn();
                --m_activeThreadCount;
                if(cb_fiber->getState() == Fiber::READY) {
                    schedule(cb_fiber);
                    cb_fiber.reset();
                } else if(cb_fiber->getState() == Fiber::EXCEPT
                        || cb_fiber->getState() == Fiber::TERM) {
                    cb_fiber->reset(nullptr);
                } else {//if(cb_fiber->getState() != Fiber::TERM) {
                    cb_fiber->m_state = Fiber::HOLD;
                    cb_fiber.reset();
                }
            } else {
                if(is_active) {
                    --m_activeThreadCount;
                    continue;
                }
                if(idle_fiber->getState() == Fiber::TERM) {
                    dwframe_LOG_INFO(g_logger) << "idle fiber term";
                    
                    break;
                }

                ++m_idleThreadCount;
                idle_fiber->swapIn();
                --m_idleThreadCount;
                if(idle_fiber->getState() != Fiber::TERM
                        && idle_fiber->getState() != Fiber::EXCEPT) {
                    idle_fiber->m_state = Fiber::HOLD;
                }
            }
        }
    }

    void Scheduler::idle() {
        //dwframe_LOG_INFO(g_logger) << "[ERROR] Scheduler::idle";
        //dwframe::Fiber::YieldToHold();
        /*while(!stopping()) {
            dwframe::Fiber::YieldToHold();
            //dwframe_LOG_INFO(g_logger) << "idle";
        }*/
        dwframe_LOG_INFO(g_logger) << "idle";
        while(!stopping()) {
            dwframe::Fiber::YieldToHold();
        }
    }

    void Scheduler::tickle(){
        //dwframe_LOG_INFO(g_logger) << "tickle";
    }
    bool Scheduler::stopping(){
        MutexType::Lock lock(m_mutex);
        return m_autoStop&&m_stoping&&m_fibers.empty()&&m_activeThreadCount==0;
    }
}