
/*******************************************************************************
 * video_inout_base.h
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : cuifeng5
 * Version: V1.0.0  2021年6月15日 Create
 *
 * Description : 
 * Modification: 
 *******************************************************************************/

#ifndef __VIDEO_INOUT_BASE_H_
#define __VIDEO_INOUT_BASE_H_

/* 输入输出接口 */
typedef enum
{
    VIDEO_CABLE_VGA = 0,
    VIDEO_CABLE_HDMI,
    VIDEO_CABLE_DVI,

    VIDEO_CABLE_BUTT,
    VIDEO_CABLE_NONE = 0xFF,
} VIDEO_CABLE_E;


/* 视频偏移方向 */
typedef enum
{
    VIDEO_OFFSET_AUTO = 0,
    VIDEO_OFFSET_LEFT,
    VIDEO_OFFSET_RIGHT,
    VIDEO_OFFSET_UP,
    VIDEO_OFFSET_DOWN,

    VIDEO_OFFSET_BUTT,
} VIDEO_OFFSET_E;


/* csc参数 */
typedef enum
{
    VIDEO_CSC_LUMA = 0,
    VIDEO_CSC_CONTRAST,
    VIDEO_CSC_HUE,
    VIDEO_CSC_SATUATURE,

    VIDEO_CSC_BUTT,
} VIDEO_CSC_E;


/* 分辨率 */
typedef struct
{
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Fps;
} VIDEO_RES_S;


/* 输出状态 */
typedef struct
{
    VIDEO_CABLE_E enCable;
    VIDEO_RES_S stRes;
} VIDEO_IO_STATUS_S;


/* 时序 */
typedef struct
{
    VIDEO_CABLE_E enCable;
    BT_TIMING_S stTiming;
} VIDEO_TIMING_S;


/* 视频偏移 */
typedef struct
{
    VIDEO_OFFSET_E enDir;
    UINT32 u32Pixel;
} VIDEO_OFFSET_S;


/* csc参数 */
typedef struct
{
    VIDEO_CSC_E enCsc;
    UINT32 u32Value;
} VIDEO_CSC_PARAM_S;


/* EDID缓存 */
typedef struct
{
    VIDEO_CABLE_E enCable;
    UINT32 u32Len;
    UINT8 au8Buff[256];
} VIDEO_EDID_BUFF_S;


/* 对应接口的视频参数 */
typedef struct
{
    VIDEO_CABLE_E enCable;
    VOID *pvArg;
    UINT32 u32Len; 
} VIDEO_CABLE_PARAM_S;

#endif /* __VIDEO_INOUT_BASE_H_ */
