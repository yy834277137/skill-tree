/** @file ICF_Interface.h
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
 *  @note    更该接口
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

#include "ICF_base.h"

#define ICF_MAJOR_VERSION            1
#define ICF_SUB_VERSION              2
#define ICF_REVISION_VERSION         6

#define ICF_VERSION_YEAR             2020
#define ICF_VERSION_MONTH            8
#define ICF_VERSION_DAY              4

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

/**  @fn       int (CALLBACK * ICF_MemAllocCB)
  *  @brief    <内存释放回调，释放输入内存或中间结果内存；系统内存释放回调接口>
  *  @param    pData           [in]   - 内存指针
  *  @param    nDataType       [in]   - 内存类型
  *  @return   int
  */
typedef int (ICF_CALLBACK *ICF_MemReleaseCB)(void        *pInitHandle,
                                             void        *pData,
                                             int          nDataType);

 /** @fn       int (CALLBACK * ICF_MemAllocCB)
  *  @brief    <系统内存申请回调函数接口>
  *  @param    lBufSize        [in]   - 内存大小
  *  @param    nMemSpace       [in]   - 内存类型
  *  @param    stMemBuffer     [out]  - 返回的包含虚拟地址和物理地址的内存buffer
  *  @return   int
  */
typedef int (ICF_CALLBACK *ICF_MemAllocCB)(void            *pInitHandle,
                                           long long        lBufSize,
                                           int              nMemSpace,
                                           ICF_MEM_BUFFER  *stMemBuffer);

/** @fn       int (CALLBACK *ICF_DynamicRouterCB)
 *  @brief    <节点输出数据发送路径的动态路由器,路由无效下按照配置的连接关系进行传输>
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

/** @fn  int ICF_GetRuntimeLimit 
 *  @brief  <引擎运行需要的最低限制>
 *  @param  pInitParam         [in]   - 初始化参数
 *  @param  LimitParam         [out]  - 参数限制
 *  @return 错误码
*/
int ICF_GetRuntimeLimit(const ICF_LIMIT_INPUT *pLimitInput,
                        ICF_LIMIT_OUTPUT      *pLimitOutput);

/** @fn  int ICF_Init 
 *  @brief  <引擎初始化>
 *  @param  pInitParam         [in]   - 初始化参数
 *  @param  nSize              [in]   - 参数大小
 *  @param  pInitHandle        [out]  - init返回handle，代表应用(如HMS、FD等)
 *  @return 错误码
*/
int ICF_Init(void                  *pInitParam,
             int                    nInitParamSize,
             void                 **pInitHandle);

/** @fn  int ICF_Create 
 *  @brief  <引擎句柄创建>
 *  @param  pInitHandle        [in]   - init返回handle，代表应用(如HMS、FD等)
 *  @param  nGraphType         [in]   - 任务编号Task.json中的GraphType
 *  @param  nGraphId           [in]   - 图序号
 *  @param  pAppParam          [in]   - 用户参数信息，用于该路算子创建时参数传入
 *  @param  pCreateHandle      [out]  - 创建句柄，create返回handle，代表应用中某一路
 *  @return 错误码
*/
int ICF_Create(void                *pInitHandle,
               int                  nGraphType,
               int                  nGraphId,
               ICF_APP_PARAM_INFO  *pAppParam,
               void               **pCreateHandle);

/** @fn  int ICF_Destory 
 *  @brief  <引擎句柄销毁>
 *  @param  pCreateHandle      [in]   - 创建句柄，create返回handle，代表应用中某一路
 *  @return 错误码
*/
int ICF_Destroy(void               *pCreateHandle);


/** @fn  int ICF_Input_data 
 *  @brief  <引擎分析数据输入>
 *  @param  pCreateHandle      [in]   - 创建句柄，create返回handle，代表应用中某一路
 *  @param  pInputData         [in]   - 分析数据
 *  @param  nDataSize          [in]   - 分析数据大小
 *  @return 错误码
*/
int ICF_Input_data(void            *pCreateHandle,
                   void            *pInputData,
                   unsigned int     nDataSize);


/** @fn  int ICF_SubFunction 
 *  @brief  <引擎分析数据输入 同步接口> (暂时不开放)
 *  @param  pCreateHandle      [in]   - 创建句柄，create返回handle，代表应用中某一路
 *  @param  nNodeID            [in]   - 节点NodeID
 *  @param  pInputData         [in]   - 输入数据
 *  @param  nDataSize          [in]   - 数据大小
 *  @return 错误码
*/
int ICF_SubFunction(void            *pCreateHandle,
                    int              nNodeID,
                    void            *pInputData,
                    unsigned int     nDataSize);


/** @fn  int ICF_Set_config 
 *  @brief  <引擎设置任务参数>
 *  @param  pCreateHandle      [in]   - 创建句柄，create返回handle，代表应用中某一路
 *  @param  nNodeID            [in]   - NodeID
 *  @param  nKey               [in]   - 任务参数索引
 *  @param  pConfigData        [in]   - 配置数据
 *  @param  nConfSize          [in]   - 数据大小
 *  @return 错误码
*/
int ICF_Set_config(void          *pCreateHandle,
                   int            nNodeID,
                   int            nKey,
                   void          *pConfigData,
                   unsigned int   nConfSize);

/** @fn  int ICF_Get_config 
 *  @brief  <引擎获取任务参数>
 *  @param  pCreateHandle      [in]   - 创建句柄，create返回handle，代表应用中某一路
 *  @param  nNodeID            [in]   - NodeID
 *  @param  nKey               [in]   - 任务参数索引
 *  @param  pConfigData        [out]  - 配置数据
 *  @param  nConfSize          [out]  - 数据大小
 *  @return 错误码
*/
int ICF_Get_config(void          *pCreateHandle,
                   int            nNodeID,
                   int            nKey,
                   void          *pConfigData,
                   unsigned int   nConfSize);

/** @fn  int ICF_Set_callback 
 *  @brief  <引擎设置回调函数>
 *  @param  pCreateHandle     [in]   - 创建句柄，create返回handle，代表应用中某一路
 *  @param  nNodeID           [in]   - 节点对应索引
 *  @param  callback_type     [in]   - 回调类型
 *  @param  callback_func     [in]   - 回调函数
 *  @param  pUsr              [in]   - 用户数据指针
 *  @param  nUserSize         [in]   - 用户数据大小
 *  @return 错误码
*/
int ICF_Set_callback(void       *pCreateHandle,
                     int         nNodeID,
                     int         nCallbackType,
                     void       *pCallbackFunc,
                     void       *pUsr,
                     int         nUserSize);

/**@fn       ICF_GetVersion
 * @brief    引擎版本获取接口
 * @param1   pEngineVersionParam  [out]   - 引擎版本结构体
 * @param2   nSize                [in]    - 结构体大小
 * @return   错误码
 * @note
 */
int ICF_GetVersion(void *pEngineVersionParam, 
                   int   nSize);

/**@fn       ICF_APP_GetVersion
 * @brief    应用版本获取接口
 * @param1   pVersionParam        [out]   - 封装层版本结构体
 * @param2   nSize                [in]    - 结构体大小
 * @return   错误码
 * @note
 */
int ICF_APP_GetVersion(void *pVersionParam, 
                       int   nSize);

/** @fn  int ICF_Finit 
 *  @brief  <引擎反初始化>
 *  @param  pInitHandle           [in]  - ICF_Init返回handle，代表应用(如HMS、FD等)
 *  @return 错误码
*/
int ICF_Finit(void *pInitHandle);

#ifdef __cplusplus
}
#endif

#endif /* _ICF_INTERFACE_H_ */
