/*******************************************************************************
* face_hal.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : zongkai5 <zongkai5@hikvision.com>
* Version: V2.0.0  2021年10月30日 Create
*
* Description :
* Modification:
*******************************************************************************/

/* ========================================================================== */
/*                          头文件区									   */
/* ========================================================================== */
#include <platform_hal.h>
/* #include "hcnn_scheduler.h" */
#include "encrypt_proc.h"
#include <arm_neon.h>
#include "face_hal.h"

/* ========================================================================== */
/*                          宏定义区									   */
/* ========================================================================== */
/*人脸比对库*/
#define CMPMDL_PATH "./face/models/Compare_v5.0.0_v1_GPU_P4_INT8_gen20200608.bin"

#define FACE_HAL_CHECK_CHAN(chan, loop, str)  \
    { \
        if (chan > FACE_MAX_CHAN_NUM - 1) \
        { \
            SAL_ERROR("%s \n", str); \
            goto loop; \
        } \
    }

#define FACE_HAL_CHECK_PTR(ptr, loop, str) \
    { \
        if (!ptr) \
        { \
            SAL_ERROR("%s \n", str); \
            goto loop; \
        } \
    }

#define FACE_HAL_CHECK_RET(ret, loop, str) \
    { \
        if (ret) \
        { \
            SAL_ERROR("%s, ret: 0x%x \n", str, ret); \
            goto loop; \
        } \
    }

#define FACE_HAL_CHECK_RET_NO_LOOP(ret, str) \
    { \
        if (ret) \
        { \
            SAL_ERROR("%s, ret: 0x%x \n", str, ret); \
        } \
    }


/* ========================================================================== */
/*                          数据结构区									   */
/* ========================================================================== */
/* 算法资源管理全局变量 */
static FACE_COMMON_PARAM g_stFaceCommonPrm = {0};
/* 人脸特征数据库 */
static FACE_MODEL_DATA_BASE g_stModelDataBase = {0};
/* ========================================================================== */
/*                          函数定义区									   */
/* ========================================================================== */

/* 图片建模业务线，dfr建模结果回调处理接口，对应于graph4 */
extern INT32 Face_DrvGetOutputResult0(int nNodeID,
                                      int nCallBackType,
                                      void *pstOutput,
                                      unsigned int nSize,
                                      void *pUsr,
                                      int nUserSize);

/* 人脸登录业务线，选帧结果回调处理接口，对应于graph17 */
extern INT32 Face_DrvGetOutputResult1(int nNodeID,
                                      int nCallBackType,
                                      void *pstOutput,
                                      unsigned int nSize,
                                      void *pUsr,
                                      int nUserSize);

/* 人脸登录业务线，dfr建模结果回调处理接口，对应于graph5 */
extern INT32 Face_DrvGetOutputResult2(int nNodeID,
                                      int nCallBackType,
                                      void *pstOutput,
                                      unsigned int nSize,
                                      void *pUsr,
                                      int nUserSize);

/* 人脸抓拍业务线，选帧结果回调处理接口，对应于graph7 */
extern INT32 Face_DrvGetOutputResult3(int nNodeID,
                                      int nCallBackType,
                                      void *pstOutput,
                                      unsigned int nSize,
                                      void *pUsr,
                                      int nUserSize);

/* 人脸抓拍业务线，dfr建模结果回调处理接口，对应于graph3 */
extern INT32 Face_DrvGetOutputResult4(int nNodeID,
                                      int nCallBackType,
                                      void *pstOutput,
                                      unsigned int nSize,
                                      void *pUsr,
                                      int nUserSize);

/**
 * @function    Face_HalGetJdecBuf
 * @brief         获取jpeg图片解码缓存
 * @param[in]  NULL
 * @param[out] NULL
 * @return  BGRA缓存数据地址
 */
FACE_JDEC_BUF_INFO *Face_HalGetJdecBuf(VOID)
{
    return &g_stFaceCommonPrm.stJdecBufInfo;
}

/**
 * @function    Face_HalGetComPrm
 * @brief         获取算法参数全局变量
 * @param[in]  NULL
 * @param[out] NULL
 * @return 模块通用参数地址
 */
FACE_COMMON_PARAM *Face_HalGetComPrm(VOID)
{
    return &g_stFaceCommonPrm;
}

/**
 * @function    Face_HalGetDataBase
 * @brief         获取人脸数据库
 * @param[in]  NULL
 * @param[out] NULL
 * @return 模型数据地址
 */
FACE_MODEL_DATA_BASE *Face_HalGetDataBase(VOID)
{
    return &g_stModelDataBase;
}

/**
 * @function    Face_HalGetVaceHandle
 * @brief         获取引擎句柄
 * @param[in]  - mode : 识别模式(0为图片注册，1为人脸登录，2为人脸抓拍)
 * @param[out]
 * @return     pInitHandle : 返回引擎句柄
 */
VOID *Face_HalGetVaceHandle(FACE_ANA_MODE_E mode)
{
    return g_stFaceCommonPrm.pInitHandle;
}

/**
 * @function    Face_HalGetICFHandle
 * @brief         获取人脸Handle句柄
 * @param[in]  mode:模式(0:图片建模模式,1:抓拍模式(人脸登录) ,2:人脸抓拍)
 * @param[out]   NULL
 * @return    pengine_hdl :
 */
VOID *Face_HalGetICFHandle(FACE_ANA_MODE_E mode, UINT32 u32NodeIdx)
{
    return g_stFaceCommonPrm.pEngineChnHandle[mode][u32NodeIdx];
}

/**
 * @function    Face_HalgetTimeMilli
 * @brief         获取毫秒
 * @param[in]  NULL
 * @param[out] NULL
 * @return time : 返回当前时间的毫秒表示
 */
UINT64 Face_HalgetTimeMilli(void)
{
    struct timeval tv;
    UINT64 time = 0;

    gettimeofday(&tv, NULL);
    time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    return time;
}

/**
 * @function    Face_HalGetAnaDataTab
 * @brief         获取人脸分析缓存
 * @param[in]  mode : 类型
 * @param[out]  NULL
 * @return       ICF_INPUT_DATA_V2 *: 成功返回抓拍或图片类型人脸Buf空间
 */
FACE_ANA_BUF_INFO *Face_HalGetAnaDataTab(FACE_ANA_MODE_E mode)
{
    if (mode >= FACE_MODE_MAX_NUM)
    {
        FACE_LOGE("Invalid mode %d! Pls Check!\n", mode);
        return NULL;
    }

    return &g_stFaceCommonPrm.astFaceBufData[mode];
}

/**
 * @function    Face_HalGetAnaFreeBuf
 * @brief         获取人脸分析空闲缓存
 * @param[in]  mode : 类型
 * @param[out] 空闲缓存索引pIdx
 * @return    SAL_SOK
 */
UINT32 Face_HalGetAnaFreeBuf(FACE_ANA_MODE_E mode, void *pIdx)
{
    /* Variables Definitions */
    UINT32 i = 0;
    UINT32 uiIdx = 0;
    UINT32 uiTmpIdx = 0;

    UINT32 *pFreeBufId = NULL;
    FACE_ANA_BUF_INFO *pstFaceAnaBufInfo = NULL;

    /* Prm Validation */
    if (mode >= FACE_MODE_MAX_NUM)
    {
        FACE_LOGE("Invalid mode %d! Pls Check!\n", mode);
        return SAL_FAIL;
    }

    FACE_HAL_CHECK_PTR(pIdx, err, "pIndx Err");

    pFreeBufId = (UINT32 *)pIdx;

    pstFaceAnaBufInfo = Face_HalGetAnaDataTab(mode);
    FACE_HAL_CHECK_PTR(pstFaceAnaBufInfo, err, "pstFaceAnaBufInfo err!");

    uiIdx = pstFaceAnaBufInfo->uiBufIdx;

    for (i = 0; i < pstFaceAnaBufInfo->uiMaxBufNum; i++)
    {
        uiTmpIdx = (uiIdx + i) % pstFaceAnaBufInfo->uiMaxBufNum;
        if (SAL_TRUE == *(UINT32 *)pstFaceAnaBufInfo->stFaceBufData[uiTmpIdx].pUseFlag[0])
        {
            continue;
        }

        pstFaceAnaBufInfo->uiBufIdx = uiTmpIdx;                          /* Save current Buf Index */
        *(pstFaceAnaBufInfo->stFaceBufData[uiTmpIdx].pUseFlag[0]) = SAL_TRUE;  /* Marked flag as USING!!! */

        *pFreeBufId = uiTmpIdx;
        break;
    }

    if (pstFaceAnaBufInfo->uiMaxBufNum == i)
    {
        FACE_LOGW("No Free Bufffffffffff! Mode %d\n", mode);
        return SAL_FAIL;
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Face_HalI420ToNv21
 * @brief         视频帧格式转换将I420的格式转成nv21
 * @param[in]   *pYuvFrm       源视频帧分量(软解输出,对应格式是PIXEL_FORMAT_YUV_PLANAR_420)
 * @param[in]   yStride		目标图像的跨度
 * @param[in]   uvStride		目标图像的跨度
 * @param[in]   pDstLumaAddr	目的图像Y分量指针(针对场编码将顶场与底场进行交织） HISI平台视频帧Y
 * @param[in]   pDstChromaAddr 目标图像的UV 分量指针 （VU交替排列，针对场编码的时候顶场与底场交织）HISI平台视频帧UV
 * @param[out] NULL
 * @return  成功返回申请成功的mmz虚拟地址
 */
INT32 Face_HalI420ToNv21(FACE_YUV_FRAME_EX *pYuvFrm,
                         UINT32 yDstStride,
                         UINT32 uvDstStride,
                         PUINT8 pDstLumaAddr,
                         PUINT8 pDstChromaAddr)
{
    UINT32 err = SAL_SOK;
    UINT32 index = 0;
/*    UINT32 index_w = 0; */
    UINT32 i = 0;
/*    UINT32 ylen = 0; */
/*    UINT32 uvlen = 0; */
    PUINT8 pChromaAddr_u = NULL;    /* 输入色度u首地址 */
    PUINT8 pChromaAddr_v = NULL;    /* 输入色度v首地址 */
    PUINT8 plumaTopAddr = NULL;     /* 输入源亮度顶场首地址 */
    PUINT8 plumaBotAddr = NULL;     /* 输入源亮度底场首地址 */
    PUINT8 plumadst_o = NULL;       /* 输出亮度偶数行首地址 */
    PUINT8 plumadst_e = NULL;       /* 输出亮度奇数行首地址 */
    PUINT8 pchromaTopAddr_u = NULL; /* 输入源色度u顶场首地址 */
    PUINT8 pchromaBotAddr_u = NULL; /* 输入源色度u底场首地址 */
    PUINT8 pchromaTopAddr_v = NULL; /* 输入源色度v顶场首地址 */
    PUINT8 pchromaBotAddr_v = NULL; /* 输入源色度v底场首地址 */
/*    PUINT8 pchromadst_o = NULL;     / * 输出亮度偶数行首地址 * / */
/*    PUINT8 pchromadst_e = NULL;     / * 输出亮度奇数行首地址 * / */
    uint8x16_t y_8x16;
    uint8x8_t u_8x8, v_8x8;
/*    uint8x8_t y_8x8_1, y_8x8_2; */
    uint8x16_t y_8x8_11; /* , y_8x8_21; */
    uint8x8x2_t tmp_8x8x2;
    uint8x16_t vu8x16;
/*    uint8x16_t y8x16; */
    UINT32 offset = 0;
    UINT32 width = 0;
    UINT32 height = 0;
    UINT32 srcYStride = 0;
    UINT32 srcUvStride = 0;

    /* 输入参数检查 */
    if ((NULL == pYuvFrm)
        || (0 == yDstStride)
        || (0 == uvDstStride)
        || (NULL == pDstLumaAddr)
        || (NULL == pDstChromaAddr))
    {
        FACE_LOGE("Err, pYuvFrm:%p,yDstStride:%d,uvDstStride:%d,pDstLumaAddr:%p,pDstChromaAddr:%p\n",
                  pYuvFrm, yDstStride, uvDstStride, pDstLumaAddr, pDstChromaAddr);

        return SAL_FAIL;
    }

    /* 待格式转换前的图像大小 */
    width = pYuvFrm->width;
    height = pYuvFrm->height;
    srcYStride = pYuvFrm->pitchY;
    srcUvStride = pYuvFrm->pitchUv;

    /* ylen = width * height; */
    /* uvlen = ylen >> 2; */

    do
    {
        if (INTERLACED_FRAME_MODE == pYuvFrm->frameMode)
        {
            if ((NULL == pYuvFrm->yTopAddr)
                || (NULL == pYuvFrm->uTopAddr)
                || (NULL == pYuvFrm->yBotAddr)
                || (NULL == pYuvFrm->uBotAddr))
            {
                FACE_LOGE("Err, yTopAddr:%p,uTopAddr:%p yBotAddr:%p uBotAddr:%p\n",
                          pYuvFrm->yTopAddr, pYuvFrm->uTopAddr, pYuvFrm->yBotAddr, pYuvFrm->uBotAddr);

                err = SAL_FAIL;
                break;
            }

            /* Y分量 顶场一行底场一行*/
            plumaTopAddr = pYuvFrm->yTopAddr;
            plumaBotAddr = pYuvFrm->yBotAddr;

            for (index = 0; index < height; index += 2)
            {
                plumadst_e = pDstLumaAddr + index * yDstStride;        /* 0 2 4 6 8...行 */
                plumadst_o = pDstLumaAddr + index * yDstStride + yDstStride; /* 1 3 5 7 9...行 */

                for (i = 0; i < width; i += 16)
                {
                    y_8x16 = vld1q_u8(plumaTopAddr);    /* 顶场读入16个像素 */
                    vst1q_u8(plumadst_e, y_8x16);        /* store 16个像素 */

                    y_8x16 = vld1q_u8(plumaBotAddr);   /* 底场读入16个像素 */
                    vst1q_u8(plumadst_o, y_8x16);       /* store 16个像素 */

                    plumaTopAddr += 16;
                    plumaBotAddr += 16;
                    plumadst_e += 16;
                    plumadst_o += 16;
                }
            }

            pchromaTopAddr_u = pYuvFrm->uTopAddr;               /* u 顶场 */
            pchromaTopAddr_v = pYuvFrm->vTopAddr;               /* v 顶场 */
            pchromaBotAddr_u = pYuvFrm->uBotAddr;               /* u 底场 */
            pchromaBotAddr_v = pYuvFrm->vBotAddr;               /* v 底场 */

            for (index = 0; index < (height >> 1); index += 2)
            {
                plumadst_e = pDstChromaAddr + index * uvDstStride;
                /* 取顶场VU交织 */
                for (i = 0; i < width; i += 16)
                {
                    u_8x8 = vld1_u8(pchromaTopAddr_u); /* 顶场u 读入8个像素 */
                    v_8x8 = vld1_u8(pchromaTopAddr_v); /* 顶场v 读入8个像素 */
                    tmp_8x8x2 = vzip_u8(v_8x8, u_8x8);  /* 交织 vu 得到 16个像素 */
                    vu8x16 = vcombine_u8(tmp_8x8x2.val[0], tmp_8x8x2.val[1]);

                    vst1q_u8(plumadst_e, vu8x16); /* store 16个像素 */

                    pchromaTopAddr_u += 8;
                    pchromaTopAddr_v += 8;
                    plumadst_e += 16;
                }

                plumadst_o = pDstChromaAddr + index * uvDstStride + uvDstStride;
                /* 取底场VU交织 */
                for (i = 0; i < width; i += 16)
                {
                    u_8x8 = vld1_u8(pchromaBotAddr_u); /* 底场u读入8个像素 */
                    v_8x8 = vld1_u8(pchromaBotAddr_v); /* 底场v读入8个像素 */
                    tmp_8x8x2 = vzip_u8(v_8x8, u_8x8); /* 交织 */
                    vu8x16 = vcombine_u8(tmp_8x8x2.val[0], tmp_8x8x2.val[1]);

                    /* store 数据 */
                    vst1q_u8(plumadst_o, vu8x16);
                    /* 地址累加 */
                    pchromaBotAddr_u += 8;
                    pchromaBotAddr_v += 8;
                    plumadst_o += 16;
                }
            }
        }
        else if (PROGRESSIVE_FRAME_MODE == pYuvFrm->frameMode)
        {
            if ((NULL == pYuvFrm->yTopAddr)
                || (NULL == pYuvFrm->uTopAddr)
                || (NULL == pYuvFrm->vTopAddr)
                || (width < yDstStride)
                || (width % 2u)
                || (yDstStride % 16u))
            {
                FACE_LOGE("Err, yTopAddr:%p,uTopAddr:%p vTopAddr:%p width:%d ,yStride:%d\n",
                          pYuvFrm->yTopAddr, pYuvFrm->uTopAddr, pYuvFrm->vTopAddr, width, yDstStride);

                err = SAL_FAIL;
                break;
            }

            /* 处理原则是nv21图像的缩放要求16字节对齐，源图像不满足，截取源图像中中间的部分，左右各裁剪一点 */

            /* 图像左边裁剪的偏移量 */
            offset = SAL_alignDown(((width - yDstStride) / 2), 4u); /* 4对齐 确保uv分量的偏移是2对齐 */

            /* y分量的指针先偏移好 */
            plumaTopAddr = pYuvFrm->yTopAddr + offset;

            /* Y分量每一次拷贝16个像素点(实际效率(每一次拷贝16个)和使用memcpy一次全部拷贝效率差不多) */
            for (index = 0; index < height; index++)
            {
                for (i = 0; i < yDstStride / 16; i++)
                {
                    y_8x8_11 = vld1q_u8(plumaTopAddr);      /* load 16个y分量像素值 */
                    vst1q_u8(pDstLumaAddr, y_8x8_11);       /* store 16个y分量像素值 */
                    pDstLumaAddr += 16;
                    plumaTopAddr += 16;
                }

                /* 图像左右边裁剪的偏移量 */
                plumaTopAddr += (srcYStride - yDstStride);
            }

            /* UV分量的指针先偏移好 */
            pChromaAddr_u = pYuvFrm->uTopAddr + offset / 2;
            pChromaAddr_v = pYuvFrm->vTopAddr + offset / 2;
            /* 注意YUV420颜色分量长度为一半,为兼容后续输出显示部分代码和Hisi解码输出保持一致将UV反置 */
            for (index = 0; index < height / 2; index++)
            {
                for (i = 0; i < uvDstStride / 16; i++)
                {
                    u_8x8 = vld1_u8(pChromaAddr_u); /* load 8个u分量像素值 */
                    v_8x8 = vld1_u8(pChromaAddr_v); /* load 8个v分量像素值 */
                    tmp_8x8x2 = vzip_u8(v_8x8, u_8x8); /* vu交织 */
                    vu8x16 = vcombine_u8(tmp_8x8x2.val[0], tmp_8x8x2.val[1]);

                    vst1q_u8(pDstChromaAddr, vu8x16);  /* store 16个vu分量数值 */
                    pChromaAddr_u += 8;
                    pChromaAddr_v += 8;
                    pDstChromaAddr += 16;
                }

                /* 图像左右边裁剪的偏移量 */
                pChromaAddr_u += srcUvStride - uvDstStride / 2;
                pChromaAddr_v += srcUvStride - uvDstStride / 2;
            }
        }
        else
        {
            FACE_LOGE("Err, frameMode:%d\n", pYuvFrm->frameMode);

            err = SAL_FAIL;
            break;
        }
    }
    while (0);

    return err;
}

/**
 * @function    Face_HalMemFree
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID Face_HalMemFree(VOID *buf)
{
    UINT8 *base_buf, *use_buf;

    if (NULL != buf)
    {
        use_buf = (UINT8 *)buf;
        base_buf = *(UINT8 **)(use_buf - sizeof(UINT8 *));

        SAL_memfree(base_buf, "FACE", "face_MemAlloc");
    }
}

/**
 * @function    Face_HalFreeMemTab
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Face_HalFreeMemTab(HKA_MEM_TAB *mem_tab, INT32 tab_num)
{
    HKA_S32 i = 0;

    FACE_HAL_CHECK_RET((NULL == mem_tab), err, "mem_tab == null!");
    FACE_HAL_CHECK_RET((tab_num < 1), err, "tab_num < 1");

    for (i = 0; i < tab_num; i++)
    {
        if (NULL != mem_tab[i].base)
        {
            Face_HalMemFree(mem_tab[i].base);
            mem_tab[i].base = NULL;
        }
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Face_HalMemAlloc
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID *Face_HalMemAlloc(HKA_SZT size, INT32 align)
{
    HKA_U08 *base_buf = NULL;
    HKA_U08 *use_buf = NULL;

    FACE_HAL_CHECK_RET((size <= 0), err, "(size  <= 0)");
    FACE_HAL_CHECK_RET((align <= 0), err, "(align <= 0)");

    base_buf = (HKA_U08 *)SAL_memMalloc(size + align + sizeof(HKA_U08 *), "FACE", "face_MemAlloc");

    if (NULL == base_buf)
    {
        return NULL;
    }

    use_buf = base_buf + sizeof(HKA_U08 *);

    while ((HKA_SZT)(intptr_t)use_buf % (HKA_SZT)align)
    {
        use_buf++;
    }

    *(HKA_U08 **)(use_buf - sizeof(HKA_U08 *)) = base_buf;

    return (VOID *)use_buf;
err:
    return NULL;
}

/**
 * @function    Face_HalAllocMemTab
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Face_HalAllocMemTab(HKA_MEM_TAB *mem_tab, HKA_S32 tab_num)
{
    HKA_S32 i = 0;
    HKA_SZT size = 0;
    VOID *buf = NULL;
    HKA_MEM_ALIGNMENT align = HKA_MEM_ALIGN_4BYTE;

    FACE_HAL_CHECK_RET((NULL == mem_tab), err, "(HKA_NULL == mem_tab)");
    FACE_HAL_CHECK_RET((tab_num < 1), err, "(tab_num < 1)");

    for (i = 0; i < tab_num; i++)
    {
        size = mem_tab[i].size;
        align = mem_tab[i].alignment;

        if (size != 0)
        {
            buf = Face_HalMemAlloc(size, (HKA_S32)align);
            FACE_HAL_CHECK_RET((NULL == buf), err, "(NULL == buf)");
        }
        else
        {
            buf = NULL;
        }

        mem_tab[i].base = buf;
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Face_HalCompare
 * @brief         人脸比对接口
 * @param[in]  chan: 通道号
 * @param[in]  pFeatData: 人脸特征数据(需要比对的)
 * @param[in]  fSim: 相似度标准
 * @param[in]  mode: 处理模式(0:登录比对, 1:注册比对)
 * @param[out] NULL
 * @return SAL_SOK
 */
UINT32 Face_HalCompare(UINT32 chan, void *pFeatData, float fSim, UINT32 mode)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;
    UINT32 bSuccess = 0;
    UINT32 bReg = 0;
    UINT32 uiFaceID = 0;
    UINT32 uiMaxIdx = 0;
    UINT8 *pFeatureData = NULL;
    float fTmpSim = 0.0;
    float fMaxSim = 0.0;

    FACE_COMMON_PARAM *pstFaceHalComm = NULL;
    FACE_DSP_DATABASE_PARAM *pstFeatureData = NULL;

    SAE_FACE_IN_DATA_FEATCMP_1V1_T stFeature_1v1 = {0};

    FACE_HAL_CHECK_CHAN(chan, err, "invalid chan!");
    FACE_HAL_CHECK_PTR(pFeatData, err, "pFeatData == null!");

    pstFeatureData = (FACE_DSP_DATABASE_PARAM *)pFeatData;

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_HAL_CHECK_PTR(pstFaceHalComm, err, "pstFaceHalComm == null!");

    FACE_LOGI("uiModelCnt is %d\n", g_stModelDataBase.uiModelCnt);

    for (i = 0; i < g_stModelDataBase.uiModelCnt; i++)
    {
        pFeatureData = (UINT8 *)((char *)(g_stModelDataBase.pFeatureData[i]) + FACE_TEATURE_HEADER_LENGTH);
        memcpy(&uiFaceID, (char *)(g_stModelDataBase.pFeatureData[i]), FACE_TEATURE_HEADER_LENGTH);

#if 0  /* 保存人脸建模数据 */
		Face_HalDumpFaceFeature((char *)pFeatureData, 272, "/home/config/login_dump", i);
#endif

        /* 校验特征数据是否满足当前extern compare版本 */
        {
            s32Ret = SAE_FACE_DFR_Compare_Extern_CheckFeature(pstFaceHalComm->pExternCompare,
                                                              pFeatureData, FACE_FEATURE_LENGTH);
            FACE_HAL_CHECK_RET(s32Ret, err, "SAE_FACE_DFR_Compare_Extern_CheckFeature failed!");
		
            s32Ret = SAE_FACE_DFR_Compare_Extern_CheckFeature(pstFaceHalComm->pExternCompare,
                                                              pstFeatureData->Featdata, FACE_FEATURE_LENGTH);
            FACE_HAL_CHECK_RET(s32Ret, err, "SAE_FACE_DFR_Compare_Extern_CheckFeature failed!");
        }

        /* 执行1v1比对 */
        {
            stFeature_1v1.feat1 = pFeatureData;
            stFeature_1v1.feat2 = pstFeatureData->Featdata;
            stFeature_1v1.feat_len = FACE_FEATURE_LENGTH;

            s32Ret = SAE_FACE_DFR_Compare_Extern_1v1(pstFaceHalComm->pExternCompare,
                                                     &stFeature_1v1,
                                                     sizeof(SAE_FACE_IN_DATA_FEATCMP_1V1_T),
                                                     &fTmpSim);
            if (SAL_SOK == s32Ret)
            {
                FACE_LOGI("i %d #1v1 sim: %f max_sim %.4f faceId %d\n", i, fTmpSim, fMaxSim, uiFaceID);

                /* 根据设置的相似度进行判断，记录最大相似度对应的数据ID */
                if (fTmpSim >= fSim)
                {
                    if (fTmpSim >= fMaxSim)
                    {
                        fMaxSim = fTmpSim;
                        uiMaxIdx = i;
                    }

                    /* 人脸注册时判断是否为重复人脸，mode 0:登录，mode 1:注册 */
                    if (1 == mode)
                    {
                        bReg = 1;
                        pstFeatureData->uiRepeatId = uiFaceID;  /* 保存数据库中重复的人脸 */
                        break;
                    }
                    else
                    {
                        bSuccess = 1;
                    }
                }
                else
                {
                    FACE_LOGW("i %d faceId %d Cmp sim %.4f < std %.4f\n", i, uiFaceID, fTmpSim, fSim);
                }
            }
            else
            {
                FACE_LOGE("i %d HIKFR_Compare_1vs1 err: 0x%x, std_sim %.4f, faceId %d \n", i, s32Ret, fSim, uiFaceID);
            }
        }
    }

    /* 人脸登录失败 */
    if (0 == mode && 0 == bSuccess)
    {
        FACE_LOGE("Found NO Face!\n");
        return SAL_FAIL;
    }

    /* 若人脸已注册，返回已注册人脸ID */
    if (1 == mode && 1 == bReg)
    {
        pstFeatureData->bFlag = FACE_REGISTER_REPEAT;/*设定为重复注册*/
        FACE_LOGW("Face existed in DataBase! FaceId %d \n", uiFaceID);
        return SAL_FAIL;
    }

    if (0 == mode)
    {
        /* 识别成功, 保存对应的人脸ID */
        memcpy(&pstFeatureData->uiFaceId, g_stModelDataBase.pFeatureData[uiMaxIdx], sizeof(UINT32));
    }

    FACE_LOGI("Face Module: Compare End! chan %d i %d Get Max Sim %.4f std %.4f \n", chan, uiMaxIdx, fMaxSim, fSim);
    return SAL_SOK;
err:
	if (pstFeatureData)
	{
		pstFeatureData->bFlag = FACE_REGISTER_FAIL;/*设定为注册失败(对比失败)*/
	}
	
	FACE_LOGE("Face Compare failed!!! \n");
    return SAL_FAIL;
}

/**
 * @function    Face_HalUpChnResolution
 * @brief         更新解码通道分辨率
 * @param[in]  chan: 通道号
 * @param[in]  *pPrm: 解码参数
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 Face_HalUpChnResolution(UINT32 chan, void *pPrm)
{
    /* 变量定义 */
    INT32 uiVdecChn = 0;

    IA_UPDATE_OUTCHN_PRM stOutChnPrm = {0};

    /* 入参有效性检验 */
    FACE_HAL_CHECK_CHAN(chan, err, "invalid chan!");
    FACE_HAL_CHECK_PTR(pPrm, err, "pPrm == null!");

    /* 上层应用确认解码通道目前设置为1 */
    uiVdecChn = *(UINT32 *)pPrm;

    /* 更新解码通道输出分辨率，临时写死为960*540，目前算法只支持该分辨率的人脸检测 */
    stOutChnPrm.uiVdecChn = uiVdecChn;
    stOutChnPrm.enModule = IA_MOD_FACE;
    stOutChnPrm.uiWidth = FACE_LOGIN_IMG_WIDTH;
    stOutChnPrm.uiHeight = FACE_LOGIN_IMG_HEIGHT;
    stOutChnPrm.uiVpssChn = FACE_VPSS_CHAN_ID;
    if (SAL_SOK != IA_UpdateOutChnResolution(&stOutChnPrm))
    {
        FACE_LOGE("Module %d, Update Chn Resolution [%d x %d] Failed! \n", stOutChnPrm.enModule, FACE_LOGIN_IMG_WIDTH, FACE_LOGIN_IMG_HEIGHT);
        return SAL_FAIL;
    }

    FACE_LOGI("update vdec chn resolution ok! Vdec %d w %d h %d\n", uiVdecChn, FACE_LOGIN_IMG_WIDTH, FACE_LOGIN_IMG_HEIGHT);
    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Face_HalGetFrame
 * @brief         人脸获取解码数据
 * @param[in]  uiVdecChn-解码通道
 * @param[in]  pFrame-帧数据
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 Face_HalGetFrame(UINT32 uiVdecChn, void *pFrame)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 u32NeedFree = 1;

    SYSTEM_FRAME_INFO *pstSysFrameInfo = NULL;
    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    /* checker */
    FACE_HAL_CHECK_PTR(pFrame, err, "pFrame == null!");

    pstSysFrameInfo = (SYSTEM_FRAME_INFO *)pFrame;

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_HAL_CHECK_PTR(pstFaceHalComm, err, "pFrame == null!");

    SAL_mutexLock(pstFaceHalComm->pVdecCpyFrmMutex);

    do
    {
        s32Ret = vdec_tsk_GetVdecFrame(uiVdecChn,
                                       FACE_VPSS_CHAN_ID,
                                       VDEC_FACE_GET_FRAME,
                                       &u32NeedFree,
                                       &pstFaceHalComm->stVdecCpyTmpFrameInfo,
                                       NULL);
        if (SAL_SOK != s32Ret)
        {
            goto unlock;
        }

        (VOID)sys_hal_getVideoFrameInfo(&pstFaceHalComm->stVdecCpyTmpFrameInfo, &stVideoFrmBuf);

        if (FACE_LOGIN_IMG_WIDTH != stVideoFrmBuf.frameParam.width || FACE_LOGIN_IMG_HEIGHT != stVideoFrmBuf.frameParam.height)
        {
            FACE_LOGW("vdecChn %d, outChn %d, w %d, h %d, not %d_%d! need update! \n",
                      uiVdecChn, FACE_VPSS_CHAN_ID, stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height,
                      FACE_LOGIN_IMG_WIDTH, FACE_LOGIN_IMG_HEIGHT);
             /*先释放通道帧buf，再设置通道属性*/
            if (u32NeedFree)
            {
                /* 释放通道帧Buf */
                s32Ret = vdec_tsk_PutVdecFrame(uiVdecChn, FACE_VPSS_CHAN_ID, &pstFaceHalComm->stVdecCpyTmpFrameInfo);
                if (SAL_SOK != s32Ret)
                {
                    FACE_LOGE("Vdec %d Put Frame fail!\n", uiVdecChn);
                    goto unlock;
                }
            }

            s32Ret = Face_HalUpChnResolution(0, &uiVdecChn);
            if (SAL_SOK != s32Ret)
            {
                FACE_LOGE("Update Face Chn Resolution Failed! \n ");
                goto unlock;
            }

            s32Ret = vdec_tsk_SetOutChnDataFormat(uiVdecChn, FACE_VPSS_CHAN_ID, SAL_VIDEO_DATFMT_YUV420SP_VU);
            if (SAL_SOK != s32Ret)
            {
                FACE_LOGE("update face login vdecChn [%d] data format failed! \n", uiVdecChn);
                goto unlock;
            }


            FACE_LOGI("Update uiVdecChn %d Resolution Success! format %d \n", uiVdecChn, SAL_VIDEO_DATFMT_YUV420SP_VU);
        }
    }
    while (FACE_LOGIN_IMG_WIDTH != stVideoFrmBuf.frameParam.width || FACE_LOGIN_IMG_HEIGHT != stVideoFrmBuf.frameParam.height);

    /* 对帧数据进行拷贝 */
    s32Ret = Ia_TdeQuickCopy(&pstFaceHalComm->stVdecCpyTmpFrameInfo,
                             pstSysFrameInfo,
                             0, 0,
                             stVideoFrmBuf.frameParam.width,
                             stVideoFrmBuf.frameParam.height,
                             SAL_FALSE);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("FACE Module: Tde Copy fail! vdec %d\n", uiVdecChn);
        s32Ret = vdec_tsk_PutVdecFrame(uiVdecChn, FACE_VPSS_CHAN_ID, &pstFaceHalComm->stVdecCpyTmpFrameInfo);
        if (SAL_SOK != s32Ret)
        {
            (VOID)vdec_tsk_PutVdecFrame(uiVdecChn, FACE_VPSS_CHAN_ID, &pstFaceHalComm->stVdecCpyTmpFrameInfo);
            FACE_LOGE("Vdec %d Put Frame fail!\n", uiVdecChn);
        }

        goto unlock;
    }

    if (u32NeedFree)
    {
        /* 释放通道帧Buf */
        s32Ret = vdec_tsk_PutVdecFrame(uiVdecChn, FACE_VPSS_CHAN_ID, &pstFaceHalComm->stVdecCpyTmpFrameInfo);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("Vdec %d Put Frame fail!\n", uiVdecChn);
            goto unlock;
        }
    }

    SAL_mutexUnlock(pstFaceHalComm->pVdecCpyFrmMutex);

    return SAL_SOK;

unlock:
    SAL_mutexUnlock(pstFaceHalComm->pVdecCpyFrmMutex);
err:
    return SAL_FAIL;
}

/**
 * @function    Face_HalDumpNv21
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID Face_HalDumpNv21(CHAR *pData, UINT32 u32DataSize, CHAR *pStrHead, UINT32 u32StrTailIdx, UINT32 u32W, UINT32 u32H)
{
    CHAR acPath[256] = {0};

    snprintf(acPath, 256, "%s_%d_w_%d_h_%d.nv21", pStrHead, u32StrTailIdx, u32W, u32H);

    FACE_LOGI("start dump into file: %s, from ptr %p, size %d \n", acPath, pData, u32DataSize);

    Face_HalDebugDumpData(pData, acPath, u32DataSize);

    FACE_LOGI("end dump into file: %s \n", acPath);
    return;
}

/**
 * @function   Face_HalDumpFaceFeature
 * @brief      ToDo
 * @param[in]  CHAR *pData
 * @param[in]  UINT32 u32DataSize
 * @param[in]  CHAR *pStrHead
 * @param[in]  UINT32 u32StrTailIdx
 * @param[in]  UINT32 u32W
 * @param[in]  UINT32 u32H
 * @param[out] None
 * @return     VOID
 */
VOID Face_HalDumpFaceFeature(CHAR *pData, UINT32 u32DataSize, CHAR *pStrHead, UINT32 u32StrTailIdx)
{
    CHAR acPath[256] = {0};

    snprintf(acPath, 256, "%s_%d.feature", pStrHead, u32StrTailIdx);

    FACE_LOGI("start dump into file: %s, from ptr %p, size %d \n", acPath, pData, u32DataSize);

    Face_HalDebugDumpData(pData, acPath, u32DataSize);

    FACE_LOGI("end dump into file: %s \n", acPath);
    return;
}

/**
 * @function    Face_HalLoginProc
 * @brief         人脸登录单次处理接口
 * @param[in]  chan-通道号
 * @param[in]  fSim-比对相似度
 * @param[in]  帧数据-用于单张建模用
 * @param[in]  bTmp-规避调试参数
 * @param[out] pOutInfo-人脸登录输出参数(1-是否登录成功 2-比对对应的id)
 * @return SAL_SOK
 */
INT32 Face_HalLoginProc(UINT32 chan, float fSim, void *pFrame, void *pOutInfo)
{
    /* 变量定义 */
    UINT32 s32Ret = SAL_FAIL;
    UINT32 uiWidth = 0;
    UINT32 uiHeight = 0;
    UINT32 uiFreeBufId = 0;

#if 1  /* 计算耗时使用，后续删除 */
    UINT64 time_comp_start = 0;
    UINT64 time_proc_err = 0;
    UINT64 time_proc_start = 0;
    UINT64 time_comp_end = 0;
    UINT64 time_proc_end = 0;
#endif

    void *pHandle = NULL;
    void *pVir = NULL;
    SYSTEM_FRAME_INFO *pstSystemFrame = NULL;
    FACE_ANA_BUF_INFO *pstFaceAnaBufTab = NULL;
    ICF_INPUT_DATA_V2 *pstFaceAnaBuf = NULL;
    FACE_VIDEO_LOGIN_OUT_S *pstVideoLoginOut = NULL;
    FACE_DSP_LOGIN_OUTPUT_PARAM *pstLoginOutputParam = NULL;
    FACE_COMMON_PARAM *pstFaceHalComm = NULL;
    SAE_FACE_IN_DATA_INPUT_T *pstFaceInDataInfo = NULL;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};
    FACE_DSP_DATABASE_PARAM stFaceFeatData = {0};

    /* 入参有效性检验 */
    FACE_HAL_CHECK_CHAN(chan, err, "chan er");
    FACE_HAL_CHECK_PTR(pFrame, err, "pFrame null");
    FACE_HAL_CHECK_PTR(pOutInfo, err, "pOutInfo null");

    time_proc_start = Face_HalgetTimeMilli();

    pstSystemFrame = (SYSTEM_FRAME_INFO *)pFrame;
    pstLoginOutputParam = (FACE_DSP_LOGIN_OUTPUT_PARAM *)pOutInfo;

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_HAL_CHECK_PTR(pstFaceHalComm, err, "pstFaceHalComm == null!");

    if (SAL_SOK != Face_HalGetAnaFreeBuf(FACE_VIDEO_LOGIN_MODE, &uiFreeBufId))
    {
        FACE_LOGE("Get Free Buf Failed! Chan %d\n", chan);
        return SAL_FAIL;
    }

    (VOID)sys_hal_getVideoFrameInfo(pstSystemFrame, &stVideoFrmBuf);

    uiWidth = FACE_LOGIN_IMG_WIDTH;
    uiHeight = FACE_LOGIN_IMG_HEIGHT;

    FACE_LOGI("Get From Vdec---chan %d w %d h %d uiFreeBufId %d\n", chan, uiWidth, uiHeight, uiFreeBufId);

    pVir = (void *)stVideoFrmBuf.virAddr[0];
    if (NULL == pVir)
    {
        FACE_LOGE("pVir == NULL!!! chan %d\n", chan);
        return SAL_FAIL;
    }

    pstFaceAnaBufTab = Face_HalGetAnaDataTab(FACE_VIDEO_LOGIN_MODE);
    pstFaceAnaBuf = &pstFaceAnaBufTab->stFaceBufData[uiFreeBufId];
    pstFaceInDataInfo = &pstFaceAnaBufTab->stFaceData[uiFreeBufId];

    /* 填充帧号 */
    pstFaceAnaBuf->stBlobData[0].nFrameNum = pstFaceAnaBufTab->uiFrameNum;

    /* 拷贝nv21数据送入引擎 */
    memcpy(pstFaceInDataInfo->data_info[0].yuv_data.y,
           (void *)pVir,
           FACE_LOGIN_IMG_WIDTH * FACE_LOGIN_IMG_HEIGHT * 3 / 2);

#if 0  /* 调试接口 */
    Face_HalDumpNv21((CHAR *)pVir,
                     FACE_LOGIN_IMG_WIDTH * FACE_LOGIN_IMG_HEIGHT * 3 / 2,
                     "/home/config/login",
                     pstFaceAnaBufTab->uiFrameNum,
                     FACE_LOGIN_IMG_WIDTH, FACE_LOGIN_IMG_HEIGHT);
#endif

    /* 模块控制参数 */
    pstFaceInDataInfo->proc_type = SAE_FACE_PROC_TYPE_TRACK_SELECT;
    pstFaceInDataInfo->face_proc_type = SAE_FACE_FACE_PROC_TYPE_MULTI_FACE;  /* 开启多人脸才会进行多人脸抓拍选帧 */
    pstFaceInDataInfo->error_proc_type = SAE_FACE_ERROR_PROC_FACE; /* SAE_FACE_ERROR_PROC_MODULE; */
    pstFaceInDataInfo->ls_img_enable = 0;
    pstFaceInDataInfo->priv_data = NULL;
    pstFaceInDataInfo->priv_data_size = 0;
    pstFaceInDataInfo->liveness_type = SAE_FACE_LIVENESS_TYPE_DISABLE;
    pstFaceInDataInfo->track_enable = 1;     /* 此业务线，必须track_enable使能，否则就会报错 */
    pstFaceInDataInfo->compare_enable = 1;  /* 此业务线，该功能无效 */

    /* 清空登录结果，避免旧数据干扰 */
    pstVideoLoginOut = &pstFaceAnaBufTab->uFaceProcOut.stVideoLoginProcOut;

    sal_memset_s(pstVideoLoginOut, sizeof(FACE_VIDEO_LOGIN_OUT_S),
                 0x00, sizeof(FACE_VIDEO_LOGIN_OUT_S));

    pHandle = Face_HalGetICFHandle(FACE_VIDEO_LOGIN_MODE, 0);
    if (NULL == pHandle)
    {
        *(pstFaceAnaBuf->pUseFlag[0]) = SAL_FALSE;
        FACE_LOGE("Get Vcae Handle Failed! Mode %d\n ", FACE_VIDEO_LOGIN_MODE);
        return SAL_FAIL;
    }

    s32Ret = pstFaceHalComm->stIcfFuncP.IcfInputData(pHandle, pstFaceAnaBuf, sizeof(ICF_INPUT_DATA_V2));
    while (SAL_SOK != s32Ret)
    {
        FACE_LOGE("Input data error 0x%x. Mode %d \n", s32Ret, FACE_VIDEO_LOGIN_MODE);
        s32Ret = pstFaceHalComm->stIcfFuncP.IcfInputData(pHandle, pstFaceAnaBuf, sizeof(ICF_INPUT_DATA_V2));
        usleep(500);
    }

    pstFaceAnaBufTab->uiFrameNum++;   /* 帧序号累加 */

    /* 等待算法回调结果 */
    sem_wait(&pstFaceAnaBufTab->sem);
	
	if (FACE_REGISTER_FAIL == pstFaceAnaBufTab->uFaceProcOut.stVideoLoginProcOut.enProcSts)
	{
	    pstLoginOutputParam->bSuccess = SAL_FALSE;
        return SAL_FAIL;
	}	

    FACE_LOGI("Get output Result! Mode %d\n", FACE_VIDEO_LOGIN_MODE);

    /* 打印调试信息，返回目标信息 */
    FACE_LOGI("============= Target Info Print ===============\n");

    if (FACE_REGISTER_SUCCESS != pstVideoLoginOut->enProcSts)       /* 返回状态0为成功返回 */
    {
        stFaceFeatData.bFlag = FACE_REGISTER_FAIL;   /* 标记为建模失败 */
        memset(&stFaceFeatData.Featdata[0], 0, FACE_FEATURE_LENGTH);
        FACE_LOGE("Get Output Feature Data Failed!\n");
    }
    else
    {
        stFaceFeatData.bFlag = FACE_REGISTER_SUCCESS;    /* 标记为建模成功 */
        stFaceFeatData.dataLen = pstVideoLoginOut->u32FeatDataLen;

        FACE_LOGI("Len %d \n", stFaceFeatData.dataLen);
        memcpy(&stFaceFeatData.Featdata[0], pstVideoLoginOut->acFeaureData, pstVideoLoginOut->u32FeatDataLen);

#if 0  /* 保存人脸建模数据 */
        Face_HalDumpFaceFeature((CHAR *)stFaceFeatData.Featdata, 272, "./dump", 4);
#endif
    }

    /* 建模成功进行比对 */
    if (FACE_REGISTER_SUCCESS == stFaceFeatData.bFlag)
    {
        time_comp_start = Face_HalgetTimeMilli();

        FACE_LOGI("into face compare, compare_score %f! \n", fSim);

        /* 人脸对比 */
        s32Ret = Face_HalCompare(chan, &stFaceFeatData, fSim, 0);   /*具体入参待确定 */
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("Face Module: chan %d Compare Fail!\n", chan);
            return SAL_FAIL;
        }

        /* 2. 比对成功，对入参中的face_id进行填充，并返回成功 */
        pstLoginOutputParam->uiFaceId = stFaceFeatData.uiFaceId;
        pstLoginOutputParam->bSuccess = SAL_TRUE;

        time_comp_end = Face_HalgetTimeMilli();

        FACE_LOGI("Compare End! FaceId %d sim %.4f cost %llu ms\n", stFaceFeatData.uiFaceId, fSim, time_comp_end - time_comp_start);
    }
    else
    {
        time_proc_err = Face_HalgetTimeMilli();
        FACE_LOGE("Build Model Failed! chan %d Mode %d, fail cost %llu ms\n", chan, FACE_VIDEO_LOGIN_MODE, time_proc_err - time_proc_start);
        return SAL_FAIL;
    }

    time_proc_end = Face_HalgetTimeMilli();

    FACE_LOGI("Login Proc End! cost %llu ms\n", time_proc_end - time_proc_start);
    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Face_HalDebugDumpData
 * @brief         dump数据接口
 * @param[in]  pData - dump数据
 * @param[in]  pPath - dump数据目标
 * @param[in]  uiSize - dump数据长度
 * @param[out] NULL
 * @return NULL
 */
void Face_HalDebugDumpData(CHAR *pData, CHAR *pPath, UINT32 uiSize)
{
    INT32 ret = 0;
    FILE *fp = NULL;

    fp = fopen(pPath, "a+");
    if (!fp)
    {
        FACE_LOGE("fopen %s failed! \n", pPath);
        goto exit;
    }

    ret = fwrite(pData, uiSize, 1, fp);
    if (ret < 0)
    {
        FACE_LOGE("fwrite err! \n");
        goto exit;
    }

    fflush(fp);

exit:
    if (NULL != fp)
    {
        fclose(fp);
        fp = NULL;
    }

    return;
}

/**
 * @function    Face_HalUpdateCompareLib
 * @brief         更新人脸数据库(暂时不使用)
 * @param[in]
 * @param[out]
 * @return
 */
UINT32 Face_HalUpdateCompareLib(void)
{
    /* 仅MvsN比对时需要更新人脸底库，目前只用到1vs1 */
    return SAL_SOK;
}

/**
 * @function    Face_HalGetVersion
 * @brief         获取人脸组件版本信息
 * @param[in]  NULL
 * @param[out] NULL
 * @return NULL
 */
INT32 Face_HalGetVersion(void)
{
    INT32 s32Ret = SAL_FAIL;

    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    SAE_VERSION_T stVersionInfo = {0};

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_HAL_CHECK_PTR(pstFaceHalComm, err, "pstFaceHalComm == null!");

    /* 引擎版本 */
    s32Ret = pstFaceHalComm->stIcfFuncP.SaeGetAppVersion(&stVersionInfo, sizeof(SAE_VERSION_T));
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("SAE_GetVersion fail! ret=0x%x\n", s32Ret);
        return s32Ret;
    }

    FACE_LOGI("SAE V%d.%d.%d DATE:%d-%d-%d\n",
              stVersionInfo.sae_app_version.major_version,
              stVersionInfo.sae_app_version.minor_version,
              stVersionInfo.sae_app_version.revis_version,
              stVersionInfo.sae_app_version.version_year,
              stVersionInfo.sae_app_version.version_month,
              stVersionInfo.sae_app_version.version_day);

    /* 算法版本 */
    FACE_LOGI("PKG NAME: %s\nVERSION: %d-%d-%d\nPLAT:%s, SYSINFO: %s, ACCURACY: %s, ENCRYPTION: %s, BUILD_TIME: %s, VERSION_PROPERTIES: %s\n",
              stVersionInfo.dfr_pkg_version.algo_name,
              stVersionInfo.dfr_pkg_version.major_version,
              stVersionInfo.dfr_pkg_version.minor_version,
              stVersionInfo.dfr_pkg_version.revis_version,
              stVersionInfo.dfr_pkg_version.plat_name,
              stVersionInfo.dfr_pkg_version.sys_info,
              stVersionInfo.dfr_pkg_version.accuracy,
              stVersionInfo.dfr_pkg_version.encryption,
              stVersionInfo.dfr_pkg_version.build_time,
              stVersionInfo.dfr_pkg_version.version_properties);

    sal_memcpy_s(&g_stFaceCommonPrm.stVersionInfo.stAppVerInfo, sizeof(FACE_VERSION_DATA),
                 &stVersionInfo.sae_app_version, sizeof(SAE_APP_VERSION_T));

    FACE_LOGI("Get Version Info End!\n");
    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Face_HalMemPoolSystemMallocCb
 * @brief      申请系统内存的回调函数
 * @param[in]  void *pInitHandle
 * @param[in]  ICF_MEM_INFO_V2    *pMemInfo
 * @param[in]  ICF_MEM_BUFFER_V2  *stMemBuffer
 * @param[out] None
 * @return     INT32
 */
INT32 Face_HalMemPoolSystemMallocCb(void *pInitHandle,
                                    ICF_MEM_INFO_V2 *pMemInfo,
                                    ICF_MEM_BUFFER_V2 *pstMemBuffer)
{
    INT32 s32Ret = SAL_FAIL;
    VOID *va = NULL;

    ALLOC_VB_INFO_S stVbInfo = {0};

    switch (pMemInfo->eMemType)
    {
        case ICF_MEM_MALLOC:
        {
            va = SAL_memZalloc(pMemInfo->nMemSize, "face", "engine");
            if (NULL == va)
            {
                FACE_LOGE("malloc failed! \n");
                return SAL_FAIL;
            }

            pstMemBuffer->pVirMemory = (void *)va;
            pstMemBuffer->pPhyMemory = (VOID *)va;
            break;
        }
        case ICF_RN_MEM_MMZ_IOMMU_WITH_CACHE:
        {
            FACE_LOGI("RK MEM ALLOC, lBufSize13 = %llu \n", pMemInfo->nMemSize);
            s32Ret = mem_hal_iommuMmzAlloc(pMemInfo->nMemSize, "face", "face_engine", NULL, SAL_TRUE, &stVbInfo);
            if (SAL_SOK != s32Ret)
            {
                FACE_LOGE("rk cma Malloc err!!\n");
                return SAL_FAIL;
            }

            pstMemBuffer->pVirMemory = (void *)stVbInfo.pVirAddr;
            pstMemBuffer->pPhyMemory = (VOID *)stVbInfo.u64VbBlk;  /* 当前引擎复用物理地址传递MB */

            goto exit;
        }
        default:
        {
            FACE_LOGE("invalid mem type %d \n", pMemInfo->eMemType);
            return SAL_FAIL;
        }
    }

exit:
    return SAL_SOK;
}

/**
 * @function   Face_HalMemPoolSystemFreeCb
 * @brief      释放系统内存的回调函数
 * @param[in]  IA_MEM_TYPE_E enMemType
 * @param[in]  UINT32 u32MemSize
 * @param[in]  IA_MEM_PRM_S *pstMemBuf
 * @param[out] None
 * @return     static INT32
 */
INT32 Face_HalMemPoolSystemFreeCb(void *pInitHandle,
                                  ICF_MEM_INFO_V2 *pMemInfo,
                                  ICF_MEM_BUFFER_V2 *pstMemBuffer)

{
    INT32 s32Ret = SAL_FAIL;

    switch (pMemInfo->eMemType)
    {
        case ICF_MEM_MALLOC:
        {
            SAL_memfree(pstMemBuffer->pVirMemory, "face", "engine");
            break;
        }
        case ICF_RN_MEM_MMZ_IOMMU_WITH_CACHE:
        {
            FACE_LOGI("RK MEM ALLOC, lBufSize13 = %llu \n", pstMemBuffer->nMemSize);

            /* 当前引擎复用物理地址传递MB */
            s32Ret = mem_hal_iommuMmzFree(pstMemBuffer->nMemSize, "face", "face_engine", (UINT64)pstMemBuffer->pPhyMemory, pstMemBuffer->pVirMemory, (UINT64)pstMemBuffer->pPhyMemory);
            if (SAL_SOK != s32Ret)
            {
                FACE_LOGE("rk iommu mmz free err!!\n");
                return SAL_FAIL;
            }

            return SAL_SOK;
        }
        default:
        {
            FACE_LOGE("invalid mem type %d \n", pMemInfo->eMemType);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function    Face_HalFree
 * @brief         释放内存
 * @param[in]   p-待释放内存
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 Face_HalFree(VOID *p)
{
    if (NULL != p)
    {
        FACE_LOGI("no need malloc! \n");
        goto exit;
    }

    SAL_memfree(p, "FACE", "face_malloc");
    p = NULL;

exit:
    return SAL_SOK;
}

/**
 * @function    Face_HalMalloc
 * @brief         malloc一段内存
 * @param[in]    pp -内存地址指针
 * @param[in]    uiSize-内存大小
 * @param[out]  NULL
 * @return SAL_SOK
 */
INT32 Face_HalMalloc(VOID **pp, UINT32 uiSize)
{
    if (NULL != *pp)
    {
        FACE_LOGI("no need malloc! \n");
        goto exit;
    }

    *pp = SAL_memMalloc(uiSize, "FACE", "face_malloc");
    if (NULL == *pp)
    {
        FACE_LOGE("malloc Failed!\n");
        return SAL_FAIL;
    }

    sal_memset_s(*pp, uiSize, 0x00, uiSize);

exit:
    return SAL_SOK;
}

/**
 * @function    Face_HalIcfInit
 * @brief         算法资源初始化
 * @param[in]  NULL
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_HalIcfInit(void)
{
    /* 变量定义 */
    INT32 s32Ret = SAL_FAIL;

    ICF_INIT_PARAM_V2 stInitParam = {0};
    ICF_INIT_HANDLE_V2 stInitHandle = {0};

    UINT64 time0 = 0;
    UINT64 time1 = 0;

    time0 = Face_HalgetTimeMilli();

    /* cnn全局调度句柄 */
    stInitParam.pScheduler = IA_GetScheHndl();

    /* 引擎配置文件路径 */
    stInitParam.stConfigInfo.pAlgCfgPath = "./face/config/AlgCfg.json";
    stInitParam.stConfigInfo.pTaskCfgPath = "./face/config/TASK.json";
    stInitParam.stConfigInfo.pToolkitCfgPath = "./face/config/ToolkitCfg.json";

    /* 支持用户在外部申请内存管理函数 */
    stInitParam.stMemConfig.pMemSytemMallocInter = (void *)Face_HalMemPoolSystemMallocCb;
    stInitParam.stMemConfig.pMemSytemFreeInter = (void *)Face_HalMemPoolSystemFreeCb;

    /*引擎初始化*/
    s32Ret = g_stFaceCommonPrm.stIcfFuncP.IcfInit(&stInitParam,
                                                  sizeof(ICF_INIT_PARAM_V2),
                                                  &stInitHandle,
                                                  sizeof(ICF_INIT_HANDLE_V2));
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("ICF_Init error %x\n", s32Ret);
        return SAL_FAIL;
    }

    time1 = Face_HalgetTimeMilli();

    g_stFaceCommonPrm.pInitHandle = stInitHandle.pInitHandle;

    FACE_LOGI("ICF INIT OK! get init handle %p,  cost %llu ms\n", g_stFaceCommonPrm.pInitHandle, time1 - time0);
    return SAL_SOK;
}

/**
 * @function:   Face_HalInitAlgApi
 * @brief:      加载引擎应用层动态库符号
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Face_HalInitAlgApi(VOID)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = Sal_GetLibHandle("libSaeAlg.so", &g_stFaceCommonPrm.pIcfAlgHandle);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibHandle failed!");

    /* ICF_Interface.h */
    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfAlgHandle, "SAE_GetVersion", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.SaeGetAppVersion);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_APP_GetVersion_V2 failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Face_HalInitIcfApi
 * @brief      动态加载icf引擎框架符号
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalInitIcfApi(VOID)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = Sal_GetLibHandle("libicf.so", &g_stFaceCommonPrm.pIcfLibHandle);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibHandle failed!");

    /* ICF_Interface.h */
    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_Init_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfInit);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_Init_V2 failed!");

    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_Finit_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfFinit);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_Finit_V2 failed!");

    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_Create_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfCreate);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_Create_V2 failed!");

    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_Destroy_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfDestroy);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_Destroy_V2 failed!");

    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_LoadModel_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfLoadModel);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_Destroy_V2 failed!");

    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_UnloadModel_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfUnloadModel);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_Destroy_V2 failed!");

    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_SetConfig_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfSetConfig);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_SetConfig_V2 failed!");

    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_GetConfig_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfGetConfig);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_GetConfig_V2 failed!");

    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_SetCallback_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfSetCallback);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_SetCallback_V2 failed!");

    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_GetVersion_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfGetVersion);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_GetVersion_V2 failed!");

    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_InputData_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfInputData);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_InputData_V2 failed!");

    /* ICF_toolkit.h */
    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_GetMemPoolStatus_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfGetMemPoolStatus);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_GetMemPoolStatus_V2 failed!");

    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_Package_GetState_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfGetPackageStatus);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_Package_GetState_V2 failed!");

    s32Ret = Sal_GetLibSymbol(g_stFaceCommonPrm.pIcfLibHandle, "ICF_GetDataPtrFromPkg_V2", (VOID **)&g_stFaceCommonPrm.stIcfFuncP.IcfGetPackageDataPtr);
    FACE_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "ICF_GetDataPtrFromPkg_V2 failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Face_HalInitRtld
 * @brief:      Face模块加载动态库符号
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Face_HalInitRtld(VOID)
{
    INT32 s32Ret = SAL_SOK;

    /* libicf.so */
    s32Ret = Face_HalInitIcfApi();
    FACE_HAL_CHECK_RET(s32Ret, err, "Face_HalInitIcfApi failed!");

    /* libSaeAlg.so */
    s32Ret = Face_HalInitAlgApi();
    FACE_HAL_CHECK_RET(s32Ret, err, "Face_HalInitAlgApi failed!");

    FACE_LOGI("init run time loader end! \n");
    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Face_HalPrMem
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
void Face_HalPrMem(ICF_MEMSIZE_PARAM_V2 *pstMemSize)
{
    float total_memsize = 0;

    /* 鎵撳嵃娑堣�楃殑鍐呭瓨澶у皬 */
    for (int tt = 0; tt < ICF_MEM_TYPE_NUM; ++tt)
    {
        if (pstMemSize->stNonSharedMemInfo[tt].nMemSize <= 0)
            continue;

        FACE_LOGI("stNonSharedMemInfo[%d].nMemSize = [%llu]B [%f]MB type %d\n", tt,
                  pstMemSize->stNonSharedMemInfo[tt].nMemSize,
                  pstMemSize->stNonSharedMemInfo[tt].nMemSize / 1024.0 / 1024.0,
                  pstMemSize->stNonSharedMemInfo[tt].eMemType);
        total_memsize += pstMemSize->stNonSharedMemInfo[tt].nMemSize / 1024.0 / 1024.0;
    }

    for (int tt = 0; tt < ICF_MEM_TYPE_NUM; ++tt)
    {
        if (pstMemSize->stSharedMemInfo[tt].nMemSize <= 0)
            continue;

        FACE_LOGI("stSharedMemInfo[%d].nMemSize = [%llu]B [%f]MB type %d\n", tt,
                  pstMemSize->stSharedMemInfo[tt].nMemSize,
                  pstMemSize->stSharedMemInfo[tt].nMemSize / 1024.0 / 1024.0,
                  pstMemSize->stSharedMemInfo[tt].eMemType);
        total_memsize += pstMemSize->stSharedMemInfo[tt].nMemSize / 1024.0 / 1024.0;
    }

    FACE_LOGI("total memsize [%f]M\n", total_memsize);
}

/**
 * @function   Face_HalInitEngineChannel
 * @brief      初始化引擎通道(loadModel + create + setCallBack)
 * @param[in]  FACE_ANA_MODE_E enProcLine
 * @param[in]  FACE_INIT_ENGINE_CHANNEL_PRM_S *pstInitPrm
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalInitEngineChannel(FACE_ANA_MODE_E enProcLine, /* 业务线枚举，DSP内部定义 */
                                       UINT32 u32LineIdx, /* 业务线中的处理节点，当前人脸抓拍业务线需要先选帧、再建模的处理 */
                                       FACE_INIT_ENGINE_CHANNEL_PRM_S *pstInitPrm)
{
    INT32 s32Ret = SAL_FAIL;

    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    ICF_MODEL_PARAM_V2 stModelParam = {0};
    ICF_CREATE_PARAM_V2 createParam = {0};
    ICF_CALLBACK_PARAM_V2 callbackParam = {0};

    if (NULL == pstInitPrm || enProcLine >= FACE_MODE_MAX_NUM)
    {
        FACE_LOGE("invalid input prm! pstInitPrm %p, enProcLine %d \n", pstInitPrm, enProcLine);
        return SAL_FAIL;
    }

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_HAL_CHECK_PTR(pstFaceHalComm, err, "pstFaceHalComm == null!");

    stModelParam.nGraphID = pstInitPrm->u32GraphId;      /* SAE_FACE_GID_DET_FEATURE_1; */
    stModelParam.nGraphType = pstInitPrm->u32GraphType;    /* SAE_GTYPE_FACE; */
    stModelParam.pAppParam = pstInitPrm->pstAppParam;     /* pstAppParam; */
    stModelParam.nMaxCacheNums = 0;
    stModelParam.pInitHandle = pstInitPrm->pIcfInitHandle;

    FACE_LOGI("nGraphID %d, nGraphType %d, pAppParam %p, pInitHandle %p \n",
              stModelParam.nGraphID, stModelParam.nGraphType, stModelParam.pAppParam, stModelParam.pInitHandle);

    s32Ret = pstFaceHalComm->stIcfFuncP.IcfLoadModel(&stModelParam,
                                                     sizeof(ICF_MODEL_PARAM_V2),
                                                     &pstFaceHalComm->astProcLineHandle[enProcLine].stIcfModelHandle,
                                                     sizeof(ICF_MODEL_HANDLE_V2));
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("ICF_LoadModel graph_1_11 error:%x \n", s32Ret);
        pstFaceHalComm->stIcfFuncP.IcfUnloadModel(&pstFaceHalComm->astProcLineHandle[enProcLine].stIcfModelHandle, sizeof(ICF_MODEL_HANDLE_V2));
        return SAL_FAIL;
    }

    FACE_LOGI("ICF_LoadModel success!\n");

    /* 显示内存占用 模型内存 */
    Face_HalPrMem(&pstFaceHalComm->astProcLineHandle[enProcLine].stIcfModelHandle.modelMemSize);

    memcpy(&createParam.modelParam, &stModelParam, sizeof(ICF_MODEL_PARAM_V2));
    memcpy(&createParam.modelHandle, &pstFaceHalComm->astProcLineHandle[enProcLine].stIcfModelHandle, sizeof(ICF_MODEL_HANDLE_V2));

    s32Ret = pstFaceHalComm->stIcfFuncP.IcfCreate(&createParam,
                                                  sizeof(ICF_CREATE_PARAM_V2),
                                                  &pstFaceHalComm->astProcLineHandle[enProcLine].stIcfCreateHandle[u32LineIdx],
                                                  sizeof(ICF_CREATE_HANDLE_V2));
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("ICF_Create graph_1_11 error:%x \n", s32Ret);
        pstFaceHalComm->stIcfFuncP.IcfDestroy(&pstFaceHalComm->astProcLineHandle[enProcLine].stIcfCreateHandle[u32LineIdx], sizeof(ICF_CREATE_HANDLE_V2));
        return SAL_FAIL;
    }

    g_stFaceCommonPrm.pEngineChnHandle[enProcLine][u32LineIdx] = pstFaceHalComm->astProcLineHandle[enProcLine].stIcfCreateHandle[u32LineIdx].pChannelHandle;

    FACE_LOGI("ICF_Create success!\n");

    /* 显示内存占用 句柄内存及运行内存 */
    Face_HalPrMem(&pstFaceHalComm->astProcLineHandle[enProcLine].stIcfCreateHandle[u32LineIdx].createMemSize);

    /* ICF引擎 设置回调 */
    callbackParam.nNodeID = pstInitPrm->u32PostNodeId;              /* nid_graph_post; */
    callbackParam.nCallbackType = ICF_CALLBACK_OUTPUT;
    callbackParam.pCallbackFunc = (void *)pstInitPrm->pCallBackFunc;        /* ICF_OutDataCallback_face_graph_4_5; */
    callbackParam.nUserSize = 0;
    callbackParam.pUsr = NULL;

    s32Ret = pstFaceHalComm->stIcfFuncP.IcfSetCallback(pstFaceHalComm->astProcLineHandle[enProcLine].stIcfCreateHandle[u32LineIdx].pChannelHandle,
                                                       &callbackParam,
                                                       sizeof(ICF_CALLBACK_PARAM_V2));
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("ICF_SetCallback_V2 graph_1_11 error:%x \n", s32Ret);
        return SAL_FAIL;
    }

    FACE_LOGI("ICF_SetCallback_V2 success!\n");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Face_HalSetPicRegisterDefaultConfig
 * @brief      图片注册业务线的默认配置
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalSetPicRegisterDefaultConfig(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    ICF_CONFIG_PARAM_V2 configParam = { 0 };

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_HAL_CHECK_PTR(pstFaceHalComm, err, "pstFaceHalComm == null!");

    /* 人脸关键点+评分的默认配置 */
    {
        SAE_FACE_CFG_QLTY_THRSH_T stQualityThreshCfg = {0};

        stQualityThreshCfg.qty_thresh.landmark_confidence = 0.0;
        stQualityThreshCfg.qty_thresh.detect_orientation = 0;
        stQualityThreshCfg.qty_thresh.eye_distance = 18.0;
        stQualityThreshCfg.qty_thresh.color_confidence = 0.0;
        stQualityThreshCfg.qty_thresh.gray_scale = 0;
        stQualityThreshCfg.qty_thresh.gray_mean_range.low = 0;
        stQualityThreshCfg.qty_thresh.gray_mean_range.high = 255.0;
        stQualityThreshCfg.qty_thresh.gray_variance_range.low = 0.0;
        stQualityThreshCfg.qty_thresh.gray_variance_range.high = 128.0;
        stQualityThreshCfg.qty_thresh.clearity_score = 0.0f;
        stQualityThreshCfg.qty_thresh.pose_pitch = 90;
        stQualityThreshCfg.qty_thresh.pose_yaw = 90;
        stQualityThreshCfg.qty_thresh.pose_roll = 90;
        stQualityThreshCfg.qty_thresh.pose_confidence = 0.0;
        stQualityThreshCfg.qty_thresh.frontal_score = 0.0f;
        stQualityThreshCfg.qty_thresh.visible_score = 0.0f;
        stQualityThreshCfg.qty_thresh.face_score = 0.1f;

        memset(&configParam, 0, sizeof(configParam));

        configParam.nNodeID = SAE_FACE_NID_DET_FEATURE_1_DFR_QUALITY;
        configParam.nKey = SAE_FACE_CFG_FACE_QUALITY_THRSH;
        configParam.pConfigData = &stQualityThreshCfg;
        configParam.nConfSize = sizeof(SAE_FACE_CFG_QLTY_THRSH_T);

        s32Ret = pstFaceHalComm->stIcfFuncP.IcfSetConfig(pstFaceHalComm->astProcLineHandle[FACE_PICTURE_MODE].stIcfCreateHandle[0].pChannelHandle,
                                                         &configParam,
                                                         sizeof(ICF_CONFIG_PARAM_V2));
        FACE_HAL_CHECK_RET(SAL_SOK != s32Ret, err, "ICF_Set_config SAE_FACE_CFG_FACE_QUALITY_THRSH failed!");
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Face_HalInitRegisterAnaData
 * @brief      初始化图片注册全局缓存数据(静态图像)
 * @param[in]  VOID
 * @param[out] None
 * @return     static
 */
static INT32 Face_HalInitRegisterAnaData(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;
    UINT32 imageSize = FACE_REGISTER_MAX_WIDTH * FACE_REGISTER_MAX_HEIGHT * 3 / 2 + 64;  /* 最大1080P,+64是为了使用的内存首地址64位对齐后，保证可用内存仍是1080p*/

    FACE_ANA_BUF_INFO *pstPicInputData = NULL;
    ALLOC_VB_INFO_S stVbInfo = {0};

    /* 建模入库帧数据缓存申请 */
    pstPicInputData = Face_HalGetAnaDataTab(FACE_PICTURE_MODE);
    if (NULL == pstPicInputData)
    {
        FACE_LOGE("get pic register global prm failed! mode: %d \n", FACE_PICTURE_MODE);
        return SAL_FAIL;
    }

    /* 图片注册最大缓存个数为3 */
    pstPicInputData->uiMaxBufNum = FACE_MAX_REGISTER_BUF_NUM;

    for (i = 0; i < pstPicInputData->uiMaxBufNum; i++)
    {
        pstPicInputData->stFaceBufData[i].pUseFlag[0] = (int *)&pstPicInputData->uiRlsFlag[i];

        pstPicInputData->stFaceBufData[i].nBlobNum = 1;
        pstPicInputData->stFaceBufData[i].stBlobData[0].eBlobFormat = ICF_INPUT_FORMAT_YUV_NV21; /* DFR+FEATURE使用NV21数据 */
        pstPicInputData->stFaceBufData[i].stBlobData[0].nFrameNum = 0;
        pstPicInputData->stFaceBufData[i].stBlobData[0].pData = &(pstPicInputData->stFaceData[i]);

        /* 输入数据 */
        pstPicInputData->stFaceData[i].reserved[0] = 0;
        pstPicInputData->stFaceData[i].priv_data = NULL;
        pstPicInputData->stFaceData[i].priv_data_size = 0;

        /* 填充默认ROI参数，默认全屏 */
        pstPicInputData->stFaceData[i].data_info[0].roi_rect.x = 0;
        pstPicInputData->stFaceData[i].data_info[0].roi_rect.y = 0;
        pstPicInputData->stFaceData[i].data_info[0].roi_rect.width = 1;
        pstPicInputData->stFaceData[i].data_info[0].roi_rect.height = 1;

        /* 数据优先级配置 */
        pstPicInputData->stFaceData[i].data_priority.det_priority = SAE_FACE_PROC_PRIO_TYPE_HIGH;
        pstPicInputData->stFaceData[i].data_priority.feat_priority = SAE_FACE_PROC_PRIO_TYPE_HIGH; /* 实际上这个流程没有建模，所以配置低或者不配都可以，但是在建模活体会用到这个（demo这么写的） */

        /* 控制参数 */
        pstPicInputData->stFaceData[i].face_proc_type = SAE_FACE_FACE_PROC_TYPE_SINGLE_FACE_BIGGEST;
        pstPicInputData->stFaceData[i].proc_type = SAE_FACE_PROC_TYPE_DET_FEATURE;
        pstPicInputData->stFaceData[i].error_proc_type = SAE_FACE_ERROR_PROC_FACE;//SAE_FACE_ERROR_PROC_MODULE
        pstPicInputData->stFaceData[i].ls_img_enable = 0;
        pstPicInputData->stFaceData[i].track_enable = 0;
        pstPicInputData->stFaceData[i].attribute_enable = 0;
        pstPicInputData->stFaceData[i].liveness_type = SAE_FACE_LIVENESS_TYPE_DISABLE;
        pstPicInputData->stFaceData[i].compare_enable = 0;

        /* 当前图片注册仅使用可见光原图 */
        pstPicInputData->stFaceData[i].img_type_info.proc_main_type = SAE_FACE_IMG_TYPE_RGB;

        /* 指定 data_info 索引0,1上的光源有效性及类型描述 */
        pstPicInputData->stFaceData[i].img_type_info.img_type_describe[0].img_type = SAE_FACE_IMG_TYPE_RGB;
        pstPicInputData->stFaceData[i].img_type_info.img_type_describe[0].valid = 1;

        /* 指定 data_info 索引2,3上的光源有效性及类型描述 */
        pstPicInputData->stFaceData[i].img_type_info.img_type_describe[1].img_type = SAE_FACE_IMG_TYPE_IR;
        pstPicInputData->stFaceData[i].img_type_info.img_type_describe[1].valid = 0;
        s32Ret = mem_hal_vbAlloc(imageSize,
                                 "FACE", "mmz_with_cache", NULL,
                                 SAL_TRUE,
                                 &stVbInfo);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("MmzAlloc Failed!!ret is 0x%x\n", s32Ret);
            return SAL_FAIL;
        }

        /* 图像初始化信息 */
        pstPicInputData->stFaceData[i].data_info[0].yuv_data.format = SAE_VCA_YUV420;
        pstPicInputData->stFaceData[i].data_info[0].yuv_data.scale_rate = 1;
        pstPicInputData->stFaceData[i].data_info[0].yuv_data.image_w = FACE_REGISTER_MAX_WIDTH;
        pstPicInputData->stFaceData[i].data_info[0].yuv_data.image_h = FACE_REGISTER_MAX_HEIGHT;
        pstPicInputData->stFaceData[i].data_info[0].yuv_data.pitch_y = FACE_REGISTER_MAX_WIDTH;
        pstPicInputData->stFaceData[i].data_info[0].yuv_data.pitch_uv = FACE_REGISTER_MAX_WIDTH;
        pstPicInputData->stFaceData[i].data_info[0].yuv_data.y = (UINT8 *)SAL_align((PhysAddr)stVbInfo.pVirAddr, 64);   /*算法要求，送入引擎的buff要保证首地址为64位对齐*/
        pstPicInputData->stFaceData[i].data_info[0].yuv_data.u = (UINT8 *)SAL_align((PhysAddr)stVbInfo.pVirAddr, 64) + FACE_REGISTER_MAX_WIDTH * FACE_REGISTER_MAX_HEIGHT;
        pstPicInputData->stFaceData[i].data_info[0].yuv_data.v = pstPicInputData->stFaceData[i].data_info[0].yuv_data.u;
    }

    FACE_LOGI("pic register data init end! \n");
    return SAL_SOK;
}

/**
 * @function   Face_HalInitPicRegisterLine
 * @brief      初始化图片注册业务线资源
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalInitPicRegisterLine(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    SAE_FACE_ABILITY_PARAM_T stSaeFaceAbilityPrm = {0};
    ICF_APP_PARAM_INFO_V2 stAppParamInfo = {0};

    FACE_INIT_ENGINE_CHANNEL_PRM_S stInitEngineChnPrm = {0};

    /* fixme: 平台类型为RK3588 */
    stSaeFaceAbilityPrm.platform.type = 4;

    stSaeFaceAbilityPrm.dfr_detect.enable = 1;
    stSaeFaceAbilityPrm.dfr_landmark.enable = 1;
    stSaeFaceAbilityPrm.dfr_quality.enable = 1;
    stSaeFaceAbilityPrm.dfr_feature.enable = 1;

    stSaeFaceAbilityPrm.dfr_detect.max_width = 1920;
    stSaeFaceAbilityPrm.dfr_detect.max_height = 1080;
    stSaeFaceAbilityPrm.dfr_detect.max_face_num = 10;
    stSaeFaceAbilityPrm.fd_track.max_width = 1920;
    stSaeFaceAbilityPrm.fd_track.max_height = 1080;
    stSaeFaceAbilityPrm.fd_quality.max_width = 1920;
    stSaeFaceAbilityPrm.fd_quality.max_height = 1080;
    stSaeFaceAbilityPrm.dfr_liveness.max_width = 1920;
    stSaeFaceAbilityPrm.dfr_liveness.max_height = 1080;
    stSaeFaceAbilityPrm.dfr_compare.patch_num = 1;
    stSaeFaceAbilityPrm.dfr_compare.feat_dim = 272;
    stSaeFaceAbilityPrm.dfr_compare.head_length = 16;
    stSaeFaceAbilityPrm.dfr_compare.max_feat_num = 50000;
    stSaeFaceAbilityPrm.dfr_compare.top_n = 1;

    /* 模型路径 */
    snprintf(stSaeFaceAbilityPrm.fd_track.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", FD_TRACK_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.fd_quality.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", FD_QUALITY_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_detect.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_DETECT_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_landmark.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_LANDMARK_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_quality.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_QUALITY_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_liveness.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_LIVENESS_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_attribute.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_ATTRIBUTE_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_feature.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_FEATURE_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_compare.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_COMPARE_MODEL_PATH);

    /* 加载模型需要外部申请malloc内存，load_model成功后需要释放这块内存 */
    stSaeFaceAbilityPrm.model_buff.nSize = 60 * 1024 * 1024;
    stSaeFaceAbilityPrm.model_buff.pData = SAL_memZalloc(stSaeFaceAbilityPrm.model_buff.nSize, "FACE", "MODEL_BUFF");
    if (NULL == stSaeFaceAbilityPrm.model_buff.pData)
    {
        FACE_LOGE("malloc model buff failed! \n");
        goto exit;
    }

    stAppParamInfo.stAppParamCfgBuff.pBuff = &stSaeFaceAbilityPrm;
    stAppParamInfo.stAppParamCfgBuff.nBuffSize = sizeof(SAE_FACE_ABILITY_PARAM_T);

    /* 创建引擎通道，load model + create + set config */
    stInitEngineChnPrm.pIcfInitHandle = g_stFaceCommonPrm.pInitHandle;
    stInitEngineChnPrm.u32GraphId = SAE_FACE_GID_DET_FEATURE_1;
    stInitEngineChnPrm.u32GraphType = SAE_GTYPE_FACE;
    stInitEngineChnPrm.u32PostNodeId = SAE_FACE_NID_DET_FEATURE_1_POST;
    stInitEngineChnPrm.pCallBackFunc = Face_DrvGetOutputResult0;
    stInitEngineChnPrm.pstAppParam = &stAppParamInfo;

    s32Ret = Face_HalInitEngineChannel(FACE_PICTURE_MODE, 0, &stInitEngineChnPrm);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("init engine channle failed! mode %d \n", FACE_PICTURE_MODE);
        goto exit;
    }

    /* 配置图片注册业务线的默认参数 */
    s32Ret = Face_HalSetPicRegisterDefaultConfig();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("init engine channle failed! mode %d \n", FACE_PICTURE_MODE);
        goto exit;
    }

    /* 图片注册业务全局缓存初始化 */
    s32Ret = Face_HalInitRegisterAnaData();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("init register ana data failed! \n");
        goto exit;
    }

    s32Ret = SAL_SOK;
    FACE_LOGI("init pic register line end! \n");

exit:
    /* 加载模型成功后释放动态申请模型内存 */
    if (stSaeFaceAbilityPrm.model_buff.pData)
    {
        SAL_memfree(stSaeFaceAbilityPrm.model_buff.pData, "FACE", "MODEL_BUFF");
        stSaeFaceAbilityPrm.model_buff.pData = NULL;
    }

    return s32Ret;
}

/**
 * @function   Face_HalInitLoginAnaData
 * @brief      初始化人脸登录业务全局缓存数据(视频流)
 * @param[in]  VOID
 * @param[out] None
 * @return     static
 */
static INT32 Face_HalInitLoginAnaData(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;

    FACE_ANA_BUF_INFO *pstVideoInputData = NULL;
    ALLOC_VB_INFO_S stVbInfo = {0};

    pstVideoInputData = Face_HalGetAnaDataTab(FACE_VIDEO_LOGIN_MODE);
    if (NULL == pstVideoInputData)
    {
        FACE_LOGE("get login video global prm failed! mode %d \n", FACE_VIDEO_LOGIN_MODE);
        return SAL_FAIL;
    }

    /* 人脸登录业务最大缓存个数为16 */
    pstVideoInputData->uiMaxBufNum = FACE_INPUT_DATA_NUM;

    for (i = 0; i < pstVideoInputData->uiMaxBufNum; i++)
    {
        pstVideoInputData->stFaceBufData[i].pUseFlag[0] = (int *)&pstVideoInputData->uiRlsFlag[i];

        pstVideoInputData->stFaceBufData[i].nBlobNum = 1;
        pstVideoInputData->stFaceBufData[i].stBlobData[0].nShape[0] = FACE_LOGIN_IMG_WIDTH;
        pstVideoInputData->stFaceBufData[i].stBlobData[0].nShape[1] = FACE_LOGIN_IMG_HEIGHT;
        pstVideoInputData->stFaceBufData[i].stBlobData[0].eBlobFormat = ICF_INPUT_FORMAT_YUV_NV21;
        pstVideoInputData->stFaceBufData[i].stBlobData[0].nFrameNum = 0;
        pstVideoInputData->stFaceBufData[i].stBlobData[0].pData = &(pstVideoInputData->stFaceData[i]);

        /* 输入数据 */
        pstVideoInputData->stFaceData[i].reserved[0] = 0;
        pstVideoInputData->stFaceData[i].priv_data = NULL;
        pstVideoInputData->stFaceData[i].priv_data_size = 0;

        /* 数据优先级配置 */
        pstVideoInputData->stFaceData[i].data_priority.det_priority = SAE_FACE_PROC_PRIO_TYPE_HIGH;
        pstVideoInputData->stFaceData[i].data_priority.feat_priority = SAE_FACE_PROC_PRIO_TYPE_HIGH; /* 实际上这个流程没有建模，所以配置低或者不配都可以，但是在建模活体会用到这个（demo这么写的） */

        /* 指定主体光源 用于跟踪、属性、建模等 */
        pstVideoInputData->stFaceData[i].img_type_info.proc_main_type = SAE_FACE_IMG_TYPE_RGB;

        /* 指定 data_info 索引0,1上的光源有效性及类型描述 */
        pstVideoInputData->stFaceData[i].img_type_info.img_type_describe[0].img_type = SAE_FACE_IMG_TYPE_RGB;
        pstVideoInputData->stFaceData[i].img_type_info.img_type_describe[0].valid = 1;

        /* 指定 data_info 索引2,3上的光源有效性及类型描述 */
        pstVideoInputData->stFaceData[i].img_type_info.img_type_describe[1].img_type = SAE_FACE_IMG_TYPE_IR;
        pstVideoInputData->stFaceData[i].img_type_info.img_type_describe[1].valid = 0;

        s32Ret = mem_hal_vbAlloc(FACE_CAP_IMG_WIDTH * FACE_CAP_IMG_HEIGHT * 3 / 2,
                                 "FACE", "mmz_with_cache", NULL,
                                 SAL_TRUE,
                                 &stVbInfo);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("MmzAlloc Failed!!ret is 0x%x\n", s32Ret);
            return SAL_FAIL;
        }

        /* 光源图像赋值 */
        pstVideoInputData->stFaceData[i].data_info[0].yuv_data.format = SAE_VCA_YUV420;
        pstVideoInputData->stFaceData[i].data_info[0].yuv_data.scale_rate = 1;
        pstVideoInputData->stFaceData[i].data_info[0].frame_num = 0;
        pstVideoInputData->stFaceData[i].data_info[0].time_stamp = 0;

        /* 填充默认ROI参数，默认全屏 */
        pstVideoInputData->stFaceData[i].data_info[0].roi_rect.x = 0;
        pstVideoInputData->stFaceData[i].data_info[0].roi_rect.y = 0;
        pstVideoInputData->stFaceData[i].data_info[0].roi_rect.width = 1;
        pstVideoInputData->stFaceData[i].data_info[0].roi_rect.height = 1;

        pstVideoInputData->stFaceData[i].data_info[0].yuv_data.image_w = FACE_CAP_IMG_WIDTH;
        pstVideoInputData->stFaceData[i].data_info[0].yuv_data.image_h = FACE_CAP_IMG_HEIGHT;
        pstVideoInputData->stFaceData[i].data_info[0].yuv_data.pitch_y = FACE_CAP_IMG_WIDTH;
        pstVideoInputData->stFaceData[i].data_info[0].yuv_data.pitch_uv = FACE_CAP_IMG_WIDTH;
        pstVideoInputData->stFaceData[i].data_info[0].yuv_data.y = (unsigned char *)stVbInfo.pVirAddr;
        pstVideoInputData->stFaceData[i].data_info[0].yuv_data.u = (unsigned char *)stVbInfo.pVirAddr + FACE_CAP_IMG_WIDTH * FACE_CAP_IMG_HEIGHT;
        pstVideoInputData->stFaceData[i].data_info[0].yuv_data.v = (unsigned char *)pstVideoInputData->stFaceData[i].data_info[0].yuv_data.u;
    }

    FACE_LOGI("login data init end! \n");
    return SAL_SOK;
}

/**
 * @function   Face_HalSetVideoLoginFeatureNodeDefaultConfig
 * @brief      人脸登录业务-建模节点的默认配置
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalSetVideoLoginFeatureNodeDefaultConfig(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    ICF_CONFIG_PARAM_V2 configParam = {0};

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_HAL_CHECK_PTR(pstFaceHalComm, err, "pstFaceHalComm == null!");

    /* 抓拍模式选帧的默认配置 */
    {
        SAE_FACE_CFG_QLTY_THRSH_T stCfgQltyThrsh = {0};

        stCfgQltyThrsh.qty_thresh.landmark_confidence = 0.0;
        stCfgQltyThrsh.qty_thresh.detect_orientation = 0;
        stCfgQltyThrsh.qty_thresh.eye_distance = 0.0;
        stCfgQltyThrsh.qty_thresh.color_confidence = 0.0;
        stCfgQltyThrsh.qty_thresh.gray_scale = 0;
        stCfgQltyThrsh.qty_thresh.gray_mean_range.low = 0;
        stCfgQltyThrsh.qty_thresh.gray_mean_range.high = 255.0;
        stCfgQltyThrsh.qty_thresh.gray_variance_range.low = 0.0;
        stCfgQltyThrsh.qty_thresh.gray_variance_range.high = 128.0;
        stCfgQltyThrsh.qty_thresh.clearity_score = 0.0f;
        stCfgQltyThrsh.qty_thresh.pose_pitch = 90;
        stCfgQltyThrsh.qty_thresh.pose_yaw = 90;
        stCfgQltyThrsh.qty_thresh.pose_roll = 90;
        stCfgQltyThrsh.qty_thresh.pose_confidence = 0.0;
        stCfgQltyThrsh.qty_thresh.frontal_score = 0.0f;
        stCfgQltyThrsh.qty_thresh.visible_score = 0.0f;
        stCfgQltyThrsh.qty_thresh.face_score = 0.0f;

        memset(&configParam, 0, sizeof(ICF_CONFIG_PARAM_V2));

        configParam.nNodeID	= SAE_FACE_NID_DET_FEATURE_LOG_DFR_QUALITY;
        configParam.nKey = SAE_FACE_CFG_FACE_QUALITY_THRSH;
        configParam.pConfigData	= &stCfgQltyThrsh;
        configParam.nConfSize = sizeof(SAE_FACE_CFG_QLTY_THRSH_T);

        s32Ret = pstFaceHalComm->stIcfFuncP.IcfSetConfig(pstFaceHalComm->astProcLineHandle[FACE_VIDEO_LOGIN_MODE].stIcfCreateHandle[1].pChannelHandle,
                                                         &configParam,
                                                         sizeof(ICF_CONFIG_PARAM_V2));
        FACE_HAL_CHECK_RET(SAL_SOK != s32Ret, err, "ICF_Set_config SAE_FACE_CFG_FD_SELECT_FRAME_SETTING failed!");
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Face_HalSetVideoLoginSelNodeDefaultConfig
 * @brief      人脸登录业务线-选帧节点的默认配置
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalSetVideoLoginSelNodeDefaultConfig(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    ICF_CONFIG_PARAM_V2 configParam = {0};

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_HAL_CHECK_PTR(pstFaceHalComm, err, "pstFaceHalComm == null!");

    /* 抓拍模式选帧的默认配置 */
    {
        SAE_FACE_CFG_SELECT_FRAME_PARAM_T stSelectFrameParam = {0};

        if (1)
        {
            /* 间隔抓拍，间隔之间独立，不限制目标抓拍次数 */
            stSelectFrameParam.reset_flag = 1;                                          /* 配置时，清空内部缓存，重新开始 */
            stSelectFrameParam.select_gap = 5;                                          /* 每 gap 帧抓拍期间最优的目标 */
            stSelectFrameParam.capture_num_max = 2147483647;                            /* 最大的int32的数，表示不限制抓拍数量 */
            stSelectFrameParam.quality_comp_thresh = -2.0;                              /* 与历史最优的比较阈值，-2.0表示不比较历史最优 */
            stSelectFrameParam.quality_thresh = 0.6;                                    /* 达到抓拍的最低评分阈值 */
            stSelectFrameParam.crop_rect_x = 3;                                         /* 抠图外扩比例 */
            stSelectFrameParam.crop_rect_y_down = 1;                                        /* 抠图外扩比例 */
            stSelectFrameParam.crop_rect_y_up = 3;                                      /* 抠图外扩比例 */
        }
        else
        {
            /* 间隔抓拍，且质量递进，且限制目标抓拍数 */
            stSelectFrameParam.reset_flag = 1;                                  /* 配置时，清空内部缓存，重新开始 */
            stSelectFrameParam.select_gap = 5;                                  /* 每 gap 帧抓拍期间最优的目标 */
            stSelectFrameParam.capture_num_max = 1;                             /* 一个目标最多抓拍一次 */
            stSelectFrameParam.quality_comp_thresh = 0;                         /* 与历史最优的比较阈值，质量递进 */
            stSelectFrameParam.quality_thresh = 0.6;                            /* 达到抓拍的最低评分阈值 */
            stSelectFrameParam.crop_rect_x = 3;                                 /* 抠图外扩比例 */
            stSelectFrameParam.crop_rect_y_down = 1;                                /* 抠图外扩比例 */
            stSelectFrameParam.crop_rect_y_up = 3;                              /* 抠图外扩比例 */
        }

        configParam.nNodeID	= SAE_FACE_NID_TRACK_SELECT_1_SELECT_FRAME;
        configParam.nKey = SAE_FACE_CFG_FD_SELECT_FRAME_SETTING;
        configParam.pConfigData = &stSelectFrameParam;
        configParam.nConfSize = sizeof(SAE_FACE_CFG_SELECT_FRAME_PARAM_T);

        s32Ret = pstFaceHalComm->stIcfFuncP.IcfSetConfig(pstFaceHalComm->astProcLineHandle[FACE_VIDEO_LOGIN_MODE].stIcfCreateHandle[0].pChannelHandle,
                                                         &configParam,
                                                         sizeof(ICF_CONFIG_PARAM_V2));
        FACE_HAL_CHECK_RET(SAL_SOK != s32Ret, err, "ICF_Set_config SAE_FACE_CFG_FD_SELECT_FRAME_SETTING failed!");
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Face_HalInitLoginNode1_Select
 * @brief      人脸登录业务线_选帧节点初始化
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalInitLoginNode1_Select(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    SAE_FACE_ABILITY_PARAM_T stSaeFaceAbilityPrm = {0};
    ICF_APP_PARAM_INFO_V2 stAppParamInfo = {0};

    FACE_INIT_ENGINE_CHANNEL_PRM_S stInitEngineChnPrm = {0};

    /* fixme: 平台类型为RK3588 */
    stSaeFaceAbilityPrm.platform.type = 4;

    stSaeFaceAbilityPrm.dfr_detect.enable = 1;
    stSaeFaceAbilityPrm.fd_track.enable = 1;
    stSaeFaceAbilityPrm.fd_quality.enable = 1;
    stSaeFaceAbilityPrm.select_frame.enable = 1;

    stSaeFaceAbilityPrm.dfr_detect.max_width = 1920;
    stSaeFaceAbilityPrm.dfr_detect.max_height = 1080;
    stSaeFaceAbilityPrm.dfr_detect.max_face_num = 10;
    stSaeFaceAbilityPrm.fd_track.max_width = 1920;
    stSaeFaceAbilityPrm.fd_track.max_height = 1080;
    stSaeFaceAbilityPrm.fd_quality.max_width = 1920;
    stSaeFaceAbilityPrm.fd_quality.max_height = 1080;
    stSaeFaceAbilityPrm.select_frame.max_width = 1920;
    stSaeFaceAbilityPrm.select_frame.max_height = 1080;
    stSaeFaceAbilityPrm.select_frame.sel_type = 0;                 /* 预留 */
    stSaeFaceAbilityPrm.select_frame.track_num_max = 5;            /* 最大跟踪人数 */
    stSaeFaceAbilityPrm.select_frame.crop_flag = 1;                /* 是否开启抠图 [0 不开启 1开启] */
    stSaeFaceAbilityPrm.select_frame.crop_queue_len = 7;           /* 抠图队列长度 */
    stSaeFaceAbilityPrm.select_frame.crop_image_w_max = 500;       /* 抠图能力集 宽 */
    stSaeFaceAbilityPrm.select_frame.crop_image_h_max = 500;       /* 抠图能力集 高 */

    /* 模型路径 */
    snprintf(stSaeFaceAbilityPrm.fd_track.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", FD_TRACK_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.fd_quality.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", FD_QUALITY_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_detect.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_DETECT_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_landmark.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_LANDMARK_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_quality.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_QUALITY_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_liveness.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_LIVENESS_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_attribute.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_ATTRIBUTE_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_feature.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_FEATURE_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_compare.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_COMPARE_MODEL_PATH);

    /* 加载模型需要外部申请malloc内存，load_model成功后需要释放这块内存 */
    stSaeFaceAbilityPrm.model_buff.nSize = 40 * 1024 * 1024;
    stSaeFaceAbilityPrm.model_buff.pData = SAL_memZalloc(stSaeFaceAbilityPrm.model_buff.nSize, "FACE", "MODEL_BUFF");
    if (NULL == stSaeFaceAbilityPrm.model_buff.pData)
    {
        FACE_LOGE("malloc model buff failed! \n");
        goto exit;
    }

    stAppParamInfo.stAppParamCfgBuff.pBuff = &stSaeFaceAbilityPrm;
    stAppParamInfo.stAppParamCfgBuff.nBuffSize = sizeof(SAE_FACE_ABILITY_PARAM_T);

    /* 创建人脸登录业务线，节点一: 选帧 */
    {
        stInitEngineChnPrm.pIcfInitHandle = g_stFaceCommonPrm.pInitHandle;
        stInitEngineChnPrm.u32GraphId = SAE_FACE_GID_TRACK_SELECT_1;
        stInitEngineChnPrm.u32GraphType = SAE_GTYPE_FACE;
        stInitEngineChnPrm.u32PostNodeId = SAE_FACE_NID_TRACK_SELECT_1_POST;
        stInitEngineChnPrm.pCallBackFunc = Face_DrvGetOutputResult1;
        stInitEngineChnPrm.pstAppParam = &stAppParamInfo;

        s32Ret = Face_HalInitEngineChannel(FACE_VIDEO_LOGIN_MODE, 0, &stInitEngineChnPrm);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("init engine channle failed! mode %d \n", FACE_VIDEO_LOGIN_MODE);
            goto exit;
        }
    }

    /* 配置人脸登录业务线选帧节点的默认参数 */
    s32Ret = Face_HalSetVideoLoginSelNodeDefaultConfig();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("set video login select node default config failed! mode %d \n", FACE_VIDEO_LOGIN_MODE);
        goto exit;
    }

    FACE_LOGI("init video login node1 select end! \n");

exit:
    /* 加载模型成功后释放动态申请模型内存 */
    if (stSaeFaceAbilityPrm.model_buff.pData)
    {
        SAL_memfree(stSaeFaceAbilityPrm.model_buff.pData, "FACE", "MODEL_BUFF");
        stSaeFaceAbilityPrm.model_buff.pData = NULL;
    }

    return s32Ret;
}

/**
 * @function   Face_HalInitLoginNode2_Feature
 * @brief      人脸登录业务线_建模节点初始化
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalInitLoginNode2_Feature(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    SAE_FACE_ABILITY_PARAM_T stSaeFaceAbilityPrm = {0};
    ICF_APP_PARAM_INFO_V2 stAppParamInfo = {0};

    FACE_INIT_ENGINE_CHANNEL_PRM_S stInitEngineChnPrm = {0};

    /* fixme: 平台类型为RK3588 */
    stSaeFaceAbilityPrm.platform.type = 4;

    stSaeFaceAbilityPrm.dfr_detect.enable = 1;
    stSaeFaceAbilityPrm.dfr_landmark.enable = 1;
    stSaeFaceAbilityPrm.dfr_quality.enable = 1;
    stSaeFaceAbilityPrm.dfr_feature.enable = 1;

    stSaeFaceAbilityPrm.dfr_detect.max_width = 1920;
    stSaeFaceAbilityPrm.dfr_detect.max_height = 1080;
    stSaeFaceAbilityPrm.dfr_detect.max_face_num = 10;
    stSaeFaceAbilityPrm.fd_track.max_width = 1920;
    stSaeFaceAbilityPrm.fd_track.max_height = 1080;
    stSaeFaceAbilityPrm.fd_quality.max_width = 1920;
    stSaeFaceAbilityPrm.fd_quality.max_height = 1080;
    stSaeFaceAbilityPrm.dfr_liveness.max_width = 1920;
    stSaeFaceAbilityPrm.dfr_liveness.max_height = 1080;
    stSaeFaceAbilityPrm.dfr_compare.patch_num = 1;
    stSaeFaceAbilityPrm.dfr_compare.feat_dim = 272;
    stSaeFaceAbilityPrm.dfr_compare.head_length = 16;
    stSaeFaceAbilityPrm.dfr_compare.max_feat_num = 50000;
    stSaeFaceAbilityPrm.dfr_compare.top_n = 1;

    /* 模型路径 */
    snprintf(stSaeFaceAbilityPrm.fd_track.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", FD_TRACK_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.fd_quality.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", FD_QUALITY_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_detect.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_DETECT_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_landmark.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_LANDMARK_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_quality.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_QUALITY_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_liveness.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_LIVENESS_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_attribute.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_ATTRIBUTE_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_feature.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_FEATURE_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_compare.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_COMPARE_MODEL_PATH);

    /* 加载模型需要外部申请malloc内存，load_model成功后需要释放这块内存 */
    stSaeFaceAbilityPrm.model_buff.nSize = 60 * 1024 * 1024;
    stSaeFaceAbilityPrm.model_buff.pData = SAL_memZalloc(stSaeFaceAbilityPrm.model_buff.nSize, "FACE", "MODEL_BUFF");
    if (NULL == stSaeFaceAbilityPrm.model_buff.pData)
    {
        FACE_LOGE("malloc model buff failed! \n");
        goto exit;
    }

    stAppParamInfo.stAppParamCfgBuff.pBuff = &stSaeFaceAbilityPrm;
    stAppParamInfo.stAppParamCfgBuff.nBuffSize = sizeof(SAE_FACE_ABILITY_PARAM_T);

    /* 创建人脸登录业务线，节点二: 人脸属性和建模 */
    {
        stInitEngineChnPrm.pIcfInitHandle = g_stFaceCommonPrm.pInitHandle;
        stInitEngineChnPrm.u32GraphId = SAE_FACE_GID_DET_FEATURE_LOG;
        stInitEngineChnPrm.u32GraphType = SAE_GTYPE_FACE;
        stInitEngineChnPrm.u32PostNodeId = SAE_FACE_NID_DET_FEATURE_LOG_POST;
        stInitEngineChnPrm.pCallBackFunc = Face_DrvGetOutputResult2;
        stInitEngineChnPrm.pstAppParam = &stAppParamInfo;

        s32Ret = Face_HalInitEngineChannel(FACE_VIDEO_LOGIN_MODE, 1, &stInitEngineChnPrm);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("init engine channle failed! mode %d \n", FACE_VIDEO_LOGIN_MODE);
            goto exit;
        }
    }

    s32Ret = Face_HalSetVideoLoginFeatureNodeDefaultConfig();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("set video login feature node default config failed! mode %d \n", FACE_VIDEO_LOGIN_MODE);
        goto exit;
    }

    FACE_LOGI("init video login node2 feature end! \n");

exit:
    /* 加载模型成功后释放动态申请模型内存 */
    if (stSaeFaceAbilityPrm.model_buff.pData)
    {
        SAL_memfree(stSaeFaceAbilityPrm.model_buff.pData, "FACE", "MODEL_BUFF");
        stSaeFaceAbilityPrm.model_buff.pData = NULL;
    }

    return s32Ret;
}

/**
 * @function   Face_HalInitVideoLoginLine
 * @brief      初始化人脸登录业务线资源(视频流)
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalInitVideoLoginLine(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    /* 配置人脸登录业务线节点一，选帧 */
    s32Ret = Face_HalInitLoginNode1_Select();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("init login select node failed! mode %d \n", FACE_VIDEO_LOGIN_MODE);
        goto exit;
    }

    /* 配置人脸登录业务线节点二，建模 */
    s32Ret = Face_HalInitLoginNode2_Feature();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("init login feature node failed! mode %d \n", FACE_VIDEO_LOGIN_MODE);
        goto exit;
    }

    /* 人脸登录业务全局缓存初始化 */
    s32Ret = Face_HalInitLoginAnaData();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("init login ana data failed! \n");
        goto exit;
    }

    s32Ret = SAL_SOK;
    FACE_LOGI("init video login line end! \n");

exit:
    return s32Ret;
}

/**
 * @function   Face_HalSetVideoCapSelNodeDefaultConfig
 * @brief      人脸抓拍业务线的默认配置
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalSetVideoCapSelNodeDefaultConfig(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    ICF_CONFIG_PARAM_V2 configParam = {0};

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_HAL_CHECK_PTR(pstFaceHalComm, err, "pstFaceHalComm == null!");

    /* 抓拍模式选帧的默认配置 */
    {
        SAE_FACE_CFG_SELECT_FRAME_PARAM_T stSelectFrameParam = {0};

        if (1)
        {
            /* 间隔抓拍，间隔之间独立，不限制目标抓拍次数 */
            stSelectFrameParam.reset_flag = 1;                                  /* 配置时，清空内部缓存，重新开始 */
            stSelectFrameParam.select_gap = 3;                                  /* 每 gap 帧抓拍期间最优的目标 */
            stSelectFrameParam.capture_num_max = 10;                            /* 单个目标最大抓拍10次 */
            stSelectFrameParam.quality_comp_thresh = 0.05;                      /* 与历史最优的比较阈值，质量递进 */
            stSelectFrameParam.quality_thresh = 0.6;                            /* 达到抓拍的最低评分阈值 */
            stSelectFrameParam.crop_rect_x = 3;                                 /* 抠图外扩比例，以下巴x方向中心点为基准，外扩x倍w，下同 */
            stSelectFrameParam.crop_rect_y_down = 3;                            /* 抠图外扩比例 */
            stSelectFrameParam.crop_rect_y_up = 7;                              /* 抠图外扩比例 */
        }
        else
        {
            /* 间隔抓拍，且质量递进，且限制目标抓拍数 */
            stSelectFrameParam.reset_flag = 1;                                  /* 配置时，清空内部缓存，重新开始 */
            stSelectFrameParam.select_gap = 5;                                  /* 每 gap 帧抓拍期间最优的目标 */
            stSelectFrameParam.capture_num_max = 1;                             /* 一个目标最多抓拍一次 */
            stSelectFrameParam.quality_comp_thresh = 0;                         /* 与历史最优的比较阈值，质量递进 */
            stSelectFrameParam.quality_thresh = 0.6;                            /* 达到抓拍的最低评分阈值 */
            stSelectFrameParam.crop_rect_x = 5;                                 /* 抠图外扩比例 */
            stSelectFrameParam.crop_rect_y_down = 1;                            /* 抠图外扩比例 */
            stSelectFrameParam.crop_rect_y_up = 5;                              /* 抠图外扩比例 */
        }

        configParam.nNodeID	= SAE_FACE_NID_TRACK_SELECT_SELECT_FRAME;
        configParam.nKey = SAE_FACE_CFG_FD_SELECT_FRAME_SETTING;
        configParam.pConfigData = &stSelectFrameParam;
        configParam.nConfSize = sizeof(SAE_FACE_CFG_SELECT_FRAME_PARAM_T);

        s32Ret = pstFaceHalComm->stIcfFuncP.IcfSetConfig(pstFaceHalComm->astProcLineHandle[FACE_VIDEO_CAP_MODE].stIcfCreateHandle[0].pChannelHandle,
                                                         &configParam,
                                                         sizeof(ICF_CONFIG_PARAM_V2));
        FACE_HAL_CHECK_RET(SAL_SOK != s32Ret, err, "ICF_Set_config SAE_FACE_CFG_FD_SELECT_FRAME_SETTING failed!");
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Face_HalSetVideoCapFeatureNodeDefaultConfig
 * @brief      人脸抓拍业务线的默认配置
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalSetVideoCapFeatureNodeDefaultConfig(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    ICF_CONFIG_PARAM_V2 configParam = {0};

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_HAL_CHECK_PTR(pstFaceHalComm, err, "pstFaceHalComm == null!");

    /* 抓拍模式选帧的默认配置 */
    {
        SAE_FACE_CFG_QLTY_THRSH_T stCfgQltyThrsh = {0};

        stCfgQltyThrsh.qty_thresh.landmark_confidence = 0.0;
        stCfgQltyThrsh.qty_thresh.detect_orientation = 0;
        stCfgQltyThrsh.qty_thresh.eye_distance = 0.0;
        stCfgQltyThrsh.qty_thresh.color_confidence = 0.0;
        stCfgQltyThrsh.qty_thresh.gray_scale = 0;
        stCfgQltyThrsh.qty_thresh.gray_mean_range.low = 0;
        stCfgQltyThrsh.qty_thresh.gray_mean_range.high = 255.0;
        stCfgQltyThrsh.qty_thresh.gray_variance_range.low = 0.0;
        stCfgQltyThrsh.qty_thresh.gray_variance_range.high = 128.0;
        stCfgQltyThrsh.qty_thresh.clearity_score = 0.0f;
        stCfgQltyThrsh.qty_thresh.pose_pitch = 90;
        stCfgQltyThrsh.qty_thresh.pose_yaw = 90;
        stCfgQltyThrsh.qty_thresh.pose_roll = 90;
        stCfgQltyThrsh.qty_thresh.pose_confidence = 0.0;
        stCfgQltyThrsh.qty_thresh.frontal_score = 0.0f;
        stCfgQltyThrsh.qty_thresh.visible_score = 0.0f;
        stCfgQltyThrsh.qty_thresh.face_score = 0.1f;

        memset(&configParam, 0, sizeof(ICF_CONFIG_PARAM_V2));

        configParam.nNodeID	= SAE_FACE_NID_DET_FEATURE_CAP_DFR_QUALITY;
        configParam.nKey = SAE_FACE_CFG_FACE_QUALITY_THRSH;
        configParam.pConfigData	= &stCfgQltyThrsh;
        configParam.nConfSize = sizeof(SAE_FACE_CFG_QLTY_THRSH_T);

        s32Ret = pstFaceHalComm->stIcfFuncP.IcfSetConfig(pstFaceHalComm->astProcLineHandle[FACE_VIDEO_CAP_MODE].stIcfCreateHandle[1].pChannelHandle,
                                                         &configParam,
                                                         sizeof(ICF_CONFIG_PARAM_V2));
        FACE_HAL_CHECK_RET(SAL_SOK != s32Ret, err, "ICF_Set_config SAE_FACE_CFG_FD_SELECT_FRAME_SETTING failed!");
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Face_HalInitVideoCapAnaData
 * @brief      初始化人脸抓拍业务全局缓存数据(视频流)
 * @param[in]  VOID
 * @param[out] None
 * @return     static
 */
static INT32 Face_HalInitVideoCapAnaData(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;

    FACE_ANA_BUF_INFO *pstVideoCapInputData = NULL;
    ALLOC_VB_INFO_S stVbInfo = {0};

    pstVideoCapInputData = Face_HalGetAnaDataTab(FACE_VIDEO_CAP_MODE);
    if (NULL == pstVideoCapInputData)
    {
        FACE_LOGE("get video cap global prm failed! mode %d \n", FACE_VIDEO_CAP_MODE);
        return SAL_FAIL;
    }

    /* 人脸登录业务最大缓存个数为16 */
    pstVideoCapInputData->uiMaxBufNum = FACE_INPUT_DATA_NUM;

    for (i = 0; i < pstVideoCapInputData->uiMaxBufNum; i++)
    {
        pstVideoCapInputData->stFaceBufData[i].pUseFlag[0] = (int *)&pstVideoCapInputData->uiRlsFlag[i];

        pstVideoCapInputData->stFaceBufData[i].nBlobNum = 1;
        pstVideoCapInputData->stFaceBufData[i].stBlobData[0].nShape[0] = FACE_CAP_IMG_WIDTH;
        pstVideoCapInputData->stFaceBufData[i].stBlobData[0].nShape[1] = FACE_CAP_IMG_HEIGHT;
        pstVideoCapInputData->stFaceBufData[i].stBlobData[0].eBlobFormat = ICF_INPUT_FORMAT_YUV_NV21;
        pstVideoCapInputData->stFaceBufData[i].stBlobData[0].nFrameNum = 0;
        pstVideoCapInputData->stFaceBufData[i].stBlobData[0].pData = &(pstVideoCapInputData->stFaceData[i]);

        /* 输入数据 */
        pstVideoCapInputData->stFaceData[i].reserved[0] = 0;
        pstVideoCapInputData->stFaceData[i].priv_data = NULL;
        pstVideoCapInputData->stFaceData[i].priv_data_size = 0;

        /* 数据优先级配置 */
        pstVideoCapInputData->stFaceData[i].data_priority.det_priority = SAE_FACE_PROC_PRIO_TYPE_LOW;
        pstVideoCapInputData->stFaceData[i].data_priority.feat_priority = SAE_FACE_PROC_PRIO_TYPE_LOW; /* 实际上这个流程没有建模，所以配置低或者不配都可以，但是在建模活体会用到这个（demo这么写的） */

        /* 指定主体光源 用于跟踪、属性、建模等 */
        pstVideoCapInputData->stFaceData[i].img_type_info.proc_main_type = SAE_FACE_IMG_TYPE_RGB;

        /* 指定 data_info 索引0,1上的光源有效性及类型描述 */
        pstVideoCapInputData->stFaceData[i].img_type_info.img_type_describe[0].img_type = SAE_FACE_IMG_TYPE_RGB;
        pstVideoCapInputData->stFaceData[i].img_type_info.img_type_describe[0].valid = 1;

        /* 指定 data_info 索引2,3上的光源有效性及类型描述 */
        pstVideoCapInputData->stFaceData[i].img_type_info.img_type_describe[1].img_type = SAE_FACE_IMG_TYPE_IR;
        pstVideoCapInputData->stFaceData[i].img_type_info.img_type_describe[1].valid = 0;

        s32Ret = mem_hal_vbAlloc(FACE_CAP_IMG_WIDTH * FACE_CAP_IMG_HEIGHT * 3 / 2,
                                 "FACE", "mmz_with_cache", NULL,
                                 SAL_TRUE,
                                 &stVbInfo);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("MmzAlloc Failed!!ret is 0x%x\n", s32Ret);
            return SAL_FAIL;
        }

        /* 光源图像赋值 */
        pstVideoCapInputData->stFaceData[i].data_info[0].yuv_data.format = SAE_VCA_YUV420;
        pstVideoCapInputData->stFaceData[i].data_info[0].yuv_data.scale_rate = 1;
        pstVideoCapInputData->stFaceData[i].data_info[0].frame_num = 0;
        pstVideoCapInputData->stFaceData[i].data_info[0].time_stamp = 0;

        /* 填充默认ROI参数，默认全屏 */
        pstVideoCapInputData->stFaceData[i].data_info[0].roi_rect.x = 0;
        pstVideoCapInputData->stFaceData[i].data_info[0].roi_rect.y = 0;
        pstVideoCapInputData->stFaceData[i].data_info[0].roi_rect.width = 1;
        pstVideoCapInputData->stFaceData[i].data_info[0].roi_rect.height = 1;

        pstVideoCapInputData->stFaceData[i].data_info[0].yuv_data.image_w = FACE_CAP_IMG_WIDTH;
        pstVideoCapInputData->stFaceData[i].data_info[0].yuv_data.image_h = FACE_CAP_IMG_HEIGHT;
        pstVideoCapInputData->stFaceData[i].data_info[0].yuv_data.pitch_y = FACE_CAP_IMG_WIDTH;
        pstVideoCapInputData->stFaceData[i].data_info[0].yuv_data.pitch_uv = FACE_CAP_IMG_WIDTH;
        pstVideoCapInputData->stFaceData[i].data_info[0].yuv_data.y = (unsigned char *)stVbInfo.pVirAddr;
        pstVideoCapInputData->stFaceData[i].data_info[0].yuv_data.u = (unsigned char *)stVbInfo.pVirAddr + FACE_CAP_IMG_WIDTH * FACE_CAP_IMG_HEIGHT;
        pstVideoCapInputData->stFaceData[i].data_info[0].yuv_data.v = (unsigned char *)pstVideoCapInputData->stFaceData[i].data_info[0].yuv_data.u;
    }

    FACE_LOGI("video cap data init end! \n");
    return SAL_SOK;
}

/**
 * @function   Face_HalInitVideoCapNode1_Select
 * @brief      人脸抓拍业务线_选帧节点初始化
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalInitVideoCapNode1_Select(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    SAE_FACE_ABILITY_PARAM_T stSaeFaceAbilityPrm = {0};
    ICF_APP_PARAM_INFO_V2 stAppParamInfo = {0};

    FACE_INIT_ENGINE_CHANNEL_PRM_S stInitEngineChnPrm = {0};

    /* fixme: 平台类型为RK3588 */
    stSaeFaceAbilityPrm.platform.type = 4;

    stSaeFaceAbilityPrm.dfr_detect.enable = 1;
    stSaeFaceAbilityPrm.fd_track.enable = 1;
    stSaeFaceAbilityPrm.fd_quality.enable = 1;
    stSaeFaceAbilityPrm.select_frame.enable = 1;

    stSaeFaceAbilityPrm.dfr_detect.max_width = 1920;
    stSaeFaceAbilityPrm.dfr_detect.max_height = 1080;
    stSaeFaceAbilityPrm.dfr_detect.max_face_num = 10;
    stSaeFaceAbilityPrm.fd_track.max_width = 1920;
    stSaeFaceAbilityPrm.fd_track.max_height = 1080;
    stSaeFaceAbilityPrm.fd_quality.max_width = 1920;
    stSaeFaceAbilityPrm.fd_quality.max_height = 1080;
    stSaeFaceAbilityPrm.select_frame.max_width = 1920;
    stSaeFaceAbilityPrm.select_frame.max_height = 1080;
    stSaeFaceAbilityPrm.select_frame.sel_type = 0;                 /* 预留 */
    stSaeFaceAbilityPrm.select_frame.track_num_max = 5;            /* 最大跟踪人数 */
    stSaeFaceAbilityPrm.select_frame.crop_flag = 1;                /* 是否开启抠图 [0 不开启 1开启] */
    stSaeFaceAbilityPrm.select_frame.crop_queue_len = 7;           /* 抠图队列长度 */
    stSaeFaceAbilityPrm.select_frame.crop_image_w_max = 500;       /* 抠图能力集 宽 */
    stSaeFaceAbilityPrm.select_frame.crop_image_h_max = 500;       /* 抠图能力集 高 */

    /* 模型路径 */
    snprintf(stSaeFaceAbilityPrm.fd_track.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", FD_TRACK_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.fd_quality.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", FD_QUALITY_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_detect.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_DETECT_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_landmark.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_LANDMARK_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_quality.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_QUALITY_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_liveness.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_LIVENESS_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_attribute.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_ATTRIBUTE_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_feature.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_FEATURE_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_compare.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_COMPARE_MODEL_PATH);

    /* 加载模型需要外部申请malloc内存，load_model成功后需要释放这块内存 */
    stSaeFaceAbilityPrm.model_buff.nSize = 40 * 1024 * 1024;
    stSaeFaceAbilityPrm.model_buff.pData = SAL_memZalloc(stSaeFaceAbilityPrm.model_buff.nSize, "FACE", "MODEL_BUFF");
    if (NULL == stSaeFaceAbilityPrm.model_buff.pData)
    {
        FACE_LOGE("malloc model buff failed! \n");
        goto exit;
    }

    stAppParamInfo.stAppParamCfgBuff.pBuff = &stSaeFaceAbilityPrm;
    stAppParamInfo.stAppParamCfgBuff.nBuffSize = sizeof(SAE_FACE_ABILITY_PARAM_T);

    /* 创建人脸抓拍业务线，节点一: 选帧 */
    {
        stInitEngineChnPrm.pIcfInitHandle = g_stFaceCommonPrm.pInitHandle;
        stInitEngineChnPrm.u32GraphId = SAE_FACE_GID_TRACK_SELECT;
        stInitEngineChnPrm.u32GraphType = SAE_GTYPE_FACE;
        stInitEngineChnPrm.u32PostNodeId = SAE_FACE_NID_TRACK_SELECT_POST;
        stInitEngineChnPrm.pCallBackFunc = Face_DrvGetOutputResult3;
        stInitEngineChnPrm.pstAppParam = &stAppParamInfo;

        s32Ret = Face_HalInitEngineChannel(FACE_VIDEO_CAP_MODE, 0, &stInitEngineChnPrm);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("init engine channle failed! mode %d \n", FACE_VIDEO_CAP_MODE);
            goto exit;
        }
    }

    /* 配置人脸抓拍业务线选帧节点的默认参数 */
    s32Ret = Face_HalSetVideoCapSelNodeDefaultConfig();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("set video capture select node default config failed! mode %d \n", FACE_VIDEO_CAP_MODE);
        goto exit;
    }

    FACE_LOGI("init video cap node1 select end! \n");

exit:
    /* 加载模型成功后释放动态申请模型内存 */
    if (stSaeFaceAbilityPrm.model_buff.pData)
    {
        SAL_memfree(stSaeFaceAbilityPrm.model_buff.pData, "FACE", "MODEL_BUFF");
        stSaeFaceAbilityPrm.model_buff.pData = NULL;
    }

    return s32Ret;
}

/**
 * @function   Face_HalInitVideoCapNode2_Feature
 * @brief      人脸抓拍业务线_建模节点初始化
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalInitVideoCapNode2_Feature(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    SAE_FACE_ABILITY_PARAM_T stSaeFaceAbilityPrm = {0};
    ICF_APP_PARAM_INFO_V2 stAppParamInfo = {0};

    FACE_INIT_ENGINE_CHANNEL_PRM_S stInitEngineChnPrm = {0};

    /* fixme: 平台类型为RK3588 */
    stSaeFaceAbilityPrm.platform.type = 4;

    stSaeFaceAbilityPrm.dfr_detect.enable = 1;
    stSaeFaceAbilityPrm.dfr_landmark.enable = 1;
    stSaeFaceAbilityPrm.dfr_quality.enable = 1;
    stSaeFaceAbilityPrm.dfr_feature.enable = 1;

    stSaeFaceAbilityPrm.dfr_detect.max_width = 1920;
    stSaeFaceAbilityPrm.dfr_detect.max_height = 1080;
    stSaeFaceAbilityPrm.dfr_detect.max_face_num = 10;
    stSaeFaceAbilityPrm.fd_track.max_width = 1920;
    stSaeFaceAbilityPrm.fd_track.max_height = 1080;
    stSaeFaceAbilityPrm.fd_quality.max_width = 1920;
    stSaeFaceAbilityPrm.fd_quality.max_height = 1080;
    stSaeFaceAbilityPrm.dfr_liveness.max_width = 1920;
    stSaeFaceAbilityPrm.dfr_liveness.max_height = 1080;
    stSaeFaceAbilityPrm.dfr_compare.patch_num = 1;
    stSaeFaceAbilityPrm.dfr_compare.feat_dim = 272;
    stSaeFaceAbilityPrm.dfr_compare.head_length = 16;
    stSaeFaceAbilityPrm.dfr_compare.max_feat_num = 50000;
    stSaeFaceAbilityPrm.dfr_compare.top_n = 1;

    /* 模型路径 */
    snprintf(stSaeFaceAbilityPrm.fd_track.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", FD_TRACK_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.fd_quality.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", FD_QUALITY_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_detect.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_DETECT_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_landmark.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_LANDMARK_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_quality.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_QUALITY_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_liveness.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_LIVENESS_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_attribute.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_ATTRIBUTE_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_feature.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_FEATURE_MODEL_PATH);
    snprintf(stSaeFaceAbilityPrm.dfr_compare.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_COMPARE_MODEL_PATH);

    /* 加载模型需要外部申请malloc内存，load_model成功后需要释放这块内存 */
    stSaeFaceAbilityPrm.model_buff.nSize = 60 * 1024 * 1024;
    stSaeFaceAbilityPrm.model_buff.pData = SAL_memZalloc(stSaeFaceAbilityPrm.model_buff.nSize, "FACE", "MODEL_BUFF");
    if (NULL == stSaeFaceAbilityPrm.model_buff.pData)
    {
        FACE_LOGE("malloc model buff failed! \n");
        goto exit;
    }

    stAppParamInfo.stAppParamCfgBuff.pBuff = &stSaeFaceAbilityPrm;
    stAppParamInfo.stAppParamCfgBuff.nBuffSize = sizeof(SAE_FACE_ABILITY_PARAM_T);

    /* 创建人脸抓拍业务线，节点二: 人脸属性和建模 */
    {
        stInitEngineChnPrm.pIcfInitHandle = g_stFaceCommonPrm.pInitHandle;
        stInitEngineChnPrm.u32GraphId = SAE_FACE_GID_DET_FEATURE_CAP;
        stInitEngineChnPrm.u32GraphType = SAE_GTYPE_FACE;
        stInitEngineChnPrm.u32PostNodeId = SAE_FACE_NID_DET_FEATURE_CAP_POST;
        stInitEngineChnPrm.pCallBackFunc = Face_DrvGetOutputResult4;
        stInitEngineChnPrm.pstAppParam = &stAppParamInfo;

        s32Ret = Face_HalInitEngineChannel(FACE_VIDEO_CAP_MODE, 1, &stInitEngineChnPrm);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("init engine channle failed! mode %d \n", FACE_VIDEO_CAP_MODE);
            goto exit;
        }
    }

    s32Ret = Face_HalSetVideoCapFeatureNodeDefaultConfig();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("set video capture feature node default config failed! mode %d \n", FACE_VIDEO_CAP_MODE);
        goto exit;
    }

    FACE_LOGI("init video cap node2 feature end! \n");

exit:
    /* 加载模型成功后释放动态申请模型内存 */
    if (stSaeFaceAbilityPrm.model_buff.pData)
    {
        SAL_memfree(stSaeFaceAbilityPrm.model_buff.pData, "FACE", "MODEL_BUFF");
        stSaeFaceAbilityPrm.model_buff.pData = NULL;
    }

    return s32Ret;
}

/**
 * @function   Face_HalInitVideoCapLine
 * @brief      初始化人脸抓拍业务线资源(视频流)
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalInitVideoCapLine(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    /* 人脸抓拍业务线需要两个处理节点，第一步为选帧 */
    s32Ret = Face_HalInitVideoCapNode1_Select();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("init video capture node 1 select failed! mode %d \n", FACE_VIDEO_CAP_MODE);
        goto exit;
    }

    /* 人脸抓拍业务线需要两个处理节点，第二步为建模 */
    s32Ret = Face_HalInitVideoCapNode2_Feature();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("init video capture node 2 feature failed! mode %d \n", FACE_VIDEO_CAP_MODE);
        goto exit;
    }

    /* 人脸抓拍业务全局缓存初始化 */
    s32Ret = Face_HalInitVideoCapAnaData();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("init register ana data failed! \n");
        goto exit;
    }

    s32Ret = SAL_SOK;
    FACE_LOGI("init video capture line end! \n");

exit:
    return s32Ret;
}

/**
 * @function   Face_HalInitExternCompareLine
 * @brief      初始化外部比对资源(1v1 1vN)
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_HalInitExternCompareLine(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    SAE_FACE_ABILITY_PARAM_T stSaeFaceAbilityPrm = {0};
    ICF_MEM_CONFIG stMemPoolConfig = { 0 };

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_HAL_CHECK_PTR(pstFaceHalComm, err, "pstFaceHalComm == null!");

    /* fixme: 平台类型为RK3588 */
    stSaeFaceAbilityPrm.platform.type = 4;

    stSaeFaceAbilityPrm.dfr_compare.patch_num = 1;
    stSaeFaceAbilityPrm.dfr_compare.feat_dim = 272;
    stSaeFaceAbilityPrm.dfr_compare.head_length = 16;
    stSaeFaceAbilityPrm.dfr_compare.max_feat_num = 50000;
    stSaeFaceAbilityPrm.dfr_compare.top_n = 1;

    /* 模型路径 */
    snprintf(stSaeFaceAbilityPrm.dfr_compare.model_bin, SAE_FACE_MAX_STRING_NUM, "%s", DFR_COMPARE_MODEL_PATH);

    /* 加载模型需要外部申请malloc内存，load_model成功后需要释放这块内存 */
    stSaeFaceAbilityPrm.model_buff.nSize = 100 * 1024;
    stSaeFaceAbilityPrm.model_buff.pData = SAL_memZalloc(stSaeFaceAbilityPrm.model_buff.nSize, "FACE", "MODEL_BUFF");
    if (NULL == stSaeFaceAbilityPrm.model_buff.pData)
    {
        FACE_LOGE("malloc model buff failed! \n");
        goto err;
    }

    /* 外部比对库未创建，则进行初始化，当前20M malloc类型的内存50M MMZ内存为引擎默认大小，无精确使用大小统计 */
    if (NULL == pstFaceHalComm->pMemPoolExtCmp)
    {
        stMemPoolConfig.nNum = 2;
        stMemPoolConfig.stMemInfo[0].eMemType = ICF_MEM_MALLOC;
        stMemPoolConfig.stMemInfo[0].nMemSize = 20 * 1024 * 1024;
        stMemPoolConfig.stMemInfo[1].eMemType = ICF_RN_MEM_MMZ_IOMMU_WITH_CACHE;
        stMemPoolConfig.stMemInfo[1].nMemSize = 50 * 1024 * 1024;
        s32Ret = MemPoolObjInit_V2(g_stFaceCommonPrm.pInitHandle, &stMemPoolConfig, &pstFaceHalComm->pMemPoolExtCmp);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("init register ana data failed! \n");
            goto err;
        }
    }

    if (NULL == pstFaceHalComm->pExternCompare)
    {
        s32Ret = SAE_FACE_DFR_Compare_Extern_Create(g_stFaceCommonPrm.pInitHandle,
                                                    pstFaceHalComm->pMemPoolExtCmp,
                                                    &stSaeFaceAbilityPrm,
                                                    &pstFaceHalComm->pExternCompare);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("create extern compare failed! ret: 0x%x \n", s32Ret);
            goto err;
        }
    }

    s32Ret = SAL_SOK;
    FACE_LOGI("init extern compare lib line end! \n");

    goto exit;

err:
    if (pstFaceHalComm->pExternCompare)
    {
        SAE_FACE_DFR_Compare_Extern_Destroy(pstFaceHalComm->pExternCompare);
        pstFaceHalComm->pExternCompare = NULL;
    }

    if (pstFaceHalComm->pMemPoolExtCmp)
    {
        MemPoolObjFinit_V2(pstFaceHalComm->pMemPoolExtCmp);
        pstFaceHalComm->pMemPoolExtCmp = NULL;
    }

exit:
    /* 加载模型成功后释放动态申请模型内存 */
    if (stSaeFaceAbilityPrm.model_buff.pData)
    {
        SAL_memfree(stSaeFaceAbilityPrm.model_buff.pData, "FACE", "MODEL_BUFF");
        stSaeFaceAbilityPrm.model_buff.pData = NULL;
    }

    return s32Ret;
}

/**
 * @function    Face_HalInit
 * @brief         算法整体初始化
 * @param[in]  NULL
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 Face_HalInit(void)
{
    /* 变量定义 */
    INT32 s32Ret = SAL_FAIL;

    /* 若算法资源已经初始化过，返回成功 */
    if (SAL_TRUE == g_stFaceCommonPrm.bInit)
    {
        FACE_LOGI("Resources of Face Algorithm is Inited! Return Success!\n");
        return SAL_SOK;
    }

    /* 初始化解密资源 */
    if (SAL_SOK != IA_InitEncrypt(g_stFaceCommonPrm.pAlgEncryptHdl))
    {
        FACE_LOGE("Init Encrypt Handle Failed!\n");
        return SAL_FAIL;
    }

    /* 初始化硬件调度资源 */
    if (SAL_SOK != IA_InitHwCore())
    {
        FACE_LOGE("Init Dsp Core Failed!\n");
        return SAL_FAIL;
    }

    g_stFaceCommonPrm.pSchedulerHdl = IA_GetScheHndl();
    if (NULL == g_stFaceCommonPrm.pSchedulerHdl)
    {
        FACE_LOGE("sche == nuLL\n");
        return SAL_FAIL;
    }

    /* 引擎框架初始化 */
    s32Ret = Face_HalIcfInit();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("ICF INIT FAILED!\n");
        return SAL_FAIL;
    }

    /* 初始化图片注册业务线 */
    s32Ret = Face_HalInitPicRegisterLine();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("ICF INIT FAILED!\n");
        return SAL_FAIL;
    }

    /* 初始化人脸登录业务线(视频流) */
    s32Ret = Face_HalInitVideoLoginLine();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("ICF INIT FAILED!\n");
        return SAL_FAIL;
    }

    /* 初始化人脸抓拍业务线(视频流) */
    s32Ret = Face_HalInitVideoCapLine();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("ICF INIT FAILED!\n");
        return SAL_FAIL;
    }

    /* 初始化人脸静态比对业务线(1v1 1vN) */
    s32Ret = Face_HalInitExternCompareLine();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("ICF INIT FAILED!\n");
        return SAL_FAIL;
    }

    /* 获取版本信息 */
    if (SAL_SOK != Face_HalGetVersion())
    {
        FACE_LOGE("Get Version Errrrrrrrrrrr!\n");
        return SAL_FAIL;
    }

    g_stFaceCommonPrm.bInit = SAL_TRUE;

    FACE_LOGI("Face Module: Init Resouce Ok!\n");
    return SAL_SOK;
}

/**
 * @function    Face_HalDeInit
 * @brief         算法整体去初始化
 * @param[in]  NULL
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 Face_HalDeInit(void)
{
    /* 变量定义 */
    UINT32 s32Ret = SAL_FAIL;
    UINT32 i = 0;

    /* 算法资源已经去初始化，返回成功 */
    if (SAL_FALSE == g_stFaceCommonPrm.bInit)
    {
        FACE_LOGI("Resource of Face Algorithm is DeInit! Return Success!\n");
        return SAL_SOK;
    }

    /* 去初始化解密资源 */
    s32Ret = IA_DeinitEncrypt(g_stFaceCommonPrm.pAlgEncryptHdl);
    if (SAL_SOK != s32Ret)
    {
        printf("encrypt_deinit fail ret=0x%x\n", s32Ret);
        return s32Ret;
    }

    g_stFaceCommonPrm.pAlgEncryptHdl = NULL;

    FACE_LOGI("encrypt deinit ok!\n");

    for (i = 0; i < g_stModelDataBase.uiMaxModelCnt; i++)
    {
        s32Ret = SAL_memfree(g_stModelDataBase.pFeatureData[i], "FACE", "Face DataBase");
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("i %d Rls DataBase Mem Failed! ret 0x%x\n", i, s32Ret);
            return SAL_FAIL;
        }
    }

    g_stModelDataBase.uiModelCnt = 0;
    g_stModelDataBase.uiMaxModelCnt = 0;
    for (i = 0; i < g_stModelDataBase.uiMaxModelCnt; i++)
    {
        g_stModelDataBase.pFeatureData[i] = NULL;
    }

    FACE_LOGW("free data base ok!\n");

    /* 释放算法申请的内存表 */
#if 0
    s32Ret = Ia_ResetXsiMem(IA_MEM_MODE_FACE, IA_HISI_MMZ_CACHE);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("Reset Xsi Mem Failed! s32Ret 0x%x\n", s32Ret);
        return SAL_FAIL;
    }

#endif

    /* 算法资源初始化标志位置为False */
    g_stFaceCommonPrm.bInit = SAL_FALSE;

    FACE_LOGI("Face Module: DeInit End!\n");

    return SAL_SOK;
}

