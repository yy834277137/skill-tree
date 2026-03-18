/**
 * @file   dup_hal_inter.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief
 * @author
 * @date
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#ifndef _DUP_HAL_INTER_H_

#define _DUP_HAL_INTER_H_

#include "sal.h"

#include "platform_hal.h"



//#include "hal_inc_exter/dup_common_api.h"

typedef struct _DUP_PLAT_OPS
{
    INT32 (*init)();
    INT32 (*deinit)();
    INT32 (*create)(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm, DUP_HAL_CHN_INIT_ATTR *pstChnInitAttr);
    INT32 (*start)(UINT32 u32GrpId, UINT32 u32Chn);
    INT32 (*stop)(UINT32 u32GrpId, UINT32 u32Chn);
    INT32 (*destroy)(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm);
    INT32 (*setParam)(UINT32 dupChan, UINT32 chn, PARAM_INFO_S *pParamInfo);
    INT32 (*getParam)(UINT32 dupChan, UINT32 chn, PARAM_INFO_S *pParamInfo);
    INT32 (*getFrame)(UINT32 grpId, UINT32 chn, SYSTEM_FRAME_INFO *pstFrame, UINT32 u32TimeoutMs);
    INT32 (*releaseFrame)(UINT32 grpId, UINT32 chn, SYSTEM_FRAME_INFO *pstFrame);
    INT32 (*sendFrame)(UINT32 dupChan, SYSTEM_FRAME_INFO *pstFrame, UINT32 u32TimeoutMs);
    INT32 (*getPhyChnNum)(void);
    INT32 (*getExtChnNum)(void);
    INT32 (*scaleYuvRange)(SAL_VideoSize *pstSrcSize, SAL_VideoSize *pstDstSize, 
                           SAL_VideoCrop *pstSrcCropPrm, SAL_VideoCrop *pstDstCropPrm, 
                           CHAR *pu8Src, CHAR *pu8Dst, BOOL bIfMirror);
} DUP_PLAT_OPS;

const DUP_PLAT_OPS *dup_hal_register(void);


#endif /* _DUP_HAL_INTER_H_ */


