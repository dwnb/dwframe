#ifndef _DWFRAME_THREAD_H
#define _DWFRAME_THREAD_H

#include <thread>
#include <pthread.h>
#include <functional>
#include <memory>
#include <string>
#include <semaphore.h>
#include <mutex>
#include "iostream"
#include "Noncopyable.h"
//#include "log.h"

//static_assert(__cplusplus == 201402, "");
namespace dwframe{
    //static dwframe::Logger::pointer g_logger = dwframe_log_name("system");
    
    class Semaphore: Noncopyable{
    public:
        Semaphore(uint32_t count =0);
        ~Semaphore();

        void wait();
        void notify();
    private:
      sem_t m_semaphore;  
    };
    
    //类的构造加锁，析构解锁
    template<class T>
    struct ScopedLockImpl{
    public:
        ScopedLockImpl(T& mutex)
            :m_mutex(mutex){
                m_mutex.lock();
                m_locked=true;
        }
        ~ScopedLockImpl(){
                m_mutex.unlock();
        }

        void lock(){
            if(!m_locked){
                m_mutex.lock();
                m_locked=true;
            }
        }

        void unlock(){
            if(m_locked){
                m_mutex.unlock();
                m_locked=false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
    };
    class Mutex:Noncopyable{
    private:
        pthread_mutex_t mutex;
    public:
        using Lock = ScopedLockImpl<Mutex>;

        Mutex(){
            pthread_mutex_init(&mutex,nullptr);
        };
        ~Mutex(){
            pthread_mutex_destroy(&mutex);
        };

        void lock(){
            pthread_mutex_lock(&mutex);
        }


        void unlock(){
            pthread_mutex_unlock(&mutex);
        }
    };
    template<class T>
    struct ReadScopedLockImpl{
    public:
        ReadScopedLockImpl(T& mutex)
            :m_mutex(mutex){
                m_mutex.rdlock();
                m_locked=true;
        }
        ~ReadScopedLockImpl(){
                m_mutex.unlock();
        }

        void lock(){
            if(!m_locked){
                m_mutex.rdlock();
                m_locked=true;
            }
        }

        void unlock(){
            if(m_locked){
                m_mutex.unlock();
                m_locked=false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
    };

    template<class T>
    struct WriteScopedLockImpl{
    public:
        WriteScopedLockImpl(T& mutex)
            :m_mutex(mutex){
                m_mutex.wrlock();
                m_locked=true;
        }
        ~WriteScopedLockImpl(){
                unlock();
        }

        void lock(){
            if(!m_locked){
                m_mutex.wrlock();
                m_locked=true;
            }
        }

        void unlock(){
            if(m_locked){
                m_mutex.unlock();
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
    };
    class RWMutex:Noncopyable{//本来想用C++17的shared_mutex，但不太熟
    private:
        pthread_rwlock_t rw_mutex;
    public:
        using ReadLock = ReadScopedLockImpl<RWMutex>;
        using WriteLock = WriteScopedLockImpl<RWMutex>;

        RWMutex(){
            pthread_rwlock_init(&rw_mutex,nullptr);
        };
        ~RWMutex(){
            pthread_rwlock_destroy(&rw_mutex);
        };

        void rdlock(){
            pthread_rwlock_rdlock(&rw_mutex);
        }

        void wrlock(){
            pthread_rwlock_wrlock(&rw_mutex);
        }

        void unlock(){
            pthread_rwlock_unlock(&rw_mutex);
            
        }
    };
    //利用自旋锁提升性能
    class SpinLock:Noncopyable{
    public:
        using Lock = ScopedLockImpl<SpinLock>;
        SpinLock(){
            pthread_spin_init(&m_mutex,0);
        }
        ~SpinLock(){
            pthread_spin_destroy(&m_mutex);
        }

        void lock(){
            pthread_spin_lock(&m_mutex);
        }
        void unlock(){
            pthread_spin_unlock(&m_mutex);
        }
    private:
        pthread_spinlock_t m_mutex;
    };
    class Thread{
    public:
        using pointer = std::shared_ptr<Thread>; 
    private:
        Thread(const Thread &) = delete;
        Thread(Thread&) = delete;

        Thread& operator=(const Thread&) = delete;
        Thread& operator=(Thread&) = delete;
        static void* run(void *arg);
    private:
        pid_t m_id = -1;
        pthread_t m_thread = 0;
        std::function<void()> m_cb;
        std::string m_name;

        Semaphore m_semaphore;
    public:
        Thread(std::function<void()> cb,const std::string& name);
        ~Thread();
        
        const std::string& getName()const{return m_name;}
        pid_t getID()const{
            return m_id;
        }
        void join();

        static Thread* GetThis();//获取当前线程
        static const std::string& GetName();//给日志使用获取当前线程名称
        static void SetName(const std::string& name);
    };
}

#endif // _DWFRAME_THREAD_H
