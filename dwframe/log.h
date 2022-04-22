#ifndef __DWFRAME_LOG_H__
#define __DWFRAME_LOG_H__

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <vector>
#include <tuple>
#include <iostream>
#include <string.h>
#include <sstream>//这俩文件注意学习
#include <fstream>//
#include <sys/time.h>
#include <map>
#include "util.h"
#include "singleton.h"
#include "thread.h"


/**
 * @brief 使用流式方式将日志级别level的日志写入到logger，借助临时对象的方法，创建出临时包装器
 * 包装器析构时把日志内容输出
 */
#define dwframe_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        dwframe::LogEventWrap(dwframe::LogEvent::pointer(new dwframe::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, dwframe::GetThreadId(),\
                dwframe::GetFiberId(),dwframe::Thread::GetName()))).getSS()

/**
 * @brief 使用流式方式将日志级别debug的日志写入到logger
 */
#define dwframe_LOG_DEBUG(logger) dwframe_LOG_LEVEL(logger, dwframe::LogLevel::DEBUG)

/**
 * @brief 使用流式方式将日志级别info的日志写入到logger
 */
#define dwframe_LOG_INFO(logger) dwframe_LOG_LEVEL(logger, dwframe::LogLevel::INFO)

/**
 * @brief 使用流式方式将日志级别warn的日志写入到logger
 */
#define dwframe_LOG_WARN(logger) dwframe_LOG_LEVEL(logger, dwframe::LogLevel::WARN)

/**
 * @brief 使用流式方式将日志级别error的日志写入到logger
 */
#define dwframe_LOG_ERROR(logger) dwframe_LOG_LEVEL(logger, dwframe::LogLevel::ERROR)

/**
 * @brief 使用流式方式将日志级别fatal的日志写入到logger
 */
#define dwframe_LOG_FATAL(logger) dwframe_LOG_LEVEL(logger, dwframe::LogLevel::FATAL)

/**
 * @brief 使用格式化方式将日志级别level的日志写入到logger
 */
#define dwframe_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        dwframe::LogEventWrap(dwframe::LogEvent::pointer(new dwframe::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, dwframe::GetThreadId(),\
                dwframe::GetFiberId(),dwframe::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别debug的日志写入到logger
 */
#define dwframe_LOG_FMT_DEBUG(logger, fmt, ...) dwframe_LOG_FMT_LEVEL(logger, dwframe::LogLevel::DEBUG, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别info的日志写入到logger
 */
#define dwframe_LOG_FMT_INFO(logger, fmt, ...)  dwframe_LOG_FMT_LEVEL(logger, dwframe::LogLevel::INFO, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别warn的日志写入到logger
 */
#define dwframe_LOG_FMT_WARN(logger, fmt, ...)  dwframe_LOG_FMT_LEVEL(logger, dwframe::LogLevel::WARN, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别error的日志写入到logger
 */
#define dwframe_LOG_FMT_ERROR(logger, fmt, ...) dwframe_LOG_FMT_LEVEL(logger, dwframe::LogLevel::ERROR, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别fatal的日志写入到logger
 */
#define dwframe_LOG_FMT_FATAL(logger, fmt, ...) dwframe_LOG_FMT_LEVEL(logger, dwframe::LogLevel::FATAL, fmt, __VA_ARGS__)

/**
 * @brief 获取主日志器
 */
#define dwframe_LOG_ROOT() dwframe::LoggerMgr::GetInstance()->getRoot()

/**
 * @brief 获取name的日志器
 */
#define dwframe_LOG_NAME(name) dwframe::LoggerMgr::GetInstance()->getLogger(name)

#define dwframe_log_root() dwframe::Loggermgr::Getinstance()->getRoot()
#define dwframe_log_name(name) dwframe::Loggermgr::Getinstance()->getLogger(name)
#define GetLogger(name) dwframe::Loggermgr::Getinstance()->Getlogger(name)

namespace dwframe {
    class Logger;
    class LoggerManager;
    //日志级别
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


    //日志输出地
    class LogAppender {
    public:
        using pointer = std::shared_ptr<LogAppender>;
        using MutexType = SpinLock;
    protected: 
        LogLevel::Level m_level = LogLevel::DEBUG;
        LogFormatter::pointer m_formatter;
        bool has_formatter = false;
    public:
        MutexType m_mutex;
    public:
        virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event)=0;//回顾下overide的用法
        void setFormatter(LogFormatter::pointer val){
            MutexType::Lock lock(m_mutex);
            m_formatter = val;
            if(m_formatter){
                has_formatter = true;
            }else {
                has_formatter = false;
            }
        }
        void SetFormatter(LogFormatter::pointer val){
            MutexType::Lock lock(m_mutex);
            m_formatter = val;
        }
        bool hasFormatter(){
            return has_formatter;
        }
        LogFormatter::pointer getFormatter(){
            MutexType::Lock lock(m_mutex);
            return m_formatter;
        }

        LogLevel::Level getLevel(){ return m_level; }//只有一条语句原子类型，复杂类型（类多个成员）的线程安全可能会引起内存错误
        void setLevel(LogLevel::Level level){ m_level = level; }
        std::string toYamlstring();
        /*写成虚函数的目的：
        * 日志输出地有很多，会有大量继承，
        * 如果不定义为虚函数，子类析构时可能会产生内存释放问题
        */
        virtual ~LogAppender() {}
    };


    //日志器
    class Logger : public std::enable_shared_from_this<Logger>{
        friend class LoggerManager;
    public:
        using pointer = std::shared_ptr<Logger>;
        using MutexType = SpinLock;
    private:
        std::string m_name;      //日志名称
        LogLevel::Level m_level;//日志级别，满足日志级别的才会输出
        std::list<LogAppender::pointer>  m_appenders;//输出目地的集合
        LogFormatter::pointer m_formatter;
        Logger::pointer m_root;
    public:
        MutexType m_mutex;
    public:
        Logger(const std::string name = "root");
        void log(LogLevel::Level level, const LogEvent::pointer event);
        void debug(LogEvent::pointer event);
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


    //控制台日志输出
    class StdoutLogAppender : public LogAppender{
    public:
        using pointer = std::shared_ptr<StdoutLogAppender>;
    public:
        virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override;//回顾下overide的用法
    };
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

    class LoggerManager{
    public:
        using MutexType = SpinLock;
        LoggerManager();
        Logger::pointer getLogger(const std::string& name);
        Logger::pointer Getlogger(const std::string& name);
        bool insertLogger(const std::string& name,const Logger::pointer &logger);
        bool insertLogger(const std::string& name);
        void init();
        void toString();

        Logger::pointer getRoot() const { return m_root; }
        
        MutexType m_mutex;
    private:
        std::map<std::string,Logger::pointer> m_loggers;
        Logger::pointer m_root;
    };

    using Loggermgr = Singleton<LoggerManager>;

}


#endif // !__FRAME