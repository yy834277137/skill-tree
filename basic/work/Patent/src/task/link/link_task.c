/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   FileName: link_task.c
   Description: 不同产品在能力集模块下link_cfg.c里进行配置，本文件接口根据配置信息进行绑定，
   从而搭建起产品特有的数据流
   Author: yeyanzhong
   Date: 2020.12.10
   Modification History:
******************************************************************************/
#include "sal.h"
#include "link_task_api.h"
#include "link_cfg_api.h"

/******************************************************************
   Function:   link_taskBindMods
   Description: 对系统中的模块根据配置信息BIND_S进行绑定
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_taskBindMods()
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    UINT32 u32Cnt = 1;
    BIND_S *pstBinderInfo  =  NULL;
    BIND_S *pstBinder  =  NULL;
    INST_INFO_S *pstInstSrc = NULL;
    INST_INFO_S *pstInstDst = NULL;

    s32Ret = link_cfg_getBinderInfo(&pstBinderInfo, &u32Cnt);
    if (s32Ret)
    {
        LINK_LOGE("fail. s32Ret %d", s32Ret);
        return s32Ret;
    }

    for (i = 0; i < u32Cnt; i++)
    {
        pstBinder  =  &pstBinderInfo[i];

        pstInstSrc = link_drvGetInst(pstBinder->szSrcInst);
        pstInstDst = link_drvGetInst(pstBinder->szDstInst);
        if (!pstInstSrc || !pstInstDst)
        {
            LINK_LOGE("pstInstSrc %p, pstInstDst %p \n", pstInstSrc, pstInstDst);
            return SAL_FAIL;
        }
        s32Ret = link_drvBind(pstInstSrc, pstBinder->szSrcNode, pstInstDst, pstBinder->szDstNode);
        if (s32Ret)
        {
            LINK_LOGE("fail. s32Ret %d", s32Ret);
            return s32Ret;
        }
    }
    return s32Ret;
}

