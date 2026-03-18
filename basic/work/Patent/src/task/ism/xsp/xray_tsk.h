/**
 * @file   xray_tsk.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  X ray解析和预处理任务头文件
 * @author liwenbin
 * @date   2019/10/19
 * @note :
 * @note \n History:
   1.Date        : 2019/10/19
     Author      : liwenbin
     Modification: Created file
 */

#ifndef _XRAY_TSK_H_
#define _XRAY_TSK_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */



/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */



/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */



/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/**
 * @function:   Host_XrayInputStart
 * @brief:      启动X ray数据输入，本地预览通过本接口。
 * @param[in]:  UINT32 chan     通道号
 * @param[in]:  UINT32* pParam  命令参数
 * @param[in]:  UINT32* pBuf    命令参数缓冲
 * @param[out]: None
 * @return:     int             成功SAL_SOK，失败SAL_FALSE
 */
int Host_XrayInputStart(UINT32 chan, UINT32 *pParam, UINT32 *pBuf);

/**
 * @function:   Host_XrayInputStop
 * @brief:      停止X ray数据输入，本地预览通过本接口。
 * @param[in]:  UINT32 chan     通道号
 * @param[in]:  UINT32* pParam  命令参数
 * @param[in]:  UINT32* pBuf    命令参数缓冲
 * @param[out]: None
 * @return:     int             成功SAL_SOK，失败SAL_FALSE
 */
int Host_XrayInputStop(UINT32 chan, UINT32 *pParam, UINT32 *pBuf);

/**
 * @function:   Host_XrayPlaybackStart
 * @brief:      启动X ray数据输入，回拉通过本接口。
 * @param[in]:  UINT32 chan     通道号
 * @param[in]:  UINT32* pParam  命令参数
 * @param[in]:  UINT32* pBuf    命令参数缓冲
 * @param[out]: None
 * @return:     int             成功SAL_SOK，失败SAL_FALSE
 */
int Host_XrayPlaybackStart(UINT32 chan, UINT32 *pParam, UINT32 *pBuf);

/**
 * @function:   Host_XrayPlaybackStop
 * @brief:      停止X ray数据输入，回拉通过本接口。
 * @param[in]:  UINT32 chan     通道号
 * @param[in]:  UINT32* pParam  命令参数
 * @param[in]:  UINT32* pBuf    命令参数缓冲
 * @param[out]: None
 * @return:     int             成功SAL_SOK，失败SAL_FALSE
 */
int Host_XrayPlaybackStop(UINT32 chan, UINT32 *pParam, UINT32 *pBuf);

/**
 * @fn      Host_XrayTransform
 * @brief   转存归一化RAW数据图片
 * @param   chan[IN] 通道号，暂无效
 * @param   pParam[IN] 命令参数
 * @param   pBuf[IN/OUT] 命令参数Buffer，结构体
 * @return  SAL_STATUS SAL_SOK-成功，SAL_FAIL-失败
 */
SAL_STATUS Host_XrayTransform(UINT32 chan, UINT32 *pParam, VOID *pBuf);

/**
 * @function:   Host_XraySetParam
 * @brief:      设置X ray处理参数
 * @param[in]:  UINT32 chan     通道号
 * @param[in]:  UINT32* pParam  命令参数
 * @param[in]:  UINT32* pBuf    命令参数缓冲
 * @param[out]: None
 * @return:     int             成功SAL_SOK，失败SAL_FALSE
 */
int Host_XraySetParam(UINT32 chan, UINT32 *pParam, UINT32 *pBuf);

/**
 * @fn      Host_XrayRawStoreEn
 * @brief   保存源RAW功能的开启和关闭
 * 
 * @param   chan[IN] 通道号
 * @param   pParam[IN] 命令参数，保存源RAW是否开启
 * @param   pBuf[IN] 不使用，无效
 * 
 * @return  SAL_STATUS 
 */
SAL_STATUS Host_XrayRawStoreEn(UINT32 chan, UINT32 *pParam, UINT32 *pBuf);

/**
 * @fn      Host_XrayChangeSpeed
 * @brief   切换传送带速度
 * @param   chan[IN] 通道号，该接口对此参数不作要求，不会使用该参数
 * @param   pParam[IN] 命令参数，该接口对此参数不作要求，不会使用该参数
 * @param   pBuf[IN] 命令参数Buffer
 * @return  SAL_STATUS SAL_SOK-设置成功，SAL_FAIL-失败
 */
SAL_STATUS Host_XrayChangeSpeed(UINT32 chan, UINT32 *pParam, VOID *pBuf);

/**
 * @fn      Host_XrayGetSliceNumAfterCls
 * @brief   获取清屏后当前屏幕已显示的条带数 
 *  
 * @param   chan[IN] 通道号
 * @param   pParam[OUT] 命令参数，清屏后当前屏幕已显示的条带数
 * @param   pBuf[IN] 命令参数Buffer，该接口对此参数不作要求，不会使用该参数 
 *  
 * @return  SAL_STATUS SAL_SOK-设置成功，SAL_FAIL-失败
 */
SAL_STATUS Host_XrayGetSliceNumAfterCls(UINT32 chan, UINT32 *pParam, VOID *pBuf);

SAL_STATUS Host_XrayGetTransBufferSize(UINT32 chan, UINT32 *pParam, VOID *pBuf);

/**
 * @function:   xray_tsk_init
 * @brief:      X RAY处理模块初始化
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32 成功SAL_SOK，失败SAL_FALSE
 */
SAL_STATUS xray_tsk_init(void);

/**
 * @function:   HOST_XrawFullOrZeroData
 * @brief:      X RAY运行状态
 * @param[in]:  UINT32 chan  通道号
 * @param[out]: None
 * @return:     INT32        成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS HOST_XrawFullOrZeroData(UINT32 chan, UINT32 *pParam, VOID *pBuf);

#endif /* end of #ifndef _XRAY_TSK_H_*/

