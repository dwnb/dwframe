#ifndef __DWFRAME_CONFIG_H_
#define __DWFRAME_CONFIG_H_

#include <memory>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <exception>
#include <yaml-cpp/yaml.h>
#include <ctype.h>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <functional>
#include <set>

#include "thread.h"
#include "log.h"


namespace dwframe{
    //基类：把一些公用的属性放到这里
    class ConfigVarBase{
    private:
    protected:
        std::string m_name;
        std::string m_description;
    public:
        typedef std::shared_ptr<ConfigVarBase> pointer;
    public:
        ConfigVarBase(const std::string& name,const std::string & description = "")
            :m_name(name)
            ,m_description(description){
                std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }

        virtual ~ConfigVarBase(){}

        const std::string& getName() const { return m_name;}
        const std::string& getDescription() const { return m_description;}

        virtual std::string toString() = 0;
        virtual bool fromString(const std::string& val) = 0;
    };
    //F from type
    //T to_type
    template<typename F, typename T>
    class LexicalCast{
    public:
        T operator()(const F& v){
            return boost::lexical_cast<T>(v);
        }
    };
    //偏特化
    //vector
    template<typename T>
    class LexicalCast<std::string, std::vector<T>>{
    public:
        std::vector<T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for(size_t i = 0;i < node.size(); ++i){
                ss.str("");
                ss<<node[i];
                vec.push_back(LexicalCast<std::string,T>()(ss.str()));
            }

            return vec;//C++11h会自己优化把vec变成move
        }
    };

    template<typename T>
    class LexicalCast<std::vector<T>, std::string>{
    public:
        std::string operator()(const std::vector<T>& v){
            YAML::Node node;
            for(auto& i:v){
                node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
            }
            std::stringstream ss;
            ss<<node;

            return ss.str();//C++11h会自己优化把vec变成move
        }
    };
    //list
    template<typename T>
    class LexicalCast<std::string, std::list<T>>{
    public:
        std::list<T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);
            typename std::list<T> lis;
            std::stringstream ss;
            for(size_t i = 0;i < node.size(); ++i){
                ss.str("");
                ss<<node[i];
                lis.push_back(LexicalCast<std::string,T>()(ss.str()));
            }

            return lis;//C++11h会自己优化把vec变成move
        }
    };

    template<typename T>
    class LexicalCast<std::list<T>, std::string>{
    public:
        std::string operator()(const std::list<T>& v){
            YAML::Node node;
            for(auto& i:v){
                node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
            }
            std::stringstream ss;
            ss<<node;

            return ss.str();//C++11h会自己优化把vec变成move
        }
    };

    
    //set
    template<typename T>
    class LexicalCast<std::string, std::set<T>>{
    public:
        std::set<T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);
            typename std::set<T> Set;
            std::stringstream ss;
            for(size_t i = 0;i < node.size(); ++i){
                ss.str("");
                ss<<node[i];
                Set.insert(LexicalCast<std::string,T>()(ss.str()));
            }

            return Set;//C++11h会自己优化把vec变成move
        }
    };

    template<typename T>
    class LexicalCast<std::set<T>, std::string>{
    public:
        std::string operator()(const std::set<T>& v){
            YAML::Node node;
            for(auto& i:v){
                node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
            }
            std::stringstream ss;
            ss<<node;

            return ss.str();//C++11h会自己优化把vec变成move
        }
    };
    //unordered_set
    template<typename T>
    class LexicalCast<std::string, std::unordered_set<T>>{
    public:
        std::unordered_set<T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);
            typename std::set<T> USet;
            std::stringstream ss;
            for(size_t i = 0;i < node.size(); ++i){
                ss.str("");
                ss<<node[i];
                USet.insert(LexicalCast<std::string,T>()(ss.str()));
            }

            return USet;//C++11h会自己优化把vec变成move
        }
    };

    template<typename T>
    class LexicalCast<std::unordered_set<T>, std::string>{
    public:
        std::string operator()(const std::unordered_set<T>& v){
            YAML::Node node;
            for(auto& i:v){
                node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
            }
            std::stringstream ss;
            ss<<node;

            return ss.str();//C++11h会自己优化把vec变成move
        }
    };
    //map
    template<typename T>
    class LexicalCast<std::string, std::map<std::string,T>>{
    public:
        std::map<std::string,T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string,T> _Map;
            std::stringstream ss;
            for(auto it = node.begin();it!=node.end();++it){
                ss.str("");
                ss<<it->second;
                _Map.insert(std::make_pair(it->first.Scalar(),LexicalCast<std::string,T>()(ss.str())));
            }

            return _Map;//C++11h会自己优化把vec变成move
        }
    };

    template<typename T>
    class LexicalCast<std::map<std::string,T> , std::string>{
    public:
        std::string operator()(const std::map<std::string,T>& v){
            YAML::Node node;
            for(auto& i:v){
                node[i.first]=YAML::Load(LexicalCast<T,std::string>()(i.second));
            }
            std::stringstream ss;
            ss<<node;

            return ss.str();//C++11h会自己优化把vec变成move
        }
    };
    
    //unordered_map
    template<typename T>
    class LexicalCast<std::string, std::unordered_map<std::string,T>>{
    public:
        std::unordered_map<std::string,T> operator()(const std::string& v){
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string,T> _Map;
            std::stringstream ss;
            for(auto it = node.begin();it!=node.end();++it){
                ss.str("");
                ss<<it->second;
                _Map.insert(std::make_pair(it->first.Scalar(),LexicalCast<std::string,T>()(ss.str())));
            }

            return _Map;//C++11h会自己优化把vec变成move
        }
    };

    template<typename T>
    class LexicalCast<std::unordered_map<std::string,T> , std::string>{
    public:
        std::string operator()(const std::unordered_map<std::string,T>& v){
            YAML::Node node;
            for(auto& i:v){
                node[i.first]=YAML::Load(LexicalCast<T,std::string>()(i.second));
            }
            std::stringstream ss;
            ss<<node;

            return ss.str();//C++11h会自己优化把vec变成move
        }
    };
    //具体实现类
    //实现复杂数据类型，序列化（变为字符串，对象转换为字节序列的过程称）与反序列化（变回原始数据，把字节序列恢复为对象）
    template<typename T, typename FromStr = LexicalCast<std::string, T>, 
                typename ToStr = LexicalCast<T, std::string>>
    class ConfigVar:public ConfigVarBase{
    public:
        using RWLock  = RWMutex;
        using pointer = std::shared_ptr<ConfigVar>;
        using callBack = std::function<void (const T& old_value,const T& new_value)>;
    private:
        T m_val;
        //uint_64唯一
        std::map<uint64_t,callBack> m_cbs;
        RWLock m_mutex;
    public:
        ConfigVar(const std::string& name
                 ,const T& default_value
                 ,const std::string& description = "")
            :ConfigVarBase(name,description)
            ,m_val(default_value){
               
        }
        uint64_t addListener(callBack cb){
            static uint64_t s_fun_id =0;

            RWLock::WriteLock lock(m_mutex);
            m_cbs[++s_fun_id] = cb;

            return s_fun_id;
        }
        void delListener(const uint64_t& key){
            RWLock::WriteLock lock(m_mutex);
            m_cbs.erase(key);
        }
        callBack getListener(uint64_t key){
            RWLock::ReadLock lock(m_mutex);
            return m_cbs.count(key)>0?m_cbs[key]:nullptr;
        }
        virtual std::string toString() override{
            try{
                //return boost::lexical_cast<std::string>(m_val);
                //此处前一个()作用是产生一个临时对象，然后仿函数
                RWLock::ReadLock lock(m_mutex);
                return ToStr()(m_val);
            }catch(std::exception & e){
                dwframe_LOG_ERROR(dwframe_log_root())<<"Configvar::toString exception"
                <<e.what()<<" convert: "<< typeid(m_val).name() << "to_string";
            }

            return "";
        }

        virtual bool fromString(const std::string& val) override{
            try{
                //m_val = boost::lexical_cast<T>(val);
                //此处前一个()作用是产生一个临时对象，然后仿函数
                setValue(FromStr()(val));
            }catch(std::exception& e){
                dwframe_LOG_ERROR(dwframe_log_root())<<"Configvar::toString exception"
                <<e.what()<<" convert: "<< typeid(m_val).name() << "to_string";
            }
            return false;
        }

        const T getValue()  { //不能加const，因为m_mutex会改变
            RWLock::ReadLock lock(m_mutex);
            return m_val; 
        }
        void setValue(const T& v){ 
            {
                RWLock::ReadLock lock(m_mutex);
                if(m_val==v){
                    return;
                }
                for(auto &i:m_cbs){
                    i.second(m_val,v);
                }
            }
            
            RWLock::WriteLock lock(m_mutex);
            m_val = v;
        }

        void clearListener(){
            RWLock::WriteLock lock(m_mutex);
            m_cbs.clear();
        }
    };

    class Config
    {
    public:
        using RWLock  = RWMutex; 
        using ConfigVarMap = std::map<std::string, ConfigVarBase::pointer>;
    private:
        static ConfigVarMap& GetDatas() {//这样定义的原因是因为静态变量初始化顺序不一定,在运行时调用到s_datas可能还未定义，这样写s_datas一定被定义
            static ConfigVarMap s_datas;
            return s_datas;
        }
    public:
        Config(/* args */);
        template<typename T>
        static typename ConfigVar<T>::pointer LookUp(const std::string& name,
                    const T& default_value, const std::string& description = ""){
            //std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            RWLock::WriteLock lock(GetMutex());
            auto it = dwframe::Config::GetDatas().find(name);
            if(it != dwframe::Config::GetDatas().end()){
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if(tmp){
                    dwframe_LOG_INFO(dwframe_log_root()) << " Lookup =  " <<name<<" exist !";
                }else{
                    dwframe_LOG_ERROR(dwframe_log_root()) << " Lookup name name conflicts with an existing name of a different type! " 
                    <<"\n"<<"Lookup name: "<<name;
                    throw std::invalid_argument(name);
                }
            }
            //string::find_first_not_of
            if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678")!=std::string::npos){
                
                dwframe_LOG_ERROR(dwframe_log_root()) << " Lookup name invalid " <<name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::pointer v(new ConfigVar<T>(name, default_value,description));
            dwframe::Config::GetDatas()[name] = v;

            //dwframe_LOG_INFO(dwframe_log_root())<< "s_datas.size: "<<s_datas.size();
            return v;
        }
        

        static void LodeFromYaml(const YAML::Node& root);
        static ConfigVarBase::pointer LookupBase(const std::string& name);//抽象类要用指针
        ~Config();

        static RWLock & GetMutex(){
            static RWLock s_mutex;
            return s_mutex;
        }
    };
    
    
    
}

#endif // DEBUG