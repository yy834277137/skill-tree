/**
 * @file   system_common_api.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  DSP 模块初始化功能模块头文件
 * @author dsp
 * @date   2022/5/7
 * @note
 * @note \n History
   1.日    期: 2022/5/7
     作    者: dsp
     修改历史: 创建文件
 */

#ifndef _SYSTEM_COMMON_API_H_
#define _SYSTEM_COMMON_API_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include "sal.h"

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/**
 * @function   System_GetBoardType
 * @brief      system 根据CPLD信息获取主板型号

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_GetBoardType();

/**
 * @function   System_commonAudioInit
 * @brief      system 通用模块 音频的初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonAudioInit();

/**
 * @function   System_commonAudioDeInit
 * @brief      system 通用模块 音频的去初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonAudioDeInit();

/**
 * @function   System_commonVdecInit
 * @brief      system 通用模块解码,转封装的初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonVdecInit();

/**
 * @function   System_commonVdecDeInit
 * @brief      system 通用模块解码,转封装的去初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonVdecDeInit();

/**
 * @function   System_commonIaVprocInit
 * @brief      视频处理模块的初始化，这里主要用在分析仪-
               里做镜像功能，目前主要用在包裹抓图

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonIaVprocInit();

/**
 * @function   System_commonSlaveVideoInit
 * @brief      system 通用模块 从片视频的初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonSlaveVideoInit();

/**
 * @function   System_commonVideoInit
 * @brief      system 通用模块 视频的初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonVideoInit();

/**
 * @function   System_commonSysDeInit
 * @brief      system 通用模块 系统全局的去初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonSysDeInit();

/**
 * @function   System_commonSysInit
 * @brief      system 通用模块 系统全局的初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonSysInit();

/**
 * @function   System_commonDeInit
 * @brief      system 通用模块的去初始化

 * @param[out] None
 * @return     INT32
 */
INT32 System_commonDeInit();

/**
 * @function   System_commonInit
 * @brief      system 通用模块的初始化

 * @param[out] None
 * @return     INT32
 */
INT32 System_commonInit();

/*******************************************************************************
* 函数名  : System_videoInputInit
* 描  述  :输入模块的初始化
* 输  入  :
*         :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 System_videoInputInit();

/**
 * @function   System_SysInit
 * @brief      system sys prm的初始化

 * @param[out] None
 * @return     INT32
 */
INT32 System_SysInit();

/**
 * @function   System_XrayInit
 * @brief      system 成像的初始化

 * @param[out] None
 * @return     INT32
 */
INT32 System_XrayInit();

#endif


