/**
 * @file
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  部分公用接口（从xpack剥离出）
 * @author
 * @date   2021/10/10
 * @note
 * @note \n History
   1.日    期: 2021/10/3
     作    者:
     修改历史: 创建文件
 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "plat_com_inter_hal.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#line __LINE__ "plat_com_inter_hal.c"


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */
void fill_pure_color(U08 *pu8Buf1, U32 width, U32 height, U32 stride, U32 start_line, U32 end_line, U32 u32BgValue, SAL_VideoDataFormat enFmt);

/*****************************************************************************
                            宏定义
*****************************************************************************/

#define COMMON_CHECK_PTR_NULL(ptr) if (ptr == NULL) {return SAL_FAIL; }


/*****************************************************************************
                            全局结构体
*****************************************************************************/


/**
 * @function    SAL_halGetPixelBpp
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 SAL_halGetPixelBpp(SAL_VideoDataFormat format, UINT32 *bpp)
{
    COMMON_CHECK_PTR_NULL(bpp);

    switch (format)
    {
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        {
            *bpp = 1;
            break;
        }
        case SAL_VIDEO_DATFMT_ARGB_8888:
        {
            *bpp = 4;
            break;
        }
        case SAL_VIDEO_DATFMT_BAYER_RAW_DUAL:
        case SAL_VIDEO_DATFMT_BAYER_RAW_SINGLE:
        case SAL_VIDEO_DATFMT_BAYER_FMT_BYTE:
        {
            *bpp = 1;
            break;
        }
        case SAL_VIDEO_DATFMT_RGB24_888:
        {
            *bpp = 3;
            break;
        }
        default:
        {
            SAL_LOGE("err foramt %d\n", format);
            *bpp = 0;
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function    SAL_halGetPixelMemSize
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 SAL_halGetPixelMemSize(UINT32 u32Width, UINT32 u32Height, SAL_VideoDataFormat format, UINT32 *u32Size)
{
    COMMON_CHECK_PTR_NULL(u32Size);

    switch (format)
    {
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        {
            *u32Size = u32Width * u32Height * 1.5;
            break;
        }
        case SAL_VIDEO_DATFMT_RGBA_8888:
        case SAL_VIDEO_DATFMT_BGRA_8888:
        case SAL_VIDEO_DATFMT_ARGB_8888:
        {
            /* rk 的jpeg编码要求图像内存宽高即虚宽虚高,16字节对齐，所以申请内存的时候以虚宽虚高来计算内存 */
            *u32Size = SAL_align(u32Width, 16) * SAL_align(u32Height, 16) * 4;
            break;
        }
        case SAL_VIDEO_DATFMT_RGB24_888:
        {
            /* rk 的jpeg编码要求图像内存宽高即虚宽虚高,16字节对齐，所以申请内存的时候以虚宽虚高来计算内存 */
            *u32Size = SAL_align(u32Width, 16) * SAL_align(u32Height, 16) * 3;
            break;
        }

        case SAL_VIDEO_DATFMT_BAYER_RAW_DUAL:
        {
            *u32Size = u32Width * u32Height * 5;
            break;
        }
        case SAL_VIDEO_DATFMT_BAYER_RAW_SINGLE:
        {
            *u32Size = u32Width * u32Height * 2;
            break;
        }
        case SAL_VIDEO_DATFMT_BAYER_FMT_BYTE:
        {
            *u32Size = u32Width * u32Height * 1;
            break;
        }
        default:
        {
            SAL_LOGE("err foramt %d\n", format);
            *u32Size = 0;
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}


/**
 * @function    SAL_halSetSrcFramePrm
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 SAL_halSetSrcFramePrm(SAL_VideoFrameBuf *pstVideoFrame, UINT32 u32TimeRef, UINT64 framePts)
{
    if (NULL == pstVideoFrame)
    {
        SAL_LOGE("error!\n");
        return SAL_FAIL;
    }

    pstVideoFrame->frameNum = u32TimeRef;
    pstVideoFrame->pts = framePts;

    return SAL_SOK;
}


/**
 * @function    Xpack_DrvSoftCopyYuv
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 SAL_halSoftCopyYuv(XPACK_IMG *pstSrc, XPACK_IMG *pstDst)
{
    INT32 j = 0;
    CHAR *srcY = NULL;
    CHAR *srcVU = NULL;
    CHAR *dstY = NULL;
    CHAR *dstVU = NULL;

    if (pstSrc == NULL || pstDst == NULL)
    {
        SAL_LOGE("input prm is NULL \n");
        return SAL_FALSE;
    }

    if (pstSrc->picPrm.enPixFmt == SAL_VIDEO_DATFMT_YUV420SP_VU || pstSrc->picPrm.enPixFmt == SAL_VIDEO_DATFMT_YUV420SP_UV)
    {
        srcY = pstSrc->picPrm.VirAddr[0];
        srcVU = pstSrc->picPrm.VirAddr[1];
        dstY = pstDst->picPrm.VirAddr[0];
        dstVU = pstDst->picPrm.VirAddr[1];
        /* SAL_LOGE("srcy %p dsty %p w %d h %d s %d %d\n",srcY,dstY,pstSrc->picPrm.u32Width,pstSrc->picPrm.u32Height,pstSrc->picPrm.u32Stride,pstDst->picPrm.u32Stride); */
        for (j = 0; j < pstSrc->picPrm.u32Height; j++)
        {
            memcpy(dstY, srcY, pstSrc->picPrm.u32Width);
            srcY += pstSrc->picPrm.u32Stride;
            dstY += pstDst->picPrm.u32Stride;
            if (j % 2 == 0)
            {
                if (pstSrc->bNeedGray)
                {
                    memset(dstVU, 0x80, pstSrc->picPrm.u32Width);
                }
                else
                {
                    memcpy(dstVU, srcVU, pstSrc->picPrm.u32Width);
                }

                srcVU += pstSrc->picPrm.u32Stride;
                dstVU += pstDst->picPrm.u32Stride;
            }
        }
    }
    else
    {
        SAL_LOGE("err format %d\n", pstSrc->picPrm.enPixFmt);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    Xpack_DrvSoftCopyRaw
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 SAL_halSoftCopyRaw(XPACK_IMG *pstSrc, XPACK_IMG *pstDst)
{
    INT32 j = 0;
    UINT32 Bpp;
    CHAR *srcData = NULL;
    CHAR *dstData = NULL;

    COMMON_CHECK_PTR_NULL(pstSrc);
    COMMON_CHECK_PTR_NULL(pstDst);

    SAL_halGetPixelBpp(pstSrc->picPrm.enPixFmt, &Bpp);

    srcData = pstSrc->picPrm.VirAddr[0];
    dstData = pstDst->picPrm.VirAddr[0];

    COMMON_CHECK_PTR_NULL(srcData);
    COMMON_CHECK_PTR_NULL(dstData);

    for (j = 0; j < pstSrc->picPrm.u32Height; j++)
    {
        memcpy(dstData, srcData, pstSrc->picPrm.u32Width * Bpp);
        srcData += pstSrc->picPrm.u32Stride;
        dstData += pstDst->picPrm.u32Stride;
    }

    return SAL_SOK;
}

/**
 * @function    Xpack_DrvSoftCopyRgb
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 SAL_halSoftCopyRgb(XPACK_IMG *pstSrc, XPACK_IMG *pstDst)
{
    INT32 j = 0;
    UINT32 Bpp;
    CHAR *srcRGB = NULL;
    CHAR *dstRGB = NULL;

    COMMON_CHECK_PTR_NULL(pstSrc);
    COMMON_CHECK_PTR_NULL(pstDst);

    SAL_halGetPixelBpp(pstSrc->picPrm.enPixFmt, &Bpp);

    srcRGB = pstSrc->picPrm.VirAddr[0];
    dstRGB = pstDst->picPrm.VirAddr[0];

    COMMON_CHECK_PTR_NULL(srcRGB);
    COMMON_CHECK_PTR_NULL(dstRGB);

    for (j = 0; j < pstSrc->picPrm.u32Height; j++)
    {
        memcpy(dstRGB, srcRGB, pstSrc->picPrm.u32Width * Bpp);
        srcRGB += pstSrc->picPrm.u32Stride * Bpp;
        dstRGB += pstDst->picPrm.u32Stride * Bpp;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Xpack_drvDataCapy
* 描  述  :   xpack数据拷贝
* 输  入  : 无
* 输  出  : 无
* 返回值  : 成功:SAL_SOK
*                     : 失败:SAL_FAIL
*******************************************************************************/
INT32 SAL_halDataCopy(XPACK_IMG *pstSrc, XPACK_IMG *pstDst)
{
    INT32 ret = SAL_SOK;
            
    COMMON_CHECK_PTR_NULL(pstSrc);
    COMMON_CHECK_PTR_NULL(pstDst);

    if (pstSrc->picPrm.enPixFmt != pstDst->picPrm.enPixFmt)
    {
        SAL_LOGE("NORMAL not support enPixFmt diff enPixFmt[%d %d] \n",
            pstSrc->picPrm.enPixFmt, pstDst->picPrm.enPixFmt);
        return SAL_FALSE;
    }

    switch (pstSrc->picPrm.enPixFmt)
    {
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        case SAL_VIDEO_DATFMT_YUV420P:
        case SAL_VIDEO_DATFMT_YUV422SP_UV:
        case SAL_VIDEO_DATFMT_YUV422SP_VU:
        case SAL_VIDEO_DATFMT_YUV422P:
        {
            pstSrc->picPrm.VirAddr[1] += pstSrc->u32CopyYuvSkip;
            ret = SAL_halSoftCopyYuv(pstSrc, pstDst);
            if (ret != SAL_SOK)
            {
                SAL_LOGE("soft copy yuv failed\n ");
            }

            break;
        }

        case SAL_VIDEO_DATFMT_BAYER_RAW_DUAL:
        case SAL_VIDEO_DATFMT_BAYER_RAW_SINGLE:
        case SAL_VIDEO_DATFMT_BAYER_FMT_BYTE:
        {
            ret = SAL_halSoftCopyRaw(pstSrc, pstDst);
            if (ret != SAL_SOK)
            {
                SAL_LOGE("soft copy Raw failed\n ");
            }

            break;
        }

        case SAL_VIDEO_DATFMT_RGB24_888:
        case SAL_VIDEO_DATFMT_RGB16_565:
        case SAL_VIDEO_DATFMT_ARGB_8888:
        {
            ret = SAL_halSoftCopyRgb(pstSrc, pstDst);
            if (ret != SAL_SOK)
            {
                SAL_LOGE("soft copy rgb failed\n ");
            }

            break;
        }

        default:
        {
            SAL_LOGE("NORMAL not support enPixFmt enPixFmt%d SOFT_COPY\n", pstSrc->picPrm.enPixFmt);
            break; 
        }
    }
        
    return ret;

}

/**
 * @function    SAL_halMakeFrame
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 SAL_halMakeFrame(void *virAddr, PhysAddr phyAddr, UINT32 pool, UINT32 w, UINT32 h, UINT32 s, INT32 pixelFormat, SAL_VideoFrameBuf *pstVideoFrame)
{
    UINT32 u32LumaSize = 0;
    UINT32 u32ChrmSize = 0;
    //UINT32 Bpp;

    COMMON_CHECK_PTR_NULL(pstVideoFrame);

    if (NULL == virAddr || 0 == phyAddr)
    {
        SAL_LOGE("error! %p,%p\n", virAddr, (void *)phyAddr);
        return SAL_FAIL;
    }

    switch (pixelFormat)
    {
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        {
            u32LumaSize = (s * h);
            u32ChrmSize = (s * h / 4);
            pstVideoFrame->stride[0] = s;
            pstVideoFrame->virAddr[0] = (PhysAddr)virAddr;
            pstVideoFrame->phyAddr[0] = phyAddr;
            pstVideoFrame->stride[1] = s;
            pstVideoFrame->virAddr[1] = pstVideoFrame->virAddr[0] + u32LumaSize;
            pstVideoFrame->phyAddr[1] = pstVideoFrame->phyAddr[0] + u32LumaSize;
            pstVideoFrame->stride[2] = s;
            pstVideoFrame->virAddr[2] = pstVideoFrame->virAddr[1] + u32ChrmSize;
            pstVideoFrame->phyAddr[2] = pstVideoFrame->phyAddr[0] + u32ChrmSize;
            break;
        }
        case SAL_VIDEO_DATFMT_ARGB_8888:
        case SAL_VIDEO_DATFMT_BGRA_8888:
        case SAL_VIDEO_DATFMT_RGB24_888:
        {
            /* SAL_halGetPixelBpp(pixelFormat, &Bpp); */
            pstVideoFrame->stride[0] = s; /* RK3588里虚高是以像素为单位，不以内存BYTE为单位，所以计算跨距时不需要乘bpp */
            pstVideoFrame->virAddr[0] = (PhysAddr)virAddr;
            pstVideoFrame->phyAddr[0] = phyAddr;
            break;
        }
        default:
        {
            SAL_LOGE("err pixel format %d\n", pixelFormat);
            return SAL_FAIL;
        }
    }

    pstVideoFrame->poolId = pool;
    pstVideoFrame->frameParam.width = w;
    pstVideoFrame->frameParam.height = h;
    pstVideoFrame->frameParam.dataFormat = pixelFormat;


    return SAL_SOK;
}


/**
 * @fn      Sal_halBuildSysFrame
 * @brief   构建System Frame信息
 * @param   [OUT] pstSysFrame       System Frame信息
 * @param   [OUT] pstVidFrame       Video Frame信息，可为NULL，NULL则不输出
 * @param   [OUT]      poolId       内存池Id
 * @param   [OUT]      vbBlk        帧缓存块地址
 * @param   [OUT]      pts          时间戳信息
 * @param   [OUT]      frameNum     帧序号信息
 * @param   [OUT]      virAddr      视频帧起始虚拟地址
 * @param   [OUT]      phyAddr      视频帧起始物理地址
 * @param   [OUT]      stride       视频帧内存跨距，单位：pixel
 * @param   [OUT]      vertStride   视频帧内存竖向跨距，不能小于抠图高+视频帧高，单位：pixel
 * @param   [OUT]      frameParam   视频帧尺寸信息
 * @param   [OUT]           width       视频帧宽度信息
 * @param   [OUT]           height      视频帧高度信息
 * @param   [OUT]           dataFormat  视频帧数据格式
 * @param   [OUT]           crop        视频帧内存抠取信息
 * @param   [OUT]               left        实际视频帧数据起点距离内存块起始的水平偏移，单位：byte
 * @param   [OUT]               top         实际视频帧数据起点距离内存块起始的竖直偏移，单位：byte
 * @param   [OUT]               width       抠取宽，等于实际视频帧宽，单位：pixel，不使用
 * @param   [OUT]               height      抠取高，等于实际视频帧高，单位：pixel，不使用
 * @param   [IN]  *pstImgData       实际视频帧数据
 * @param   [IN]        stMbAttr        视频帧使用的内存块信息
 * @param   [IN]  pstPriv           私有信息，可为NULL，当为NULL时即无私有信息 
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS Sal_halBuildSysFrame(SYSTEM_FRAME_INFO *pstSysFrame, SAL_VideoFrameBuf *pstVidFrame, XIMAGE_DATA_ST *pstImgData, SAL_SYSFRAME_PRIVATE *pstPriv)
{
    UINT32 i = 0, bpp = 1, u32CropTop = 0;
    SAL_VideoFrameBuf stVidFrame = {0};
    MEDIA_BUF_ATTR *pstMbAttr = NULL;
    SAL_VideoFrameBuf *pstVidFrameTmp = NULL;

    if (NULL == pstSysFrame || NULL == pstImgData)
    {
        XPACK_LOGE("pstSysFrame(%p) OR pstMbAttr(%p) is NULL\n", pstSysFrame, pstMbAttr);
        return SAL_FAIL;
    }
    if (NULL != pstVidFrame)
    {
        pstVidFrameTmp = pstVidFrame;
    }
    else
    {
        pstVidFrameTmp = &stVidFrame;
    }

    if (0 == pstSysFrame->uiAppData)
    {
        if (SAL_SOK != sys_hal_allocVideoFrameInfoSt(pstSysFrame))
        {
            XPACK_LOGE("sys_hal_allocVideoFrameInfoSt failed\n");
            return SAL_FAIL;
        }
    }

    memset(pstVidFrameTmp, 0x0, sizeof(SAL_VideoFrameBuf));

    pstMbAttr = &pstImgData->stMbAttr;
    pstVidFrameTmp->poolId = pstMbAttr->poolId;
    pstVidFrameTmp->vbBlk = pstMbAttr->u64MbBlk;
    pstVidFrameTmp->frameParam.width = pstImgData->u32Width;
    pstVidFrameTmp->frameParam.height = pstImgData->u32Height;
    pstVidFrameTmp->frameParam.dataFormat = SAL_dspDataFormatToSal(pstImgData->enImgFmt);

    if (pstImgData->pPlaneVir[0] > pstMbAttr->virAddr && 0 != pstMbAttr->virAddr)
    {
        SAL_halGetPixelBpp(pstVidFrameTmp->frameParam.dataFormat, &bpp);
        u32CropTop = (pstImgData->pPlaneVir[0] - pstMbAttr->virAddr) / (pstImgData->u32Stride[0] * bpp);
        pstVidFrameTmp->frameParam.crop.left = (pstImgData->pPlaneVir[0] - pstMbAttr->virAddr) % (pstImgData->u32Stride[0] * bpp);     // 构建视频帧时以内存为偏移量
        pstVidFrameTmp->frameParam.crop.top = u32CropTop * bpp;       // 构建视频帧时以内存为偏移量
        pstVidFrameTmp->frameParam.crop.width = pstImgData->u32Width;
        pstVidFrameTmp->frameParam.crop.height = pstImgData->u32Height;
    }

    if (NULL != pstPriv)
    {
        pstVidFrameTmp->pts = pstPriv->u64Pts;
        pstVidFrameTmp->frameNum = pstPriv->u32FrameNum;
    }

    for (i = 0; i < 4; i++)
    {
        pstVidFrameTmp->virAddr[i] = (PhysAddr)(pstImgData->pPlaneVir[i]);
        pstVidFrameTmp->phyAddr[i] = pstMbAttr->phyAddr + (pstImgData->pPlaneVir[i] - pstMbAttr->virAddr);
        pstVidFrameTmp->stride[i] = pstImgData->u32Stride[i];
        pstVidFrameTmp->vertStride[i] = ximg_get_height(pstImgData); /* yuv420格式uv的高为未除2 */
    }

    return sys_hal_buildVideoFrame(pstVidFrameTmp, pstSysFrame);
}

