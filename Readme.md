## 日志系统log

### 日志级别器：

```cpp
class LogLevel {
    public:
        
        enum Level{
            UNKNOW= 0, 
            DEBUG = 1,//调试
            INFO  = 2,//信息
            WARN  = 3,//警告
            ERROR = 4,//错误
            FATAL = 5//崩溃
        };

        static const char* ToString(LogLevel::Level Level);
        static LogLevel::Level FromString(const std::string& str);
    };
```

ToString与FromString写法精妙：利用宏替换的写法：[https://blog.csdn.net/yanggangclcsdn/article/details/49704089](https://blog.csdn.net/yanggangclcsdn/article/details/49704089)

### 日志事件器：

```cpp
    class LogEvent {
    public:
        using pointer = std::shared_ptr<LogEvent>;
    private:
        const char* m_file     = nullptr; //文件名
        int32_t     m_line     = 0;//行号
        uint32_t    m_elapse   = 0;//程序启动开始到现在毫秒数
        uint32_t    m_threadid = 0;//线程id
        uint32_t    m_fiberid  = 0;//协程id
        uint64_t    m_time     = 0;//时间戳
        int32_t     m_mtime    = 0;//时间戳
        
        std::stringstream m_ss;//消息内容
        std::shared_ptr<Logger> m_logger;
        LogLevel::Level m_level;
        std::string m_thread_name = 0; //线程名
    public:
        LogEvent(std::shared_ptr<Logger> logger,LogLevel::Level level,
                const char* file,int32_t line,uint32_t elapse, uint32_t thread_id,
                uint32_t fiber_id,const std::string& thread_name);
        //~LogEvent();不能只有声明没有实现
        const char* getFile()   const { return m_file; }
        int32_t     getLine()   const{ return m_line; }
        uint32_t    getElapse() const{ return m_elapse; }
        uint32_t    getThreadid()const{ return m_threadid; }
        uint32_t    getFiberid()const{ return m_fiberid; }
        uint64_t    getTime()   const{ return m_time; }
        int32_t     getmTime()   const{ return m_mtime; }
        std::string getContent()const{ return m_ss.str(); }
        std::stringstream& getSS() { return m_ss; }
        std::shared_ptr<Logger>  getLogger()const { return m_logger; }
        LogLevel::Level  getLevel()const { return m_level; }
        const std::string getThreadName()const {return m_thread_name;}
        void format(const char* fmt,...);
        void format(const char* fmt,va_list va);
    };
```

主要功能是保存日志的一些关键信息：日志器，文件名，行号，间隔时间，协程id，线程id，时间戳

- 日志事件包装器：

```cpp
 /**
     * @brief 日志时间包装器，写一个临时日志事件对象放在其中，利用智能能指针自动创建与释放内存，在包装器生命结束时，自动写入日志
     */
    class LogEventWrap {
    public:

        /**
         * @brief 构造函数
         * @param[in] e 日志事件
         */
        LogEventWrap(LogEvent::pointer e);

        /**
         * @brief 析构函数，包装器析构时输出日志内容
         */
        ~LogEventWrap();

        /**
         * @brief 获取日志事件
         */
        LogEvent::pointer getEvent() const { return m_event;}

        /**
         * @brief 获取日志内容流
         */
        std::stringstream& getSS();
    private:
        /**
         * @brief 日志事件
         */
        LogEvent::pointer m_event;
    };
```

主要功能是依据日志输出地输出日志到具体地方：具体过程入下：构造函数传入日志事件器，析构函数依据日志事件器里存放的日志器输出到具体日志器里；

### 日志格式器

```cpp
//日志格式器
    class LogFormatter {
    public:
        using pointer = std::shared_ptr<LogFormatter>; 
    public:
        class FormatItem{//抽象类
        public:
            using pointer = std::shared_ptr<FormatItem>;
            virtual ~FormatItem(){}
            //FormatItem(const std::string& fmt = "");
            //virtual std::string format(LogEvent::pointer event)=0;
            virtual void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event)=0;//相比上一行直接输出到流里性能更好
        };

        std::string m_pattern;
        std::vector<FormatItem::pointer> m_items;

        bool error = false;
    public:
        LogFormatter(const std::string &pattern);

        std::string format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event);
        void init();

        bool isError()const{return error;}  
        std::string getPattern()const{
            return m_pattern;
        } 
    };
```

注意构init()格式解析tuple的写法,在init()此处就体现了多态的思想,m_items存的类型是抽象父类FormatItem::pointer，而真实是寸的子类FormatItem。
tuple为<str,fmt,int> 0表示普通字符串，1表示要格式输出的字符，先依据0或1判断是否为fmt字符串，再依据str判断是哪一种。

### 日志输出地

一个抽象类，用来扩展子类，子类有控制台输出地，文件输出地
成员保含：

   1. 日志输出地的级别
   1. 日志输出地的日志格式器

成员函数包含：

   1. 日志输出
   1. 设置格式，获取格式

功能主要是保存日志输出地的各种信息及格式。

### 日志器

```cpp
    class Logger : public std::enable_shared_from_this<Logger>{
        friend class LoggerManager;
    public:
        using pointer = std::shared_ptr<Logger>;
        using MutexType = SpinLock;
    private:
        std::string m_name;      //日志名称
        LogLevel::Level m_level;//日志级别，满足日志级别的才会输出
        std::list<LogAppender::pointer>  m_appenders;//输出目地的集合
        LogFormatter::pointer m_formatter;//日志格式器
        Logger::pointer m_root;//根日志器
    public:
        MutexType m_mutex;
    public:
        Logger(const std::string name = "root");//初始化日志器，赋名
        void log(LogLevel::Level level, const LogEvent::pointer event);//打印普通日志信息
        void debug(LogEvent::pointer event);//打印debug信息
        void info(LogEvent::pointer event);
        void warn(LogEvent::pointer event);
        void error(LogEvent::pointer event);
        void fatal(LogEvent::pointer event);

        void addAppender(LogAppender::pointer appender);
        void delAppender(LogAppender::pointer appender);
        void clearAppends();
        void setFormatter(const LogFormatter::pointer logFormatter);
        void setFormatter(const std::string& logFormatter);
        std::string toYamlstring();
            
        LogFormatter::pointer getFormatter();
        const std::string& getName(){ return m_name;}
        LogLevel::Level getLevel() const{
            return m_level;
        }
        void setLevel(LogLevel::Level val){
            m_level = val;
        }
    };
```

### 控制台日志输出

```cpp
class StdoutLogAppender : public LogAppender{
    public:
        using pointer = std::shared_ptr<StdoutLogAppender>;
    public:
        virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override;//回顾下overide的用法
    };
```

### 文件日志输出

```cpp
//文件日志输出
    class FileoutLogAppender : public LogAppender{
    public:
        using pointer = std::shared_ptr<StdoutLogAppender>;
    private:
        std::string m_filename;
        std::ofstream m_filestream;
    public:
        FileoutLogAppender(const std::string &filename);
        std::string getFileName(){
            return m_filename;
        }
        bool reopen();
        virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override;//回顾下overide的用法
    };
```

### 日志管理器——单例模式

单例：

```cpp
 //指针单例模式
    template<typename T, typename x = void,int N=0>
    class Singleton{
        public:
        static T* Getinstance(){
            static T v;
            return &v;
        }
    };

    //智能指针单例模式
    template<typename T, typename x = void,int N=0>
    class SingletonPtr{
        public:
        static std::shared_ptr<T> Getinstance(){
            static std::shared_ptr<T> v;
            return v;
        }
    };
```

管理器：

```cpp
class LoggerManager{
    public:
        using MutexType = SpinLock;
        LoggerManager();
        Logger::pointer getLogger(const std::string& name);
        Logger::pointer Getlogger(const std::string& name);
        bool insertLogger(const std::string& name,const Logger::pointer &logger);
        bool insertLogger(const std::string& name);
        void init();
        void toString();//此处写法用的typeid

        Logger::pointer getRoot() const { return m_root; }
        
        MutexType m_mutex;
    private:
        std::map<std::string,Logger::pointer> m_loggers;
        Logger::pointer m_root;
    };

    using Loggermgr = Singleton<LoggerManager>;//重点
```

typeid用法：
[https://zh.cppreference.com/w/cpp/language/typeid](https://zh.cppreference.com/w/cpp/language/typeid)

![](https://cdn.jsdelivr.net/gh/dwnb/Picture/log.png)

## 配置模块——config观察者模式

## 线程封装、锁（互斥、读写、自旋）信号量封装

### 线程封装

```cpp
class Thread{
    public:
        using pointer = std::shared_ptr<Thread>; 
    private:
        Thread(const Thread &) = delete;
        Thread(Thread&) = delete;

        Thread& operator=(const Thread&) = delete;
        Thread& operator=(Thread&) = delete;
        static void* run(void *arg);//线程函数
    private:
        pid_t m_id = -1;//线程id
        pthread_t m_thread = 0;
        std::function<void()> m_cb;//回调函数
        std::string m_name;

        Semaphore m_semaphore;//信号量
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
```

m_cb可以利用std::bind()巧妙实现“无参”传递函数
具体流程如下图所示：
![](https://cdn.jsdelivr.net/gh/dwnb/Picture/thread.png)

### 禁止copy类——Noncopyable

- 删除其拷贝构造
- 删除其拷贝赋值

```cpp
class Noncopyable {
public:
    
    Noncopyable() = default;

    
    ~Noncopyable() = default;

    
    Noncopyable(const Noncopyable&) = delete;

    
    Noncopyable& operator=(const Noncopyable&) = delete;
};
```

### 防止被继承方法：

[https://blog.csdn.net/docblue/article/details/8548297](https://blog.csdn.net/docblue/article/details/8548297)

### 锁（读写，互斥，自旋）

- 锁都是禁止copy的类,继承自Noncopyable
- 封装锁的方法：

利用C++构造即初始化与析构释放资源的特性，构造函数利用初始化锁，析构函数销毁锁。

- 加锁，解锁的封装：

同样利用C++构造即初始化与析构释放资源的特性，构造函数加锁，析构函数释放锁，这样很好的避免死锁，因为利用析构函数释放资源的特性，局部作用域创建对象加锁，即使离开作用域未解锁也会因为对象销毁把所释放掉。很好的解决了脱离作用域忘记释放锁的问题。

### 信号量

```cpp
class Semaphore: Noncopyable{
    public:
        Semaphore(uint32_t count =0);
        ~Semaphore();

        void wait();
        void notify();
    private:
      sem_t m_semaphore;  
};
```

构造及初始化，析构释放资源。