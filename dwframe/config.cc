#include "config.h"

namespace dwframe{

    ConfigVarBase::pointer Config::LookupBase(const std::string& name){
        //dwframe_LOG_INFO(dwframe_log_root())<< s_datas.size();
        auto it = dwframe::Config::GetDatas().find(name);
        return it == dwframe::Config::GetDatas().end() ? nullptr : it->second;
    }
    static void ListALLMember(const std::string& prefix,
                              const YAML::Node& node,
                              std::list<std::pair<std::string,const YAML::Node>>& output){
            if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") 
                != std::string::npos){
                    dwframe_LOG_ERROR(dwframe_log_root()) << "Config invalid name:" << prefix 
                        <<":"<<node;
                    throw std::invalid_argument(prefix);
                    return;
            }
            
            output.push_back(std::make_pair(prefix,node));

            if(node.IsMap()){
                for(auto it = node.begin();
                    it !=node.end();++it){
                        ListALLMember(prefix.empty()? it->first.Scalar() : prefix+"."+it->first.Scalar(),
                        it->second,output);
                    }
            }

   }
    void Config::LodeFromYaml(const YAML::Node& root){//静态函数直接初始化即可
        std::list<std::pair<std::string,const YAML::Node>> all_node;//需要遍历所有节点值，而不用直接访问某个节点，
                                                                    //建议list，时间复杂度O(n),空间O(n),其他容器比如vector底层会有额外空间浪费
        ListALLMember("",root,all_node);

        for(auto & i:all_node){
            std::string key = i.first;
            if(key.empty()){
                continue;
            }

            std::transform(key.begin(),key.end(),key.begin(),::tolower);
            ConfigVarBase::pointer var = LookupBase(key);

            if(var){
                if(i.second.IsScalar()){
                    var->fromString(i.second.Scalar());
                }else{
                    std::stringstream ss;
                    ss<< i.second;
                    var->fromString(ss.str());
                }
            }
        }
    }
}