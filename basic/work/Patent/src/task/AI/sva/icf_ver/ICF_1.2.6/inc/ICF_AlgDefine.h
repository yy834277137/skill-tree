/** @file       ICF_AlgDefine.h
 *  @note       HangZhou Hikvision Digital Technology Co., Ltd. 
                All right reserved
 *  @brief      算子层对封装层接口及数据结构，纯C特性
 *  
 *  @author     曹贺磊
 *  @date       2019/12/24
 *  @note       增加并行节点相关定义
 *
 *
 *  @author     吴海威
 *  @date       2019/7/24
 *  @note       修改Process输入输出及Handle结构体定义
 *
 *  @author     tianmin
 *  @date       2019/7/11
 *  @note
 *
 */

#ifndef _ICF_ALGDEFINE_H_
#define _ICF_ALGDEFINE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ICF_base.h"
#include "ICF_AlgTool.h"
#include "ICF_toolkit.h"
#include "ICF_MemPool.h"

// 全局常量定义
#define ICF_ALGP_MODEL_NUM         (64)  // 最大模型个数(对应算子配置文件中"model_paths"数组最多只能配置64个模型路径)
#define ICF_ALGP_USERDATA_NUM      (128) // 最大用户数据长度(对应算子配置文件中"user_params"数组最多只能配置128个int值)
#define ICF_ALGP_NAME_LEN          (64)  // 接口名字长度(对应算子配置文件中"pluginfunc_create_model","pluginfunc_destroy_model"等封装层回调接口名字最长包含64个字符)
#define ICF_ALGP_PATH_LEN          (256) // 路径长度(对应算子配置文件中"model_paths"模型路径和"plugindll_path"动态库路径长度)
#define ICF_MAX_ASS_NODES          (8)   // 拼装NODE最多包含算子个数(对应流水配置文件中，如果使用串行节点，"AlgType"数组最多配置8个算子类型，如[1,2,3,4,5,6,7,8])


// 模型句柄信息
typedef struct _ICF_MODEL_HANDLE_INFO_
{
    void         *pInitHandle;                                        // ICF_Init创建的句柄,框架内部初始化
    void         *pModelHandle;                                       // 模型句柄
    ICF_MEM_TAB   stModelMemTab[ICF_MEM_TYPE_NUM];                    // 模型内存信息，暂时不用 
}ICF_MODEL_HANDLE_INFO;

// 算法句柄信息
typedef struct _ICF_ALG_HANDLE_INFO_
{
    void         *pInitHandle;                                        // ICF_Init创建的句柄,框架内部初始化
    void         *pAlgHandle;                                         // 算法句柄
    int          *pUserData;                                          // 指向ICF_MODEL_CREATE_PARAM中的nUserData
    int           nAlgType;                                           // 本算子的类型，由算子配置文件配置
    ICF_MEM_TAB   stAlgMemTab[ICF_MEM_TYPE_NUM];                      // stAlgMemTab[0]提供给封装层传递输出结果内存表到Node
}ICF_ALG_HANDLE_INFO;


// 模型创建参数
typedef struct _ICF_MODEL_CREATE_PARAM_
{
    int            nModelNums;                                         // 模型数，由算子配置文件配置计算生成
    void          *pModelBuffer[ICF_ALGP_MODEL_NUM];                   // 模型文件内容
    long long      nModelBufferSize[ICF_ALGP_MODEL_NUM];               // 模型内容长度
    int            nModelType;                                         // 模型类型，当前版本等于nAlgType，由算子配置文件配置
    int            nDlProcType[ICF_ALGP_MODEL_NUM];                    // 处理类型，当前版本未使用
    int            nUserData[ICF_ALGP_USERDATA_NUM];                   // 用户配置用于控制封装算子的参数
    void          *pSchedule;                                          // 封装层透传调度器，由ICF_Init接口输入
    ICF_APP_PARAM_INFO stAppInfo;                                      // APP输入信息
}ICF_MODEL_CREATE_PARAM; 
 
// 算法创建参数
typedef struct _ICF_ALG_CREATE_PARAM_ 
{ 
    void         *pModelHdl;                                          // 模型句柄 ICF_MODEL_HANDLE_INFO中的pModelHandle
    int           nImgWidth;                                          // 宽，由算子配置文件配置
    int           nImgHeight;                                         // 高，由算子配置文件配置
    int           nAlgType;                                           // 本算子的类型，由算子配置文件配置
    int           nNodeID;                                            // NODE编号，0:此时Node配置为shared,不绑定
    ICF_MODEL_CREATE_PARAM *pModelParam;                              // 指向创建模型时的输入参数ICF_MODEL_CREATE_PARAM
}ICF_ALG_CREATE_PARAM;

// 事件处理驱动器信息
typedef struct _ICF_EVENT_DRIVER_
{
    int           nDriverNums;                                        // 有效事件接收者个数
    int           nNodeID[ICF_EVENT_NUMMAX];// 重要提示                // 句柄对应的事件接收者的Node编号,目前实际存放是algType
    void         *pHandle[ICF_EVENT_NUMMAX];                          // 接收者Alg句柄
}ICF_EVENT_DRIVER;

// 并行节点参数
typedef struct _ICF_PARANODE_PARAM_
{
    int                  nProcessUnit;                                // 处理单元个数
    ICF_ALG_HANDLE_INFO* pAlgHdlInfo[ICF_MAX_ASS_NODES];              // 对应的句柄信息
    int                  reserved[16];                                // 保留位
}ICF_PARANODE_PARAM;

// 并行节点处理进度
typedef struct _ICF_PARANODE_PROGRESS
{
    int           nSplitTotal;                                         // 总共拆包个数
    int           nSplitIndex;                                         // 对应拆包队列中的索引, -1代表无效（不拆包模式）
    int           nProcessDone;                                        // 已处理完的个数
    int           nProcessUnit;                                        // 处理子节点单元索引
    long long     nTimeConsume;                                        // 从处理开始计时的时间戳,单位：us
    int           nReserved[8];                                        // 保留字段
}ICF_PARANODE_PROGRESS;

// 并性节点信息
typedef struct _ICF_PARA_PARA_RUNTIME_
{
    ICF_PARANODE_PROGRESS stParaNodeProg;                               // 并行节点使用 并行节点处理进度
    ICF_PARANODE_PARAM    stParaNodParam;                               // 并行节点使用 包含了各个句柄信息
}ICF_PARA_PARA_RUNTIME;

// 算子运行时动态信息
typedef struct ICF_ALG_RUNTIME_
{
    void             *pNodeHandle;                                     // 通用参数 当前节点的句柄,工具接口中使用
    char             *pszNodeName;                                     // 通用参数 当前节点的名字
    int               nChannelID;                                      // 通用参数 当前数据包的通道ID,0:无效
    int               nCurNodeID;                                      // 通用参数 调用该算子的节点ID,0:无效
    int               nQueueNum;                                       // 通用参数 调用该算子的节点输入队列数
    int               nQueueLens[ICF_NODE_PORTMAX];                    // 通用参数 调用该算子的节点输入队列数据包数
    int               nQueueLensMax[ICF_NODE_PORTMAX];                 // 通用参数 调用该算子的节点输入队列最大长度

    ICF_EVENT_DRIVER      stEventDriver;                               // 事件驱动使用 事件驱动器
    ICF_PARA_PARA_RUNTIME stParaNodeRuntime;                           // 并行节点运行时参数
}ICF_ALG_RUNTIME;

// 并行节点策略
typedef struct _ICF_PARANODE_STRA_
{
    int               nSplit;                                           // 拆包个数
    int               nSync;                                            // 是否同步
    int               reserved[8];                                      // 保留字段
}ICF_PARANODE_STRA;

/* @fn     FunEventProcess
*  @brief  
*  1.功能简介:跨算子实时事件控制，FunEventProcess接口为算子控制逻辑(需用户自定义)
*  2.使用场景:根据本算子process结果实时调用其它算子相关处理逻辑
*  3.使用方式:在流水配置文件中配置控制关系，调用ICF_EventDriver接口触发实时控制，框架会调用对应算子的FunEventProcess接口
*             用户自行实现时间处理接口，算子配置文件中"pluginfunc_eventprocess"字段指定用户定义函数名以及"plugindll_path"
* 字段指定动态库，框架自行注册
*  @param  pAlgHdlInfo      [in]  算法句柄
*  @param  pstMsgPackage    [io]  输入输出事件
*  @return 封装层定义错误码
*/
typedef int(*FunEventProcess)(ICF_ALG_HANDLE_INFO *pAlgHdlInfo, 
                              void                *pstMsgInfo);

/* @fn     FunCreateModel
*  @brief  
*  1.功能简介:算子创建模型
*  2.使用场景:算子初始化时
*  3.使用方式:用户自行实现创建模型接口，算子配置文件中"pluginfunc_create_model"字段指定用户定义函数名以及"plugindll_path"
* 字段指定动态库，框架自行注册
*  @param  pstModelParam    [in]  模型句柄创建参数
*  @param  pstModelHdlInfo  [out] 模型句柄结构体
*  @return 封装层定义错误码
*/
typedef int(*FunCreateModel)(ICF_MODEL_CREATE_PARAM *pstModelParam, 
                             ICF_MODEL_HANDLE_INFO  *pstModelHdlInfo);

/* @fn     FunDestroyModel
*  @brief  销毁模型句柄，销毁创建时获取的资源
*  1.功能简介:算子销毁模型
*  2.使用场景:算子卸载时
*  3.使用方式:用户自行实现销毁模型接口，算子配置文件中"pluginfunc_destroy_model"字段指定用户定义函数名以及"plugindll_path"
* 字段指定动态库，框架自行注册
*  @param  pstModelHdlInfo  [in]  模型句柄结构体
*  @return 封装层定义错误码
*/
typedef int(*FunDestroyModel)(ICF_MODEL_HANDLE_INFO *pstModelHdlInfo);

/* @fn     FunCreate
*  @brief  创建算法句柄
*  1.功能简介:算子创建
*  2.使用场景:算子创建好模型后创建运行时资源
*  3.使用方式:用户自行实现创建接口，算子配置文件中"pluginfunc_create"字段指定用户定义函数名以及"plugindll_path"
* 字段指定动态库，框架自行注册
*  @param  pstAlgParam      [in]  算法句柄创建参数结构体
*  @param  pstAlgHdlInfo    [out] 算法句柄结构体
*  @return 封装层定义错误码
*/
typedef int(*FunCreate)(ICF_ALG_CREATE_PARAM *pstAlgParam, 
                        ICF_ALG_HANDLE_INFO  *pstAlgHdlInfo);

/* @fn     FunDestroy
*  @brief  销毁句柄
*  1.功能简介:算子销毁
*  2.使用场景:算子销毁运行时资源
*  3.使用方式:用户自行实现销毁接口，算子配置文件中"pluginfunc_destroy"字段指定用户定义函数名以及"plugindll_path"
* 字段指定动态库，框架自行注册
*  @param  pstAlgHdlInfo    [in]  算法句柄结构体
*  @return 封装层定义错误码
*/
typedef int(*FunDestroy)(ICF_ALG_HANDLE_INFO *pstAlgHdlInfo);

/* @fn     FunProcess
*  @brief  算子处理/数据源生成接口，由引擎调度/独立线程
*  1.功能简介:算子运行
*  2.使用场景:算子实际运行处理逻辑
*  3.使用方式:用户自行实现算子运行接口，算子配置文件中"pluginfunc_process"字段指定用户定义函数名以及"plugindll_path"
* 字段指定动态库，框架自行注册
*  @param  pAlgHdlInfo      [in]  算法句柄
*  @param  pstAlgRuntime    [in]  算子运行时信息
*  @param  pstPackageHdl    [io]  算子输入输出数据包
*  @param  nPackageBatchs   [in]  batch 数目
*  @return 封装层定义错误码
*/
typedef int(*FunProcess)(ICF_ALG_HANDLE_INFO *pAlgHdlInfo,
                         ICF_ALG_RUNTIME     *pstAlgRuntime,
                         void                *pstPackageHdl[],
                         int                  nPackageBatchs);


/* @fn     FunSetConfig
*  @brief  设置参数
*  1.功能简介:算子配置
*  2.使用场景:算子参数配置
*  3.使用方式:用户自行实现参数配置接口，算子配置文件中"pluginfunc_set_config"字段指定用户定义函数名以及"plugindll_path"
* 字段指定动态库，框架自行注册
*  @param  pstAlgHdlInfo    [in]  算法句柄
*  @param  nKey             [in]  配置选项
*  @param  pConfigData      [in]  配置数据
*  @param  nConfSize        [in]  配置结构体大小
*  @return 封装层定义错误码
*/
typedef int(*FunSetConfig)(ICF_ALG_HANDLE_INFO *pstAlgHdlInfo, 
                           int                  nKey, 
                           void                *pConfigData, 
                           unsigned int         nConfSize);

/* @fn     FunGetConfig
*  @brief  获取参数
*  1.功能简介:获取算子配置
*  2.使用场景:获取算子参数配置
*  3.使用方式:用户自行实现获取配置参数接口，算子配置文件中"pluginfunc_get_config"字段指定用户定义函数名以及"plugindll_path"
* 字段指定动态库，框架自行注册
*  @param  pstAlgHdlInfo    [in]  算法句柄
*  @param  nKey             [in]  配置选项
*  @param  pConfigData      [out] 配置数据
*  @param  nConfSize        [in]  配置结构体大小
*  @return 封装层定义错误码
*/
typedef int(*FunGetConfig)(ICF_ALG_HANDLE_INFO *pstAlgHdlInfo, 
                           int                  nKey, 
                           void                *pConfigData, 
                           unsigned int         nConfSize);

/* @fn     FuncParaSendStra
*  @brief  并行节点数据分发策略
*  1.功能简介:并行节点数据分发
*  2.使用场景:并行节点处理时一个包中包含多个需要拆分的处理目标
*  3.使用方式:用户自行实现数据分发接口，流水配置中配置并行节点"StrategyFunc"相应字段以及"NodePluginPath"字段指定动态库，框架自行注册
*  @param  pstAlgRuntime    [in]  运行时信息
*  @param  pstPackageHdl    [io]  算子输入输出数据包
*  @param  nPackageBatchs   [in]  batch 数目
*  @param  pParaNodeStra    [io]  并行节点策略
*  @return 封装层定义错误码
*/
typedef int(*FuncParaSendStra)(ICF_ALG_RUNTIME     *pstAlgRuntime,
                               void                *pstPackageHdl[],
                               int                  nPackageBatchs,
                               ICF_PARANODE_STRA   *pParaNodeStra);

/* @fn     FuncParaSync
*  @brief  并行节点同步策略
*  1.功能简介:并行节点同步回调接口
*  2.使用场景:并行节点实际处理结束后需要同步的场景(如使用并行节点同时处理4张图做检测，需等待4张图全部结束再进入下一个模块)
*  3.使用方式:用户自行实现数据分发接口，流水配置中配置并行节点"Parallel_Sync"相应字段以及"NodePluginPath"字段指定动态库，框架自行注册
*  @param  pstAlgRuntime    [in]  运行时信息
*  @param  pstPackageHdl    [io]  算子输入输出数据包
*  @param  nPackageBatchs   [in]  batch 数目
*  @return 封装层定义错误码
*/
typedef int(*FuncParaSync)(ICF_ALG_RUNTIME     *pstAlgRuntime,
                           void                *pstPackageHdl[],
                           int                  nPackageBatchs);


#ifdef __cplusplus
}
#endif

#endif /* _ICF_ALGDEFINE_H_ */

