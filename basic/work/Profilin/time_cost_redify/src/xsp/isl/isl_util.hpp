/**
 * Image SoLution Utility Interface
 */

#ifndef __ISL_UTIL_H__
#define __ISL_UTIL_H__

#include <iostream>
#include <fstream>
#include <memory>
#include "isl_hal.hpp"

namespace isl
{

/// 减数小于被减数时输出0，SUBtract with Cutting ofF ZERO
template<typename _T>
inline _T subcf0(_T a, _T b)
{
    return ((a > b) ? (a - b) : 0);
}

/// 两数相减后的绝对值
template<typename _T>
inline size_t subabs(_T a, _T b)
{
    return ((a > b) ? (a - b) : (b - a));
}

/// 计算容器的内存使用量
template <typename _T>
inline size_t container_memuse(const _T& container)
{
    return container.size() * sizeof(typename _T::value_type);
}

/**
 * @fn      write_file
 * @brief   将内存中（起始地址data，长度size）二进制数据写入文件
 * @note    提供ofs与bEnd参数是为了防止在写入同一个文件时频繁的打开关闭文件，降低效率
 *  
 * @param   [IN] filename 文件名
 * @param   [IN] data 二进制数据存放的内存地址
 * @param   [IN] size 二进制数据长度，单位：字节
 * @param   [IN] bNew 是否覆盖重写，true为重写，false为追加（默认），*ofs非法时有效 
 * @param   [IN] bEnd 是否结束该文件流，true为结束（默认），*ofs非空且该文件流已被打开时有效 
 * @param   [IN/OUT] ofs 文件流指针，若*ofs非空且该文件流已被打开，则继续写入该文件流，否则重新创建 
 *                       当*ofs为空且创建文件流成功时，输出该文件流对象，且接口内不关闭该对象 
 */
using pofs_t = std::unique_ptr<std::ofstream>;
void write_file(const std::string& filename, const void* data, size_t size, bool bNew=false, bool bEnd=true, pofs_t* ofs=nullptr);

/**
 * @fn      get_tick_count
 * @brief   获取单调时钟，单位：ms
 * 
 * @return  uint64_t 单调时钟计数值
 */
uint64_t get_tick_count(void);

} // namespace

#endif

