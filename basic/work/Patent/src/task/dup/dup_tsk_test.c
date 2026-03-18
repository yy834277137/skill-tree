/**
 * @file   dup_tsk_test.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  DUP测试代码，主要功能是向DUP发送图像帧
 * @author yeyanzhong
 * @date   2021.7.26
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
     2021.7.26, 增加测试代码，还未调试
 */

#include "dup_tsk_api.h"
#include "system_prm_api.h"
#include "platform_hal.h"
#include "sal.h"


void *dup_testThread(void *pDupHandle)
{
    INT32 s32Ret = SAL_FAIL;
    ALLOC_VB_INFO_S stVbInfo = {0};
    UINT32 u32LumaSize = 0;
    //UINT32 u32ChrmSize = 0;
    SAL_VideoFrameBuf salVideoInfo = {0};
    UINT32 u32Width, u32Height = 0;
    UINT64 u64BlkSize = 0;
    Ptr pImgSrc = NULL;
    DupHandle dupHandle = *(DupHandle *)pDupHandle;
    
    SYSTEM_FRAME_INFO stFrmInfo;
    memset(&stFrmInfo, 0, sizeof(stFrmInfo));


    DSPINITPARA *pDspInitPara = NULL;

    pDspInitPara = SystemPrm_getDspInitPara();
    if (pDspInitPara == NULL)
    {
        DISP_LOGE("error\n");
        return NULL;
    }

    pImgSrc = (pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgAddr[0]);
    u32Width = pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgW[0];
    u32Height = pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgH[0];
    u64BlkSize = pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgSize[0];

 
    sys_hal_allocVideoFrameInfoSt(&stFrmInfo);

   s32Ret = mem_hal_vbAlloc(u64BlkSize, "dup_task", "test", NULL, SAL_FALSE, &stVbInfo);
    if (SAL_FAIL == s32Ret)
    {
        DISP_LOGE("mem_hal_vbAlloc failed size %llu !\n", u64BlkSize);
        return NULL;
    }

    u32LumaSize = (u32Width * u32Height);
    //u32ChrmSize = (u32Width * u32Height / 2);

    salVideoInfo.poolId = stVbInfo.u32PoolId;
    salVideoInfo.frameParam.width = u32Width;
    salVideoInfo.frameParam.height = u32Height;
    salVideoInfo.phyAddr[0] = stVbInfo.u64PhysAddr;
    salVideoInfo.phyAddr[1] = salVideoInfo.phyAddr[0]+ u32LumaSize;
    salVideoInfo.phyAddr[2] = salVideoInfo.phyAddr[0]+ u32LumaSize;

    salVideoInfo.virAddr[0] = (UINT64)stVbInfo.pVirAddr;
    salVideoInfo.virAddr[1] = salVideoInfo.virAddr[0]+ u32LumaSize;
    salVideoInfo.virAddr[2] = salVideoInfo.virAddr[0]+ u32LumaSize;

    salVideoInfo.stride[0] = u32Width;
    salVideoInfo.stride[1] = u32Width;
    salVideoInfo.stride[2] = u32Width;
    salVideoInfo.vbBlk = stVbInfo.u64VbBlk;
    memcpy((void *)salVideoInfo.virAddr[0], pImgSrc, u64BlkSize);

    sys_hal_buildVideoFrame(&salVideoInfo, &stFrmInfo);
    stFrmInfo.uiDataAddr = (PhysAddr)salVideoInfo.virAddr[0];
    stFrmInfo.uiDataWidth = u32Width;
    stFrmInfo.uiDataHeight = u32Height;

    SAL_SET_THR_NAME();

    while (1)
    {
        dup_task_sendToDup(dupHandle, &stFrmInfo);
        usleep(60);
    }

    return NULL;
}

extern INST_CFG_S stVpDupInstCfg;

void dup_task_test()
{
    SAL_ThrHndl thrHndl;
    DupHandle dupHandle;
    UINT32 uiChn = 0;

    dup_task_vpDupCreate(&stVpDupInstCfg, uiChn, &dupHandle);
    SAL_thrCreate(&thrHndl, dup_testThread, SAL_THR_PRI_DEFAULT, 0, (void *)&dupHandle);
    
    return;
}
