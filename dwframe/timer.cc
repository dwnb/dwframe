#include "timer.h"
#include "util.h"
#include "log.h"
namespace dwframe{
    static dwframe::Logger::pointer g_logger = dwframe_log_name("system");
    bool Timer::Comparator::operator()(const Timer::pointer& lhs,const Timer::pointer& rhs)const{
        if(!lhs && !rhs){
            return false;
        }

        if(!lhs){
            return true;
        }

        if(!rhs){
            return false;
        }

        if(lhs->m_next < rhs->m_next){
            return true;
        }

        if(rhs->m_next < lhs->m_next){
            return false;
        }

        return lhs.get() < rhs.get();
    }

    Timer::Timer(uint64_t ms,std::function<void()> cb,
                bool recurring,TimerManager* manager)
            :m_recurring(recurring)
            ,m_ms(ms)
            ,m_cb(cb)
            ,m_manager(manager){
        m_next = dwframe::GetCurrentMS() +m_ms;
    }

    Timer::Timer(uint64_t next)
    :m_next(next){

    }
    bool Timer::cancel(){
        TimerManager::RWLock::WriteLock lock(m_manager->m_mutex);
        if(m_cb){
            m_cb = nullptr;
            auto it = m_manager->m_timers.find(shared_from_this());
            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }
    bool Timer::refresh(){
        TimerManager::RWLock::WriteLock lock(m_manager->m_mutex);
        if(!m_cb){
            return false;
        }
        
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end()){
            return false;
        }

        m_manager->m_timers.erase(it);//注意理解此处为什么先删除在添加,因为
                                     //此处set是按照时间顺序排列的，不删除不会改变
                                     //其位置导致数据结构紊乱，set无法修改key值
        m_next = dwframe::GetCurrentMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());
        return true;
    }
    bool Timer::reset(uint64_t ms,bool form_now){
        if(ms == m_ms&&!form_now){
            return true;
        }

        TimerManager::RWLock::WriteLock lock(m_manager->m_mutex);
        if(!m_cb){
            return false;
        }
        
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end()){
            return false;
        }

        m_manager->m_timers.erase(it);//注意理解此处为什么先删除在添加,因为
                                     //此处set是按照时间顺序排列的，不删除不会改变
                                     //其位置导致数据结构紊乱，set无法修改key值
        
        uint64_t start = 0;
        if(form_now){
            start = dwframe::GetCurrentMS();
        }else{
            start = m_next -m_ms;
        }
        m_ms = ms;
        m_next =  start+ m_ms;
        m_manager ->addTimer(shared_from_this(),lock);
        m_manager ->m_timers.insert(shared_from_this());
        //m_manager->m_timers.insert(shared_from_this());
        return true;
    }
    TimerManager::TimerManager(/* args */){
        m_previousTime = dwframe::GetCurrentMS();
    }
    
    TimerManager::~TimerManager(){

    }
    Timer::pointer TimerManager::addtimer(uint64_t ms,std::function<void()>cb,
                            bool recurring){//添加定时器
        Timer::pointer timer(new Timer(ms,cb,recurring,this));
        RWLock::WriteLock lock(m_mutex);
        auto it = m_timers.insert(timer).first;//返回pair，第一个是位置，第二个是是否成功
        bool at_front = (it == m_timers.begin())&&!m_tickled;
        if(at_front){
            m_tickled = true;
        }
        if(at_front){
            onTimerInsertedAtFront();
        }
       // dwframe_LOG_INFO(g_logger)<<"errno";
        //m_mutex.unlock();
        //std::cout<<"unlock"<<std::endl;
        return timer;
    }
    static void Ontimer(std::weak_ptr<void> weak_cond,std::function<void()> cb){
        std::shared_ptr<void> tmp = weak_cond.lock();

        if(tmp){
            cb();
        }
    }
    Timer::pointer TimerManager::addConditionTimer(uint64_t ms,std::function<void()>cb,
                            std::weak_ptr<void> weak_cond,
                            bool recurring){ 
        return addtimer(ms,std::bind(&Ontimer,weak_cond,cb),recurring);                     
    }

    uint64_t TimerManager::getNextTimer(){
        RWLock::ReadLock lock(m_mutex);
        m_tickled = false;
        if(m_timers.empty()){
            return ~0ll;//0取反得到最大值
        }

        const Timer::pointer& next = *m_timers.begin();
        uint64_t now_ms = dwframe::GetCurrentMS();

        if(now_ms>=next->m_next){
            return 0;
        }else {
            return next->m_next-now_ms;
        }

        return 0;
    }
    void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs){
        uint64_t now_ms = dwframe::GetCurrentMS();
        std::vector<Timer::pointer> expired;//存放已经超时的
        
        {
            RWLock::ReadLock lock(m_mutex);
            if(m_timers.empty()){
                return;
            }
        }
        RWLock::WriteLock lock(m_mutex);
        if(m_timers.empty()) {
            return;
        }
        
        /*bool rollover = detectClockRollover(now_ms);
        if(!rollover && ((*m_timers.begin())->m_next>now_ms)){
            return ;
        }*/

        Timer::pointer now_timer(new Timer(now_ms));

        auto it = m_timers.upper_bound(now_timer);
        expired.insert(expired.begin(),m_timers.begin(),it);
        m_timers.erase(m_timers.begin(),it);
        cbs.reserve(expired.size());

        for(auto &timer:expired){
            cbs.push_back(timer->m_cb);
            if(timer->m_recurring){
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            }else{
                timer->m_cb = nullptr;
            }
        }
        //dwframe_LOG_INFO(g_logger)<<"errno";
    }
    void TimerManager::addTimer(Timer::pointer val,RWMutex::WriteLock& lock){
        auto it = m_timers.insert(val).first;//返回pair，第一个是位置，第二个是是否成功
        bool at_front = (it == m_timers.begin())&&!m_tickled;
        if(at_front){
            m_tickled = true;
        }
        if(at_front){
            onTimerInsertedAtFront();
        }
    }
    bool TimerManager::detectClockRollover(uint64_t now_ms){
        bool rollover = false;
        if(now_ms<m_previousTime&&now_ms<(m_previousTime - 60*60*1000)){
            rollover = true;
        }

        m_previousTime = now_ms;
        return rollover;
    }
    bool TimerManager::hasTimer(){
        RWLock::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }
}