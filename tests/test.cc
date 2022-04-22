#include "../dwframe/log.h"
#include "../dwframe/util.h"
//#include "../dwframe/config.h"

int main(int argc, const char** argv) {
    dwframe::Logger::pointer logger(new dwframe::Logger);
    logger->addAppender(dwframe::LogAppender::pointer(new dwframe::StdoutLogAppender));//不允许new抽象类
    dwframe::LogAppender::pointer fappender(new dwframe::FileoutLogAppender("./log.txt"));
    fappender->setLevel(dwframe::LogLevel::ERROR);
    logger->addAppender(fappender);
    
    
    //dwframe::LogEvent::pointer event(new dwframe::LogEvent(__FILE__,__LINE__,0,dwframe::GetThreadId(),dwframe::GetFiberId()));
    //event->getSS()<<"hellow dwframe";
    //logger->log(dwframe::LogLevel::DEBUG,event);

    dwframe_LOG_DEBUG(logger)<<"write";
    dwframe_LOG_FMT_INFO(logger,"%s","YES");
    dwframe_LOG_FMT_ERROR(logger,"%s","YES");
    std::cout<<"hellow dwframe"<<std::endl;
    //while(1);


    auto l = dwframe::Loggermgr::Getinstance();
    l->insertLogger("stdout");
    auto y = dwframe::LogAppender::pointer(new dwframe::StdoutLogAppender);
    y->setFormatter(dwframe::LogFormatter::pointer(new  dwframe::LogFormatter("[%D]%T%m%n")));
    l->getLogger("stdout")->addAppender(y);

    dwframe_LOG_DEBUG(l->getLogger("stdout"))<<"dddd";
    return 0;
}