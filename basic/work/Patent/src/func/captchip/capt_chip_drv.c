/*******************************************************************************
* capt_hal_drv.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : liuxianying <liuxianying@hikvision.com.cn>
* Version: V1.0.0  2017年09月08日 Create
*
* Description : 硬件平台采集模块
* Modification:
*******************************************************************************/
#include <sal.h>
#include <dspcommon.h>
#include <platform_hal.h>
#include "capbility.h"
#include "./inc/mstar_drv.h"
#include "./inc/adv7842_drv.h"
#include "./inc/edid.h"
#include "./inc/fpga_drv.h"
#include "hal_inc_inter/capt_hal_inter.h"
#include "mcu_drv.h"
#include "capt_chip_lt86102sxe.h"
#include "capt_chip_lt9211.h"


/*****************************************************************************
                               宏定义
*****************************************************************************/
#define CAPT_CHIP_NAME "capt_chip"



/*****************************************************************************
                               结构体定义
*****************************************************************************/

/*****************************************************************************
                               全局变量
*****************************************************************************/

static CAPT_CHIP_STATUS stCaptChipStatus[CAPT_CHN_NUM_MAX] = {0};
static CAPT_CHIP_FUNC_S *pExtChipFunc[CAPT_CHN_NUM_MAX] = { NULL };


/*****************************************************************************
                                函数
*****************************************************************************/
/*****************************************************************************
 函 数 名  : capt_func_chipInitMutexLock
 功能描述  : 采集模块初始化锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月10日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_func_chipInitMutexLock(UINT32 uiChn)
{
    pthread_mutex_t * pViMutex = NULL;


    pViMutex = &stCaptChipStatus[uiChn].captChipMutex;

    memset(pViMutex, 0, sizeof(pthread_mutex_t));
    pthread_mutex_init(pViMutex, NULL);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_func_chipMutexLock
 功能描述  : 采集模块采集上锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月10日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_func_chipMutexLock(UINT32 uiChn)
{
    pthread_mutex_t * pViMutex = NULL;

    pViMutex = &stCaptChipStatus[uiChn].captChipMutex;

    pthread_mutex_lock(pViMutex);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_func_chipMutexUnLock
 功能描述  : 采集模块采集去锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月10日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_func_chipMutexUnLock(UINT32 uiChn)
{
    pthread_mutex_t * pViMutex = NULL;

    pViMutex = &stCaptChipStatus[uiChn].captChipMutex;

    pthread_mutex_unlock(pViMutex);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : capt_func_chipUpdateByFile
* 描  述  : 固件通过文件升级
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipUpdateByFile(const VOID *pvArg1, const VOID *pvArg2, const VOID *pvArg3, const VOID *pvArg4)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32Chn = *((UINT32 *)pvArg1);
    const char *szFileName = *(const char **)pvArg2;
    UINT8 *pu8Buff = NULL;
    INT32 s32Len = 0;

    SAL_UNUSED(pvArg1);
    SAL_UNUSED(pvArg2);
    SAL_UNUSED(pvArg3);
    SAL_UNUSED(pvArg4);

    if (NULL == szFileName)
    {
        CAPT_LOGE("capt chn[%u] update firmware fail: invalid file\n", u32Chn);
        return SAL_FAIL;
    }

    if (NULL == pExtChipFunc[u32Chn] || NULL == pExtChipFunc[u32Chn]->pfuncUpdate)
    {
       return SAL_FAIL;
    }

    s32Len = SAL_GetFileSize(szFileName);
    if (s32Len <= 0)
    {
        SAL_ERROR("get file size failed! \n");
        return SAL_FAIL;
    }

    pu8Buff = (UINT8 *)SAL_memZalloc((UINT32)s32Len, CAPT_CHIP_NAME, CAPT_MEM_NAME);
    if (!pu8Buff)
    {
        SAL_ERROR("malloc failed! \n");
        return SAL_FAIL;
    }
    
    if (SAL_SOK != SAL_ReadFromFile(szFileName, 0, (CHAR *)pu8Buff, (UINT32)s32Len, (UINT32 *)&s32Len))
    {
        CAPT_LOGE("capt chn[%u] update firmware fail, invalid file:%s\n", u32Chn, szFileName);
        
        (VOID)SAL_memfree(pu8Buff, "capt", "file_buf");
        return SAL_FAIL;
    }
    
    s32Ret = pExtChipFunc[u32Chn]->pfuncUpdate(u32Chn, pu8Buff, s32Len);
    (VOID)SAL_memfree(pu8Buff, "capt", "file_buf");
    
    return s32Ret;
}


/*******************************************************************************
* 函数名  : capt_func_chipEdidToFile
* 描  述  : 保存edid文件到文件
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipEdidToFile(const VOID *pvArg1, const VOID *pvArg2, const VOID *pvArg3, const VOID *pvArg4)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32Chn = *((UINT32 *)pvArg1);
    CAPT_CABLE_E enCable = (CAPT_CABLE_E)(*((INT32 *)pvArg2));
    const char *szFileName = *(const char **)pvArg3;
    UINT8 *pu8Buff = NULL;
    UINT32 u32Len = 0;

    SAL_UNUSED(pvArg1);
    SAL_UNUSED(pvArg2);
    SAL_UNUSED(pvArg3);
    SAL_UNUSED(pvArg4);

    if (NULL == szFileName)
    {
        CAPT_LOGE("capt chn[%u] update firmware fail: invalid file\n", u32Chn);
        return SAL_FAIL;
    }

    if (NULL == pExtChipFunc[u32Chn] || NULL == pExtChipFunc[u32Chn]->pfuncGetEdid)
    {
        return 0;
    }

    pu8Buff = (UINT8 *)SAL_memZalloc(512, CAPT_CHIP_NAME, CAPT_MEM_NAME);
    if (NULL == pu8Buff)
    {
        CAPT_LOGE("SAL_memZalloc fail\n");
        return SAL_FAIL;
    }

    (VOID)capt_func_chipMutexLock(u32Chn);
    u32Len = pExtChipFunc[u32Chn]->pfuncGetEdid(u32Chn, pu8Buff, 512, enCable);
    (VOID)capt_func_chipMutexUnLock(u32Chn);
    if (0 == u32Len)
    {
        CAPT_LOGE("get cable[%d] edid from chn[%u] fail\n", enCable, u32Chn);
        free(pu8Buff);
        return SAL_FAIL;
    }

    s32Ret = SAL_WriteToFile(szFileName, 0, SEEK_SET, (CHAR *)pu8Buff, u32Len);
    free(pu8Buff);
    if (SAL_SOK != s32Ret)
    {
        CAPT_LOGE("capt chn[%u] save cable[%d] edid[%u] to file %s fail\n", u32Chn, enCable, u32Len, szFileName);
        return SAL_FAIL;
    }
    
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : capt_func_chipAutoAdjust
* 描  述  : 自动调整图像
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipDebugAutoAdjust(const VOID *pvArg1, const VOID *pvArg2, const VOID *pvArg3, const VOID *pvArg4)
{
    UINT32 u32Chn = *((UINT32 *)pvArg1);

    SAL_UNUSED(pvArg1);
    SAL_UNUSED(pvArg2);
    SAL_UNUSED(pvArg3);
    SAL_UNUSED(pvArg4);

    if (NULL == pExtChipFunc[u32Chn] || NULL == pExtChipFunc[u32Chn]->pfuncAutoAdjust)
    {
        return SAL_FAIL;
    }


    (VOID)capt_func_chipMutexLock(u32Chn);
    (VOID)pExtChipFunc[u32Chn]->pfuncAutoAdjust(u32Chn, CAPT_CABLE_VGA);
    (VOID)capt_func_chipMutexUnLock(u32Chn);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : capt_func_chipHotPlug
* 描  述  : 拉一次hot plug
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipHotPlug(const VOID *pvArg1, const VOID *pvArg2, const VOID *pvArg3, const VOID *pvArg4)
{
    UINT32 u32Chn = *((UINT32 *)pvArg1);
    CAPT_CABLE_E enCable = (CAPT_CABLE_E)(*((UINT32 *)pvArg2));

    SAL_UNUSED(pvArg1);
    SAL_UNUSED(pvArg2);
    SAL_UNUSED(pvArg3);
    SAL_UNUSED(pvArg4);

    if (NULL == pExtChipFunc[u32Chn] || NULL == pExtChipFunc[u32Chn]->pfuncHotPlug)
    {
         return SAL_FAIL;
    }

    (VOID)capt_func_chipMutexLock(u32Chn);
    (VOID)pExtChipFunc[u32Chn]->pfuncHotPlug(u32Chn, enCable);
    (VOID)capt_func_chipMutexUnLock(u32Chn);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : capt_func_chipReloadEdid
* 描  述  : 从文件中重新加载一次EDID
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipReloadEdid(const VOID *pvArg1, const VOID *pvArg2, const VOID *pvArg3, const VOID *pvArg4)
{
    UINT32 u32Chn = *((UINT32 *)pvArg1);
    CAPT_CABLE_E enCable = (CAPT_CABLE_E)(*((UINT32 *)pvArg2));

    SAL_UNUSED(pvArg1);
    SAL_UNUSED(pvArg2);
    SAL_UNUSED(pvArg3);
    SAL_UNUSED(pvArg4);

    (VOID)capt_func_chipMutexLock(u32Chn);
    (VOID)EDID_ReLoadEdid(u32Chn, enCable);
    (VOID)capt_func_chipMutexUnLock(u32Chn);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : capt_func_chipAutoAdjust
* 描  述  : 自动调整图像
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipAutoAdjust(UINT32 u32Chn, CAPT_CABLE_E enCable)
{

    if (NULL == pExtChipFunc[u32Chn] || NULL == pExtChipFunc[u32Chn]->pfuncAutoAdjust)
    {
        return SAL_FAIL;
    }


    (VOID)capt_func_chipMutexLock(u32Chn);
    (VOID)pExtChipFunc[u32Chn]->pfuncAutoAdjust(u32Chn, enCable);
    (VOID)capt_func_chipMutexUnLock(u32Chn);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : capt_func_chipFpgaSetResolution
* 描  述  : 写fpga寄存器的值
* 输  入  : UINT32 u32Reg : 寄存器地址
* 输  出  : UINT32 u32Val : 值
* 返回值  : 
*******************************************************************************/
INT32 capt_func_chipFpgaSetResolution(UINT32 u32Chn, UINT32 u32Width, UINT32 u32Height, UINT32 u32Fps)
{
    /*新项目可以根据不同方式，调用不同接口设置*/
    if (SAL_SOK != Fpga_SetResolution(u32Chn, u32Width, u32Height, u32Fps))
    {
         CAPT_LOGE("Fpga_SetResolution failed !\n");
         return SAL_FAIL;
    }
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : capt_func_chipFpgaDectedAndReset
* 描  述  : 检测FPGA统计的宽高并复位
* 输  入  : 
* 输  出  : 
* 返回值  : 
*******************************************************************************/
INT32 capt_func_chipFpgaDectedAndReset(UINT32 u32Chn)
{
     if(SAL_SOK != Fpga_Reset(u32Chn))
     {
         CAPT_LOGE("Fpga_Reset failed !\n");
         return SAL_FAIL;
     }
     
     return SAL_SOK;
}


/*******************************************************************************
* 函数名  : capt_func_chipAutoAdjust
* 描  述  : 自动调整图像
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipGetAdjustStatus(UINT32 u32Chn)
{

    if (NULL == pExtChipFunc[u32Chn] || NULL == pExtChipFunc[u32Chn]->pfuncAutoAdjust)
    {
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : capt_func_chipGetEdidInfo
* 描  述  : 获取EDID信息
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipGetEdidInfo(UINT32 u32Chn, CAPT_EDID_INFO_S *pstEdidInfo)
{
    INT32 s32Ret = SAL_SOK;
    
    (VOID)capt_func_chipMutexLock(u32Chn);
    s32Ret =  EDID_GetEdidInfo(u32Chn, pstEdidInfo);
    (VOID)capt_func_chipMutexUnLock(u32Chn);

    return s32Ret;
}

/**
 * @function   capt_func_chipSetDispEdid
 * @brief      将输出EDID回写到输入
 * @param[in]  UINT32 u32Chn             通道号
 * @param[in]  UINT32 u32ViNum           采集创建数量
 * @param[in]  CAPT_EDID_DISP_S *pstEdid 设置指令
 * @param[out] None
 * @return     INT32 SAL_SOK : 成功
 *                  SAL_FAIL : 失败
 */
INT32 capt_func_chipSetDispEdid(UINT32 u32Chn,UINT32 u32ViNum, CAPT_EDID_DISP_S *pstEdid)
{
    INT32 s32Ret = SAL_SOK;

    if (pstEdid->enMode >= INPUT_VIDEO_NONE)
    {
        CAPT_LOGE("invalid input mode[%d]\n", pstEdid->enMode);
        return SAL_FAIL;
    }
    
    (VOID)capt_func_chipMutexLock(u32Chn);
    s32Ret = EDID_DispLoopBack(u32Chn, pstEdid->uiDispChn, u32ViNum, \
        (INPUT_VIDEO_VGA == pstEdid->enMode) ? EDID_SIGNAL_ANALOG : EDID_SIGNAL_DIGTAL);
    (VOID)capt_func_chipMutexUnLock(u32Chn);

    return s32Ret;
}

/**
 * @function   capt_func_chipSetEdid
 * @brief      设置EDID
 * @param[in]  UINT32 u32Chn                   采集通道号
 * @param[in]  UINT32 u32ViNum                 采集创建数
 * @param[in]  CAPT_EDID_INFO_ST *pstEdidInfo  edid信息
 * @param[out] None
 * @return     INT32 SAL_SOK : 成功
 *                  SAL_FAIL : 失败
 */
INT32 capt_func_chipSetEdid(UINT32 u32Chn,UINT32 u32ViNum, CAPT_EDID_INFO_ST *pstEdidInfo)
{
    INT32 s32Ret = 0;
    EDID_SIGNAL_TYPE_E enSignalType = EDID_SIGNAL_BUTT;
    pstEdidInfo->enInputMode = INPUT_VIODO_BUTT;
    pstEdidInfo->enError = CAPT_EDID_ERROR_FORMAT;
    
    /* 应用下发的通道数大于创建的通道数*/
    if (u32Chn >= u32ViNum)
    {
        pstEdidInfo->enError = CAPT_EDID_ERROR_NO_EEPROM;
        CAPT_LOGE("invalid chn[%u], must less than %u\n", u32Chn, u32ViNum);
        return SAL_FAIL;
    }

    (VOID)capt_func_chipMutexLock(u32Chn);
    s32Ret = EDID_SetEdid(u32Chn, pstEdidInfo->pEdidAddr, pstEdidInfo->uiByteNum, &enSignalType);
    (VOID)capt_func_chipMutexUnLock(u32Chn);
    if (SAL_SOK != s32Ret)
    {
        /* 文件格式有误 */
        if (EDID_SIGNAL_BUTT == enSignalType)
        {
            pstEdidInfo->enError = CAPT_EDID_ERROR_FORMAT;
        }

        /* VGA的会直接写入EEPROM，可能有问题，HDMI不用保存到EEPROM */
        if (EDID_SIGNAL_ANALOG == enSignalType)
        {
            if (2 == u32ViNum)
            {
                pstEdidInfo->enError = CAPT_EDID_ERROR_NO_EEPROM;
            }
            else
            {
                pstEdidInfo->enError = CAPT_EDID_ERROR_IIC;
            }
        }
        else
        {
            pstEdidInfo->enError = CAPT_EDID_ERROR_FORMAT;
        }

        return SAL_FAIL;
    }

    pstEdidInfo->enInputMode = (EDID_SIGNAL_DIGTAL == enSignalType) ? INPUT_VIDEO_HDMI : INPUT_VIDEO_VGA;
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : capt_func_chipSetCsc
* 描  述  : 设置CSC
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipSetCsc(UINT32 u32Chn, VIDEO_CSC_S *pstCsc)
{
    INT32 s32Ret = SAL_SOK;

    if (u32Chn >= CAPT_CHN_NUM_MAX)
    {
        CAPT_LOGE("capt invalid chn[%u]\n", u32Chn);
        return SAL_FAIL;
    }
    
    if (NULL == pExtChipFunc[u32Chn] || NULL == pExtChipFunc[u32Chn]->pfuncSetCsc)
    {
        CAPT_LOGE("capt chn[%u] csc function not init\n", u32Chn);
        return SAL_FAIL;
    }
    
    (VOID)capt_func_chipMutexLock(u32Chn);
    
    s32Ret = pExtChipFunc[u32Chn]->pfuncSetCsc(u32Chn, pstCsc);
    
    (VOID)capt_func_chipMutexUnLock(u32Chn);

    return s32Ret;
}


/*******************************************************************************
* 函数名  : capt_func_chipIsFpgaNeedReset
* 描  述  : 自动调整图像
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
inline BOOL capt_func_chipIsFpgaNeedReset(VOID)
{
    return (HARDWARE_INPUT_CHIP_MSTAR == HARDWARE_GetInputChip()) ? SAL_TRUE : SAL_FALSE;
}


/*******************************************************************************
* 函数名  : CaptHal_AdvModeInit
* 描  述  : 用于adv7842模块初始化
* 输  入  : - chnNum   : 通道号 
* 输  出  : - 无
* 返回值  : 0        : 成功
*          -1        : 失败
*******************************************************************************/
INT32 capt_func_chipCheckStatus(UINT32 chnId)
{
    CAPT_STATUS_S *pstStatus = &(stCaptChipStatus[chnId].stCaptStatus);

    if ((0 != pstStatus->stRes.u32Width) &&
        (0 != pstStatus->stRes.u32Height) &&
        (0 != pstStatus->stRes.u32Fps) &&
        (pstStatus->enCable < CAPT_CABLE_BUTT) &&
        (SAL_TRUE == pstStatus->bVideoDetected))
    {
        return 1;
    }

    return 0;
}


/*******************************************************************************
* 函数名  : capt_func_chipGetStatus
* 描  述  : 用于视频输入状态
* 输  入  : - chnId    : vi 通道 
* 输  出  : - outPrm
* 返回值  : 0        : 成功
*          -1        : 失败
*******************************************************************************/
INT32 capt_func_chipGetStatus(UINT32 chnId, CAPT_STATUS_S *pstStatus)
{
    if (chnId >= 2U)
    {
        CAPT_LOGE("input is %d error\n",chnId);
        return SAL_FAIL;
    }
    
    if (NULL == pstStatus)
    {
        CAPT_LOGE("input %d outPrm is NULL\n",chnId);
        return SAL_FAIL;
    }
    
    SAL_memCpySize(pstStatus, &(stCaptChipStatus[chnId].stCaptStatus), sizeof(CAPT_STATUS_S));

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : capt_func_chipGetStatus
* 描  述  : 用于视频输入状态
* 输  入  : - chnId    : vi 通道 
* 输  出  : - outPrm
* 返回值  : 0        : 成功
*          -1        : 失败
*******************************************************************************/
INT32 capt_func_chipSetStatus(UINT32 chnId, CAPT_STATUS_S *pstStatus)
{
    if (chnId >= 2U)
    {
        CAPT_LOGE("input is %d error\n",chnId);
        return SAL_FAIL;
    }
    
    if (NULL == pstStatus)
    {
        CAPT_LOGE("input %d outPrm is NULL\n",chnId);
        return SAL_FAIL;
    }
    
    SAL_memCpySize(&(stCaptChipStatus[chnId].stCaptStatus), pstStatus, sizeof(CAPT_STATUS_S));

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : capt_func_chipDetect
* 描  述  : 检测输入并配置
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipDetect(UINT32 chnId, CAPT_STATUS_S *pstStatus)
{
    INT32 ret = SAL_SOK;
    
    if (NULL == pExtChipFunc[chnId] || NULL == pExtChipFunc[chnId]->pfuncDetect)
    {
         return SAL_FAIL;
    }

    ret = pExtChipFunc[chnId]->pfuncDetect(chnId, pstStatus);

    return ret;
}

/*******************************************************************************
* 函数名  : capt_func_chipScale
* 描  述  : 输入分辨率大于面板分辨率时，会对输入分辨率进行缩放，计算缩放后的分辨率
* 输  入  : CAPT_RESOLUTION_S *pstSrc
* 输  出  : CAPT_RESOLUTION_S *pstDst
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipScale(UINT32 chnId, CAPT_RESOLUTION_S *pstDst, CAPT_RESOLUTION_S *pstSrc)
{
    INT32 ret = SAL_SOK;
    
    if (NULL == pExtChipFunc[chnId] || NULL == pExtChipFunc[chnId]->pfuncScale)
    {
         return SAL_FAIL;
    }

    ret = pExtChipFunc[chnId]->pfuncScale(pstDst, pstSrc);

    return ret;
   
}

/*******************************************************************************
* 函数名  : capt_func_chipSetOutputRes
* 描  述  : 设置输出分辨率
* 输  入  : UINT32 u32Chn
          CAPT_RESOLUTION_S *pstRes
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipSetOutputRes(UINT32 chnId, CAPT_RESOLUTION_S *pstRes)
{
    
    if (NULL == pExtChipFunc[chnId] || NULL == pExtChipFunc[chnId]->pfuncSetRes)
    {
         return SAL_FAIL;
    }

    (VOID)pExtChipFunc[chnId]->pfuncSetRes(chnId, pstRes);

    return SAL_SOK;
   
}

/*******************************************************************************
* 函数名  : capt_func_chipSetOutputRes
* 描  述  : 获取输出输出分辨率
* 输  入  : UINT32 u32Chn
          CAPT_RESOLUTION_S *pstRes
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipGetOutputRes(UINT32 chnId, CAPT_RESOLUTION_S *pstRes)
{

    if (NULL == pExtChipFunc[chnId] || NULL == pExtChipFunc[chnId]->pfuncSetRes)
    {
       return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : VencHal_drvGetViSrcRate
 功能描述  : 获取采集源帧率
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史       :
  1.日      期   : 2019年03月06日
    作      者   : 
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_func_chipGetViSrcRate(UINT32 viChn)
{
    INT32 ret = 0;
    UINT32 fps = 60;
    static CAPT_STATUS_S outPrm = {0};
    CAPB_PRODUCT *pstProduct = capb_get_product();
    CAPB_CAPT *pstCaptCap = capb_get_capt();

    if(NULL == pstProduct || NULL == pstCaptCap)
    {
        CAPT_LOGE("get platform capbility fail\n");
        return SAL_FAIL;
    }

    /* 采集由DSP内部生成，返回生成的帧率 */
    if (VIDEO_INPUT_INSIDE == pstProduct->enInputType)
    { 
        return pstCaptCap->uiUserFps;
    }

     ret = capt_func_chipGetStatus(viChn, &outPrm);
     if (ret < 0)
     {
         CAPT_LOGE("viChn = %d\n", viChn);
         goto EXIT;
     }
     
     fps = outPrm.stRes.u32Fps ? outPrm.stRes.u32Fps : 60;

EXIT:
    return fps;
}

/**
 * @function   capt_func_chipMcuheartBeatStart
 * @brief      开始发送心跳
 * @param[in]  UINT32 u32Chn  
 * @param[out] None
 * @return     INT32 SAL_SOK : 成功
*              SAL_FAIL : 失败
 */
INT32 capt_func_chipMcuheartBeatStart(UINT32 u32Chn)
{
    HARDWARE_INPUT_CHIP_E enChip = HARDWARE_GetInputChip();

    if (HARDWARE_INPUT_CHIP_MCU_MSTAR == enChip || HARDWARE_INPUT_CHIP_LT9211_MCU_MSTAR == enChip)
    {
        return mcu_func_heartBeatStart(u32Chn);
    }
    else
    {
        CAPT_LOGW("current device not support mcu\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   CaptHal_drvHeartBeatStop
 * @brief      停止发送心跳
 * @param[in]  UINT32 u32Chn  
 * @param[out] None
 * @return     INT32 SAL_SOK : 成功
 *                  SAL_FAIL : 失败
 */
INT32 capt_func_chipMcuheartBeatStop(UINT32 u32Chn)
{
    HARDWARE_INPUT_CHIP_E enChip = HARDWARE_GetInputChip();

    if (HARDWARE_INPUT_CHIP_MCU_MSTAR == enChip || HARDWARE_INPUT_CHIP_LT9211_MCU_MSTAR == enChip)
    {
        return mcu_func_heartBeatStop(u32Chn);
    }
    else
    {
        CAPT_LOGE("current device not support mcu\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   capt_func_chipGetBuildTime
 * @brief      获取对应芯片固件的编译时间
 * @param[in]  UINT32 u32Chn  
 * @param[out] None
 * @return     INT32 SAL_SOK : 成功
 *                  SAL_FAIL : 失败
 */
INT32 capt_func_chipGetBuildTime(UINT32 u32Chn, VIDEO_CHIP_BUILD_TIME_S *pstTime)
{
    INT32 s32Ret = SAL_SOK;

    switch (pstTime->enChip)
    {
        case VIDEO_INPUT_CHIP_MSTAR:
            s32Ret = mcu_drvMstarGetBuildTime(u32Chn, (UINT8 *)pstTime->szBuildTime, sizeof(pstTime->szBuildTime));
            break;
        case VIDEO_MISC_CHIP_FPGA:
            s32Ret = Fpga_GetBuildTime(u32Chn, (UINT8 *)pstTime->szBuildTime, sizeof(pstTime->szBuildTime));
            break;
        case VIDEO_MISC_CHIP_MCU:
            s32Ret = mcu_func_getBuildTime(u32Chn, (UINT8 *)pstTime->szBuildTime, sizeof(pstTime->szBuildTime));
            break;
        default:
            CAPT_LOGE("capt chn[%u] unsupport chip:0x%x\n", u32Chn, pstTime->enChip);
            return SAL_FAIL; 
    }

    return s32Ret;
}

/**
 * @function   capt_func_chipReset
 * @brief      按顺序先后复位FPGA,MSTAR,MCU
 * @param[in]  UINT32 u32Chn  
 * @param[out] None
 * @return     INT32 SAL_SOK : 成功
 *                  SAL_FAIL : 失败
 */
INT32 capt_func_chipReset(UINT32 u32Chn)
{
    INT32 s32Ret = SAL_SOK;

    if (SAL_SOK != (s32Ret = mcu_func_fpgaReset(u32Chn)))
    {
        CAPT_LOGE("capt chn[%u] FPGA reset failed\n", u32Chn);
        return s32Ret;
    }

    if (SAL_SOK != (s32Ret = mcu_func_mstarReset(u32Chn)))
    {
        CAPT_LOGE("capt chn[%u] mstar reset failed\n", u32Chn);
        return s32Ret;
    }

    if (SAL_SOK != (s32Ret = mcu_func_mcuReset(u32Chn)))
    {
        CAPT_LOGE("capt chn[%u] MCU reset failed\n", u32Chn);
        return s32Ret;
    }
    CAPT_LOGI("capt chn[%u] FPGA MSTAR MCU reset successfully\n", u32Chn);
    
    return s32Ret;
}

/**
 * @function   capt_func_chipMcuFirmwareUpdate
 * @brief      单片机固件包升级
 * @param[in]  UINT32 u32Chn         采集通道
 * @param[in]  const UINT8 *pu8Buff  升级固件
 * @param[in]  UINT32 u32Len         数长度
 * @param[out] None
 * @return     INT32
 */
INT32 capt_func_chipMcuFirmwareUpdate(UINT32 u32Chn, const UINT8 *pu8Buff, UINT32 u32Len)
{
    HARDWARE_INPUT_CHIP_E enChip = HARDWARE_GetInputChip();

    if (HARDWARE_INPUT_CHIP_MCU_MSTAR == enChip || HARDWARE_INPUT_CHIP_LT9211_MCU_MSTAR == enChip)
    {
        return mcu_func_firmwareUpdate( u32Chn, pu8Buff,  u32Len);
    }
    else
    {
        CAPT_LOGE("current device not support mcu\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : CaptHal_AdvModeInit
* 描  述  : 用于采集芯片模块初始化
* 输  入  : - chnNum   : 通道号 
* 输  出  : - 无
* 返回值  : 0        : 成功
*          -1        : 失败
*******************************************************************************/
INT32 capt_func_chipInit(UINT32 chnId)
{
    INT32 ret = SAL_SOK;
    HARDWARE_INPUT_CHIP_E enChip = HARDWARE_GetInputChip();
    VIDEO_EDID_BUFF_S *pstEdidBuff = NULL;
    
    if (chnId >= 2 )
    {
        CAPT_LOGE("chnid is > 1 !!!\n");
        return SAL_FAIL;
    }

    if (NULL == pExtChipFunc[chnId] || NULL == pExtChipFunc[chnId]->pfuncInit)
    {
        CAPT_LOGE("chn[%u] init function not set\n", chnId);
        return SAL_FAIL;
    }
    (VOID)capt_func_chipMutexLock(chnId);

    if(NULL !=  pExtChipFunc[chnId]->pfuncSetEdid)
    {
        (VOID)EDID_RegisterSetFunc(chnId, (EDID_SetChipEdid)pExtChipFunc[chnId]->pfuncSetEdid);
    }
    
    ret = pExtChipFunc[chnId]->pfuncInit(chnId);
    if (SAL_SOK != ret)
    {
        CAPT_LOGE("chnid is %d !!!\n",chnId);
        (VOID)capt_func_chipMutexUnLock(chnId);
        return SAL_FAIL; 
    }

       

    if (HARDWARE_INPUT_CHIP_MCU_MSTAR == enChip || HARDWARE_INPUT_CHIP_LT9211_MCU_MSTAR == enChip)
    {
        pstEdidBuff = (VIDEO_EDID_BUFF_S *)EDID_GetEdid(chnId, VIDEO_CABLE_VGA);
        (VOID)mcu_func_mstarCheckEdid(chnId, VIDEO_CABLE_VGA, pstEdidBuff);
        pstEdidBuff = (VIDEO_EDID_BUFF_S *)EDID_GetEdid(chnId, VIDEO_CABLE_HDMI);
        (VOID)mcu_func_mstarCheckEdid(chnId, VIDEO_CABLE_HDMI, pstEdidBuff);
        pstEdidBuff = (VIDEO_EDID_BUFF_S *)EDID_GetEdid(chnId, VIDEO_CABLE_DVI);
        (VOID)mcu_func_mstarCheckEdid(chnId, VIDEO_CABLE_DVI, pstEdidBuff);
    }

    /* fpga初始化配置为不出图 */
    SAL_clear(&(stCaptChipStatus[chnId].stCaptStatus));
    stCaptChipStatus[chnId].stCaptStatus.enCable = CAPT_CABLE_BUTT;
    stCaptChipStatus[chnId].stCaptStatus.bVideoDetected = SAL_FALSE;
    Fpga_SetResolution(chnId, 0, 0, 60);
    
    (VOID)capt_func_chipMutexUnLock(chnId);

    CAPT_LOGI("capt_chip chnid is %d success !!!\n",chnId);

    return SAL_SOK;
    
}


/*******************************************************************************
* 函数名  : capt_func_chipNewCreate
* 描  述  : 创建外接输入芯片
* 输  入  : - chnNum   : 通道号 
* 输  出  : - 无
* 返回值  : 0        : 成功
*          -1        : 失败
*******************************************************************************/
static INT32 capt_func_chipNewCreate(UINT32 uiChn)
{

    INT32 i = 0;
    CAPB_CAPT *pstCaptCap = capb_get_capt();

    for(i = 0; i < pstCaptCap->stCaptChip[uiChn].u8ChipNum; i++)
    {
        switch(pstCaptCap->stCaptChip[uiChn].astChipInfo[i].enCaptChip)
        {
            case CAPT_CHIP_FPGA:
                break;
            case CAPT_CHIP_MST91A4:
                pExtChipFunc[uiChn] = mstar_chipRegister();
                break;
            case CAPT_CHIP_LT9211_MIPI:
                capt_chip_lt9211ModuleInit(i,pstCaptCap->stCaptChip[uiChn].astChipInfo[i].stChipI2c.u8BusIdx,
                                               pstCaptCap->stCaptChip[uiChn].astChipInfo[i].stChipI2c.u8ChipAddr);
                break;
            case CAPT_CHIP_LT86102SXE:
                capt_chip_lt86102sxeInit(pstCaptCap->stCaptChip[uiChn].astChipInfo[i].stChipI2c.u8BusIdx,
                                               pstCaptCap->stCaptChip[uiChn].astChipInfo[i].stChipI2c.u8ChipAddr);
                break;
            default:
                break;
        }
    }


    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : capt_func_chipCreate
* 描  述  : 创建外接输入芯片
* 输  入  : - chnNum   : 通道号 
* 输  出  : - 无
* 返回值  : 0        : 成功
*          -1        : 失败
*******************************************************************************/
INT32 capt_func_chipCreate(UINT32 uiChn)
{
    HARDWARE_INPUT_CHIP_E enChip = HARDWARE_GetInputChip();
    UINT32  u32BoardType = HARDWARE_GetBoardType();
    CAPB_CAPT *pstCaptCap = capb_get_capt();
    if(NULL == pstCaptCap)
    {
        CAPT_LOGE("get platform capbility fail\n");
        return SAL_FAIL;
    }

    if(u32BoardType == DB_RS20032_V1_0)
    {
         
        capt_func_chipNewCreate(uiChn);
       
    }
    else
    {
        switch (enChip)
        {
            case HARDWARE_INPUT_CHIP_ADV7842:
                pExtChipFunc[uiChn] = ADV_chipRegister();
                break;
            case HARDWARE_INPUT_CHIP_MSTAR:
                pExtChipFunc[uiChn] = mstar_chipRegister();
                break;
            case HARDWARE_INPUT_CHIP_MCU_MSTAR:
                pExtChipFunc[uiChn] = mcu_func_mstarRegister();
                break;
            case HARDWARE_INPUT_CHIP_LT9211_MCU_MSTAR:
                pExtChipFunc[uiChn] = mcu_func_mstarRegister();
                /* 一个mstar对应一个lt9211 */ 
                capt_chip_lt9211ModuleInit(uiChn,pstCaptCap->stCaptChip[uiChn].astChipInfo[0].stChipI2c.u8BusIdx,
                                            pstCaptCap->stCaptChip[uiChn].astChipInfo[0].stChipI2c.u8ChipAddr);
                break;
            default:
                CAPT_LOGW("capt chn[%u] invalid capt chip[%d]\n", uiChn, enChip);
            break;
        }

        Fpga_Init(uiChn);

    }
    capt_func_chipInitMutexLock(uiChn);

    return SAL_SOK;
}



