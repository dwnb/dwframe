#include "log.h"

#include "map"
#include "functional"
#include "stdarg.h"
#include "config.h"
#include "typeinfo"
namespace dwframe{
    const char* LogLevel::ToString(LogLevel::Level level){
        switch (level){
#define xx(name) \
        case LogLevel::name:\
            return #name;\
            break;
        xx(DEBUG);
        xx(INFO);
        xx(WARN);
        xx(ERROR);
        xx(FATAL);
#undef xx
            default:
            return "UNKNOW";
        }
        return "UNKNOW";

    }
    LogLevel::Level LogLevel::FromString(const std::string& str) {
#define XX(level, v) \
    if(str == #v) { \
        return LogLevel::level; \
    }
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::UNKNOW;
#undef XX
}

    class MessageFormatItem : public LogFormatter::FormatItem{
    public:
        MessageFormatItem(const std::string& str = "") {}
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem{
    public:
        LevelFormatItem(const std::string& str = "") {}
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            os << LogLevel::ToString(level);
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem{
    public:
        ElapseFormatItem(const std::string& str = "") {}
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            os << event->getElapse();
        }
    };

    class NameFormatItem : public LogFormatter::FormatItem{
    public:
        NameFormatItem(const std::string& str = "") {}
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            os << event->getLogger()->getName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem{
    public:
        ThreadIdFormatItem(const std::string& str = "") {}
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            os << event->getThreadid();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem{
    public:
        FiberIdFormatItem(const std::string& str = "") {}
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            os << event->getFiberid();
        }
    };

    

    class DateTimeFormatItem : public LogFormatter::FormatItem{
    public:
        DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
            : m_format(format){//此处若format使用默认值，并不会在此处初始化m_format
            if(m_format.empty()) m_format = "%Y-%m-%d %H:%M:%S";
        }
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            tm tm;
            time_t time = event->getTime();
            localtime_r(&time,&tm);
            char strtime[64];
            strftime(strtime,sizeof(strtime),m_format.c_str(),&tm);
            os << strtime;
        }
    private:
        std::string m_format;
    };

    class DatemTimeFormatItem : public LogFormatter::FormatItem{
    public:
        DatemTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
            : m_format(format){//此处若format使用默认值，并不会在此处初始化m_format
            if(m_format.empty()) m_format = "%Y-%m-%d %H:%M:%S";
        }
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            tm tm;
            time_t time = event->getTime();
            localtime_r(&time,&tm);
            char strtime[64];
            strftime(strtime,sizeof(strtime),m_format.c_str(),&tm);
            os << strtime;
            if(event->getmTime()<100&&event->getmTime()>=10){
                os << ":0" << std::to_string(event->getmTime());
            }else if(event->getmTime()<10){
                os << ":00" << std::to_string(event->getmTime());
            }else{
                os << ':' << std::to_string(event->getmTime());
            }
        }
    private:
        std::string m_format;
    };

    class FileNameFormatItem : public LogFormatter::FormatItem{
    public:
        FileNameFormatItem(const std::string& str = "") {}
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            os << event->getFile();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem{
    public:
        LineFormatItem(const std::string& str = "") {}
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            os << event->getLine();
        }
    };
    class NewLineFormatItem : public LogFormatter::FormatItem{
    public:
        NewLineFormatItem(const std::string& str = "") {}
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            os << std::endl;
        }
    };

    class TabFormatItem : public LogFormatter::FormatItem{
    public:
        TabFormatItem(const std::string& str = "") {}
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            os << '\t';
        }
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem{
    public:
        ThreadNameFormatItem(const std::string& str = "") {}
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            os << event->getThreadName();
        }
    };



    class StringFormatItem : public LogFormatter::FormatItem{
    public:
        StringFormatItem(const std::string& str = "") 
            : m_string(str){

        }
        void format(std::ostream &os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) override{
            os << m_string;
        }
    private:
        std::string m_string;
    };

    LogEvent::LogEvent(std::shared_ptr<Logger> logger,LogLevel::Level level,
            const char* file,int32_t line,uint32_t elapse, uint32_t thread_id,
            uint32_t fiber_id,const std::string& thread_name)
        : m_file(file)
        , m_line(line)
        , m_elapse(elapse)
        , m_threadid(thread_id)
        , m_fiberid(fiber_id)
        , m_logger(logger)
        , m_level(level)
        , m_thread_name(thread_name){//必须按照定义顺序初始化

        timeval tv;
        gettimeofday(&tv,NULL); 
        m_time = tv.tv_sec;
        m_mtime= tv.tv_usec/1000;
    }

    void LogEvent::format(const char* fmt,...){
        va_list args;
        va_start(args,fmt);
        format(fmt,args);
        va_end(args);
    }
    void LogEvent::format(const char* fmt,va_list va){
        char *buf=nullptr;
        int len = vasprintf(&buf,fmt,va);
        if(len>-1){
            m_ss<<std::string(buf,len);
            free(buf);
        }
        
    }
    Logger::Logger(const std::string name)
        :m_name(name)
        ,m_level(LogLevel::DEBUG){
            m_formatter.reset(new LogFormatter("[%D{%Y-%m-%d %H:%M:%S}] [T:%N] [%t] [F:%F] [%c]%T[%p]%T[%f: %l]  %T%m %n"));

        //if(name == "root") m_appenders.push_back(StdoutLogAppender::pointer(new StdoutLogAppender));
        
    }
    void Logger::addAppender(LogAppender::pointer appender){
        MutexType::Lock lock(m_mutex);
        if(!appender->getFormatter()){//判断新增的appender是否有formatter没有把自己的formatter给予
            //MutexType::Lock ll(appender->m_mutex);
            appender->SetFormatter(m_formatter);
        }
        m_appenders.push_back(appender);
    }
    void Logger::delAppender(LogAppender::pointer appender){
        MutexType::Lock lock(m_mutex);
        //是否可以用set或者map重构
        for(auto it = m_appenders.begin();it != m_appenders.end();++it){
            if(*it == appender){
                m_appenders.erase(it);
                break;
            }
        }
    }
    void Logger::clearAppends(){
        MutexType::Lock lock(m_mutex);
        m_appenders.clear();
    }
    void Logger::log(LogLevel::Level level, const LogEvent::pointer event){
        if(level >= m_level){//如果大于日志器的日志级别则把数据传递到每个输出地
            auto self = shared_from_this();//?????
            MutexType::Lock lock(m_mutex);
            if(!m_appenders.empty()){
                for(auto &data : m_appenders){
                    data->log(self,level,event);
                }
            }else if(m_root){
                m_root->log(level,event);
            }
            
        }
    }

    void Logger::setFormatter(const LogFormatter::pointer logFormatter){
        MutexType::Lock lock(m_mutex);
        m_formatter = logFormatter;
        for(auto& i : m_appenders) {
           // std::cout <<"****"<<i->hasFormatter()<<std::endl;
           //MutexType::Lock ll(i->m_mutex);
            if(!i->hasFormatter()) {
                i->SetFormatter(m_formatter);
            }
        }
    }
    void Logger::setFormatter(const std::string& logFormatter){
        LogFormatter::pointer logf(new LogFormatter(logFormatter));
        if(logf->isError()){
            std::cout <<"Logger setFormatter name = " <<m_name<<"\n vlaue = "
                        <<logFormatter<<" invalid formatter\n";
            return;
        }
        setFormatter(logf);
    }
    LogFormatter::pointer Logger::getFormatter(){
        MutexType::Lock lock(m_mutex);
        return m_formatter;
    }
    void Logger::debug(LogEvent::pointer event){
        log(LogLevel::DEBUG,event);
    }
    void Logger::info(LogEvent::pointer event){
        log(LogLevel::INFO,event);
    }
    void Logger::warn(LogEvent::pointer event){
        log(LogLevel::WARN,event);
    }
    void Logger::error(LogEvent::pointer event){
        log(LogLevel::ERROR,event);
    }
    void Logger::fatal(LogEvent::pointer event){
        log(LogLevel::FATAL,event);
    }

    LogEventWrap::LogEventWrap(LogEvent::pointer e)
        : m_event(e){
        
    }
    LogEventWrap::~LogEventWrap(){
        m_event->getLogger()->log(m_event->getLevel(),m_event);
    }
    std::stringstream& LogEventWrap::getSS(){
        return m_event->getSS();
    }
    FileoutLogAppender::FileoutLogAppender(const std::string &filename)
        : m_filename(filename){
            reopen();
    }
    
    bool FileoutLogAppender::reopen(){
        MutexType::Lock lock(m_mutex);
        if(m_filename.c_str()){
            m_filestream.close();
        }
        m_filestream.open(m_filename,std::fstream::out | std::fstream::app);
        return !!m_filestream;
    }


    void FileoutLogAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) {//回顾下overide的用法
        if(level >= m_level){
            MutexType::Lock lock(m_mutex);
            m_filestream<< m_formatter->format(logger,level,event);
        }
    }
    void StdoutLogAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event) {//回顾下overide的用法
        if(level >= m_level){
            MutexType::Lock lock(m_mutex);
            std::cout<< m_formatter->format(logger,level,event);
        }
    }

    LogFormatter::LogFormatter(const std::string &pattern)
        : m_pattern(pattern){
            init();
    }
    std::string LogFormatter::format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::pointer event){
        std::stringstream ss;
        for(auto &data : m_items){
            data->format(ss,logger,level,event);
        }
        return ss.str();
    }

    //格式化日志
   //普通字符串为格式0
   //%xxx为格式1
   //%xxx{XXX}为格式1
    void LogFormatter::init(){
        //string,format,type:分别表示格式字符串%n、%m，format表示{}里的格式，type表示格式类型，
        std::vector<std::tuple<std::string,std::string,int>> vec;
        std::string nstr;
        for(size_t i = 0;i < m_pattern.size();++i){
            if(m_pattern[i] != '%'){
                nstr.push_back(m_pattern[i]);
                continue;
            }

            if((i + 1) < m_pattern.size() && m_pattern[i + 1] == '%') {
                    nstr.append(1, '%');
                    i++;//跳过真实%；
                    continue;
            }

            size_t n = i + 1;
            int fmt_status = 0;
            size_t fmt_begin = 0;

            std::string str;
            std::string fmt;
            while(n < m_pattern.size()) {
                if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                        && m_pattern[n] != '}')) {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }
                if(fmt_status == 0) {
                    if(m_pattern[n] == '{') {
                        str = m_pattern.substr(i + 1, n - i - 1);
                        //std::cout << "*" << str << std::endl;
                        fmt_status = 1; //解析格式
                        fmt_begin = n;
                        ++n;
                        continue;
                    }
                } else if(fmt_status == 1) {
                    if(m_pattern[n] == '}') {
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        //std::cout << "#" << fmt << std::endl;
                        fmt_status = 0;
                        ++n;
                        break;
                    }
                }
                ++n;
                if(n == m_pattern.size()) {
                    if(str.empty()) {
                        str = m_pattern.substr(i + 1);
                    }
                }
            }

            if(fmt_status == 0) {
                if(!nstr.empty()) {
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                    nstr.clear();
                }
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n - 1;
            } else if(fmt_status == 1) {
                std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                error = true;
                vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
            }
        }

        if(!nstr.empty()){
            vec.push_back(std::make_tuple(nstr,std::string(),0));
            nstr.clear();
        }

        static std::map<std::string, std::function<FormatItem::pointer(const std::string& str)>> s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::pointer(new C(fmt));}}
        XX(m, MessageFormatItem),           //m:消息
        XX(p, LevelFormatItem),             //p:日志级别
        XX(r, ElapseFormatItem),            //r:累计毫秒数
        XX(c, NameFormatItem),              //c:日志名称
        XX(t, ThreadIdFormatItem),          //t:线程id
        XX(n, NewLineFormatItem),           //n:换行
        XX(d, DateTimeFormatItem),          //d:时间
        XX(f, FileNameFormatItem),          //f:文件名
        XX(l, LineFormatItem),              //l:行号
        XX(T, TabFormatItem),               //T:Tab
        XX(F, FiberIdFormatItem),           //F:协程id
        XX(D, DatemTimeFormatItem),          //D:输出ms时间戳
        XX(N, ThreadNameFormatItem)        //N:线程名称
#undef XX
        };

        for(auto& i : vec) {
        if(std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::pointer(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::pointer(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));//写法牛逼，调用的是function绑定的函数，返回值是FormatItem类型
            }
        }

        //std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
    //std::cout << m_items.size() << std::endl;
        /*
        %p：输出日志信息的优先级，即DEBUG，INFO，WARN，ERROR，FATAL。
        %d：输出日志时间点的日期或时间，默认格式为ISO8601，也可以在其后指定格式，如：%d{yyyy/MM/dd HH:mm:ss,SSS}。
        %r：输出自应用程序启动到输出该log信息耗费的毫秒数。
        %t：输出产生该日志事件的线程名。
        %l：输出日志事件的发生位置，相当于%c.%M(%F:%L)的组合，包括类全名、方法、文件名以及在代码中的行数。例如：test.TestLog4j.main(TestLog4j.java:10)。
        %c：输出日志信息所属的类目，通常就是所在类的全名。
        %M：输出产生日志信息的方法名。
        %F：输出日志消息产生时所在的文件名称。
        %L:：输出代码中的行号。
        %m:：输出代码中指定的具体日志信息。
        %n：输出一个回车换行符，Windows平台为"rn"，Unix平台为"n"。
        %x：输出和当前线程相关联的NDC(嵌套诊断环境)，尤其用到像java servlets这样的多客户多线程的应用中。
        %%：输出一个"%"字符。
        */
        
    }

    LoggerManager::LoggerManager(){
        m_root.reset(new Logger);

        m_root->addAppender(LogAppender::pointer(new StdoutLogAppender));

        m_loggers[m_root->m_name] = m_root;
        init();
    }
    struct LogAppenderDefine{
        int type = 0;//0file//2stdout
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::string file;
        bool operator == (const LogAppenderDefine& oth)const{
            return  type == oth.type&&
                    level ==oth.level&&
                    formatter == oth.formatter&&
                    file == oth.file;
        }
    };
    struct LogDefine{
        std::string name;
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;

        std::vector<LogAppenderDefine> appenders;

        bool operator == (const LogDefine& oth)const{
            return  name == oth.name&&
                    level ==oth.level&&
                    formatter == oth.formatter&&
                    appenders == oth.appenders;
        }
        /*
        *template < class T,                        // set::key_type/value_type
           class Compare = less<T>,        // set::key_compare/value_compare
           class Alloc = allocator<T>      // set::allocator_type
           > class set;
        * 代码第二行可知set使用默认模板参数添加数据时，T一定有重载的<
        */
        bool operator < (const LogDefine& oth)const{
            return  name < oth.name;
        }
    };
    
    dwframe::ConfigVar<std::set<LogDefine>>::pointer g_log_define
                       = dwframe::Config::LookUp("logs",std::set<LogDefine>(),"logs config");
    template<>
class LexicalCast<std::string, LogDefine> {
public:
    LogDefine operator()(const std::string& v) {
        YAML::Node n = YAML::Load(v);
        LogDefine ld;
        if(!n["name"].IsDefined()) {
            std::cout << "log config error: name is null, " << n
                      << std::endl;
            throw std::logic_error("log config name is null");
        }
        ld.name = n["name"].as<std::string>();
        ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
        if(n["formatter"].IsDefined()) {
            ld.formatter = n["formatter"].as<std::string>();
        }

        if(n["appenders"].IsDefined()) {
            //std::cout << "==" << ld.name << " = " << n["appenders"].size() << std::endl;
            for(size_t x = 0; x < n["appenders"].size(); ++x) {
                auto a = n["appenders"][x];
                if(!a["type"].IsDefined()) {
                    std::cout << "log config error: appender type is null, " << a
                              << std::endl;
                    continue;
                }
                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if(type == "FileLogAppender") {
                    lad.type = 1;
                    if(!a["file"].IsDefined()) {
                        std::cout << "log config error: fileappender file is null, " << a
                              << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else if(type == "StdoutLogAppender") {
                    lad.type = 2;
                    if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else {
                    std::cout << "log config error: appender type is invalid, " << a
                              << std::endl;
                    continue;
                }
                if(a["level"].IsDefined()){
                    lad.level = LogLevel::FromString(a["level"].as<std::string>());
                }else{
                    lad.level = ld.level;
                }

                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};

    template<>
    class LexicalCast<LogDefine, std::string> {
    public:
        std::string operator()(const LogDefine& i) {
            YAML::Node n;
            n["name"] = i.name;
            if(i.level != LogLevel::UNKNOW) {
                n["level"] = LogLevel::ToString(i.level);
            }
            if(!i.formatter.empty()) {
                n["formatter"] = i.formatter;
            }

            for(auto& a : i.appenders) {
                YAML::Node na;
                if(a.type == 1) {
                    na["type"] = "FileLogAppender";
                    na["file"] = a.file;
                } else if(a.type == 2) {
                    na["type"] = "StdoutLogAppender";
                }
                if(a.level != LogLevel::UNKNOW) {
                    na["level"] = LogLevel::ToString(a.level);
                }

                if(!a.formatter.empty()) {
                    na["formatter"] = a.formatter;
                }

                n["appenders"].push_back(na);
            }
            std::stringstream ss;
            ss << n;
            return ss.str();
        }
    };

    struct LogIniter{
        LogIniter(){
            g_log_define->addListener([](const std::set<LogDefine>& old_value
                            ,const std::set<LogDefine>& new_value){
                            dwframe_LOG_INFO(dwframe_log_root()) <<"logger_conf_changed";
                           //新增
                            for(auto &i:new_value){
                                auto it = old_value.find(i);
                                 dwframe::Logger::pointer logger;
                                if(it == old_value.end()){
                                    //新增logger
                                    logger = dwframe_log_name(i.name);
                                }else{
                                    if(!(i==*it)){
                                        //修改logger
                                        logger = dwframe_log_name(i.name);
                                    } 
                                }
                                logger->setLevel(i.level);
                                if(!i.formatter.empty()){
                                    logger->setFormatter(i.formatter);
                                }
                                logger->clearAppends();
                                for(auto& a:i.appenders){
                                    dwframe::LogAppender::pointer ap;
                                    if(a.type == 1){
                                        ap.reset(new FileoutLogAppender(a.file));
                                    }else if(a.type ==2){
                                        ap.reset(new StdoutLogAppender);
                                    }
                                    
                                    ap->setLevel(a.level);
                                    if(!a.formatter.empty()){
                                        dwframe::LogFormatter::pointer fmt(new dwframe::LogFormatter(a.formatter));
                                        if(!fmt->isError()){
                                            ap->setFormatter(fmt);
                                        }else{
                                            std::cout<<"error,Using default fomatter."<<std::endl;
                                        }
                                    }
                                    logger->addAppender(ap);
                                }
                                
                            }
                            //删除
                            for(auto &i:old_value){
                                auto it = new_value.find(i);
                                    if(it == new_value.end()){
                                        //删除logger
                                        auto logger = dwframe_log_name(i.name);
                                        logger->setLevel((LogLevel::Level)100);
                                        logger->clearAppends();
                                    }
                                
                            }  

                        });
        }
    };  
    static LogIniter _log_init;//全局定义，调用构造函数
    void LoggerManager::init(){

    }
    Logger::pointer LoggerManager::getLogger(const std::string& name){ 
        MutexType::Lock lock(m_mutex);
        auto it = m_loggers.find(name);
        if(it != m_loggers.end()){
            return it->second;
        }

        Logger::pointer logger(new Logger(name));

        logger->m_root = m_root;
        m_loggers[name] = logger;
        return logger;
    }

    bool LoggerManager::insertLogger(const std::string& name,const Logger::pointer &logger){
        if(m_loggers.count(name)>0){
            return false;
        }else {
            m_loggers[name]=logger;
            return true;
        }

        return false;
    }
    Logger::pointer LoggerManager::Getlogger(const std::string& name){
        auto it = m_loggers.find(name);
        if(it != m_loggers.end()){
            return it->second;
        }
        throw std::invalid_argument(name);
    }
    bool LoggerManager::insertLogger(const std::string& name){
        if(m_loggers.count(name)>0){
            return false;
        }else {
            m_loggers[name]=Logger::pointer(new Logger(name));
            return true;
        }

        return false;
    }

    void LoggerManager::toString(){
        //std::cout<<m_loggers.size()<<std::endl;
        for(auto &x:m_loggers){
            std::cout<<"name: "<<x.second->getName()<<std::endl;
            std::cout<<"level: "<<dwframe::LogLevel::ToString(x.second->getLevel())<<std::endl;
            std::cout<<"formater: "<<x.second->getFormatter()->getPattern()<<std::endl;
            std::cout<<"appenders: "<<std::endl;
           for(auto &y:x.second->m_appenders){

               if(typeid(StdoutLogAppender)== typeid(*y)){//typedid用法
                   std::cout<<'\t'<<" type: "<<"StdoutLogAppender"<<std::endl;
               }else if(typeid(FileoutLogAppender)== typeid(*y)){
                   std::cout<<'\t'<<" type: "<<"FileoutLogAppender"<<std::endl;
                   auto z = std::dynamic_pointer_cast<FileoutLogAppender>(y);
                   std::cout<<'\t'<<" file_name: "<<z->getFileName()<<std::endl;
               }

               std::cout<<'\t'<<" level: "<<dwframe::LogLevel::ToString(y->getLevel())<<std::endl;
               std::cout<<'\t'<<" formater: "<<y->getFormatter()->getPattern()<<std::endl;
               std::cout<<'\t'<<" -----------------------------"<<std::endl;
            }
           std::cout<<"*********************************"<<std::endl;
        }
    }


    
}