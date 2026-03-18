/**
 * @file xsp_mod_cnn.cpp
 * @brief CNN模块实现文件 - AI超分辨率和降噪功能的实现
 *
 * 实现了AI_XSP和AI_DN功能，支持透射和背散射两种模式，
 * 提供同步和异步处理能力。
 *
 * 主要功能模块：
 * - 模型文件管理和匹配
 * - AI模型初始化和加载
 * - 数据预处理和后处理
 * - 模型动态切换
 * - 异步任务处理
 */

#include "xsp_mod_cnn.hpp"
#include "core_def.hpp"
#include "xsp_alg.hpp"
#include "isl_pipe.hpp"

#ifndef _MSC_VER
#include <unistd.h>
#endif

/**************************************************************************************************
 *                                      FileMatcher方法实现
 ***************************************************************************************************/

/**
 * @brief AIXSP模型匹配器初始化
 * @param dir_path 模型文件目录路径
 * @param suffix 模型文件后缀名 (默认为"rknn")
 * @note 扫描指定目录下的模型文件，建立索引结构用于快速匹配
 */
XRAY_LIB_HRESULT FileMatcher::init(const std::filesystem::path &dir_path, const std::string &suffix)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    log_info("Initializing FileMatcher for directory: %s, suffix: %s", dir_path.string().c_str(), suffix.c_str());

    hr = load_files(dir_path, suffix);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "LibXRay: Failed to load Aixsp model files.");

    build_index();
    log_info("Loaded %zu model files, %zu channel candidates", files.size(), channel_candidates.size());

    return XRAY_LIB_OK;
}

/**
 * @brief 获取指定索引的模型文件参数
 * @param idx 索引值libXRay_def.h
 * @return AiXspFileParams 模型文件参数
 * @throw std::out_of_range 当索引越界时抛出异常
 */
AiXspFileParams FileMatcher::get_idx_files(int32_t idx)
{
    try
    {
        return files.at(idx);
    }
    catch (const std::out_of_range &e)
    {
        std::string error_msg = "index " + std::to_string(idx) +
                                " out of range [0, " + std::to_string(files.size() - 1) + "]";
        log_error("FileMatcher::get_idx_files failed: %s", error_msg.c_str());
        throw std::out_of_range(error_msg);
    }
}

/**
 * @brief 加载模型文件
 * @param dir_path 模型文件目录路径
 * @param suffix 模型文件后缀名
 * @note 使用正则表达式解析模型文件名，提取参数信息
 */
XRAY_LIB_HRESULT FileMatcher::load_files(const std::filesystem::path &dir_path, const std::string &suffix)
{
    std::string pattern_str = R"(AIXSP_1_(\d+)_(\d+\.\d+x\d+\.\d+)_(\d+)_(\d+)_(\d+)\.)" + suffix;
    std::regex pattern(pattern_str);

    log_info("Loading model files from %s with pattern: %s", dir_path.string().c_str(), pattern_str.c_str());

    // 检测该文件路径是否有效
    if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path))
    {
        log_error("Invalid directory path: %s", dir_path.string().c_str());
        return XRAY_LIB_INVALID_PARAM;
    }

    for (const auto &entry : std::filesystem::directory_iterator(dir_path))
    {
        std::string filename = entry.path().filename().string();
        std::smatch matches;

        if (std::regex_match(filename, matches, pattern))
        {
            AiXspFileParams params{
                std::stoi(matches[1]), // 通道数
                matches[2].str(),      // 缩放系数
                std::stoi(matches[3]), // 宽度
                std::stoi(matches[4]), // 高度
                std::stoi(matches[5]), // 噪声参数
                filename               // 模型名称
            };
            files.push_back(params);
            log_debug("Loaded model: %s (ch:%d, scale:%s, w:%d, h:%d, noise:%d)",
                      filename.c_str(), params.AiXspChn, params.AiXspSf.c_str(),
                      params.AiXspWid, params.AiXspHei, params.AiXspNoise);
        }
    }

    return XRAY_LIB_OK;
}

/**
 * @brief 构建模型索引
 * @note 建立多级索引结构，加速模型查找
 */
void FileMatcher::build_index()
{
    log_info("Building model index...");
    for (const auto &file : files)
    {
        channel_index[file.AiXspChn][file.AiXspSf][file.AiXspWid][file.AiXspHei][file.AiXspNoise] = file;
        channel_candidates.insert(file.AiXspChn);
    }
    log_info("Model index built successfully");
}

/**
 * @brief 根据参数匹配合适的模型文件
 * @param target_c 目标通道数
 * @param target_w 目标宽度
 * @param target_h 目标高度
 * @param target_n 目标噪声参数
 * @param target_scale 目标缩放系数
 * @return std::optional<std::string> 匹配的模型文件名，如果未找到则返回std::nullopt
 * @note 按照通道数、缩放系数、宽度、高度、噪声参数的优先级顺序匹配
 */
std::optional<std::string> FileMatcher::find_match(int32_t target_c, int32_t target_w, int32_t target_h,
                                                   int32_t target_n, const std::string &target_scale)
{
    selected_values.clear();
    selected_scale_factor.clear();

    log_debug("Finding model match - channels: %d, width: %d, height: %d, noise: %d, scale: %s",
              target_c, target_w, target_h, target_n, target_scale.c_str());

    // 按优先级顺序匹配参数
    auto try_match = [&](auto get_candidates, auto get_value, const std::string &param_name)
    {
        auto candidates = get_candidates();
        if (candidates.empty())
        {
            log_debug("No candidates found for %s", param_name.c_str());
            return false;
        }

        // 如果是通道数量进行匹配，则需要一一对应
        if (param_name == "channels" && (get_value() == 1 || get_value() == 2 || get_value() == 3))
        {
            auto it = candidates.find(get_value());
            if (it != candidates.end())
            {
                selected_values.push_back(*it);
                log_debug("Matched %s: %d", param_name.c_str(), *it);
                return true;
            }
            else
            {
                log_debug("No exact match found for %s: %d", param_name.c_str(), get_value());
                return false;
            }
        }

        auto it = candidates.lower_bound(get_value());
        if (it != candidates.end())
        {
            selected_values.push_back(*it);
            log_debug("Matched %s: %d (requested: %d)", param_name.c_str(), *it, get_value());
            return true;
        }
        else
        {
            log_debug("No match found for %s: %d", param_name.c_str(), get_value());
            return false;
        }
    };

    // 匹配通道
    if (!try_match([&]
                   { return channel_candidates; }, [&]
                   { return target_c; }, "channels"))
    {
        return std::nullopt;
    }

    // 匹配缩放系数
    auto &scale_candidates = channel_index[selected_values[0]];
    auto scale_it = scale_candidates.find(target_scale);
    if (scale_it != scale_candidates.end())
    {
        selected_scale_factor = target_scale;
        log_debug("Matched scale factor: %s", target_scale.c_str());
    }
    else
    {
        log_debug("No scale factor match found for: %s", target_scale.c_str());
        return std::nullopt;
    }

    // 匹配宽度
    if (!try_match([&]
                   { 
          auto& submap = channel_index[selected_values[0]][selected_scale_factor];
          std::set<int32_t> widths;
          for (auto&& [w, _] : submap) widths.insert(w);
          return widths; }, [&]
                   { return target_w; }, "width"))
        return std::nullopt;

    // 匹配高度
    if (!try_match([&]
                   { 
          auto& submap = channel_index[selected_values[0]][selected_scale_factor][selected_values[1]];
          std::set<int32_t> heights;
          for (auto&& [h, _] : submap) heights.insert(h);
          return heights; }, [&]
                   { return target_h; }, "height"))
        return std::nullopt;

    // 匹配噪声
    if (!try_match([&]
                   { 
          auto& submap = channel_index[selected_values[0]][selected_scale_factor][selected_values[1]][selected_values[2]];
          std::set<int32_t> noises;
          for (auto&& [n, _] : submap) noises.insert(n);
          return noises; }, [&]
                   { return target_n; }, "noise"))
        return std::nullopt;

    // 返回匹配的文件名
    auto &submap = channel_index[selected_values[0]][selected_scale_factor][selected_values[1]][selected_values[2]][selected_values[3]];
    log_info("Found matching model: %s", submap.AiXspModName.c_str());
    return submap.AiXspModName;
}

/**************************************************************************************************
 *                                      CnnModule方法实现
 ***************************************************************************************************/

/**
 * @brief 构造函数
 * @note 初始化所有成员变量为默认值
 */
CnnModule::CnnModule() : m_pSharedPara(nullptr), m_bAsyncRunning(false), m_taskId(0)
{
    log_info("CnnModule constructor");
    memset(&m_stCnnPara, 0, sizeof(m_stCnnPara));
    memset(&m_tModelInfo, 0, sizeof(m_tModelInfo));
}

/**
 * @brief CNN模块初始化
 * @param szPublicFileFolderName 公共文件夹路径
 * @param stXRaylib_AIPara AI-XSP初始化参数
 * @param pPara 全局共享参数
 * @return XRAY_LIB_HRESULT 错误码
 * @note 初始化CNN模块，加载模型文件，设置模型参数，初始化异步线程池
 */
XRAY_LIB_HRESULT CnnModule::Init(const char *szPublicFileFolderName, XRAYLIB_AI_PARAM stXRaylib_AIPara, SharedPara *pPara)
{
    XSP_CHECK(szPublicFileFolderName, XRAY_LIB_NULLPTR, "Public path is null.");

    XRAY_LIB_HRESULT hr;
    m_pSharedPara = pPara;          // 全局共享参数
    m_stCnnPara = stXRaylib_AIPara; // AIXSP是否启用相关参数

    log_info("Initializing CNN module with AI enabled: %d, plat_mode: %d",
             stXRaylib_AIPara.nUseAI, stXRaylib_AIPara.plat_mode);

    if (0 == stXRaylib_AIPara.nUseAI)
    {
        log_info("AI_XSP Init: AI is not enabled.");
        return XRAY_LIB_OK;
    }

    // 初始化AIXSP模型文件路径和模型文件匹配器
    hr = InitModelSelector(szPublicFileFolderName, stXRaylib_AIPara);
    if (XRAY_LIB_OK != hr)
    {
        log_error("Failed to initialize file paths: %X", hr);
        return hr;
    }

    // 根据输出图像尺寸选择合适的AIXSP模型
    hr = InitModelParam();
    if (XRAY_LIB_OK != hr)
    {
        log_error("Failed to initialize model parameters: %X", hr);
        return XRAY_LIB_AI_XSP_INIT_ERROR;
    }

    // 设置AI-XSP初始化参数
    strcpy(m_szPathDLModelName, m_tModelInfo.strModelName);
    m_stCnnPara.AI_channels_in = m_tModelInfo.AI_channels_in;
    m_stCnnPara.AI_width_in = m_tModelInfo.AI_width_in;
    m_stCnnPara.AI_height_in = m_tModelInfo.AI_height_in;
    m_stCnnPara.AI_channels_out = m_tModelInfo.AI_channels_out;
    m_stCnnPara.AI_width_out = m_tModelInfo.AI_width_out;
    m_stCnnPara.AI_height_out = m_tModelInfo.AI_height_out;

    log_info("Selected model: %s (in:%dx%d, out:%dx%d, ch:%d)",
             m_tModelInfo.strModelName, m_tModelInfo.AI_width_in, m_tModelInfo.AI_height_in,
             m_tModelInfo.AI_width_out, m_tModelInfo.AI_height_out, m_tModelInfo.AI_channels_in);

    // 自研AI-XSP初始化
    strcpy(m_szPathDLModelFile, m_szPathDLModelPath);
    strcat(m_szPathDLModelFile, m_szPathDLModelName);

    SYSTEM_MODEL_PLAT_PRM tSystemModelPlatPrm;
    tSystemModelPlatPrm.Input_Pass_Through = 0;
    tSystemModelPlatPrm.Output_Pass_Through = 0;
    tSystemModelPlatPrm.pModelPath = m_szPathDLModelFile;
    tSystemModelPlatPrm.eSysTemplat = m_stCnnPara.systemPlat;
    tSystemModelPlatPrm.eSysProcessType = m_stCnnPara.systemProcessType;
    tSystemModelPlatPrm.eSysPrecisionType = m_stCnnPara.systemProcessPrecision;
    tSystemModelPlatPrm.sSysInputDataType = TENSOR_FLOAT32;
    tSystemModelPlatPrm.sSysOutputDataType = TENSOR_FLOAT32;
    tSystemModelPlatPrm.eSysCoreMask = m_stCnnPara.rayinSchedulerParam.aiSrNpuCore;

    SYSTEM_MODEL_INSIDE_PRM tSystemModelInsidePrm;
    tSystemModelInsidePrm.inputChnSize = m_stCnnPara.AI_channels_in;
    tSystemModelInsidePrm.inputHei = m_stCnnPara.AI_width_in;
    tSystemModelInsidePrm.inputWid = m_stCnnPara.AI_height_in;
    tSystemModelInsidePrm.outputHei = m_stCnnPara.AI_width_out;
    tSystemModelInsidePrm.outputWid = m_stCnnPara.AI_height_out;
    tSystemModelInsidePrm.outputChnSize = m_stCnnPara.AI_channels_out;

    int32_t ret = AI_XSP_Init(m_stCnnPara.nChannel, &tSystemModelPlatPrm, &tSystemModelInsidePrm);
    XSP_CHECK(0 == ret, XRAY_LIB_AI_XSP_INIT_ERROR, "LibXRay: Failed to initialize the AI_XSP.");
    log_info("AI_XSP Initialize success.");

    // 初始化异步处理线程池
    hr = InitAsyncThread();
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "LibXRay: Failed to initialize async thread pool.");

    // 初始化异步内存池
    InitAsyncTaskMemPool();

    return XRAY_LIB_OK;
}

/**
 * @brief 初始化内存池
 * @note 预分配内存池中的内存块
 */
void CnnModule::InitAsyncTaskMemPool()
{
    std::lock_guard<std::mutex> lock(m_memoryPoolMutex);

    log_info("Initializing memory pool with %zu entries", m_poolSize);

    m_memoryPool.clear();
    m_memoryPool.resize(m_poolSize);

    // 初始化所有内存池条目
    for (size_t i = 0; i < m_poolSize; i++)
    {
        m_memoryPool[i].inUse = false;
        m_memoryPool[i].inputLowMem.clear();
        m_memoryPool[i].inputHighMem.clear();
        m_memoryPool[i].inputZDataMem.clear();
        m_memoryPool[i].outputLowMem.clear();
        m_memoryPool[i].outputHighMem.clear();
        m_memoryPool[i].outputZDataMem.clear();

        m_memoryPool[i].inputLowSize = 0;
        m_memoryPool[i].inputHighSize = 0;
        m_memoryPool[i].inputZDataSize = 0;
        m_memoryPool[i].outputLowSize = 0;
        m_memoryPool[i].outputHighSize = 0;
        m_memoryPool[i].outputZDataSize = 0;
    }

    log_info("Memory pool initialized successfully");
}

/**
 * @brief 释放内存池
 * @note 释放内存池占用的所有资源
 */
void CnnModule::ReleaseMemoryPool()
{
    std::lock_guard<std::mutex> lock(m_memoryPoolMutex);

    log_info("Releasing memory pool");

    for (size_t i = 0; i < m_poolSize; i++)
    {
        m_memoryPool[i].inUse = false;
        m_memoryPool[i].inputLowMem.clear();
        m_memoryPool[i].inputHighMem.clear();
        m_memoryPool[i].inputZDataMem.clear();
        m_memoryPool[i].outputLowMem.clear();
        m_memoryPool[i].outputHighMem.clear();
        m_memoryPool[i].outputZDataMem.clear();
    }

    m_memoryPool.clear();
    log_info("Memory pool released successfully");
}

/**
 * @brief 获取内存池中的内存块
 * @return std::shared_ptr<AsyncTaskMemoryPool> 内存块指针
 * @note 从内存池中获取一个可用的内存块
 */
std::shared_ptr<AsyncTaskMemoryPool> CnnModule::AcquireAsyncMemPool()
{
    std::lock_guard<std::mutex> lock(m_memoryPoolMutex);

    // 查找未使用的内存块
    for (size_t i = 0; i < m_poolSize; i++)
    {
        if (!m_memoryPool[i].inUse)
        {
            m_memoryPool[i].inUse = true;
            log_debug("Acquired memory pool entry %zu", i);
            return std::shared_ptr<AsyncTaskMemoryPool>(&m_memoryPool[i],
                                                        [this](AsyncTaskMemoryPool *pool)
                                                        {
                                                            std::lock_guard<std::mutex> lock(m_memoryPoolMutex);
                                                            pool->inUse = false;
                                                            log_debug("Released memory pool entry");
                                                        });
        }
    }

    log_error("No available memory pool entries, creating new one");
    // 如果没有可用的内存块，创建一个新的
    auto newPool = std::make_shared<AsyncTaskMemoryPool>();
    return newPool;
}

/**
 * @brief 更新内存池大小
 * @param caliDataIn 输入数据（用于确定所需内存大小）
 * @param caliDataOut 输出数据（用于确定所需内存大小）
 * @note 根据输入输出数据的大小更新内存池中内存块的大小
 */
void CnnModule::UpdateAsyncMemPoolSize(const Calilhz &caliDataIn, const Calilhz &caliDataOut)
{
    std::lock_guard<std::mutex> lock(m_memoryPoolMutex);

    size_t inputLowSize = caliDataIn.low.size;
    size_t inputHighSize = caliDataIn.high.size;
    size_t inputZDataSize = caliDataIn.zData.size;
    size_t outputLowSize = caliDataOut.low.size;
    size_t outputHighSize = caliDataOut.high.size;
    size_t outputZDataSize = caliDataOut.zData.size;

    for (size_t i = 0; i < m_poolSize; i++)
    {
        if (!m_memoryPool[i].inUse)
        {
            if (m_memoryPool[i].inputLowSize != inputLowSize ||
                m_memoryPool[i].inputHighSize != inputHighSize ||
                m_memoryPool[i].inputZDataSize != inputZDataSize ||
                m_memoryPool[i].outputLowSize != outputLowSize ||
                m_memoryPool[i].outputHighSize != outputHighSize ||
                m_memoryPool[i].outputZDataSize != outputZDataSize)
            {
                // 重新分配内存
                m_memoryPool[i].inputLowMem.resize(inputLowSize);
                m_memoryPool[i].inputHighMem.resize(inputHighSize);
                m_memoryPool[i].inputZDataMem.resize(inputZDataSize);
                m_memoryPool[i].outputLowMem.resize(outputLowSize);
                m_memoryPool[i].outputHighMem.resize(outputHighSize);
                m_memoryPool[i].outputZDataMem.resize(outputZDataSize);

                // 更新大小信息
                m_memoryPool[i].inputLowSize = inputLowSize;
                m_memoryPool[i].inputHighSize = inputHighSize;
                m_memoryPool[i].inputZDataSize = inputZDataSize;
                m_memoryPool[i].outputLowSize = outputLowSize;
                m_memoryPool[i].outputHighSize = outputHighSize;
                m_memoryPool[i].outputZDataSize = outputZDataSize;

                log_debug("Updated memory pool entry %zu sizes", i);
            }
        }
    }
}

/**
 * @brief 释放接口
 * @return XRAY_LIB_HRESULT 错误码
 * @note 释放CNN模块占用的资源，包括AI模型和异步线程池
 */
XRAY_LIB_HRESULT CnnModule::Release()
{
    log_info("Releasing CNN module resources");

    // 清理异步线程池
    if (m_bAsyncRunning)
    {
        log_info("Cleaning up async thread pool");
        CleanupAsyncThreadPool();
    }

    // 释放异步处理内存池
    ReleaseMemoryPool();

    if (m_stCnnPara.nUseAI != 0 && m_stCnnPara.plat_mode == 0)
    {
        // 自研AI-XSP释放资源
        XRAY_LIB_HRESULT hr = AI_XSP_DeInit(m_stCnnPara.nChannel);
        XSP_CHECK(0 == hr, XRAY_LIB_AI_XSP_RELEASE_ERR, "LibXRay: AIXSP release failed.");
        log_info("AI_XSP resources released");
    }

    log_info("CNN module release completed");
    return XRAY_LIB_OK;
}

/**
 * @brief 获取内存表所需内存大小(字节单位)
 * @param MemTab [O] 内存表
 * @param ability [I] 算法库能力集
 * @return XRAY_LIB_HRESULT 错误码
 * @note 算法库所需内存由外部申请, 需提前计算所需内存大小
 */
XRAY_LIB_HRESULT CnnModule::GetMemSize(XRAY_LIB_MEM_TAB &MemTab, XRAY_LIB_ABILITY &ability)
{
    MemTab.size = 0;

    // 计算实时处理的高度和宽度
    int32_t nRTProcessHeight = (ability.nMaxHeightRealTime + XRAY_LIB_MAX_FILTER_KERNEL_LENGTH * 2) * XRAY_LIB_MAX_RESIZE_FACTOR;
    int32_t nRTProcessWidth = ability.nDetectorWidth * XRAY_LIB_MAX_RESIZE_FACTOR;

    log_info("Calculating memory requirements - process size: %dx%d", nRTProcessWidth, nRTProcessHeight);

    if (XRAYLIB_ENERGY_SCATTER == ability.nEnergyMode)
    {
        // OPENVINO内存
        m_matOPENVINOReceiveTemp.SetMem(ability.fResizeScale * nRTProcessWidth * nRTProcessHeight * XSP_ELEM_SIZE(XSP_32F));
        XspMalloc((void **)&m_matOPENVINOReceiveTemp.data, m_matOPENVINOReceiveTemp.Size(), MemTab);
    }
    else
    {
        // 透射模式下的内存分配
        m_matAIXSPReceive.SetMem(3 * nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
        XspMalloc((void **)&m_matAIXSPReceive.data, m_matAIXSPReceive.Size(), MemTab);

        m_matTempHwc.SetMem(3 * nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
        XspMalloc((void **)&m_matTempHwc.data, m_matTempHwc.Size(), MemTab);

        m_matAiInChn0.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
        XspMalloc((void **)&m_matAiInChn0.data, m_matAiInChn0.Size(), MemTab);

        m_matAiInChn1.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
        XspMalloc((void **)&m_matAiInChn1.data, m_matAiInChn1.Size(), MemTab);

        m_matAiInChn2.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
        XspMalloc((void **)&m_matAiInChn2.data, m_matAiInChn2.Size(), MemTab);

        m_matAiOutChn0.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
        XspMalloc((void **)&m_matAiOutChn0.data, m_matAiOutChn0.Size(), MemTab);

        m_matAiOutChn1.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
        XspMalloc((void **)&m_matAiOutChn1.data, m_matAiOutChn1.Size(), MemTab);

        m_matAiOutChn2.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
        XspMalloc((void **)&m_matAiOutChn2.data, m_matAiOutChn2.Size(), MemTab);
    }

    log_info("Total memory required: %d bytes", MemTab.size);
    return XRAY_LIB_OK;
}

/**
 * @brief AIXSP处理（同步）
 * @param caliDataIn 输入归一化数据
 * @param caliDataOut 输出归一化数据
 * @return XRAY_LIB_HRESULT 错误码
 * @note 根据能量模式选择透射或背散射处理
 */
XRAY_LIB_HRESULT CnnModule::AiXsp_SyncProcess(Calilhz &caliDataIn, Calilhz &caliDataOut)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;

    // 如果启用了AI-XSP且为非平台模式
    if (0 != m_stCnnPara.nUseAI && 0 == m_stCnnPara.plat_mode)
    {
        if (XRAYLIB_ENERGY_SCATTER == m_pSharedPara->m_enEnergyMode)
        {
            hr = BackScatterProcess(caliDataIn.low, caliDataOut.low);
            XSP_CHECK(XRAY_LIB_OK == hr, hr, "LibXRay: BackScatterProcess failed.Return Value: %X.", hr);
        }
        else
        {
            hr = TransMissionProcess(caliDataIn, caliDataOut);
            XSP_CHECK(XRAY_LIB_OK == hr, hr, "LibXRay: AiXsp_SyncProcess failed.Return Value: %X.", hr);
        }
        return hr;
    }

    // 如果未启用AI-XSP，直接复制数据
    log_info("AI_XSP not enabled, copying data directly");
    hr = CopyDataWithoutAI(caliDataIn, caliDataOut);
    return hr;
}

/**
 * @brief AIXSP预处理，内部分为投射和背散两种模式
 * @return XRAY_LIB_HRESULT 错误码
 * @note 根据能量模式选择透射或背散射处理
 */
XRAY_LIB_HRESULT CnnModule::AiXsp_PreProcess(XMat &matInLow, XMat &matInHigh, Calilhz &caliAIXspIn, Calilhz &caliAIXspOut)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;

    // 如果启用了AI-XSP且为非平台模式
    if (0 != m_stCnnPara.nUseAI && 0 == m_stCnnPara.plat_mode)
    {
        if (XRAYLIB_ENERGY_SCATTER == m_pSharedPara->m_enEnergyMode)
        {
            return XRAY_LIB_OK;
        }
        else
        {
            hr = TransMissionPreProcess(matInLow, matInHigh, caliAIXspIn, caliAIXspOut);
            XSP_CHECK(XRAY_LIB_OK == hr, hr, "CnnModule: TransMissionPreProcess failed.Return Value: %X.", hr);
        }
    }
    return hr;
}

/**
 * @brief AIXSP预处理
 * @param matInLow 低能输入图像
 * @param matInHigh 高能输入图像
 * @param caliAIXspIn 输入归一化数据
 * @param caliAIXspOut 输出归一化数据
 * @return XRAY_LIB_HRESULT 错误码
 * @note 对输入图像进行预处理，准备AIXSP处理所需的数据格式
 */
XRAY_LIB_HRESULT CnnModule::TransMissionPreProcess(XMat &matInLow, XMat &matInHigh, Calilhz &caliAIXspIn, Calilhz &caliAIXspOut)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;

    auto sFactor = AiXsp_GetScale();

    // 若开启AIXSP
    if (0 != m_stCnnPara.nUseAI && 0 == m_stCnnPara.plat_mode)
    {
        // 计算AIXSP送入的条带尺寸
        int32_t nAIXSPHeiIn = matInLow.hei;
        int32_t nAIXSPWidIn = matInLow.wid;

        // 计算AIXSP输出的条带尺寸
        int32_t nAIXSPHeiOut = static_cast<int32_t>(nAIXSPHeiIn * sFactor.first);
        int32_t nAIXSPWidOut = static_cast<int32_t>(nAIXSPWidIn * sFactor.second);

        // 初始化输入矩阵
        hr = m_matAiInChn0.Reshape(nAIXSPHeiIn, nAIXSPWidIn, XSP_32FC1);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "MatAiInChn0 init failed.");
        hr = m_matAiInChn1.Reshape(nAIXSPHeiIn, nAIXSPWidIn, XSP_32FC1);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "MatAiInChn1 init failed.");
        hr = m_matAiInChn2.Reshape(nAIXSPHeiIn, nAIXSPWidIn, XSP_32FC1);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "MatAiInChn2 init failed.");

        // 数据类型转换和归一化
        ImgConvertType(matInLow.Ptr<uint16_t>(), m_matAiInChn0.Ptr<float32_t>(), matInLow.size, 1.0f / 65535.0f, 0.0f, 1.0f);
        ImgConvertType(matInHigh.Ptr<uint16_t>(), m_matAiInChn1.Ptr<float32_t>(), matInHigh.size, 1.0f / 65535.0f, 0.0f, 1.0f);

        // 输入输出AIXSP数据初始化
        hr = caliAIXspIn.low.Init(m_matAiInChn0.hei, m_matAiInChn0.wid, XSP_32FC1, (uint8_t *)m_matAiInChn0.data.ptr);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx1: Get in_calilhz low data failed.");

        hr = caliAIXspIn.high.Init(m_matAiInChn1.hei, m_matAiInChn1.wid, XSP_32FC1, (uint8_t *)m_matAiInChn1.data.ptr);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx1: Get in_calilhz high data failed.");

        hr = caliAIXspIn.zData.Init(m_matAiInChn2.hei, m_matAiInChn2.wid, XSP_32FC1, (uint8_t *)m_matAiInChn2.data.ptr);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx1: Get in_calilhz high data failed.");

        hr = caliAIXspOut.low.Init(nAIXSPHeiOut, nAIXSPWidOut, XSP_32FC1, (uint8_t *)m_matAiOutChn0.data.ptr);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: Get out_calilhz l data failed.");

        hr = caliAIXspOut.high.Init(nAIXSPHeiOut, nAIXSPWidOut, XSP_32FC1, (uint8_t *)m_matAiOutChn1.data.ptr);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: Get out_calilhz h data failed.");

        hr = caliAIXspOut.zData.Init(nAIXSPHeiOut, nAIXSPWidOut, XSP_32FC1, (uint8_t *)m_matAiOutChn2.data.ptr);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: Get out_calilhz h data failed.");
    }
    else
    {
        log_info("AI_XSP PreProcess: AI is not enabled");
    }

    return hr;
}

/**
 * @brief 投射机型AIXSP处理
 * @param caliDataIn 输入归一化数据
 * @param caliDataOut 输出归一化数据
 * @return XRAY_LIB_HRESULT 错误码
 * @note 透射模式下AIXSP处理的具体实现，包括数据预处理、AI推理、后处理
 */
XRAY_LIB_HRESULT CnnModule::TransMissionProcess(Calilhz &caliDataIn, Calilhz &caliDataOut)
{
    // 检查输入数据有效性
    XSP_CHECK(caliDataIn.high.IsValid() && caliDataIn.low.IsValid() && caliDataIn.zData.IsValid(),
              XRAY_LIB_NULLPTR, "ImageIn ptr is Null.");

    XSP_CHECK(MatSizeEq(caliDataIn.high, caliDataIn.low) && MatSizeEq(caliDataIn.high, caliDataIn.zData),
              XRAY_LIB_XMAT_SIZE_ERR);

    XSP_CHECK(caliDataIn.low.wid <= m_stCnnPara.AI_width_in, XRAY_LIB_AI_XSP_HEIGHT_OVERFLOW,
              "width in mismatch. width in : %d, width ai in : %d.", caliDataIn.low.wid, m_stCnnPara.AI_width_in);

    XRAY_LIB_HRESULT hr;

    // 获取输入数据尺寸和参数
    int32_t nWidIn = caliDataIn.low.wid;
    int32_t nHeiIn = caliDataIn.low.hei;
    int32_t nAiXspNB = AiXsp_GetNbLen();
    auto sFactor = AiXsp_GetScale();

    /****************************
     *      输入初始化
     ****************************/
    XMat matLowIn, matHighIn, matZDataIn;
    int32_t nAIHeightIn = nHeiIn; // 实际条带输入高度
    int32_t nAIWidthIn = nWidIn;  // 实际条带输入宽度

    hr = matLowIn.Init(nAIHeightIn, nAIWidthIn, XSP_32FC1, caliDataIn.low.Ptr());
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "matLowIn Init failed.");
    hr = matHighIn.Init(nAIHeightIn, nAIWidthIn, XSP_32FC1, caliDataIn.high.Ptr());
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "matHighIn Init failed.");
    hr = matZDataIn.Init(nAIHeightIn, nAIWidthIn, XSP_32FC1, caliDataIn.zData.Ptr());
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "matZDataIn Init failed.");

    float32_t *pLowIn = matLowIn.Ptr<float32_t>();
    float32_t *pHighIn = matHighIn.Ptr<float32_t>();
    float32_t *pZDataIn = matZDataIn.Ptr<float32_t>();

    /****************************
     *      输出初始化
     ****************************/
    XMat matLowOut, matHighOut, matZDataOut;
    int32_t nAIHeightOut = caliDataOut.low.hei; // 实际条带输出高度
    int32_t nAIWidthOut = caliDataOut.low.wid;  // 实际条带输出宽度

    // 输出数据尺寸确定
    hr = caliDataOut.low.Reshape(nAIHeightOut, nAIWidthOut, XSP_32FC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "caliDataOut.low Reshape failed.");
    hr = caliDataOut.high.Reshape(nAIHeightOut, nAIWidthOut, XSP_32FC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "caliDataOut.high Reshape failed.");
    hr = caliDataOut.zData.Reshape(nAIHeightOut, nAIWidthOut, XSP_32FC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "caliDataOut.zData Reshape failed.");

    hr = matLowOut.Init(nAIHeightOut, nAIWidthOut, XSP_32FC1, caliDataOut.low.Ptr());
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "matLowOut Init failed.");
    hr = matHighOut.Init(nAIHeightOut, nAIWidthOut, XSP_32FC1, caliDataOut.high.Ptr());
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "matHighOut Init failed.");
    hr = matZDataOut.Init(nAIHeightOut, nAIWidthOut, XSP_32FC1, caliDataOut.zData.Ptr());
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "matZDataOut Init failed.");

    float32_t *pLowOut = matLowOut.Ptr<float32_t>();
    float32_t *pHighOut = matHighOut.Ptr<float32_t>();
    float32_t *pZDataOut = matZDataOut.Ptr<float32_t>();

    /****************************
     *   CNN模块FP32输入初始化
     ****************************/
    // AIXSP输入数据整合
    hr = m_matAIXSPReceive.Reshape(m_stCnnPara.AI_height_in * m_stCnnPara.AI_channels_in, m_stCnnPara.AI_width_in, XSP_32FC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "m_matAIXSPReceive Reshape failed.");

    XMat matLowAi, matHighAi, matZDataAi;
    hr = matLowAi.Init(m_stCnnPara.AI_height_in, m_stCnnPara.AI_width_in, XSP_32FC1, m_matAIXSPReceive.Ptr());
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "matLowAi Init failed.");
    hr = matHighAi.Init(m_stCnnPara.AI_height_in, m_stCnnPara.AI_width_in, XSP_32FC1, m_matAIXSPReceive.Ptr(m_stCnnPara.AI_height_in));
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "matHighAi Init failed.");
    hr = matZDataAi.Init(m_stCnnPara.AI_height_in, m_stCnnPara.AI_width_in, XSP_32FC1, m_matAIXSPReceive.Ptr(m_stCnnPara.AI_height_in * 2));
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "matZDataAi Init failed.");

    // 输入类型转化为FP32
    float32_t *pReceiveLow = matLowAi.Ptr<float32_t>();
    float32_t *pReceiveHigh = matHighAi.Ptr<float32_t>();
    float32_t *pReceiveZData = matZDataAi.Ptr<float32_t>();

    // 数据拷贝时左侧需要预留出AI_XSP_MIN_NB的邻边
    for (int32_t i = 0; i < nAIHeightIn; i++)
    {
        memcpy(pReceiveLow + i * m_stCnnPara.AI_width_in + nAiXspNB, pLowIn + i * nAIWidthIn, nAIWidthIn * sizeof(float32_t));
        memcpy(pReceiveHigh + i * m_stCnnPara.AI_width_in + nAiXspNB, pHighIn + i * nAIWidthIn, nAIWidthIn * sizeof(float32_t));
        memcpy(pReceiveZData + i * m_stCnnPara.AI_width_in + nAiXspNB, pZDataIn + i * nAIWidthIn, nAIWidthIn * sizeof(float32_t));
    }

    // 输入数据各通道下部AI_XSP_MIN_NB高度需要做镜像处理，使得当前输出条带下部数据正常处理
    for (int32_t nh = 0; nh < nAiXspNB; nh++)
    {
        memcpy(m_matAIXSPReceive.Ptr(nAIHeightIn + nh), m_matAIXSPReceive.Ptr(nAIHeightIn - nh - 1),
               (nAIWidthIn + nAiXspNB) * sizeof(float32_t));
        memcpy(m_matAIXSPReceive.Ptr(nAIHeightIn + m_stCnnPara.AI_height_in + nh),
               m_matAIXSPReceive.Ptr(nAIHeightIn + m_stCnnPara.AI_height_in - nh - 1),
               (nAIWidthIn + nAiXspNB) * sizeof(float32_t));
        memcpy(m_matAIXSPReceive.Ptr(nAIHeightIn + 2 * m_stCnnPara.AI_height_in + nh),
               m_matAIXSPReceive.Ptr(nAIHeightIn + 2 * m_stCnnPara.AI_height_in - nh - 1),
               (nAIWidthIn + nAiXspNB) * sizeof(float32_t));
    }

    // 两侧数据进行对称补充，确保两侧数据经过AIXSP正常处理
    for (int32_t nh = 0; nh < nAIHeightIn + nAiXspNB; nh++)
    {
        float32_t *pLow = matLowAi.Ptr<float32_t>(nh);
        float32_t *pHigh = matHighAi.Ptr<float32_t>(nh);
        float32_t *pZData = matZDataAi.Ptr<float32_t>(nh);
        for (int32_t nw = 0; nw < nAiXspNB; nw++)
        {
            // 左边沿镜像复制
            pLow[nw] = pLow[2 * nAiXspNB - nw];
            pHigh[nw] = pHigh[2 * nAiXspNB - nw];
            pZData[nw] = pZData[2 * nAiXspNB - nw];

            // 右边沿镜像复制
            pLow[nAIWidthIn + nAiXspNB + nw] = pLow[nAIWidthIn + nAiXspNB - nw - 1];
            pHigh[nAIWidthIn + nAiXspNB + nw] = pHigh[nAIWidthIn + nAiXspNB - nw - 1];
            pZData[nAIWidthIn + nAiXspNB + nw] = pZData[nAIWidthIn + nAiXspNB - nw - 1];
        }
    }

    // RK开发版的数据输入格式与其他开发板不同，需额外做一次转换
    if (SYS_RK_PLAT == m_stCnnPara.systemPlat)
    {
        hr = m_matTempHwc.Reshape(m_stCnnPara.AI_height_in, m_stCnnPara.AI_width_in * m_stCnnPara.AI_channels_in, XSP_32FC1);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "m_matTempHwc Reshape failed.");
        ConvertChwToHwc(m_stCnnPara.AI_channels_in, m_matAIXSPReceive, m_matTempHwc);
        memcpy(m_matAIXSPReceive.Ptr(), m_matTempHwc.Ptr(), m_stCnnPara.AI_height_in * m_stCnnPara.AI_width_in * m_stCnnPara.AI_channels_in * sizeof(float));
    }

    /****************************
     *     AI_XSP接口调用
     ****************************/
    int32_t nLengthIn = m_stCnnPara.AI_height_in * m_stCnnPara.AI_width_in * m_stCnnPara.AI_channels_in * sizeof(float32_t);
    int32_t nLengthOut = 0;
    int32_t nChannel = m_stCnnPara.nChannel;
    void *pData = LIBXRAY_NULL;
    int32_t ret = 0;

    ret = AI_XSP_RunProcess(nChannel, m_matAIXSPReceive.Ptr(), nLengthIn);
    XSP_CHECK(0 == ret, XRAY_LIB_AI_XSP_RUN_ERROR, "AI_XSP_RunProcess failed.");

    ret = AI_XSP_GetResult(nChannel, &pData, &nLengthOut);
    XSP_CHECK(0 == ret, XRAY_LIB_AI_XSP_GET_ERROR, "AI_XSP_GetResult failed.");

    /****************************
     *       后处理操作
     ****************************/
    XMat matAiResult;
    hr = matAiResult.Init(m_stCnnPara.AI_height_out * m_stCnnPara.AI_channels_out, m_stCnnPara.AI_width_out, XSP_32FC1, (uint8_t *)pData);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "matAiResult Init failed.");

    for (int32_t i = 0; i < nAIHeightOut; i++)
    {
        memcpy(pLowOut + i * matLowOut.wid, matAiResult.Ptr<float32_t>(i) + static_cast<int32_t>(sFactor.first * nAiXspNB),
               nAIWidthOut * sizeof(float32_t));
        memcpy(pHighOut + i * matHighOut.wid, matAiResult.Ptr<float32_t>(m_stCnnPara.AI_height_out + i) + static_cast<int32_t>(sFactor.second * nAiXspNB),
               nAIWidthOut * sizeof(float32_t));
        memcpy(pZDataOut + i * matZDataOut.wid, matAiResult.Ptr<float32_t>(m_stCnnPara.AI_height_out * 2 + i) + static_cast<int32_t>(sFactor.second * nAiXspNB),
               nAIWidthOut * sizeof(float32_t));
    }

    return XRAY_LIB_OK;
}

/**
 * @brief 背散射AIXSP处理
 * @param matIn 输入图像
 * @param matOut 输出图像
 * @return XRAY_LIB_HRESULT 错误码
 * @note 背散射模式下AIXSP处理的具体实现，单通道处理
 */
XRAY_LIB_HRESULT CnnModule::BackScatterProcess(XMat &matIn, XMat &matOut)
{
    XSP_CHECK(matIn.IsValid(), XRAY_LIB_XMAT_INVALID, "MatIn Error.");
    XSP_CHECK(matOut.IsValid(), XRAY_LIB_XMAT_INVALID, "MatOut Error.");

    XRAY_LIB_HRESULT hr;

    int32_t nHeight = matIn.hei;
    int32_t nWidth = matIn.wid;

    hr = m_matOPENVINOReceiveTemp.Reshape(m_stCnnPara.AI_height_in, m_stCnnPara.AI_width_in, XSP_32F);
    XSP_CHECK(hr == XRAY_LIB_OK, XRAY_LIB_XMAT_INVALID, "m_matOPENVINOReceiveTemp Reshape Failed.");

    /* 16位无符号整型映射成float型并归一化，模型输入做预处理 */
    // 探测器方向需要补的总大小
    int32_t nWidInPadFillNeed = nWidth - m_stCnnPara.AI_width_in;
    // 时间轴方向需要补的总大小
    int32_t nHeiInPadFillNeed = nHeight - m_stCnnPara.AI_height_in;
    // 如果当前输入Hei>模型需要处理的Ai_Hei, 单边需要跳过的高度
    int32_t nHeiProcStart = 0, nWidProcLeft = 0;

    nHeiProcStart = nHeiInPadFillNeed / 2;
    nWidProcLeft = nWidInPadFillNeed / 2;

    for (int32_t nAih = 0, nhIn = nHeiProcStart; nAih < m_stCnnPara.AI_height_in; nAih++, nhIn++)
    {
        for (int32_t nAiw = 0, nwIn = nWidProcLeft; nAiw < m_stCnnPara.AI_width_in; nAiw++, nwIn++)
        {
            m_matOPENVINOReceiveTemp.Ptr<float32_t>(nAih, nAiw)[0] = static_cast<float32_t>(matIn.NeighborPtr<uint16_t>(nhIn, nwIn)[0]) / 65535.0f / m_fGrayAiRatio;
        }
    }

    float32_t fGrayValue = 0.0f;
    int32_t length = nHeight * nWidth;
    int32_t nLengthResult = 0;
    int32_t nChannel = m_stCnnPara.nChannel; // 视角通道号 需验证是否为1

    // 运行AI模型
    hr = AI_XSP_RunProcess(nChannel, m_matOPENVINOReceiveTemp.Ptr(), length * sizeof(float32_t));
    if (0 != hr)
    {
        log_error("AI_XSP_RunProcess failed with error: %d", hr);
        return XRAY_LIB_AI_XSP_RUN_ERROR;
    }

    // 获取模型结果
    void *ptrOPENVINOOut = LIBXRAY_NULL;
    hr = AI_XSP_GetResult(nChannel, &ptrOPENVINOOut, &nLengthResult);
    if (0 != hr)
    {
        log_error("AI_XSP_GetResult failed with error: %d", hr);
        return XRAY_LIB_AI_XSP_GET_ERROR;
    }
    
    XMat matAiXspOut;
    matAiXspOut.Init(m_stCnnPara.AI_height_out, m_stCnnPara.AI_width_out, XSP_32FC1, static_cast<uint8_t*>(ptrOPENVINOOut));

    // 处理模型输出结果
    for (int32_t nh = 0, nAih = nHeiProcStart > 0 ? 0 : std::abs(nHeiProcStart); nh < matOut.hei; nh++, nAih++)
    {
        for (int32_t nw = 0, nAiw = nWidProcLeft > 0 ? 0 : std::abs(nWidProcLeft); nw < matOut.wid; nw++, nAiw++)
        {
            fGrayValue = matAiXspOut.Ptr<float32_t>(nAih, nAiw)[0];
            fGrayValue = MAX(0.0, MIN(1.0, fGrayValue));
            fGrayValue *= (65535 * m_fGrayAiRatio);
            fGrayValue = Clamp(fGrayValue, 0.0f, 65535.0f);
            matOut.Ptr<uint16_t>(nh, nw)[0] = (uint16_t)fGrayValue;
        }
    }

    return XRAY_LIB_OK;
}

/**
 * @brief 数据格式切换，CHW转HWC
 * @param nChannel 图像通道数
 * @param src 输入图像
 * @param des 输出图像
 * @return XRAY_LIB_HRESULT 错误码
 * @note 目前只有RK平台需要此转换，将通道优先格式转换为高度-宽度-通道格式
 */
XRAY_LIB_HRESULT CnnModule::ConvertChwToHwc(int nChannel, XMat &src, XMat &des)
{
    XSP_CHECK(src.IsValid() && des.IsValid(), XRAY_LIB_NULLPTR, "LibXRay: Image ptr is Null.");

    for (int c = 0; c < nChannel; ++c)
    {
        for (int h = 0; h < src.hei / nChannel; ++h)
        {
            for (int w = 0; w < src.wid; ++w)
            {
                int dstIdx = h * src.wid * nChannel + w * nChannel + c;
                int srcIdx = c * src.hei / nChannel * src.wid + h * src.wid + w;
                des.Ptr<float32_t>()[dstIdx] = src.Ptr<float32_t>()[srcIdx];
            }
        }
    }
    return XRAY_LIB_OK;
}

/**
 * @brief 获取高、宽的缩放比例
 * @return std::pair<float32_t, float32_t> heiScale : 高缩放比例， widScale : 宽缩放比例
 * @note 返回当前模型输入输出之间的缩放比例
 */
std::pair<float32_t, float32_t> CnnModule::AiXsp_GetScale()
{    
    std::pair<float32_t, float32_t> scaleRatio = std::make_pair(1.0f, 1.0f);

    // 检查输入尺寸是否为零，防止除零错误
    if (m_tModelInfo.AI_width_in == 0 || m_tModelInfo.AI_height_in == 0)
    {
        return scaleRatio;
    }

    float32_t fWidScaleFactor = float32_t(m_tModelInfo.AI_width_out) / float32_t(m_tModelInfo.AI_width_in);
    float32_t fHeiScaleFactor = float32_t(m_tModelInfo.AI_height_out) / float32_t(m_tModelInfo.AI_height_in);
    scaleRatio = std::make_pair(fHeiScaleFactor, fWidScaleFactor);

    log_debug("Scale ratio calculated - height: %.2f, width: %.2f",
              fHeiScaleFactor, fWidScaleFactor);

    return scaleRatio;
}

/**
 * @brief 获取当前模型输入扩边大小
 * @return int32_t 扩边大小
 * @note 根据模型类型返回不同的扩边大小
 */
int32_t CnnModule::AiXsp_GetNbLen()
{
    // 目前背散只支持该邻边模型
    if (XRAYLIB_ENERGY_SCATTER == m_pSharedPara->m_enEnergyMode)
    {
        return AI_XSP_BS_MIN_NB;
    }
    else
    {
        int32_t neighborLen = (m_tModelInfo.AI_width_out == m_tModelInfo.AI_width_in &&
                           m_tModelInfo.AI_height_out == m_tModelInfo.AI_height_in)
                              ? AI_XSP_DN_MIN_NB
                              : AI_XSP_SR_MIN_NB;

        log_debug("Neighbor length determined: %d (DN:%d, SR:%d)",
                neighborLen, AI_XSP_DN_MIN_NB, AI_XSP_SR_MIN_NB);
        return neighborLen;
    }
}

/**
 * @brief 文件路径初始化
 * @param szPublicFileFolderName 公共文件夹路径
 * @param stXRaylib_AIPara AI-XSP初始化参数
 * @return XRAY_LIB_HRESULT 错误码
 * @note 根据平台类型初始化模型文件路径
 */
XRAY_LIB_HRESULT CnnModule::InitModelSelector(const char *szPublicFileFolderName, XRAYLIB_AI_PARAM stXRaylib_AIPara)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    int32_t nPathLength = XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME;
    memset(m_szPathDLModelName, 0, sizeof(char) * nPathLength);
    memset(m_szPathDLModelFile, 0, sizeof(char) * nPathLength);
    memset(m_szPathDLModelPath, 0, sizeof(char) * nPathLength);
    strcpy(m_szPathDLModelPath, szPublicFileFolderName);

    // 根据平台类型设置不同的模型路径
    if (X86_INTEL_GPU == stXRaylib_AIPara.systemProcessType)
    {
        if (XRAYLIB_ENERGY_SCATTER == m_pSharedPara->m_enEnergyMode)
        {
            strcat(m_szPathDLModelPath, "/AI_XSP_MODEL/intel/backscatter/");
            hr = m_filematcher.init(m_szPathDLModelPath, "xml");
        }
        else
        {
            strcat(m_szPathDLModelPath, "/AI_XSP_MODEL/intel/");
            hr = m_filematcher.init(m_szPathDLModelPath, "xml");
        }
    }
    else if (X86_NVIDIA_GPU == stXRaylib_AIPara.systemProcessType && 0 == stXRaylib_AIPara.plat_mode)
    {
        strcat(m_szPathDLModelPath, "/AI_XSP_MODEL/nvidia/");
        hr = m_filematcher.init(m_szPathDLModelPath, "onnx");
    }
    else if (X86_NVIDIA_GPU == stXRaylib_AIPara.systemProcessType && 1 == stXRaylib_AIPara.plat_mode)
    {
        strcat(m_szPathDLModelPath, "/AI_XSP_MODEL/nvidia_research/");
        hr = m_filematcher.init(m_szPathDLModelPath, "onnx");
    }
    else if (SYS_RK_PLAT == stXRaylib_AIPara.systemPlat && 0 == stXRaylib_AIPara.plat_mode)
    {
        strcat(m_szPathDLModelPath, "/AI_XSP_MODEL/rk/");
        hr = m_filematcher.init(m_szPathDLModelPath, "rknn");
    }
    else if (SYS_RK_PLAT == stXRaylib_AIPara.systemPlat && 1 == stXRaylib_AIPara.plat_mode)
    {
        strcat(m_szPathDLModelPath, "/AI_XSP_MODEL/rk_research/");
        hr = m_filematcher.init(m_szPathDLModelPath, "rknn");
    }
    else if (SYS_SIHI_PLAT == stXRaylib_AIPara.systemPlat && 1 == stXRaylib_AIPara.plat_mode)
    {
        strcat(m_szPathDLModelPath, "/AI_XSP_MODEL/hisi_research/");
        hr = m_filematcher.init(m_szPathDLModelPath, "bin");
    }
    else
    {
        log_error("xraylib_AIPara.systemProcessType is %d, xraylib_AIPara.systemPlat is %d, xraylib_AIPara.plat_mode is %d",
                  stXRaylib_AIPara.systemProcessType, stXRaylib_AIPara.systemPlat, stXRaylib_AIPara.plat_mode);
        log_warn("Unknown Plat Type.");
        hr = XRAY_LIB_AI_XSP_INIT_ERROR;
    }

    log_info("Model path initialized: %s", m_szPathDLModelPath);
    return hr;
}

/**
 * @brief 根据图像大小选择模型
 * @return XRAY_LIB_HRESULT 错误码
 * @note 根据输入图像大小选择合适的AI模型,主要用于初始化时初步选择模型
 */
XRAY_LIB_HRESULT CnnModule::InitModelParam()
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    AiXspFileParams stAixspInit;
    float32_t fHeiScaleFactor = 1.0f;
    float32_t fWidScaleFactor = 1.0f;

    // 选择idx为0的模型，做初始化用
    try
    {
        stAixspInit = m_filematcher.get_idx_files(0);
    }
    catch (const std::out_of_range &e)
    {
        log_error("Failed to get initial model: %s", e.what());
        return XRAY_LIB_AI_XSP_INIT_ERROR;
    }

    // 解析缩放系数
    std::regex pattern(R"((\d+\.\d+)x(\d+\.\d+))"); // 正则表达式匹配格式2.0x2.0
    std::smatch matches;
    if (std::regex_match(stAixspInit.AiXspSf, matches, pattern))
    {
        if (3 == matches.size())
        {
            fHeiScaleFactor = std::stof(matches[1].str());
            fWidScaleFactor = std::stof(matches[2].str());
            log_info("Parsed scale factor: height=%.1f, width=%.1f", fHeiScaleFactor, fWidScaleFactor);
        }
    }
    else
    {
        log_error("Parse failure: Invalid string format for scale factor: %s", stAixspInit.AiXspSf.c_str());
        return XRAY_LIB_AI_XSP_INIT_ERROR;
    }

    // 设置模型参数
    strcpy(m_tModelInfo.strModelName, stAixspInit.AiXspModName.c_str());
    m_tModelInfo.AI_width_in = stAixspInit.AiXspWid;
    m_tModelInfo.AI_width_out = static_cast<int32_t>(stAixspInit.AiXspWid * fWidScaleFactor);
    m_tModelInfo.AI_height_in = stAixspInit.AiXspHei;
    m_tModelInfo.AI_height_out = static_cast<int32_t>(stAixspInit.AiXspHei * fHeiScaleFactor);
    m_tModelInfo.AI_channels_in = stAixspInit.AiXspChn;
    m_tModelInfo.AI_channels_out = stAixspInit.AiXspChn;

    log_info("Initial model parameters set - name: %s, input: %dx%d, output: %dx%d, channels: %d",
             m_tModelInfo.strModelName, m_tModelInfo.AI_width_in, m_tModelInfo.AI_height_in,
             m_tModelInfo.AI_width_out, m_tModelInfo.AI_height_out, m_tModelInfo.AI_channels_in);

    return hr;
}

/**
 * @brief 切换自研模型
 * @param DspHeight DSP要求的图像高度
 * @param DspWidth DSP要求的图像宽度
 * @param OriHeight 原始图像高度
 * @param OriWidth 原始图像宽度
 * @return XRAY_LIB_HRESULT 错误码
 * @note 根据能量模式选择透射或背散射模型切换
 */
XRAY_LIB_HRESULT CnnModule::AiXsp_SwitchModel(int32_t DspHeight, int32_t DspWidth,
                                              int32_t OriHeight, int32_t OriWidth)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;

    // 检查参数有效性
    if (DspHeight == 0 || DspWidth == 0 || m_stCnnPara.nUseAI == 0)
    {
        log_info("Skipping model switch - invalid parameters or AI not enabled");
        return XRAY_LIB_OK;
    }

    // 根据能量模式选择不同的切换方式
    if (XRAYLIB_ENERGY_SCATTER == m_pSharedPara->m_enEnergyMode)
    {
        log_info("backscatter model doesn't need be changed.");
        // hr = BackscatterSwitchModel(OriHeight, OriWidth);
        return XRAY_LIB_OK;
    }
    else
    {
        log_info("Switching transmission model");
        hr = TransmissionSwitchModel(DspHeight, DspWidth, OriHeight, OriWidth);
    }

    return hr;
}

/**
 * @brief 透射切换自研模型
 * @param DspHeight DSP要求的图像高度
 * @param DspWidth DSP要求的图像宽度
 * @param OriHeight 原始图像高度
 * @param OriWidth 原始图像宽度
 * @return XRAY_LIB_HRESULT 错误码
 * @note 透射模式下切换自研模型
 */
XRAY_LIB_HRESULT CnnModule::TransmissionSwitchModel(int32_t DspHeight, int32_t DspWidth,
                                                    int32_t OriHeight, int32_t OriWidth)
{
    if (DspHeight == 0 || DspWidth == 0)
    {
        return XRAY_LIB_OK;
    }

    int32_t nLowAirPossion = 3000; // 模型统一使用3000的强降噪模型
    int32_t channels = 3;          // 模型统一使用三通道输入
    int32_t nAiXspNB = AiXsp_GetNbLen();
    // float32_t fAiHeiSF = ((DspWidth == OriWidth) ? 1.0 : 2.0); // 高度缩放系数
    // float32_t fAiWidSF = ((DspWidth == OriWidth) ? 1.0 : 2.0); // 宽度缩放系数
    float32_t fAiHeiSF = 0.0f, fAiWidSF = 0.0f;
    if (DspWidth != OriWidth || DspHeight != OriHeight)
    {
        fAiHeiSF = 2.0;
        fAiWidSF = 2.0;
    }
    else
    {
        fAiHeiSF = 1.0;
        fAiWidSF = 1.0;
    }

    int32_t nInputSliceWidth = OriWidth + 2 * nAiXspNB;   // 需要输入的条带宽度
    int32_t nInputSliceHeight = OriHeight + 3 * nAiXspNB; // 需要输入的条带高度（条带原始高度+3倍最小邻边，其中两倍最小领边高度用于更新上个条带，一倍最小领边高度用于保证当前条带下边沿正常处理）

    // 使用 std::ostringstream 转换为字符串,并保留一位小数
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << fAiHeiSF << "x" << std::setprecision(1) << fAiWidSF;
    std::string ScalingFactor = ss.str();

    // 匹配合适的模型文件，输入宽度、输入高度、噪声、模型类型
    if (auto modelname = m_filematcher.find_match(channels, nInputSliceWidth, nInputSliceHeight, nLowAirPossion, ScalingFactor.c_str()))
    {
        // 校验当前模型和前一次模型是否一致，否则不切换模型
        if (strcmp(m_tModelInfo.strModelName, modelname.value().c_str()) != 0)
        {
            strcpy(m_tModelInfo.strModelName, modelname.value().c_str());
            m_tModelInfo.AI_width_in = m_filematcher.selected_values[1];
            m_tModelInfo.AI_width_out = m_filematcher.selected_values[1] * fAiWidSF;
            m_tModelInfo.AI_height_in = m_filematcher.selected_values[2];
            m_tModelInfo.AI_height_out = m_filematcher.selected_values[2] * fAiHeiSF;
            log_info("AI_XSP SwitchModel: Switch model to %s", m_tModelInfo.strModelName);
        }
        else
        {
            log_info("AI_XSP SwitchModel: Current model[%s] is already matched", m_tModelInfo.strModelName);
            return XRAY_LIB_OK;
        }
    }
    else
    {
        log_error("Transmission AiXsp_SwitchModel No model found for input size %dx%d, channels %d ScalingFactor %s", nInputSliceWidth, nInputSliceHeight, m_tModelInfo.AI_channels_out, ScalingFactor.c_str());
        return XRAY_LIB_OK;
    }

    m_stCnnPara.AI_channels_in = m_tModelInfo.AI_channels_in;
    m_stCnnPara.AI_width_in = m_tModelInfo.AI_width_in;
    m_stCnnPara.AI_height_in = m_tModelInfo.AI_height_in;
    m_stCnnPara.AI_channels_out = m_tModelInfo.AI_channels_out;
    m_stCnnPara.AI_width_out = m_tModelInfo.AI_width_out;
    m_stCnnPara.AI_height_out = m_tModelInfo.AI_height_out;

    strcpy(m_szPathDLModelName, m_tModelInfo.strModelName);
    strcpy(m_szPathDLModelFile, m_szPathDLModelPath);
    strcat(m_szPathDLModelFile, m_szPathDLModelName);

    XRAY_LIB_HRESULT hr;
    /* 自研AI-XSP释放 */
    hr = AI_XSP_DeInit(m_stCnnPara.nChannel);
    XSP_CHECK(0 == hr, XRAY_LIB_AI_XSP_RELEASE_ERR, "LibXRay: AIXSP release failed.");
    log_info("AI_XSP Release!");

    /* 自研AI-XSP初始化 */
    SYSTEM_MODEL_PLAT_PRM tSystemModelPlatPrm;
    const char *strPath = m_szPathDLModelFile;
    tSystemModelPlatPrm.Input_Pass_Through = 0;
    tSystemModelPlatPrm.Output_Pass_Through = 0;
    tSystemModelPlatPrm.pModelPath = strPath;
    tSystemModelPlatPrm.eSysTemplat = m_stCnnPara.systemPlat;
    tSystemModelPlatPrm.eSysProcessType = m_stCnnPara.systemProcessType;
    tSystemModelPlatPrm.eSysPrecisionType = m_stCnnPara.systemProcessPrecision;
    tSystemModelPlatPrm.sSysInputDataType = TENSOR_FLOAT32;
    tSystemModelPlatPrm.sSysOutputDataType = TENSOR_FLOAT32;
    tSystemModelPlatPrm.eSysCoreMask = m_stCnnPara.rayinSchedulerParam.aiSrNpuCore;

    /* AI_XSP高度 宽度与算法库定义相反 */
    SYSTEM_MODEL_INSIDE_PRM tSystemModelInsidePrm;
    tSystemModelInsidePrm.inputChnSize = m_stCnnPara.AI_channels_in;
    tSystemModelInsidePrm.outputChnSize = m_stCnnPara.AI_channels_out;
    tSystemModelInsidePrm.inputHei = m_stCnnPara.AI_width_in;
    tSystemModelInsidePrm.inputWid = m_stCnnPara.AI_height_in;
    tSystemModelInsidePrm.outputHei = m_stCnnPara.AI_width_out;
    tSystemModelInsidePrm.outputWid = m_stCnnPara.AI_height_out;

    int32_t ret = AI_XSP_Init(m_stCnnPara.nChannel, &tSystemModelPlatPrm, &tSystemModelInsidePrm);
    XSP_CHECK(0 == ret, XRAY_LIB_AI_XSP_INIT_ERROR, "LibXRay: Failed to initialize the AI_XSP.");
    log_info("AI_XSP Initialize success.");

    return XRAY_LIB_OK;
}

/**
 * @brief 背散射切换自研模型
 * @param sSliceHeight 条带高度
 * @param sSliceWidth 条带宽度
 * @return XRAY_LIB_HRESULT 错误码
 * @note 背散射模式下切换自研模型
 */
XRAY_LIB_HRESULT CnnModule::BackscatterSwitchModel(int32_t sSliceHeight, int32_t sSliceWidth)
{
    XSP_CHECK(sSliceHeight > 0 || sSliceWidth > 0, XRAY_LIB_AI_XSP_INIT_ERROR,
              "AiXsp_SwitchModel ParamIn Error.");

    log_info("Switching backscatter model - slice size: %dx%d", sSliceWidth, sSliceHeight);

    log_info("BackScatter Model doesn't need be changed.");

    #if 0

    // 计算需要输入的条带尺寸
    int32_t nAiXspNB = AiXsp_GetNbLen();
    int32_t nInputSliceHeight = sSliceHeight + 2 * nAiXspNB;
    int32_t nInputSliceWidth = sSliceWidth;

    log_info("Calculated input size for backscatter: %dx%d", nInputSliceWidth, nInputSliceHeight);

    // 匹配合适的模型文件
    if (auto modelname = m_filematcher.find_match(1, nInputSliceWidth, nInputSliceHeight, 3000, "1.0x1.0"))
    {
        // 校验当前模型和前一次模型是否一致，否则不切换模型
        if (strcmp(m_tModelInfo.strModelName, modelname.value().c_str()) != 0)
        {
            strcpy(m_tModelInfo.strModelName, modelname.value().c_str());
            m_tModelInfo.AI_width_in = m_filematcher.selected_values[1];
            m_tModelInfo.AI_width_out = m_filematcher.selected_values[1];
            m_tModelInfo.AI_height_in = m_filematcher.selected_values[2];
            m_tModelInfo.AI_height_out = m_filematcher.selected_values[2];
            log_info("AI_XSP SwitchModel: Switch model to %s", m_tModelInfo.strModelName);
        }
        else
        {
            log_info("AI_XSP SwitchModel: Current model[%s] is already matched", m_tModelInfo.strModelName);
            return XRAY_LIB_OK;
        }
    }
    else
    {
        log_error("Backscatter AiXsp_SwitchModel No model found for input size %dx%d, channels %d ScalingFactor %s",
                  nInputSliceWidth, nInputSliceHeight, m_tModelInfo.AI_channels_out, "1.0x1.0");
        return XRAY_LIB_OK;
    }

    // 更新模型参数
    m_stCnnPara.AI_channels_in = m_tModelInfo.AI_channels_in;
    m_stCnnPara.AI_width_in = m_tModelInfo.AI_width_in;
    m_stCnnPara.AI_height_in = m_tModelInfo.AI_height_in;
    m_stCnnPara.AI_channels_out = m_tModelInfo.AI_channels_out;
    m_stCnnPara.AI_width_out = m_tModelInfo.AI_width_out;
    m_stCnnPara.AI_height_out = m_tModelInfo.AI_height_out;

    // 更新模型文件路径
    strcpy(m_szPathDLModelName, m_tModelInfo.strModelName);
    strcpy(m_szPathDLModelFile, m_szPathDLModelPath);
    strcat(m_szPathDLModelFile, m_szPathDLModelName);

    // 释放当前模型
    XRAY_LIB_HRESULT hr;
    hr = AI_XSP_DeInit(m_stCnnPara.nChannel);
    XSP_CHECK(0 == hr, XRAY_LIB_AI_XSP_RELEASE_ERR, "LibXRay: AIXSP release failed.");
    log_info("AI_XSP Release!");

    // 重新初始化模型
    SYSTEM_MODEL_PLAT_PRM tSystemModelPlatPrm;
    const char *strPath = m_szPathDLModelFile;
    tSystemModelPlatPrm.Input_Pass_Through = 0;
    tSystemModelPlatPrm.Output_Pass_Through = 0;
    tSystemModelPlatPrm.pModelPath = strPath;
    tSystemModelPlatPrm.eSysTemplat = m_stCnnPara.systemPlat;
    tSystemModelPlatPrm.eSysProcessType = m_stCnnPara.systemProcessType;
    tSystemModelPlatPrm.eSysPrecisionType = m_stCnnPara.systemProcessPrecision;
    tSystemModelPlatPrm.sSysInputDataType = TENSOR_FLOAT32;
    tSystemModelPlatPrm.sSysOutputDataType = TENSOR_FLOAT32;
    tSystemModelPlatPrm.eSysCoreMask = m_stCnnPara.rayinSchedulerParam.aiSrNpuCore;

    SYSTEM_MODEL_INSIDE_PRM tSystemModelInsidePrm;
    tSystemModelInsidePrm.inputChnSize = m_stCnnPara.AI_channels_in;
    tSystemModelInsidePrm.outputChnSize = m_tModelInfo.AI_channels_out;
    tSystemModelInsidePrm.inputHei = m_stCnnPara.AI_width_in;
    tSystemModelInsidePrm.inputWid = m_stCnnPara.AI_height_in;
    tSystemModelInsidePrm.outputHei = m_stCnnPara.AI_width_out;
    tSystemModelInsidePrm.outputWid = m_stCnnPara.AI_height_out;

    int32_t ret = AI_XSP_Init(m_stCnnPara.nChannel, &tSystemModelPlatPrm, &tSystemModelInsidePrm);
    XSP_CHECK(0 == ret, XRAY_LIB_AI_XSP_INIT_ERROR, "LibXRay: Failed to initialize the AI_XSP.");
    log_info("AI_XSP Initialize success.");
#endif 

    return XRAY_LIB_OK;
}

/**************************************************************************************************
 *                                      异步处理实现
 ***************************************************************************************************/

/**
 * @brief 初始化异步处理线程池（简化版）
 * @param threadCount 线程数量，默认为2
 * @return XRAY_LIB_HRESULT 错误码
 * @note 初始化异步处理所需的工作线程
 */
XRAY_LIB_HRESULT CnnModule::InitAsyncThread(int32_t threadCount)
{
    std::lock_guard<std::mutex> lock(m_taskQueueMutex);

    // 如果已经初始化，先清理
    if (m_bAsyncRunning)
    {
        log_warn("Async thread pool already initialized. Cleaning up first...");
        CleanupAsyncThreadPool();
    }

    // 设置异步处理运行标志
    m_bAsyncRunning = true;

    // 创建工作线程
    try
    {
        for (int32_t i = 0; i < threadCount; i++)
        {
            m_workerThreads.emplace_back(&CnnModule::AsyncWorkerThread, this);
            log_info("Created async worker thread %d", i + 1);
        }
    }
    catch (const std::exception &e)
    {
        log_error("Failed to create async worker threads: %s", e.what());
        m_bAsyncRunning = false;
        return XRAY_LIB_AI_XSP_INIT_ERROR;
    }

    log_info("Async thread pool initialized with %d threads", threadCount);
    return XRAY_LIB_OK;
}

/**
 * @brief 异步任务工作线程函数
 * @note 工作线程的主循环，处理任务队列中的任务
 */
void CnnModule::AsyncWorkerThread()
{
    log_info("Async worker thread started");

    while (m_bAsyncRunning)
    {
        std::shared_ptr<AIAsyncTask> task;
        {
            std::unique_lock<std::mutex> lock(m_taskQueueMutex);

            // 等待任务或停止信号
            m_taskQueueCV.wait(lock, [this]
                               { return !m_taskQueue.empty() || !m_bAsyncRunning; });

            // 如果停止信号且队列为空，退出线程
            if (!m_bAsyncRunning && m_taskQueue.empty())
            {
                break;
            }

            // 获取任务
            if (!m_taskQueue.empty())
            {
                task = m_taskQueue.front();
                m_taskQueue.pop();
            }
        }

        // 处理任务
        if (task)
        {
            try
            {
                // 执行AI处理
                XRAY_LIB_HRESULT hr = XRAY_LIB_OK;

                if (0 != m_stCnnPara.nUseAI && 0 == m_stCnnPara.plat_mode)
                {
                    if (XRAYLIB_ENERGY_SCATTER == m_pSharedPara->m_enEnergyMode)
                    {
                        hr = BackScatterProcess(task->inputData->low, task->outputData->low);
                    }
                    else
                    {
                        hr = TransMissionProcess(*task->inputData, *task->outputData);
                    }
                }
                else
                {
                    // 如果未启用AI-XSP，直接复制数据
                    hr = CopyDataWithoutAI(*task->inputData, *task->outputData);
                }

                // 设置任务完成状态和结果
                task->completed = true;
                task->resultCode = hr;
                task->finishTime = std::chrono::steady_clock::now();
                task->resultPromise.set_value(hr);

                // 将完成的任务添加到已完成队列
                {
                    std::lock_guard<std::mutex> completedLock(m_completedTasksMutex);
                    m_completedTasks.push(task);
                    m_completedTasksCV.notify_one();
                }
            }
            catch (const std::exception &e)
            {
                log_error("Exception in async task %d: %s", task->taskId, e.what());
                task->completed = true;
                task->resultCode = XRAY_LIB_AI_XSP_ASYN_ERROR;
                task->finishTime = std::chrono::steady_clock::now();
                task->resultPromise.set_exception(std::current_exception());

                // 即使出错也要将任务添加到已完成队列
                {
                    std::lock_guard<std::mutex> completedLock(m_completedTasksMutex);
                    m_completedTasks.push(task);
                    m_completedTasksCV.notify_one();
                }
            }
        }
    }

    log_info("Async worker thread stopped");
}

/**
 * @brief 复制数据（未启用AI时使用）
 * @param inputData 输入数据
 * @param outputData 输出数据
 * @return XRAY_LIB_HRESULT 错误码
 * @note 当未启用AI-XSP时，直接复制数据的辅助函数
 */
XRAY_LIB_HRESULT CnnModule::CopyDataWithoutAI(Calilhz &inputData, Calilhz &outputData)
{
    XRAY_LIB_HRESULT result = XRAY_LIB_OK;

    if (XRAYLIB_ENERGY_LOW == m_pSharedPara->m_enEnergyMode || XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
    {
        result = outputData.low.Reshape(inputData.low.hei, inputData.low.wid, inputData.low.tpad, XSP_32FC1);
        if (XRAY_LIB_OK == result)
        {
            memcpy(outputData.low.Ptr(), inputData.low.Ptr(), inputData.low.hei * inputData.low.wid * sizeof(float32_t));
        }
    }
    if (XRAYLIB_ENERGY_HIGH == m_pSharedPara->m_enEnergyMode || XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
    {
        XRAY_LIB_HRESULT hr = outputData.high.Reshape(inputData.high.hei, inputData.high.wid, inputData.low.tpad, XSP_32FC1);
        if (XRAY_LIB_OK == hr)
        {
            memcpy(outputData.high.Ptr(), inputData.high.Ptr(), inputData.high.hei * inputData.high.wid * sizeof(float32_t));
        }
        else
        {
            result = hr;
        }
    }
    if (XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
    {
        XRAY_LIB_HRESULT hr = outputData.zData.Reshape(inputData.zData.hei, inputData.zData.wid, inputData.low.tpad, XSP_32FC1);
        if (XRAY_LIB_OK == hr)
        {
            memcpy(outputData.zData.Ptr(), inputData.zData.Ptr(), inputData.zData.hei * inputData.zData.wid * sizeof(float32_t));
        }
        else
        {
            result = hr;
        }
    }

    return result;
}

/**
 * @brief 对送入AIXSP的高低能数据进行带偏移的幂函数变换，以提高精度
 * @param [I/O] inputData 输入数据（0-1范围）
 * @param fPower 幂指数（小于1增强低灰度，大于1增强高灰度）
 * @param offset 偏移量，保护边界信息（建议0.05-0.1）
 */
void CnnModule::PowerTransform(Calilhz &inputData, float32_t fPower, float32_t offset)
{
    // 参数校验
    if (fPower <= 0.0f) {
        // 使用安全默认值
        fPower = 0.5f;
    }
    
    // 偏移量限制在合理范围
    offset = std::max(0.0f, std::min(0.3f, offset));
    
    // 计算映射参数
    float32_t range_scale = 1.0f - 2.0f * offset;
    
    for(int32_t hei = 0; hei < inputData.low.hei; hei++)
    {
        float32_t *pLow = inputData.low.Ptr<float32_t>(hei);
        float32_t *pHigh = inputData.high.Ptr<float32_t>(hei);
        
        for(int32_t wid = 0; wid < inputData.low.wid; wid++)
        {
            float32_t oriLow = pLow[wid];
            float32_t oriHigh = pHigh[wid];

            // 带偏移的幂函数变换
            // 1. 将[0,1]映射到[offset, 1-offset]
            float32_t mappedLow = offset + oriLow * range_scale;
            float32_t mappedHigh = offset + oriHigh * range_scale;
            
            // 2. 应用幂变换
            float32_t transLow = std::pow(mappedLow, fPower);
            float32_t transHigh = std::pow(mappedHigh, fPower);
            
            // 3. 将结果映射回[0,1]范围
            float32_t min_output = std::pow(offset, fPower);
            float32_t max_output = std::pow(1.0f - offset, fPower);
            float32_t output_range = max_output - min_output;
            
            // 避免除零
            if (output_range < 1e-6f) {
                output_range = 1e-6f;
            }
            
            pLow[wid] = (transLow - min_output) / output_range;
            pHigh[wid] = (transHigh - min_output) / output_range;

            // clamp到0-1范围
            pLow[wid] = std::max(0.0f, std::min(1.0f, pLow[wid]));
            pHigh[wid] = std::max(0.0f, std::min(1.0f, pHigh[wid]));
        }
    }
}

/**
 * @brief 对AIXSP处理后的数据进行反幂函数变换恢复
 * @param [I/O] inputData 经过幂函数变换的数据
 * @param fPower 幂指数（必须与变换时相同）
 * @param offset 偏移量（必须与变换时相同）
 */
void CnnModule::InversePowerTransform(Calilhz &inputData, float32_t fPower, float32_t offset)
{
    // 参数校验
    if (fPower <= 0.0f) {
        throw std::invalid_argument("Power parameter must be positive");
    }
    
    // 偏移量限制
    offset = std::max(0.0f, std::min(0.3f, offset));
    
    float32_t inversePower = 1.0f / fPower;
    
    // 计算反变换参数
    float32_t min_output = std::pow(offset, fPower);
    float32_t max_output = std::pow(1.0f - offset, fPower);
    float32_t output_range = max_output - min_output;
    float32_t range_scale = 1.0f - 2.0f * offset;

    for(int32_t hei = 0; hei < inputData.low.hei; hei++)
    {
        float32_t *pLow = inputData.low.Ptr<float32_t>(hei);
        float32_t *pHigh = inputData.high.Ptr<float32_t>(hei);
        
        for(int32_t wid = 0; wid < inputData.low.wid; wid++)
        {
            float32_t transLow = pLow[wid];
            float32_t transHigh = pHigh[wid];

            // 反变换步骤：
            // 1. 将输入值映射回变换后的范围
            float32_t scaledLow = transLow * output_range + min_output;
            float32_t scaledHigh = transHigh * output_range + min_output;
            
            // 2. 应用反幂变换
            float32_t mappedLow = std::pow(scaledLow, inversePower);
            float32_t mappedHigh = std::pow(scaledHigh, inversePower);
            
            // 3. 映射回原始[0,1]范围
            pLow[wid] = (mappedLow - offset) / range_scale;
            pHigh[wid] = (mappedHigh - offset) / range_scale;

            // clamp到0-1范围
            pLow[wid] = std::max(0.0f, std::min(1.0f, pLow[wid]));
            pHigh[wid] = std::max(0.0f, std::min(1.0f, pHigh[wid]));
        }
    }
}


/**
 * @brief 清理异步线程池
 * @note 停止所有工作线程并清理资源
 */
void CnnModule::CleanupAsyncThreadPool()
{
    std::lock_guard<std::mutex> lock(m_taskQueueMutex);

    // 设置停止标志
    m_bAsyncRunning = false;

    // 通知所有线程
    m_taskQueueCV.notify_all();

    // 等待所有线程结束
    for (auto &thread : m_workerThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    m_workerThreads.clear();

    // 清理任务队列
    while (!m_taskQueue.empty())
    {
        m_taskQueue.pop();
    }

    log_info("Async thread pool cleaned up");
}

/**
 * @brief 提交AIXSP异步处理任务
 * @param caliDataIn 输入归一化数据
 * @param caliDataOut 输出归一化数据
 * @return XRAY_LIB_HRESULT 任务ID（新版本返回成功状态码）
 * @note 异步提交AIXSP处理任务，使用内存池预分配的内存
 */
XRAY_LIB_HRESULT CnnModule::SubmitAsynTask(Calilhz &caliDataIn, Calilhz &caliDataOut)
{
    XSP_CHECK(m_bAsyncRunning, XRAY_LIB_AI_XSP_ASYN_INIT_ERR, "Async thread pool not initialized");

    // 检查输入数据有效性
    if (!caliDataIn.low.IsValid() || !caliDataIn.high.IsValid() || !caliDataIn.zData.IsValid())
    {
        log_error("Invalid input data");
        return XRAY_LIB_AI_XSP_ASYN_INIT_ERR;
    }

    try
    {
        // 创建异步任务
        auto task = std::make_shared<AIAsyncTask>();
        task->submitTime = std::chrono::steady_clock::now();

        // 生成任务ID
        task->taskId = m_taskId++;

        // 更新内存池大小（如果需要）
        UpdateAsyncMemPoolSize(caliDataIn, caliDataOut);

        // 获取内存池中的内存块
        auto memoryPool = AcquireAsyncMemPool();

        // 完整初始化 inputData，使用内存池中的内存
        task->inputData = new Calilhz;

        // 输入数据 - 使用内存池的内存
        task->inputData->low.data.fl = memoryPool->inputLowMem.data();
        task->inputData->low.SetMem(memoryPool->inputLowSize);
        task->inputData->low.Reshape(caliDataIn.low.hei, caliDataIn.low.wid, caliDataIn.low.tpad, caliDataIn.low.bpad, caliDataIn.low.type);
        memcpy(task->inputData->low.Ptr(), caliDataIn.low.Ptr(), caliDataIn.low.size);

        task->inputData->high.data.fl = memoryPool->inputHighMem.data();
        task->inputData->high.SetMem(memoryPool->inputHighSize);
        task->inputData->high.Reshape(caliDataIn.high.hei, caliDataIn.high.wid, caliDataIn.high.tpad, caliDataIn.high.bpad, caliDataIn.high.type);
        memcpy(task->inputData->high.Ptr(), caliDataIn.high.Ptr(), caliDataIn.high.size);

        task->inputData->zData.data.fl = memoryPool->inputZDataMem.data();
        task->inputData->zData.SetMem(memoryPool->inputZDataSize);
        task->inputData->zData.Reshape(caliDataIn.zData.hei, caliDataIn.zData.wid, caliDataIn.zData.tpad, caliDataIn.zData.bpad, caliDataIn.zData.type);
        memcpy(task->inputData->zData.Ptr(), caliDataIn.zData.Ptr(), caliDataIn.zData.size);

        // 完整初始化 outputData，使用内存池中的内存
        task->outputData = new Calilhz;

        // 输出数据 - 使用内存池的内存
        task->outputData->low.data.fl = memoryPool->outputLowMem.data();
        task->outputData->low.SetMem(memoryPool->outputLowSize);
        task->outputData->low.Reshape(caliDataOut.low.hei, caliDataOut.low.wid, caliDataOut.low.tpad, caliDataOut.low.bpad, caliDataOut.low.type);

        task->outputData->high.data.fl = memoryPool->outputHighMem.data();
        task->outputData->high.SetMem(memoryPool->outputHighSize);
        task->outputData->high.Reshape(caliDataOut.high.hei, caliDataOut.high.wid, caliDataOut.high.tpad, caliDataOut.high.bpad, caliDataOut.high.type);

        task->outputData->zData.data.fl = memoryPool->outputZDataMem.data();
        task->outputData->zData.SetMem(memoryPool->outputZDataSize);
        task->outputData->zData.Reshape(caliDataOut.zData.hei, caliDataOut.zData.wid, caliDataOut.zData.tpad, caliDataOut.zData.bpad, caliDataOut.zData.type);

        // 将内存块附加到任务上，以便在任务完成时释放
        task->memoryPool = memoryPool;

        // 添加到AIXSP异步处理任务队列
        {
            std::lock_guard<std::mutex> queueLock(m_taskQueueMutex);
            m_taskQueue.push(task);
        }

        // 通知工作线程
        m_taskQueueCV.notify_one();

        return XRAY_LIB_OK;
    }
    catch (const std::exception &e)
    {
        log_error("Failed to submit async task: %s", e.what());
        return XRAY_LIB_AI_XSP_ASYN_INIT_ERR;
    }
}

/**
 * @brief 异步AIXSP处理
 * @param caliDataIn 输入归一化数据
 * @param caliDataOut 输出归一化数据
 * @return XRAY_LIB_HRESULT 错误码
 * @note 异步提交AIXSP处理任务，不再返回任务ID
 */
XRAY_LIB_HRESULT CnnModule::AiXsp_AsynProcess(Calilhz &caliDataIn, Calilhz &caliDataOut)
{
    return SubmitAsynTask(caliDataIn, caliDataOut);
}

/**
 * @brief 获取最新异步任务结果
 * @param caliDataOut 输出归一化数据（用于接收处理结果）
 * @param timeoutMs 超时时间（毫秒），-1表示无限等待
 * @return XRAY_LIB_HRESULT 任务结果状态码
 * @note 获取AIXSP最新处理完的结果，不再需要指定taskid
 */
XRAY_LIB_HRESULT CnnModule::AiXsp_GetAsynResult(Calilhz &caliDataOut, int32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(m_completedTasksMutex);

    // 等待已完成任务或超时
    if (timeoutMs == 0)
    {
        // 非阻塞模式
        if (m_completedTasks.empty())
        {
            log_error("No completed tasks available");
            return XRAY_LIB_AI_XSP_ASYN_NOT_FOUND;
        }
    }
    else if (timeoutMs > 0)
    {
        // 带超时的等待
        if (!m_completedTasksCV.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                         [&]
                                         { return !m_completedTasks.empty(); }))
        {
            log_error("No completed tasks available within timeout");
            return XRAY_LIB_AI_XSP_ASYN_TIMEOUT;
        }
    }
    else
    {
        // 无限等待
        m_completedTasksCV.wait(lock, [&]
                                { return !m_completedTasks.empty(); });
    }

    // 获取队列头部的任务
    auto task = m_completedTasks.front();
    m_completedTasks.pop(); // 从队列中移除已获取的任务

    // 确保任务已完成
    if (!task->completed)
    {
        log_error("Task %d is not completed but in completed queue", task->taskId);
        return XRAY_LIB_AI_XSP_ASYN_ERROR;
    }

    // 复制输出数据
    if (caliDataOut.low.IsValid() && task->outputData->low.IsValid())
    {
        memcpy(caliDataOut.low.Ptr<float32_t>(), task->outputData->low.Ptr<float32_t>(),
               task->outputData->low.size);
    }

    if (caliDataOut.high.IsValid() && task->outputData->high.IsValid())
    {
        memcpy(caliDataOut.high.Ptr<float32_t>(), task->outputData->high.Ptr<float32_t>(),
               task->outputData->high.size);
    }

    if (caliDataOut.zData.IsValid() && task->outputData->zData.IsValid())
    {
        memcpy(caliDataOut.zData.Ptr<float32_t>(), task->outputData->zData.Ptr<float32_t>(),
               task->outputData->zData.size);
    }

    log_debug("Retrieved async task %d result: %X", task->taskId, task->resultCode);
    return task->resultCode;
}

/**
 * @brief 获取CNN输入宽度
 * @return int32_t 输入宽度
 */
int32_t CnnModule::AiXsp_GetWidthIn()
{
    return m_stCnnPara.AI_width_in;
}

/**
 * @brief 获取CNN输入高度
 * @return int32_t 输入高度
 */
int32_t CnnModule::AiXsp_GetHeiIn()
{
    return m_stCnnPara.AI_height_in;
}