#pragma once

#include <memory>
#include "fiber.h"
#include "thread.h"
#include "list"
#include <atomic>
#include <vector>
#include <map>
#include "log.h"
namespace dwframe{
    class Scheduler{
    public:
        using pointer = std::shared_ptr<Scheduler>;
        using MutexType = Mutex;
    private:
        struct FiberAndThread
        {
            Fiber::pointer fiber;
            std::function<void()> cb;
            pid_t thread;

            FiberAndThread(Fiber::pointer f,int thr)
            : fiber(f)
            , thread(thr){

            }

            FiberAndThread(Fiber::pointer *f,int thr)
            : thread(thr){
                fiber.swap(*f);
            }

            FiberAndThread(std::function<void()> f,int thr)
            : cb(f)
            , thread(thr){

            }

            FiberAndThread(std::function<void()> *f,int thr)
            :thread(thr){
                cb.swap(*f);
            }

            FiberAndThread()
            :thread(-1){

            }
            void reset(){
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }

            
        };
    private:
        MutexType m_mutex;
        std::vector<Thread::pointer> m_threads;
        std::list<FiberAndThread> m_fibers;
        Fiber::pointer m_rootFiber;
        std::string m_name;
        std::map<int,std::list<FiberAndThread>> m_thrFibers;
    protected:
        std::vector<int> m_threadIds;
        size_t m_threadCount = 0;
        std::atomic<size_t> m_activeThreadCount = {0};
        std::atomic<size_t> m_idleThreadCount = {0};
        bool m_stoping = true;
        bool m_autoStop = false;
        
        pid_t m_rootThread=0;
    private:
        template<typename FiberOrcb>
        bool scheduleNolock(FiberOrcb fc,int thread){
           // MutexType::Lock lock(m_mutex);
            
            bool need_tickle = m_fibers.empty();
            FiberAndThread ft(fc,thread);
            if(ft.fiber|| ft.cb){
                m_fibers.push_back(ft);
                //dwframe_LOG_ERROR(dwframe_log_root())<<"m_fibers.size:"<<m_fibers.size();
            }
            return need_tickle;
        }
    protected:
        virtual void tickle();
        void run();
        virtual bool stopping();
        virtual void idle();
        void setThis();
        inline bool hasIdleThreads(){ return m_idleThreadCount>0; }
    public:
        Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
        virtual ~Scheduler();
        const std::string& getName()const {return m_name;}

        static Scheduler* GetThis();
        static Fiber* GetMainFiber();

        void start();
        void stop();
    
        template<typename FiberOrcb>
        void schedule(FiberOrcb fc,int thread =-1){
            bool  need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNolock(fc,thread);
            }
            
            if(need_tickle){
                tickle();
            }

        }

        template<typename InputIterator>
        void schedule(InputIterator begin,InputIterator end){
            bool  need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while(begin!=end){
                    need_tickle = scheduleNolock(&*begin,-1)||need_tickle;
                    ++begin;
                }
                
            }
            
            if(need_tickle){
                tickle();
            }

        }
    };
}