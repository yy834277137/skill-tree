#ifndef _OPENCL_DEF_HPP_
#define _OPENCL_DEF_HPP_

#if defined COMPILE_OPENCL
#include "opencl/cl.h"
#define XSP_OCL_RUN_(fun) \
do {                      \
        return fun;       \
} while (0)     
#else
#define XSP_OCL_RUN_(fun) ((void)0)
#endif // COMPILE_OPENCL

#define XSP_OCL_RUN(fun) XSP_OCL_RUN_(fun)

#endif // _OPENCL_DEF_HPP_
