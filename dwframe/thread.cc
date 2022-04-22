#include "thread.h"
#include "log.h"
#include "util.h"
namespace dwframe{

//定义系统日志
static dwframe::Logger::pointer sysLog = dwframe_log_name("system");
//线程局部变量用好了有很多好处,在线程中多出一个此变量的副本
static thread_local Thread* t_thread = nullptr;//线程局部变量，指向当前线程
static thread_local std::string t_thread_name = "UNKNOW";//线程局部变量，指向当前线程名称

Semaphore::Semaphore(uint32_t count ){
    /* If  pshared  has  the  value  0,  then the semaphore is shared between the threads of a process, and
       should be located at some address that is visible to all threads (e.g.,  a  global  variable,  or  a
       variable allocated dynamically on the heap).

       If  pshared  is  nonzero, then the semaphore is shared between processes, and should be located in a
       region of shared memory (see shm_open(3), mmap(2),  and  shmget(2)).   (Since  a  child  created  by
       fork(2)  inherits its parent's memory mappings, it can also access the semaphore.)  Any process that
       can access the shared memory region can operate on the semaphore using sem_post(3), sem_wait(3), and
       so on.
    */
     if(sem_init(&m_semaphore,0,count)){
         throw std::logic_error("sem_init error");
     }
}
Semaphore::~Semaphore(){
    sem_destroy(&m_semaphore);
}
void Semaphore::wait(){
    if(sem_wait(&m_semaphore)){
        throw std::logic_error("sem_wait error");
    }
}
void Semaphore::notify(){
    if(sem_post(&m_semaphore)){
        throw std::logic_error("sem_post error");
    }
}

void Thread::SetName(const std::string& name){
    if(t_thread){
        t_thread->m_name = name;
    }
    t_thread_name = name;
}
Thread* Thread::GetThis(){//获取当前线程
    return t_thread;
}
const std::string& Thread::GetName(){//给日志使用获取当前线程名称
    return t_thread_name;
}

Thread::Thread(std::function<void()> cb,const std::string& name)
    :m_cb(cb){
    
    if(name.empty()){
        m_name = "UNKNOW";
    }else{
        m_name = name;
    }

    int rt = pthread_create(&m_thread,nullptr,&Thread::run,this);
    if(rt){
        dwframe_LOG_ERROR(sysLog)<< "pthread_create thread fail, rt=" <<rt <<"name="<<name;
        throw std::logic_error("pthread_create error");
    }//else{
       // dwframe_LOG_INFO(sysLog)<< "pthread_create success!" ;
    //}
    
    m_semaphore.wait();
}
Thread::~Thread(){
    if(m_thread){
        pthread_detach(m_thread);
    }
}
void* Thread::run(void *arg){
    Thread* _thread = (Thread*)arg;
    t_thread = _thread;
    t_thread->m_id = dwframe::GetThreadId();
    t_thread_name = t_thread->m_name;
    pthread_setname_np(pthread_self(),_thread->m_name.substr(0,15).c_str());

    std::function<void()> cb;//防止智能指针的存在无法释放内存
    cb.swap(_thread->m_cb);

    //static方法没有this指针所以不能直接调用m_semaphore
    _thread->m_semaphore.notify();
    cb();
    return 0;
}
void Thread::join(){
    if(m_thread){
        int rt = pthread_join(m_thread,nullptr);
        if(rt){
            dwframe_LOG_ERROR(sysLog)<< "pthread_join thread fail, rt=" <<rt 
            <<"m_name= "<<m_name;
        throw std::logic_error("pthread_create error");
        }
        m_thread =0;
    }
}
}