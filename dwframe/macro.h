#pragma once

#include <string.h>
#include "util.h"
#include <assert.h>

//条件大概率成不成立
#if defined __GNUC__ || defined_llvm__
#   define dwframe_LIKELY(x)    __builtin_expect(!!(x),1)
#   define dwframe_UNLIKELY(x)  __builtin_expect(!!(x),0)
#else
#   define dwframe_LIKELY(x)    (x)
#   define dwframe_UNLIKELY(x)  (x)
#endif
/// 断言宏封装
#define dwframe_ASSERT(x) \
    if(!(x)) { \
        dwframe_LOG_ERROR(dwframe_log_root()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << dwframe::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

/// 断言宏封装
#define dwframe_ASSERT2(x, w) \
    if(!(x)) { \
      dwframe_LOG_ERROR(dwframe_log_root()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << dwframe::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }
