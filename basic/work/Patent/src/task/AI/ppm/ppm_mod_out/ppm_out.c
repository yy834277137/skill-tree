/**
 * @file:   ppm_out.c
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  人包关联应用交互层源文件
 * @author: sunzelin
 * @date    2020/12/28
 * @note:
 * @note \n History:
   1.日    期: 2020/12/28
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include "ppm_out.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/
VOID ATTRIBUTE_WEAK Ppm_DrvSetMetaCtlFlag(UINT32 flag)
{
	return;
}

VOID ATTRIBUTE_WEAK ppm_debug_dump_depth(VOID)
{
	return;
}

VOID ATTRIBUTE_WEAK Ppm_DrvParsePos(VOID)
{
	return;
}

VOID ATTRIBUTE_WEAK Ppm_DrvSetDepthSyncGap(UINT32 gap)
{
	return;
}

/**
 * @function   Ppm_DrvGenerateDepthInfo
 * @brief      从json数据中格式化生成深度信息
 * @param[in]  VOID *pJsonData        
 * @param[in]  UINT32 u32DataLen  
 * @param[out] None
 * @return     INT32
 */
INT32 ATTRIBUTE_WEAK Ppm_DrvGenerateDepthInfo(VOID *pJsonData, UINT32 u32DataLen)
{
	return SAL_SOK;
}

/**
 * @function:   Ppm_DrvPrintDbgPrm
 * @brief:
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
VOID ATTRIBUTE_WEAK Ppm_DrvPrintDbgPrm(VOID)
{
    return;
}

/**
 * @function:   set_sync_gap
 * @brief:
 * @param[in]:  BOOL bChgFlag, UINT64 in_gap
 * @param[out]: None
 * @return:     INT32
 */
VOID ATTRIBUTE_WEAK set_sync_gap(BOOL bChgFlag, UINT64 in_gap)
{
    return;
}

/**
 * @function    Ppm_DrvSetSyncGapCnt
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID ATTRIBUTE_WEAK Ppm_DrvSetSyncGapCnt(UINT32 cnt)
{
    return;
}

/**
 * @function    Ppm_DrvSetPosInfoFlag
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID ATTRIBUTE_WEAK Ppm_DrvSetPosInfoFlag(UINT32 flag)
{
    return;
}

/**
 * @function    Ppm_DrvSetPackSensity
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID ATTRIBUTE_WEAK Ppm_DrvSetPackSensity(UINT32 sensity)
{
    return;
}

/**
 * @function:   Ppm_DrvSetDbgSwitch
 * @brief:
 * @param[in]:  UINT32 flag
 * @param[out]: None
 * @return:     INT32
 */
VOID ATTRIBUTE_WEAK Ppm_DrvSetDbgSwitch(UINT32 flag)
{
    return;
}

/**
 * @function:   Ppm_DrvGetPosInfo
 * @brief:      获取pos信息
 * @param[in]:  UINT32 chan
 * @param[in]:  PPM_POS_INFO_S *pstPosInfo
 * @param[out]: None
 * @return:     INT32
 */
INT32 ATTRIBUTE_WEAK Ppm_DrvGetPosInfo(UINT32 chan, PPM_POS_INFO_S *pstPosInfo)
{
    return SAL_SOK;
}

/**
 * @function:   Ppm_TskSetVprocHandle
 * @brief:      设置vproc通道句柄
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 ATTRIBUTE_WEAK Ppm_TskSetVprocHandle(UINT32 chan, UINT32 u32OutChn, VOID *pDupChnHandle)
{
    return SAL_SOK;
}

/**
 * @function:   Ppm_TskInit
 * @brief:      任务层初始化接口
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 ATTRIBUTE_WEAK Ppm_TskInit(VOID)
{
    return SAL_SOK;
}

/**
 * @function:   CmdProc_ppmCmdProc
 * @brief:      人包关联交互命令处理函数
 * @param[in]:  HOST_CMD cmd
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 ATTRIBUTE_WEAK CmdProc_ppmCmdProc(HOST_CMD cmd, UINT32 chan, void *prm)
{
    return SAL_SOK;
}

