#ifndef _NENO_DEF_HPP_
#define _NENO_DEF_HPP_

#if defined(__ARM_NEON__) || defined(__ARM_NEON)
#define COMPILE_NENO
#include <arm_neon.h>
#define XSP_NENO_RUN_(fun) \
do {                       \
        return fun;        \
} while (0)  
#else
#define XSP_NENO_RUN_(fun) ((void)0)
#endif

#define XSP_NENO_RUN(fun) XSP_NENO_RUN_(fun)

#endif // _NENO_DEF_HPP_
