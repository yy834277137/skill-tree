/**
 * @file   audioResample_drv.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---重采样接口
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，audio_drv中提取重采样接口
 */

/*****************************************************************************
                        头文件
*****************************************************************************/
#include "sal.h"
#include "sal_mem_new.h"
#include "audio_common.h"
#include "audioResample_drv_api.h"
#include "hik_res_lib.h"

/*****************************************************************************
                            宏定义
*****************************************************************************/


#define RESAMPLE_FRAME_LEN (320)

/*****************************************************************************
                         结构定义
*****************************************************************************/

typedef struct _AUDIO_RESAMPLE_DRV_
{
    void *pHandle;
    UINT32 u32FrameLen;
    UINT32 u3FrameNum;

    UINT8 *pInputBuf;               /* 外部传入 */
    UINT32 u32InputBufLen;
    UINT32 u32InputLen;             /* 外部传入 */

    UINT8 *pOutputBuf;              /* 输出 */
    UINT32 u32OutputBufLen;
    UINT32 u32OutputLen;            /* 输出 */

    MEM_TAB stMemTab[1];
    HIK_RESAMPLE_PROC_PARAM stProcParam; /*可以省略in/out buf*/

} AUDIO_RESAMPLE_DRV_S;


/*****************************************************************************
                        函数
*****************************************************************************/

/**
 * @function   audioResample_drv_create
 * @brief    创建重采样句柄
 * @param[in]  RESAMPLE_INFO_S *pstResampleInfo 重采样初始化参数
 * @param[out] void **ppHandle 重采样句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioResample_drv_create(void **ppHandle, RESAMPLE_INFO_S *pstResampleInfo)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 u32ResampleDataSize = 0;
    HIK_RESAMPLE_PARAM stParam = {0};
    AUDIO_RESAMPLE_DRV_S *pstResampleCtrl = NULL;
    MEM_TAB *pstMemTab = NULL;

    if ((NULL == ppHandle) || (NULL == pstResampleInfo))
    {
        AUD_LOGE("pstInPrm is NULL!\n");
        return SAL_FAIL;
    }

    if ((0 == pstResampleInfo->u32SrcRate)
        || (0 == pstResampleInfo->u32DesRate)
        || (NULL != *ppHandle))
    {
        AUD_LOGE("sample rate err! %d -> %d!\n", pstResampleInfo->u32SrcRate, pstResampleInfo->u32DesRate);
        return SAL_FAIL;
    }

    if (NULL == pstResampleInfo->pInputBuf
        || NULL == pstResampleInfo->pOutputBuf)
    {
        AUD_LOGE("buf null!\n");
        return SAL_FAIL;
    }

    pstResampleCtrl = (AUDIO_RESAMPLE_DRV_S *)SAL_memMalloc(sizeof(AUDIO_RESAMPLE_DRV_S), "audio", "resample");
    if (NULL == pstResampleCtrl)
    {
        AUD_LOGE("ERROR: malloc!\n");
        return SAL_FAIL;
    }

    /* 取采样率的十进制高三位作为一次重采样处理数据的大小，保证输出采样频率 / 输入采样频率 * 输入采样点数得到的输出采样点数为正整数 */
    u32ResampleDataSize = pstResampleInfo->u32SrcRate;
    while (1)
    {
        if (u32ResampleDataSize > 100 && u32ResampleDataSize < 1000)
        {
            break;
        }
        u32ResampleDataSize = u32ResampleDataSize / 10;
    }

    pstMemTab = &pstResampleCtrl->stMemTab[0];
    pstResampleCtrl->u32FrameLen = u32ResampleDataSize;
    stParam.channel_num = 1;
    stParam.level = 1;
    stParam.bits_per_sample = 16;
    stParam.proc_num = u32ResampleDataSize;
    stParam.src_sample_rate = pstResampleInfo->u32SrcRate;
    stParam.dst_sample_rate = pstResampleInfo->u32DesRate;

    s32Ret = HIK_RES_GetMemSize(&stParam, pstMemTab);
    if (s32Ret != HIK_RESAMPLE_LIB_S_OK)
    {
        SAL_memfree(pstResampleCtrl, "audio", "resample");
        AUD_LOGE(" get mem size error, 0x%x \n" , s32Ret);
        return SAL_FAIL;
    }

    pstMemTab->base = (unsigned char *)SAL_memAlign(pstMemTab->alignment, pstMemTab->size, "audio", "resample");
    pstResampleCtrl->stProcParam.in_pcm_buf = (short *)SAL_memMalloc(stParam.in_pcm_buf_size, "audio", "resample");
    pstResampleCtrl->stProcParam.out_pcm_buf = (short *)SAL_memMalloc(stParam.out_pcm_buf_size, "audio", "resample");
    /* SAL_INFO("in pcm size %d,out pcm size %d\n", stParam.in_pcm_buf_size, stParam.out_pcm_buf_size); */

    if (NULL == pstMemTab->base
        || NULL == pstResampleCtrl->stProcParam.in_pcm_buf
        || NULL == pstResampleCtrl->stProcParam.out_pcm_buf)
    {
        AUD_LOGE("ERROR: malloc!\n");
        goto fail;
    }

    memset(pstResampleCtrl->stProcParam.in_pcm_buf, 0x00, stParam.in_pcm_buf_size);


    s32Ret = HIK_RES_Create(&stParam, pstMemTab, &(pstResampleCtrl->pHandle));
    if (s32Ret != HIK_RESAMPLE_LIB_S_OK)
    {
        AUD_LOGE("ERROR: init!\n");
        goto fail;
    }

    pstResampleCtrl->pInputBuf = pstResampleInfo->pInputBuf;
    pstResampleCtrl->u32InputBufLen = pstResampleInfo->u32InputBufLen;

    pstResampleCtrl->pOutputBuf = pstResampleInfo->pOutputBuf;
    pstResampleCtrl->u32OutputBufLen = pstResampleInfo->u32OutputBufLen;
    
    pstResampleCtrl->u32InputLen = 0;

    *ppHandle = (void *)pstResampleCtrl;

    AUD_LOGI(" audioResample create success! level %d,sample_rate %d,%d,frameLen %d,u32ResampleDataSize %d\n", stParam.level, stParam.src_sample_rate, stParam.dst_sample_rate, stParam.proc_num, u32ResampleDataSize);

    return SAL_SOK;

fail:
    if (pstResampleCtrl != NULL)
    {
        SAL_memfree(pstResampleCtrl, "audio", "resample");
    }

    if (pstMemTab->base != NULL)
    {
        SAL_memfree(pstMemTab->base, "audio", "resample");
    }

    if (pstResampleCtrl->stProcParam.in_pcm_buf != NULL)
    {
        SAL_memfree(pstResampleCtrl->stProcParam.in_pcm_buf, "audio", "resample");
    }

    if (pstResampleCtrl->stProcParam.out_pcm_buf != NULL)
    {
        SAL_memfree(pstResampleCtrl->stProcParam.out_pcm_buf, "audio", "resample");
    }

    *ppHandle = NULL;

    return SAL_FAIL;
}

/**
 * @function   audioResample_drv_Process
 * @brief    重采样处理
 * @param[in]  void * pHandle 重采样句柄
 * @param[in] AUDIO_INTERFACE_INFO_S *pstInPrm 重采样参数
 * @param[out] AUDIO_INTERFACE_INFO_S *pstInPrm 重采样参数
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioResample_drv_Process(void *pHandle, AUDIO_INTERFACE_INFO_S *pstInterfaceInfo)
{
    INT32 s32Ret = SAL_FAIL;
    AUDIO_RESAMPLE_DRV_S *pstResampleCtrl = NULL;
    UINT32 u32TotalDecLen = 0;
    UINT32 u32RestLen = 0;
    UINT32 u32FrmNum = 0;

    if ((NULL == pHandle) || (NULL == pstInterfaceInfo))
    {
        AUD_LOGE("pstInPrm is NULL!\n");
        return SAL_FAIL;
    }

    pstResampleCtrl = (AUDIO_RESAMPLE_DRV_S *)pHandle;

    if (pstInterfaceInfo->uiInputLen + pstResampleCtrl->u32InputLen > pstResampleCtrl->u32InputBufLen)
    {
        AUD_LOGE("buff errro InterfaceInfo uiOutputLen %d, ResampleCtrl u32InputLen %d, uiInputBufLen %d \n", pstInterfaceInfo->uiInputLen, pstResampleCtrl->u32InputLen, pstResampleCtrl->u32InputBufLen);
        return SAL_FAIL;
    }

    memmove(pstResampleCtrl->pInputBuf + pstResampleCtrl->u32InputLen, pstInterfaceInfo->pInputBuf, pstInterfaceInfo->uiInputLen);
    pstResampleCtrl->u32InputLen += pstInterfaceInfo->uiInputLen;

    pstResampleCtrl->u32FrameLen = pstResampleCtrl->u32FrameLen <= 0 ? RESAMPLE_FRAME_LEN : pstResampleCtrl->u32FrameLen;

    /* 每个采样点16bit*/
    if (pstResampleCtrl->u32InputLen < pstResampleCtrl->u32FrameLen * 2)
    {
        /* 当前数据不够一次重采样，且没有进行过一次重采样没有数据输出 */
        AUD_LOGD("Resample need more data! pstResampleCtrl->u32InputLen:%u pstResampleCtrl->u32FrameLen * 2:%u\n", pstResampleCtrl->u32InputLen, pstResampleCtrl->u32FrameLen * 2);
        return SAL_EBUSY;
    }

    pstResampleCtrl->u32OutputLen = 0;
    memcpy(pstResampleCtrl->stProcParam.in_pcm_buf, pstResampleCtrl->pInputBuf, pstResampleCtrl->u32FrameLen * 2);
    do
    {
        //AUD_LOGI("uiInputLen %d,uiOutputLen %d\n", pstResampleCtrl->u32InputLen, pstResampleCtrl->u32OutputLen);

        s32Ret = HIK_RES_Process(pstResampleCtrl->pHandle, &pstResampleCtrl->stProcParam);
        pstResampleCtrl->u3FrameNum++;
        if (s32Ret == HIK_RESAMPLE_LIB_S_OK)
        {
            if ((pstResampleCtrl->u32OutputLen + pstResampleCtrl->stProcParam.output_pcm_size * 2) > pstResampleCtrl->u32OutputBufLen)
            {
                AUD_LOGE("buff errro uiOutputLen %d, uiOutputBufLen %d \n", pstResampleCtrl->u32OutputLen, pstResampleCtrl->u32OutputBufLen);
                return SAL_FAIL;
            }

            memcpy(pstResampleCtrl->pOutputBuf + pstResampleCtrl->u32OutputLen, pstResampleCtrl->stProcParam.out_pcm_buf, pstResampleCtrl->stProcParam.output_pcm_size * 2);
            pstResampleCtrl->u32OutputLen += pstResampleCtrl->stProcParam.output_pcm_size * 2;
            u32TotalDecLen += pstResampleCtrl->u32FrameLen * 2;
            u32RestLen = pstResampleCtrl->u32InputLen - u32TotalDecLen;
            u32FrmNum++;

            if (pstResampleCtrl->u32FrameLen * 2 <= u32RestLen)
            {
                memcpy(pstResampleCtrl->stProcParam.in_pcm_buf, pstResampleCtrl->pInputBuf + u32TotalDecLen, pstResampleCtrl->u32FrameLen * 2);
            }
            else
            {
                /* 剩余数据不够一次重采样，但进行过至少一次重采样有数据输出 */
                if (u32RestLen > 0)
                {
                    if (u32TotalDecLen < u32RestLen)
                    {
                        AUD_LOGE("buff errro u32TotalDecLen %d, u32RestLen %d \n", u32TotalDecLen, u32RestLen);
                    }

                    memmove(pstResampleCtrl->pInputBuf, pstResampleCtrl->pInputBuf + u32TotalDecLen, u32RestLen);
                    pstResampleCtrl->u32InputLen = u32RestLen;
                }
                else
                {
                    pstResampleCtrl->u32InputLen = 0;
                }

                /* 剩余数据保存在重采样InputBuf中，在下一次数据来的时候被一起输出，或在重采样handle销毁时被丢弃 */
                /* 暂停码流时剩余数据没被播放，并在继续播放后被先播放，后续最好优化 */
                break;
            }
        }
        else if (s32Ret == HIK_RESAMPLE_LIB_S_NEED_MORE_DATA)
        {
            SAL_WARN("frame %d: need more data \n", u32FrmNum);
            break;
        }
        else
        {
            s32Ret = HIK_RES_Reset(pstResampleCtrl->pHandle);
            pstResampleCtrl->u3FrameNum = 0;
            pstResampleCtrl->u32InputLen = 0;
            SAL_ERROR("frame %d: need more data \n", pstResampleCtrl->u3FrameNum);
            return SAL_FAIL;
        }
    }
    while (1);

    memcpy(pstInterfaceInfo->pOutputBuf, pstResampleCtrl->pOutputBuf, pstResampleCtrl->u32OutputLen);
    pstInterfaceInfo->uiOutputLen = pstResampleCtrl->u32OutputLen;
    pstResampleCtrl->u32OutputLen = 0;
    /* SAL_INFO("resample in len %d rest %d out len %d\n", pstInterfaceInfo->uiInputLen,pstResampleCtrl->uiInputLen, pstInterfaceInfo->uiOutputLen); */

    return SAL_SOK;
}

/**
 * @function   audioResample_drv_destroy
 * @brief    销毁重采样句柄
 * @param[in]  void **ppHandle 重采样句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioResample_drv_destroy(void **ppHandle)
{
    AUDIO_RESAMPLE_DRV_S *pstResampleCtrl = NULL;
    MEM_TAB *pstMemTab = NULL;

    if (NULL == ppHandle)
    {
        AUD_LOGE("pstInPrm is NULL!\n");
        return SAL_FAIL;
    }

    pstResampleCtrl = (AUDIO_RESAMPLE_DRV_S *)(*ppHandle);
    if (NULL == pstResampleCtrl)
    {
        AUD_LOGE("pstInPrm is NULL!\n");
        return SAL_FAIL;
    }

    pstMemTab = &pstResampleCtrl->stMemTab[0];
    if (pstMemTab->base != NULL)
    {
        SAL_memfree(pstMemTab->base, "audio", "resample");
    }

    if (pstResampleCtrl->stProcParam.in_pcm_buf != NULL)
    {
        SAL_memfree(pstResampleCtrl->stProcParam.in_pcm_buf, "audio", "resample");
    }

    if (pstResampleCtrl->stProcParam.out_pcm_buf != NULL)
    {
        SAL_memfree(pstResampleCtrl->stProcParam.out_pcm_buf, "audio", "resample");
    }

    SAL_memfree(pstResampleCtrl, "audio", "resample");

    AUD_LOGI(" deinit success!\n");
    *ppHandle = NULL;

    return SAL_SOK;
}

