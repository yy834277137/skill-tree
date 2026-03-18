#ifndef _AVX_DEF_HPP_
#define _AVX_DEF_HPP_


#if defined(__AVX__) || defined(__AVX512__) || defined(__AVX512F__)
#include <immintrin.h>
#define XSP_AVX_RUN_(fun)  \
do {                       \
        return fun;        \
} while (0)  
#else
#define XSP_AVX_RUN_(fun) ((void)0)
#endif 

#define XSP_AVX_RUN(fun) XSP_AVX_RUN_(fun)

#endif // _AVX_DEF_HPP_
