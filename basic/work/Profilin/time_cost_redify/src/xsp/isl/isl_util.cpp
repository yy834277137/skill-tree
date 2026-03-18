/**
 * Image SoLution Utility Interface
 */

#include <vector>
#include <array>
#include <chrono>
#include "isl_util.hpp"
#include "spdlog/spdlog.h"
#include "isl_log.hpp"

namespace isl
{

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
void write_file(const std::string& filename, const void* data, size_t size, bool bNew, bool bEnd, pofs_t* ofs)
{
    if (filename.empty() || data == nullptr || size == 0)
    {
        return;
    }

    std::ios_base::openmode mode = std::ios::out | std::ios::binary;
    if (!bNew)
    {
        mode |= std::ios::app;
    }

    /// 定义文件流状态：无输入，未初始化，已损坏，完好的
    enum { fs_na, fs_null, fs_bad, fs_good } fs_stat = fs_na;
    if (ofs)
    {
        if (*ofs)
        {
            fs_stat = (*ofs)->good() ? fs_good : fs_bad;
        }
        else
        {
            fs_stat = fs_null;
        }
    }

    auto fp = (fs_stat == fs_good) ? std::move(*ofs) : std::make_unique<std::ofstream>(filename, mode);
    if (!fp || !fp->is_open())
    {
        int32_t state = static_cast<int32_t>(fp->rdstate());
        ISL_LOGW("open {} failed, badbit {}, failbit {}, errinfo: {}", 
                 filename, state & std::ios::badbit, state & std::ios::failbit, strerror(errno));
        fp->clear();
        return;
    }

    fp->write((const char*)data, size);
    if (fp->fail())
    {
        int32_t state = static_cast<int32_t>(fp->rdstate());
        ISL_LOGW("write to {} failed, badbit {}, failbit {}, errinfo: {}", 
                 filename, state & std::ios::badbit, state & std::ios::failbit, strerror(errno));
        fp->clear();
    }

    if (bEnd || fs_stat == fs_na)
    {
        fp->close();
        fp->flush();
    }
    else if (!bEnd && (fs_stat == fs_null || fs_stat == fs_bad))
    {
        *ofs = std::move(fp);
    }

    return;
}


/**
 * @fn      get_tick_count
 * @brief   获取单调时钟，单位：ms
 * 
 * @return  uint64_t 单调时钟计数值
 */
uint64_t get_tick_count(void)
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    return ms;
}

} // namespace

