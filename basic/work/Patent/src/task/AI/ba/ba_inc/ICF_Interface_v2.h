/** @file ICF_Interface_v2.h
 *  @note Hikvision Digital Technology Co., Ltd. All Rights Reserved
 *  @brief 智能引擎接口
 *
 *  @author  刘锦胜
 *  @version 1.2.5
 *  @date    2020/06/12
 *  @note    1、绑核模式可配置线程堆栈大小；
 *  @note    2、线程池模式可配置线程数量；
 *
 *  @author  刘锦胜
 *  @version 1.2.4
 *  @date    2020/06/08
 *  @note    1、修改接口支持单进程多ICF应用同时使用（具体的，init中加入返回值pInitHandle；create加入pInitHandle；finit加入pInitHandle参数）；
 *  @note    2、将用户参数从init中修改到create中，以支持多路应用创建时使用不同参数（具体的，在create中加入pAppParam参数）；
 
 *  @author  曹贺磊
 *  @version 1.0.0
 *  @date    2019/11/14
 *  @note    更改接口
 *
 *  @author  郭俞江
 *  @version 0.9.1
 *  @date    2016/3/7
 *  @note    创建
 */

#ifndef _ICF_INTERFACE_H_
#define _ICF_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ICF_base_v2.h"

#define ICF_MAJOR_VERSION            2
#define ICF_SUB_VERSION              1
#define ICF_REVISION_VERSION         8

#define ICF_VERSION_YEAR             2022
#define ICF_VERSION_MONTH            5
#define ICF_VERSION_DAY              25

/** @fn       void(CALLBACK * ICF_DataCallBack)
 *  @brief    <数据回调>
 *  @param    nNodeID         [out]  - Node序号
 *  @param    nCallBackType   [out]  - 数据类型 ICF_CALLBACK_TYPE
 *  @param    pstOutput       [in]   - 回调数据
 *  @param    nSize           [in]   - 回调数据大小
 *  @param    pUsr            [in]   - 设置回调时用户传入的自定义数据指针
 *  @param    nUserSize       [in]   - 设置回调时用户传入的自定义数据大小
 *  @return   void
 */
typedef void (ICF_CALLBACK *ICF_DataCallBack)(int           nNodeID,
                                              int           nCallBackType,
                                              void         *pstOutput,
                                              unsigned int  nSize,
                                              void         *pUsr,
                                              int           nUserSize);

/**  @fn       int (CALLBACK * ICF_SourceReleaseCB)
  *  @brief    源数据或算子数据释放回调，释放源数据内存或算子数据内存
  *            需要注意，释放源数据或释放算子数据的回调都是这个定义。
  *            该回调每次能拿到一个SOURCE_BLOB的数据，有多少个BLOB或算子就会调用多少次
  *  @param    pMemPool        [in]   - 内存池句柄（可从该句柄中获取InitHandle）
  *  @param    pData           [in]   - 内存指针(ICF_MEDIA_INFO_V2/ICF_SOURCE_BLOB_V2/算子输出内存)
  *  @param    nDataType       [in]   - 数据类型(ICF_ANA_DATA_TYPE_E/自定义算子数据类型)
  *  @return   int
  */
typedef int (ICF_CALLBACK *ICF_SourceReleaseCB)(void        *pMemPool,
                                                void        *pData,
                                                int          nDataType);

/**  @fn       int (CALLBACK * ICF_MemAllocCB)
  *  @brief    <内存释放回调，释放输入内存或中间结果内存；系统内存释放回调接口>
  *  @param    pInitHandle     [in]     - InitHandle,外部注册接口建议不要使用,内置接口用于日志监控等操作
  *  @param    pMemInfo        [in]     - 分配时的内存信息
  *  @param    stMemBuffer     [in]     - 内存Buffer
  *  @return   int
  */
typedef int (ICF_CALLBACK *ICF_MemReleaseCB)(void            *pInitHandle,
                                             ICF_MEM_INFO_V2    *pMemInfo,
                                             ICF_MEM_BUFFER_V2  *stMemBuffer);

/** @fn       int (CALLBACK * ICF_MemAllocCB)
 *  @brief    <系统内存申请回调函数接口>
 *  @param    pInitHandle     [in]     - InitHandle,外部注册接口建议不要使用,内置接口用于日志监控等操作
 *  @param    pMemInfo        [in]     - 内存信息
 *  @param    stMemBuffer     [out]    - 内存Buffer
 *  @return   int
 */
typedef int (ICF_CALLBACK *ICF_MemAllocCB)(void               *pInitHandle,
                                           const ICF_MEM_INFO_V2 *pMemInfo,
                                           ICF_MEM_BUFFER_V2     *stMemBuffer);

/** @fn       int (CALLBACK *ICF_DynamicRouterCB)
 *  @brief    <节点输出数据发送路径的动态路由器,路由无效下按照配置的连接关系进行传输>
 *  @param    pInitHandle        [in]   - InitHandle
 *  @param    pNodeHandle        [in]   - 节点句柄
 *  @param    pPackageHandle     [in]   - 待发送的数据包,等价于1.1.0版本的ICF_ANA_PACKAGE
 *  @return   ICF_SUCCESS:内部认为路由有效,如果返回失败或者返回成功而未做任何操作则认为路由无效
 */
typedef int (ICF_CALLBACK *ICF_DynamicRouterCB)(void *pInitHandle,
                                                void *pNodeHandle,
                                                void *pPackageHandle);

/** @fn     FuncAbnormalAlarm
*  @brief  框架异常报警回调
*  1.功能简介:异常报警回调接口
*  2.使用场景:引擎需要监控框架异常行为
*  3.使用方式:用户自行实现回调接口，调用ICF_SetCallBack接口设置回调函数;
*             打开监控模块的总开关;
*             异常报警监控帧率与监控模式的帧率设置保持一致;
*  @param    pInitHandle     [in]     InitHandle
*  @param    nReturn         [in]     异常情况返回值
*  @param    eAlarmType      [io]     报警级别
*  @param    pUsr            [in]   - 设置回调时用户传入的自定义数据指针
*  @param    nUserSize       [in]   - 设置回调时用户传入的自定义数据大小
*  @return 回调状态码
*/
typedef int (ICF_CALLBACK *ICF_FuncAbnormalAlarm)(void               *pInitHandle,
                                                  int                 nReturn,
                                                  ICF_ABNORMAL_LEVEL  eAlarmType,
                                                  void               *pUsr,
                                                  int                 nUserSize);

/** @fn  int ICF_Init
 *  @brief  <引擎初始化>
 *  @param  pInitInParam      [in]          - 输入参数结构体 ICF_INIT_PARAM_V2
 *  @param  nInitInParamSize  [in]          - 输入参数大小
 *  @param  pInitOutParam     [out]         - 输出参数结构体 ICF_INIT_HANDLE_V2
 *  @param  nInitOutParamSize [in]          - 输出参数大小
 *  @return 在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
 */
#define ICF_Init ICF_Init_V2
int ICF_EXPORT ICF_Init_V2(void                  *pInitParam,
                           unsigned int           nInitParamSize,
                           void                  *pInitOutParam,
                           unsigned int           nInitOutParamSize);

/** @fn  int ICF_Finit
 *  @brief  <引擎反始化>
 *  @param  pFinitInParam     [in]          - 输入参数结构体 ICF_INIT_HANDLE_V2
 *  @param  nFinitInParamSize [in]          - 输入参数大小
 *  @return 在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
 */
#define ICF_Finit ICF_Finit_V2
int ICF_EXPORT ICF_Finit_V2(void                *pFinitInParam,
                            unsigned int         nFinitInParamSize);

/** @fn  int ICF_LoadModel
 *  @brief  <预加载模型>
 *  @param  pLMInParam         [in]          - 输入参数结构体 ICF_MODEL_PARAM_V2
 *  @param  nLMInParamSize     [in]          - 输入参数大小
 *  @param  pLMOutParam        [out]         - 输出参数结构体 ICF_MODEL_HANDLE_V2
 *  @param  nLMOutParamSize    [in]          - 输出参数大小
 *  @return 在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
 */
#define ICF_LoadModel ICF_LoadModel_V2
int ICF_EXPORT ICF_LoadModel_V2(void          *pLMInParam,
                                unsigned int   nLMInParamSize,
                                void          *pLMOutParam,
                                unsigned int   nLMOutParamSize);

/** @fn  int ICF_UnloadModel
 *  @brief  <模型卸载>
 *  @param  pULMInParam      [in]          - 输入参数结构体 ICF_MODEL_HANDLE_V2
 *  @param  nULMInParamSize  [in]          - 输入参数大小
 *  @return 在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
 */
#define ICF_UnloadModel ICF_UnloadModel_V2
int ICF_EXPORT ICF_UnloadModel_V2(void        *pULMInParam,
                                  unsigned int nULMInParamSize);

/** @fn  int ICF_Create
 *  @brief  <引擎创建>
 *  @param  pCreateParam         [in]          - 输入参数结构体 ICF_CREATE_PARAM_V2
 *  @param  nCreateParamSize     [in]          - 输入参数大小
 *  @param  pCreateHandle        [out]         - 输出handle ICF_CREATE_HANDLE_V2
 *  @param  nHandleSize          [in]          - 结构体大小
 *  @return 在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
 */
#define ICF_Create ICF_Create_V2
int ICF_EXPORT ICF_Create_V2(void           *pCreateParam,
                             unsigned int    nCreateParamSize,
                             void           *pCreateHandle,
                             unsigned int    nHandleSize);

/** @fn  int ICF_Destory 
 *  @brief  <引擎句柄销毁>
 *  @param  pCreateHandle      [in]   - ICF_CREATE_HANDLE，代表应用中某一路
 *  @param  nHandleSize        [in]   - 句柄结构体大小
 *  @return 在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
*/
#define ICF_Destroy ICF_Destroy_V2
int ICF_EXPORT ICF_Destroy_V2(void         *pCreateHandle,
                              unsigned int  nHandleSize);


/** @fn  int ICF_InputData
 *  @brief  <引擎分析数据输入>
 *  @param  pChannelHandle     [in]   - ICF_CREATE_HANDLE_V2.pChannelHandle，代表应用中某一路
 *  @param  pInputData         [in]   - 分析数据
 *  @param  nDataSize          [in]   - 分析数据大小
 *  @return 在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
*/
#define ICF_InputData ICF_InputData_V2
int ICF_EXPORT ICF_InputData_V2(void            *pChannelHandle,
                                void            *pInputData,
                                unsigned int     nDataSize);

/** @fn  int ICF_SetConfig
 *  @brief  <引擎配置>
 *  @param  pChannelHandle   [in]          - ICF_CREATE_HANDLE_V2.pChannelHandle
 *  @param  pConfigParam     [in]          - 引擎配置参数结构体 ICF_CONFIG_PARAM_V2
 *  @param  nConfigParamSize [in]          - 参数大小
 *  @return 在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
 */
#define ICF_SetConfig ICF_SetConfig_V2
int ICF_EXPORT ICF_SetConfig_V2(void          *pChannelHandle,
                                void          *pConfigParam,
                                unsigned int   nConfigParamSize);

/** @fn  int ICF_GetConfig
 *  @brief  <获取引擎配置>
 *  @param  pChannelHandle   [in]          - ICF_CREATE_HANDLE_V2.pChannelHandle
 *  @param  pConfigParam     [in/out]      - 引擎配置参数结构体 ICF_CONFIG_PARAM_V2
 *  @param  nConfigParamSize [out]         - 参数大小
 *  @return 在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
 */
#define ICF_GetConfig ICF_GetConfig_V2
int ICF_EXPORT ICF_GetConfig_V2(void          *pChannelHandle,
                                void          *pConfigParam,
                                unsigned int   nConfigParamSize);

/** @fn  int ICF_SetCallback
 *  @brief  <获取引擎配置>
 *  @param  pChannelHandle  [in]          - ICF_CREATE_HANDLE_V2.pChannelHandle
 *  @param  pCBParam        [in]          - 回调参数结构体 ICF_CALLBACK_PARAM_V2
 *  @param  nCBParamSize    [in]          - 参数大小
 *  @return 在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
 */
#define ICF_SetCallback ICF_SetCallback_V2
int ICF_EXPORT ICF_SetCallback_V2(void         *pChannelHandle,
                                  void         *pCBParam,
                                  unsigned int  nCBParamSize);

/** @fn  int ICF_SubFunction
 *  @brief  <引擎分析数据输入 同步接口, 目前仅限于最简单串行数据流，不支持分支和特殊节点，请谨慎使用>
 *  @param  pChannelHandle     [in]   - 创建句柄，ICF_CREATE_HANDLE.pChannelHandle
 *  @param  pInputData         [in]   - 输入数据
 *  @param  nInputDataSize     [in]   - 输入数据大小
 *  @param  pOutputData        [out]  - 输出数据
 *  @param  pOutputDataSize    [out]  - 输出数据大小
 *  @return 在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
*/
#define ICF_SubFunction ICF_SubFunction_V2
int ICF_EXPORT ICF_SubFunction_V2(void            *pChannelHandle,
                                  void            *pInputData,
                                  unsigned int     nInputDataSize,
                                  void           **pOutputData,
                                  unsigned int    *pOutputDataSize);

/** @fn  int ICF_System_SetConfig
 *  @brief  <引擎配置ICF框架参数>
 *  @param pInitHandle           -I    ICF句柄 ICF_INIT_HANDLE_V2.pInitHandle
 *  @param nKey                  -I    参数key ICF_KEY_CONFIG
 *  @param pParam                -I    参数
 *  @param nSize                 -I    参数大小
 *  @return 在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
 */
#define ICF_System_SetConfig ICF_System_SetConfig_V2
int ICF_EXPORT ICF_System_SetConfig_V2(void *pInitHandle,
                                       int   nKey,
                                       void *pParam,
                                       int   nSize);

/** @fn  int ICF_System_GetConfig
 *  @brief  <引擎获取ICF框架参数>
 *  @param pInitHandle           -I    ICF句柄 ICF_INIT_HANDLE_V2.pInitHandle
 *  @param nKey                  -I    参数key ICF_KEY_CONFIG
 *  @param pParam                -O    参数
 *  @param nSize                 -I    参数大小
 *  @return 在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
 */
#define ICF_System_GetConfig ICF_System_GetConfig_V2
int ICF_EXPORT ICF_System_GetConfig_V2(void *pInitHandle,
                                       int   nKey,
                                       void *pParam,
                                       int   nSize);

/**@fn       ICF_GetVersion
 * @brief    引擎版本获取接口
 * @param1   pEngineVersionParam  [out]   - 引擎版本结构体
 * @param2   nSize                [in]    - 结构体大小
 * @return   在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
 */
#define ICF_GetVersion ICF_GetVersion_V2
int ICF_EXPORT ICF_GetVersion_V2(void *pEngineVersionParam,
                                 int   nSize);

/**@fn       ICF_APP_GetVersion
 * @brief    应用版本获取接口
 * @param1   pVersionParam        [out]   - 封装层版本结构体
 * @param2   nSize                [in]    - 结构体大小
 * @return   在入参为空时确保返回NULL错误码，入参中的参数为空则可能由于参数不存在或则非法而返回参数错误或则NULL错误
 */
#define ICF_APP_GetVersion ICF_APP_GetVersion_V2
int ICF_APP_GetVersion_V2(void *pVersionParam,
                          int   nSize);

#ifdef __cplusplus
}
#endif

#endif /* _ICF_INTERFACE_H_ */
