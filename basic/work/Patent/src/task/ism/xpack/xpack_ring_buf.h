/*******************************************************************************
* xpack_ring_buf.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : 
* Version: 
*
* Description : 1.定义了一种动态的循环扫描数据送显的全屏数据的循环缓冲,相关操作接口为ring_buf_xxx();
                2.定义了一种静态全屏raw数据分段存储缓冲,相关操作接口为segment_buf_xxx()。
*
*                                             条带                  
*                 LatterOffset-> ------------- 20 -------------  ↑
*                                ------------- 21 -------------  LatterLen
*                                ------------- 22 -------------  ↓
*                 FrontOffset -> ------------- 11 -------------  ↑
*                                ------------- 12 -------------  |
*                                ------------- 13 -------------  |
*                                ------------- 14 -------------  |
*                                ------------- 15 -------------  FrontLen
*                                ------------- 16 -------------  |
*                                ------------- 17 -------------  |
*                                ------------- 18 -------------  |
*                                ------------- 19 -------------  ↓
* Modification:
*******************************************************************************/
#ifndef _XPACK_RING_BUF_H_
#define _XPACK_RING_BUF_H_

/*****************************************************************************
                                头文件
*****************************************************************************/
#include "dspcommon.h"
#include "platform_hal.h"
#include "ximg_proc.h"

/*****************************************************************************
                                宏定义
*****************************************************************************/
#define DUPLI_PLANE_MAX (4)     // 循环缓冲支持的最大重复平面个数

/*****************************************************************************
                                结构体定义
*****************************************************************************/

typedef struct ringScanBuff* ring_buf_handle;
typedef struct segmentBufAttr* segment_buf_handle;

/*
 * 循环缓冲区读写索引移动的方向，与出图方向相关
 */
typedef enum
{
    RING_DIR_NONE,
    RING_DIR_UP,    // 循环缓冲区索引由低地址向高地址循环移动，此时向左出图
    RING_DIR_DOWN,  // 循环缓冲区索引由高地址向低地址循环移动，此时向右出图
    RING_DIR_MAX
} RING_BUF_DIR;

/*
 * 循环缓冲区工作模式，仅单向循环显示或者可双向切换循环显示
 */
typedef enum
{
    RING_MODE_ONEWAY,   // 循环缓冲区单向循环工作模式
    RING_MODE_TWOWAY,   // 循环缓冲区可双向切换循环工作模式
    RING_MODE_MAX
} RING_BUF_MODE;

/*
 * 当刷新数据不足一屏时，刷新数据在屏幕上的位置
 */
typedef enum
{
    RING_REFRESH_NONE,      // 默认使用
    RING_REFRESH_LEFT,      // 刷新数据贴近屏幕左侧
    RING_REFRESH_MIDDLE,    // 刷新数据在屏居中
    RING_REFRESH_RIGHT,     // 刷新数据贴近屏幕右侧
    RING_REFRESH_MAX, 
} RING_BUF_REFRESH_LOC;

/*
 * 存储分段数据和分段信息
 */
typedef struct 
{
    UINT32 u32FrontPartOffset;              // 屏幕前部数据起始偏移，单位：pixel
    UINT32 u32FrontPartLen;                 // 屏幕前部数据长度，单位：pixel
    UINT32 u32LatterPartOffset;             // 屏幕后部数据起始偏移，单位：pixel
    UINT32 u32LatterPartLen;                // 屏幕后部数据长度，单位：pixel
    XIMAGE_DATA_ST stFscRawData;            // 全屏数据存储信息
} SEG_BUF_DATA;

/*****************************************************************************
                            循环缓冲接口
*****************************************************************************/
/**
 * @function   ring_buf_init
 * @brief      创建一个循环缓冲
 * @param[in]  UINT32         s32BufWidth     总体内存宽度，实际用于循环缓冲的宽度为s32BufWidth - (u32MaxCropWidth - u32ScreenW)，循环缓冲至少需要为显示帧宽度的2倍，单位：pixel
 * @param[in]  UINT32         u32BufHeight    内存高度，单位：pixel
 * @param[in]  UINT32         u32ScreenW      显示帧宽度，单位：pixel
 * @param[in]  UINT32         u32MaxCropWidth 支持的最大抠图宽度，u32MaxCropWidth - u32ScreenW为内存尾部保留区域，单位：pixel
 * @param[in]  DSP_IMG_DATFMT enDataFmt       数据类型
 * @param[in]  UINT32         u32DplctdBufNum 实际重复内存的个数，需不大于XAPCK_DATA_CNT_MAX
 * @param[out] NONE
 * @return     ring_buf_handle 循环缓冲的句柄
 */
ring_buf_handle ring_buf_init(UINT32 u32BufWidth, UINT32 u32BufHeight, UINT32 u32ScreenW, UINT32 s32ScreenH, UINT32 u32MaxCropWidth, DSP_IMG_DATFMT enDataFmt, UINT32 u32DplctdBufNum);

/**
 * @function   ring_buf_deinit
 * @brief      销毁循环缓冲，释放资源
 * @param[in]  ring_buf_handle ringBufHandle 循环缓冲句柄
 * @param[out] NONE
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS ring_buf_deinit(ring_buf_handle ringBufHandle);

/**
 * @function   ring_buf_clear
 * @brief      初始化缓冲颜色为指定颜色
 * @param[in]  ring_buf_handle ringBufHandle 循环缓冲句柄
 * @param[in]  UINT32          u32BgValue    填充颜色值
 * @param[out] NONE
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
void *ring_buf_clear(ring_buf_handle ringBufHandle, UINT32 u32BgValue);

/**
 * @function   ring_buf_reset
 * @brief      循环缓冲方向参数复位，初始化缓冲颜色为指定颜色
 * @param[in]  ring_buf_handle ringBufHandle 循环缓冲句柄 
 * @param[in]  RING_BUF_DIR    enRingBufDir  循环方向，为RING_DIR_NONE时不改变方向
 * @param[in]  UINT32          u32BgValue    填充颜色值
 * @param[out] NONE
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS ring_buf_reset(ring_buf_handle ringBufHandle, RING_BUF_DIR enRingBufDir, RING_BUF_MODE enBufMode, UINT32 u32BgValue);

/**
 * @function   ring_buf_put
 * @brief      向循环缓冲中添加数据
 * @param[IN]  ring_buf_handle ringBufHandle    循环缓冲句柄
 * @param[IN]  XIMAGE_DATA_ST  *pstImageDataSrc 数据源
 * @param[IN]  RING_BUF_DIR    enRingBufDir     循环方向
 * @param[OUT] NONE
 * @return     UINT64 将循环缓冲区展开成平面后的数据距离起点偏移量（单位：像素），-1时添加数据失败
 */
UINT64 ring_buf_put(ring_buf_handle ringBufHandle, XIMAGE_DATA_ST *pstImageDataSrc, RING_BUF_DIR enRingBufDir);

/**
 * @function   ring_buf_update
 * @brief      更新循环缓冲中当前显示的数据，支持两种方式指定更新位置。
 *             1.通过u64WriteLoc直接指定写位置，优先级最高。
 *             2.通过enRefreshLoc指定数据更新模式，默认将数据更新到当前循环缓冲最新位置，
 *               当更新数据宽度小于屏幕宽度时，可以在屏幕居左/中/右位置更新。
 * @param[IN]  ring_buf_handle      ringBufHandle   循环缓冲句柄
 * @param[IN]  XIMAGE_DATA_ST      *pstUpdateImgSrc 更新数据源
 * @param[in]  UINT64               u64UpdateLoc    在指定的写位置更新数据，当和enRefreshLoc同时指定时，以u64WriteLoc为准，-1时无效
 * @param[in]  RING_BUF_REFRESH_LOC enRefreshLoc    图像刷新时刷新位置，更新数据不足一屏时指定在屏幕居左/中/右更新，与u64WriteLoc不可同时使用
 * @param[in]  UINT32               u32TopOffset    写入数据在内存高度方向的偏移
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS ring_buf_update(ring_buf_handle ringBufHandle, XIMAGE_DATA_ST *pstUpdateImgSrc, UINT64 u64UpdateLoc, RING_BUF_REFRESH_LOC enRefreshLoc, UINT32 u32TopOffset);

/**
 * @function   ring_buf_get
 * @brief      按顺序获取循环缓冲中的数据，每此获取后若缓存不为0则将读取位置向后偏移u32RefreshCol个像素
 * @param[IN]  ring_buf_handle ringBufHandle  循环缓冲句柄
 * @param[IN]  UINT32          u32RefreshCol  显示更新的列数，不能超过屏幕宽度，为0时屏幕不更新数据，获取当前显示数据
 * @param[OUT] XIMAGE_DATA_ST  *pstOutImage[] 获取的全屏图像，指针数组
 * @param[IN]  INT32           s32ArrayLen    参数3指针数组的成员个数，即获取的全屏图像的重复平面的个数
 * @return     UINT32 获取的帧数据在内存中的偏移量，单位：pixel
 */
UINT32 ring_buf_get(ring_buf_handle ringBufHandle, UINT32 u32RefreshCol, UINT32 u32TopOffset, XIMAGE_DATA_ST *pstOutImage[], INT32 s32ArrayLen);

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
 * @param[OUT]          pPlaneVir   图像数据，为空时不进行数据拷贝，仅拷贝图像信息
 * @param[OUT]          stMbAttr    内存信息
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS ring_buf_crop(ring_buf_handle ringBufHandle, UINT64 u64ReadLoc, UINT32 u32ReadOffset, SAL_VideoCrop *pstCropPrm, XIMAGE_DATA_ST *pstOutImage);

/**
 * @function   ring_buf_size
 * @brief      获取循环缓冲中待显示的数据量
 *  
 * @param[in]  ring_buf_handle ringBufHandle 循环缓冲句柄
 * @param[out] UINT64 *pu64WriteLoc 当前循环缓冲区的写位置，可为NULL，非NULL时才输出
 * @param[out] UINT64 *pu64ReadLoc  当前循环缓冲区的读位置，可为NULL，非NULL时才输出
 * @param[out] UINT64 *pu64DispLocS 当前显示的读起始位置，可为NULL，非NULL时才输出
 * @param[out] UINT64 *pu64DispLocE 当前显示的读结束位置，可为NULL，非NULL时才输出
 *  
 * @return     INT32 数据量，单位：pixel，-1表示出错
 */
INT32 ring_buf_size(ring_buf_handle ringBufHandle, UINT64 *pu64WriteLoc, UINT64 *pu64ReadLoc, UINT64 *pu64DispLocS, UINT64 *pu64DispLocE);

/**
 * @function   ring_buf_status
 * @brief      打印循环缓冲区信息
 * @param[in]  ring_buf_handle ringBufHandle 循环缓冲句柄
 * @return     INT32 数据量，单位：pixel，-1表示出错
 */
SAL_STATUS ring_buf_status(ring_buf_handle ringBufHandle);


/*****************************************************************************
                            分段缓冲接口
*****************************************************************************/
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
segment_buf_handle segment_buf_init(UINT32 u32BufWidth, UINT32 buffLen, UINT32 u32ScreenLen, DSP_IMG_DATFMT enDataFmt);

/**
 * @function   segment_buf_deinit
 * @brief      销毁分段存储缓冲，释放资源
 * @param[in]  segment_buf_handle pSegmentBufHandle 循环缓冲句柄
 * @param[out] NONE
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS segment_buf_deinit(segment_buf_handle pSegmentBufHandle);

/**
 * @function   segment_buf_reset
 * @brief      分段存储缓冲参数复位
 * @param[in]  segment_buf_handle pSegmentBufHandle 分段存储缓冲句柄 
 * @param[out] NONE
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS segment_buf_reset(segment_buf_handle segmentBufHandle);

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
SAL_STATUS segment_buf_put(segment_buf_handle segmentBufHandle, XIMAGE_DATA_ST *pstSegmentDataIn, RING_BUF_DIR enInDir, UINT32 *pu32RawDataLoc);

/**
 * @function   segment_buf_update
 * @brief      将另一个分段缓冲信息更新到本分段缓冲中
 * @param[in]  segment_buf_handle  pSegmentBufHandle 需要更新的分段缓冲句柄
 * @param[in]  segment_buf_handle  pSegSrcBufHandle  信息来源分段缓冲句柄
 * @param[out] None
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS segment_buf_update(segment_buf_handle pSegmentBufHandle, segment_buf_handle pSegSrcBufHandle);

/**
 * @function        segment_buf_get
 * @brief           从分段存储缓冲中获取当前显示部分的raw数据，默认获取一屏范围的数据
 * @param[IN]       segment_buf_handle pSegmentBufHandle 分段缓冲句柄
 * @param[IN]       SEG_BUF_DATA       *pstRawBuf        获取的分段缓冲存储数据信息
 * @param[IN/OUT]:  UINT32             *pu32Length       获取的分段缓冲数据高度，非空时传入期望获取的高度，返回实际获取的高度，该值介于屏幕宽度和分段缓存长度之间
 * @param[OUT]      None
 * @return          SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS segment_buf_get(segment_buf_handle pSegmentBufHandle, SEG_BUF_DATA *pstRawBuf, UINT32 *pu32Length);

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
SAL_STATUS segment_buf_crop(segment_buf_handle pSegmentBufHandle, UINT32 u32CropLoc, UINT32 u32CropOffset, UINT32 u32CropLen, XIMAGE_DATA_ST *pstOutImage);

#endif