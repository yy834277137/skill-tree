/**
 * @file   mem_hal_inter.h
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
#ifndef _VGS_HAL_INTER_H_

#define _VGS_HAL_INTER_H_

#include "sal.h"
#include "platform_hal.h"



typedef struct _VGS_PLAT_OPS
{

    /*****************************************************************************
     函 数 名  : drawLineOSDArray
     功能描述  : 使用 VGS 画osd和框
     输入参数  : stTask vgs画osd任务参数
                              pstOsdPrm 画osd的信息参数
                              pstLinePrm画框参数
     输出参数  : 无
     返 回 值  : 成功SAL_SOK
                          失败SAL_FAIL
    *****************************************************************************/
    INT32 (*drawLineOSDArray)(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm, VGS_DRAW_LINE_PRM *pstLinePrm);
    
    /*****************************************************************************
     函 数 名  : drawOsdSingle
     功能描述  : 使用 VGS 画osd
     输入参数  : stTask vgs画osd任务参数
                              pstOsdPrm 画osd的信息参数
     输出参数  : 无
     返 回 值  : 成功SAL_SOK
                          失败SAL_FAIL
    *****************************************************************************/
    INT32 (*drawOsdSingle)(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm);

    /*****************************************************************************
     函 数 名  : drawOSDArray
     功能描述  : 使用 VGS 画osd
     输入参数  : stTask vgs画osd任务参数
                              pstOsdPrm 画osd的信息参数
     输出参数  : 无
     返 回 值  : 成功SAL_SOK
                          失败SAL_FAIL
    *****************************************************************************/
    INT32 (*drawOSDArray)(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm);

    /*****************************************************************************
     函 数 名  : drawLineArray
     功能描述  : 使用 VGS 画多条线
     输入参数  : stTask vgs画线任务参数
                              pstLinePrm 画线的信息参数
     输出参数  : 无
     返 回 值  : 成功SAL_SOK
                          失败SAL_FAIL
    *****************************************************************************/
    INT32 (*drawLineArray)(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_DRAW_LINE_PRM *pstLinePrm);

    /*****************************************************************************
     函 数 名  : vgs_hisiScaleFrame
     功能描述  : 使用 VGS 缩放
     输入参数  : stTask vgs缩放任务参数
     输出参数  : 无
     返 回 值  : 成功SAL_SOK
                          失败SAL_FAIL
    *****************************************************************************/
    INT32 (*scaleFrame)(SYSTEM_FRAME_INFO *pDstSystemFrame, SYSTEM_FRAME_INFO *pSrcSystemFrame, UINT32 dstW, UINT32 dstH);

} VGS_PLAT_OPS;

/*******************************************************************************
* 函数名  : disp_halRegister
* 描  述  : 注册hisi disp显示函数
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
VGS_PLAT_OPS *vgs_halRegister(void);


#endif /* _MEM_HAL_INTER_H_ */


