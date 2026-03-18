/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   FileName: vdec_hal_inter.h
   Description:
   Author:
   Date:
   Modification History:
******************************************************************************/
#ifndef _VDEC_HAL_INTER_H_

#define _VDEC_HAL_INTER_H_

typedef struct tagVdechalfunc
{
    INT32 (*MemAlloc)(UINT32 u32Format, void *prm);
    INT32 (*MemFree)(void *ptr, void *prm);
    INT32 (*VdecCreate)(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm);
    INT32 (*VdecStart)(void *handle);
    INT32 (*VdecStop)(void *handle);
    INT32 (*VdecDeinit)(void *handle);
    INT32 (*VdecSetPrm)(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm);
    INT32 (*VdecReset)(void *handle);
    INT32 (*VdecClear)(void *handle);
    INT32 (*VdecDecframe)(void *handle, SAL_VideoFrameBuf *pstSrcframe, SAL_VideoFrameBuf *pstDstframe);
    INT32 (*VdecMakeframe)(PIC_FRAME_PRM *pPicFramePrm, void *pstframe);
    INT32 (*VdecCpyframe)(VDEC_PIC_COPY_EN *copyEn, void *pstSrcframe, void *pstDstframe);
    INT32 (*VdecGetframe)(void *handle, UINT32 dstChn, void *pstframe);
    INT32 (*VdecReleaseframe)(void *handle, UINT32 dstChn, void *pstframe);
    INT32 (*VdecMallocYUVMem)(UINT32 u32width, UINT32 u32height, HAL_MEM_PRM *pOutFrm);
    INT32 (*VdecFreeYuvPoolBlockMem)(HAL_MEM_PRM *addrPrm);
    INT32 (*VdecMmap)(void *pVideoFrame, PIC_FRAME_PRM *pPicprm);
} VDEC_HAL_FUNC;

/*******************************************************************************
* 函数名  : vdec_halRegister
* 描  述  : 注册hisi vdec函数
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
VDEC_HAL_FUNC *vdec_halRegister(void);



#endif /* _VDEC_HAL_INTER_H_ */


