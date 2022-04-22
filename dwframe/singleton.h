#ifndef  __DWFRAME__SNGLETON_H__
#define  __DWFRAME__SNGLETON_H__

#include <memory>
namespace dwframe
{
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

}
#endif // ! __DWFRAME__SNGLETON_H__