/**
 * @file xsp_mod_cnn.hpp
 * @brief CNN模块头文件 - 实现AI超分辨率(AI_XSP)和降噪(AI_DN)功能
 *
 * 该模块支持透射和背散射两种模式，提供同步和异步处理能力。
 * 主要功能包括：
 * - 模型文件管理和匹配
 * - AI模型初始化和加载
 * - 数据预处理和后处理
 * - 模型动态切换
 * - 异步任务处理
 *
 */

#ifndef _XSP_MOD_CNN_HPP_
#define _XSP_MOD_CNN_HPP_

#include "xsp_interface.hpp"
#include "xray_shared_para.hpp"
#include "utils/ini.h"

// 系统头文件
#include <filesystem>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <queue>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <iomanip>

/**************************************************************************************************
 *                                      常量定义
 ***************************************************************************************************/

/** @brief AIXSP默认模型的标准差 */
#define AI_XSP_MODEL_STD_DEV 1200

/** @brief AIXSP_SR模型所需要的最小邻边行数（超分辨率模型） */
#define AI_XSP_SR_MIN_NB 4

/** @brief AIXSP_DN模型所需要的最小邻边行数（降噪模型） */
#define AI_XSP_DN_MIN_NB 8

/** @brief AIXSP_DN模型所需要的最小邻边行数（降噪模型） */
#define AI_XSP_BS_MIN_NB 6

/**************************************************************************************************
 *                                      AI模型参数定义
 ***************************************************************************************************/

/**
 * @brief XRAY模型参数结构体
 */
typedef struct _XRAY_MODEL_PARA_T_
{
    int32_t AI_width_in;                  // AI-XSP输入图像宽度（探测器方向像素数）
    int32_t AI_height_in;                 // AI-XSP输入图像高度（采集列数）
    int32_t AI_width_out;                 // AI-XSP输出图像宽度
    int32_t AI_height_out;                // AI-XSP输出图像高度
    int32_t AI_channels_in;               // AI-XSP输入通道数
    int32_t AI_channels_out;              // AI-XSP输出通道数
    char strModelName[XRAY_LIB_MAX_PATH]; // 模型文件名
} XRAY_MODEL_PARA_T;

/**
 * @brief 内存池相关结构
 */
struct AsyncTaskMemoryPool
{
    // 预分配的内存块
    std::vector<float32_t> inputLowMem;
    std::vector<float32_t> inputHighMem;
    std::vector<float32_t> inputZDataMem;
    std::vector<float32_t> outputLowMem;
    std::vector<float32_t> outputHighMem;
    std::vector<float32_t> outputZDataMem;

    // 内存块大小
    size_t inputLowSize = 0;
    size_t inputHighSize = 0;
    size_t inputZDataSize = 0;
    size_t outputLowSize = 0;
    size_t outputHighSize = 0;
    size_t outputZDataSize = 0;

    // 标记内存是否在使用中
    bool inUse = false;
};

/**
 * @brief AI异步任务结构体
 */
struct AIAsyncTask
{
    int32_t taskId;                                   // 任务ID
    Calilhz *inputData;                               // 输入数据指针
    Calilhz *outputData;                              // 输出数据指针
    std::promise<XRAY_LIB_HRESULT> resultPromise;     // 结果承诺
    std::chrono::steady_clock::time_point submitTime; // 任务提交时间
    bool completed = false;                           // 任务是否完成
    XRAY_LIB_HRESULT resultCode = XRAY_LIB_OK;        // 任务结果代码
    std::chrono::steady_clock::time_point finishTime; // 任务完成时间
    std::shared_ptr<AsyncTaskMemoryPool> memoryPool;  // 内存池引用

    // 析构函数，释放申请的内存
    ~AIAsyncTask()
    {
        if (inputData)
        {
            // 注意：使用内存池时不需要释放data.fl内存
            // 内存池会自动管理内存的释放
            delete inputData;
            inputData = nullptr;
        }
        if (outputData)
        {
            // 注意：使用内存池时不需要释放data.fl内存
            // 内存池会自动管理内存的释放
            delete outputData;
            outputData = nullptr;
        }
        // memoryPool是shared_ptr，会自动释放
    }
};

/**************************************************************************************************
 *                                      模型文件匹配器定义
 ***************************************************************************************************/

/**
 * @brief AIXSP模型文件参数结构体
 *
 * 存储从模型文件名中解析出的参数信息
 */
struct AiXspFileParams
{
    int32_t AiXspChn;         // 模型的通道数
    std::string AiXspSf;      // 模型的缩放系数（格式：高度x宽度，如"2.0x2.0"）
    int32_t AiXspWid;         // 模型的输入宽度
    int32_t AiXspHei;         // 模型的输入高度
    int32_t AiXspNoise;       // 模型的噪声参数
    std::string AiXspModName; // 模型文件名
};

/**
 * @brief AIXSP模型文件匹配器类
 *
 * 负责扫描、解析和匹配AI模型文件，支持多种参数的精确匹配
 */
struct FileMatcher
{
public:
    /**
     * @brief AIXSP模型匹配器初始化
     * @param dir_path 模型文件目录路径
     * @param suffix 模型文件后缀名 (默认为"rknn")
     * @note 扫描指定目录下的模型文件，建立索引结构
     */
    XRAY_LIB_HRESULT init(const std::filesystem::path &dir_path, const std::string &suffix = "rknn");

    /**
     * @brief 获取指定索引的模型文件参数
     * @param idx 索引值
     * @return AiXspFileParams 模型文件参数
     * @throw std::out_of_range 当索引越界时抛出异常
     */
    AiXspFileParams get_idx_files(int32_t idx);

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
    std::optional<std::string> find_match(int32_t target_c, int32_t target_w, int32_t target_h,
                                          int32_t target_n, const std::string &target_scale);

    /**
     * @brief 获取选择的模型文件参数值
     */
    std::vector<int32_t> selected_values; // 匹配过程中选择的参数值（通道、宽度、高度、噪声）

    /**
     * @brief 获取选择的缩放系数
     */
    std::string selected_scale_factor; // 匹配过程中选择的缩放系数

private:
    /**
     * @brief 加载模型文件
     * @param dir_path 模型文件目录路径
     * @param suffix 模型文件后缀名
     * @note 使用正则表达式解析模型文件名，提取参数信息
     */
    XRAY_LIB_HRESULT load_files(const std::filesystem::path &dir_path, const std::string &suffix);

    /**
     * @brief 构建模型索引
     * @note 建立多级索引结构，加速模型查找
     */
    void build_index();

    /**
     * @brief 模型文件参数列表
     */
    std::vector<AiXspFileParams> files; // 所有加载的模型文件参数

    /**
     * @brief 模型通道候选集
     */
    std::set<int32_t> channel_candidates; // 所有可用的通道数

    /**
     * @brief 模型索引结构
     * 格式: channel -> scale_factor -> width -> height -> noise -> AiXspFileParams
     */
    std::map<int32_t, std::map<std::string, std::map<int32_t, std::map<int32_t, std::map<size_t, AiXspFileParams>>>>> channel_index; // 多级索引结构
};

/**************************************************************************************************
 *                                      CNN模块类定义
 ***************************************************************************************************/

/**
 * @brief CNN模块类，继承自BaseModule
 *
 * 该类实现了AI超分辨率(AI_XSP)和降噪(AI_DN)功能，支持透射和背散射两种模式。
 * 主要功能包括：
 * - 模型初始化和加载
 * - 数据预处理和后处理
 * - 模型切换
 * - 内存管理
 * - 异步任务处理
 */
class CnnModule : public BaseModule
{
public:
    // 构造和析构函数
    CnnModule();
    virtual ~CnnModule() = default;

    // 初始化和资源管理
    XRAY_LIB_HRESULT Init(const char *szPublicFileFolderName, XRAYLIB_AI_PARAM stXRaylib_AIPara, SharedPara *pPara);
    virtual XRAY_LIB_HRESULT Release();
    virtual XRAY_LIB_HRESULT GetMemSize(XRAY_LIB_MEM_TAB &MemTab, XRAY_LIB_ABILITY &ability);

    // 主要处理接口
    XRAY_LIB_HRESULT AiXsp_PreProcess(XMat &matInLow, XMat &matInHigh, Calilhz &caliAIXspIn, Calilhz &caliAIXspOut);
    XRAY_LIB_HRESULT AiXsp_SyncProcess(Calilhz &caliDataIn, Calilhz &caliDataOut);
    XRAY_LIB_HRESULT AiXsp_AsynProcess(Calilhz &caliDataIn, Calilhz &caliDataOut);
    XRAY_LIB_HRESULT AiXsp_GetAsynResult(Calilhz &caliDataOut, int32_t timeoutMs = -1);
    XRAY_LIB_HRESULT AiXsp_SwitchModel(int32_t DspHeight, int32_t DspWidth, int32_t OriHeight, int32_t OriWidth);

    // 状态查询
    int32_t AiXsp_GetWidthIn();
    int32_t AiXsp_GetHeiIn();
    int32_t AiXsp_GetNbLen();
    std::pair<float32_t, float32_t> AiXsp_GetScale();

    // 公共成员变量
    float32_t m_fGrayAiRatio; // 背散用比例系数

private:
    // 初始化相关
    XRAY_LIB_HRESULT InitModelSelector(const char *szPublicFileFolderName, XRAYLIB_AI_PARAM stXRaylib_AIPara);
    XRAY_LIB_HRESULT InitModelParam();

    // 模型切换
    XRAY_LIB_HRESULT TransmissionSwitchModel(int32_t DspHeight, int32_t DspWidth,
                                             int32_t OriHeight, int32_t OriWidth);
    XRAY_LIB_HRESULT BackscatterSwitchModel(int32_t sSliceHeight, int32_t sSliceWidth);

    // 核心处理函数
    XRAY_LIB_HRESULT TransMissionPreProcess(XMat &matInLow, XMat &matInHigh, Calilhz &caliAIXspIn, Calilhz &caliAIXspOut);
    XRAY_LIB_HRESULT TransMissionProcess(Calilhz &caliDataIn, Calilhz &caliDataOut);
    XRAY_LIB_HRESULT BackScatterProcess(XMat &matIn, XMat &matOut);

    // 工具函数
    XRAY_LIB_HRESULT ConvertChwToHwc(int32_t nChannel, XMat &src, XMat &des);
    XRAY_LIB_HRESULT CopyDataWithoutAI(Calilhz &inputData, Calilhz &outputData);
    void PowerTransform(Calilhz &inputData, float32_t fPower = 0.5f, float32_t offset = 0.1f);
    void InversePowerTransform(Calilhz &inputData, float32_t fPower = 0.5f, float32_t offset = 0.1f);

    // 异步处理
    XRAY_LIB_HRESULT InitAsyncThread(int32_t threadCount = 2);
    XRAY_LIB_HRESULT SubmitAsynTask(Calilhz &caliDataIn, Calilhz &caliDataOut);
    void AsyncWorkerThread();
    void CleanupAsyncThreadPool();

    // 异步处理内存池管理
    void InitAsyncTaskMemPool();
    void ReleaseMemoryPool();
    std::shared_ptr<AsyncTaskMemoryPool> AcquireAsyncMemPool();
    void UpdateAsyncMemPoolSize(const Calilhz& caliDataIn, const Calilhz& caliDataOut);

    /**************************************************************************
     *                               成员变量
     ***************************************************************************/

    XRAYLIB_AI_PARAM m_stCnnPara;   // 当前网络载入模型的参数信息
    SharedPara *m_pSharedPara;      // 全局共享参数
    XRAY_MODEL_PARA_T m_tModelInfo; // 模型信息存储
    FileMatcher m_filematcher;      // 模型文件匹配器

    /**************************************************************************
     *                               文件名及文件路径
     ***************************************************************************/

    char m_szPathDLModelName[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME]; // AI-XSP模型名称
    char m_szPathDLModelFile[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME]; // AI-XSP模型文件路径
    char m_szPathDLModelPath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME]; // AI-XSP模型文件夹路径

    /**************************************************************************
     *                               临时内存
     ***************************************************************************/

    XMat m_matOPENVINOReceiveTemp; // OPENVINO BS使用内存
    XMat m_matAIXSPReceive;        // 透射机型输入AIXSP的内存
    XMat m_matTempHwc;             // 临时HWC格式内存（RK平台专用）
    XMat m_matAiInChn0;            // AI通道0的输入内存
    XMat m_matAiInChn1;            // AI通道1的输入内存
    XMat m_matAiInChn2;            // AI通道2的输入内存
    XMat m_matAiOutChn0;           // AI通道0的输出内存
    XMat m_matAiOutChn1;           // AI通道1的输出内存
    XMat m_matAiOutChn2;           // AI通道2的输出内存

    /**************************************************************************
     *                               异步处理成员变量
     ***************************************************************************/

    std::atomic<bool> m_bAsyncRunning;                    // 异步处理是否运行中
    std::vector<std::thread> m_workerThreads;             // 工作线程池
    std::queue<std::shared_ptr<AIAsyncTask>> m_taskQueue; // 任务队列
    std::mutex m_taskQueueMutex;                          // 任务队列互斥锁
    std::condition_variable m_taskQueueCV;                // 任务队列条件变量

    std::queue<std::shared_ptr<AIAsyncTask>> m_completedTasks; // 已完成任务队列
    std::mutex m_completedTasksMutex;                          // 已完成任务队列互斥锁
    std::condition_variable m_completedTasksCV;                // 已完成任务队列条件变量
    int32_t m_taskId;                                          // 任务ID

    std::vector<AsyncTaskMemoryPool> m_memoryPool; // 异步任务内存池
    std::mutex m_memoryPoolMutex; // 异步任务内存池互斥锁
    const size_t m_poolSize = 4; // 内存池大小
};

#endif // _XSP_MOD_CNN_HPP_