#include "../dwframe/config.h"
#include "../dwframe/log.h"
#include "yaml-cpp/yaml.h"

dwframe::ConfigVar<int>::pointer g_int_value_config1  = 
    dwframe::Config::LookUp("system.port",(int)8080,"system port");
dwframe::ConfigVar<int>::pointer g_int_value_config2  = 
    dwframe::Config::LookUp("system.value",(int)10,"system value");//用LookUp创建ConfigVar
dwframe::ConfigVar<std::vector<int>>::pointer g_intVec_value_config  = 
    dwframe::Config::LookUp("system.int_vec",std::vector<int>{1,2},"system int vec");
dwframe::ConfigVar<std::list<int>>::pointer g_intList_value_config  = 
    dwframe::Config::LookUp("system.int_list",std::list<int>{3,4},"system int list");
dwframe::ConfigVar<std::set<int>>::pointer g_intSet_value_config  = 
    dwframe::Config::LookUp("system.int_set",std::set<int>{0,47},"system int Set");
dwframe::ConfigVar<std::map<std::string,int>>::pointer g_mapintstr_value_config  = 
    dwframe::Config::LookUp("system.map_str_int",std::map<std::string,int>{{"k",2}},"system str int umap");
dwframe::ConfigVar<std::unordered_map<std::string,int>>::pointer g_umapintstr_value_config  = 
    dwframe::Config::LookUp("system.umap_str_int",std::unordered_map<std::string,int>{{"u",2}},"system std int umap");


class A{
public:
    int x=0;
    int y=0;
};
void print_yaml(const YAML::Node &node,int level){
    if(node.IsScalar()){
        dwframe_LOG_INFO(dwframe_log_root()) << node.Scalar() <<"-"<< node.Type() <<"-" <<level;
    }else if(node.IsNull()){
        dwframe_LOG_INFO(dwframe_log_root()) << "NULL -" << node.Type() << "-" <<level;
    }else if(node.IsMap()){
        for(auto it = node.begin(); it!=node.end(); ++it){
            dwframe_LOG_INFO(dwframe_log_root()) << it->first << "-"<<it->first.Type() << "-" <<level;
            print_yaml(it->second , level+1);
        }
    }else if(node.IsSequence()){
        for(size_t i=0;i<node.size();++i){
            dwframe_LOG_INFO(dwframe_log_root()) << i << "-"<<node[i].Type() << "-" <<level;
            print_yaml(node[i] , level+1);
        }
    }
}
void test_yaml(){
    //dwframe_LOG_INFO(GetLogger("system"))<<"hello";
    dwframe::Loggermgr::Getinstance()->toString();
    YAML::Node root = YAML::LoadFile("/root/server/config/log.yml");
    dwframe::Config::LodeFromYaml(root);
    dwframe_LOG_INFO(dwframe_log_root()) <<"============================";
    dwframe::Loggermgr::Getinstance()->toString();
    //dwframe_LOG_INFO(dwframe_log_root()) << root;
    dwframe_LOG_INFO(GetLogger("system"))<<"hello";

    GetLogger("system")->setFormatter("%d-%m%n");
    dwframe_LOG_INFO(GetLogger("system"))<<"hello";
    //print_yaml(root,0);
}

void test_config(){
    dwframe_LOG_INFO(dwframe_log_root()) <<"*******************************";
    dwframe_LOG_INFO(dwframe_log_root()) << "before: "<<g_int_value_config1->getValue();
    dwframe_LOG_INFO(dwframe_log_root()) << "before:"<<g_int_value_config2->toString();

#define testOut(var,name,prefix) \
        {\
            auto &v = var->getValue();\
            for(auto& i:v){\
                dwframe_LOG_INFO(dwframe_log_root()) <<#prefix" " #name ": "<<i;\
            }\
            dwframe_LOG_INFO(dwframe_log_root()) <<#prefix" ""yaml: \n" #name <<var->toString();\
        }\

#define testOutTAB(var,name,prefix) \
        {\
            auto &v = var->getValue();\
            for(auto& i:v){\
                dwframe_LOG_INFO(dwframe_log_root()) <<#prefix" " #name ": {"<<\
                i.first<<"-"<<i.second<<"}";\
            }\
            dwframe_LOG_INFO(dwframe_log_root()) <<#prefix" ""yaml: \n" #name <<var->toString();\
        }\

    g_intList_value_config->addListener([](const std::list<int>& old_value,const std::list<int>& new_value){
        dwframe_LOG_INFO(dwframe_log_root()) << "oldvalue.size() "<<old_value.size();
        dwframe_LOG_INFO(dwframe_log_root()) << "newvalue.size() "<<new_value.size();
    });

    dwframe_LOG_INFO(dwframe_log_root()) <<"*******************************";
    YAML::Node root = YAML::LoadFile("/root/server/config/log.yml");
    dwframe::Config::LodeFromYaml(root);
    dwframe_LOG_INFO(dwframe_log_root()) <<"-------------------------------";
    dwframe_LOG_INFO(dwframe_log_root()) << "after: "<<g_int_value_config1->getValue();
    dwframe_LOG_INFO(dwframe_log_root()) << "after: "<<g_int_value_config2->toString();

    auto &x = g_intVec_value_config->getValue();
    for(auto &dd:x){
        dwframe_LOG_INFO(dwframe_log_root()) << "after int_vec: "<<dd;
    }

    testOut(g_intList_value_config,int_List,after);
    testOut(g_intSet_value_config,int_set,after);
    testOutTAB(g_mapintstr_value_config,map_str_int,after);
    testOutTAB(g_umapintstr_value_config,umap_str_int,after);

}
int main(int argc,char *argv[]){
    //dwframe_LOG_INFO(dwframe_log_root()) << g_int_value_config->getValue();
    //dwframe_LOG_INFO(dwframe_log_root()) << g_int_value_config->toString();
    //test_config();
    test_yaml();
    
    //while(1);
    return 0;
}