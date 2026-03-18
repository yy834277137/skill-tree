/***
 * @file   stream_bits_info_def.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  码流信息的定义
 * @author liwenbin
 * @date   2022-02-24
 * @note
 * @note History:
 */

#ifndef _STREAM_BITS_INFO_DEF_H_
#define _STREAM_BITS_INFO_DEF_H_

#include "sal.h"


#define MAX_NALU_PER_STREAM_FRAMR	(16)
#define MAX_STREAM_FRAMR_CNT		(1)

#define MAX_FPS_P_STANDARD	(25)
#define MAX_FPS_N_STANDARD	(30)


typedef enum eH264FrameTypeE
{
    STREAM_TYPE_UNDEFINE = 0,              /* 未定义的帧类型 */
    STREAM_TYPE_H264_IFRAME,           /* 视频数据 I 帧 */
    STREAM_TYPE_H264_PFRAME,           /* 视频数据 P 帧 */
    STREAM_TYPE_H265_IFRAME,           /* 视频数据 I 帧 */
    STREAM_TYPE_H265_PFRAME,           /* 视频数据 P 帧 */
    STREAM_TYPE_MAX
} STREAM_FRAME_TYPE_E;

/* 码流 NALU信息 */
typedef struct tagStreamNaluInfoSt
{
    UINT8 *pucNaluPtr;
    UINT32 uiNaluLen;
    UINT64 u64PTS;            /* 该帧的 pts */
} STREAM_NALU_INFO_ST;

/* 一帧码流 NALU 信息 */
typedef struct tagStreamFrameInfoSt
{
    UINT32 uiValid;
    UINT32 uiFrameWidth;
    UINT32 uiFrameHeight;
    UINT32 uiFps;
    STREAM_FRAME_TYPE_E eFrameType;
    UINT32 uiNaluNum;
    STREAM_NALU_INFO_ST stNaluInfo[MAX_NALU_PER_STREAM_FRAMR];
    UINT32 platPrivLen;
    void *pvPlatPriv;               /* 平台私有信息 */
} STREAM_FRAME_INFO_ST;

/* 码流包 */
typedef struct tagSystemBitsDataInfoSt
{
    UINT32 uiFrameCnt;                                      /* 获取到的视频帧个数 */
    STREAM_FRAME_INFO_ST stStreamFrameInfo[MAX_STREAM_FRAMR_CNT]; /* 获取到的视频帧信息 */
} SYSTEM_BITS_DATA_INFO_ST;

/* 视频的矩阵属性: 旋转、镜像、分辨率、帧率属性 */
typedef struct tagVideoMatrixInfo
{
    UINT32 width;
    UINT32 height;
    UINT32 rotate;
    UINT32 mirror;
} VIDEO_MATRIX_INFO;


#endif


