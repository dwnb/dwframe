#pragma once

#define dwframe_little_endian 1
#define dwframe_big_endian 2

#include <byteswap.h>
#include <stdint.h>
#include <type_traits>
#include <bits/endian.h>
namespace dwframe{
    template<typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint64_t),T>::type
    byteswap(T value){
        return (T)bswap_64((uint64_t)value);
    }

    template<typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint32_t),T>::type
    byteswap(T value){
        return (T)bswap_32((uint32_t)value);
    }

    template<typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint16_t),T>::type
    byteswap(T value){
        return (T)bswap_16((uint16_t)value);
    }

#if BYTE_ORDER == BIG_ENDIAN
#define dwframe_byte_order dwframe_big_endian
#else
#define dwframe_byte_order dwframe_little_endian
#endif

#if dwframe_byte_order == dwframe_big_endian
    //只在小端机器上执行byteswap, 在大端机器上什么都不做
    template<class T>
    T byteswapOnLittleEndian(T t){
        return t;
    }
    //只在大端机器上执行byteswap, 在小端机器上什么都不做
    template<class T>
    T byteswapOnBigEndian(T t){
        return byteswap(t);
    }
#else
    //只在小端机器上执行byteswap, 在大端机器上什么都不做
    template<class T>
    T byteswapOnLittleEndian(T t){
        return byteswap(t);
    }
    //只在大端机器上执行byteswap, 在小端机器上什么都不做
    template<class T>
    T byteswapOnBigEndian(T t){
        return t;
    }
#endif
}