/*******************************************************************************
* sva_out.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2020年6月8日 Create
*
* Description :
* Modification:
*******************************************************************************/
/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sva_out.h"

#ifndef SVA_NO_USE

/**
 * @function:   Sva_HalSetCfgPrm
 * @brief:      设置安检智能配置参数
 * @param[in]:  cJSON *pstItem
 * @param[out]: None
 * @return:     INT32
 */
INT32 ATTRIBUTE_WEAK Sva_HalSetCfgPrm(cJSON *pstItem)
{
    return SAL_SOK;
}

#endif

void ATTRIBUTE_WEAK debug_dump_set_flag(int flag)
{
	return;
}

/**
 * @function   Sva_DrvSetDualPkgMatchThresh
 * @brief      设置包裹匹配阈值
 * @param[in]  float fMatchThresh  
 * @param[out] None
 * @return     INT32
 */
INT32 ATTRIBUTE_WEAK Sva_DrvSetDualPkgMatchThresh(float fMatchThresh)
{
	return SAL_SOK;
}

/**
 * @function   Sva_DrvPrintDpmDebugSts
 * @brief      打印包裹匹配模块内部调试信息
 * @param[in]  None
 * @param[out] None
 * @return     INT32
 */
INT32 ATTRIBUTE_WEAK Sva_DrvPrintDpmDebugSts(VOID)
{
	return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalUpDateSvaResult
* 描  述  : 更新算法结果
* 输  入  : - pstSrc: 拷贝源变量
*         : - pstDst: 拷贝目的变量
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 ATTRIBUTE_WEAK Sva_HalCpySvaResult(SVA_PROCESS_OUT *pstSrc, SVA_PROCESS_OUT *pstDst)
{
    return SAL_SOK;
}

ATTRIBUTE_WEAK VOID Sva_DrvGetQueDbgInfo(void)
{
	return ;
}

/*******************************************************************************
* 函数名  : Sva_HalClearSvaOut
* 描  述  : 清空智能结果信息
* 输  入  : - chan : 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK INT32 Sva_HalClearSvaOut(UINT32 chan)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalJpegDrawAiInfo
* 描  述  : 将智能信息画在yuv上
* 输  入  : - chan        : 通道号
*         : - pstJpegFrame: jpeg帧数据
*         : - pstOut      : 智能结果信息
*         : - uiWith      : 宽度
*         : - uiHeight    : 高度
*         : - uiModule    : 模块
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK INT32 Sva_HalJpegDrawAiInfo(UINT32 chan,
                                           SYSTEM_FRAME_INFO *pstJpegFrame,
                                           SVA_PROCESS_IN *pstIn,
                                           SVA_PROCESS_OUT *pstOut,
                                           UINT32 uiWith,
                                           UINT32 uiHeight,
                                           UINT32 uiModule)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalGetIcfParaJson
* 描  述  : 获取相应的配置文件路径
* 输  入  : - cfg_num: 文件
* 输  出  : 无
* 返回值  : 对应的json文件路径
*******************************************************************************/
ATTRIBUTE_WEAK CHAR *Sva_HalGetIcfParaJson(SVA_CFG_JSONFILE_E cfg_num)
{
    return NULL;
}

/**
 * @function:   Sva_DrvPrClassMap
 * @brief:      打印大类映射表
 * @param[in]:  None
 * @param[out]: None
 * @return:     INT32
 */
ATTRIBUTE_WEAK VOID Sva_DrvPrClassMap(VOID)
{
	return;
}

/*******************************************************************************
* 函数名  : Sva_HalSetAlgDbgFlag
* 描  述  : 设置算法调试开关
* 输  入  : UINT32 : uiFlag
* 输  出  : 无
* 返回值  : 成功: SAL_SOK
            失败: SAL_FAIL
*******************************************************************************/
ATTRIBUTE_WEAK void Sva_HalSetAlgDbgFlag(UINT32 uiFlag)
{
    return;
}

/**
 * @function    Sva_HalprintfAlgDbgNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
ATTRIBUTE_WEAK void Sva_HalprintfAlgDbgNum(UINT32 uiFlag)
{
    return;
}

/*******************************************************************************
* 函数名  : Sva_HalSetAlgDbgLevel
* 描  述  : 设置算法调试等级
* 输  入  : UINT32 : uiDbgLevel
* 输  出  : 无
* 返回值  : 成功: SAL_SOK
            失败: SAL_FAIL
*******************************************************************************/
ATTRIBUTE_WEAK void Sva_HalSetAlgDbgLevel(UINT32 uiDbgLevel)
{
    return;
}

/*******************************************************************************
* 函数名:   Sva_HalGetFromPoolWithPts
* 描  述   :   根据PTS从缓存池获取智能结果信息
* 输  入   :   - chan     : 通道号
*          :   - pRidx: 读索引指针
*          :   - u64Pts: 用于匹配的PTS
*          :   - pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*                      SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK INT32 Sva_HalGetFromPoolWithPts(UINT32 chan, UINT32 *pRidx, UINT64 u64Pts, SVA_PROCESS_OUT *pstSvaOut)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalGetViSrcRate
* 描  述  : 获取VI的输入帧率
* 输  入  : 无
* 输  出  : 无
* 返回值  : vi帧率
*******************************************************************************/
ATTRIBUTE_WEAK INT32 Sva_HalGetViSrcRate(VOID)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalSetDbgCnt
* 描  述  : 设置框滞留时间(算法内部使用)
* 输  入  : - level: 日志级别
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
ATTRIBUTE_WEAK VOID Sva_HalSetDbgCnt(UINT32 dbgCnt)
{
    return;
}

/**
 * @function:   Sva_DrvSetJpegPosSwitch
 * @brief:      设置Jpeg图片增加Pos信息开关
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
ATTRIBUTE_WEAK INT32 Sva_DrvSetJpegPosSwitch(UINT32 chan, void *prm)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetSvaOut
* 描  述  : 获取当前智能检测结果
* 输  入  : - chan: 通道号
*         : - *pOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK INT32 Sva_DrvGetSvaOut(UINT32 chan, SVA_PROCESS_OUT *pOut)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetCfgParam
* 描  述  : 获取外部配置的智能模块参数
* 输  入  : - chan: 通道号
*         : - *pOut: 外部配置的智能参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK INT32 Sva_DrvGetCfgParam(UINT32 chan, SVA_PROCESS_IN *pOut)
{
    return SAL_SOK;
}

ATTRIBUTE_WEAK VOID Sva_HalSetDumpYuvCnt(UINT32 chan, UINT32 cnt, UINT32 mode)
{
    return;
}

ATTRIBUTE_WEAK VOID Sva_DrvSetPrOutInfoFlag(UINT32 u32Flag)
{
	return;
}

/**
 * @function    Sva_HalSetDumpYuvFlag
 * @brief       设置yuv的dump开关
 * @param[in]
 * @param[out]
 * @return
 */
ATTRIBUTE_WEAK VOID Sva_HalSetDumpYuvFlag(BOOL bFlag)
{
	return;
}

/*******************************************************************************
* 函数名  : Sva_drvGetResult
* 描  述  : 获取智能处理结果
* 输  入  : - chan    通道号
* 输  出  : - pResult 智能处理结果
* 返回值  : 成功:SAL_SOK
*         : 失败:SAL_FAIL
*******************************************************************************/
ATTRIBUTE_WEAK INT32 Sva_drvGetResult(UINT32 chan, void *pResult, UINT32 *pFlag, UINT32 *pAiTimeRef)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetDbLevel
* 描  述  : 设置日志打印级别
* 输  入  : - level: 日志级别
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
ATTRIBUTE_WEAK void Sva_DrvSetDbLevel(UINT32 level)
{
    return;
}

ATTRIBUTE_WEAK VOID Sva_DrvSetSplitDbgSwitch(UINT32 u32Flag)
{
	return;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetDCtestLevel
* 描  述  : 设置双芯片调试开关
* 输  入  : uiFlag         : 调试总开关
*         : uiYuvFlag      : 导出yuv调试开关
*         : uiInputFrmRate : 数据输入帧率
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
ATTRIBUTE_WEAK void Sva_DrvSetDCtestLevel(UINT32 uiFlag, UINT32 uiYuvFlag, UINT32 uiInputFrmRate)
{
    return;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetDCtestLevel
* 描  述  : 获取双芯片调试开关
* 输  入  : idx : 开关索引值
* 输  出  : 无
* 返回值  : 索引值对应的开关状态
*******************************************************************************/
ATTRIBUTE_WEAK UINT32 Sva_DrvGetDCtestLevel(UINT32 idx)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvDbgSaveJpg
* 描  述  : 设置保存图片信息开关
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
ATTRIBUTE_WEAK void Sva_DrvDbgSaveJpg(UINT32 chan)
{
    return;
}

/*******************************************************************************
* 函数名  : Sva_DrvDbgSaveJpg
* 描  述  : 设置保存图片信息开关
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
ATTRIBUTE_WEAK void Sva_DrvChgPosWriteFlag(UINT32 uiFlag)
{
    return;
}

/**
 * @function:   __attribute__
 * @brief:      通道抓图个数计数
 * @param[in]:  (weak)
 * @param[out]: None
 * @return:
 */
ATTRIBUTE_WEAK void Sva_DrvAlgDbgNum(UINT32 chan)
{
    return;
}

/**
 * @function    Sva_DrvTransSetPrtFlag
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
ATTRIBUTE_WEAK VOID Sva_DrvTransSetPrtFlag(BOOL bFlag)
{
    return;
}

/**
 * @function:   Sva_DrvClearXpackRslt
 * @brief:      外部模块清空全局参数
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     INT32
 */
ATTRIBUTE_WEAK INT32 Sva_DrvClearXpackRslt(UINT32 chan)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetSvaOffsetBufTab
* 描  述  : 获取通道全局变量
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : 成功: 返回偏移量的缓存表指针
*           失败: 返回NULL
*******************************************************************************/
ATTRIBUTE_WEAK SVA_OFFSET_BUF_TAB_S *Sva_DrvGetSvaOffsetBufTab(UINT32 chan)
{
    return NULL;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetSvaFromPool
* 描  述  : 获取当前智能检测结果
* 输  入  : - chan: 通道号
*                : - uiTimeRef: 搜索时用于匹配的帧号
*                : - *pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK INT32 Sva_DrvGetSvaFromPool(UINT32 chan, UINT32 uiTimeRef, SVA_PROCESS_OUT *pstSvaOut)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetSyncMainChan
* 描  述  : 获取同步模式主视角通道(默认1为主视角，0为辅视角)
* 输  入  : 无
* 输  出  : 无
* 返回值  : 通道号
*******************************************************************************/
ATTRIBUTE_WEAK UINT32 Sva_DrvGetSyncMainChan()
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetAlgMode
* 描  述  : 
* 输  入  : 无
* 输  出  : 无
* 返回值  : 通道号
*******************************************************************************/
ATTRIBUTE_WEAK UINT32 Sva_DrvGetAlgMode()
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetInputGapNum
* 描  述  : 获取帧处理间隔
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK UINT32 Sva_DrvGetInputGapNum(void)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_tskSetMirrorChnPrm
* 描  述  : 设置镜像通道参数
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK INT32 Sva_tskSetMirrorChnPrm(UINT32 chan, void *prm)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_tskSetScaleChnPrm
* 描  述  : 设置缩放通道参数
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK INT32 Sva_tskSetScaleChnPrm(UINT32 chan, void *prm)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_tskDevInit
* 描  述  : 模块设备层初始化
* 输  入  : pDupHandle : 数据分发句柄
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK INT32 Sva_tskDevInit(UINT32 chan)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_tskInit
* 描  述  : 模块tsk层初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK INT32 Sva_tskInit(void)
{
    return SAL_SOK;
}

ATTRIBUTE_WEAK INT32 Sva_DrvDelPackResFromEngine(UINT32 chan)
{
    return SAL_SOK;
}

ATTRIBUTE_WEAK INT32 Sva_DrvGetPackResFromEngine(UINT32 chan, SVA_PROCESS_OUT_DATA *pstSvaProcessDataToXpack)
{
    return SAL_SOK;
}

ATTRIBUTE_WEAK INT32 Sva_DrvDelDisResFromEngine(UINT32 chan)
{
    return SAL_SOK;
}

ATTRIBUTE_WEAK INT32 Sva_DrvSendAiDataToEngine(UINT32 chan, SVA_PACKAGE_DATE_IN *pstSvaAiData)
{
    return SAL_SOK;
}

ATTRIBUTE_WEAK SYSTEM_FRAME_INFO *Sva_DrvGetSvaNodeFrame(UINT32 chan)
{
    return SAL_SOK;
}

ATTRIBUTE_WEAK INT32 Sva_DrvGetRunState(UINT32 chan,UINT32 *pChnStatus, UINT32 *pUiSwitch)
{
    return SAL_SOK;
}

#if 1

/*******************************************************************************
* 函数名  : CmdProc_svaCmdProc
* 描  述  : 应用层命令字交互接口
* 输  入  : cmd : 数据分发句柄
*         : chan: 通道号
*         : prm : 参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK INT32 CmdProc_svaCmdProc(HOST_CMD cmd, UINT32 chan, void *prm)
{
    return SAL_SOK;
}

#endif

