/*******************************************************************************
* xpack_ring_buf.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : 
* Version: 
*
* Description :
* Modification:
*******************************************************************************/

/*****************************************************************************
                                头文件
*****************************************************************************/
#include "xpack_ring_buf.h"

#line __LINE__ "xpack_ring_buf.c"

/*****************************************************************************
                                宏定义
*****************************************************************************/
#define CHECK_PTR_IS_NULL(ptr, errno) \
    do { \
        if (!ptr) \
        { \
            SAL_LOGE("the '%s' is NULL\n", # ptr); \
            return (errno); \
        } \
    } while (0)

#define SINGLE_PUT_WIDTH    (24)    // 分段向缓冲区添加数据时每段的宽度

/*****************************************************************************
                                结构体
*****************************************************************************/
/* 
 * ring buffer
 */
typedef struct ringScanBuff
{
    INT32 s32BufWidth;              /* 总体内存宽度，单位：pixel*/
    INT32 s32RingWidth;             /* 循环内存宽度，单位：pixel*/
    INT32 s32ScreenWidth;           /* 单屏的宽度，单位：pixel */
    INT32 s32ScreenHeight;          /* 单屏的高度，单位：pixel */
    INT32 s32MaxCropWidth;          /* 支持的最大抠图宽度，单位：pixel */
    INT32 wIdx;                     /* 数据缓存写索引，相对于循环起点的偏移量，循环起点根据循环方向变化，单位：pixel */
    INT32 rIdx;                     /* 数据缓存读索引，相对于循环起点的偏移量，循环起点根据循环方向变化，单位：pixel */
    INT32 s32IdxRange;              /* 索引值最大范围，单位：pixel */
    INT32 s32LastScreenWidxStart;   /* 最后一屏起始点的写索引位置，单位：pixel */
    INT32 s32LastScreenWidxEnd;     /* 最后一屏结束点的写索引位置，单位：pixel */
    UINT32 u32BgValue;              /* 填充背景色 */
    INT32 s32ReservedWIdx;          /* 切换到TwoWay模式时记录的OneWay模式的写索引，切回到OneWay模式时将wIdx恢复为此值 */
    INT32 s32ReservedRIdx;          /* 切换到TwoWay模式时记录的OneWay模式的读索引，切回到OneWay模式时将rIdx恢复为此值 */
    UINT64 u64ReservedWDataLoc;     /* 切换到TwoWay模式时记录的OneWay模式的写位置，切回到OneWay模式时将u64WriteDataLoc恢复为此值 */
    UINT64 u64ReservedRDataLoc;     /* 切换到TwoWay模式时记录的OneWay模式的读位置，切回到OneWay模式时将u64ReadDataLoc恢复为此值 */
    UINT64 u64WriteDataLoc;         /* 显示数据写的位置，实际是写入列数的计数值，从0开始以显示条带宽度累加 */
    UINT64 u64ReadDataLoc;          /* 显示数据读的位置，实际是读取列数的计数值，从0开始以刷新列数累加 */
    RING_BUF_DIR enLastWriteDir;    /* 上次向循环缓冲区添加数据的方向，影响从循环缓冲读数据的方向 */
    RING_BUF_DIR enRingBufDir;      /* 循环缓冲区的循环方向，和enLastWriteDir联合使用控制读写数据位置变化，两者相同时读写数据位置增大，否则读写数据位置减小 */
    RING_BUF_MODE enRingBufMode;    /* 循环缓冲区的循环方式，仅单向循环显示或者双向循环显示 */
    void *mChnMutexHdl;             /* 通道信号量*/
    INT32 s32DplctdBufNum;          /* 循环缓冲重复内存的数量 */
    XIMAGE_DATA_ST stRingDispImg[DUPLI_PLANE_MAX];  /*显示数据内存管理*/
} RING_SCAN_BUFF;


/*
 * 分段存储缓冲结构体，在高度方向循环
 * 仅使用单屏缓冲存放当前屏幕的raw类型数据，将全屏数据在内存分为前部和后部两段，将两段拼接起来为一整屏数据
 * 前部是屏幕上先出现的部分数据，后部是屏幕上后出现的部分数据，根据出图方向的不同其指代的屏幕上实际的包裹区域相反
 */
typedef struct segmentBufAttr
{
    UINT32 buffLen;                         // 缓冲区内存总长度，单位：pixel
    UINT32 u32ScreenLen;                    // 分段总和长度，单位：pixel
    BOOL bResetDir;                         // 当前方向是否重置标记
    BOOL bGoToInit;                         // 增长到头，TRUE时表示前部后部指针已完成切换
    RING_BUF_DIR enLastInDir;               // 上一次向缓冲区添加数据的方向
    RING_BUF_DIR u32CurDir;                 // 当前缓冲区循环方向
    UINT32 u32FrontPartOffset;              // 屏幕前部数据起始偏移，单位：pixel
    UINT32 u32FrontPartLen;                 // 屏幕前部数据长度，单位：pixel
    UINT32 u32LatterPartOffset;             // 屏幕后部数据起始偏移，单位：pixel
    UINT32 u32LatterPartLen;                // 屏幕后部数据长度，单位：pixel
    XIMAGE_DATA_ST stSegDispImg;            // 图像数据存储
} SEGMENT_BUF_ATTR;

/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */
VOID xpack_media_buf_free(MEDIA_BUF_ATTR *pstMbAttr);

/**
 * @function   ring_buf_write
 * @brief      向循环缓冲中写数据，集中处理写数据回头，第一屏和最后一屏数据保持统一等特殊情况
 * @param[in]  ring_buf_handle      ringBufHandle    循环缓冲句柄
 * @param[in]  UINT64               u64WriteLoc      在指定的写位置更新数据，当和enRefreshLoc同时指定时，以u64WriteLoc为准, -1时无效
 * @param[in]  RING_BUF_REFRESH_LOC enRefreshLoc     图像刷新时刷新位置，在屏幕居左/中/右更新，仅更新数据不足一屏时使用，与u64WriteLoc不可同时使用
 * @param[in]  UINT32               u32TopOffset     写入数据在内存高度方向的偏移
 * @param[in]  XIMAGE_DATA_ST       *pstImageDataSrc 图像数据源
 * @param[out] None
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
static SAL_STATUS ring_buf_write(ring_buf_handle ringBufHandle, UINT64 u64WriteLoc, RING_BUF_REFRESH_LOC enRefreshLoc, UINT32 u32TopOffset, RING_BUF_DIR enWriteDir, XIMAGE_DATA_ST *pstImageDataSrc)
{
    INT32 i = 0;
    /* 源数据属性参数 */
    INT32 s32SrcDispWidth = 0;   // 使用INT32类型，需要记录符号
    UINT32 u32SrcDispHeight = 0;
    /* 循环缓冲区属性参数 */
    INT32 s32RingBufWidth = 0;
    INT32 s32ScreenWidth = 0;
    INT32 s32IdxRange = 0;
    INT32 s32LastScreenWidxStart = 0;       // 最后一屏起始点的写索引位置，单位：pixel
    INT32 s32LastScreenWidxEnd = 0;         // 最后一屏结束点的写索引位置，单位：pixel
    /* 写控制参数 */
    INT32 s32CurWidx = 0;                   // 表示当前数据写入位置的写索引值
    INT32 s32NextWidx = 0;                  // 表示当前写操作完成后的写索引值
    UINT32 u32RefreshOffset = 0;            // 更新屏幕时写数据偏移量
    INT32 s32SequentWriteOffset = 0;        // 预期的当前写的起始点相对于内存开始偏移，实际要考虑内存回头情况
    INT32 s32SequentWriteWidth = 0;         // 按内存顺序写的数据宽度
    INT32 s32FirstScreenWOffset = 0;        // 按顺序写的数据中，需要同步在第一屏写的起始点相对于内存开始偏移
    INT32 s32FirstScreenWOffset2 = 0;       // 写数据有回头时，回头部分同步在第一屏写的起始点相对于内存开始偏移
    INT32 s32FinalScreenWOffset2 = 0;       // 写数据有回头时，回头部分同步在最后一屏写的起始点相对于内存开始偏移
    INT32 s32FinalScreenWriteLeft = 0;      // 写在最后一屏的源数据的左边偏移
    INT32 s32FinalScreenWriteWidth = 0;
    INT32 s32FinalScreenWriteWidth2 = 0;    // 和s32FirstScreenWOffset2搭配，表示回头写的部分需要同步至第一屏的数据宽度
    UINT32 u32WriteBackOffset = 0;          // 超出内存范围，回头写的开始位置
    INT32 s32WriteBackWidth = 0;            // 超出内存范围，回头写的数据宽度
    UINT32 u32SeqWritePartLeft = 0;         // 回头时源数据按顺序写的部分左偏移
    UINT32 u32BackWritePartLeft = 0;        // 回头时源数据回头写的部分的左偏移
    /* 图像拷贝参数 */
    SAL_VideoCrop stCropSrcPrm = {0};
    SAL_VideoCrop stCropDstPrm = {0};
    XIMAGE_DATA_ST *pstImgData = NULL;

    CHECK_PTR_IS_NULL(ringBufHandle, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstImageDataSrc, SAL_FAIL);

    s32ScreenWidth = ringBufHandle->s32ScreenWidth;
    s32RingBufWidth = ringBufHandle->s32RingWidth;
    s32IdxRange = ringBufHandle->s32IdxRange;

    s32SrcDispWidth = pstImageDataSrc->u32Width;
    u32SrcDispHeight = pstImageDataSrc->u32Height;
    if (s32SrcDispWidth > s32IdxRange)
    {
        SAL_LOGE("ringHandle:%p, ring buf writing data width[%d] should not exceeds ring buf idx range[%d]\n", ringBufHandle, s32SrcDispWidth, s32IdxRange);
        return SAL_FAIL;
    }

    s32LastScreenWidxStart = ringBufHandle->s32LastScreenWidxStart;
    s32LastScreenWidxEnd = ringBufHandle->s32LastScreenWidxEnd;
    /* 计算当前写开始位置的写索引s32CurWidx和写结束位置的写索引s32NextWidx */
    s32CurWidx = ringBufHandle->wIdx;
    if (RING_REFRESH_NONE != enRefreshLoc)
    {
        /* 全屏刷新数据不足一屏时确定刷新数据在屏幕上的位置 */
        if (s32SrcDispWidth < s32ScreenWidth)
        {
            u32RefreshOffset = 0;
            if (RING_REFRESH_LEFT == enRefreshLoc)
            {
                u32RefreshOffset = s32ScreenWidth - s32SrcDispWidth;
            }
            else if (RING_REFRESH_MIDDLE == enRefreshLoc)
            {
                u32RefreshOffset = (s32ScreenWidth - s32SrcDispWidth) / 2;
            }
        }
        /* 全屏更新时，将数据更新到当前所写位置前部的区域 */
        if (ringBufHandle->wIdx >= (s32SrcDispWidth + u32RefreshOffset))
        {
            s32CurWidx = ringBufHandle->wIdx - (s32SrcDispWidth + u32RefreshOffset);
        }
        else
        {
            s32CurWidx = ringBufHandle->wIdx + s32IdxRange - (s32SrcDispWidth + u32RefreshOffset);
        }
    }
    /* u64WriteLoc有效时，从u64WriteLoc指定位置写入数据 */
    if (-1 != u64WriteLoc)
    {
        if (s32IdxRange < SAL_SUB_ABS(u64WriteLoc, ringBufHandle->u64WriteDataLoc))
        {
            SAL_LOGE("The gap between the specified write loc[%llu] and write loc[%llu] is too wide[%llu]( > ring buf idx range[%u])\n", 
                     u64WriteLoc, ringBufHandle->u64WriteDataLoc, SAL_SUB_ABS(u64WriteLoc, ringBufHandle->u64WriteDataLoc),  s32IdxRange);
            return SAL_FAIL;
        }
        s32CurWidx = u64WriteLoc % s32IdxRange;
    }
    s32NextWidx = s32CurWidx + s32SrcDispWidth;

    /* 非更新时写数据范围在第一屏之内，或超出内存最大范围，不允许此情况发生 */
    if ((s32CurWidx >= s32LastScreenWidxEnd || s32CurWidx < 0) && RING_REFRESH_NONE == enRefreshLoc)
    {
        SAL_LOGE("Invalid memory curWidx:%u, screen wIdx[0 ~ %u], dir:%d srcDataWidth:%u, stride:%u\n", 
                 s32CurWidx, s32LastScreenWidxEnd, enWriteDir, s32SrcDispWidth, s32RingBufWidth);
        return SAL_FAIL;
    }

    s32SequentWriteWidth = s32SrcDispWidth;
    /* 1.根据写索引判断各种异常情况，确定各种情况下各个部分写的宽度 */
    /* 刚写到最后一屏，只有部分写在最后一屏范围内 */
    if ((s32CurWidx < s32LastScreenWidxStart) && 
        (s32NextWidx > s32LastScreenWidxStart && s32NextWidx <= s32LastScreenWidxEnd))
    {
        s32FinalScreenWriteWidth = s32NextWidx - s32LastScreenWidxStart;
        /* 确定最后一屏写的部分是源数据左边还是右边部分 */
        if (RING_DIR_UP == enWriteDir)
        {
            s32FinalScreenWriteLeft = s32SrcDispWidth - s32FinalScreenWriteWidth;
        }
        else
        {
            s32FinalScreenWriteLeft = 0;
        }
        s32WriteBackWidth = 0;
    }
    /* 全部数据都写在最后一屏范围内 */
    else if ((s32CurWidx >= s32LastScreenWidxStart && s32CurWidx < s32LastScreenWidxEnd) && 
             (s32NextWidx > s32LastScreenWidxStart && s32NextWidx <= s32LastScreenWidxEnd))
    {
        s32FinalScreenWriteWidth = s32SequentWriteWidth;
        s32FinalScreenWriteLeft = 0;
        s32WriteBackWidth = 0;
    }
    /* 写数据范围会超出内存最大长度，超出部分回头写 */
    else if (s32CurWidx < s32LastScreenWidxEnd && s32NextWidx > s32LastScreenWidxEnd)
    {
        s32SequentWriteWidth = s32LastScreenWidxEnd - s32CurWidx;
        if (s32CurWidx < s32LastScreenWidxStart)
        {
            s32FinalScreenWriteWidth = s32ScreenWidth;
            /* 确定最后一屏写的部分是源数据左边还是右边部分 */
            if (RING_DIR_UP == enWriteDir)
            {
                s32FinalScreenWriteLeft = s32LastScreenWidxStart - s32CurWidx;
            }
            else
            {
                s32FinalScreenWriteLeft = s32NextWidx - s32LastScreenWidxEnd;
            }
        }
        else
        {
            s32FinalScreenWriteWidth = s32SequentWriteWidth;
            /* 确定最后一屏写的部分是源数据左边还是右边部分 */
            if (RING_DIR_UP == enWriteDir)
            {
                s32FinalScreenWriteLeft = 0;
            }
            else
            {
                s32FinalScreenWriteLeft = s32NextWidx - s32LastScreenWidxEnd;
            }
        }

        s32WriteBackWidth = s32NextWidx - s32LastScreenWidxEnd;
        /* 确定回头写的部分是源数据左边还是右边部分 */
        if (RING_DIR_UP == enWriteDir)
        {
            /* 回头部分是右边部分 */
            u32SeqWritePartLeft = 0;
            u32BackWritePartLeft = s32SequentWriteWidth;
        }
        else
        {
            /* 回头部分是左边部分 */
            u32SeqWritePartLeft = s32SrcDispWidth - s32SequentWriteWidth;
            u32BackWritePartLeft = 0;
        }

        /* 若回头写数据又写到了最后一屏范围里 */
        if (s32WriteBackWidth > ringBufHandle->s32IdxRange - s32ScreenWidth)
        {
            s32FinalScreenWriteWidth2 = s32WriteBackWidth - (ringBufHandle->s32IdxRange - s32ScreenWidth);
        }
    }

    /* 2.将索引转换为实际内存写偏移量，并确定各部分写的偏移量 */
    if (RING_DIR_UP == enWriteDir)
    {
        s32SequentWriteOffset = s32CurWidx + s32ScreenWidth;
        s32FirstScreenWOffset = SAL_SUB_SAFE(s32SequentWriteOffset, s32IdxRange);
        u32WriteBackOffset = s32ScreenWidth;
        s32FirstScreenWOffset2 = 0;
        s32FinalScreenWOffset2 = ringBufHandle->s32IdxRange;
    }
    else
    {
        s32SequentWriteOffset = s32IdxRange - s32CurWidx - s32SequentWriteWidth;
        s32FirstScreenWOffset = s32SequentWriteOffset + s32IdxRange;
        u32WriteBackOffset = s32IdxRange - s32WriteBackWidth;
        s32FirstScreenWOffset2 = ringBufHandle->s32IdxRange + s32ScreenWidth - s32FinalScreenWriteWidth2;
        s32FinalScreenWOffset2 = ringBufHandle->s32IdxRange - s32LastScreenWidxStart - s32FinalScreenWriteWidth2;
    }

    SAL_LOGD("ringHandle:%p, rIdx:%u, widx:%u, dir:%d, writeLoc:%llu[refOffset:%u], seq[%u, %u], 1st[%u, %u], back[%u, %u, last[%u, %u]], top:%u\n", 
               ringBufHandle, ringBufHandle->rIdx, ringBufHandle->wIdx, enWriteDir, (-1 != u64WriteLoc) ? u64WriteLoc : 0, u32RefreshOffset, 
               s32SequentWriteOffset, s32SequentWriteWidth, s32FirstScreenWOffset, s32FinalScreenWriteWidth, 
               u32WriteBackOffset, s32WriteBackWidth, s32FinalScreenWOffset2, s32FinalScreenWriteWidth2, u32TopOffset);
    /* 3.开始写数据 */
    for (i = 0; i < ringBufHandle->s32DplctdBufNum; i++)
    {
        pstImgData = &ringBufHandle->stRingDispImg[i];

        stCropSrcPrm.top = 0;
        stCropDstPrm.top = u32TopOffset;
        stCropDstPrm.height = stCropSrcPrm.height = u32SrcDispHeight;

        /* 按顺序写内存 */
        if (0 < s32SequentWriteWidth)
        {
            stCropDstPrm.width = stCropSrcPrm.width = s32SequentWriteWidth;
            stCropSrcPrm.left = u32SeqWritePartLeft;
            stCropDstPrm.left = s32SequentWriteOffset;
            if (SAL_SOK != ximg_crop(pstImageDataSrc, pstImgData, &stCropSrcPrm, &stCropDstPrm))
            {
                goto EXIT;
            }
        }

        /* 当写入到了内存最后一屏范围时，将同样的数据写入到内存第一屏范围 */
        if (0 < s32FinalScreenWriteWidth)
        {
            stCropDstPrm.width = stCropSrcPrm.width = s32FinalScreenWriteWidth;
            stCropSrcPrm.left = s32FinalScreenWriteLeft;
            stCropDstPrm.left = s32FirstScreenWOffset;
            if (SAL_SOK != ximg_crop(pstImageDataSrc, pstImgData, &stCropSrcPrm, &stCropDstPrm))
            {
                goto EXIT;
            }
        }

        /* 如果写入所有数据会超出内存范围时，超出内存部分回头写 */
        if (0 < s32WriteBackWidth)
        {
            stCropDstPrm.width = stCropSrcPrm.width = s32WriteBackWidth;
            stCropSrcPrm.left = u32BackWritePartLeft;
            stCropDstPrm.left = u32WriteBackOffset;
            if (SAL_SOK != ximg_crop(pstImageDataSrc, pstImgData, &stCropSrcPrm, &stCropDstPrm))
            {
                goto EXIT;
            }

            /* 回头写的数据写到了最后一屏的范围时 */
            if (0 < s32FinalScreenWriteWidth2)
            {
                stCropDstPrm.top = stCropSrcPrm.top = 0;
                stCropDstPrm.height = stCropSrcPrm.height = pstImageDataSrc->u32Height;
                stCropDstPrm.width = stCropSrcPrm.width = s32FinalScreenWriteWidth2;
                stCropSrcPrm.left = s32FinalScreenWOffset2;
                stCropDstPrm.left = s32FirstScreenWOffset2;
                if (SAL_SOK != ximg_crop(pstImgData, pstImgData, &stCropSrcPrm, &stCropDstPrm))
                {
                    goto EXIT;
                }
            }
        }
    }

    return SAL_SOK;

EXIT:
    SAL_LOGE("ximg_crop failed, rIdx:%u, widx:%u, dir:%d, seq[%u, %u], fir[%u, %u], back[%u, %u]\n", 
             ringBufHandle->rIdx, ringBufHandle->wIdx, enWriteDir, 
             s32SequentWriteOffset, s32SequentWriteWidth, s32FirstScreenWOffset, s32FinalScreenWriteWidth, 
             u32WriteBackOffset, s32WriteBackWidth);
    return SAL_FAIL;
}

/**
 * @function   ring_buf_init
 * @brief      创建一个循环缓冲
 * @param[in]  UINT32         s32BufWidth     总体内存宽度，实际用于循环缓冲的宽度为s32BufWidth-(u32MaxCropWidth-u32ScreenW)，循环缓冲至少需要为显示帧宽度的2倍，单位：pixel
 * @param[in]  UINT32         u32BufHeight    内存高度，单位：pixel
 * @param[in]  UINT32         u32ScreenW      显示帧宽度，单位：pixel
 * @param[in]  UINT32         u32MaxCropWidth 支持的最大抠图宽度，u32MaxCropWidth-u32ScreenW为内存尾部保留区域，单位：pixel
 * @param[in]  DSP_IMG_DATFMT enDataFmt       数据类型
 * @param[in]  UINT32         u32DplctdBufNum 实际重复内存的个数，需不大于XAPCK_DATA_CNT_MAX
 * @param[out] NONE
 * @return     ring_buf_handle 循环缓冲的句柄
 */
ring_buf_handle ring_buf_init(UINT32 s32BufWidth, UINT32 u32BufHeight, UINT32 u32ScreenW, UINT32 s32ScreenH, UINT32 u32MaxCropWidth, DSP_IMG_DATFMT enDataFmt, UINT32 u32DplctdBufNum)
{
    INT32 i = 0;
    ring_buf_handle ringBuf = NULL;
    XIMAGE_DATA_ST *pstDispImg = NULL;
    MEDIA_BUF_ATTR *pstMediaBuf = NULL;

    if (s32BufWidth - SAL_SUB_SAFE(u32MaxCropWidth, u32ScreenW) < 2 * u32ScreenW)
    {
        SAL_LOGE("Ring buf width[%u] require at least 2 times of screen width[2 * %u]\n", 
                 s32BufWidth, u32ScreenW);
        return NULL;
    }
    if (u32MaxCropWidth < u32ScreenW || u32MaxCropWidth > s32BufWidth - u32ScreenW)
    {
        SAL_LOGE("u32MaxCropWidth[%u] requires at least u32ScreenW[%u] but less than s32BufWidth[%u] - u32ScreenW[%u]\n", 
                 u32MaxCropWidth, u32ScreenW, s32BufWidth, u32ScreenW);
        return NULL;
    }
    if (DUPLI_PLANE_MAX < u32DplctdBufNum)
    {
        SAL_LOGE("u32DplctdBufNum[%u] is too large, should not bigger than ring buf DUPLI_PLANE_MAX[%u]\n", u32DplctdBufNum, DUPLI_PLANE_MAX);
        return NULL;
    }

    ringBuf = (ring_buf_handle)malloc(sizeof(RING_SCAN_BUFF));
    memset(ringBuf, 0x0, sizeof(RING_SCAN_BUFF));

    for (i = 0; i < u32DplctdBufNum; i++)
    {
        pstDispImg = &ringBuf->stRingDispImg[i];
        pstMediaBuf = &pstDispImg->stMbAttr;

        SAL_clear(pstMediaBuf);
        if (SAL_SOK != ximg_create(s32BufWidth, u32BufHeight, enDataFmt, "ring", NULL, pstDispImg))
        {
            SAL_LOGE("ximg_create failed, w:%u, h:%u, size:%u fmt:0x%x\n", 
                       pstDispImg->u32Width, pstDispImg->u32Height, pstMediaBuf->memSize, pstDispImg->enImgFmt);
            free(ringBuf);
            return NULL;
        }
    }

    /* 参数初始化 */
    ringBuf->s32BufWidth = s32BufWidth;
    ringBuf->s32RingWidth = s32BufWidth - (u32MaxCropWidth - u32ScreenW);
    ringBuf->s32ScreenWidth = u32ScreenW;
    ringBuf->s32ScreenHeight = s32ScreenH;
    ringBuf->s32MaxCropWidth = u32MaxCropWidth;
    ringBuf->s32DplctdBufNum = u32DplctdBufNum;
    ringBuf->s32IdxRange = ringBuf->s32RingWidth - ringBuf->s32ScreenWidth;
    ringBuf->s32LastScreenWidxStart = ringBuf->s32IdxRange - ringBuf->s32ScreenWidth;
    ringBuf->s32LastScreenWidxEnd = ringBuf->s32IdxRange;
    ringBuf->u32BgValue =  (DSP_IMG_DATFMT_YUV420_MASK & enDataFmt) ? XIMG_BG_DEFAULT_YUV : XIMG_BG_DEFAULT_RGB;
    ringBuf->u64ReservedWDataLoc = 0;
    ringBuf->u64ReservedRDataLoc = 0;
    ringBuf->wIdx = ringBuf->s32ScreenWidth;
    ringBuf->rIdx = ringBuf->s32ScreenWidth;
    ringBuf->u64WriteDataLoc = ringBuf->s32ScreenWidth;
    ringBuf->u64ReadDataLoc = ringBuf->s32ScreenWidth;
    ringBuf->enLastWriteDir = RING_DIR_UP;
    ringBuf->enRingBufDir = RING_DIR_UP;
    ringBuf->enRingBufMode = RING_MODE_ONEWAY;

    SAL_mutexCreate(SAL_MUTEX_ERRORCHECK, &ringBuf->mChnMutexHdl);

    return ringBuf;
}

/**
 * @function   ring_buf_deinit
 * @brief      销毁循环缓冲，释放资源
 * @param[in]  ring_buf_handle ringBufHandle 循环缓冲句柄
 * @param[out] NONE
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS ring_buf_deinit(ring_buf_handle ringBufHandle)
{
    INT32 i = 0;

    CHECK_PTR_IS_NULL(ringBufHandle, SAL_FAIL);

    for (i = 0; i < ringBufHandle->s32DplctdBufNum; i++)
    {
        ximg_destroy(&ringBufHandle->stRingDispImg[i]);
    }
    
    free(ringBufHandle);
    ringBufHandle = NULL;
    return SAL_SOK;
}

/**
 * @function   ring_buf_clear
 * @brief      初始化缓冲颜色为指定颜色
 * @param[in]  ring_buf_handle ringBufHandle 循环缓冲句柄 
 * @param[in]  UINT32          u32BgValue    填充颜色值
 * @param[out] NONE
 * @return     void
 */
void *ring_buf_clear(ring_buf_handle ringBufHandle, UINT32 u32BgValue)
{
    UINT32 i = 0;
    CHECK_PTR_IS_NULL(ringBufHandle, NULL);

    ringBufHandle->u32BgValue = u32BgValue;

    for (i = 0; i < ringBufHandle->s32DplctdBufNum; i++)
    {
        if (NULL == ringBufHandle->stRingDispImg[i].pPlaneVir[0])
        {
            break;
        }
        ximg_fill_color(&ringBufHandle->stRingDispImg[i], 0, ringBufHandle->stRingDispImg[i].u32Height, 0, ringBufHandle->stRingDispImg[i].u32Width, u32BgValue);
    }

    return NULL;
}

/**
 * @function   ring_buf_reset
 * @brief      循环缓冲方向参数复位，初始化缓冲颜色为指定颜色
 * @param[in]  ring_buf_handle ringBufHandle 循环缓冲句柄 
 * @param[in]  RING_BUF_DIR    enRingBufDir  循环方向，为RING_DIR_NONE时不改变方向
 * @param[in]  UINT32          u32BgValue    填充颜色值
 * @param[out] NONE
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS ring_buf_reset(ring_buf_handle ringBufHandle, RING_BUF_DIR enRingBufDir, RING_BUF_MODE enBufMode, UINT32 u32BgValue)
{
    CHECK_PTR_IS_NULL(ringBufHandle, SAL_FAIL);

    ring_buf_clear(ringBufHandle, u32BgValue);

    if (RING_DIR_NONE != enRingBufDir)
    {
        ringBufHandle->enRingBufDir = enRingBufDir;
        ringBufHandle->enLastWriteDir = enRingBufDir;
    }

    /* 由RING_MODE_ONEWAY切回到RING_MODE_TWOWAY模式 */
    if (RING_MODE_ONEWAY == ringBufHandle->enRingBufMode && RING_MODE_TWOWAY == enBufMode)
    {
        /* 记录ONEWAY模式下的索引位置 */
        ringBufHandle->s32ReservedWIdx = ringBufHandle->wIdx;
        ringBufHandle->s32ReservedRIdx = ringBufHandle->rIdx;
        ringBufHandle->u64ReservedWDataLoc = ringBufHandle->u64WriteDataLoc;
        ringBufHandle->u64ReservedRDataLoc = ringBufHandle->u64ReadDataLoc;
        /* 条带回拉保留第一屏图像 */
        ringBufHandle->wIdx = ringBufHandle->s32ScreenWidth;
        ringBufHandle->rIdx = ringBufHandle->s32ScreenWidth;
        ringBufHandle->u64WriteDataLoc = ringBufHandle->s32ScreenWidth;
        ringBufHandle->u64ReadDataLoc = ringBufHandle->s32ScreenWidth;
    }
    /* 由RING_MODE_TWOWAY切回到RING_MODE_ONEWAY模式 */
    else if (RING_MODE_TWOWAY == ringBufHandle->enRingBufMode && RING_MODE_ONEWAY == enBufMode)
    {
        ringBufHandle->wIdx = ringBufHandle->s32ReservedWIdx;
        ringBufHandle->rIdx = ringBufHandle->s32ReservedRIdx;
        ringBufHandle->u64WriteDataLoc = ringBufHandle->u64ReservedWDataLoc;
        ringBufHandle->u64ReadDataLoc = ringBufHandle->u64ReservedRDataLoc;
    }
    /* 默认一屏空白数据方便整包回拉等操作 */
    else
    {
        ringBufHandle->wIdx = ringBufHandle->s32ScreenWidth;
        ringBufHandle->rIdx = ringBufHandle->s32ScreenWidth;
        ringBufHandle->u64WriteDataLoc = ringBufHandle->s32ScreenWidth;
        ringBufHandle->u64ReadDataLoc = ringBufHandle->s32ScreenWidth;
    }
    ringBufHandle->enRingBufMode = enBufMode;

    return SAL_SOK;
}

/**
 * @function   ring_buf_splitput
 * @brief      将完整源图像的子图添加到循环缓冲中
 * @param[IN]  ring_buf_handle ringBufHandle    循环缓冲句柄
 * @param[IN]  XIMAGE_DATA_ST  *pstImageDataSrc 数据源
 * @param[IN]  SAL_VideoCrop   *pstCropPrm      子图像信息
 * @param[IN]  RING_BUF_DIR    enRingBufDir     数据添加方向
 * @param[OUT] NONE
 * @return     UINT64 将循环缓冲区展开成平面后的数据距离起点偏移量（单位：像素），即从该Loc为起始写入这段数据，-1时添加数据失败
 */
static UINT64 ring_buf_splitput(ring_buf_handle ringBufHandle, XIMAGE_DATA_ST *pstImageDataSrc, SAL_VideoCrop *pstCropPrm, RING_BUF_DIR enRingBufDir)
{
    INT32 s32IdxRange = 0;
    UINT32 u32SplitPutWidth = 0;
    UINT64 u64RetWriteLoc = 0, u64NextWriteLoc = 0;
    XIMAGE_DATA_ST stSubPutImg = {0};
    
    CHECK_PTR_IS_NULL(ringBufHandle, -1);
    CHECK_PTR_IS_NULL(pstImageDataSrc, -1);
    CHECK_PTR_IS_NULL(pstCropPrm, -1);

    s32IdxRange = ringBufHandle->s32IdxRange;
    u32SplitPutWidth = pstCropPrm->width;

    /* 判断写位置，超出范围则出错返回 */
    if (ringBufHandle->enLastWriteDir == enRingBufDir)
    {
        if (enRingBufDir == ringBufHandle->enRingBufDir)
        {
            u64RetWriteLoc = ringBufHandle->u64WriteDataLoc;
            u64NextWriteLoc = ringBufHandle->u64WriteDataLoc + u32SplitPutWidth;
        }
        else
        {
            if (ringBufHandle->u64WriteDataLoc < u32SplitPutWidth)
            {
                SAL_LOGE("handle:%p, can't put data[%u] under ring buf mode[%d] put data dir[%d] anymore, main dir:%d, last put dir:%d\n", 
                        ringBufHandle, u32SplitPutWidth, ringBufHandle->enRingBufMode, enRingBufDir, ringBufHandle->enRingBufDir, ringBufHandle->enLastWriteDir);
                return -1;
            }
            u64NextWriteLoc = ringBufHandle->u64WriteDataLoc - u32SplitPutWidth;
            u64RetWriteLoc = u64NextWriteLoc;
        }
    }
    else
    {
        /* 切换回拉方向，写位置发生改变，将写索引转换另一方向的写索引 */
        if (RING_MODE_TWOWAY == ringBufHandle->enRingBufMode)
        {
            ringBufHandle->wIdx = s32IdxRange - ringBufHandle->wIdx;
            if (ringBufHandle->wIdx >= s32IdxRange)
            {
                ringBufHandle->wIdx -= s32IdxRange;
            }
            ringBufHandle->rIdx = ringBufHandle->wIdx; /* 直接将切换方向前未显示数据一次刷新完，方向随wIdx反向 */
            ringBufHandle->u64ReadDataLoc = ringBufHandle->u64WriteDataLoc;
        }
        ringBufHandle->enLastWriteDir = enRingBufDir; /* 紧跟在rIdx方向改变后面，保证ring_buf_get获取显示帧时enLastWriteDir和rIdx匹配 */

        if (enRingBufDir == ringBufHandle->enRingBufDir)
        {
            u64NextWriteLoc = ringBufHandle->u64WriteDataLoc + ringBufHandle->s32ScreenWidth;
            u64RetWriteLoc = u64NextWriteLoc;
            u64NextWriteLoc = u64NextWriteLoc + u32SplitPutWidth;
        }
        else
        {
            if (ringBufHandle->u64WriteDataLoc < (ringBufHandle->s32ScreenWidth + u32SplitPutWidth))
            {
                SAL_LOGE("handle:%p, can't put data[w:%u] under ring buf mode[%d] put data dir[%d] anymore, main dir:%d, last put dir:%d\n", 
                        ringBufHandle, u32SplitPutWidth, ringBufHandle->enRingBufMode, enRingBufDir, ringBufHandle->enRingBufDir, ringBufHandle->enLastWriteDir);
                return -1;
            }
            u64NextWriteLoc = ringBufHandle->u64WriteDataLoc - ringBufHandle->s32ScreenWidth;
            u64NextWriteLoc = u64NextWriteLoc - u32SplitPutWidth;
            u64RetWriteLoc = u64NextWriteLoc;
        }
    }

    if (SAL_SOK != ximg_create_sub(pstImageDataSrc, &stSubPutImg, pstCropPrm))
    {
        SAL_LOGE("ximg_create_sub failed, ringHandle:%p\n", ringBufHandle);
        return -1;
    }
    if (SAL_SOK != ring_buf_write(ringBufHandle, -1, RING_REFRESH_NONE, 0, enRingBufDir, &stSubPutImg))
    {
        SAL_LOGE("ring_buf_write failed, ringHandle:%p\n", ringBufHandle);
        return -1;
    }

    ringBufHandle->wIdx += stSubPutImg.u32Width;
    if (ringBufHandle->wIdx >= s32IdxRange)
    {
        ringBufHandle->wIdx -= s32IdxRange;
    }
    ringBufHandle->u64WriteDataLoc = u64NextWriteLoc;

    return u64RetWriteLoc;
}

/**
 * @function   ring_buf_put
 * @brief      向循环缓冲中添加数据
 * @param[IN]  ring_buf_handle ringBufHandle    循环缓冲句柄
 * @param[IN]  XIMAGE_DATA_ST  *pstImageDataSrc 数据源
 * @param[IN]  RING_BUF_DIR    enRingBufDir     循环方向
 * @param[OUT] NONE
 * @return     UINT64 将循环缓冲区展开成平面后的数据距离起点偏移量（单位：像素），即从该Loc为起始写入这段数据，-1时添加数据失败
 */
UINT64 ring_buf_put(ring_buf_handle ringBufHandle, XIMAGE_DATA_ST *pstImageDataSrc, RING_BUF_DIR enRingBufDir)
{
    INT32 s32IdxRange = 0;
    INT32 s32TryCnt = 0, s32FreeSize = 0;
    INT32 s32SinglePutWidth = 0, s32HasPutWidth = 0, s32PutRemainWidth = 0;
    UINT64 u64RetWriteLoc = 0, u64FirstRetLoc = 0;
    SAL_VideoCrop stCropPrm = {0};
    CHECK_PTR_IS_NULL(ringBufHandle, -1);
    CHECK_PTR_IS_NULL(pstImageDataSrc, -1);
    if (RING_DIR_UP != enRingBufDir && RING_DIR_DOWN != enRingBufDir)
    {
        SAL_LOGE("Invalid ring buf direction[%d]\n", enRingBufDir);
        return -1;
    }

    s32IdxRange = ringBufHandle->s32IdxRange;
    s32PutRemainWidth = pstImageDataSrc->u32Width;
    stCropPrm.top = 0;
    stCropPrm.height = pstImageDataSrc->u32Height;
    /* 循环缓冲剩余空间不足，等待空间足够后再向其中添加数据 */
    s32FreeSize = SAL_SUB_SAFE(s32IdxRange - ringBufHandle->s32ScreenWidth - SINGLE_PUT_WIDTH, ring_buf_size(ringBufHandle, NULL, NULL, NULL, NULL));
    if (s32FreeSize < s32PutRemainWidth)
    {
        while (0 < s32PutRemainWidth)
        {
            s32SinglePutWidth = SAL_MIN(SINGLE_PUT_WIDTH, s32PutRemainWidth);
            if (s32FreeSize >= s32SinglePutWidth)   // 缓冲区中已有足够剩余空间，直接将源数据拆分放入缓冲区
            {
                stCropPrm.width = s32SinglePutWidth;
                stCropPrm.left = (RING_DIR_UP == enRingBufDir) ? s32HasPutWidth : (pstImageDataSrc->u32Width - s32HasPutWidth - stCropPrm.width);
                if (-1 != (u64RetWriteLoc = ring_buf_splitput(ringBufHandle, pstImageDataSrc, &stCropPrm, enRingBufDir)))
                {
                    s32HasPutWidth += stCropPrm.width;
                    s32PutRemainWidth = SAL_SUB_SAFE(pstImageDataSrc->u32Width, s32HasPutWidth);
                    u64FirstRetLoc = (0 == u64FirstRetLoc) ? u64RetWriteLoc : u64FirstRetLoc;       // 记录第一次送入数据所返回的wLoc作为当前所有整体写入所返回的wLoc
                    s32TryCnt = 0;
                }
                SAL_LOGD("handle:%p, put %d ms for sufficient free ring buf size[%d], remain data width[%u]\n", 
                         ringBufHandle, s32TryCnt * 5, s32FreeSize, s32PutRemainWidth);
            }
            else
            {
                if (s32TryCnt < 100)
                {
                    s32TryCnt++;
                    SAL_msleep(5);
                }
                else
                {
                    SAL_LOGW("handle:%p, waiting for sufficient free ring buf size timeouted[%d ms], free size[%d], remain data width[%u]\n", 
                            ringBufHandle, s32TryCnt * 5, s32FreeSize, s32PutRemainWidth);
                    break;
                }
            }
            s32FreeSize = SAL_SUB_SAFE(s32IdxRange - ringBufHandle->s32ScreenWidth - SINGLE_PUT_WIDTH, ring_buf_size(ringBufHandle, NULL, NULL, NULL, NULL));
        }
    }

    /* 向循环缓冲写入剩余数据 */
    if (0 < s32PutRemainWidth)
    {
        stCropPrm.width = s32PutRemainWidth;
        stCropPrm.left = (RING_DIR_UP == enRingBufDir) ? s32HasPutWidth : (pstImageDataSrc->u32Width - s32HasPutWidth - stCropPrm.width);
        if (-1 != (u64RetWriteLoc = ring_buf_splitput(ringBufHandle, pstImageDataSrc, &stCropPrm, enRingBufDir)))
        {
            s32HasPutWidth += stCropPrm.width;
            s32PutRemainWidth = SAL_SUB_SAFE(pstImageDataSrc->u32Width, s32HasPutWidth);
            u64FirstRetLoc = (0 == u64FirstRetLoc) ? u64RetWriteLoc : u64FirstRetLoc;       // 记录第一次送入数据所返回的wLoc作为当前所有整体写入所返回的wLoc
        }
    }

    return (enRingBufDir == ringBufHandle->enRingBufDir) ? u64FirstRetLoc : u64RetWriteLoc;
}

/**
 * @function   ring_buf_update
 * @brief      更新循环缓冲中当前显示的数据，支持两种方式指定更新位置。
 *             1.通过u64WriteLoc直接指定写位置，优先级最高。
 *             2.通过enRefreshLoc指定数据更新模式，默认将数据更新到当前循环缓冲最新位置，
 *               当更新数据宽度小于屏幕宽度时，可以在屏幕居左/中/右位置更新。
 * @param[IN]  ring_buf_handle      ringBufHandle   循环缓冲句柄
 * @param[IN]  XIMAGE_DATA_ST      *pstUpdateImgSrc 更新数据源
 * @param[IN]  UINT64               u64UpdateLoc    在指定的写位置更新数据，当和enRefreshLoc同时指定时，以u64WriteLoc为准，-1时无效
 * @param[IN]  RING_BUF_REFRESH_LOC enRefreshLoc    图像刷新时刷新位置，更新数据不足一屏时指定在屏幕居左/中/右更新，与u64WriteLoc不可同时使用
 * @param[IN]  UINT32               u32TopOffset    写入数据在内存高度方向的偏移
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS ring_buf_update(ring_buf_handle ringBufHandle, XIMAGE_DATA_ST *pstUpdateImgSrc, UINT64 u64UpdateLoc, RING_BUF_REFRESH_LOC enRefreshLoc, UINT32 u32TopOffset)
{
    RING_BUF_DIR enRingDir = RING_DIR_NONE;
    CHECK_PTR_IS_NULL(ringBufHandle, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstUpdateImgSrc, SAL_FAIL);

    if (RING_MODE_TWOWAY == ringBufHandle->enRingBufMode && -1 != u64UpdateLoc)
    {
        enRingDir = ringBufHandle->enRingBufDir;
    }
    else
    {
        enRingDir = ringBufHandle->enLastWriteDir;
    }

    // static int i = 0;
    // ximg_dump(pstUpdateImgSrc, i, "/mnt/dumpdata/", "src", NULL, u64UpdateLoc);
    // ximg_dump(&ringBufHandle->stRingDispImg[0], i, "/mnt/dumpdata/", "pre", NULL, u64UpdateLoc);
    if (SAL_SOK != ring_buf_write(ringBufHandle, u64UpdateLoc, enRefreshLoc, u32TopOffset, enRingDir, pstUpdateImgSrc))
    {
        SAL_LOGE("ring_buf_write failed\n");
        return SAL_FAIL;
    }
    // ximg_dump(&ringBufHandle->stRingDispImg[0], i++, "/mnt/dumpdata/", "post", NULL, u64UpdateLoc);

    return SAL_SOK;
}

/**
 * @function   ring_buf_get
 * @brief      按顺序获取循环缓冲中的数据，每此获取后若缓存不为0则将读取位置向后偏移u32RefreshCol个像素
 * @param[IN]  ring_buf_handle ringBufHandle  循环缓冲句柄
 * @param[IN]  UINT32          u32RefreshCol  显示更新的列数，不能超过屏幕宽度，为0时屏幕不更新数据，获取当前显示数据
 * @param[OUT] XIMAGE_DATA_ST  *pstOutImage[] 获取的全屏图像，指针数组
 * @param[IN]  INT32           s32ArrayLen    参数4指针数组的成员个数，即获取的全屏图像的重复平面的个数
 * @return     UINT32 获取的帧数据在内存中的偏移量，单位：pixel
 */
UINT32 ring_buf_get(ring_buf_handle ringBufHandle, UINT32 u32RefreshCol, UINT32 u32TopOffset, XIMAGE_DATA_ST *pstOutImage[], INT32 s32ArrayLen)
{
    SAL_STATUS ret = SAL_SOK;
    INT32 i = 0;
    INT32 s32MemoryOffset = 0;         // 当前写索引对应于内存中的偏移量，单位：像素
    INT32 s32ScreenWidth = 0;
    BOOL bSuccess = SAL_FALSE;
    SAL_VideoCrop stCropPrm = {0};
    ring_buf_handle psRingBuff = ringBufHandle;
    XIMAGE_DATA_ST *pstRingBufImg = NULL;

    CHECK_PTR_IS_NULL(psRingBuff, -1);
    CHECK_PTR_IS_NULL(pstOutImage, -1);
    if (psRingBuff->s32ScreenWidth < u32RefreshCol)
    {
        SAL_LOGE("Invalid refresh col num[%u], which shoude be less than screen width[%d]\n", u32RefreshCol, psRingBuff->s32ScreenWidth);
        return -1;
    }
    if (s32ArrayLen <= 0 || s32ArrayLen > psRingBuff->s32DplctdBufNum)
    {
        SAL_LOGE("Invalid array len[%u], which shoude be less than actual duplicated buffer num[%d]\n", s32ArrayLen, psRingBuff->s32DplctdBufNum);
        return -1;
    }

    s32ScreenWidth = psRingBuff->s32ScreenWidth;

    /* 将rIdx转化为相对内存起始的偏移 */
    if (RING_DIR_UP == psRingBuff->enLastWriteDir)
    {
        s32MemoryOffset = psRingBuff->rIdx;
    }
    else
    {
        s32MemoryOffset = psRingBuff->s32RingWidth - (psRingBuff->rIdx + s32ScreenWidth);
    }

    for (i = 0; i < s32ArrayLen; i++)
    {
        pstRingBufImg = &psRingBuff->stRingDispImg[i];

        if (NULL != pstOutImage[i])
        {
            stCropPrm.top = u32TopOffset;
            stCropPrm.left = s32MemoryOffset;
            stCropPrm.width = s32ScreenWidth;
            stCropPrm.height = psRingBuff->s32ScreenHeight;
            /* 从完整循环缓冲中截取当前一个屏幕图像输出 */
            ret = ximg_create_sub(pstRingBufImg, pstOutImage[i], &stCropPrm);
            if (SAL_SOK != ret)
            {
                SAL_LOGE("ximg_create_sub failed, src[%u x %u], crop[%u %u %u %u].\n", 
                         pstRingBufImg->u32Width, pstRingBufImg->u32Height, 
                         stCropPrm.top, stCropPrm.left, stCropPrm.width, stCropPrm.height);
                return -1;
            }
            bSuccess = SAL_TRUE;
        }
    }
    if (SAL_TRUE != bSuccess)
    {
        SAL_LOGE("All pstOutImage[] are null\n");
        return -1;
    }

    if (psRingBuff->rIdx != psRingBuff->wIdx)
    {
        if (psRingBuff->enRingBufDir == psRingBuff->enLastWriteDir)
        {
            psRingBuff->u64ReadDataLoc += u32RefreshCol;
            if (psRingBuff->u64ReadDataLoc > psRingBuff->u64WriteDataLoc)
            {
                psRingBuff->u64ReadDataLoc = psRingBuff->u64WriteDataLoc;
            }
        }
        else
        {
            psRingBuff->u64ReadDataLoc = SAL_SUB_SAFE(psRingBuff->u64ReadDataLoc, u32RefreshCol);
            if (psRingBuff->u64ReadDataLoc < psRingBuff->u64WriteDataLoc + s32ScreenWidth)
            {
                psRingBuff->u64ReadDataLoc = psRingBuff->u64WriteDataLoc + s32ScreenWidth;
            }
        }
    }
    /* 更新读索引 */
    /* 读索引小于写索引，写索引未回头 */
    if (psRingBuff->rIdx < psRingBuff->wIdx)
    {
        psRingBuff->rIdx += u32RefreshCol;
        if (psRingBuff->rIdx >= psRingBuff->wIdx)
        {
            psRingBuff->rIdx = psRingBuff->wIdx;
        }

        SAL_LOGD("handle:%p, rIdx:%u, widx:%u, rLoc:%llu, wLoc:%llu, dir:%d\n", 
                 psRingBuff, psRingBuff->rIdx, psRingBuff->wIdx, psRingBuff->u64ReadDataLoc, psRingBuff->u64WriteDataLoc, psRingBuff->enLastWriteDir);
    }
    /* 读索引大于写索引，写索引已回头 */
    else if (psRingBuff->rIdx > psRingBuff->wIdx)
    {
        psRingBuff->rIdx += u32RefreshCol;
        /* 读索引超出最大范围，读索引回头 */
        if (psRingBuff->rIdx >= psRingBuff->s32IdxRange)
        {
            psRingBuff->rIdx -= psRingBuff->s32IdxRange;
            /* 读索引回头后不能超出写索引 */
            if (psRingBuff->rIdx >= psRingBuff->wIdx)
            {
                psRingBuff->rIdx = psRingBuff->wIdx;
            }
        }
        SAL_LOGD("handle:%p, rIdx:%u, widx:%u, rLoc:%llu, wLoc:%llu, dir:%d\n", 
                 psRingBuff, psRingBuff->rIdx, psRingBuff->wIdx, psRingBuff->u64ReadDataLoc, psRingBuff->u64WriteDataLoc, psRingBuff->enLastWriteDir);
    }

    return s32MemoryOffset;
}

/**
 * @function        ring_buf_crop
 * @brief           通过指定的位置索引获取显示缓冲中的数据，抠取区域位于指定抠取位置的数据增长方向
 * @param[IN]       ring_buf_handle ringBufHandle  循环缓冲句柄
 * @param[IN]       UINT64          u64ReadLoc     指定读数据位置，由ring_buf_put接口返回值合理转化，为-1时无效
 * @param[IN]       UINT32          u32ReadOffset  实际抠取位置与u64ReadLoc偏移量，即实际读取位置为u64ReadLoc+u32ReadOffset
 * @param[IN]       SAL_VideoCrop   *pstCropPrm    从目标位置抠取的图像位置尺寸参数
 * @param[IN]           left        不使用
 * @param[IN]           top         抠取范围的上边界
 * @param[IN]           width       抠取范围的宽度
 * @param[IN]           height      抠取范围的高度
 * @param[IN/OUT]   XIMAGE_DATA_ST  *pstOutImage:  获取的图像，由外部通过ximg_create(_sub)接口创建空白图像
 * @param[IN]           enImgFmt    图像格式，见枚举DSP_IMG_DATFMT
 * @param[OUT]          u32Width    图像宽，等于抠取图像宽，单位：Pixel
 * @param[OUT]          u32Height   图像高，等于抠取图像高，单位：Pixel
 * @param[OUT]          u32Stride   内存跨距，等于图像宽，单位：Pixel
 * @param[IN/OUT]       pPlaneVir   图像数据，输入为空时不进行数据拷贝，仅拷贝图像信息
 * @param[OUT]          stMbAttr    内存信息
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS ring_buf_crop(ring_buf_handle ringBufHandle, UINT64 u64ReadLoc, UINT32 u32ReadOffset, SAL_VideoCrop *pstCropPrm, XIMAGE_DATA_ST *pstOutImage)
{
    INT32 s32MemoryOffset = 0;
    UINT32 u32ReadWidth = 0;
    UINT32 u32ExtraCropOffset = 0;
    UINT32 u32ExtraCropWidth = 0;
    SAL_VideoCrop stSrcCropPrm = {0};
    SAL_VideoCrop stDstCropPrm = {0};
    XIMAGE_DATA_ST *pstRingBufImg = NULL;

    CHECK_PTR_IS_NULL(ringBufHandle, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstCropPrm, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstOutImage, SAL_FAIL);

    u32ReadWidth = pstCropPrm->width;

    if (u32ReadWidth > ringBufHandle->s32MaxCropWidth)
    {
        SAL_LOGE("Expected crop width[%u] exceeds max crop width[%d]\n", u32ReadWidth, ringBufHandle->s32MaxCropWidth);
        return SAL_FAIL;
    }
    if (RING_MODE_ONEWAY == ringBufHandle->enRingBufMode && 
        (u64ReadLoc + u32ReadOffset + pstCropPrm->width > ringBufHandle->u64WriteDataLoc))
    {
        SAL_LOGE("The ReadLoc[%llu] + ReadOffset[%u] + ReadWidth[%u] > ring buf write loc[%llu]\n", 
                 u64ReadLoc, u32ReadOffset, pstCropPrm->width, ringBufHandle->u64WriteDataLoc);
        return SAL_FAIL;
    }
    if (SAL_SUB_ABS(u64ReadLoc, ringBufHandle->u64WriteDataLoc) > ringBufHandle->s32IdxRange)
    {
        SAL_LOGE("The gap between the specified read loc[%llu] and write loc[%llu] is too wide[%llu]( > idx range[%d])\n", 
                 u64ReadLoc, ringBufHandle->u64WriteDataLoc, SAL_SUB_ABS(u64ReadLoc, ringBufHandle->u64WriteDataLoc), ringBufHandle->s32IdxRange);
        return SAL_FAIL;
    }
    if (SAL_SUB_ABS(u64ReadLoc + u32ReadOffset + pstCropPrm->width, ringBufHandle->u64WriteDataLoc) > ringBufHandle->s32IdxRange)
    {
        SAL_LOGE("The gap between the specified read loc[%llu + %u + %u] and write loc[%llu] is too wide[%llu]( > idx range[%d])\n", 
                 u64ReadLoc, u32ReadOffset, pstCropPrm->width, ringBufHandle->u64WriteDataLoc, 
                 SAL_SUB_ABS(u64ReadLoc + u32ReadOffset + pstCropPrm->width, ringBufHandle->u64WriteDataLoc), ringBufHandle->s32IdxRange);
        return SAL_FAIL;
    }

    if (RING_DIR_UP == ringBufHandle->enRingBufDir)
    {
        s32MemoryOffset = (u64ReadLoc + u32ReadOffset) % ringBufHandle->s32IdxRange;
        /* 转换为对内存起点的偏移量 */
        /* 位于最后一屏范围内时，从第一屏取数据 */
        if (s32MemoryOffset > ringBufHandle->s32LastScreenWidxStart && s32MemoryOffset < ringBufHandle->s32LastScreenWidxEnd)
        {
            s32MemoryOffset = s32MemoryOffset - ringBufHandle->s32LastScreenWidxStart;
        }
        else
        {
            s32MemoryOffset = s32MemoryOffset + ringBufHandle->s32ScreenWidth;
        }
    }
    else
    {
        s32MemoryOffset = (u64ReadLoc + u32ReadOffset + u32ReadWidth) % ringBufHandle->s32IdxRange;
        s32MemoryOffset = ringBufHandle->s32IdxRange - s32MemoryOffset;
    }

    u32ExtraCropOffset = ringBufHandle->s32ScreenWidth;
    u32ExtraCropWidth = SAL_SUB_SAFE(s32MemoryOffset + u32ReadWidth, ringBufHandle->s32RingWidth);

    pstRingBufImg = &ringBufHandle->stRingDispImg[0];

    stDstCropPrm.top = stSrcCropPrm.top = pstCropPrm->top;
    stDstCropPrm.height = stSrcCropPrm.height = pstCropPrm->height;
    if (0 < u32ExtraCropWidth)
    {
        stSrcCropPrm.left = u32ExtraCropOffset;
        stDstCropPrm.left = ringBufHandle->s32RingWidth;
        stDstCropPrm.width = stSrcCropPrm.width = u32ExtraCropWidth;

        ximg_crop(pstRingBufImg, NULL, &stSrcCropPrm, &stDstCropPrm);
    }

    stSrcCropPrm.left = s32MemoryOffset;
    stSrcCropPrm.width = u32ReadWidth;

    /* 输出图像地址有效时，进行图像硬拷贝，否则执行软拷贝，仅拷贝图像信息 */
    if (NULL != pstOutImage->pPlaneVir[0])
    {
        /* 从完整循环缓冲中截取指定位置图像输出 */
        if (SAL_SOK != ximg_crop(pstRingBufImg, pstOutImage, &stSrcCropPrm, NULL))
        {
            SAL_LOGE("ximg_crop failed, src[%u x %u], crop[%u %u %u %u].\n", 
                    pstRingBufImg->u32Width, pstRingBufImg->u32Height, 
                    stSrcCropPrm.left, stSrcCropPrm.top, stSrcCropPrm.width, stSrcCropPrm.height);
            return SAL_FAIL;
        }
    }
    else
    {
        if (SAL_SOK != ximg_create_sub(pstRingBufImg, pstOutImage, &stSrcCropPrm))
        {
            SAL_LOGE("ximg_create_sub failed, src[%u x %u], crop[%u %u %u %u].\n", 
                     pstRingBufImg->u32Width, pstRingBufImg->u32Height, 
                     stSrcCropPrm.left, stSrcCropPrm.top, stSrcCropPrm.width, stSrcCropPrm.height);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   ring_buf_size
 * @brief      获取循环缓冲中待显示的数据量
 *  
 * @param[in]  ring_buf_handle ringBufHandle 循环缓冲句柄
 * @param[out] UINT64 *pu64WriteLoc 当前循环缓冲区的写位置，可为NULL，非NULL时才输出
 * @param[out] UINT64 *pu64ReadLoc  当前循环缓冲区的读位置，可为NULL，非NULL时才输出
 * @param[out] UINT64 *pu64DispLocS 当前显示的起始读位置，可为NULL，非NULL时才输出
 * @param[out] UINT64 *pu64DispLocE 当前显示的结束读位置，可为NULL，非NULL时才输出
 *  
 * @return     INT32 数据量，单位：pixel，-1表示出错
 */
INT32 ring_buf_size(ring_buf_handle ringBufHandle, UINT64 *pu64WriteLoc, UINT64 *pu64ReadLoc, UINT64 *pu64DispLocS, UINT64 *pu64DispLocE)
{
    UINT64 u64RLoc = 0, u64WLoc = 0;
    UINT64 u64DispLocS = 0, u64DispLocE = 0; // 当前显示图像的读写位置
    CHECK_PTR_IS_NULL(ringBufHandle, -1);

    u64WLoc = ringBufHandle->u64WriteDataLoc;
    u64RLoc = ringBufHandle->u64ReadDataLoc;
    if (NULL != pu64WriteLoc)
    {
        *pu64WriteLoc = u64WLoc;
    }
    if (NULL != pu64ReadLoc)
    {
        *pu64ReadLoc = u64RLoc;
    }    

    if (NULL != pu64DispLocS || NULL != pu64DispLocE)
    {
        if (RING_MODE_TWOWAY == ringBufHandle->enRingBufMode)
        {
            if (u64WLoc < u64RLoc) // 反向回拉（与过包方向相同）
            {
                u64DispLocS = u64WLoc;
                u64DispLocE = u64RLoc;
            }
            else // 正向回拉（与过包方向相反）
            {
                u64DispLocS = SAL_SUB_SAFE(u64RLoc, ringBufHandle->s32ScreenWidth);
                u64DispLocE = u64WLoc;
            }
        }
        else
        {
            u64DispLocS = SAL_SUB_SAFE(u64RLoc, ringBufHandle->s32ScreenWidth);
            u64DispLocE = u64WLoc;
        }
        if (NULL != pu64DispLocS)
        {
            *pu64DispLocS = u64DispLocS;
        }
        if (NULL != pu64DispLocE)
        {
            *pu64DispLocE = u64DispLocE;
        }   
    }

    return DIST(ringBufHandle->wIdx, ringBufHandle->rIdx, ringBufHandle->s32IdxRange);
}

/**
 * @function   ring_buf_status
 * @brief      打印循环缓冲区信息
 * @param[in]  ring_buf_handle ringBufHandle 循环缓冲句柄
 * @return     INT32 数据量，单位：pixel，-1表示出错
 */
SAL_STATUS ring_buf_status(ring_buf_handle ringBufHandle)
{
    INT32 s32Size = 0;
    UINT64 u64RLoc = 0, u64WLoc = 0;
    UINT64 u64DispImgLocS = 0, u64DispImgLocE = 0; // 当前显示图像的读写位置
    CHECK_PTR_IS_NULL(ringBufHandle, SAL_SOK);
    CHAR cDirString[RING_DIR_MAX][16] = {"none", "up", "down"};
    CHAR cModeString[RING_MODE_MAX][16] = {"one way", "tow way"};

    s32Size = ring_buf_size(ringBufHandle, &u64WLoc, &u64RLoc, &u64DispImgLocS, &u64DispImgLocE);

    SAL_print("\nhandle:%p, buffer height:%u, screen height:%d, duplicated buffer number:%d, data format:0x%x, bg:0x%x\n",
              ringBufHandle, ringBufHandle->stRingDispImg[0].u32Height, ringBufHandle->s32ScreenHeight,
              ringBufHandle->s32DplctdBufNum, ringBufHandle->stRingDispImg[0].enImgFmt, ringBufHandle->u32BgValue);
    SAL_print("\n|**************************** buffer width[%4d] ****************************|\n", ringBufHandle->s32BufWidth);
    SAL_print("|**************************** ring width[%4d] **************************|   |\n", ringBufHandle->s32RingWidth);
    SAL_print("|*** screen width[%4d] ***|************** idx range[%4d] **************|   |\n", ringBufHandle->s32ScreenWidth, ringBufHandle->s32IdxRange);
    SAL_print("|                          |                    |*** max crop width[%4d] ***|\n", ringBufHandle->s32MaxCropWidth);

    SAL_print("\n|      dir |  lastDir |     mode |          |  wIdx |  rIdx |              wLocation |              rLocation |\n"
              "| %8s | %8s | %8s | reserved | %5d | %5d | %22llu | %22llu | left data size %d\n" 
              "|                                   current | %5d | %5d | %22llu | %22llu | screenLoc [%llu %llu]\n", 
              cDirString[ringBufHandle->enRingBufDir], cDirString[ringBufHandle->enLastWriteDir], cModeString[ringBufHandle->enRingBufMode], 
              ringBufHandle->s32ReservedWIdx, ringBufHandle->s32ReservedRIdx, 
              ringBufHandle->u64ReservedWDataLoc, ringBufHandle->u64ReservedRDataLoc, s32Size, 
              ringBufHandle->wIdx, ringBufHandle->rIdx, u64WLoc, u64RLoc, u64DispImgLocS, u64DispImgLocE);

    return SAL_SOK;
}


/**
 * @function   segment_buf_update_prm
 * @brief      更新分段缓冲前后部分参数，有回头写时，分两次更新参数，第一次刚好写满一屏，第二次为回头部分
 * @param[in]  ring_buf_handle ringBufHandle 循环缓冲句柄
 * @param[in]  UINT32          u32DataHeight 循环缓冲句柄
 * @param[out] NONE
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
static SAL_STATUS segment_buf_update_prm(segment_buf_handle pSegmentBufHandle, UINT32 u32SequentWriteLen)
{
    UINT32 *pu32GrowPartOffset = NULL;
    UINT32 *pu32GrowPartLen = NULL;
    UINT32 *pu32ShrinkPartOffset = NULL;
    UINT32 *pu32ShrinkPartLen = NULL;
    UINT32 u32GrowPartMoveMax = 0;
    UINT32 u32CurGrowPartLen = 0;

    CHECK_PTR_NULL(pSegmentBufHandle, SAL_FAIL);
    if (u32SequentWriteLen > pSegmentBufHandle->buffLen)
    {
        SAL_LOGE("Invalid segment put len[%u], which should be less than cached len[%u]\n", u32SequentWriteLen, pSegmentBufHandle->buffLen);
        return SAL_FAIL;
    }

    pSegmentBufHandle->bGoToInit = SAL_FALSE;
    if (RING_DIR_DOWN == pSegmentBufHandle->u32CurDir)
    {
        /* 前部减小，尾部增加 */
        pu32GrowPartOffset = &pSegmentBufHandle->u32LatterPartOffset;
        pu32GrowPartLen = &pSegmentBufHandle->u32LatterPartLen;
        pu32ShrinkPartOffset = &pSegmentBufHandle->u32FrontPartOffset;
        pu32ShrinkPartLen = &pSegmentBufHandle->u32FrontPartLen;
        u32GrowPartMoveMax = pSegmentBufHandle->buffLen - pSegmentBufHandle->u32ScreenLen;
    }
    else
    {
        /* 前部增加，尾部减小 */
        pu32GrowPartOffset = &pSegmentBufHandle->u32FrontPartOffset;
        pu32GrowPartLen = &pSegmentBufHandle->u32FrontPartLen;
        pu32ShrinkPartOffset = &pSegmentBufHandle->u32LatterPartOffset;
        pu32ShrinkPartLen = &pSegmentBufHandle->u32LatterPartLen;
        u32GrowPartMoveMax = 0;
    }

    /* 收缩部分长度不为0，继续收缩 */
    if (0 < *pu32ShrinkPartLen)
    {
        *pu32GrowPartLen += u32SequentWriteLen;
        u32CurGrowPartLen = *pu32GrowPartLen;
        if (*pu32ShrinkPartLen >= u32SequentWriteLen)
        {
            *pu32ShrinkPartLen -= u32SequentWriteLen;
        }
        else    // 收缩部分不足一次写入数据长度，收缩部分清0，增长部分到大最大
        {
            *pu32GrowPartLen = pSegmentBufHandle->u32ScreenLen;
            *pu32ShrinkPartLen = 0;
        }
        if (RING_DIR_DOWN == pSegmentBufHandle->u32CurDir)
        {
            pSegmentBufHandle->u32LatterPartOffset = SAL_SUB_SAFE(u32CurGrowPartLen, pSegmentBufHandle->u32ScreenLen);
            pSegmentBufHandle->u32FrontPartOffset = pSegmentBufHandle->buffLen - pSegmentBufHandle->u32FrontPartLen;
        }
        else
        {
            pSegmentBufHandle->u32LatterPartOffset = 0;
            pSegmentBufHandle->u32FrontPartOffset = pSegmentBufHandle->buffLen - u32CurGrowPartLen;
        }
    }
    else
    {
        if (*pu32GrowPartOffset != u32GrowPartMoveMax)
        {
            /* 增长部分平移 */
            *pu32ShrinkPartLen = 0;
            *pu32GrowPartLen = pSegmentBufHandle->u32ScreenLen;
            if (RING_DIR_DOWN == pSegmentBufHandle->u32CurDir)
            {
                *pu32GrowPartOffset += u32SequentWriteLen;
                *pu32ShrinkPartOffset = pSegmentBufHandle->buffLen;
            }
            else
            {
                *pu32GrowPartOffset -= u32SequentWriteLen;
                *pu32ShrinkPartOffset = 0;
            }
        }
    }

    /* 增长部分移动到头，将其复位为收缩部分 */
    if (*pu32GrowPartOffset == u32GrowPartMoveMax)
    {
        *pu32ShrinkPartLen = *pu32GrowPartLen;
        *pu32ShrinkPartOffset = *pu32GrowPartOffset;
        *pu32GrowPartLen = 0;
        if (RING_DIR_DOWN == pSegmentBufHandle->u32CurDir)
        {
            *pu32GrowPartOffset = 0;
        }
        else
        {
            *pu32GrowPartOffset = pSegmentBufHandle->buffLen;
        }

        pSegmentBufHandle->bGoToInit = SAL_TRUE;
    }

    return SAL_SOK;
}

/**
 * @function   segment_buf_init
 * @brief      创建一个分段存储缓冲
 * @param[in]  UINT32         u32BufWidth  内存宽度，单位：pixel
 * @param[in]  UINT32         buffLen      缓冲分段存储的数据长度,也是内存高度，单位：pixel
 * @param[in]  UINT32         u32ScreenLen 屏幕长度,也即分段缓冲存储的单屏数据长度，单位：pixel
 * @param[in]  DSP_IMG_DATFMT enDataFmt    数据类型
 * @param[out] NONE
 * @return     segment_buf_handle 分段存储缓冲的句柄
 */
segment_buf_handle segment_buf_init(UINT32 u32BufWidth, UINT32 buffLen, UINT32 u32ScreenLen, DSP_IMG_DATFMT enDataFmt)
{
    segment_buf_handle segmentBuf = NULL;
    XIMAGE_DATA_ST *pstSegDispImg = NULL;
    MEDIA_BUF_ATTR *pstMediaBuf = NULL;
    CAPB_XSP *pstCapbXsp = capb_get_xsp();

    if (u32BufWidth < pstCapbXsp->xraw_width_resized || buffLen < pstCapbXsp->xraw_height_resized)
    {
        SAL_LOGE("Segment buf size %u x %u is too small, require at least %ux%u\n", 
                 u32BufWidth, buffLen, pstCapbXsp->xraw_width_resized, pstCapbXsp->xraw_height_resized);
        return NULL;
    }

    segmentBuf = (segment_buf_handle)malloc(sizeof(SEGMENT_BUF_ATTR));
    memset(segmentBuf, 0x0, sizeof(SEGMENT_BUF_ATTR));

    pstSegDispImg = &segmentBuf->stSegDispImg;
    pstMediaBuf = &pstSegDispImg->stMbAttr;
    SAL_clear(pstMediaBuf);
    if (SAL_SOK != ximg_create(u32BufWidth, buffLen, enDataFmt, "segment", NULL, pstSegDispImg))
    {
        SAL_LOGE("ximg_create failed, w:%u, h:%u, size:%u fmt:0x%x\n", 
                    pstSegDispImg->u32Width, pstSegDispImg->u32Height, pstMediaBuf->memSize, pstSegDispImg->enImgFmt);
        free(segmentBuf);
        return NULL;
    }

    /* 参数初始化 */
    segmentBuf->buffLen = buffLen;
    segmentBuf->u32ScreenLen = u32ScreenLen;
    segmentBuf->u32CurDir = RING_DIR_DOWN;
    segment_buf_reset(segmentBuf);

    return segmentBuf;
}

/**
 * @function   segment_buf_deinit
 * @brief      销毁分段存储缓冲，释放资源
 * @param[in]  segment_buf_handle pSegmentBufHandle 循环缓冲句柄
 * @param[out] NONE
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS segment_buf_deinit(segment_buf_handle pSegmentBufHandle)
{
    CHECK_PTR_IS_NULL(pSegmentBufHandle, SAL_FAIL);

    xpack_media_buf_free(&pSegmentBufHandle->stSegDispImg.stMbAttr);

    free(pSegmentBufHandle);
    pSegmentBufHandle = NULL;
    return SAL_SOK;
}

/**
 * @function   segment_buf_reset
 * @brief      分段存储缓冲参数复位
 * @param[in]  segment_buf_handle pSegmentBufHandle 分段存储缓冲句柄 
 * @param[out] NONE
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS segment_buf_reset(segment_buf_handle pSegmentBufHandle)
{
    CHECK_PTR_IS_NULL(pSegmentBufHandle, SAL_FAIL);
 
    pSegmentBufHandle->bResetDir = SAL_TRUE;
    /* 全部作为前部 */
    if (RING_DIR_DOWN == pSegmentBufHandle->u32CurDir)
    {
        pSegmentBufHandle->u32LatterPartOffset = 0;
        pSegmentBufHandle->u32LatterPartLen = 0;
        pSegmentBufHandle->u32FrontPartLen = pSegmentBufHandle->u32ScreenLen;
        pSegmentBufHandle->u32FrontPartOffset = pSegmentBufHandle->buffLen - pSegmentBufHandle->u32FrontPartLen;
    }
    /* 全部作为后部 */
    else
    {
        pSegmentBufHandle->u32LatterPartOffset = 0;
        pSegmentBufHandle->u32LatterPartLen = pSegmentBufHandle->u32ScreenLen;
        pSegmentBufHandle->u32FrontPartOffset = pSegmentBufHandle->buffLen;
        pSegmentBufHandle->u32FrontPartLen = 0;
    }

    ximg_fill_color(&pSegmentBufHandle->stSegDispImg, 0, pSegmentBufHandle->stSegDispImg.u32Height, 0, pSegmentBufHandle->stSegDispImg.u32Width, 0);

    return SAL_SOK;
}

/**
 * @function   segment_buf_put
 * @brief      向分段存储缓冲中添加数据，也可以是空白数据，空白数据不需实际数据，只需要宽高信息
 * @param[in]  segment_buf_handle  pSegmentBufHandle 分段缓冲句柄
 * @param[in]  XIMAGE_DATA_ST      *pstSegmentDataIn 添加的源数据
 * @param[in]        enImgFmt;                        图像格式
 * @param[in]        u32Width;                        图像宽，pPlaneVir[0]为空时表示添加的空白数据宽
 * @param[in]        u32Height;                       图像高，pPlaneVir[0]为空时表示添加的空白数据高
 *                   u32Stride[XIMAGE_PLANE_MAX];     图像跨距
 * @param[in]        *pPlaneVir[XIMAGE_PLANE_MAX];    为空时，表示向缓存添加空白数据
 *                   stMbAttr;                        Media Buffer属性
 * @param[in]  RING_BUF_DIR        enInDir            循环方向，方向具体值与分段缓存写方向不相关，当方向切换时分段缓存写方向改变
 * @param[in]  UINT32              *pu32RawDataLoc  当前写入的数据在分段缓存中的位置，以起始位置计
 * @param[out] None
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS segment_buf_put(segment_buf_handle pSegmentBufHandle, XIMAGE_DATA_ST *pstSegmentDataIn, RING_BUF_DIR enInDir, UINT32 *pu32RawDataLoc)
{
    UINT32 u32DataHeight = 0;

    UINT32 u32FPartOffset = 0;
    UINT32 u32LPartOffset = 0;
    UINT32 u32LPartLen = 0;

    UINT32 u32SequentWriteOffset = 0;       // 按内存顺序写的数据宽度
    UINT32 u32SequentWriteLen = 0;          // 按内存顺序写的数据宽度
    UINT32 u32WriteBackOffset = 0;          // 超出内存范围，回头写的数据宽度
    UINT32 u32WriteBackLen = 0;             // 超出内存范围，回头写的数据宽度
    UINT32 u32SeqWritePartTop = 0;
    UINT32 u32BackWritePartTop = 0;

    SAL_VideoCrop stSrcCropPrm = {0};
    SAL_VideoCrop stDstCropPrm = {0};
    XIMAGE_DATA_ST *pstSegImg = NULL;

    CHECK_PTR_IS_NULL(pSegmentBufHandle, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstSegmentDataIn, SAL_FAIL);

    if (pSegmentBufHandle->bResetDir)
    {
        /* 保证初始化后第一次添加数据时上一次和当前方向一致 */
        pSegmentBufHandle->bResetDir = SAL_FALSE;
        pSegmentBufHandle->enLastInDir = enInDir;
        pSegmentBufHandle->u32CurDir = RING_DIR_DOWN;
    }

    /* 方向切换触发缓存循环方向改变 */
    if (enInDir != pSegmentBufHandle->enLastInDir)
    {
        pSegmentBufHandle->u32CurDir = RING_DIR_DOWN == pSegmentBufHandle->u32CurDir ? RING_DIR_UP : RING_DIR_DOWN;
        
        //if (pSegmentBufHandle->enLastInDir != RING_DIR_NONE)
        if (SAL_FALSE == pSegmentBufHandle->bGoToInit)
        {
            if (pSegmentBufHandle->u32ScreenLen == pSegmentBufHandle->u32FrontPartLen || 
                pSegmentBufHandle->u32ScreenLen == pSegmentBufHandle->u32LatterPartLen)
            {
                XPACK_LOGW("front and latter changed\n");
                if (pSegmentBufHandle->u32ScreenLen == pSegmentBufHandle->u32LatterPartLen)
                {
                    pSegmentBufHandle->u32FrontPartOffset = pSegmentBufHandle->u32LatterPartOffset;
                    pSegmentBufHandle->u32FrontPartLen = pSegmentBufHandle->u32LatterPartLen;
                    pSegmentBufHandle->u32LatterPartOffset = 0;
                    pSegmentBufHandle->u32LatterPartLen = 0;
                }
                else
                {
                    pSegmentBufHandle->u32LatterPartOffset = pSegmentBufHandle->u32FrontPartOffset;
                    pSegmentBufHandle->u32LatterPartLen = pSegmentBufHandle->u32FrontPartLen;
                    pSegmentBufHandle->u32FrontPartOffset = pSegmentBufHandle->buffLen;
                    pSegmentBufHandle->u32FrontPartLen = 0;
                }
            }
        }
    }

    pSegmentBufHandle->enLastInDir = enInDir;
    pstSegImg = &pSegmentBufHandle->stSegDispImg;
    u32DataHeight = pstSegmentDataIn->u32Height;

    u32FPartOffset = pSegmentBufHandle->u32FrontPartOffset;
    u32LPartOffset = pSegmentBufHandle->u32LatterPartOffset;
    u32LPartLen = pSegmentBufHandle->u32LatterPartLen;

    /* 同向写入,分段缓冲区的前部减小，后部扩大，从后部结尾往后写 */
    if (RING_DIR_DOWN == pSegmentBufHandle->u32CurDir)
    {
        u32SequentWriteOffset = u32LPartOffset + u32LPartLen;
        u32SequentWriteLen = u32DataHeight;
        /* 数据有回头 */
        if (u32DataHeight > pSegmentBufHandle->buffLen - u32SequentWriteOffset)
        {
            u32SequentWriteLen = pSegmentBufHandle->buffLen - u32SequentWriteOffset;
            u32WriteBackLen = u32DataHeight - u32SequentWriteLen;
            u32WriteBackOffset = 0;                                 // 回头部分从缓冲区顶部写

            u32SeqWritePartTop = 0;
            u32BackWritePartTop = u32SequentWriteLen;
        }
    }
    /* 反向写入,分段缓冲区的前部扩大，后部减小，从前部开头往前写 */
    else
    {
        /* 由于回拉raw数据根据方向有镜像调整 */
        if (SAL_SOK != ximg_flip(pstSegmentDataIn, pstSegmentDataIn, XIMG_FLIP_VERT))
        {
            SAL_LOGE("ximg_flip failed, %u x %u.\n", 
                    pstSegmentDataIn->u32Width, pstSegmentDataIn->u32Height);
            return SAL_FAIL;
        }

        if (u32DataHeight <= u32FPartOffset)
        {
            u32SequentWriteOffset = u32FPartOffset - u32DataHeight;
            u32SequentWriteLen = u32DataHeight;
        }
        else
        {
            /* 数据有回头 */
            u32SequentWriteOffset = 0;
            u32SequentWriteLen = u32FPartOffset;
            u32WriteBackLen = u32DataHeight - u32SequentWriteLen;
            u32WriteBackOffset = pSegmentBufHandle->buffLen - u32WriteBackLen;  // 回头部分从缓冲区底部写

            u32SeqWritePartTop = u32DataHeight - u32SequentWriteLen;
            u32BackWritePartTop = 0;
        }
    }

    SAL_LOGD("inDir:%d, dir:%d, sequentWrite[Offset:%u, Len:%u], writeBack[Offset:%u, Len:%u], fro[offset:%u, w:%u], lat[offset:%u, w:%u]\n", 
               enInDir, pSegmentBufHandle->u32CurDir, u32SequentWriteOffset, u32SequentWriteLen, u32WriteBackOffset, u32WriteBackLen, 
               pSegmentBufHandle->u32FrontPartOffset, pSegmentBufHandle->u32FrontPartLen, pSegmentBufHandle->u32LatterPartOffset, pSegmentBufHandle->u32LatterPartLen);

    /* 向左和向右出图时全屏nraw数据按相同规格存储，只是给算法配置的旋转和镜像方向有不同 */
    if (NULL != pstSegmentDataIn->pPlaneVir[0])     // 数据地址为空时不拷贝数据，仅更新参数
    {
        stSrcCropPrm.left = 0;
        stSrcCropPrm.top = u32SeqWritePartTop;
        stSrcCropPrm.width = pstSegmentDataIn->u32Width;
        stSrcCropPrm.height = u32SequentWriteLen;

        stDstCropPrm.left = 0;
        stDstCropPrm.top = u32SequentWriteOffset;
        stDstCropPrm.width = pstSegImg->u32Width;
        stDstCropPrm.height = u32SequentWriteLen;
        if (SAL_SOK != ximg_crop(pstSegmentDataIn, pstSegImg, &stSrcCropPrm, &stDstCropPrm))
        {
            goto EXIT;
        }

        if (NULL != pu32RawDataLoc)
        {
            *pu32RawDataLoc = u32SequentWriteOffset;
        }
    }
    segment_buf_update_prm(pSegmentBufHandle, u32SequentWriteLen);

    /* 回头数据拷贝 */
    if (0 < u32WriteBackLen)
    {
        if (NULL != pstSegmentDataIn->pPlaneVir[0])
        {
            stSrcCropPrm.left = 0;
            stSrcCropPrm.top = u32BackWritePartTop;
            stSrcCropPrm.width = pstSegmentDataIn->u32Width;
            stSrcCropPrm.height = u32WriteBackLen;

            stDstCropPrm.left = 0;
            stDstCropPrm.top = u32WriteBackOffset;
            stDstCropPrm.width = pstSegImg->u32Width;
            stDstCropPrm.height = u32WriteBackLen;
            if (SAL_SOK != ximg_crop(pstSegmentDataIn, pstSegImg, &stSrcCropPrm, &stDstCropPrm))
            {
                goto EXIT;
            }
        }
        segment_buf_update_prm(pSegmentBufHandle, u32WriteBackLen);
    }

    return SAL_SOK;
EXIT:
    SAL_LOGE("ximg_crop failed, src[%u x %u], crop[%u %u %u %u], inDir:%d, dir:%d, sequentWrite[Offset:%u, Len:%u], writeBack[Offset:%u, Len:%u], fro[offset:%u, w:%u], lat[offset:%u, w:%u]\n", 
             pstSegmentDataIn->u32Width, pstSegmentDataIn->u32Height, 
             stSrcCropPrm.left, stSrcCropPrm.top, stSrcCropPrm.width, stSrcCropPrm.height, 
             enInDir, pSegmentBufHandle->u32CurDir, u32SequentWriteOffset, u32SequentWriteLen, u32WriteBackOffset, u32WriteBackLen, 
             pSegmentBufHandle->u32FrontPartOffset, pSegmentBufHandle->u32FrontPartLen, pSegmentBufHandle->u32LatterPartOffset, pSegmentBufHandle->u32LatterPartLen);
    return SAL_FAIL;
}

/**
 * @function   segment_buf_update
 * @brief      将另一个分段缓冲信息更新到本分段缓冲中
 * @param[in]  segment_buf_handle  pSegmentBufHandle 需要更新的分段缓冲句柄
 * @param[in]  segment_buf_handle  pSegSrcBufHandle  信息来源分段缓冲句柄
 * @param[out] None
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS segment_buf_update(segment_buf_handle pSegmentBufHandle, segment_buf_handle pSegSrcBufHandle)
{
    CHECK_PTR_IS_NULL(pSegmentBufHandle, SAL_FAIL);
    CHECK_PTR_IS_NULL(pSegSrcBufHandle, SAL_FAIL);

    pSegmentBufHandle->bResetDir = SAL_FALSE;
    pSegmentBufHandle->bGoToInit = pSegSrcBufHandle->bGoToInit;
    pSegmentBufHandle->u32CurDir = pSegSrcBufHandle->u32CurDir;
    pSegmentBufHandle->enLastInDir = pSegSrcBufHandle->enLastInDir;
    pSegmentBufHandle->u32FrontPartLen = pSegSrcBufHandle->u32FrontPartLen;
    pSegmentBufHandle->u32FrontPartOffset = pSegSrcBufHandle->u32FrontPartOffset;
    pSegmentBufHandle->u32LatterPartLen = pSegSrcBufHandle->u32LatterPartLen;
    pSegmentBufHandle->u32LatterPartOffset = pSegSrcBufHandle->u32LatterPartOffset;
    ximg_flip(&pSegSrcBufHandle->stSegDispImg, &pSegmentBufHandle->stSegDispImg, XIMG_FLIP_NONE);

    return SAL_SOK;
}

/**
 * @function        segment_buf_get
 * @brief           从分段存储缓冲中获取当前显示部分的raw数据，默认获取一屏范围的数据
 * @param[IN]       segment_buf_handle pSegmentBufHandle 分段缓冲句柄
 * @param[IN]       SEG_BUF_DATA       *pstRawBuf        获取的分段缓冲存储数据信息
 * @param[IN/OUT]:  UINT32             *pu32Length       获取的分段缓冲数据高度，非空时传入期望获取的高度，返回实际获取的高度，该值介于屏幕宽度和分段缓存长度之间
 * @param[OUT]      None
 * @return          SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS segment_buf_get(segment_buf_handle pSegmentBufHandle, SEG_BUF_DATA *pstRawBuf, UINT32 *pu32Length)
{
    UINT32 u32OutsideLen = 0;   // 获取的位于屏幕范围外的数据长度
    UINT32 u32GotLen = 0;       // 实际获取的数据长度
    
    CHECK_PTR_IS_NULL(pSegmentBufHandle, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstRawBuf, SAL_FAIL);

    u32GotLen = pSegmentBufHandle->u32ScreenLen;    // 默认获取一屏数据
    if (NULL != pu32Length)
    {
        if (*pu32Length < pSegmentBufHandle->u32ScreenLen)
        {
            SAL_LOGE("Got data length requirs at least screen length[%u]\n", pSegmentBufHandle->u32ScreenLen);
            return SAL_FAIL;
        }
        if (*pu32Length > pSegmentBufHandle->buffLen)
        {
            SAL_LOGW("Got data length[%u] is limited to buffer length[%u]\n", *pu32Length, pSegmentBufHandle->buffLen);
            *pu32Length = pSegmentBufHandle->buffLen;
        }
        u32GotLen = *pu32Length;
    }

    u32OutsideLen = u32GotLen - pSegmentBufHandle->u32ScreenLen;
    if (RING_DIR_DOWN == pSegmentBufHandle->u32CurDir)
    {
        /* 此方向优先取LatterPart数据，剩下的从FrontPart和其之上的数据中取 */
        if (0 != pSegmentBufHandle->u32FrontPartLen)
        {
            /* 缓存的当前一屏数据不连续时 */
            pstRawBuf->u32FrontPartOffset = pSegmentBufHandle->u32FrontPartOffset - u32OutsideLen;
            pstRawBuf->u32FrontPartLen = pSegmentBufHandle->u32FrontPartLen + u32OutsideLen;
            pstRawBuf->u32LatterPartOffset = pSegmentBufHandle->u32LatterPartOffset;
            pstRawBuf->u32LatterPartLen = pSegmentBufHandle->u32LatterPartLen;
        }
        else
        {
            /* 缓存的当前一屏数据连续时 */
            if (pSegmentBufHandle->u32LatterPartOffset >= u32OutsideLen)
            {
                /* 当前全屏内存之前的部分已够获取的一屏之外的数据 */
                pstRawBuf->u32LatterPartOffset = pSegmentBufHandle->u32LatterPartOffset - u32OutsideLen;
                pstRawBuf->u32LatterPartLen = pSegmentBufHandle->u32LatterPartLen + u32OutsideLen;
                pstRawBuf->u32FrontPartOffset = 0;
                pstRawBuf->u32FrontPartLen = 0;
            }
            else
            {
                /* 当前全屏内存之前的部分不够获取的一屏之外的数据，剩下的部分从内存尾获取 */
                pstRawBuf->u32LatterPartOffset = 0;
                pstRawBuf->u32LatterPartLen = pSegmentBufHandle->u32LatterPartLen + pSegmentBufHandle->u32LatterPartOffset;
                pstRawBuf->u32FrontPartLen = u32OutsideLen - pSegmentBufHandle->u32LatterPartOffset;
                pstRawBuf->u32FrontPartOffset = pSegmentBufHandle->buffLen - pstRawBuf->u32FrontPartLen;
            }
        }
    }
    else
    {
        /* 此方向优先取FrontPart数据，剩下的从LatterPart和其之下的数据中取 */
        if (0 != pSegmentBufHandle->u32LatterPartLen)
        {
            pstRawBuf->u32FrontPartOffset = pSegmentBufHandle->u32FrontPartOffset;
            pstRawBuf->u32FrontPartLen = pSegmentBufHandle->u32FrontPartLen;
            pstRawBuf->u32LatterPartOffset = pSegmentBufHandle->u32LatterPartOffset;
            pstRawBuf->u32LatterPartLen = pSegmentBufHandle->u32LatterPartLen + u32OutsideLen;
        }
        else
        {
            /* 缓存的当前一屏数据连续时 */
            if (pSegmentBufHandle->buffLen - (pSegmentBufHandle->u32FrontPartOffset + pSegmentBufHandle->u32FrontPartLen) >= u32OutsideLen)
            {
                /* 当前全屏内存之后的部分已够获取的一屏之外的数据 */
                pstRawBuf->u32LatterPartOffset = 0;
                pstRawBuf->u32LatterPartLen = 0;
                pstRawBuf->u32FrontPartOffset = pSegmentBufHandle->u32FrontPartOffset;
                pstRawBuf->u32FrontPartLen = pSegmentBufHandle->u32FrontPartLen + u32OutsideLen;
            }
            else
            {
                /* 当前全屏内存之后的部分不够获取的一屏之外的数据，剩下的部分从内存头获取 */
                pstRawBuf->u32FrontPartOffset = pSegmentBufHandle->u32FrontPartOffset;
                pstRawBuf->u32FrontPartLen = pSegmentBufHandle->buffLen - pSegmentBufHandle->u32FrontPartOffset;
                pstRawBuf->u32LatterPartOffset = 0;
                pstRawBuf->u32LatterPartLen = u32OutsideLen - (pstRawBuf->u32FrontPartLen - pSegmentBufHandle->u32FrontPartLen);
            }
        }
    }

    ximg_create_sub(&pSegmentBufHandle->stSegDispImg, &pstRawBuf->stFscRawData, NULL);

    return SAL_SOK;
}

/**
 * @function        segment_buf_crop
 * @brief           从分段存储缓冲中指定位置抠取数据，均是沿高度方向抠取
 * @param[IN]       segment_buf_handle  pSegmentBufHandle 分段缓冲句柄
 * @param[IN/OUT]:  UINT32              u32CropLoc        指定抠取位置，抠取数据位置由segment_buf_put接口pu32RawDataLoc参数返回
 * @param[IN]       UINT32              u32CropOffset     实际抠取位置与u32CropLoc偏移量，即实际读取位置为u32CropLoc+u32CropOffset
 * @param[IN]       UINT32              u32CropLen        抠取的数据长度
 * @param[IN/OUT]   XIMAGE_DATA_ST      *pstOutImage:     获取的图像，由外部通过ximg_create(_sub)接口创建空白图像
 * @param[IN]           enImgFmt    图像格式，见枚举DSP_IMG_DATFMT
 * @param[IN]           u32Width    抠取图像宽，单位：Pixel
 * @param[IN]           u32Height   抠取图像高，单位：Pixel
 * @param[IN]           u32Stride   内存跨距，单位：Pixel
 * @param[IN/OUT]       pPlaneVir   图像数据
 * @param[IN]           stMbAttr    内存信息
 * @param[OUT]      None
 * @return          SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS segment_buf_crop(segment_buf_handle pSegmentBufHandle, UINT32 u32CropLoc, UINT32 u32CropOffset, UINT32 u32CropLen, XIMAGE_DATA_ST *pstOutImage)
{
    UINT32 u32SeqCropLoc = 0, u32SeqCropLen = 0, u32BackCropLen = 0;
    SAL_VideoCrop stSrcCropPrm = {0};
    SAL_VideoCrop stDstCropPrm = {0};
    XIMAGE_DATA_ST *pstSegBufImg = NULL;

    CHECK_PTR_IS_NULL(pSegmentBufHandle, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstOutImage, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstOutImage->pPlaneVir[0], SAL_FAIL);

    pstSegBufImg = &pSegmentBufHandle->stSegDispImg;
    if (pstSegBufImg->enImgFmt != pstOutImage->enImgFmt)
    {
        SAL_LOGE("Output ximg format[0x%x] is not equal to segment buffer ximg format[0x%x]\n", pstOutImage->enImgFmt, pstSegBufImg->enImgFmt);
        return SAL_FAIL;
    }

    u32SeqCropLoc = (u32CropLoc + u32CropOffset) % pSegmentBufHandle->buffLen;
    u32BackCropLen = SAL_SUB_SAFE(u32SeqCropLoc + u32CropLen, pSegmentBufHandle->buffLen);
    u32SeqCropLen = SAL_SUB_SAFE(u32CropLen, u32BackCropLen);

    stSrcCropPrm.top = u32SeqCropLoc;
    stDstCropPrm.top = 0;
    stDstCropPrm.left = stSrcCropPrm.left = 0;
    stDstCropPrm.width = stSrcCropPrm.width = pstSegBufImg->u32Width;
    stDstCropPrm.height = stSrcCropPrm.height = u32SeqCropLen;

    if (SAL_SOK != ximg_crop(pstSegBufImg, pstOutImage, &stSrcCropPrm, &stDstCropPrm))
    {
        SAL_LOGE("ximg_crop failed, src[%u x %u], crop[%u %u %u %u], dst[%u x %u], crop[%u %u %u %u]\n", 
                 pstSegBufImg->u32Width, pstSegBufImg->u32Height, 
                 stSrcCropPrm.left, stSrcCropPrm.top, stSrcCropPrm.width, stSrcCropPrm.height, 
                 pstOutImage->u32Width, pstOutImage->u32Height, 
                 stDstCropPrm.left, stDstCropPrm.top, stDstCropPrm.width, stDstCropPrm.height);
        return SAL_FAIL;
    }

    SAL_LOGD("segHandle:%p, cropLoc:%u, offset:%u, len:%u, seq[%u, %u], back[0, %u]\n", 
               pSegmentBufHandle, u32CropLoc, u32CropOffset, u32CropLen, u32SeqCropLoc, u32SeqCropLen, u32BackCropLen);

    if (0 < u32BackCropLen)
    {
        stSrcCropPrm.top = 0;
        stDstCropPrm.top = u32SeqCropLen;
        stDstCropPrm.left = stSrcCropPrm.left = 0;
        stDstCropPrm.width = stSrcCropPrm.width = pstSegBufImg->u32Width;
        stDstCropPrm.height = stSrcCropPrm.height = u32BackCropLen;

        if (SAL_SOK != ximg_crop(pstSegBufImg, pstOutImage, &stSrcCropPrm, &stDstCropPrm))
        {
            SAL_LOGE("ximg_crop failed, src[%u x %u], crop[%u %u %u %u].\n", 
                     pstSegBufImg->u32Width, pstSegBufImg->u32Height, 
                     stSrcCropPrm.left, stSrcCropPrm.top, stSrcCropPrm.width, stSrcCropPrm.height);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}
