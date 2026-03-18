/**
 * @file   rgn_hisi.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  rgn---OSD处理接口封装
 * @author 
 * @date   2022年02月12日 Create
 * @note
 * @note \n History
 */

#include "platform_sdk.h"
#include "platform_hal.h"
#include "osg_nt.h"
#include "osd_hal_inter.h"

typedef enum _HD_RGN_ALPHA_TYPE {
    ALPHA_0 = 0, ///< alpha 0%
    ALPHA_25,    ///< alpha 25%
    ALPHA_37_5,  ///< alpha 37.5%
    ALPHA_50,    ///< alpha 50%
    ALPHA_62_5,  ///< alpha 62.5%
    ALPHA_75,    ///< alpha 75%
    ALPHA_87_5,  ///< alpha 87.5%
    ALPHA_100,    ///< alpha 100%
    RGN_ALPHA_TYPE_MAX, ENUM_DUMMY4WORD(HD_RGN_ALPHA_TYPE)
} HD_RGN_ALPHA_TYPE;


/**
 * @function   osg_nt_create
 * @brief    创建OSD句柄
 * @param[in]  UINT32 u32Idx rgn id
 * @param[in]  unsigned char *pInBuf 输入buf，外部申请
 * @param[out] void **ppHandle hal层OSD句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osg_nt_create(UINT32 u32Start, UINT32 u32Idx, void **pHandle, unsigned char *pInBuf)
{
    OSG_CHN_S *pstOsg = NULL;

    if ((NULL == pHandle) || (NULL == pInBuf))
    {
        OSD_LOGE("inv prm!!!\n");
        return SAL_FAIL;
    }

    memset(pInBuf, 0, sizeof(OSG_CHN_S));

    pstOsg = (OSG_CHN_S *)pInBuf;
    pstOsg->bCreated = SAL_FALSE;
    pstOsg->u32Idx = u32Idx - u32Start;

    *pHandle = pInBuf;

    //OSD_LOGI("start:%u idx:%u create success\n", u32Start, u32Idx);

    return SAL_SOK;
}


/**
 * @function   osg_nt_destroy
 * @brief      销毁OSD句柄
 * @param[in]  UINT32 chan venc hal层通道号
 * @param[in]  void *pHandle OSD句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osg_nt_destroy(UINT32 u32EncChan, void *pHandle)
{
    OSG_CHN_S *pstOsg = NULL;
    HD_RESULT enRet = HD_OK;

    if (NULL == pHandle)
    {
        OSD_LOGE("u32EncChan %d, pHandle is null\n", u32EncChan);
        return SAL_FAIL;
    }

    pstOsg = (OSG_CHN_S *)pHandle;
    if (SAL_FALSE == pstOsg->bCreated)
    {
        OSD_LOGW("u32EncChan %d,not create\n", u32EncChan);
        return SAL_SOK;
    }

    if (pstOsg->id > 0)
    {
        enRet = hd_videoenc_stop(pstOsg->id);
        if (HD_OK != enRet)
        {
            OSD_LOGE("0x%x hd_videoenc_stop fail:%d\n", pstOsg->id, enRet);
            return SAL_FAIL;
        }

        enRet = hd_videoenc_close(pstOsg->id);
        if (HD_OK != enRet)
        {
            OSD_LOGW("hd_videoenc_close fail:%d\n", enRet);
            return SAL_FAIL;
        }
        pstOsg->id = 0;
    }
    pstOsg->bCreated = SAL_FALSE;

    return SAL_SOK;
}

/**
 * @function   osg_nt_process
 * @brief      osd叠加处理
 * @param[in]  UINT32 chan venc hal层通道号
 * @param[in]   void *pHandle hal层OSD句柄
 * @param[in]   PUINT8 pCharImgBuf 输入buf
 * @param[in]   UINT32 u32DstW, UINT32 u32DstH, UINT32 u32DstX, UINT32 u32DstY OSD区域信息
 * @param[in]   UINT32 u32Color OSD颜色
 * @param[in]   UINT32 bTranslucent 是否半透明
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osg_nt_process(UINT32 u32Chan, void *pHandle, PUINT8 pCharImgBuf, UINT64 u64PhyAddr,
                       UINT32 u32DstW, UINT32 u32DstH, UINT32 u32DstX, UINT32 u32DstY, UINT32 u32Color, UINT32 bTranslucent)
{
    INT32 enRet = HD_OK;
    OSG_CHN_S *pstOsg = NULL;
    HD_OSG_STAMP_IMG stStampImg;
    HD_OSG_STAMP_ATTR stStampAttr;
    VENDOR_VIDEOENC_OSG stOsgSel = {0};

    if ((NULL == pHandle) || (NULL == pCharImgBuf))
    {
        OSD_LOGE("prm err\n");
        return SAL_FAIL;
    }

    memset(&stStampImg, 0, sizeof(stStampImg));
    memset(&stStampAttr, 0, sizeof(stStampAttr));

    pstOsg = (OSG_CHN_S *)pHandle;

    if (0 == pstOsg->id)
    {
        enRet = hd_videoenc_open(HD_VIDEOENC_IN(0, HD_STAMP(pstOsg->u32Idx)), HD_VIDEOENC_OUT(0, u32Chan), &pstOsg->id);
        if (HD_OK != enRet)
        {
            OSD_LOGE("%u %u:hd_videoenc_open stamp fail:%d\n", u32Chan, pstOsg->u32Idx, enRet);
            return SAL_FAIL;
        }
        pstOsg->bCreated = SAL_TRUE;
    }

#if 0
    if (SAL_TRUE == pstOsg->bStarted)
    {
        enRet = hd_videoenc_stop(pstOsg->id);
        if (HD_OK != enRet)
        {
            OSD_LOGE("%u %u:hd_videoenc_stop stamp fail:%d\n", u32Chan, pstOsg->u32Idx, enRet);
            return SAL_FAIL;
        }
        pstOsg->bStarted = SAL_FALSE;
    }
#endif

    stStampImg.p_addr = (UINTPTR)u64PhyAddr;
    stStampImg.ddr_id = u64PhyAddr > 0x100000000 ? DDR_ID1 : DDR_ID0;
    stStampImg.dim.w  = u32DstW;
    stStampImg.dim.h  = u32DstH;
    stStampImg.fmt    = HD_VIDEO_PXLFMT_ARGB1555;
    enRet = hd_videoenc_set(pstOsg->id, HD_VIDEOENC_PARAM_IN_STAMP_IMG, (VOID *)&stStampImg);
    if (HD_OK != enRet)
    {
        OSD_LOGE("set stamp img fail:%d\n", enRet);
        return SAL_FAIL;
    }

    stStampAttr.align_type = HD_OSG_ALIGN_TYPE_TOP_LEFT;
    stStampAttr.alpha      = ALPHA_100;
    stStampAttr.position.x = u32DstX;
    stStampAttr.position.y = u32DstY;
    enRet = hd_videoenc_set(pstOsg->id, HD_VIDEOENC_PARAM_IN_STAMP_ATTR, (VOID *)&stStampAttr);
    if (HD_OK != enRet)
    {
        OSD_LOGE("set stamp attr fail:%d\n", enRet);
        return SAL_FAIL;
    }

    stOsgSel.external_osg = 0;
    enRet = vendor_videoenc_set(pstOsg->id, VENDOR_VIDEOENC_PARAM_OSG_SEL, &stOsgSel);
    if (HD_OK != enRet) {
        OSD_LOGE("Fail to set using external osg\n");
        return SAL_FAIL;
    }

    if (SAL_TRUE != pstOsg->bStarted)
    {
        enRet = hd_videoenc_start(pstOsg->id);
        if (HD_OK != enRet)
        {
            OSD_LOGE("%u %u:hd_videoenc_start stamp fail:%d\n", u32Chan, pstOsg->u32Idx, enRet);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}


/**
 * @function   osg_nt_getMemSize
 * @brief    获取osd hdl所需内存大小
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
UINT32 osg_nt_getMemSize(void)
{
    return sizeof(OSG_CHN_S);
}


/**
 * @function   osd_hal_register
 * @brief      注册hal层回调函数
 * @param[in]  None
 * @param[out] OSD_PLAT_OPS_S * 回调函数结构指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_hal_register(OSD_PLAT_OPS_S *pstOsdPlatOps)
{
    if (NULL == pstOsdPlatOps)
    {
        return SAL_FAIL;
    }

    /* 注册hal层处理函数 */
    pstOsdPlatOps->GetMemSize = osg_nt_getMemSize;
    pstOsdPlatOps->Create = osg_nt_create;
    pstOsdPlatOps->Destroy = osg_nt_destroy;
    pstOsdPlatOps->Process = osg_nt_process;

    return SAL_SOK;
}

