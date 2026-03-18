/**
 * Image SoLution Log Module
 */
#define SPDLOG_HEADER_ONLY
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "isl_log.hpp"

namespace isl
{

struct IslLog
{
    std::shared_ptr<spdlog::sinks::sink> console_sink;
    std::shared_ptr<spdlog::sinks::sink> file_sink;
    std::shared_ptr<spdlog::logger> logger = spdlog::default_logger();

    IslLog() : console_sink(spdlog::default_logger()->sinks().front()), 
               file_sink(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(ISL_DEFAULT_LOG_FILE_NAME, ISL_DEFAULT_LOG_FILE_SIZE, ISL_DEFAULT_LOG_FILE_NUM))
    {
        logger->sinks().push_back(file_sink), 
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] [%s:%#] %v");
        spdlog::flush_every(std::chrono::seconds(5));
        logger->flush_on(spdlog::level::warn);
    }

    void ResetFileSink(const std::string& filename, size_t filesize, size_t filenum)
    {
        auto& sinks = logger->sinks();
        sinks.erase(std::remove(sinks.begin(), sinks.end(), file_sink), sinks.end()); // 移除file_sink
        file_sink.reset();
        file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename, filesize, filenum);
        sinks.push_back(file_sink);

        return;
    }
};

static IslLog isllog;


/**
 * @fn      SetLogConsoleLevel
 * @brief   设置终端标准输出的日志等级
 * 
 * @param   [IN] lvl_name 日志等级，0-"trace"，1-"debug"，2-"info"，3-"warning"，4-"error"，5-"critical"，6-"off"
 */
void SetLogConsoleLevel(const std::string& lvl_name)
{
    isllog.console_sink->set_level(spdlog::level::from_str(lvl_name));
}


/**
 * @fn      SetLogFileLevel
 * @brief   设置写入文件的日志等级
 * 
 * @param   [IN] lvl_name 日志等级，0-"trace"，1-"debug"，2-"info"，3-"warning"，4-"error"，5-"critical"，6-"off"
 */
void SetLogFileLevel(const std::string& lvl_name)
{
    isllog.file_sink->set_level(spdlog::level::from_str(lvl_name));
}


/**
 * @fn      SetLogFileAttr
 * @brief   设置循环日志文件的属性，包括日志文件名、每个文件大小和总文件数量
 * 
 * @param   [IN] filename 日志文件路径名
 * @param   [IN] filesize 单个日志文件的最大大小，单位：字节
 * @param   [IN] filenum 最多支持的日志文件数
 */
void SetLogFileAttr(const std::string& filename, size_t filesize, size_t filenum)
{
    isllog.ResetFileSink(filename, filesize, filenum);
}

/// 下面这段代码可以不使用宏定义直接获取运行时的文件名和行号，C++20新增了source_location可以直接使用
#if 0
class SourceLocation
{
public:
    constexpr SourceLocation(const char *filename_in = __builtin_FILE(), const char *funcname_in = __builtin_FUNCTION(),
                             std::uint32_t lineno_in = __builtin_LINE()) noexcept 
        : filename(filename_in), funcname(funcname_in), lineno(lineno_in) {}

    [[nodiscard]] constexpr const char *FileName() const noexcept
    {
        return filename;
    }

    [[nodiscard]] constexpr const char *FuncName() const noexcept
    {
        return funcname;
    }

    [[nodiscard]] constexpr std::uint32_t LineNum() const noexcept
    {
        return lineno;
    }

private:
    const char *filename;
    const char *funcname;
    const std::uint32_t lineno;
};

inline constexpr auto GetSourceLocation(const SourceLocation &location)
{
    return spdlog::source_loc{location.FileName(), static_cast<int>(location.LineNum()), location.FuncName()};
}

// error
template <typename... Args>
struct error
{
    constexpr error(fmt::format_string<Args...> fmt, Args &&...args, SourceLocation location = {})
    {
        spdlog::log(GetSourceLocation(location), spdlog::level::err, fmt, std::forward<Args>(args)...);
    }
};

template <typename... Args>
error(fmt::format_string<Args...> fmt, Args &&...args) -> error<Args...>;
#endif

} // namespace

