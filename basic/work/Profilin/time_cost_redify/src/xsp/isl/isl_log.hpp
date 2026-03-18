/**
 * Image SoLution Log Module 
 */

#ifndef __ISL_LOG_H__
#define __ISL_LOG_H__

#include "spdlog/fmt/bundled/core.h"
#include "isl_mat.hpp"
#include <unordered_set>

// 前向申明
namespace spdlog
{
    class logger;
    namespace sinks
    {
        class sink;
    }
}

// 重载自定义类的打印
/**
 * 重载自定义类的打印，目前支持的类有：
 * 1、std::pair, std::vector, std::array, std::unordered_set
 * 2、Point, Size, Rect, Imat
 */
namespace fmt
{

/// 重载std::pair的打印
template <typename _T1, typename _T2>
struct formatter<std::pair<_T1, _T2>>
{
    constexpr auto parse(format_parse_context &ctx)
    {
        /**
         * ctx.begin()指向格式说明符的起始位置（即“{:”之后的位置，包括“:”，所以一般开始要跳过冒号）
         * ctx.end()指向格式字符串的结束位置（即“}”之后的位置，不包括“}”） 
         * 使用自定义解析时，format函数中需要应用该格式 
         */
        return ctx.begin(); // 使用默认的格式解析
    }

    template <typename FormatContext>
    auto format(const std::pair<_T1, _T2>& _pair, FormatContext& ctx) const -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "{{{}, {}}}", _pair.first, _pair.second);
    }
};

/// 重载std::vector的打印
template <typename _T>
struct formatter<std::vector<_T>> : fmt::formatter<std::string_view>
{
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const std::vector<_T>& _vector, FormatContext& ctx) const -> decltype(ctx.out())
    {
        size_t vsize = _vector.size();
        std::string formatted = fmt::format("size {}", vsize);
        if (vsize < 16) // 小于16个数据时单行打印
        {
            formatted += ": ";
        }
        else
        {
            formatted += fmt::format("({} x {}):\n", (0 == vsize % 16) ? (vsize / 16) : (vsize / 16 + 1), 16);
        }
        for (size_t i = 0; i < vsize; ++i)
        {
            if (i > 0 && i % 16 == 0)
            {
                formatted += '\n';
            }
            if constexpr (std::is_floating_point_v<_T>)
            {
                formatted += fmt::format("{:.6f} ", _vector[i]);
            }
            else
            {
                formatted += fmt::format("{} ", _vector[i]);
            }
        }
        return std::copy(formatted.begin(), formatted.end(), ctx.out());
    }
};

/// 重载std::array的打印
template <typename _T, size_t _N>
struct formatter<std::array<_T, _N>> : fmt::formatter<std::string_view>
{
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const std::array<_T, _N>& _array, FormatContext& ctx) const -> decltype(ctx.out())
    {
        std::string formatted = fmt::format("size {}", _N);
        if (_N < 16) // 小于16个数据时单行打印
        {
            formatted += ": ";
        }
        else
        {
            formatted += fmt::format("({} x {}):\n", (0 == _N % 16) ? (_N / 16) : (_N / 16 + 1), 16);
        }
        for (size_t i = 0; i < _N; ++i)
        {
            if (i > 0 && i % 16 == 0)
            {
                formatted += '\n';
            }
            if constexpr (std::is_floating_point_v<_T>)
            {
                formatted += fmt::format("{:.6f} ", _array[i]);
            }
            else
            {
                formatted += fmt::format("{} ", _array[i]);
            }
        }
        return std::copy(formatted.begin(), formatted.end(), ctx.out());
    }
};

/// 重载std::unordered_set的打印
template <typename _T>
struct formatter<std::unordered_set<_T>> : fmt::formatter<std::string_view>
{
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const std::unordered_set<_T>& _set, FormatContext& ctx) const -> decltype(ctx.out())
    {
        std::string formatted = fmt::format("size {}", _set.size());
        if (_set.size() > 0)
        {
            formatted += fmt::format(", bucket count {}:", _set.bucket_count());
            for (size_t bidx = 0; bidx < _set.bucket_count(); ++bidx) // 按桶遍历
            {
                if (_set.bucket_size(bidx) > 0) // 桶中元素个数大于0时才打印
                {
                    formatted += fmt::format("\n{}: ", bidx);
                    for (auto it = _set.begin(bidx); it != _set.end(bidx); ++it)
                    {
                        if constexpr (std::is_floating_point_v<_T>)
                        {
                            formatted += fmt::format("{:.6f} ", *it);
                        }
                        else
                        {
                            formatted += fmt::format("{} ", *it);
                        }
                    }
                }
            }
        }
        return std::copy(formatted.begin(), formatted.end(), ctx.out());
    }
};

/// 重载std::map的打印
template <typename _T1, typename _T2>
struct formatter<std::map<_T1, _T2>> : fmt::formatter<std::string_view>
{
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const std::map<_T1, _T2>& _map, FormatContext& ctx) const -> decltype(ctx.out())
    {
        size_t msize = _map.size(), i = 0;
        std::string formatted = fmt::format("size {}", msize);
        if (msize < 8) // 小于8组数据时单行打印
        {
            formatted += ": ";
        }
        else
        {
            formatted += fmt::format("({} x {}):\n", (0 == msize % 8) ? (msize / 8) : (msize / 8 + 1), 8);
        }
        for (const auto& _pair : _map)
        {
            if (i > 0 && i % 8 == 0)
            {
                formatted += '\n';
            }
            if constexpr (std::is_integral_v<_T1>)
            {
                formatted += fmt::format("{{{}, ", _pair.first);
            }
            else
            {
                formatted += fmt::format("{{{:.6f}, ", _pair.first);
            }
            if constexpr (std::is_integral_v<_T2>)
            {
                formatted += fmt::format("{}}} ", _pair.second);
            }
            else
            {
                formatted += fmt::format("{:.6f}}} ", _pair.second);
            }
        }
        return std::copy(formatted.begin(), formatted.end(), ctx.out());
    }
};

/// 重载Point的打印
template <typename _T>
struct formatter<isl::Point<_T>>
{
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const isl::Point<_T>& _point, FormatContext& ctx) const -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "({}, {})", _point.x, _point.y);
    }
};

/// 重载Size的打印
template <typename _T>
struct formatter<isl::Size<_T>>
{
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const isl::Size<_T>& _size, FormatContext& ctx) const -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "{} x {}", _size.width, _size.height);
    }
};

/// 重载Rect的打印
template <typename _T>
struct formatter<isl::Rect<_T>>
{
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const isl::Rect<_T>& _rect, FormatContext& ctx) const -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "({}, {}) - {} x {}", _rect.x, _rect.y, _rect.width, _rect.height);
    }
};

/// 重载Imat的打印
template <typename _T>
struct formatter<isl::Imat<_T>>
{
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const isl::Imat<_T>& _imat, FormatContext& ctx) const -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "{}, type {:#x}({}|{}|{}), size {}, mem {} - {}, pwin {}, pads V{} H{}", 
                         _imat.isvalid(), _imat.itype(), _imat.channels(), _imat.btype(), _imat.depth(), _imat.isize(), 
                         (void*)_imat.ptr(), _imat.get_stride(), _imat.pwin(), _imat.get_vpads(), _imat.get_hpads());
    }
};

} // namespace fmt


#ifndef ISL_LOGD
#define ISL_LOGD(...)        SPDLOG_DEBUG(__VA_ARGS__)
#endif

#ifndef ISL_LOGI
#define ISL_LOGI(...)        SPDLOG_INFO(__VA_ARGS__)
#endif

#ifndef ISL_LOGW
#define ISL_LOGW(...)        SPDLOG_WARN(__VA_ARGS__)
#endif

#ifndef ISL_LOGE
#define ISL_LOGE(...)        SPDLOG_ERROR(__VA_ARGS__)
#endif


/// 日志文件最大占用空间：ISL_DEFAULT_LOG_FILE_SIZE × ISL_DEFAULT_LOG_FILE_NUM
#define ISL_DEFAULT_LOG_FILE_NAME       "/home/config/log/isl"  // 默认日志文件路径名
#define ISL_DEFAULT_LOG_FILE_SIZE       (1048576 * 10)          // 每个日志文件的最大大小，10MB
#define ISL_DEFAULT_LOG_FILE_NUM        (5)                     // 最多支持的日志文件数，5个

namespace isl
{

/**
 * @fn      SetLogConsoleLevel
 * @brief   设置终端标准输出的日志等级
 * 
 * @param   [IN] lvl_name 日志等级，0-"trace"，1-"debug"，2-"info"，3-"warning"，4-"error"，5-"critical"，6-"off"
 */
void SetLogConsoleLevel(const std::string& lvl_name);

/**
 * @fn      SetLogFileLevel
 * @brief   设置写入文件的日志等级
 * 
 * @param   [IN] lvl_name 日志等级，0-"trace"，1-"debug"，2-"info"，3-"warning"，4-"error"，5-"critical"，6-"off"
 */
void SetLogFileLevel(const std::string& lvl_name);

/**
 * @fn      SetLogFileAttr
 * @brief   设置循环日志文件的属性，包括日志文件名、每个文件大小和总文件数量
 * 
 * @param   [IN] filename 日志文件路径名
 * @param   [IN] filesize 单个日志文件的最大大小，单位：字节
 * @param   [IN] filenum 最多支持的日志文件数
 */
void SetLogFileAttr(const std::string& filename, size_t filesize, size_t filenum);

} // namespace isl

#endif

