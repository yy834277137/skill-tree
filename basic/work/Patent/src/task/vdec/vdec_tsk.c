/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   FileName: vdec_tsk.c
   Description:
   Author:
   Date:
   Modification History:
******************************************************************************/
#include "system_common_api.h"
#include "vdec_tsk_api.h"
#include "libdemux.h"
#include <platform_hal.h>
#include "audio_tsk_api.h"
/* #include "hal_inc_exter/sys_hal.h" */
#include "dup_tsk_api.h"
#include "sal_mem_new.h"
#include "color_space.h"
#include "bmp_info.h"
#include "system_prm_api.h"
#include "vca_unpack_api.h"
#include "jpgd_lib.h"
#include "plat_com_inter_hal.h"
#include "../vca_header/hka_types.h"
#include "ia_common.h"


#define MAX_PLAYGROUP_FRAME_NUM 150
#define PB_VDEC_CHAN_CNT_MAX    9 /* 解码倒放所用的解码通道个数 */
#define PB_VDEC_CHAN_BASE_NUM   (MX_JPEG_VDEC_CHAN + MAX_VDEC_CHAN) /* 解码倒放所用的解码通通道号基值 */
#define VDEC_ABS(x) ((x) >= 0 ? (x) : (-(x)))
#define VDEC_JPG_W_MAX  1920
#define VDEC_JPG_H_MAX  1920

#line __LINE__ "vdec_tsk.c"

/* 解码器控制 */
typedef struct
{
    UINT32 baseTime;
    UINT64 checkTime;           /* 上一帧输出图像系统时间 */
    UINT32 bUpdataDecTime;      /*更新解码基准时间*/
    UINT32 u32PlaySpeed;        /*播放速度*/
    UINT32 vdecMode;            /*解码模式*/
    UINT32 decodeAbsTime;         /* 解码器绝对时间 ，  暂定1、正方按照系统时间低32位为基准往上增加一次加1*/
    UINT32 decodeStandardTime;    /* 解码器相对时间 ps包里面的时间戳*/
    UINT32 decodePlayBackStdTime; /* 解码器倒放相对时间倒放 正方按照系统时间低32位为基准往上递减一次减1 */
    UINT32 lastFrmTimeTru;        /* 当前解码通道的上一帧实际时间戳 */
    UINT32 curFrmTimeTru;         /* 当前解码通道的当前帧实际时间戳 */
    UINT32 timestampLast;         /* 当前解码通道的上一帧时间戳 */
    UINT32 timestampFirst;        /* 当前解码通道的第一帧时间戳 */
    UINT32 isNewDec;              /* 刚开始解码的第一帧 */
    UINT32 isCopyOver;            /* 暂停解码拷贝动作是否完成 */
    UINT32 frameNum;              /* ps包中的帧序号                */
    UINT32 frameFps;            /*帧率 */
    UINT32 milliSecond;           /* 记录帧毫秒值                   */
    UINT32 syncMasterAbsTimeMs;   /* 主通道绝对毫秒时间 */
    UINT64 syncMGlg;            /*主同步当前系统时间*/
    UINT32 syncMGlg_ms;         /*主同步当前系统时间中的ms*/
    UINT64 glbTime;             /*获取系统时间*/
    INT32 adjustTime;           /*自适应时间矫正*/
    UINT32 skipVdec;            /*当前帧是否不解码*/
    UINT32 bigDiff;             /*同步主同步和子同步是否播放时间上差距较大*/
    UINT32 bFastestMode;        /*是否使用快速同步主同步*/
    UINT32 cntDebug;            /*调试打印*/
    UINT32 u32VdecPrmReset;     /*解码复位*/
    UINT32 unReleaseFrameCnt;   /*外部模块未释放帧个数*/
    DISP_WINDOW_PRM dWPrm;      /*当前解码通道绑定的显示窗口*/
    void *mutex;                /*锁*/
    DEM_GLOBAL_TIME glb_time;     /* 全局时间                               */
    DECODER_SYNC_PRM syncPrm;     /* 解码同步参数                   */
} VDEC_TASK_CTL;

typedef struct _DEC_MOD_CTRL_
{
    /*解码模块起停控制*/
    UINT32 enable;
    void *mutexHdl;
    
    /* 解码状态互斥锁 */
    void *pChnStatMuxHdl;

    /*解包模块控制句柄*/
    INT32 rtpHandle;          /*rtp解包handle*/
    INT32 psHandle;           /*ps解包handle*/
    DupHandle frontDupHandle;      /*前级dup，有些平台比如sd3403需要做放大处理*/
    DupHandle rearDupHandle;       /*后级dup，获取帧handle*/
    DUP_ChanHandle *dupChanHandle[NODE_NUM_MAX];
    INT32 chnHandle;          /*解包通道handle*/
    VDEC_STATUS chnStatus;             /*解码状态，见VDEC_STATUS*/
    SYSTEM_FRAME_INFO stFrameInfo; /*用于暂停使用*/
    DECODER_PIC_PRM chnPicPrm;
    NODE_BIND_TYPE_E  dupBindType;
} DEC_MOD_CTRL;

/*此路待解码码流的属性*/
typedef struct
{
    INT32 packType;       /* 封装方式, RTP  PS  TS*/
    INT32 u32EncType;        /* 编码方式， H264 , MJPEG, MPEG4 */
    INT32 maxWidth;       /* 码流最大宽*/
    INT32 maxHeight;      /* 码流最大高*/

    UINT8 *decBufAddr;
    UINT32 decBufLen;
    UINT32 audPlay;    /* 音频播放开关 */
} DEC_CHN_PRM;

/*解码通道控制*/
typedef struct tagVdecTskChn
{
    UINT32 u32VdecChn;
    DEC_CHN_PRM vdec_chn;
    DEC_MOD_CTRL decModCtrl;
} VDEC_TSK_CHN;
/*解复用通道控制*/
typedef struct tagVdecObjPrm
{
    UINT32 uiChnCnt;
    VDEC_TSK_CHN stObjTsk[MAX_VDEC_CHAN];
} VDEC_OBJ_COMMON;


/* 暂停解码参数*/
typedef struct
{
    UINT32 w;
    UINT32 h;
    UINT32 u32NeedDrawOnce;
    SYSTEM_FRAME_INFO stFInfoForDisp;
    SYSTEM_FRAME_INFO stFInfoForJpeg;
    HAL_MEM_PRM addrMem[2];
} VDEC_PAUSE_BUFF;

struct DEC_Stack /*声明栈，其中head相当于链表头结点，top链表的高度*/
{
    struct DEC_List *head;
    struct DEC_List *playFrame;
    INT32 top;
    UINT32 statue; /* 0表示可以入栈不能出栈，1表示不可以入栈只能出栈 */
    UINT32 w;
    UINT32 h;
};

struct DEC_List /*声明链表的节点*/
{
    SYSTEM_FRAME_INFO stFrame;
    struct DEC_List *next;
};

/*倒放缓存*/
typedef struct DEC_DRV_MEM
{
    HAL_MEM_PRM addrPrm[MAX_PLAYGROUP_FRAME_NUM * 2];
    SYSTEM_FRAME_INFO playBackMemInfo[MAX_PLAYGROUP_FRAME_NUM * 2];
    UINT32 memCnt;
} DEC_DRV_MEM_MANAGE;

short R_Y_1[] =
{
    -18, -17, -16, -15, -13, -12, -11, -10, -9, -8, -6, -5, -4, -3,
    -2, -1, 0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 15, 16, 17,
    18, 19, 20, 22, 23, 24, 25, 26, 27, 29, 30, 31, 32, 33, 34, 36,
    37, 38, 39, 40, 41, 43, 44, 45, 46, 47, 48, 50, 51, 52, 53, 54,
    55, 57, 58, 59, 60, 61, 62, 64, 65, 66, 67, 68, 69, 71, 72, 73,
    74, 75, 76, 77, 79, 80, 81, 82, 83, 84, 86, 87, 88, 89, 90, 91,
    93, 94, 95, 96, 97, 98, 100, 101, 102, 103, 104, 105, 107, 108,
    109, 110, 111, 112, 114, 115, 116, 117, 118, 119, 121, 122, 123,
    124, 125, 126, 128, 129, 130, 131, 132, 133, 135, 136, 137, 138,
    139, 140, 142, 143, 144, 145, 146, 147, 148, 150, 151, 152, 153,
    154, 155, 157, 158, 159, 160, 161, 162, 164, 165, 166, 167, 168,
    169, 171, 172, 173, 174, 175, 176, 178, 179, 180, 181, 182, 183,
    185, 186, 187, 188, 189, 190, 192, 193, 194, 195, 196, 197, 199,
    200, 201, 202, 203, 204, 206, 207, 208, 209, 210, 211, 213, 214,
    215, 216, 217, 218, 219, 221, 222, 223, 224, 225, 226, 228, 229,
    230, 231, 232, 233, 235, 236, 237, 238, 239, 240, 242, 243, 244,
    245, 246, 247, 249, 250, 251, 252, 253, 254, 256, 257, 258, 259,
    260, 261, 263, 264, 265, 266, 267, 268, 270, 271, 272, 273, 274,
    275, 277, 278
};

short R_Cr[] =
{
    -204, -202, -201, -199, -197, -196, -194, -193, -191, -189, -188,
    -186, -185, -183, -181, -180, -178, -177, -175, -173, -172, -170,
    -169, -167, -165, -164, -162, -161, -159, -158, -156, -154, -153,
    -151, -150, -148, -146, -145, -143, -142, -140, -138, -137, -135,
    -134, -132, -130, -129, -127, -126, -124, -122, -121, -119, -118,
    -116, -114, -113, -111, -110, -108, -106, -105, -103, -102, -100,
    -98, -97, -95, -94, -92, -90, -89, -87, -86, -84, -82, -81, -79,
    -78, -76, -75, -73, -71, -70, -68, -67, -65, -63, -62, -60, -59,
    -57, -55, -54, -52, -51, -49, -47, -46, -44, -43, -41, -39, -38,
    -36, -35, -33, -31, -30, -28, -27, -25, -23, -22, -20, -19, -17,
    -15, -14, -12, -11, -9, -7, -6, -4, -3, -1, 0, 1, 3, 4, 6, 7, 9,
    11, 12, 14, 15, 17, 19, 20, 22, 23, 25, 27, 28, 30, 31, 33, 35,
    36, 38, 39, 41, 43, 44, 46, 47, 49, 51, 52, 54, 55, 57, 59, 60,
    62, 63, 65, 67, 68, 70, 71, 73, 75, 76, 78, 79, 81, 82, 84, 86,
    87, 89, 90, 92, 94, 95, 97, 98, 100, 102, 103, 105, 106, 108, 110,
    111, 113, 114, 116, 118, 119, 121, 122, 124, 126, 127, 129, 130,
    132, 134, 135, 137, 138, 140, 142, 143, 145, 146, 148, 150, 151,
    153, 154, 156, 158, 159, 161, 162, 164, 165, 167, 169, 170, 172,
    173, 175, 177, 178, 180, 181, 183, 185, 186, 188, 189, 191, 193,
    194, 196, 197, 199, 201, 202
};

short G_Cr[] =
{
    -104, -103, -102, -101, -100, -99, -99, -98, -97, -96, -95, -95,
    -94, -93, -92, -91, -91, -90, -89, -88, -87, -86, -86, -85, -84, -83,
    -82, -82, -81, -80, -79, -78, -78, -77, -76, -75, -74, -73, -73, -72,
    -71, -70, -69, -69, -68, -67, -66, -65, -65, -64, -63, -62, -61, -60,
    -60, -59, -58, -57, -56, -56, -55, -54, -53, -52, -52, -51, -50, -49,
    -48, -47, -47, -46, -45, -44, -43, -43, -42, -41, -40, -39, -39, -38,
    -37, -36, -35, -34, -34, -33, -32, -31, -30, -30, -29, -28, -27, -26,
    -26, -25, -24, -23, -22, -21, -21, -20, -19, -18, -17, -17, -16, -15,
    -14, -13, -13, -12, -11, -10, -9, -8, -8, -7, -6, -5, -4, -4, -3, -2,
    -1, 0, 0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 8, 9, 10, 11, 12, 13, 13, 14,
    15, 16, 17, 17, 18, 19, 20, 21, 21, 22, 23, 24, 25, 26, 26, 27, 28, 29,
    30, 30, 31, 32, 33, 34, 34, 35, 36, 37, 38, 39, 39, 40, 41, 42, 43, 43,
    44, 45, 46, 47, 47, 48, 49, 50, 51, 52, 52, 53, 54, 55, 56, 56, 57, 58,
    59, 60, 60, 61, 62, 63, 64, 65, 65, 66, 67, 68, 69, 69, 70, 71, 72, 73,
    73, 74, 75, 76, 77, 78, 78, 79, 80, 81, 82, 82, 83, 84, 85, 86, 86, 87,
    88, 89, 90, 91, 91, 92, 93, 94, 95, 95, 96, 97, 98, 99, 99, 100, 101,
    102, 103
};

short G_Cb[] =
{
    -50, -49, -49, -48, -48, -48, -47, -47, -46, -46, -46, -45, -45, -44,
    -44, -44, -43, -43, -43, -42, -42, -41, -41, -41, -40, -40, -39, -39,
    -39, -38, -38, -37, -37, -37, -36, -36, -35, -35, -35, -34, -34, -34,
    -33, -33, -32, -32, -32, -31, -31, -30, -30, -30, -29, -29, -28, -28,
    -28, -27, -27, -26, -26, -26, -25, -25, -25, -24, -24, -23, -23, -23,
    -22, -22, -21, -21, -21, -20, -20, -19, -19, -19, -18, -18, -17, -17,
    -17, -16, -16, -16, -15, -15, -14, -14, -14, -13, -13, -12, -12, -12,
    -11, -11, -10, -10, -10, -9, -9, -8, -8, -8, -7, -7, -7, -6, -6, -5,
    -5, -5, -4, -4, -3, -3, -3, -2, -2, -1, -1, -1, 0, 0, 0, 0, 0, 1, 1,
    1, 2, 2, 3, 3, 3, 4, 4, 5, 5, 5, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 10, 10,
    10, 11, 11, 12, 12, 12, 13, 13, 14, 14, 14, 15, 15, 16, 16, 16, 17, 17,
    17, 18, 18, 19, 19, 19, 20, 20, 21, 21, 21, 22, 22, 23, 23, 23, 24, 24,
    25, 25, 25, 26, 26, 26, 27, 27, 28, 28, 28, 29, 29, 30, 30, 30, 31, 31,
    32, 32, 32, 33, 33, 34, 34, 34, 35, 35, 35, 36, 36, 37, 37, 37, 38, 38,
    39, 39, 39, 40, 40, 41, 41, 41, 42, 42, 43, 43, 43, 44, 44, 44, 45, 45,
    46, 46, 46, 47, 47, 48, 48, 48, 49, 49
};

short B_Cb[] =
{
    -258, -256, -254, -252, -250, -248, -246, -244, -242, -240, -238, -236,
    -234, -232, -230, -228, -226, -223, -221, -219, -217, -215, -213, -211,
    -209, -207, -205, -203, -201, -199, -197, -195, -193, -191, -189, -187,
    -185, -183, -181, -179, -177, -175, -173, -171, -169, -167, -165, -163,
    -161, -159, -157, -155, -153, -151, -149, -147, -145, -143, -141, -139,
    -137, -135, -133, -131, -129, -127, -125, -123, -121, -119, -117, -115,
    -113, -110, -108, -106, -104, -102, -100, -98, -96, -94, -92, -90, -88,
    -86, -84, -82, -80, -78, -76, -74, -72, -70, -68, -66, -64, -62, -60,
    -58, -56, -54, -52, -50, -48, -46, -44, -42, -40, -38, -36, -34, -32,
    -30, -28, -26, -24, -22, -20, -18, -16, -14, -12, -10, -8, -6, -4, -2,
    0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36,
    38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72,
    74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106,
    108, 110, 113, 115, 117, 119, 121, 123, 125, 127, 129, 131, 133, 135, 137,
    139, 141, 143, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165, 167,
    169, 171, 173, 175, 177, 179, 181, 183, 185, 187, 189, 191, 193, 195, 197,
    199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221, 223, 226, 228,
    230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 254, 256
};

short redAdjust[] = {
    -161, -160, -159, -158, -157, -156, -155, -153,
    -152, -151, -150, -149, -148, -147, -145, -144,
    -143, -142, -141, -140, -139, -137, -136, -135,
    -134, -133, -132, -131, -129, -128, -127, -126,
    -125, -124, -123, -122, -120, -119, -118, -117,
    -116, -115, -114, -112, -111, -110, -109, -108,
    -107, -106, -104, -103, -102, -101, -100, -99,
    -98, -96, -95, -94, -93, -92, -91, -90,
    -88, -87, -86, -85, -84, -83, -82, -80,
    -79, -78, -77, -76, -75, -74, -72, -71,
    -70, -69, -68, -67, -66, -65, -63, -62,
    -61, -60, -59, -58, -57, -55, -54, -53,
    -52, -51, -50, -49, -47, -46, -45, -44,
    -43, -42, -41, -39, -38, -37, -36, -35,
    -34, -33, -31, -30, -29, -28, -27, -26,
    -25, -23, -22, -21, -20, -19, -18, -17,
    -16, -14, -13, -12, -11, -10, -9, -8,
    -6, -5, -4, -3, -2, -1, 0, 1,
    2, 3, 4, 5, 6, 7, 9, 10,
    11, 12, 13, 14, 15, 17, 18, 19,
    20, 21, 22, 23, 25, 26, 27, 28,
    29, 30, 31, 33, 34, 35, 36, 37,
    38, 39, 40, 42, 43, 44, 45, 46,
    47, 48, 50, 51, 52, 53, 54, 55,
    56, 58, 59, 60, 61, 62, 63, 64,
    66, 67, 68, 69, 70, 71, 72, 74,
    75, 76, 77, 78, 79, 80, 82, 83,
    84, 85, 86, 87, 88, 90, 91, 92,
    93, 94, 95, 96, 97, 99, 100, 101,
    102, 103, 104, 105, 107, 108, 109, 110,
    111, 112, 113, 115, 116, 117, 118, 119,
    120, 121, 123, 124, 125, 126, 127, 128,
};

short greenAdjust1[] = {
    34, 34, 33, 33, 32, 32, 32, 31,
    31, 30, 30, 30, 29, 29, 28, 28,
    28, 27, 27, 27, 26, 26, 25, 25,
    25, 24, 24, 23, 23, 23, 22, 22,
    21, 21, 21, 20, 20, 19, 19, 19,
    18, 18, 17, 17, 17, 16, 16, 15,
    15, 15, 14, 14, 13, 13, 13, 12,
    12, 12, 11, 11, 10, 10, 10, 9,
    9, 8, 8, 8, 7, 7, 6, 6,
    6, 5, 5, 4, 4, 4, 3, 3,
    2, 2, 2, 1, 1, 0, 0, 0,
    0, 0, -1, -1, -1, -2, -2, -2,
    -3, -3, -4, -4, -4, -5, -5, -6,
    -6, -6, -7, -7, -8, -8, -8, -9,
    -9, -10, -10, -10, -11, -11, -12, -12,
    -12, -13, -13, -14, -14, -14, -15, -15,
    -16, -16, -16, -17, -17, -17, -18, -18,
    -19, -19, -19, -20, -20, -21, -21, -21,
    -22, -22, -23, -23, -23, -24, -24, -25,
    -25, -25, -26, -26, -27, -27, -27, -28,
    -28, -29, -29, -29, -30, -30, -30, -31,
    -31, -32, -32, -32, -33, -33, -34, -34,
    -34, -35, -35, -36, -36, -36, -37, -37,
    -38, -38, -38, -39, -39, -40, -40, -40,
    -41, -41, -42, -42, -42, -43, -43, -44,
    -44, -44, -45, -45, -45, -46, -46, -47,
    -47, -47, -48, -48, -49, -49, -49, -50,
    -50, -51, -51, -51, -52, -52, -53, -53,
    -53, -54, -54, -55, -55, -55, -56, -56,
    -57, -57, -57, -58, -58, -59, -59, -59,
    -60, -60, -60, -61, -61, -62, -62, -62,
    -63, -63, -64, -64, -64, -65, -65, -66,
};

short greenAdjust2[] = {
    74, 73, 73, 72, 71, 71, 70, 70,
    69, 69, 68, 67, 67, 66, 66, 65,
    65, 64, 63, 63, 62, 62, 61, 60,
    60, 59, 59, 58, 58, 57, 56, 56,
    55, 55, 54, 53, 53, 52, 52, 51,
    51, 50, 49, 49, 48, 48, 47, 47,
    46, 45, 45, 44, 44, 43, 42, 42,
    41, 41, 40, 40, 39, 38, 38, 37,
    37, 36, 35, 35, 34, 34, 33, 33,
    32, 31, 31, 30, 30, 29, 29, 28,
    27, 27, 26, 26, 25, 24, 24, 23,
    23, 22, 22, 21, 20, 20, 19, 19,
    18, 17, 17, 16, 16, 15, 15, 14,
    13, 13, 12, 12, 11, 11, 10, 9,
    9, 8, 8, 7, 6, 6, 5, 5,
    4, 4, 3, 2, 2, 1, 1, 0,
    0, 0, -1, -1, -2, -2, -3, -4,
    -4, -5, -5, -6, -6, -7, -8, -8,
    -9, -9, -10, -11, -11, -12, -12, -13,
    -13, -14, -15, -15, -16, -16, -17, -17,
    -18, -19, -19, -20, -20, -21, -22, -22,
    -23, -23, -24, -24, -25, -26, -26, -27,
    -27, -28, -29, -29, -30, -30, -31, -31,
    -32, -33, -33, -34, -34, -35, -35, -36,
    -37, -37, -38, -38, -39, -40, -40, -41,
    -41, -42, -42, -43, -44, -44, -45, -45,
    -46, -47, -47, -48, -48, -49, -49, -50,
    -51, -51, -52, -52, -53, -53, -54, -55,
    -55, -56, -56, -57, -58, -58, -59, -59,
    -60, -60, -61, -62, -62, -63, -63, -64,
    -65, -65, -66, -66, -67, -67, -68, -69,
    -69, -70, -70, -71, -71, -72, -73, -73,
};

short blueAdjust[] = {
    -276, -274, -272, -270, -267, -265, -263, -261,
    -259, -257, -255, -253, -251, -249, -247, -245,
    -243, -241, -239, -237, -235, -233, -231, -229,
    -227, -225, -223, -221, -219, -217, -215, -213,
    -211, -209, -207, -204, -202, -200, -198, -196,
    -194, -192, -190, -188, -186, -184, -182, -180,
    -178, -176, -174, -172, -170, -168, -166, -164,
    -162, -160, -158, -156, -154, -152, -150, -148,
    -146, -144, -141, -139, -137, -135, -133, -131,
    -129, -127, -125, -123, -121, -119, -117, -115,
    -113, -111, -109, -107, -105, -103, -101, -99,
    -97, -95, -93, -91, -89, -87, -85, -83,
    -81, -78, -76, -74, -72, -70, -68, -66,
    -64, -62, -60, -58, -56, -54, -52, -50,
    -48, -46, -44, -42, -40, -38, -36, -34,
    -32, -30, -28, -26, -24, -22, -20, -18,
    -16, -13, -11, -9, -7, -5, -3, -1,
    0, 2, 4, 6, 8, 10, 12, 14,
    16, 18, 20, 22, 24, 26, 28, 30,
    32, 34, 36, 38, 40, 42, 44, 46,
    49, 51, 53, 55, 57, 59, 61, 63,
    65, 67, 69, 71, 73, 75, 77, 79,
    81, 83, 85, 87, 89, 91, 93, 95,
    97, 99, 101, 103, 105, 107, 109, 112,
    114, 116, 118, 120, 122, 124, 126, 128,
    130, 132, 134, 136, 138, 140, 142, 144,
    146, 148, 150, 152, 154, 156, 158, 160,
    162, 164, 166, 168, 170, 172, 175, 177,
    179, 181, 183, 185, 187, 189, 191, 193,
    195, 197, 199, 201, 203, 205, 207, 209,
    211, 213, 215, 217, 219, 221, 223, 225,
    227, 229, 231, 233, 235, 238, 240, 242,
};

#if 0
/* analog trans table */
#define Y(r, g, b)      (0.2990 * r + 0.5870 * g + 0.1140 * b)
#define R_Y(r, g, b)    (0.7010 * r - 0.5870 * g - 0.1140 * b)
#define B_Y(r, g, b)    (-0.2990 * r - 0.5870 * g + 0.8860 * b)

#define U(r, g, b)  (-0.1690 * r - 0.3310 * g + 0.5000 * b)
#define V(r, g, b)  (0.5000 * r - 0.4190 * g - 0.0810 * b)
#else
/* digital trans table */
#define Y(r, g, b)  (77 * r / 256 + 150 * g / 256 + 29 * b / 256)
#define U(r, g, b)  (-44 * r / 256 - 87 * g / 256 + 131 * b / 256 + 128)
#define V(r, g, b)  (131 * r / 256 - 110 * g / 256 - 21 * b / 256 + 128)
#endif

/* vdec 用到的DUP 对应的实例节点配置信息 ，在link_task.c里定义*/
extern INST_CFG_S stVdecDupInstCfg;


static VDEC_OBJ_COMMON g_stVdecObjCommon;
static struct DEC_Stack gPbGroup1[MAX_VDEC_CHAN] = {0};
static struct DEC_Stack gPbGroup2[MAX_VDEC_CHAN] = {0};
static DEC_DRV_MEM_MANAGE playBackMemManage[PB_VDEC_CHAN_CNT_MAX] = {0};
static UINT32 gOutPingPangId[MAX_VDEC_CHAN] = {0};
static UINT32 gInPingPangId[MAX_VDEC_CHAN] = {0};
static BMP_RESULT_ST outBmpInfo[MAX_VDEC_CHAN] = {0};
static VDEC_PAUSE_BUFF stPauseFrameBuff[MAX_VDEC_CHAN] = {0};
static VDEC_PAUSE_BUFF stNoSignalFrameBuff[MAX_VDEC_CHAN] = {0};   /*无视频信号单独结构体保存，不和暂停帧混淆*/

static VDEC_TASK_CTL g_stVecTaskCtl[MAX_VDEC_CHAN] = {0};
static int demDbLevel = DEBUG_LEVEL0;
static VDEC_TSK_JPGTOPMP_PRM gJpgToBmpPrm = {0};

/* 人包关联目前使用的解码通道个数,人脸相机对应解码通道10，三目相机对应解码通道11 */
#define PPM_SUPRT_DEC_CHN_NUM (16)

static UINT8 aacAdtsHeader[16][7] =
{
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc}
};

static INT32 g_AacAdtsHeaderSamplingFrequency[16] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0};

static void *vdec_tsk_DemuxThread(void *prm);
static INT32 vdec_tsk_UpdateVdec(UINT32 u32VdecChn, DEM_BUFFER *pDemBuf);

/**
 * @function   vdec_tsk_MallocOS
 * @brief      申请os内存
 * @param[in]  size 申请内存大小
 * @param[out] None
 * @return
 */
void *vdec_tsk_MallocOS(INT32 size)
{
    void *vir = NULL;

    vir = SAL_memZalloc(size, "VDEC", "mallocOs");

    return vir;
}

/**
 * @function   vdec_tsk_FreeOS
 * @brief      释放os内存
 * @param[in]  vir 释放的地址
 * @param[out] None
 * @return
 */
void vdec_tsk_FreeOS(void *vir)
{
    if (vir == NULL)
    {
        VDEC_LOGE("free vir null err\n");
        return;
    }

    SAL_memfree(vir, "VDEC", "mallocOs");
}

/**
 * @function   vdec_tsk_getTimeMilli
 * @brief      获取系统时间
 * @param[in]
 * @param[out] None
 * @return     static INT32
 */
static UINT64 vdec_tsk_getTimeMilli()
{
    struct timeval tv;
    UINT64 time = 0;

    gettimeofday(&tv, NULL);
    time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    return time;
}

/**
 * @function   vdec_tsk_GetAabsTime
 * @brief      获取系统绝对时间
 * @param[in]  DEM_GLOBAL_TIME *pDT 系统时间参数
 * @param[out] None
 * @return     static INT32
 */
static UINT64 vdec_tsk_GetAabsTime(DEM_GLOBAL_TIME *pDT)
{
    struct tm tmpSysTime = {0};
    struct tm tmpSysTime1 = {0};
    struct tm *p = &tmpSysTime;
    struct tm *date = &tmpSysTime1;
    time_t tp;
    UINT64 sectime = 0;

    p->tm_year = pDT->year - 1900;
    p->tm_mon = pDT->month - 1;
    p->tm_mday = pDT->date;
    p->tm_hour = pDT->hour;
    p->tm_min = pDT->minute;
    p->tm_sec = pDT->second;
    tp = mktime(p);
    gmtime_r(&tp, date);
    /*lint -e{571}*/
    sectime = (UINT64)tp;
    return sectime;
}

/**
 * @function   vdec_tsk_SetDecTimeAdjust
 * @brief      设置解码自适应睡眠时间
 * @param[in]  s32val     同步时间差距
 * @param[in]  s32lower 比较的最小值
 * @param[in]  u32base  基准
 * @param[in]  s32upper 比较的最大值
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_SetDecTimeAdjust(INT32 s32val, INT32 s32lower, UINT32 u32base, INT32 s32upper)
{
    INT32 s32adjust = 0;

    if (s32val > s32upper) /* 快放 */
    {
        s32adjust = (s32upper - s32val); /*慢放*/
    }
    else if (s32val < s32lower) /* 慢放 */
    {
        s32adjust = (s32lower - s32val);  /*快放*/
    }

    return s32adjust;
}

/**
 * @function   vdec_tsk_InitStack
 * @brief    设置倒放栈
 * @param[in]  DEC_Stack *pPlayGroup1 帧缓存1
 * @param[in]  DEC_Stack *pPlayGroup2 帧缓存2
 * @param[out] None
 * @return     static INT32
 */
static INT32 vdec_tsk_InitStack(struct DEC_Stack *pPlayGroup1, struct DEC_Stack *pPlayGroup2)
{
    CHECK_PTR_NULL(pPlayGroup1, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pPlayGroup2, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    if (NULL == pPlayGroup1->head)
    {
        pPlayGroup1->head = (struct DEC_List *)vdec_tsk_MallocOS(sizeof(struct DEC_List));
        if (pPlayGroup1->head == NULL)
        {
            VDEC_LOGE("DEC_List malloc fail\n");
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_MEM_ALLOC);
        }
    }

    memset(pPlayGroup1->head, 0x0, sizeof(struct DEC_List));
    pPlayGroup1->top = 0;
    pPlayGroup1->statue = 0;

    if (NULL == pPlayGroup2->head)
    {
        pPlayGroup2->head = (struct DEC_List *)vdec_tsk_MallocOS(sizeof(struct DEC_List));
        if (pPlayGroup2->head == NULL)
        {
            VDEC_LOGE("DEC_List malloc fail\n");
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_MEM_ALLOC);
        }
    }

    memset(pPlayGroup2->head, 0x0, sizeof(struct DEC_List));
    pPlayGroup2->top = 0;
    pPlayGroup2->statue = 0;
    if (NULL == pPlayGroup1->playFrame)
    {
        pPlayGroup1->playFrame = (struct DEC_List *)vdec_tsk_MallocOS(sizeof(struct DEC_List) * MAX_PLAYGROUP_FRAME_NUM);
    }

    if (pPlayGroup1->playFrame != NULL)
    {
        memset(pPlayGroup1->playFrame, 0x0, sizeof(struct DEC_List) * MAX_PLAYGROUP_FRAME_NUM);
    }

    if (NULL == pPlayGroup2->playFrame)
    {
        pPlayGroup2->playFrame = (struct DEC_List *)vdec_tsk_MallocOS(sizeof(struct DEC_List) * MAX_PLAYGROUP_FRAME_NUM);
    }

    if (pPlayGroup2->playFrame != NULL)
    {
        memset(pPlayGroup2->playFrame, 0x0, sizeof(struct DEC_List) * MAX_PLAYGROUP_FRAME_NUM);
    }

    return SAL_SOK;
}

/**
 * @function   vdec_tsk_DeInitStack
 * @brief    去初始化回放栈
 * @param[in]  DEC_Stack *pPlayGroup1 帧缓存1
 * @param[in]  DEC_Stack *pPlayGroup2 帧缓存2
 * @param[out] None
 * @return     static INT32
 */
static INT32 vdec_tsk_DeInitStack(struct DEC_Stack *pPlayGroup1, struct DEC_Stack *pPlayGroup2)
{
    CHECK_PTR_NULL(pPlayGroup1, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pPlayGroup2, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    if (pPlayGroup1->head != NULL)
    {
        vdec_tsk_FreeOS(pPlayGroup1->head);
        pPlayGroup1->head = NULL;
    }

    pPlayGroup1->top = 0;
    pPlayGroup1->statue = 0;

    if (pPlayGroup2->head != NULL)
    {
        vdec_tsk_FreeOS(pPlayGroup2->head);
        pPlayGroup2->head = NULL;
    }

    pPlayGroup2->top = 0;
    pPlayGroup2->statue = 0;

    if (pPlayGroup1->playFrame != NULL)
    {
        vdec_tsk_FreeOS(pPlayGroup1->playFrame);
        pPlayGroup1->playFrame = NULL;
    }

    if (pPlayGroup2->playFrame != NULL)
    {
        vdec_tsk_FreeOS(pPlayGroup2->playFrame);
        pPlayGroup2->playFrame = NULL;
    }

    return SAL_SOK;
}

/**
 * @function   vdec_tsk_StackIn
 * @brief    数据压入栈中
 * @param[in]  DEC_Stack *node 数据
 * @param[out] None
 * @return     static INT32
 */
static INT32 vdec_tsk_StackIn(struct DEC_Stack *node)
{
    struct DEC_List *p = NULL;

    if (node->top >= MAX_PLAYGROUP_FRAME_NUM)
    {
        VDEC_LOGE("err!top=%d %d\n", node->top, MAX_PLAYGROUP_FRAME_NUM);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM);
    }

    p = node->playFrame + node->top;

    node->top++;
    p->next = node->head->next;
    node->head->next = p;

    return SAL_SOK;
}

/**
 * @function   vdec_tsk_StackOut
 * @brief     数据出栈
 * @param[in]  DEC_Stack **node   栈
 * @param[in]  SYSTEM_FRAME_INFO **pSendFrame  数据帧
 * @param[out] None
 * @return     static INT32
 */
static INT32 vdec_tsk_StackOut(struct DEC_Stack **node, SYSTEM_FRAME_INFO **pSendFrame)
{
    struct DEC_List *p = NULL;

    if ((*node)->top == 0)
    {
        VDEC_LOGE("top empty err\n");
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_DATA);
    }
    else
    {
        (*node)->top--;
        p = (*node)->head->next;
        *pSendFrame = &p->stFrame;
        (*node)->head->next = (*node)->head->next->next;
    }

    return SAL_SOK;
}

/**
 * @function   vdec_tsk_StackOut
 * @brief     数据帧映射
 * @param[in]  SYSTEM_FRAME_INFO *pstSystemFrameInfo 数据帧
 * @param[in]  PIC_FRAME_PRM *pPicprm数据帧映射出的数据
 * @param[out] None
 * @return     static INT32
 */
static INT32 vdec_tsk_Mmap(SYSTEM_FRAME_INFO *pstSystemFrameInfo, PIC_FRAME_PRM *pPicprm)
{
    INT32 s32Ret = SAL_SOK;

    CHECK_PTR_NULL(pstSystemFrameInfo, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pPicprm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    s32Ret = vdec_hal_Mmap((void *)pstSystemFrameInfo->uiAppData, pPicprm);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("sysframe mmap failed ret 0x%x\n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

#if 0

/******************************************************************
   Function:   vdec_tsk_GetVideoFrameSt
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static INT32 vdec_tsk_GetVideoFrameSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    void *pOutFrm = NULL;

    CHECK_PTR_NULL(pstSystemFrameInfo, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pOutFrm = (void *)vdec_tsk_MallocOS(sizeof(void));

    CHECK_PTR_NULL(pOutFrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    SAL_clear(pOutFrm);

    pstSystemFrameInfo->uiAppData = (PhysAddr)pOutFrm;

    return SAL_SOK;
}

/******************************************************************
   Function:   vdec_tsk_FreeVideoFrameSt
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static INT32 vdec_tsk_FreeVideoFrameSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    void *pOutFrm = NULL;

    CHECK_PTR_NULL(pstSystemFrameInfo, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pOutFrm = (void *)pstSystemFrameInfo->uiAppData;

    CHECK_PTR_NULL(pOutFrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    vdec_tsk_FreeOS(pOutFrm);

    pOutFrm = NULL;

    return SAL_SOK;
}

#endif

/**
 * @function   vdec_tsk_MallocMem
 * @brief     申请平台相关帧数据内存
 * @param[in]  UINT32 u32width 数据宽
 * @param[in]  UINT32 u32height 数据高
 * @param[in]  UINT32 u32format 数据格式
 * @param[in]  HAL_MEM_PRM *addrPrm 内存管理
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_MallocMem(UINT32 u32width, UINT32 u32height, UINT32 u32format, HAL_MEM_PRM *addrPrm)
{
    INT32 s32Ret = SAL_SOK;
    ALLOC_VB_INFO_S stVbInfo = {0};
   

    CHECK_PTR_NULL(addrPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    if (u32width > MAX_VDEC_WIDTH)
    {
        VDEC_LOGE("input prm width %d err\n", u32width);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM);
    }

    if (u32height > MAX_VDEC_HEIGHT)
    {
        VDEC_LOGE("input prm height %d err\n", u32height);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM);
    }
    if(u32format == SAL_VIDEO_DATFMT_RGB24_888)
    {
        addrPrm->memSize = u32width * u32height * 3;
    }
    else
    {
        addrPrm->memSize = u32width * u32height * 3 / 2;
    }
    
    s32Ret = mem_hal_vbAlloc(addrPrm->memSize, "vdec_tsk", "vdec", NULL, SAL_FALSE,&stVbInfo);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("mem alloc failed sret 0x%x\n",s32Ret);
        s32Ret = vdec_hal_MallocYUVMem(u32width, u32height, addrPrm);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("malloc format %d err ret 0x%x\n", u32format, s32Ret);
            return s32Ret;
        }
    }
    else
    {
        addrPrm->phyAddr = stVbInfo.u64PhysAddr;
        addrPrm->virAddr = stVbInfo.pVirAddr;
        addrPrm->poolId = stVbInfo.u32PoolId;
        addrPrm->vbBlk = stVbInfo.u64VbBlk;
    }

    return s32Ret;
}

/**
 * @function   vdec_tsk_GetPbPlatMem
 * @brief     倒放帧内存申请
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  UINT32 imgW 数据高
 * @param[in]  UINT32 imgH 数据格式
 * @param[in]  SYSTEM_FRAME_INFO *pstFrameInfo 将内存封装成数据帧
 * @param[out] None
 * @return     static INT32
 */
static INT32 vdec_tsk_GetPbPlatMem(UINT32 u32VdecChn, UINT32 imgW, UINT32 imgH, SYSTEM_FRAME_INFO *pstFrameInfo)
{
    INT32 s32Ret = 0;
    SYSTEM_FRAME_INFO *pstSysFrame = NULL;
    UINT32 indx = 0;
    HAL_MEM_PRM *addrPrm = NULL;
    SAL_VideoFrameBuf pPicFramePrm = {0};
    CAPB_VDEC_DUP *capbVdecDup = capb_get_vdecDup();

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(pstFrameInfo, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    if (playBackMemManage[u32VdecChn].memCnt >= (MAX_PLAYGROUP_FRAME_NUM * 2))
    {
        VDEC_LOGE("u32VdecChn %d mem cnt   %d is too much\n", u32VdecChn, playBackMemManage[u32VdecChn].memCnt);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM);
    }

    indx = playBackMemManage[u32VdecChn].memCnt;
    pstSysFrame = &playBackMemManage[u32VdecChn].playBackMemInfo[indx];
    addrPrm = &playBackMemManage[u32VdecChn].addrPrm[indx];

    SAL_clear(addrPrm);
    s32Ret = vdec_tsk_MallocMem(imgW, imgH, capbVdecDup->enOutputSalPixelFmt, addrPrm);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("u32VdecChn %d get mem errro s32Ret 0x%x\n", u32VdecChn, s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_MEM_ALLOC);
    }

    SAL_clear(&pPicFramePrm);
    pPicFramePrm.frameParam.width = imgW;
    pPicFramePrm.frameParam.height = imgH;
    pPicFramePrm.poolId = addrPrm->poolId;
    pPicFramePrm.stride[0] = imgW;
    pPicFramePrm.stride[1] = imgW;
    pPicFramePrm.stride[2] = imgW;
    pPicFramePrm.frameParam.dataFormat = capbVdecDup->enOutputSalPixelFmt;
    pPicFramePrm.virAddr[0] = (PhysAddr)addrPrm->virAddr;
    pPicFramePrm.virAddr[1] = pPicFramePrm.virAddr[0] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height;
    pPicFramePrm.virAddr[2] = pPicFramePrm.virAddr[1] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height / 4;
    pPicFramePrm.phyAddr[0] = addrPrm->phyAddr;
    pPicFramePrm.phyAddr[1] = pPicFramePrm.phyAddr[0] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height;
    pPicFramePrm.phyAddr[2] = pPicFramePrm.phyAddr[1] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height / 4;
    sys_hal_allocVideoFrameInfoSt(pstSysFrame);
    s32Ret = sys_hal_buildVideoFrame(&pPicFramePrm, pstSysFrame);
    if (s32Ret != SAL_SOK)
    {
        XPACK_LOGE("chn %d make frame failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    pstFrameInfo->uiAppData = pstSysFrame->uiAppData;

    playBackMemManage[u32VdecChn].memCnt++;
    return SAL_SOK;
}

/**
 * @function   vdec_tsk_PutAllPbPlatMem
 * @brief     倒放帧内存释放
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return     static INT32
 */
static INT32 vdec_tsk_PutAllPbPlatMem(UINT32 u32VdecChn)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    SYSTEM_FRAME_INFO *pstSysFrame = NULL;
    HAL_MEM_PRM *addrPrm = NULL;

    if (playBackMemManage[u32VdecChn].memCnt <= 0)
    {
        //VDEC_LOGE("u32VdecChn %d mem is not alloc cnt %d \n", u32VdecChn, playBackMemManage[u32VdecChn].memCnt);
        return SAL_SOK;
    }

    for (i = 0; i < playBackMemManage[u32VdecChn].memCnt; i++)
    {
        pstSysFrame = &playBackMemManage[u32VdecChn].playBackMemInfo[i];
        addrPrm = &playBackMemManage[u32VdecChn].addrPrm[i];
        s32Ret = vdec_hal_FreeYUVMem(addrPrm);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("u32VdecChn %d put mem errro ret 0x%x\n", u32VdecChn, s32Ret);
        }

        sys_hal_rleaseVideoFrameInfoSt(pstSysFrame);
        memset(pstSysFrame, 0, sizeof(SYSTEM_FRAME_INFO));
    }

    playBackMemManage[u32VdecChn].memCnt = 0;

    return s32Ret;
}

/**
 * @function   vdec_tsk_DupPutfreame
 * @brief     dup给到的数据帧释放
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  UINT32 voDev dup通道号
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 数据帧
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_DupPutfreame(UINT32 u32VdecChn, UINT32 voDev, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 s32Ret = SAL_SOK;
    DUP_ChanHandle *pstDupHandle = NULL;
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    DUP_COPY_DATA_BUFF pstDupCopyBuff = {0};

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(pstFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    if (voDev >= 4)
    {
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
    }

    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];

    pstDupHandle = pVdecTskChn->decModCtrl.dupChanHandle[voDev];
    CHECK_PTR_NULL(pstDupHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pstDupCopyBuff.pstDstSysFrame = pstFrame;

    if (SAL_SOK != (s32Ret = pstDupHandle->dupOps.OpDupPutBlit(pstDupHandle, &pstDupCopyBuff)))
    {
        VDEC_LOGE("chn %d chn %d get dup frame err ret 0x%x\n", u32VdecChn, voDev, s32Ret);
        return s32Ret;
    }

    return SAL_SOK;
}

/**
 * @function   vdec_tsk_PutVideoFrmaeMeth
 * @brief     释放解码数据帧 方式判断
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  UINT32 voDev dup通道号
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 数据帧
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_PutVideoFrmaeMeth(UINT32 vdecChn, UINT32 voDev, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_BIND_CTL bindCtl;

    vdec_hal_VdecGetBind(vdecChn, &bindCtl);
    if (bindCtl.needBind == 1)
    {
        s32Ret = vdec_tsk_DupPutfreame(vdecChn, voDev, pstFrame);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("chn %d put dup frame failed ret 0x%x\n", vdecChn, s32Ret);
            return s32Ret;
        }
    }
    else
    {
        s32Ret = vdec_hal_VdecReleaseframe(vdecChn, voDev, (void *)pstFrame->uiAppData);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("chn %d get hal frame failed ret 0x%x\n", vdecChn, s32Ret);
            return s32Ret;
        }
    }
    

    return s32Ret;
}

/**
 * @function   vdec_tsk_PutVdecFrame
 * @brief     释放解码数据帧
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  UINT32 chn 释放帧通道号
 * @param[in]  SYSTEM_FRAME_INFO *prm 数据帧
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_PutVdecFrame(UINT32 u32VdecChn, UINT32 chn, SYSTEM_FRAME_INFO *prm)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_TASK_CTL *pVdecTskCtl = NULL;
    
    pVdecTskCtl = &g_stVecTaskCtl[u32VdecChn];

    s32Ret = vdec_tsk_PutVideoFrmaeMeth(u32VdecChn, chn, prm);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d put hal frame failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    pVdecTskCtl->unReleaseFrameCnt--;
    if ((int)pVdecTskCtl->unReleaseFrameCnt < 0)
    {
        VDEC_LOGE("cur chn %d un releasecnt %d err \n",u32VdecChn,pVdecTskCtl->unReleaseFrameCnt);
        pVdecTskCtl->unReleaseFrameCnt = 0;
        return SAL_FAIL;
    }

    return s32Ret;
}

/**
 * @function   vdec_tsk_DupGetfreame
 * @brief     获取数据帧
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  UINT32 voDev 释放帧通道号
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 数据帧
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_DupGetfreame(UINT32 u32VdecChn, UINT32 voDev, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 s32Ret = SAL_SOK;
    DUP_ChanHandle *pstDupHandle = NULL;
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    DUP_COPY_DATA_BUFF pstDupCopyBuff = {0};
    VDEC_HAL_STATUS pvdecStatus = {0};

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(pstFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    if (voDev >= 4)
    {
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
    }

    vdec_hal_VdecChnStatus(u32VdecChn, &pvdecStatus);
    if (pvdecStatus.curChnDecoding == 0)
    {
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];

    pstDupHandle = pVdecTskChn->decModCtrl.dupChanHandle[voDev];
    if (pstDupHandle == NULL)
    {
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR);
    }

    pstDupCopyBuff.pstDstSysFrame = pstFrame;

    if (SAL_SOK != (s32Ret = pstDupHandle->dupOps.OpDupGetBlit(pstDupHandle, &pstDupCopyBuff)))
    {
        VDEC_LOGD("chn %d chn %d get dup frame err ret 0x%x\n", u32VdecChn, voDev, s32Ret);
        return s32Ret;
    }

    return SAL_SOK;
}


/**
 * @function   vdec_tsk_DupSendfreame
 * @brief     送数据帧
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  UINT32 voDev 释放帧通道号
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 数据帧
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_DupSendfreame(UINT32 u32VdecChn, UINT32 voDev, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 s32Ret = SAL_SOK;
    DupHandle pstDupHandle = 0;
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    VDEC_HAL_STATUS pvdecStatus = {0};

    //VDEC_LOGW("chn %d chn %d get dup frame err ret 0x%x\n", u32VdecChn, voDev, s32Ret);

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(pstFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    vdec_hal_VdecChnStatus(u32VdecChn, &pvdecStatus);
    if (pvdecStatus.curChnDecoding == 0)
    {
         VDEC_LOGE("chn %d chn %d get dup frame err ret 0x%x\n", u32VdecChn, voDev, s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];

    pstDupHandle = pVdecTskChn->decModCtrl.frontDupHandle;
  
    if (SAL_SOK != dup_task_sendToDup(pstDupHandle, pstFrame))
    {
        VDEC_LOGE("chn %d chn %d get dup frame err ret 0x%x\n", u32VdecChn, voDev, s32Ret);
        return s32Ret;
    }
    

    return SAL_SOK;
}


/**
 * @function   vdec_tsk_GetVideoFrmaeMeth
 * @brief     获取数据帧方式
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  UINT32 voDev 释放帧通道号
 * @param[in]  UINT32 *needFree 获取的帧是否需要释放
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 数据帧
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_GetVideoFrmaeMeth(UINT32 u32VdecChn, UINT32 voDev, UINT32 *needFree, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_BIND_CTL bindCtl;
    DEC_MOD_CTRL *pDecModeCtrl = NULL;
    UINT32 u32Chn = voDev;
    UINT32 u32TimeOut = 30;
    pDecModeCtrl = &g_stVdecObjCommon.stObjTsk[u32VdecChn].decModCtrl;
    (void)vdec_hal_VdecGetBind(u32VdecChn, &bindCtl);

    if (bindCtl.needBind == 1)
    {

        /**NT单步方式需要vpe做裁剪和格式转换(解码只能输出NX4压缩格式yuv)。解码步骤push_in vdec->pull_out vdec -> push_in vpe ->pull_out vpe*/
        if(pDecModeCtrl->dupBindType == NODE_BIND_TYPE_GET)
        {
           
            s32Ret = vdec_hal_VdecGetframe(u32VdecChn, u32Chn, (void *)pstFrame->uiAppData, u32TimeOut);
            if (s32Ret != SAL_SOK)
            {
                // VDEC_LOGE("chn %d get hal frame failed ret 0x%x\n", u32VdecChn, s32Ret);
                return s32Ret;
            }
            
            s32Ret = vdec_tsk_DupSendfreame(u32VdecChn, u32Chn, pstFrame);
            if (s32Ret != SAL_SOK)
            {
                s32Ret = vdec_hal_VdecReleaseframe(u32VdecChn, u32Chn, pstFrame);
                if (s32Ret != SAL_SOK)
                {
                    VDEC_LOGE("chn %d get dup frame failed ret 0x%x\n",u32VdecChn,s32Ret);
                    return s32Ret;
                }
                VDEC_LOGE("chn %d get dup frame failed ret 0x%x\n",u32VdecChn,s32Ret);
                return SAL_FAIL;
            }
    
            s32Ret = vdec_hal_VdecReleaseframe(u32VdecChn, u32Chn, pstFrame);
            if (s32Ret != SAL_SOK)
            {
                VDEC_LOGE("chn %d get dup frame failed ret 0x%x\n",u32VdecChn,s32Ret);
                return s32Ret;
            }
        }

        s32Ret = vdec_tsk_DupGetfreame(u32VdecChn, u32Chn, pstFrame);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGD("chn %d get dup frame failed ret 0x%x\n",u32VdecChn,s32Ret);
            return s32Ret;
        }
         
    }
    else
    {
        s32Ret = vdec_hal_VdecGetframe(u32VdecChn, u32Chn, (void *)pstFrame->uiAppData, u32TimeOut);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("chn %d get hal frame failed ret 0x%x\n", u32VdecChn, s32Ret);
            return s32Ret;
        }
    }

    *needFree = 1; /* fixme: 此处为什么要将外部变量置1 */

    return s32Ret;
}

/**
 * @function   vdec_tsk_GetVdecFrameForPaus
 * @brief     暂停状态下其他模块获取帧
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  UINT32 u32Purpose 用途
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 数据帧
 * @param[out] None
 * @return     static INT32
 */
static INT32 vdec_tsk_GetVdecFrameForPaus(UINT32 u32VdecChn, UINT32 u32Purpose, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_PIC_COPY_EN copyEn;
    PIC_FRAME_PRM pPicprm;
    void *pVideoFrame = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    if (u32Purpose == VDEC_DISP_GET_FRAME)
    {
        pVideoFrame = (void *)stPauseFrameBuff[u32VdecChn].stFInfoForDisp.uiAppData;
        if (pVideoFrame == NULL)
        {
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR);
        }

        copyEn.copyth = 1;
        copyEn.copyMeth = VDEC_DATA_ADDR_COPY;
        s32Ret = vdec_hal_VdecCpyframe(&copyEn, (void *)stPauseFrameBuff[u32VdecChn].stFInfoForDisp.uiAppData, (void *)pstFrame->uiAppData);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGW("vdecChn %d failed\n", u32VdecChn);
            return s32Ret;
        }

        vdec_tsk_Mmap(&stPauseFrameBuff[u32VdecChn].stFInfoForDisp, &pPicprm);
        pstFrame->uiDataWidth = pPicprm.width;
        pstFrame->uiDataHeight = pPicprm.height;
        if(stPauseFrameBuff[u32VdecChn].stFInfoForDisp.uiDataAddr == 0)
        {
            pstFrame->uiDataAddr = (PhysAddr)pPicprm.addr[0];
        }
        else
        {
            pstFrame->uiDataAddr = stPauseFrameBuff[u32VdecChn].stFInfoForDisp.uiDataAddr;
        }

    }
    else
    {
        pVideoFrame = (void *)stPauseFrameBuff[u32VdecChn].stFInfoForJpeg.uiAppData;
        if (pVideoFrame == NULL)
        {
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR);
        }

        copyEn.copyth = 1;
        copyEn.copyMeth = VDEC_DATA_ADDR_COPY;
        s32Ret = vdec_hal_VdecCpyframe(&copyEn, (void *)stPauseFrameBuff[u32VdecChn].stFInfoForJpeg.uiAppData, (void *)pstFrame->uiAppData);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGW("vdecChn %d failed\n", u32VdecChn);
            return s32Ret;
        }

        vdec_tsk_Mmap(&stPauseFrameBuff[u32VdecChn].stFInfoForJpeg, &pPicprm);
        pstFrame->uiDataWidth = pPicprm.width;
        pstFrame->uiDataHeight = pPicprm.height;
        if(stPauseFrameBuff[u32VdecChn].stFInfoForJpeg.uiDataAddr == 0)
        {
            pstFrame->uiDataAddr = (PhysAddr)pPicprm.addr[0];
        }
        else
        {
            pstFrame->uiDataAddr = stPauseFrameBuff[u32VdecChn].stFInfoForJpeg.uiDataAddr;
        }
    }

    return s32Ret;
}

/**
 * @function   vdec_tsk_GetVdecFrameForPausFromVdec
 * @brief     暂停帧保存
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  INT32 s32PicIndx 是否使用其他图片
 * @param[out] None
 * @return     static INT32
 */
static INT32 vdec_tsk_GetVdecFrameForPausFromVdec(UINT32 u32VdecChn, INT32 s32PicIndx)
{
    INT32 s32Ret = SAL_SOK;
    void *pVirAddr = NULL;
    UINT32 u32Width = 0;
    UINT32 u32Height = 0;
    UINT32 needFree = 0;
    SYSTEM_FRAME_INFO *pstFrameInfo;
    DSPINITPARA *pDspInitPara = NULL;
    SAL_VideoFrameBuf pPicFramePrm = {0};
    VDEC_PIC_COPY_EN copyEn = {0};
    VDEC_PAUSE_BUFF *pPauseFrameBuff = NULL;
    SYSTEM_FRAME_INFO sysFrameInfo = {0};
    PIC_FRAME_PRM pPicprm;
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    DEC_MOD_CTRL *pDecModeCtrl = NULL;
    CAPB_VDEC_DUP *capbVdecDup = capb_get_vdecDup();

    UINT32 u32JpegChn = 0;
    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];
    pDecModeCtrl = &pVdecTskChn->decModCtrl;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));

    pPauseFrameBuff = &stPauseFrameBuff[u32VdecChn];
    pstFrameInfo = &g_stVdecObjCommon.stObjTsk[u32VdecChn].decModCtrl.stFrameInfo;
    if (pstFrameInfo->uiAppData == 0x00)
    {
        VDEC_LOGE("chn %d stFrameInfo null err\n", u32VdecChn);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM);
    }

    if (pPauseFrameBuff->stFInfoForDisp.uiAppData == 0x00)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&pPauseFrameBuff->stFInfoForDisp);
        if (s32Ret != SAL_SOK)
        {
            XPACK_LOGE("chn %d alloc struct video frame failed \n", u32VdecChn);
            return s32Ret;
        }

        SAL_clear(&pPauseFrameBuff->addrMem[0]);
        s32Ret = vdec_tsk_MallocMem(MAX_VDEC_WIDTH, MAX_VDEC_HEIGHT, capbVdecDup->enOutputSalPixelFmt, &pPauseFrameBuff->addrMem[0]);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("u32VdecChn %d get mem errro s32Ret 0x%x\n", u32VdecChn, s32Ret);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_MEM_ALLOC);
        }

        SAL_clear(&pPicFramePrm);
        pPicFramePrm.frameParam.width = MAX_VDEC_WIDTH;
        pPicFramePrm.frameParam.height = MAX_VDEC_HEIGHT;
        pPicFramePrm.poolId = pPauseFrameBuff->addrMem[0].poolId;
        pPicFramePrm.stride[0] = MAX_VDEC_WIDTH;
        pPicFramePrm.stride[1] = MAX_VDEC_WIDTH;
        pPicFramePrm.stride[2] = MAX_VDEC_WIDTH;
        pPicFramePrm.frameParam.dataFormat = capbVdecDup->enOutputSalPixelFmt;
        pPicFramePrm.virAddr[0] = (PhysAddr)pPauseFrameBuff->addrMem[0].virAddr;
        pPicFramePrm.virAddr[1] = pPicFramePrm.virAddr[0] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height;
        pPicFramePrm.virAddr[2] = pPicFramePrm.virAddr[1] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height / 4;
        pPicFramePrm.phyAddr[0] = pPauseFrameBuff->addrMem[0].phyAddr;
        pPicFramePrm.phyAddr[1] = pPicFramePrm.phyAddr[0] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height;
        pPicFramePrm.phyAddr[2] = pPicFramePrm.phyAddr[1] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height / 4;
        s32Ret = sys_hal_buildVideoFrame(&pPicFramePrm, &pPauseFrameBuff->stFInfoForDisp);
        if (s32Ret != SAL_SOK)
        {
            XPACK_LOGE("chn %d make frame failed ret 0x%x\n", u32VdecChn, s32Ret);
            return s32Ret;
        }

        VDEC_LOGI("chn %d malloc disp mem vir %llu phy %llu\n", u32VdecChn, pPicFramePrm.virAddr[0], pPicFramePrm.phyAddr[0]);
    }

    if (pPauseFrameBuff->stFInfoForJpeg.uiAppData == 0x00)
    {
            s32Ret = sys_hal_allocVideoFrameInfoSt(&pPauseFrameBuff->stFInfoForJpeg);
            if (s32Ret != SAL_SOK)
            {
                XPACK_LOGE("chn %d alloc struct video frame failed \n", u32VdecChn);
                return s32Ret;
            }
    
            SAL_clear(&pPauseFrameBuff->addrMem[1]);
            s32Ret = vdec_tsk_MallocMem(MAX_VDEC_WIDTH, MAX_VDEC_HEIGHT, capbVdecDup->enOutputSalPixelFmt, &pPauseFrameBuff->addrMem[1]);
            if (s32Ret != SAL_SOK)
            {
                XPACK_LOGE("DupHal_drvGetFrameMem failed with %#x\n", s32Ret);
                return s32Ret;
            }
    
            SAL_clear(&pPicFramePrm);
            pPicFramePrm.frameParam.width = MAX_VDEC_WIDTH;
            pPicFramePrm.frameParam.height = MAX_VDEC_HEIGHT;
            pPicFramePrm.poolId = pPauseFrameBuff->addrMem[1].poolId;
            pPicFramePrm.stride[0] = MAX_VDEC_WIDTH;
            pPicFramePrm.stride[1] = MAX_VDEC_WIDTH;
            pPicFramePrm.stride[2] = MAX_VDEC_WIDTH;
            pPicFramePrm.frameParam.dataFormat = capbVdecDup->enOutputSalPixelFmt;
            pPicFramePrm.virAddr[0] = (PhysAddr)pPauseFrameBuff->addrMem[1].virAddr;
            pPicFramePrm.virAddr[1] = pPicFramePrm.virAddr[0] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height;
            pPicFramePrm.virAddr[2] = pPicFramePrm.virAddr[1] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height / 4;
            pPicFramePrm.phyAddr[0] = pPauseFrameBuff->addrMem[1].phyAddr;
            pPicFramePrm.phyAddr[1] = pPicFramePrm.phyAddr[0] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height;
            pPicFramePrm.phyAddr[2] = pPicFramePrm.phyAddr[1] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height / 4;
            s32Ret = sys_hal_buildVideoFrame(&pPicFramePrm, &pPauseFrameBuff->stFInfoForJpeg);
            if (s32Ret != SAL_SOK)
            {
                XPACK_LOGE("chn %d make frame failed ret 0x%x\n", u32VdecChn, s32Ret);
                return s32Ret;
            }
    
            VDEC_LOGI("chn %d malloc jpeg mem vir %llu phy %llu\n", u32VdecChn, pPicFramePrm.virAddr[0], pPicFramePrm.phyAddr[0]);
       
    }

    if (s32PicIndx == -1)
    {
        s32Ret = vdec_tsk_GetVideoFrmaeMeth(u32VdecChn, 0, &needFree, pstFrameInfo);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("vdecChn %d get frame failed ret 0x%x\n", u32VdecChn, s32Ret);
            return s32Ret;
        }

        copyEn.copyth = 0;
        copyEn.copyMeth = VDEC_DATA_COPY;
        s32Ret = vdec_hal_VdecCpyframe(&copyEn, (void *)pstFrameInfo->uiAppData, (void *)pPauseFrameBuff->stFInfoForDisp.uiAppData);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGW("vdecChn %d failed\n", u32VdecChn);
        }

        vdec_tsk_Mmap(&pPauseFrameBuff->stFInfoForDisp, &pPicprm);
        pPauseFrameBuff->stFInfoForDisp.uiDataWidth = pPicprm.width;
        pPauseFrameBuff->stFInfoForDisp.uiDataHeight = pPicprm.height;
        /*部分平台帧信息不带虚拟地址*/
        if(pPauseFrameBuff->stFInfoForDisp.uiDataAddr == 0)
        {
            pPauseFrameBuff->stFInfoForDisp.uiDataAddr = (PhysAddr)pPicprm.addr[0];
        }

        /*单步执行使用相通数据源*/
        if(pDecModeCtrl->dupBindType != NODE_BIND_TYPE_GET)
        {
            if (needFree == 1)
            {
                s32Ret = vdec_tsk_PutVideoFrmaeMeth(u32VdecChn, 0, pstFrameInfo);
                if (s32Ret != SAL_SOK)
                {
                    VDEC_LOGE("vdecChn %d put frame failed ret 0x%x\n", u32VdecChn, s32Ret);
                    return s32Ret;
                }
            }

            u32JpegChn = 2;
            needFree = 0;

            s32Ret = vdec_tsk_GetVideoFrmaeMeth(u32VdecChn, u32JpegChn, &needFree, pstFrameInfo);
            if (s32Ret != SAL_SOK)
            {
                VDEC_LOGE("vdecChn %d get u32JpegChn frame failed ret 0x%x\n", u32VdecChn, s32Ret);
                return s32Ret;
            }
        }

        s32Ret = vdec_hal_VdecCpyframe(&copyEn, (void *)pstFrameInfo->uiAppData, (void *)pPauseFrameBuff->stFInfoForJpeg.uiAppData);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGW("vdecChn %d failed\n", u32VdecChn);
        }

        vdec_tsk_Mmap(&pPauseFrameBuff->stFInfoForJpeg, &pPicprm);
        pPauseFrameBuff->stFInfoForJpeg.uiDataWidth = pPicprm.width;
        pPauseFrameBuff->stFInfoForJpeg.uiDataHeight = pPicprm.height;
        if(pPauseFrameBuff->stFInfoForJpeg.uiDataAddr == 0)
        {
            pPauseFrameBuff->stFInfoForJpeg.uiDataAddr = (PhysAddr)pPicprm.addr[0];
        }


        if (needFree == 1)
        {
            s32Ret = vdec_tsk_PutVideoFrmaeMeth(u32VdecChn, u32JpegChn, pstFrameInfo);
            if (s32Ret != SAL_SOK)
            {
                VDEC_LOGE("vdecChn %d put frame failed ret 0x%x\n", u32VdecChn, s32Ret);
                return s32Ret;
            }
        }
        
       
    }
    else
    {
        if (s32PicIndx >= MAX_NO_SIGNAL_PIC_CNT || s32PicIndx < 0)
        {
            VDEC_LOGE("u32VdecChn %d s32PicIndx %d err\n", u32VdecChn, s32PicIndx);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM);
        }

        pDspInitPara = SystemPrm_getDspInitPara();

        pVirAddr = pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgAddr[s32PicIndx];
        u32Width = pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgW[s32PicIndx];
        u32Height = pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgH[s32PicIndx];

        s32Ret = sys_hal_allocVideoFrameInfoSt(&sysFrameInfo);
        if (s32Ret != SAL_SOK)
        {
            XPACK_LOGE("chn %d alloc struct video frame failed \n", u32VdecChn);
            return s32Ret;
        }

        SAL_clear(&pPicprm);
        pPicprm.width = u32Width;
        pPicprm.height = u32Height;
        pPicprm.stride = u32Width;
        pPicprm.videoFormat = capbVdecDup->enOutputSalPixelFmt;
        pPicprm.addr[0] = pVirAddr;
        pPicprm.addr[1] = pPicprm.addr[0] + u32Width * u32Height;
        pPicprm.addr[2] = pPicprm.addr[1] + u32Width * u32Height / 4;
        s32Ret = vdec_hal_VdecMakeframe(&pPicprm, (void *)sysFrameInfo.uiAppData);
        if (s32Ret != SAL_SOK)
        {
            XPACK_LOGE("chn %d make frame failed ret 0x%x\n", u32VdecChn, s32Ret);
            goto end;
        }

        copyEn.copyth = 1;
        copyEn.copyMeth = VDEC_DATA_COPY;
        s32Ret = vdec_hal_VdecCpyframe(&copyEn, (void *)sysFrameInfo.uiAppData, (void *)pPauseFrameBuff->stFInfoForDisp.uiAppData);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGW("vdecChn %d failed\n", u32VdecChn);
            goto end;
        }

        vdec_tsk_Mmap(&pPauseFrameBuff->stFInfoForDisp, &pPicprm);
        pPauseFrameBuff->stFInfoForDisp.uiDataWidth = pPicprm.width;
        pPauseFrameBuff->stFInfoForDisp.uiDataHeight = pPicprm.height;
        if(pPauseFrameBuff->stFInfoForDisp.uiDataAddr == 0)
        {
            pPauseFrameBuff->stFInfoForDisp.uiDataAddr = (PhysAddr)pPicprm.addr[0];
        }

        copyEn.copyth = 1;
        copyEn.copyMeth = VDEC_DATA_COPY;
        s32Ret = vdec_hal_VdecCpyframe(&copyEn, (void *)sysFrameInfo.uiAppData, (void *)pPauseFrameBuff->stFInfoForJpeg.uiAppData);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGW("vdecChn %d failed\n", u32VdecChn);
            goto end;
        }

        vdec_tsk_Mmap(&pPauseFrameBuff->stFInfoForJpeg, &pPicprm);
        pPauseFrameBuff->stFInfoForJpeg.uiDataWidth = pPicprm.width;
        pPauseFrameBuff->stFInfoForJpeg.uiDataHeight = pPicprm.height;
        if(pPauseFrameBuff->stFInfoForJpeg.uiDataAddr == 0)
        {
            pPauseFrameBuff->stFInfoForJpeg.uiDataAddr = (PhysAddr)pPicprm.addr[0];
        }
        
end:
        s32Ret = sys_hal_rleaseVideoFrameInfoSt(&sysFrameInfo);
        if (s32Ret != SAL_SOK)
        {
            XPACK_LOGE("chn %d free struct video frame failed \n", u32VdecChn);
            return s32Ret;
        }

        VDEC_LOGI("chn %d pause use indx %d pic\n", u32VdecChn, s32PicIndx);
    }

    VDEC_LOGI("chn %d get pause frame success\n", u32VdecChn);
    return s32Ret;
}

/**
 * @function   vdec_tsk_GetPbFrame
 * @brief     其他模块获取倒放帧
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 数据帧
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_GetPbFrame(UINT32 u32VdecChn, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 s32Ret = 0;
    VDEC_PIC_COPY_EN copyEn = {0};
    struct DEC_Stack *pPBGroup = NULL;
    DEC_MOD_CTRL *pDecCtrl = NULL;
    SYSTEM_FRAME_INFO *pFrame = NULL;
    VDEC_TASK_CTL *pVdecCtl = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    pVdecCtl = &g_stVecTaskCtl[u32VdecChn];
    pDecCtrl = &g_stVdecObjCommon.stObjTsk[u32VdecChn].decModCtrl;

    if (pDecCtrl->chnStatus == VDEC_STATE_RUNNING)
    {
check:
        if (DECODE_MODE_REVERSE != pVdecCtl->vdecMode)
        {
            VDEC_LOGE("chn %d mode %d error\n", u32VdecChn, pVdecCtl->vdecMode);
            return SAL_FAIL;
        }

        if (gOutPingPangId[u32VdecChn] > 1)
        {
            gOutPingPangId[u32VdecChn] = 0;
        }

        if (gPbGroup1[u32VdecChn].top <= 0 && gPbGroup2[u32VdecChn].top <= 0)
        {
            /* gOutPingPangId[chan] = 0; */
            sleep(1);
            VDEC_LOGW("chan %d,InId %d,OutId %d\n", u32VdecChn, gInPingPangId[u32VdecChn], gOutPingPangId[u32VdecChn]);
            goto check;
        }

        pPBGroup = (0 == gOutPingPangId[u32VdecChn]) ? &gPbGroup1[u32VdecChn] : &gPbGroup2[u32VdecChn];
        if (pPBGroup->top <= 0 || 0 == pPBGroup->statue)
        {
            gOutPingPangId[u32VdecChn] = (gOutPingPangId[u32VdecChn] == 0) ? 1 : 0;
            /* usleep(2 * 1000); */
            goto check;
        }

        /* p = pPBGroup->playFrame + pPBGroup->top; */
        s32Ret = vdec_tsk_StackOut(&pPBGroup, &pFrame);
        if (SAL_FAIL == s32Ret)
        {
            VDEC_LOGE("chn %d stack out error ret 0x%x\n", u32VdecChn, s32Ret);
            return SAL_FAIL;
            /* goto check; */
        }

        SAL_clear(&copyEn);
        copyEn.copyth = 0;
        copyEn.copyMeth = VDEC_DATA_COPY;
        s32Ret = vdec_hal_VdecCpyframe(&copyEn, (void *)pFrame->uiAppData, (void *)pstFrame->uiAppData);
        if (SAL_FAIL == s32Ret)
        {
            VDEC_LOGE("error\n");
        }
    }

    return SAL_SOK;
}

/**
 * @function   vdec_tsk_GetNoSignalPic
 * @brief     获取无视频信号
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  UINT32 u32Indx 图片索引
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 数据帧
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_GetNoSignalPic(UINT32 u32VdecChn, UINT32 u32Indx, SYSTEM_FRAME_INFO *pstSystemFrame)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32Width = 0;
    UINT32 u32Height = 0;
    UINT32 picSize = 0;
    VOID *pVirAddr = NULL;
    VDEC_PIC_COPY_EN copyEn = {0};
    SAL_VideoFrameBuf pPicFramePrm = {0};
    DSPINITPARA *pDspInitPara = NULL;
    static SYSTEM_FRAME_INFO sysFrameInfo[MAX_VDEC_PLAT_CHAN] = {0};
    VDEC_PAUSE_BUFF *pPauseFrameBuff = NULL;
    PIC_FRAME_PRM pPicprm;
    CAPB_VDEC_DUP *capbVdecDup = capb_get_vdecDup();

    if (u32Indx >= MAX_NO_SIGNAL_PIC_CNT)
    {
        VDEC_LOGE("error\n");
        return SAL_FAIL;
    }
    
    if (u32VdecChn > (MAX_VDEC_CHAN - 1))
    {
        VCA_LOGE("u32Chan %d error too large !\n", u32VdecChn);
        return SAL_FAIL;
    }

    pPauseFrameBuff = &stNoSignalFrameBuff[u32VdecChn];

    pDspInitPara = SystemPrm_getDspInitPara();
    if (pDspInitPara == NULL)
    {
        VDEC_LOGE("error\n");
        return SAL_FAIL;
    }

    pVirAddr = pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgAddr[u32Indx];
    u32Width = pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgW[u32Indx];
    u32Height = pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgH[u32Indx];
    picSize = pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgSize[u32Indx];

    if (pVirAddr == NULL || picSize == 0x00 || u32Width == 0 || u32Height == 0)
    {
        VDEC_LOGE("error\n");
        return SAL_FAIL;
    }

    u32Width = (u32Width / 16) * 16;

    if (pPauseFrameBuff->stFInfoForDisp.uiAppData == 0x00)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&pPauseFrameBuff->stFInfoForDisp);
        if (s32Ret != SAL_SOK)
        {
            XPACK_LOGE("chn %d alloc struct video frame failed \n", u32VdecChn);
            return s32Ret;
        }

        SAL_clear(&pPauseFrameBuff->addrMem[0]);
        s32Ret = vdec_tsk_MallocMem(MAX_VDEC_WIDTH, MAX_VDEC_HEIGHT, capbVdecDup->enOutputSalPixelFmt, &pPauseFrameBuff->addrMem[0]);
        if (s32Ret != SAL_SOK)
        {
            XPACK_LOGE("DupHal_drvGetFrameMem failed with %#x\n", s32Ret);
            return s32Ret;
        }

        SAL_clear(&pPicFramePrm);
        pPicFramePrm.frameParam.width = MAX_VDEC_WIDTH;
        pPicFramePrm.frameParam.height = MAX_VDEC_HEIGHT;
        pPicFramePrm.poolId = pPauseFrameBuff->addrMem[0].poolId;
        pPicFramePrm.stride[0] = MAX_VDEC_WIDTH;
        pPicFramePrm.stride[1] = MAX_VDEC_WIDTH;
        pPicFramePrm.stride[2] = MAX_VDEC_WIDTH;
        pPicFramePrm.frameParam.dataFormat = capbVdecDup->enOutputSalPixelFmt;
        pPicFramePrm.virAddr[0] = (PhysAddr)pPauseFrameBuff->addrMem[0].virAddr;
        pPicFramePrm.virAddr[1] = pPicFramePrm.virAddr[0] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height;
        pPicFramePrm.virAddr[2] = pPicFramePrm.virAddr[1] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height / 4;
        pPicFramePrm.phyAddr[0] = pPauseFrameBuff->addrMem[0].phyAddr;
        pPicFramePrm.phyAddr[1] = pPicFramePrm.phyAddr[0] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height;
        pPicFramePrm.phyAddr[2] = pPicFramePrm.phyAddr[1] + pPicFramePrm.frameParam.width * pPicFramePrm.frameParam.height / 4;
        s32Ret = sys_hal_buildVideoFrame(&pPicFramePrm, &pPauseFrameBuff->stFInfoForDisp);
        if (s32Ret != SAL_SOK)
        {
            XPACK_LOGE("chn %d make frame failed ret 0x%x\n", u32VdecChn, s32Ret);
            return s32Ret;
        }
    }

    if (sysFrameInfo[u32VdecChn].uiAppData == 0x00)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&sysFrameInfo[u32VdecChn]);
        if (s32Ret != SAL_SOK)
        {
            XPACK_LOGE("chn %d alloc struct video frame failed \n", u32VdecChn);
            return s32Ret;
        }
    }

    SAL_clear(&pPicprm);
    pPicprm.width = u32Width;
    pPicprm.height = u32Height;
    pPicprm.stride = u32Width;
    pPicprm.poolId = pPauseFrameBuff->addrMem[0].poolId;
    pPicprm.videoFormat = capbVdecDup->enOutputSalPixelFmt;
    pPicprm.addr[0] = pVirAddr;
    pPicprm.addr[1] = pPicprm.addr[0] + u32Width * u32Height;
    pPicprm.addr[2] = pPicprm.addr[1] + u32Width * u32Height / 4;
    sysFrameInfo[u32VdecChn].uiDataAddr = (PhysAddr)pVirAddr;
    s32Ret = vdec_hal_VdecMakeframe(&pPicprm, (void *)sysFrameInfo[u32VdecChn].uiAppData);
    if (s32Ret != SAL_SOK)
    {
        XPACK_LOGE("chn %d make frame failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    copyEn.copyth = 1;
    copyEn.copyMeth = VDEC_DATA_COPY;
    s32Ret = vdec_hal_VdecCpyframe(&copyEn, (void *)sysFrameInfo[u32VdecChn].uiAppData, (void *)pPauseFrameBuff->stFInfoForDisp.uiAppData);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGW("vdecChn %d failed\n", u32VdecChn);
        return s32Ret;
    }

    vdec_tsk_Mmap(&pPauseFrameBuff->stFInfoForDisp, &pPicprm);
    pstSystemFrame->uiDataWidth = pPicprm.width;
    pstSystemFrame->uiDataHeight = pPicprm.height;
    pstSystemFrame->uiDataAddr = (PhysAddr)pPicprm.addr[0];
    pPauseFrameBuff->stFInfoForDisp.uiDataWidth = pPicprm.width;
    pPauseFrameBuff->stFInfoForDisp.uiDataHeight = pPicprm.height;

    copyEn.copyth = 1;
    copyEn.copyMeth = VDEC_DATA_ADDR_COPY;
    s32Ret = vdec_hal_VdecCpyframe(&copyEn, (void *)pPauseFrameBuff->stFInfoForDisp.uiAppData, (void *)pstSystemFrame->uiAppData);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGW("vdecChn %d failed\n", u32VdecChn);
        return s32Ret;
    }

    return s32Ret;
}

/**
 * @function   vdec_tsk_GetVdecFrame
 * @brief     其他模块获取解码数据帧接口
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  UINT32 voDev 通道号
 * @param[in]  UINT32 purpose 数据帧用途
 * @param[in]  UINT32 *needFree 是否需要释放
 * @param[in]  UINT32 *status 是否允许叠框
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 数据帧
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_GetVdecFrame(UINT32 vdecChn, UINT32 voDev, UINT32 purpose, UINT32 *needFree, SYSTEM_FRAME_INFO *pstFrame, UINT32 *status)
{
    INT32 s32Ret = SAL_FAIL;
    SYSTEM_FRAME_INFO pDstFrame;
    VDEC_PIC_COPY_EN copyEn;
    DEC_MOD_CTRL *pDecModCtrl = NULL;
    VDEC_TASK_CTL *pVdecTskCtl = NULL;

    if (vdecChn >= MAX_VDEC_CHAN)
    {
        VDEC_LOGE("Chan %d (Illegal parameters)\n", vdecChn); 
        return SAL_FAIL; 
    }
    if (vdecChn > (MAX_VDEC_CHAN - 1))
    {
        VCA_LOGE("u32Chan %d error too large !\n", vdecChn);
        return SAL_FAIL;
    }
    CHECK_PTR_NULL(needFree, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    *needFree = 0;  /* fixme: 此处为什么要将外部变量置0 */

    pDecModCtrl = &g_stVdecObjCommon.stObjTsk[vdecChn].decModCtrl;
    pVdecTskCtl = &g_stVecTaskCtl[vdecChn];

    VDEC_LOGD("chn %d status %d\n", vdecChn, pDecModCtrl->chnStatus);
    SAL_mutexLock(pVdecTskCtl->mutex);

    if (pDecModCtrl->chnStatus == VDEC_STATE_RUNNING)
    {
        switch (purpose)
        {
            case VDEC_DISP_GET_FRAME:
            case VDEC_JPEG_GET_FRAME:
            case VDEC_BA_GET_FRAME:
            case VDEC_FACE_GET_FRAME:
            {
                if (pVdecTskCtl->vdecMode == DECODE_MODE_REVERSE && purpose == VDEC_DISP_GET_FRAME)
                {
                    s32Ret = vdec_tsk_GetPbFrame(vdecChn, pstFrame);
                    if (s32Ret != SAL_SOK)
                    {
                        VDEC_LOGE("error\n");
                    }
                }
                else
                {
                    if (VDEC_JPEG_GET_FRAME == purpose)
                    {
                        voDev = 2;
                    }

                    s32Ret = vdec_tsk_GetVideoFrmaeMeth(vdecChn, voDev, needFree, pstFrame);
                    if (s32Ret != SAL_SOK)
                    {
                        VDEC_LOGD("vdecChn %d voDev %d purpose %d get frame err\n", vdecChn, voDev, purpose);
                    }
                }

                break;
            }
            case VDEC_I_TO_BMP_GET_FRAME:
            case VDEC_PLAY_BACK_GET_FRAME:
            {
                s32Ret = vdec_tsk_GetVideoFrmaeMeth(vdecChn, voDev, needFree, &pDstFrame);
                if (s32Ret != SAL_SOK)
                {
                    VDEC_LOGE("vdecChn %d,get frame s32Ret 0x%x error\n", vdecChn, s32Ret);
                    if (status)
                    {
                        *status = 1;
                    }

                    break;
                }

                copyEn.copyth = 0;
                copyEn.copyMeth = VDEC_DATA_COPY;
                s32Ret = vdec_hal_VdecCpyframe(&copyEn, (void *)pstFrame->uiAppData, (void *)pDstFrame.uiAppData);
                if (s32Ret != SAL_SOK)
                {
                    VDEC_LOGE("vdecChn %d,cpy frame error ret 0x%x\n", vdecChn, s32Ret);
                    if (status)
                    {
                        *status = 1;
                    }
                }

                if (*needFree == 1)
                {
                    s32Ret = vdec_tsk_PutVideoFrmaeMeth(vdecChn, voDev, &pDstFrame);
                    if (s32Ret != SAL_SOK)
                    {
                        VDEC_LOGE("vdecChn %d,put frame error ret 0x%x\n", vdecChn, s32Ret);
                    }

                    *needFree = 0;
                }

                break;
            }
            default:
            {
                break;
            }
        }
    }
    else if (pDecModCtrl->chnStatus == VDEC_STATE_PAUSE)
    {
        if (purpose == VDEC_DISP_GET_FRAME)
        {
            if (pVdecTskCtl->isCopyOver == 0)
            {
                if (!status)
                {
                    VDEC_LOGE(" err status null\n");
                }

                if (stPauseFrameBuff[vdecChn].u32NeedDrawOnce == 1)
                {
                    if (status)
                    {
                        *status = 0;
                    }

                    stPauseFrameBuff[vdecChn].u32NeedDrawOnce = 0;
                }
                else
                {
                    if (status)
                    {
                        *status = 2;
                    }
                }

                usleep(10 * 1000);

                s32Ret = vdec_tsk_GetVdecFrameForPaus(vdecChn, VDEC_DISP_GET_FRAME, pstFrame);
                goto end;
            }
        }
        else if (purpose == VDEC_JPEG_GET_FRAME)
        {
            /*超过8倍速度暂停会强制暂停存在无法获取暂停帧*/
            if (pVdecTskCtl->isCopyOver == 0)
            {
                s32Ret = vdec_tsk_GetVdecFrameForPaus(vdecChn, VDEC_JPEG_GET_FRAME, pstFrame);
                goto end;
            }
        }
    }
    else if (pDecModCtrl->chnStatus == VDEC_STATE_STOPPED)
    {
        usleep(20 * 1000);
        if (status)
        {
            *status = 1;
        }

        /*若人脸模块和行为分析模块在未开启当前解码通道时获取解码帧，则不获取视频信号，直接返回失败*/
        if(VDEC_BA_GET_FRAME == purpose || VDEC_FACE_GET_FRAME == purpose)
        {
            s32Ret = SAL_FAIL;
            goto end;

        }

        /*关闭解码后只获取无视频信号*/
        s32Ret = vdec_tsk_GetNoSignalPic(vdecChn, 1, pstFrame);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("chn %d get no signal failed\n", vdecChn);
        }

        goto end;
    }

end:
    SAL_mutexUnlock(pVdecTskCtl->mutex);
    if (s32Ret != SAL_SOK)
    {
        usleep(10 * 1000);
    }
    else 
    {
        if (*needFree == 1)
        {
            if ((int)pVdecTskCtl->unReleaseFrameCnt < 0)
            {
                VDEC_LOGW("cur chn %d un releasecnt %d err \n",vdecChn,pVdecTskCtl->unReleaseFrameCnt);
                pVdecTskCtl->unReleaseFrameCnt = 0;
            }
            pVdecTskCtl->unReleaseFrameCnt++;
        }
    }

    return s32Ret;
}

/**
 * @function   vdec_tsk_GetChnStatus
 * @brief     获取当前解码器通道状态
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  UINT32 *pStatus 状态值
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_GetChnStatus(UINT32 u32VdecChn, UINT32 *pStatus)
{
    DEC_MOD_CTRL *pDecCtrl = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));

    if (NULL == pStatus)
    {
        VDEC_LOGE("chan %d error\n", u32VdecChn);
        return SAL_FAIL;
    }

    pDecCtrl = &g_stVdecObjCommon.stObjTsk[u32VdecChn].decModCtrl;
    *pStatus = pDecCtrl->chnStatus;

    return SAL_SOK;
}

/**
 * @function   vdec_tsk_DemuxPlayBackProcess
 * @brief     倒放数据帧压栈处理
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  DEM_BUFFER *pDemBuf 数据帧信息
 * @param[out] None
 * @return     static INT32
 */
static INT32 vdec_tsk_DemuxPlayBackProcess(UINT32 u32VdecChn, DEM_BUFFER *pDemBuf)
{
    INT32 s32Ret = 0;
    struct DEC_Stack *pPBGroup = NULL;
    struct DEC_Stack *pInGroup = NULL;
    struct DEC_Stack *pOutGroup = NULL;
    struct DEC_List *p = NULL;
    SYSTEM_FRAME_INFO *pFrameOut = NULL;
    UINT32 u32CheckCnt = 0;
    UINT32 u32IsFull = 0;
    UINT32 needFree = 0;
    SYSTEM_FRAME_INFO pDstFrame;
    VDEC_PIC_COPY_EN copyEn;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(pDemBuf, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    do
    {
        u32IsFull = 0;
        /* 两个栈都满了就丢掉 */
        if ((gPbGroup1[u32VdecChn].top >= MAX_PLAYGROUP_FRAME_NUM) && (gPbGroup2[u32VdecChn].top >= MAX_PLAYGROUP_FRAME_NUM))
        {
            gInPingPangId[u32VdecChn] = 0;
            VDEC_LOGW("statck full id %d,%d\n", gInPingPangId[u32VdecChn], gOutPingPangId[u32VdecChn]);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM);

        }

        if (gInPingPangId[u32VdecChn] > 1)
        {
            gInPingPangId[u32VdecChn] = 0;
        }

        pPBGroup = (0 == gInPingPangId[u32VdecChn]) ? &gPbGroup1[u32VdecChn] : &gPbGroup2[u32VdecChn];
        if (pPBGroup->top >= MAX_PLAYGROUP_FRAME_NUM || (pDemBuf->isKeyFrame && pPBGroup->top > 0))
        {
            pPBGroup->statue = 1;
            usleep(2 * 1000);
            gInPingPangId[u32VdecChn] = (gInPingPangId[u32VdecChn] == 0) ? 1 : 0;
            u32CheckCnt++;
            if (u32CheckCnt > 10)
            {
                VDEC_LOGW("id %d,%d\n", gInPingPangId[u32VdecChn], gOutPingPangId[u32VdecChn]);
            }

            u32IsFull = 1;
        }
        else if (pPBGroup->top <= 0)
        {
            pPBGroup->statue = 0;
        }
    }
    while (u32IsFull);

    u32CheckCnt = 0;

    while (1)
    {
        if (gInPingPangId[u32VdecChn] != gOutPingPangId[u32VdecChn])
        {
            pInGroup = (0 == gInPingPangId[u32VdecChn]) ? &gPbGroup1[u32VdecChn] : &gPbGroup2[u32VdecChn];
            pOutGroup = (0 == gOutPingPangId[u32VdecChn]) ? &gPbGroup1[u32VdecChn] : &gPbGroup2[u32VdecChn];
            /* 入栈比出栈块则入栈丢帧 */
            if ((pInGroup->top >= MAX_PLAYGROUP_FRAME_NUM / 2) && (pOutGroup->top >= MAX_PLAYGROUP_FRAME_NUM / 2))
            {
                VDEC_LOGW("u32VdecChn %d,Inid:%d,OutId:%d,InTop:%d,OutTop:%d\n", u32VdecChn, gInPingPangId[u32VdecChn], gOutPingPangId[u32VdecChn], pInGroup->top, pOutGroup->top);
                continue;
            }
        }
        else
        {
            if ((gPbGroup1[u32VdecChn].top > 0) && (gPbGroup2[u32VdecChn].top > 0))
            {
                u32CheckCnt++;
                if (u32CheckCnt > 100)
                {
                    VDEC_LOGW("id %d,%d\n", gInPingPangId[u32VdecChn], gOutPingPangId[u32VdecChn]);
                }

                usleep(10 * 1000);
                continue;
            }
        }

        u32CheckCnt = 0;
        break;
    }

    if ((pPBGroup->w > 0 && pPBGroup->h > 0) && ((pPBGroup->w != pDemBuf->width) || (pPBGroup->h != pDemBuf->height)))
    {
        gPbGroup1[u32VdecChn].top = 0;
        gPbGroup2[u32VdecChn].top = 0;
        vdec_tsk_PutAllPbPlatMem(u32VdecChn);
    }

    pPBGroup->w = pDemBuf->width;
    pPBGroup->h = pDemBuf->height;
    p = pPBGroup->playFrame + pPBGroup->top;

    if (0x00 == p->stFrame.uiAppData)
    {
        s32Ret = vdec_tsk_GetPbPlatMem(u32VdecChn, pPBGroup->w, pPBGroup->h, &p->stFrame);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("u32VdecChn %d get mem errro ret 0x%x\n", u32VdecChn, s32Ret);
            return s32Ret;
        }
    }

    pFrameOut = &p->stFrame;
    s32Ret = vdec_tsk_GetVideoFrmaeMeth(u32VdecChn, 0, &needFree, &pDstFrame);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d get vdec frame err ret 0x%x\n", u32VdecChn, s32Ret);
    }

    copyEn.copyth = 0;
    copyEn.copyMeth = VDEC_DATA_COPY;
    s32Ret = vdec_hal_VdecCpyframe(&copyEn, (void *)pDstFrame.uiAppData, (void *)pFrameOut->uiAppData);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d  copy err ret 0x%x\n", u32VdecChn, s32Ret);
    }

    if (needFree == 1)
    {
        s32Ret = vdec_tsk_PutVideoFrmaeMeth(u32VdecChn, 0, &pDstFrame);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("chn %d put vdec frame err ret 0x%x\n", u32VdecChn, s32Ret);
            return s32Ret;
        }
    }

    s32Ret = vdec_tsk_StackIn(pPBGroup);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chan %d stack in errro ret 0x%x id %d\n", u32VdecChn, s32Ret, gInPingPangId[u32VdecChn]);
        return s32Ret;
    }

    return SAL_SOK;
}

/**
 * @function    vdec_tsk_DemuxisKeyFrame
 * @brief      是否是关键帧
 * @param[in]   UINT8 *addr 数据地址
 * @param[out] None
 * @return  static INT32
 */
static INT32 vdec_tsk_DemuxisKeyFrame(UINT8 *addr)
{
    if ((*(addr + 4) == 0x67) || ((*(addr + 4) == 0x09) && (*(addr + 5) == 0x10)) || (*(addr + 4) == 0x40)
        || ((*(addr + 4) & 0x1f) == 0x07)) /* 大华有的IPC(比如IPC-HFW4833F-ZAS)SPS对应的BYTE是0x27*/
    {
        return SAL_TRUE;
    }

    return SAL_FALSE;
}

/**
 * @function    vdec_tsk_SetDemuxType
 * @brief      设置解复用器类型
 * @param[in] UINT32 u32VdecChn 通道号
 * @param[in] UINT32 u32Type类型
 * @param[in] UINT32 u32SrcEncType 码流编码类型
 * @param[out] None
 * @return  static INT32
 */
static INT32 vdec_tsk_SetDemuxType(UINT32 u32VdecChn, UINT32 u32Type, UINT32 u32SrcEncType)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32DemuxChan = u32VdecChn;
    UINT32 u32DemuxType = u32Type;
    UINT32 u32EncType = u32SrcEncType;

    s32Ret = DemuxControl(DEM_SET_TYPE, &u32DemuxChan, &u32DemuxType);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("u32VdecChn %d set demux type ret 0x%x error\n", u32VdecChn, s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_UNSUPPORTED_PARAM);
    }

    s32Ret = DemuxControl(DEM_SET_ENC_TYPE, &u32DemuxChan, &u32EncType);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("u32VdecChn %d set dec type error ret 0x%x\n", u32VdecChn, s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_UNSUPPORTED_PARAM);
    }

    return s32Ret;
}

/**
 * @function    vdec_tsk_DemuxInit
 * @brief     解复用器初始化
 * @param[in] DEM_TYPE demType 解复用器类型
 * @param[in] UINT8 *pAddr 提供解复用器使用的内存
 * @param[in] UINT32 u32Length 内存长度
 * @param[in] int *pDeMuxHandle 返回解复用器handle
 * @param[out] None
 * @return  static INT32
 */
static INT32 vdec_tsk_DemuxInit(DEM_TYPE demType, UINT8 *pAddr, UINT32 u32Length, int *pDeMuxHandle)
{
    INT32 s32Ret = SAL_SOK;
    DEM_PARAM demPrm;
    DEM_ORGBUF_PRM orgBufPrm;
    DEM_PARAM *pDemPrm = &demPrm;
    /* pthread_t       task; */

    DEM_CHN_PRM demChnPrm;

    CHECK_PTR_NULL(pAddr, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pDeMuxHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    demChnPrm.demType = demType;
    demChnPrm.outType = DEM_VIDEO_OUT | DEM_AUDIO_OUT;

    /******************************************************************************************/
    s32Ret = DemuxControl(DEM_GET_HANDLE, &demChnPrm, pDemPrm);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("get  demux handle err s32Ret 0x%x demhdl %d\n", s32Ret, pDemPrm->demHdl);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
    }

    pDemPrm->bufAddr = (unsigned char *)vdec_tsk_MallocOS(pDemPrm->bufLen);
    if (NULL == pDemPrm->bufAddr)
    {
        VDEC_LOGE("demMem %p malloc memSz %d failed\n", pDemPrm->bufAddr, pDemPrm->bufLen);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_MEM_ALLOC);
    }

    memset(pDemPrm->bufAddr, 0x0, pDemPrm->bufLen);
    s32Ret = DemuxControl(DEM_CREATE, pDemPrm, NULL);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("create demux handle err s32Ret 0x%x\n", s32Ret);
        vdec_tsk_FreeOS((void **)&pDemPrm->bufAddr);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_INIT);
    }

    orgBufPrm.orgBuf = pAddr;
    orgBufPrm.orgbuflen = u32Length;
    s32Ret = DemuxControl(DEM_SET_ORGBUF, &pDemPrm->demHdl, &orgBufPrm);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("DEM_SET_ORGBUF s32Ret 0x%x\n", s32Ret);
        vdec_tsk_FreeOS((void **)&pDemPrm->bufAddr);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_CONFIG);
    }

    *pDeMuxHandle = pDemPrm->demHdl;

    return s32Ret;
}

/**
 * @function    vdec_tskDemuxCreate
 * @brief     解复用器创建
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return  static INT32
 */
static INT32 vdec_tskDemuxCreate(UINT32 u32VdecChn)
{
    INT32 s32Ret = SAL_SOK;
    SAL_ThrHndl task;
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    INT32 *pHandle = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));

    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];

    /* 初始化ps包 */
    pHandle = &pVdecTskChn->decModCtrl.psHandle;
    s32Ret = vdec_tsk_DemuxInit(DEM_PS, pVdecTskChn->vdec_chn.decBufAddr, pVdecTskChn->vdec_chn.decBufLen, pHandle);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("Demux_Init error ret 0x%x\n", s32Ret);
        return s32Ret;
    }

    /* 初始化rtp包 */
    pHandle = &pVdecTskChn->decModCtrl.rtpHandle;
    s32Ret = vdec_tsk_DemuxInit(DEM_RTP, pVdecTskChn->vdec_chn.decBufAddr, pVdecTskChn->vdec_chn.decBufLen, pHandle);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("Demux_Init error ret 0x%x\n", s32Ret);
        return s32Ret;
    }

    SAL_mutexCreate(SAL_MUTEX_NORMAL, &pVdecTskChn->decModCtrl.mutexHdl);
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &pVdecTskChn->decModCtrl.pChnStatMuxHdl);

    SAL_mutexCreate(SAL_MUTEX_NORMAL, &g_stVecTaskCtl[u32VdecChn].mutex);
    pVdecTskChn->decModCtrl.enable = SAL_FALSE;
    pVdecTskChn->decModCtrl.chnStatus = VDEC_STATE_PREPARING;

    SAL_thrCreate(&task, vdec_tsk_DemuxThread, SAL_THR_PRI_DEFAULT, 0, pVdecTskChn);

    return s32Ret;
}


/**
 * @function    vdec_tsk_ModuleCreate
 * @brief     解码模块创建
 * @param[in]
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_ModuleCreate()
{
    INT32 s32Ret = SAL_SOK;
    INT32 i = 0;
    UINT32 u32FaceVdecCnt = 0;
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    DSPINITPARA *pstDspInfo = NULL;
    
    pstDspInfo = SystemPrm_getDspInitPara();
    CHECK_PTR_NULL(pstDspInfo, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

	/* 安检机产品中最后一个解码通道用于人脸jpeg解码使用 */
    if (pstDspInfo->dspCapbPar.dev_tpye == PRODUCT_TYPE_ISM
		&& SAL_TRUE == IA_GetModEnableFlag(IA_MOD_FACE))
    {
        u32FaceVdecCnt = 1;  /* 目前从片只开放一个图片解码通道，未使用当前人脸jpeg仅使用软解 */
    }

    VDEC_LOGI("decChanCnt %d, MaxDecNum %d, uiJdecCnt %d \n", pstDspInfo->decChanCnt, MAX_VDEC_CHAN, u32FaceVdecCnt);

    if (0 == pstDspInfo->decChanCnt)           /* 不使用解码器 直接退出 */
    {
        VDEC_LOGI("not need vdec decChanCnt %d\n", pstDspInfo->decChanCnt);
        return s32Ret;
    }

    CHECK_CHAN((pstDspInfo->decChanCnt - 1), DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));

    for (i = 0; i < pstDspInfo->decChanCnt - u32FaceVdecCnt; i++)
    {
        pVdecTskChn = &g_stVdecObjCommon.stObjTsk[i];
        pVdecTskChn->u32VdecChn = i;
        pVdecTskChn->vdec_chn.decBufAddr = pstDspInfo->decShareBuf[i].pVirtAddr;
        pVdecTskChn->vdec_chn.decBufLen = pstDspInfo->decShareBuf[i].bufLen;
        pVdecTskChn->vdec_chn.maxWidth = MAX_VDEC_WIDTH;
        pVdecTskChn->vdec_chn.maxHeight = MAX_VDEC_HEIGHT;
        pVdecTskChn->vdec_chn.u32EncType = H264;     /* H264 */
        pVdecTskChn->vdec_chn.packType = DEM_PS;
        if (pVdecTskChn->decModCtrl.chnStatus > VDEC_STATE_UNPREPARED)
        {
            /*将其设置为VDEC_STATE_UNPREPARED*/
        }

        pVdecTskChn->decModCtrl.chnStatus = VDEC_STATE_UNPREPARED;
        pVdecTskChn->decModCtrl.dupBindType = stVdecDupInstCfg.stNodeCfg[IN_NODE_0].enBindType;

        s32Ret = sys_hal_allocVideoFrameInfoSt(&pVdecTskChn->decModCtrl.stFrameInfo);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("chn %d get frame mem failed ret 0x%x\n", i, s32Ret);
            return s32Ret;
        }

        /*3. 解包层初始化*/
        if (SAL_SOK != (s32Ret = vdec_tskDemuxCreate(i)))
        {
            VDEC_LOGE("Dec chn %d demux create error. ret 0x%x\n", i, s32Ret);
            return s32Ret;
        }

        if (SAL_SOK != (s32Ret = vca_unpack_init(i)))
        {
            VDEC_LOGE("Dec chn %d vca init error.ret 0x%x\n", i, s32Ret);
            return s32Ret;
        }

        if (SAL_SOK != (s32Ret = vdec_hal_Init(i)))
        {
            VDEC_LOGE("Dec chn %d hal init error.ret 0x%x\n", i, s32Ret);
            return s32Ret;
        }

        /*4. 解码器初始化
        if (SAL_SOK != Vdec_tskCreate(i))
        {
            VDEC_LOGE("Dec Init error.\n");
            return SAL_FAIL;
        }*/

        g_stVdecObjCommon.uiChnCnt++;
    }

	/* 安检机产品中最后一个解码通道用于人脸jpeg解码使用，最大处理分辨率为1080P */
    if (i < pstDspInfo->decChanCnt && i >= pstDspInfo->decChanCnt - u32FaceVdecCnt)
    {
        pVdecTskChn = &g_stVdecObjCommon.stObjTsk[i];
        pVdecTskChn->u32VdecChn = i;
        pVdecTskChn->vdec_chn.maxWidth = MAX_VDEC_WIDTH;
        pVdecTskChn->vdec_chn.maxHeight = MAX_VDEC_HEIGHT;
        pVdecTskChn->vdec_chn.u32EncType = MJPEG;    /* jpeg */

        if (pVdecTskChn->decModCtrl.chnStatus > VDEC_STATE_UNPREPARED)
        {
            /*将其设置为VDEC_STATE_UNPREPARED*/
        }

        pVdecTskChn->decModCtrl.chnStatus = VDEC_STATE_UNPREPARED;

        s32Ret = sys_hal_allocVideoFrameInfoSt(&pVdecTskChn->decModCtrl.stFrameInfo);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("chn %d get frame mem failed ret 0x%x\n", i, s32Ret);
            return s32Ret;
        }

        if (SAL_SOK != (s32Ret = vca_unpack_init(i)))
        {
            VDEC_LOGE("Dec chn %d vca init error.ret 0x%x\n", i, s32Ret);
            return s32Ret;
        }

        if (SAL_SOK != (s32Ret = vdec_hal_Init(i)))
        {
            VDEC_LOGE("Dec chn %d hal init error.ret 0x%x\n", i, s32Ret);
            return s32Ret;
        }
        
        g_stVdecObjCommon.uiChnCnt++;
    }

    VDEC_LOGI("Vdec Create Module End! \n");

    return s32Ret;
}

/**
 * @function    vdec_tsk_DemuxCtrl
 * @brief     解复用线程启动
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] UINT32 Ctrl 是否开启解复用线程
 * @param[out] None
 * @return  static INT32
 */
static INT32 vdec_tsk_DemuxCtrl(UINT32 u32VdecChn, UINT32 Ctrl)
{
    DEC_MOD_CTRL *pDecCtrl = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));

    pDecCtrl = &g_stVdecObjCommon.stObjTsk[u32VdecChn].decModCtrl;

    SAL_mutexLock(pDecCtrl->mutexHdl);
    pDecCtrl->enable = Ctrl;
    SAL_mutexSignal(pDecCtrl->mutexHdl);
    SAL_mutexUnlock(pDecCtrl->mutexHdl);

    return SAL_SOK;
}

/**
 * @function    vdec_tsk_DemuxWait
 * @brief     解复线程用等待
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[out] None
 * @return  static INT32
 */
static INT32 vdec_tsk_DemuxWait(UINT32 u32VdecChn)
{
    DEC_MOD_CTRL *pDecCtrl = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));

    pDecCtrl = &g_stVdecObjCommon.stObjTsk[u32VdecChn].decModCtrl;

    SAL_mutexLock(pDecCtrl->mutexHdl);

    if (SAL_FALSE == pDecCtrl->enable)
    {
        VDEC_LOGI("u32VdecChn %d thread wait\n", u32VdecChn);
        SAL_mutexWait(pDecCtrl->mutexHdl);
    }

    SAL_mutexUnlock(pDecCtrl->mutexHdl);

    return SAL_SOK;
}

/**
 * @function    vdec_tsk_DemuxStart
 * @brief     启动解复用器
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[out] None
 * @return  static INT32
 */
static INT32 vdec_tsk_DemuxStart(UINT32 u32VdecChn)
{
    INT32 s32Ret = 0;
    VDEC_TASK_CTL *pVdecCtl = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));

    pVdecCtl = &g_stVecTaskCtl[u32VdecChn];
    pVdecCtl->timestampLast = 0;
    pVdecCtl->timestampFirst = 0;

    vca_unpack_clearResult(u32VdecChn);

    pVdecCtl->isNewDec = 1;

    /*使能解码线程*/
    s32Ret = vdec_tsk_DemuxCtrl(u32VdecChn, SAL_TRUE);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("vdec chn %d enable error.ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    VDEC_LOGI("u32VdecChn %d start ctrl\n", u32VdecChn);

    return SAL_SOK;
}

/**
 * @function    vdec_tsk_Start
 * @brief     启动解码器
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_Start(UINT32 u32VdecChn, void *prm)
{
    INT32 s32Ret = SAL_SOK;
    INT8 s8Val = 0;
    INT32 s32WaitCnt = 0;
    INT32 i = 0;
    UINT32 u32EncType = 0;
    VDEC_STATUS enCurState = 0;
    DEC_MOD_CTRL *pDecModeCtrl = NULL;
    VDEC_TASK_CTL *pVdecTaskCtl = NULL;
    DECODER_PARAM *pstDecPrm = NULL;
    VDEC_TSK_CHN *pVdecTskChn = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(prm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pstDecPrm = (DECODER_PARAM *)prm;
    pVdecTaskCtl = &g_stVecTaskCtl[u32VdecChn];
    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];
    pDecModeCtrl = &pVdecTskChn->decModCtrl;

    enCurState = pDecModeCtrl->chnStatus;
    if (enCurState == VDEC_STATE_RUNNING)
    {
        return SAL_SOK;
    }

    /*从pause 直接start无需配置其他直接切换状态就好*/
    if (enCurState == VDEC_STATE_PAUSE)
    {
        pVdecTaskCtl->bUpdataDecTime = SAL_TRUE;
        pDecModeCtrl->chnStatus = VDEC_STATE_RUNNING;
        return SAL_SOK;
    }

    switch (pDecModeCtrl->chnStatus)
    {
        case VDEC_STATE_UNPREPARED:
        case VDEC_STATE_PREPARING:
        {
            while (pDecModeCtrl->chnStatus != VDEC_STATE_PREPARED)
            {
                usleep(10 * 1000);
                s32WaitCnt++;
                if (s32WaitCnt > 200)
                {
                    goto end;
                    s32Ret = DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_STATUS);
                    break;
                }
            }
        }
        case VDEC_STATE_PREPARED:
        case VDEC_STATE_STOPPED:
            break;
        default:
            s32Ret = DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_STATUS);
            goto end;

    }


    VDEC_LOGI("u32VdecChn %d state %d\n", u32VdecChn, pDecModeCtrl->chnStatus);

    pVdecTaskCtl->bUpdataDecTime = SAL_TRUE;

    if (MPEG2MUX_STREAM_TYPE_PS == pstDecPrm->muxType)
    {
        pVdecTskChn->vdec_chn.packType = DEM_PS;
        pDecModeCtrl->chnHandle = pDecModeCtrl->psHandle;
    }
    else if (MPEG2MUX_STREAM_TYPE_RTP == pstDecPrm->muxType)
    {
        pVdecTskChn->vdec_chn.packType = DEM_RTP;
        pDecModeCtrl->chnHandle = pDecModeCtrl->rtpHandle;
    }
    else
    {
        VDEC_LOGE("chn %d muxType %x error!\n", u32VdecChn, pstDecPrm->muxType);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
    }

    if (ENCODER_H264 == pstDecPrm->videoType || ENCODER_H265 == pstDecPrm->videoType)
    {
        u32EncType = ENCODER_H264 == pstDecPrm->videoType ? 1 : 2; /* 1表示h264，2表示h265 */
    }
    else if (ENCODER_UNKOWN == pstDecPrm->videoType)
    {
        u32EncType = 0; /* dsp 通过解包解析视频编码类型 */
    }
    else
    {
        VDEC_LOGE("chn %d videoType %x error!\n", u32VdecChn, pstDecPrm->videoType);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
    }

    pVdecTskChn->vdec_chn.audPlay = pstDecPrm->audioPlay;
    if (pstDecPrm->audioPlay)
    {
        (void)audio_tsk_decPreviewStart(u32VdecChn);
    }
    else
    {
        (void)audio_tsk_decPreviewStop(u32VdecChn);
    }

    if (pstDecPrm->audioSamplesPerSec > 0)
    {
        for (i = 0; i < 16; i++)
        {
            if (pstDecPrm->audioSamplesPerSec == g_AacAdtsHeaderSamplingFrequency[i])
            {
                s8Val = aacAdtsHeader[u32VdecChn][2];
                if (i != ((s8Val & 0x3c) >> 2))
                {
                    VDEC_LOGI("i %d,val %x_%d\n", i, ((s8Val & 0x3c) >> 2), ((s8Val & 0x3c) >> 2));
                    aacAdtsHeader[u32VdecChn][2] &= 0xc3; /* ~((1 << 2) | (1 << 3) | (1 << 4) | (1 << 5));//置为0 */
                    aacAdtsHeader[u32VdecChn][2] |= (i << 2);
                }

                s8Val = aacAdtsHeader[u32VdecChn][2];
                VDEC_LOGI("val %x_%d\n", ((s8Val & 0x3c) >> 2), s8Val);
                break;
            }
        }
    }

    s32Ret = vdec_tsk_SetDemuxType(pDecModeCtrl->chnHandle, pVdecTskChn->vdec_chn.packType, u32EncType);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d set demux type error!ret 0x%x\n", u32VdecChn, s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
    }

    s32Ret = vdec_tsk_DemuxStart(u32VdecChn);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d demux start error!ret 0x%x\n", u32VdecChn, s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_ENABLE);
    }


    pDecModeCtrl->chnStatus = VDEC_STATE_RUNNING;

    VDEC_LOGI("u32VdecChn %d muxType %d videoType %d audioPlay %d audioSamples %u,chnHandle %d start\n",
              u32VdecChn, pstDecPrm->muxType, pstDecPrm->videoType, pstDecPrm->audioPlay, pstDecPrm->audioSamplesPerSec, pDecModeCtrl->chnHandle);

end:
    if (s32Ret != SAL_SOK)
    {
        pDecModeCtrl->chnStatus = enCurState;
        VDEC_LOGE("vdec u32VdecChn %d start failed\n", u32VdecChn);
    }

    return s32Ret;
}

/**
 * @function    vdec_tsk_DemuxReset
 * @brief     解复用器复位
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_DemuxReset(UINT32 u32VdecChn)
{
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    INT32 s32Ret = SAL_SOK;
    VDEC_TASK_CTL *pVdecCtl = NULL;
    INT32 u32EncType = 0;
    INT32 *pHandle = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));

    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];

    if (pVdecTskChn->vdec_chn.packType == DEM_RTP)
    {
        pHandle = &pVdecTskChn->decModCtrl.rtpHandle;
    }
    else if (pVdecTskChn->vdec_chn.packType == DEM_PS)
    {
        pHandle = &pVdecTskChn->decModCtrl.psHandle;
    }

    DemuxControl(DEM_RESET, pHandle, (void *)&u32EncType);

    pVdecCtl = &g_stVecTaskCtl[u32VdecChn];
    pVdecCtl->u32PlaySpeed = DECODE_SPEED_1x;
    pVdecCtl->bUpdataDecTime = SAL_TRUE;
    pVdecCtl->syncPrm.mode = SYNC_MODE_NON;
    pVdecCtl->decodeAbsTime = 0;
    pVdecCtl->decodePlayBackStdTime = 0xffffffff;
    pVdecCtl->decodeStandardTime = 0;
    pVdecCtl->timestampLast = 0;
    pVdecCtl->timestampFirst = 0;
    pVdecCtl->frameNum = 0;
    pVdecCtl->cntDebug = 0;
    pVdecCtl->syncMasterAbsTimeMs = 0;
    pVdecCtl->syncMGlg = 0;
    pVdecCtl->syncMGlg_ms = 0;

    return s32Ret;
}

/**
 * @function    vdec_tsk_DemuxStop
 * @brief     解复用停止
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_DemuxStop(UINT32 u32VdecChn)
{
    INT32 s32Ret = SAL_SOK;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));

    /*暂停解码线程*/
    s32Ret = vdec_tsk_DemuxCtrl(u32VdecChn, SAL_FALSE);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("chn %d start demux error.ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    return SAL_SOK;
}

/**
 * @function    vdec_tsk_Stop
 * @brief    停止解码
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_Stop(UINT32 u32VdecChn, void *prm)
{
    INT32 s32Ret = SAL_SOK;
    INT32 s32WaitCnt = 0;
    DUP_BIND_PRM pSrcBuf;
    VDEC_STATUS enCurState = 0;
    VDEC_HAL_STATUS pvdecStatus;
    VDEC_BIND_CTL pVdecBindCtl;

    VDEC_TASK_CTL *pVdecTaskCtl = NULL;
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    DEC_MOD_CTRL *pDecModeCtrl = NULL;
    DECODEC_COVER_PICPRM *pVdecCpPrm = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(prm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecTaskCtl = &g_stVecTaskCtl[u32VdecChn];
    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];
    pDecModeCtrl = &pVdecTskChn->decModCtrl;
    pVdecTaskCtl->vdecMode = DECODE_MODE_BULL;

    pVdecCpPrm = (DECODEC_COVER_PICPRM *)prm;

    enCurState = pDecModeCtrl->chnStatus;
    if (enCurState == VDEC_STATE_STOPPED)
    {
        return s32Ret;
    }

    pVdecTaskCtl->u32VdecPrmReset = 1;
    switch (pDecModeCtrl->chnStatus)
    {
        case VDEC_STATE_RUNNING:
        {
            pDecModeCtrl->chnStatus = VDEC_STATE_PREPARING;
        }
        case VDEC_STATE_PREPARING:
        {
            while (pDecModeCtrl->chnStatus != VDEC_STATE_PREPARED)
            {
                usleep(10 * 1000);
                s32WaitCnt++;
                if (s32WaitCnt > 200)
                {
                    s32Ret = DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_STATUS);
                    goto end;
                }
            }
        }
        case VDEC_STATE_PAUSE:
        case VDEC_STATE_PREPARED:
            break;
        default:
            s32Ret = DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_STATUS);
            goto end;
    }

    s32Ret = vdec_tsk_DemuxStop(u32VdecChn);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d demux stop error!ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    if (NULL != outBmpInfo[u32VdecChn].cBmpAddr)
    {
        vdec_tsk_FreeOS(outBmpInfo[u32VdecChn].cBmpAddr);
        outBmpInfo[u32VdecChn].cBmpAddr = NULL;
    }

    if (SAL_SOK != (s32Ret = vdec_tsk_DemuxReset(u32VdecChn)))
    {
        VDEC_LOGE("u32VdecChn %d Reset failed ret 0x%x\n", u32VdecChn, s32Ret);
    }

    vdec_tsk_DeInitStack(&gPbGroup1[u32VdecChn], &gPbGroup2[u32VdecChn]);
    vdec_tsk_PutAllPbPlatMem(u32VdecChn);

    if (pVdecCpPrm->cover_sign == 1)
    {
        /* 使用应用配置索引图片 */
        s32Ret = vdec_tsk_GetVdecFrameForPausFromVdec(u32VdecChn, pVdecCpPrm->logo_pic_indx);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("u32VdecChn %d get pause frame for pause failed ret 0x%x\n", u32VdecChn, s32Ret);
        }
    }

    if (pVdecTaskCtl->u32VdecPrmReset == 1)
    {
        s32WaitCnt = 0;
        while (pVdecTaskCtl->u32VdecPrmReset)
        {
            usleep(10 * 1000);
            s32WaitCnt++;
            if (s32WaitCnt > 10)
            {
                VDEC_LOGE("vdec u32VdecChn %d reset vdec prm failed\n", u32VdecChn);
                pVdecTaskCtl->u32VdecPrmReset = 0;
                break;
            }
        }
    }

    pDecModeCtrl->chnStatus = VDEC_STATE_STOPPED;
    vdec_hal_VdecChnStatus(u32VdecChn, &pvdecStatus);
    vdec_hal_VdecGetBind(u32VdecChn, &pVdecBindCtl);

    if (pvdecStatus.bcreate == SAL_TRUE)
    {
        VDEC_LOGI("chn %d has create need deinit\n", u32VdecChn);
        if (pVdecBindCtl.needBind == 1)
        {
            if(pDecModeCtrl->dupBindType != NODE_BIND_TYPE_GET)
            {
                pSrcBuf.mod = SYSTEM_MOD_ID_VDEC;
                pSrcBuf.chn = u32VdecChn;
                if (SAL_SOK != (s32Ret = dup_task_bindToDup(&pSrcBuf, pVdecTskChn->decModCtrl.frontDupHandle, SAL_FALSE)))
                {
                    VDEC_LOGE("chn %d create handle failed ret 0x%x\n", u32VdecChn, s32Ret);
                }
            }
            dup_task_vdecDupDestroy(pVdecTskChn->decModCtrl.frontDupHandle, pVdecTskChn->decModCtrl.rearDupHandle, u32VdecChn);
        }

        if (SAL_SOK != (s32Ret = vdec_hal_VdecStop(u32VdecChn)))
        {
            VDEC_LOGE("chn %d stop failed ret 0x%x\n", u32VdecChn, s32Ret);
        }

        if (SAL_SOK != (s32Ret = vdec_hal_VdecDeinit(u32VdecChn)))
        {
            VDEC_LOGE("chn %d deinit failed ret 0x%x\n", u32VdecChn, s32Ret);
        }
    }

    pVdecTaskCtl->unReleaseFrameCnt = 0;

    VDEC_LOGI("u32VdecChn %d stop\n", u32VdecChn);
end:
    if (s32Ret != SAL_SOK)
    {
        pDecModeCtrl->chnStatus = enCurState;
        pVdecTaskCtl->u32VdecPrmReset = 0;
        VDEC_LOGE("vdec u32VdecChn %d stopped failed ret 0x%x\n", u32VdecChn, s32Ret);
    }

    return s32Ret;
}

/**
 * @function    vdec_tsk_Pause
 * @brief    解码暂停
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_Pause(UINT32 u32VdecChn, void *prm)
{
    INT32 s32Ret = SAL_SOK;
    INT32 s32WaitCnt = 0;
    VDEC_STATUS enCurState = 0;
    DEC_MOD_CTRL *pDecModCtrl = NULL;
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    VDEC_TASK_CTL *pVdecTaskCtl = NULL;
    DECODEC_COVER_PICPRM *pVdecCpPrm = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(prm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];
    pDecModCtrl = &pVdecTskChn->decModCtrl;
    pVdecTaskCtl = &g_stVecTaskCtl[u32VdecChn];
    pVdecCpPrm = (DECODEC_COVER_PICPRM *)prm;

    enCurState = pDecModCtrl->chnStatus;
    if (enCurState == VDEC_STATE_PAUSE)
    {
        return SAL_SOK;
    }

    if (enCurState == VDEC_STATE_PREPARING)
    {
        VDEC_LOGE("chn %d pause failed cur state %d\n", u32VdecChn, enCurState);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_STATUS);
    }

    switch (pDecModCtrl->chnStatus)
    {
        case VDEC_STATE_RUNNING:
        {
            pDecModCtrl->chnStatus = VDEC_STATE_PREPARING;
            VDEC_LOGI("u32VdecChn %d curstate %d\n", u32VdecChn, VDEC_STATE_RUNNING);
        }
        case VDEC_STATE_PREPARING:
        {
            VDEC_LOGI("u32VdecChn %d curstate %d\n", u32VdecChn, VDEC_STATE_PREPARING);
            while (pDecModCtrl->chnStatus != VDEC_STATE_PREPARED)
            {
                usleep(10 * 1000);
                s32WaitCnt++;
                if (s32WaitCnt > 200)
                {
                    VDEC_LOGE("chn %d copy state changing failed s32WaitCnt %d\n", u32VdecChn, s32WaitCnt);
                    s32Ret = DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_STATUS);
                    goto end;
                }
            }
        }
        case VDEC_STATE_PREPARED:
            break;
        default:
            s32Ret = DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_STATUS);
            goto end;
    }

    VDEC_LOGI("u32VdecChn %d curstate %d\n", u32VdecChn, pDecModCtrl->chnStatus);

    if (pVdecCpPrm->cover_sign == 1)
    {
        pDecModCtrl->chnStatus = VDEC_STATE_PAUSE;
        /* 使用应用配置索引图片 */
        s32Ret = vdec_tsk_GetVdecFrameForPausFromVdec(u32VdecChn, pVdecCpPrm->logo_pic_indx);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("u32VdecChn %d get pause frame for pause failed\n", u32VdecChn);
        }
    }
    else
    {
        pVdecTaskCtl->isCopyOver = 1;
        pDecModCtrl->chnStatus = VDEC_STATE_PAUSE;
        s32WaitCnt = 0;
        /* 等待拷贝完成 */
        while (pVdecTaskCtl->isCopyOver)
        {
            usleep(10 * 1000);
            s32WaitCnt++;
            if (s32WaitCnt > 200)
            {
                s32Ret = DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_TIMEOUT);
                VDEC_LOGE("chn %d copy timeout s32WaitCnt %d\n", u32VdecChn, s32WaitCnt);
                goto end;
            }
        }

        stPauseFrameBuff[u32VdecChn].u32NeedDrawOnce = 1;
    }

end:
    if (s32Ret != SAL_SOK)
    {
        if (pVdecTaskCtl->u32PlaySpeed < DECODE_SPEED_4x)
        {
            pDecModCtrl->chnStatus = VDEC_STATE_PAUSE;
            VDEC_LOGW("chn %d pause chnStatus = %d OK,get frame failed\n", u32VdecChn, pDecModCtrl->chnStatus);
            return SAL_SOK;
        }
        pDecModCtrl->chnStatus = enCurState;
        VDEC_LOGE("chn %d pause failed\n", u32VdecChn);
    }

    if (pVdecTaskCtl->isCopyOver != 0)
    {
        VDEC_LOGE("u32VdecChn %d copy u32Variable err %d\n", u32VdecChn, pVdecTaskCtl->isCopyOver);
        pVdecTaskCtl->isCopyOver = 0;
    }

    return s32Ret;
}

/**
 * @function    vdec_tsk_SetPrm
 * @brief    设置参数
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_SetPrm(UINT32 u32VdecChn, void *prm)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_TASK_CTL *pVdecTaskCtl = NULL;
    DECODER_ATTR *pVdecTaskAttr = NULL;
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    VDEC_STATUS enCurState = 0;
    UINT32 s32WaitCnt = 0;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(prm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];
    pVdecTaskCtl = &g_stVecTaskCtl[u32VdecChn];
    pVdecTaskAttr = (DECODER_ATTR *)prm;
    
    if ((pVdecTaskAttr->speed == pVdecTaskCtl->u32PlaySpeed) && (pVdecTaskAttr->vdecMode == pVdecTaskCtl->vdecMode))
    {
        VDEC_LOGI("VdecChn %d was speed %d,vdecMode %d, set speed %d vdecMode %d, prm is same\n", u32VdecChn, \
                                pVdecTaskCtl->u32PlaySpeed, pVdecTaskCtl->vdecMode, \
                                pVdecTaskAttr->speed, pVdecTaskAttr->vdecMode);
        return SAL_SOK;
    }
    
    SAL_mutexLock(pVdecTskChn->decModCtrl.pChnStatMuxHdl);

    enCurState = pVdecTskChn->decModCtrl.chnStatus;
    switch (pVdecTskChn->decModCtrl.chnStatus)
    {
        case VDEC_STATE_RUNNING:
        {
            pVdecTskChn->decModCtrl.chnStatus = VDEC_STATE_PREPARING;
        }
        case VDEC_STATE_PREPARING:
        {
            while (pVdecTskChn->decModCtrl.chnStatus != VDEC_STATE_PREPARED)
            {
                usleep(10 * 1000);
                s32WaitCnt++;
                if (s32WaitCnt > 200)
                {
                    s32Ret = DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_STATUS);
                    VDEC_LOGE("u32VdecChn %d state %d enCurState %d time out\n", u32VdecChn, pVdecTskChn->decModCtrl.chnStatus, enCurState);
                    goto end;
                }
            }
        }
        case VDEC_STATE_PAUSE:
        case VDEC_STATE_STOPPED:
        case VDEC_STATE_PREPARED:
            break;
        default:
        {
            s32Ret = DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_STATUS);
            VDEC_LOGE("u32VdecChn %d state %d enCurState %d err\n", u32VdecChn, pVdecTskChn->decModCtrl.chnStatus, enCurState);
            goto end;
        }
    }

    pVdecTaskCtl->u32PlaySpeed = pVdecTaskAttr->speed;
    pVdecTaskCtl->bUpdataDecTime = SAL_TRUE;

    if (pVdecTaskCtl->vdecMode != pVdecTaskAttr->vdecMode)
    {
        pVdecTaskCtl->decodeAbsTime = 0;
        pVdecTaskCtl->decodePlayBackStdTime = 0xffffffff; /* SAL_getTimeOfJiffies(); */
        pVdecTaskCtl->decodeStandardTime = 0;   /* SAL_getTimeOfJiffies(); */
        pVdecTaskCtl->frameNum = 0;

        VDEC_LOGI("pbT %u,dst %u\n", pVdecTaskCtl->decodePlayBackStdTime, pVdecTaskCtl->decodeStandardTime);
    }

    if ((DECODE_MODE_ONLY_I_FREAM == pVdecTaskAttr->vdecMode) || (DECODE_MODE_REVERSE == pVdecTaskAttr->vdecMode)
        || (DECODE_MODE_NORMAL == pVdecTaskAttr->vdecMode) || (DECODE_MODE_DRAG_PLAY == pVdecTaskAttr->vdecMode))
    {
        pVdecTaskCtl->u32PlaySpeed = DECODE_SPEED_1x;
    }

    if (DECODE_MODE_REVERSE == pVdecTaskAttr->vdecMode)
    {
        vdec_tsk_InitStack(&gPbGroup1[u32VdecChn], &gPbGroup2[u32VdecChn]);
    }

    if (DECODE_MODE_I_TO_BMP == pVdecTaskAttr->vdecMode)
    {
        if (outBmpInfo[u32VdecChn].cBmpAddr == NULL)
        {
            outBmpInfo[u32VdecChn].cBmpAddr = vdec_tsk_MallocOS(1024 * 1024 * 10);
            VDEC_LOGI("chn %d i 2 BMP malloc success\n", u32VdecChn);
        }
    }

    if (DECODE_MODE_DRAG_PLAY != pVdecTaskAttr->vdecMode && pVdecTaskCtl->vdecMode == DECODE_MODE_DRAG_PLAY)
    {
        pVdecTaskCtl->timestampLast = 0;
        pVdecTaskCtl->timestampFirst = 0;
        VDEC_LOGI("u32VdecChn %d,reset time %d, %d\n", u32VdecChn, pVdecTaskCtl->timestampLast, pVdecTaskCtl->timestampFirst);
    }

    pVdecTaskCtl->vdecMode = pVdecTaskAttr->vdecMode;
    
    VDEC_LOGI("Host_Decpeed u32VdecChn:%d,speed %d,vdecMode %d..[%d,%d]\n", u32VdecChn, pVdecTaskAttr->speed, pVdecTaskAttr->vdecMode, pVdecTaskCtl->u32PlaySpeed, pVdecTaskCtl->vdecMode);
end:
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("vdec u32VdecChn %d setPrm failed ret 0x%x\n", u32VdecChn, s32Ret);
    }

    pVdecTskChn->decModCtrl.chnStatus = enCurState;
    SAL_mutexUnlock(pVdecTskChn->decModCtrl.pChnStatMuxHdl);

    return s32Ret;
}

/**
 * @function    vdec_tsk_SetSynPrm
 * @brief    设置同步参数
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_SetSynPrm(UINT32 u32VdecChn, void *prm)
{
    VDEC_TASK_CTL *pVdecTaskCtl = NULL;
    DECODER_SYNC_PRM *pVdecSynPrm = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(prm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecTaskCtl = &g_stVecTaskCtl[u32VdecChn];
    pVdecSynPrm = (DECODER_SYNC_PRM *)prm;
    pVdecTaskCtl->syncPrm.mode = pVdecSynPrm->mode;
    pVdecTaskCtl->syncPrm.syncMasterChanId = pVdecSynPrm->syncMasterChanId;
    pVdecTaskCtl->bUpdataDecTime = SAL_TRUE;
    pVdecTaskCtl->bFastestMode = SAL_FALSE;

    if (SYNC_MODE_SLAVE != pVdecTaskCtl->syncPrm.mode)
    {
        pVdecTaskCtl->skipVdec = SAL_FALSE;
        pVdecTaskCtl->bigDiff = 0;
    }

    VDEC_LOGI("u32VdecChn %d,mode %d, id %d\n", u32VdecChn, pVdecTaskCtl->syncPrm.mode, pVdecTaskCtl->syncPrm.syncMasterChanId);

    return SAL_SOK;
}

/**
 * @function    vdec_tsk_ReSet
 * @brief    解码复位
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_ReSet(UINT32 u32VdecChn)
{
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    DEC_SHARE_BUF *pDecShare = NULL;
    DSPINITPARA *pstDspInfo = NULL;
    UINT32 u32VencType = 0;
    INT32 s32Ret = 0;
    VDEC_STATUS enCurState = 0;
    UINT32 s32WaitCnt = 0;
    INT32 *pHandle = NULL;
    VDEC_TASK_CTL *pVdecTaskCtl = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    pstDspInfo = SystemPrm_getDspInitPara();
    CHECK_PTR_NULL(pstDspInfo, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecTaskCtl = &g_stVecTaskCtl[u32VdecChn];
    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];

    enCurState = pVdecTskChn->decModCtrl.chnStatus;
    pVdecTaskCtl->u32VdecPrmReset = 1;

    switch (pVdecTskChn->decModCtrl.chnStatus)
    {
        case VDEC_STATE_RUNNING:
        {
            pVdecTskChn->decModCtrl.chnStatus = VDEC_STATE_PREPARING;
        }
        case VDEC_STATE_PREPARING:
        {
            while (pVdecTskChn->decModCtrl.chnStatus != VDEC_STATE_PREPARED)
            {
                SAL_msleep(10);
                s32WaitCnt++;
                if (s32WaitCnt > 200)
                {
                    VDEC_LOGE("u32VdecChn %d state %d time out\n", u32VdecChn, pVdecTskChn->decModCtrl.chnStatus);
                    s32Ret = DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_STATUS);
                    goto end;
                }
            }
            break;
        }
        case VDEC_STATE_STOPPED:
        case VDEC_STATE_PAUSE:
        case VDEC_STATE_PREPARED:
            break;
        default:
            s32Ret = DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_STATUS);
            goto end;
    }

    if (pVdecTskChn->vdec_chn.packType == DEM_RTP)
    {
        pHandle = &pVdecTskChn->decModCtrl.rtpHandle;
    }
    else if (pVdecTskChn->vdec_chn.packType == DEM_PS)
    {
        pHandle = &pVdecTskChn->decModCtrl.psHandle;
    }

    DemuxControl(DEM_RESET, pHandle, (void *)&u32VencType);

    pDecShare = &pstDspInfo->decShareBuf[u32VdecChn];
    pDecShare->readIdx = 0;
    pDecShare->writeIdx = 0;

    if (pVdecTaskCtl->u32VdecPrmReset == 1)
    {
        s32WaitCnt = 0;
        while (pVdecTaskCtl->u32VdecPrmReset)
        {
            usleep(10 * 1000);
            s32WaitCnt++;
            if (s32WaitCnt > 10)
            {
                VDEC_LOGE("vdec u32VdecChn %d reset vdec prm failed\n", u32VdecChn);
                pVdecTaskCtl->u32VdecPrmReset = 0;
                break;
            }
        }
    }

    VDEC_LOGI("Host_DecReSet u32VdecChn:%d\n", u32VdecChn);
end:
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("vdec u32VdecChn %d setPrm failed ret 0x%x\n", u32VdecChn, s32Ret);
        pVdecTaskCtl->u32VdecPrmReset = 0;
    }

    pVdecTskChn->decModCtrl.chnStatus = enCurState;

    return s32Ret;
}

/**
 * @function    vdec_tsk_AudioEnable
 * @brief    启动音频解码
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_AudioEnable(UINT32 u32VdecChn, void *prm)
{
    INT32 s32Ret = SAL_SOK;
    INT32 i = 0;
    INT8 u8Val = 0;
    DECODER_PARAM *pstDecPrm = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(prm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pstDecPrm = (DECODER_PARAM *)prm;

    g_stVdecObjCommon.stObjTsk[u32VdecChn].vdec_chn.audPlay = pstDecPrm->audioPlay;
    if (pstDecPrm->audioPlay)
    {
        (void)audio_tsk_decPreviewStart(u32VdecChn);
    }
    else
    {
        (void)audio_tsk_decPreviewStop(u32VdecChn);
    }

    if (pstDecPrm->audioSamplesPerSec > 0)
    {
        for (i = 0; i < 16; i++)
        {
            if (pstDecPrm->audioSamplesPerSec == g_AacAdtsHeaderSamplingFrequency[i])
            {
                u8Val = aacAdtsHeader[u32VdecChn][2];
                if (i != ((u8Val & 0x3c) >> 2))
                {
                    VDEC_LOGI("i %d,val %x_%d\n", i, ((u8Val & 0x3c) >> 2), ((u8Val & 0x3c) >> 2));
                    aacAdtsHeader[u32VdecChn][2] &= ~((1 << 2) | (1 << 3) | (1 << 4) | (1 << 5)); /* 置为0 */
                    aacAdtsHeader[u32VdecChn][2] |= (i << 2);
                }

                u8Val = aacAdtsHeader[u32VdecChn][2];
                VDEC_LOGI("val %x_%d\n", ((u8Val & 0x3c) >> 2), u8Val);
                break;
            }
        }
    }

    VDEC_LOGI("u32VdecChn %d audioPlay %d audioType %d,audioSamplesPerSec %d,i %d\n", u32VdecChn, pstDecPrm->audioPlay, pstDecPrm->audioType, pstDecPrm->audioSamplesPerSec, i);

    return s32Ret;
}

/**
 * @function    vdec_tsk_GetProc
 * @brief    proc信息查看
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_GetProc(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    CHECK_PTR_NULL(prm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    return s32Ret;
}

/**
 * @function    vdec_tsk_PlayGetThread
 * @brief    播放速度 延时
 * @param[in] UINT32 u32shift 倍速
 * @param[out] None
 * @return  static INT32
 */
static UINT32 vdec_tsk_PlayGetThread(UINT32 u32shift)
{
    INT32 s32thread = 0;

    if (u32shift == DECODE_SPEED_MAX)
    {
        s32thread = 128000 * 1.5;
    }
    else if (u32shift == DECODE_SPEED_256x)
    {
        s32thread = 64000 * 1.5;
    }
    else if (u32shift == DECODE_SPEED_128x)
    {
        s32thread = 32000 * 1.5;
    }
    else if (u32shift == DECODE_SPEED_64x)
    {
        s32thread = 16000 * 1.5;
    }
    else if (u32shift == DECODE_SPEED_32x)
    {

        s32thread = 8000 * 1.5;
    }
    else if (u32shift == DECODE_SPEED_16x)
    {

        s32thread = 4000 * 1.5;
    }
    else if (u32shift == DECODE_SPEED_8x)
    {

        s32thread = 2000 * 1.5;
    }
    else if (u32shift == DECODE_SPEED_4x)
    {

        s32thread = 1000;
    }
    else
    {
        s32thread = 1000;
    }

    return s32thread;
}

/**
 * @function    vdec_tsk_DemuxSpeedCtrlCalc
 * @brief    播放速度 校验控制
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] VDEC_TASK_CTL *pVdecCtl 解码通道相关时间戳
 * @param[in] INT32 threshold 是否需要自适应调整
 * @param[out] None
 * @return  static INT32
 */
static INT32 vdec_tsk_DemuxSpeedCtrlCalc(UINT32 u32VdecChn, VDEC_TASK_CTL *pVdecCtl, INT32 threshold)
{
    UINT64 u64CurMsTime = 0;
    INT32 s32DistSysTime = 0;     /* 当前系统时间与上次输出视频时系统时间的时间差 */
    INT32 s32DistFrmTime = 0;     /* 当前帧与上一帧输出视频帧的时间差 */
    INT32 s32Delay = 0;
    UINT32 u32Sleeptime = 0;
    UINT32 u32DelayThreshold = 0;
    DEC_MOD_CTRL *pDecCtrl = NULL;
    INT32 s32AdjustTime = 0;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(pVdecCtl, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    if (pVdecCtl->curFrmTimeTru < pVdecCtl->lastFrmTimeTru)
    {
        pVdecCtl->bUpdataDecTime = SAL_TRUE;

    }

    pDecCtrl = &g_stVdecObjCommon.stObjTsk[u32VdecChn].decModCtrl;

    pVdecCtl->lastFrmTimeTru = pVdecCtl->curFrmTimeTru;

    s32AdjustTime = (threshold == -3) ? pVdecCtl->adjustTime : 0;

    do
    {
        u64CurMsTime = vdec_tsk_getTimeMilli();
        if (pVdecCtl->bUpdataDecTime)
        {
            pVdecCtl->baseTime = pVdecCtl->curFrmTimeTru;
            pVdecCtl->checkTime = u64CurMsTime;
            pVdecCtl->bUpdataDecTime = SAL_FALSE;
            VDEC_LOGD("u32VdecChn %d,ft:%u,ct:%llu\n", u32VdecChn, pVdecCtl->curFrmTimeTru, u64CurMsTime);
        }

        s32DistSysTime = (INT32)(u64CurMsTime - pVdecCtl->checkTime);
        s32DistFrmTime = (INT32)(pVdecCtl->curFrmTimeTru - pVdecCtl->baseTime);
        if (DECODE_SPEED_1x == pVdecCtl->u32PlaySpeed)
        {
            s32Delay = s32DistSysTime - s32DistFrmTime + s32AdjustTime;
            u32DelayThreshold = 16000;
        }
        else if (DECODE_SPEED_1x > pVdecCtl->u32PlaySpeed)
        {
            s32Delay = (s32DistSysTime << (DECODE_SPEED_1x - pVdecCtl->u32PlaySpeed)) - s32DistFrmTime + s32AdjustTime;
            u32DelayThreshold = 16000;
        }
        else
        {
            s32Delay = (s32DistSysTime >> (pVdecCtl->u32PlaySpeed - DECODE_SPEED_1x)) - s32DistFrmTime + s32AdjustTime;
            u32DelayThreshold = 16000;
        }

        if ((pVdecCtl->u32PlaySpeed >= DECODE_SPEED_8x) && (VDEC_ABS(s32Delay) > u32DelayThreshold)) /*做个保护，大于16s的话，直接更新时间，防止时标异常后卡主*/
        {
            pVdecCtl->baseTime = pVdecCtl->curFrmTimeTru;
            pVdecCtl->checkTime = u64CurMsTime;
        }

        if (threshold == -3)
        {

            VDEC_LOGD("chn:%2d [dST:%d cT:%llu ckT:%llu] [fT:%u bT:%u dFT:%d] s32Delay:%d\n",
                      u32VdecChn, s32DistSysTime, u64CurMsTime, pVdecCtl->checkTime, pVdecCtl->curFrmTimeTru, pVdecCtl->baseTime, s32DistFrmTime, s32Delay);
        }

        if (s32Delay < threshold)
        {
            u32Sleeptime = 1;
            if (threshold == -3)
            {
                VDEC_LOGD("chn:%2d [dST:%d cT:%llu ckT:%llu] [fT:%u bT:%u dFT:%d] s32Delay:%d\n",
                          u32VdecChn, s32DistSysTime, u64CurMsTime, pVdecCtl->checkTime, pVdecCtl->curFrmTimeTru, pVdecCtl->baseTime, s32DistFrmTime, s32Delay);
            }
        }
        else
        {
            break;
        }

        if (pVdecCtl->bUpdataDecTime && (pVdecCtl->u32PlaySpeed > DECODE_SPEED_8x))
        {
            u64CurMsTime = vdec_tsk_getTimeMilli();
            pVdecCtl->baseTime = pVdecCtl->curFrmTimeTru;
            pVdecCtl->checkTime = u64CurMsTime;
            pVdecCtl->bUpdataDecTime = SAL_FALSE;
        }

        if (VDEC_STATE_RUNNING != pDecCtrl->chnStatus)
        {
            VDEC_LOGI("u32VdecChn %d chnStatus %d\n", u32VdecChn, pDecCtrl->chnStatus);
            return SAL_SOK;
        }

        usleep(u32Sleeptime * 1000);

    }
    while (1);

    return SAL_SOK;
}

/**
 * @function    vdec_tsk_DecSyncCtrl
 * @brief    同步控制
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return  static INT32
 */
static INT32 vdec_tsk_DecSyncCtrl(UINT32 u32VdecChn)
{
    VDEC_TASK_CTL *pVdecCtl = NULL;
    DEC_MOD_CTRL *pDecCtrl = NULL;
    UINT32 u32GlbTimeThread = 0;
    UINT32 u32SleepTime = 1;
    INT32 s32DistFrmMsTime = 0;
    INT32 s32Ret = SAL_SOK;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));

    pVdecCtl = &g_stVecTaskCtl[u32VdecChn];
    pDecCtrl = &g_stVdecObjCommon.stObjTsk[u32VdecChn].decModCtrl;
    pVdecCtl->glbTime = vdec_tsk_GetAabsTime(&pVdecCtl->glb_time);

start:
    if (VDEC_STATE_RUNNING != pDecCtrl->chnStatus)
    {
        VDEC_LOGI("u32VdecChn %d chnStatus %d\n", u32VdecChn, pDecCtrl->chnStatus);
        return SAL_SOK;
    }

    u32GlbTimeThread = vdec_tsk_PlayGetThread(pVdecCtl->u32PlaySpeed);
    u32GlbTimeThread /= 1000;
    pVdecCtl->cntDebug++;

    pVdecCtl->adjustTime = 0;
    if (pVdecCtl->syncPrm.mode != SYNC_MODE_NON)
    {
        if (pVdecCtl->syncPrm.mode == SYNC_MODE_SLAVE)
        {
            if (pVdecCtl->vdecMode == DECODE_MODE_DRAG_PLAY)
            {
                return SAL_SOK;
            }

            /* 主通道帧时间 frmTime */
            pVdecCtl->syncMasterAbsTimeMs = g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMasterAbsTimeMs;

            /* 调试 */
           #if 0
            if ((pVdecCtl->cntDebug > 500) && ((pVdecCtl->cntDebug % 200) == 0))
            {
                VDEC_LOGD("slave chn %d time y:%u,m:%u,d:%u,h:%u,mi:%u,s:%u,ms:%u\n", u32VdecChn,
                          pVdecCtl->glb_time.year, pVdecCtl->glb_time.month, pVdecCtl->glb_time.date,
                          pVdecCtl->glb_time.hour, pVdecCtl->glb_time.minute, pVdecCtl->glb_time.second, pVdecCtl->glb_time.msecond);
                VDEC_LOGD("master chn 5 time y:%u,m:%u,d:%u,h:%u,mi:%u,s:%u,ms:%u\n",
                          g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].glb_time.year,
                          g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].glb_time.month,
                          g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].glb_time.date,
                          g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].glb_time.hour,
                          g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].glb_time.minute,
                          g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].glb_time.second,
                          g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].glb_time.msecond);

                VDEC_LOGD("u32VdecChn %d slv gt: %llu..%u,th %u,mast:%llu..%u\n", u32VdecChn, pVdecCtl->glbTime, pVdecCtl->glb_time.msecond, u32GlbTimeThread, g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg, g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg_ms);
            }

           #endif
            if ((pVdecCtl->glbTime + u32GlbTimeThread)
                < g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg)
            {
                /*不解码,更新时标防止卡住*/

                pVdecCtl->bUpdataDecTime = SAL_TRUE;
                pVdecCtl->skipVdec = SAL_TRUE;
                pVdecCtl->bFastestMode = SAL_TRUE;
                if ((pVdecCtl->cntDebug >= 500) && ((pVdecCtl->cntDebug % 300) == 0))
                {
                    VDEC_LOGD("u32VdecChn %d is slow will skip slv gt: %llu..%u,th %u,mast:%llu..%u\n", u32VdecChn, pVdecCtl->glbTime, pVdecCtl->glb_time.msecond, u32GlbTimeThread, g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg, g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg_ms);
                }

                pVdecCtl->bigDiff = 1;
                return SAL_SOK;
            }
            else if ((pVdecCtl->glbTime > g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg + u32GlbTimeThread))
            {
                /*绝对时间加上相对时间的毫秒数据，绝对时间是I帧，相对时间每帧都在计算*/
                if(pVdecCtl->glbTime > g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg + u32GlbTimeThread + pVdecCtl->milliSecond)
                {
                    u32SleepTime = 10;
                     VDEC_LOGD("u32VdecChn %d is faster will wait slv gt: %llu..%u,pts %u,th %u,mast:%llu..%u,pts %u\n", u32VdecChn,
                          pVdecCtl->glbTime, pVdecCtl->glb_time.msecond, pVdecCtl->timestampLast,
                          u32GlbTimeThread,
                          g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg, g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg_ms,
                          g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].timestampLast);

                    pVdecCtl->bFastestMode = SAL_FALSE;
                    goto wait;
                }
            }
            else
            {
                s32DistFrmMsTime = (pVdecCtl->glbTime - g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg) * 1000 \
                                   + (pVdecCtl->glb_time.msecond - g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg_ms);
                if (s32DistFrmMsTime > 200 && (SAL_FALSE == pVdecCtl->bFastestMode))
                {
                    u32SleepTime = 10;
                    VDEC_LOGD("u32VdecChn %d is faster slv gt: %llu..%u,pts %u,dt %d,mast:%llu..%u,pts %u\n", u32VdecChn,
                              pVdecCtl->glbTime, pVdecCtl->glb_time.msecond, pVdecCtl->timestampLast,
                              s32DistFrmMsTime,
                              g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg,
                              g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg_ms,
                              g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].timestampLast);
                    /* VDEC_LOGI("u32VdecChn %d is faster slv: %llu,mast:%llu\n",u32VdecChn,pVdecCtl->glbTime, g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg); */
                    goto wait;
                }
                else if ((s32DistFrmMsTime < 50) || pVdecCtl->bFastestMode)
                {
                   #if 0
                    if ((pVdecCtl->cntDebug >= 500) && ((pVdecCtl->cntDebug % 10) == 0))
                    {
                        /* if (pVdecCtl->bFastestMode == SAL_FALSE) */
                        VDEC_LOGD("u32VdecChn %d mst %d is slow slv gt: %llu..%u pts %u,df %d,th %d,mast:%llu..%u pts %u\n", u32VdecChn,
                                  pVdecCtl->syncPrm.syncMasterChanId, pVdecCtl->glbTime, pVdecCtl->glb_time.msecond, pVdecCtl->timestampLast,
                                  s32DistFrmMsTime, (-u32GlbTimeThread * 1000),
                                  g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg,
                                  g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg_ms,
                                  g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].timestampLast);
                    }

                    #endif
                    pVdecCtl->bFastestMode = SAL_TRUE;
                    pVdecCtl->bUpdataDecTime = SAL_TRUE;
                    if (s32DistFrmMsTime > 20)
                    {
                        pVdecCtl->bFastestMode = SAL_FALSE;
                    }

                    /* pVdecCtl->adjustTime = 0; */
                    if (s32DistFrmMsTime > 20 && s32DistFrmMsTime < 200)
                    {
                        pVdecCtl->bFastestMode = SAL_FALSE;
                        /* pVdecCtl->adjustTime = vdec_tsk_SetDecTimeAdjust(s32DistFrmMsTime,-20, 0, 20); */
                        VDEC_LOGD("u32VdecChn %d mst %d is slow slv gt: %llu..%u pts %u,df %d,%d,mast:%llu..%u pts %u\n", u32VdecChn,
                                  pVdecCtl->syncPrm.syncMasterChanId, pVdecCtl->glbTime, pVdecCtl->glb_time.msecond, pVdecCtl->timestampLast,
                                  s32DistFrmMsTime, (-u32GlbTimeThread * 1000),
                                  g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg,
                                  g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg_ms,
                                  g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].timestampLast);
                    }

                    pVdecCtl->bigDiff = 0;


                }
                else
                {
                    pVdecCtl->adjustTime = vdec_tsk_SetDecTimeAdjust(s32DistFrmMsTime, -50, 0, 50);
                    /* VDEC_LOGI("u32VdecChn %d normal slv gt: %llu..%u,df %d,mast:%llu..%u\n",u32VdecChn,pVdecCtl->glbTime,pVdecCtl->glb_time.msecond,s32DistFrmMsTime, g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg,g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg_ms); */
                }

                if ((SAL_FALSE == pVdecCtl->bFastestMode) && (pVdecCtl->u32PlaySpeed >= DECODE_SPEED_8x))
                {
                    #if 1
                    /* VDEC_LOGI("u32VdecChn %d s32DistFrmMsTime %d , slv: %llu,mast:%llu\n",u32VdecChn,s32DistFrmMsTime,pVdecCtl->glbTime, g_stVecTaskCtl[pVdecCtl->syncPrm.syncMasterChanId].syncMGlg); */
                    s32Ret = vdec_tsk_DemuxSpeedCtrlCalc(u32VdecChn, pVdecCtl, -3);
                    if (s32Ret != SAL_SOK)
                    {
                        VDEC_LOGE("u32VdecChn %d error\n", u32VdecChn);
                        return SAL_FAIL;
                    }

                    #endif
                    pVdecCtl->bigDiff = 0;
                    return SAL_FAIL;
                }
            }

            return SAL_SOK;
wait:
            if (pVdecCtl->bUpdataDecTime && (pVdecCtl->u32PlaySpeed > DECODE_SPEED_8x))
            {
                pVdecCtl->baseTime = pVdecCtl->curFrmTimeTru;
                pVdecCtl->checkTime = vdec_tsk_getTimeMilli();
                pVdecCtl->bUpdataDecTime = SAL_FALSE;
            }

            usleep(u32SleepTime * 1000);
            goto start;
        }
        else if (pVdecCtl->syncPrm.mode == SYNC_MODE_MASTER)
        {
            if (pVdecCtl->vdecMode != DECODE_MODE_DRAG_PLAY)
            {
                s32Ret = vdec_tsk_DemuxSpeedCtrlCalc(u32VdecChn, pVdecCtl, 0);
                if (s32Ret != SAL_SOK)
                {
                    VDEC_LOGE("u32VdecChn %d speed ctrl error ret 0x%x\n", u32VdecChn, s32Ret);
                    return SAL_FAIL;
                }
            }

            pVdecCtl->syncMasterAbsTimeMs = pVdecCtl->curFrmTimeTru;
            pVdecCtl->syncMGlg = pVdecCtl->glbTime;
            pVdecCtl->syncMGlg_ms = pVdecCtl->glb_time.msecond;

            return SAL_FAIL;
        }
        else
        {
            VDEC_LOGE("u32VdecChn %d syncMode %d error\n", u32VdecChn, pVdecCtl->syncPrm.mode);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function    vdec_tskGetJpegInfo
 * @brief    获取jpeg参数
 * @param[in] void *prm 入参
 * @param[in] UINT32 *w 宽度
 * @param[in] UINT32 *h 高度
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tskGetJpegInfo(void *prm, UINT32 *w, UINT32 *h)
{
    UINT8 *pstBuff = NULL;

    FACE_DSP_REGISTER_JPG_PARAM *pstJpeg = NULL;

    INT32 i = 0;
    INT32 seek = 0;

    if (prm == NULL || w == NULL || h == NULL)
    {
        VDEC_LOGE("prm %p w %p h %p \n", prm, w, h);
        return SAL_FAIL;
    }

    pstJpeg = (FACE_DSP_REGISTER_JPG_PARAM *)prm;

    pstBuff = pstJpeg->pAddr;
    if (pstBuff == NULL)
    {
        VDEC_LOGE("pstBuff %p \n", pstBuff);
        return SAL_FAIL;
    }

    if (pstJpeg->uiLength < 4)
    {
        VDEC_LOGE("pstJpeg->uiLength %d \n", pstJpeg->uiLength);
        return SAL_FAIL;
    }

    if (pstBuff[0] != 0xFF || pstBuff[1] != 0xD8)
    {
        VDEC_LOGE("This is not a jpeg ( head error )! \n");
        return SAL_FAIL;
    }

    /* JFIF Format */
    if (pstBuff[2] == 0xFF && pstBuff[3] == 0xE0)
    {
        for (i = 0; i < pstJpeg->uiLength; i++)
        {
            if ((pstBuff[i]) == 0xFF && (pstBuff[i + 1]) == 0xC0)
            {
                seek = i + 2 + 3;

                VDEC_LOGI("0x%x 0x%x \n", pstBuff[seek], pstBuff[seek + 1]);

                *h = ((pstBuff[seek] << 8) + (pstBuff[seek + 1]));

                VDEC_LOGI("0x%x 0x%x \n", pstBuff[seek + 2], pstBuff[seek + 3]);

                *w = ((pstBuff[seek + 2] << 8) + (pstBuff[seek + 3]));

                VDEC_LOGI("Resolution %d X %d \n", *w, *h);

                return SAL_SOK;
            }
            else if ((pstBuff[i]) == 0xFF && (pstBuff[i + 1]) == 0xD9)
            {
                VDEC_LOGE("jpeg is end ! \n");
                return SAL_FAIL;
            }
        }
    }
    /* EXIF Format */
    else if (pstBuff[2] == 0xFF && pstBuff[3] == 0xE1)
    {
        if (0x4D == pstBuff[12] && 0x4D == pstBuff[13])
        {
            VDEC_LOGE("big endian entering! 12 %d 13 %d \n", pstBuff[12], pstBuff[13]);

            /* Big-Endian */
            for (i = 0; i < pstJpeg->uiLength; i++)
            {
                if (0 != *w && 0 != *h)
                {
                    VDEC_LOGI("Resolution %d X %d \n", *w, *h);
                    return SAL_SOK;
                }

                if ((pstBuff[i]) == 0x01 && (pstBuff[i + 1]) == 0x1A)
                {
                    seek = i + 10;

                    VDEC_LOGI("0x%x 0x%x \n", pstBuff[seek], pstBuff[seek + 1]);

                    *w = ((pstBuff[seek] << 8) + (pstBuff[seek + 1]));

                    VDEC_LOGI("Width %d \n", *w);
                }
                else if ((pstBuff[i]) == 0x01 && (pstBuff[i + 1]) == 0x1B)
                {
                    seek = i + 10;

                    VDEC_LOGI("0x%x 0x%x \n", pstBuff[seek], pstBuff[seek + 1]);

                    *h = ((pstBuff[seek] << 8) + (pstBuff[seek + 1]));

                    VDEC_LOGI("Height %d \n", *h);
                }
                else if ((pstBuff[i]) == 0xFF && (pstBuff[i + 1]) == 0xD9)
                {
                    VDEC_LOGE("jpeg is end ! \n");
                    return SAL_FAIL;
                }
            }
        }
        else if (0x49 == pstBuff[12] && 0x49 == pstBuff[13])
        {
            VDEC_LOGE("little endian entering! 12 %d 13 %d \n", pstBuff[12], pstBuff[13]);

            /* Little-Endian */
            for (i = 0; i < pstJpeg->uiLength; i++)
            {
                if (0 != *w && 0 != *h)
                {
                    VDEC_LOGI("Resolution %d X %d \n", *w, *h);
                    return SAL_SOK;
                }

                if ((pstBuff[i]) == 0x1A && (pstBuff[i + 1]) == 0x01)
                {
                    seek = i + 8;

                    VDEC_LOGI("0x%x 0x%x \n", pstBuff[seek], pstBuff[seek + 1]);

                    *w = ((pstBuff[seek + 1] << 8) + (pstBuff[seek]));

                    VDEC_LOGI("Width %d \n", *w);
                }
                else if ((pstBuff[i]) == 0x1B && (pstBuff[i + 1]) == 0x01)
                {
                    seek = i + 8;

                    VDEC_LOGI("0x%x 0x%x \n", pstBuff[seek], pstBuff[seek + 1]);

                    *h = ((pstBuff[seek + 1] << 8) + (pstBuff[seek]));

                    VDEC_LOGI("Height %d \n", *h);
                }
                else if ((pstBuff[i]) == 0xFF && (pstBuff[i + 1]) == 0xD9)
                {
                    VDEC_LOGE("jpeg is end ! \n");
                    return SAL_FAIL;
                }
            }
        }
    }
    else
    {
        VDEC_LOGE("this is not a jpeg! Pls Check file formattttttttt!\n");
    }

    return SAL_FAIL;
}

/**
 * @function	vdec_tsk_Bmp2Nv21
 * @brief	 bmp转nv21
 * @param[in] CHAR *pBmp bmp数据
 * @param[in] CHAR *pYuv yuv数据
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_Bmp2Nv21(CHAR *pBmp, CHAR *pYuv, void* prm)
{
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 w = 0;
    UINT32 h = 0;
    UINT32 offset = 0;    /* byte size */

    CHAR b, g, r;

    CHAR *y = NULL;
    CHAR *uv = NULL;
    VDEC_TSK_PIC_PRM *pstPciture = NULL;

    BITMAPFILEHEADER stBmpFileHdr = {0};
    BITMAPINFOHEADER stBmpInfoHdr = {0};

    memcpy(&stBmpFileHdr, pBmp + offset, sizeof(BITMAPFILEHEADER));
    offset += sizeof(BITMAPFILEHEADER);

    /* 判断图片类型，不是bmp图片返回报错 */
    if (0x4d42 != stBmpFileHdr.bfType)
    {
        SVA_LOGE("not support Img Info:%x \n", stBmpFileHdr.bfType);
        return SAL_FAIL;
    }

    memcpy(&stBmpInfoHdr, pBmp + offset, sizeof(BITMAPINFOHEADER));
    offset += sizeof(BITMAPINFOHEADER);

    if (stBmpInfoHdr.biClrUsed)
    {
        /* color table process not consider... */
    }

    w = stBmpInfoHdr.biWidth;
    h = stBmpInfoHdr.biHeight;
    pstPciture =(VDEC_TSK_PIC_PRM*)prm;
    pstPciture->uiW = w;
    pstPciture->uiH = h;

    if (w > 1600 || h > 1280)
    {
        SVA_LOGE("w %d, h %d, pic prm error! \n", pstPciture->uiW, pstPciture->uiH);
        return SAL_FAIL;
    }
    
    /*
    **   b g r b g r .... b g r
    **   b g r b g r .... b g r
    */

    y = pYuv;
    uv = pYuv + w * h;

    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {

            sal_memcpy_s(&b, sizeof(char), pBmp + offset, sizeof(char));
            offset += 1;
            sal_memcpy_s(&g, sizeof(char), pBmp + offset, sizeof(char));
            offset += 1;
            sal_memcpy_s(&r, sizeof(char), pBmp + offset, sizeof(char));
            offset += 1;

            y[(h - i) * w + j] = Y(r, g, b);
            uv[(h - i) / 2 * w + j + j % 2] = V(r, g, b);
            uv[(h - i) / 2 * w + j + j % 2 + 1] = U(r, g, b);
        }
    }

    return SAL_SOK;
}

/**
 * @function    vdec_tsk_Nv21ToRgb24P
 * @brief   nv21 转 rgb
 * @param[in] U08 *pSrc nv21数据
 * @param[in] U08* pRBuf 红色分量
 * @param[in] U08*pGBuf 绿色分量
 * @param[in] U08* pBBuf 蓝色分量
 * @param[in] UINT32 u32Width 宽
 * @param[in] UINT32 u32Height 高
 * @param[in] UINT32 u32Stride 跨距
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_Nv21ToRgb24P(U08 *pSrc, U08 *pRBuf, U08 *pGBuf, U08 *pBBuf, UINT32 u32Width, UINT32 u32Height, UINT32 u32Stride)
{
    INT32 i = 0, j = 0;          /*有符号整型*/
    UINT8 y = 0, u = 0, v = 0;
    INT16 r = 0, g = 0, b = 0;
    U08 *pRBufTemp = NULL;
    U08 *pGBufTemp = NULL;
    U08 *pBBufTemp = NULL;

    if ((NULL == pSrc) || (NULL == pRBuf) || (NULL == pGBuf) || (NULL == pBBuf))
    {
        VDEC_LOGE("pSrc[%p], pRBuf[%p], pGBuf[%p], pBBuf[%p] is error!\n", pSrc, pRBuf, pGBuf, pBBuf);
        return SAL_FAIL;
    }

    if ((0 == u32Width) || (0 == u32Height) || (0 == u32Stride))
    {
        VDEC_LOGE("u32Width[%u], u32Height[%u], u32Stride[%u] is error!\n", u32Width, u32Height, u32Stride);
        return SAL_FAIL;
    }

    pBBufTemp = pBBuf;
    pRBufTemp = pRBuf;
    pGBufTemp = pGBuf;

    for (i = 0; i < u32Height; i++)
    {
        for (j = 0; j < u32Width; j++)
        {
            y = *(pSrc + i * u32Stride + j);
            v = *(pSrc + u32Stride * u32Height + (i >> 1) * u32Stride + j - j % 2);
            u = *(pSrc + u32Stride * u32Height + (i >> 1) * u32Stride + 1 + j - j % 2);

            CVT_YUV2RGB(y, u, v, r, g, b);

            *(pRBufTemp++) = (UINT8)r;
            *(pGBufTemp++) = (UINT8)g;
            *(pBBufTemp++) = (UINT8)b;
        }
    }

    return SAL_SOK;
}

/**
 * @function    vdec_tsk_SaveYuv2Bmp
 * @brief   yuv转bmp
 * @param[in] BMP_RESULT_ST *date bmp 数据
 * @param[in] SYSTEM_FRAME_INFO *pVFrame 数据帧
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_SaveYuv2Bmp(SYSTEM_FRAME_INFO *pVFrame, BMP_RESULT_ST *date)
{
    INT32 i, j;
    INT32 s32Ret = SAL_SOK;
    UINT8 *pUserPageAddr = NULL, *pTmp = NULL, *pData = NULL;
    UINT32 u32StrideInBytes;
    UINT32 u32Stride;
    UINT32 offset = 0;
    UINT32 w = 0, h = 0;
    BITMAPFILEHEADER stFHdr;
    BITMAPINFOHEADER stInfoHdr;
    UINT32 nv_start = 0;
    PIC_FRAME_PRM pPicprm;
    INT16 r = 0, g = 0, b = 0; /* , a = 0; */
    UINT8 y = 0, u = 0, v = 0;

    CHECK_PTR_NULL(pVFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(date, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    s32Ret = vdec_tsk_Mmap(pVFrame, &pPicprm);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("frame mmap failed ret 0x%x\n", s32Ret);
        return SAL_FAIL;
    }

    if ((SAL_VIDEO_DATFMT_YUV420SP_VU == pPicprm.videoFormat))
    {
        u32StrideInBytes = (pPicprm.stride > pPicprm.width ? pPicprm.stride : pPicprm.width);
    }
    else
    {
        VDEC_LOGE("%s %d: This format is not support!\n", __func__, __LINE__);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
    }

    u32Stride = SAL_align(u32StrideInBytes, 4);

    pUserPageAddr = (UINT8 *)(pPicprm.addr[0]);

    CHECK_PTR_NULL(pUserPageAddr, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    nv_start = pPicprm.height * u32StrideInBytes;

    w = SAL_align(pPicprm.width, 4);
    h = SAL_align(pPicprm.height, 4);
    stFHdr.bfType = 0x4D42;
    stFHdr.bfSize = 54 + w * h * 3; /* file header + info header + bmp data */
    stFHdr.bfReserved1 = 0;
    stFHdr.bfReserved2 = 0;
    stFHdr.bfOffBits = 54;        /* file header + info header + color palette. */

    stInfoHdr.biSize = 0x28;
    stInfoHdr.biWidth = w;
    stInfoHdr.biHeight = h;
    stInfoHdr.biPlanes = 1;        /* always 1 */
    stInfoHdr.biBitCount = 24;     /* RGB888 */
    stInfoHdr.biCompression = 0;
    stInfoHdr.biSizeImage = w * h * 3;       /* size of actual bmp data, Bytes. */
    stInfoHdr.biXPelsPerMeter = 0;
    stInfoHdr.biYPelsPerMeter = 0;
    stInfoHdr.biClrUsed = 0;        /* number of color used. 0: all colors may be used. */
    stInfoHdr.biClrImportant = 0;

    /* bmp data storage : B->G->R->A */
    pData = (UINT8 *)(date->cBmpAddr + offset);
    memcpy(pData, &stFHdr, sizeof(BITMAPFILEHEADER));
    offset += sizeof(BITMAPFILEHEADER);

    pData = (UINT8 *)date->cBmpAddr + offset;
    memcpy(pData, &stInfoHdr, sizeof(BITMAPINFOHEADER));
    offset += sizeof(BITMAPINFOHEADER);
    /* VDEC_LOGI("w:%d,h:%d,s:%d.offset %d\n",w,h,u32Stride,offset); */

    pData = (UINT8 *)(date->cBmpAddr + offset);
    pTmp = pUserPageAddr;

    for (i = h - 1; i >= 0; i--)
    {
        for (j = 0; j < w; j++)
        {
            y = *(pTmp + i * u32Stride + j);
            v = *(pTmp + nv_start + (i >> 1) * u32Stride + j - j % 2);
            u = *(pTmp + nv_start + (i >> 1) * u32Stride + 1 + j - j % 2);

#if 0
            r = (y + redAdjust[v]);
            g = (y + greenAdjust1[u] + greenAdjust2[v]);
            b = (y + blueAdjust[u]);
#else
            r = (R_Y_1[y] + R_Cr[v]);
            g = (R_Y_1[y] - G_Cr[v] - G_Cb[u]); /* (R_Y[y] - G_Cr[u] + G_Cr[v]); */
            b = (R_Y_1[y] + B_Cb[u]);
#endif

            if (r > 255)
                r = 255;

            if (g > 255)
                g = 255;

            if (b > 255)
                b = 255;

            if (r < 0)
                r = 0;

            if (g < 0)
                g = 0;

            if (b < 0)
                b = 0;

            *pData = (UINT8)b;
            pData++;
            *pData = (UINT8)g;
            pData++;
            *pData = (UINT8)r;
            pData++;
        }
    }

    date->uiBmpSize = stFHdr.bfSize;
    pUserPageAddr = NULL;

    return SAL_SOK;
}

/**
 * @function    vdec_tsk_DemuxiFrame2BmpProcess
 * @brief   i帧转bmp
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] DEM_BUFFER *pDemBuf 数据
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_DemuxiFrame2BmpProcess(UINT32 u32VdecChn, DEM_BUFFER *pDemBuf)
{
    STREAM_ELEMENT stStreamEle;
    BMP_RESULT_ST stBmpResult;
    INT32 s32Ret = 0;
    UINT32 needFree = 0;
    SYSTEM_FRAME_INFO stFrameOut = {0};
    BMP_RESULT_ST *pOutBmp = NULL; /* &outBmpInfo[u32VdecChn]; */

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(pDemBuf, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pOutBmp = &outBmpInfo[u32VdecChn];

    s32Ret = sys_hal_allocVideoFrameInfoSt(&stFrameOut);
    if (s32Ret != SAL_SOK)
    {
        XPACK_LOGE("chn %d alloc struct video frame failed \n", u32VdecChn);
        goto end;
    }

    s32Ret = vdec_tsk_GetVdecFrame(u32VdecChn, 0, VDEC_DISP_GET_FRAME, &needFree, &stFrameOut, NULL);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d get frame errro ret 0x%x\n", u32VdecChn, s32Ret);
        goto end;
    }

    s32Ret = vdec_tsk_SaveYuv2Bmp(&stFrameOut, pOutBmp);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d yuv2bmp errro ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    memset(&stStreamEle, 0, sizeof(STREAM_ELEMENT));
    stStreamEle.type = STREAM_ELEMENT_DEC_BMP;
    stStreamEle.chan = u32VdecChn;
    stBmpResult.cBmpAddr = pOutBmp->cBmpAddr;
    stBmpResult.uiBmpSize = pOutBmp->uiBmpSize;
    SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)&stBmpResult, sizeof(BMP_RESULT_ST));
end:
    if (needFree == 1)
    {
        s32Ret = vdec_tsk_PutVideoFrmaeMeth(u32VdecChn, 0, &stFrameOut);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("chn %d release failed ret 0x%x\n", u32VdecChn, s32Ret);
        }
    }

    sys_hal_rleaseVideoFrameInfoSt(&stFrameOut);

    return s32Ret;
}

/**
 * @function    vdec_tsk_VdecDecframe
 * @brief   解码一帧数据
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] SAL_VideoFrameBuf *pstSrcframe 数据源
 * @param[in]  SAL_VideoFrameBuf *pstDstframe 解码后数据
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_VdecDecframe(UINT32 u32VdecChn, SAL_VideoFrameBuf *pstSrcframe, SAL_VideoFrameBuf *pstDstframe)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32TimeOut = 30; 

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(pstSrcframe, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstDstframe, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    s32Ret = vdec_hal_VdecDecframe(u32VdecChn, pstSrcframe, pstDstframe, u32TimeOut);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d dec one frame failed s32Ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

/**
 * @function    vdec_tsk_DestroyVdec
 * @brief   销毁解码器
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] BOOL bVdecDupBindSts 当前解码器和dup绑定状态
 * @param[in] unsigned int u32FrontDupHdl  dup前级通道句柄
 * @param[in] unsigned int u32RearDupHdl   dup后级通道句柄
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_DestroyVdec(UINT32 u32VdecChn, BOOL bVdecDupBindSts, unsigned int u32FrontDupHdl, unsigned int u32RearDupHdl)
{
    INT32 s32Ret = SAL_SOK;
    DEC_MOD_CTRL *pDecModeCtrl = NULL;
    DUP_BIND_PRM stDupBindPrm = {0};
      
    pDecModeCtrl = &g_stVdecObjCommon.stObjTsk[u32VdecChn].decModCtrl;
    if (SAL_TRUE == bVdecDupBindSts)
    {
        if(pDecModeCtrl->dupBindType != NODE_BIND_TYPE_GET)
        {
            stDupBindPrm.mod = SYSTEM_MOD_ID_VDEC;
            stDupBindPrm.chn = u32VdecChn;
    
            if (SAL_SOK != (s32Ret = dup_task_bindToDup(&stDupBindPrm, (DupHandle)u32FrontDupHdl, SAL_FALSE)))
            {
                VDEC_LOGE("chn %d create handle failed ret 0x%x\n", u32VdecChn, s32Ret);
            }
        }
        (VOID)dup_task_vdecDupDestroy((DupHandle)u32FrontDupHdl, (DupHandle)u32RearDupHdl, u32VdecChn);
    }

    if (SAL_SOK != (s32Ret = vdec_hal_VdecStop(u32VdecChn)))
    {
        VDEC_LOGE("chn %d stop failed ret 0x%x\n", u32VdecChn, s32Ret);
    }

    if (SAL_SOK != (s32Ret = vdec_hal_VdecDeinit(u32VdecChn)))
    {
        VDEC_LOGE("chn %d deinit failed ret 0x%x\n", u32VdecChn, s32Ret);
    }    

    return s32Ret;
}

/**
 * @function    vdec_tsk_CreateVdec
 * @brief   创建解码器
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] VDEC_PRM *pstVdecPrm 解码器创建参数
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_CreateVdec(UINT32 u32VdecChn, VDEC_PRM *pstVdecPrm)
{
    INT32 s32Ret = SAL_SOK;

    CHECK_PTR_NULL(pstVdecPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));    
    
    if (SAL_SOK != (s32Ret = vdec_hal_VdecCreate(u32VdecChn, pstVdecPrm)))
    {
        VDEC_LOGE("chn %d create failed ret 0x%x\n", u32VdecChn, s32Ret);
        return SAL_FAIL;
    }


    return SAL_SOK;
}

/**
 * @function   vdec_tsk_GetVdecChnStatus
 * @brief	   当前解码器状态
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[in]  INT32 *pCreat 解码器是否被创建
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_tsk_GetVdecChnStatus(UINT32 u32VdecChn, UINT32 *pCreat)
{
    INT32 s32Ret = SAL_FAIL;
    VDEC_HAL_STATUS pvdecStatus = {0};
    
    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));
    CHECK_PTR_NULL(pCreat, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    
    s32Ret = vdec_hal_VdecChnStatus(u32VdecChn, &pvdecStatus);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE(" Get Vdec Chn Status failed ret 0x%x\n", s32Ret);
        return s32Ret;
    }
    
    *pCreat = pvdecStatus.bcreate;
    
    return s32Ret;
}

static UINT64 vdec_tskGetTimepFromCalender(DEM_GLOBAL_TIME *pGlbTime)
{
    UINT64 ullTimep = 0;
    struct tm stTm = {0};

    if (NULL == pGlbTime)
    {
        SAL_ERROR("pGlbTime == null! \n");
        goto exit;
    }

    stTm.tm_year = pGlbTime->year-1900;
    stTm.tm_mon = pGlbTime->month-1;
    stTm.tm_mday = pGlbTime->date;
    stTm.tm_hour = pGlbTime->hour;
    stTm.tm_min = pGlbTime->minute;
    stTm.tm_sec = pGlbTime->second;

    ullTimep = SAL_getTimepFromCalender(&stTm);

    //SAL_WARN("3-------------ullTimep %llu \n", ullTimep);

exit:
    return ullTimep;
}

UINT64 vdec_tskGetTimeOfDay(VOID)
{
    return SAL_getTimeOfDayMs();
}

/**
 * @function    vdec_tsk_UpdateVdec
 * @brief   更新解码器
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] DEM_BUFFER *pDemBuf 参数
 * @param[out] None
 * @return  static INT32
 */
static INT32 vdec_tsk_UpdateVdec(UINT32 u32VdecChn, DEM_BUFFER *pDemBuf)
{
    INT32 s32Ret = SAL_SOK;
    INT32 cnt = 10;
    INT32 i = 0, j = 0;
    PARAM_INFO_S stParamInfo;
    CHAR szInstPreName[NAME_LEN] = "DUP_VDEC";
    VDEC_PRM vdecPrm = {0};
    VDEC_HAL_STATUS pvdecStatus = {0};
    VDEC_BIND_CTL pVdecBindCtl = {0};
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    CHAR szOutNodeNm[NAME_LEN] = {0};
    INST_INFO_S *pstInst = NULL;
    VDEC_TASK_CTL *pVdecTskCtl = NULL;
    DUP_BIND_PRM pSrcBuf = {0};

    DISP_LOGI("uiVoDevChn %d vdec_tsk_UpdateVdec start !\n", u32VdecChn);
    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(pDemBuf, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];
    pVdecTskCtl = &g_stVecTaskCtl[u32VdecChn];

    vdec_hal_VdecChnStatus(u32VdecChn, &pvdecStatus);
    vdec_hal_VdecGetBind(u32VdecChn, &pVdecBindCtl);

    if(pDemBuf->width == 0 ||  pDemBuf->height == 0 )
    {
        vdecPrm.decWidth = VDEC_WIDTH_DEF;
        vdecPrm.decHeight = VDEC_HEIGH_DEF;
    }
    else
    {
        vdecPrm.decWidth = pDemBuf->width;
        vdecPrm.decHeight = pDemBuf->height;
        vdecPrm.encType = pDemBuf->streamType;
    }
   
    SAL_mutexLock(pVdecTskCtl->mutex);
    while(pVdecTskCtl->unReleaseFrameCnt > 0)
    {
        VDEC_LOGE("cnt %d %d\n",pVdecTskCtl->unReleaseFrameCnt,cnt);
        usleep(20 * 1000);
        cnt--;
        if (cnt <= 0)
        {
            pVdecTskCtl->unReleaseFrameCnt = 0;
            break;
        }
    }
    if (pvdecStatus.bcreate == SAL_TRUE)
    {
        VDEC_LOGI("chn %d has create need deinit\n", u32VdecChn);

        (VOID)vdec_tsk_DestroyVdec(u32VdecChn, 
                                   pVdecBindCtl.needBind, 
                                   pVdecTskChn->decModCtrl.frontDupHandle, 
                                   pVdecTskChn->decModCtrl.rearDupHandle);
    }

    if (SAL_SOK != (s32Ret = vdec_tsk_CreateVdec(u32VdecChn, &vdecPrm)))
    {
        VDEC_LOGE("chn %d create failed ret 0x%x\n", u32VdecChn, s32Ret);
        /*创建失败直接返回错误，否者会导致绑定失败*/
        SAL_mutexUnlock(pVdecTskCtl->mutex);
        return SAL_FAIL;
    }

    if (pVdecBindCtl.needBind == 1)
    {
        if (SAL_SOK != (s32Ret = dup_task_vdecDupCreate(&stVdecDupInstCfg, u32VdecChn, &pVdecTskChn->decModCtrl.frontDupHandle, &pVdecTskChn->decModCtrl.rearDupHandle,&vdecPrm)))
        {
            VDEC_LOGE("chn %d create handle failed ret 0x%x\n", u32VdecChn, s32Ret);
        }

        snprintf(szInstPreName, NAME_LEN, "DUP_VDEC_%d", u32VdecChn);
        pstInst = link_drvGetInst(szInstPreName);
        CHECK_PTR_NULL(pstInst, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
        snprintf(szOutNodeNm, NAME_LEN, "out_disp0");
        pVdecTskChn->decModCtrl.dupChanHandle[0] = link_drvGetHandleFromNode(pstInst, szOutNodeNm);
        CHECK_PTR_NULL(pVdecTskChn->decModCtrl.dupChanHandle[0], DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR))
        snprintf(szOutNodeNm, NAME_LEN, "out_disp1");
        pVdecTskChn->decModCtrl.dupChanHandle[1] = link_drvGetHandleFromNode(pstInst, szOutNodeNm);
        CHECK_PTR_NULL(pVdecTskChn->decModCtrl.dupChanHandle[1], DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR))
        snprintf(szOutNodeNm, NAME_LEN, "out_jpeg");
        pVdecTskChn->decModCtrl.dupChanHandle[2] = link_drvGetHandleFromNode(pstInst, szOutNodeNm);
        CHECK_PTR_NULL(pVdecTskChn->decModCtrl.dupChanHandle[2], DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR))
        snprintf(szOutNodeNm, NAME_LEN, "out_ba");
        pVdecTskChn->decModCtrl.dupChanHandle[3] = link_drvGetHandleFromNode(pstInst, szOutNodeNm);
        CHECK_PTR_NULL(pVdecTskChn->decModCtrl.dupChanHandle[3], DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR))
        VDEC_LOGI("chn %d dup handle [%p %p %p %p]\n", u32VdecChn, pVdecTskChn->decModCtrl.dupChanHandle[0],
                  pVdecTskChn->decModCtrl.dupChanHandle[1], pVdecTskChn->decModCtrl.dupChanHandle[2], pVdecTskChn->decModCtrl.dupChanHandle[3]);

        /*nt需要前设置vpe输入分辨率*/
        if(pVdecTskChn->decModCtrl.dupBindType == NODE_BIND_TYPE_GET)
        {
            for(i = 0;i < 4;i++)
            {
                
                memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
                stParamInfo.enType = GRP_SIZE_IN_CFG;
                
                if(vdecPrm.encType != MJPEG)
                {
                     stParamInfo.stImgSize.u32Format = 0x610c4420;   ///HD_VIDEO_PXLFMT_YUV420_NVX4 < novatek-yuv-compress-4 of YUV420 for h264/h265 LLC_8x4
                }
                stParamInfo.stImgSize.u32Width = SAL_align(vdecPrm.decWidth, 64);
                stParamInfo.stImgSize.u32Height = SAL_align(vdecPrm.decHeight, 64);
                s32Ret = pVdecTskChn->decModCtrl.dupChanHandle[i]->dupOps.OpDupSetBlitPrm((Ptr)pVdecTskChn->decModCtrl.dupChanHandle[i], &stParamInfo);
                if(SAL_SOK != s32Ret)
                {
                    SAL_WARN("OpDupSetBlitPrm failed !!!\n");
                    SAL_mutexUnlock(pVdecTskCtl->mutex);
                    return SAL_FAIL;
                }
    
                memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
                if((vdecPrm.decWidth != SAL_align(vdecPrm.decWidth, 64)) || (vdecPrm.decHeight != SAL_align(vdecPrm.decHeight, 64)))
                {
                    memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
                    stParamInfo.enType = CHN_CROP_CFG;
                    stParamInfo.stCrop.u32CropEnable = 1;
                    stParamInfo.stCrop.u32W = vdecPrm.decWidth;
                    stParamInfo.stCrop.u32H = vdecPrm.decHeight;
                    stParamInfo.stCrop.u32X = 0;
                    stParamInfo.stCrop.u32Y = 0;
                    stParamInfo.stCrop.sInImageSize.u32Width = SAL_align(vdecPrm.decWidth, 64);
                    stParamInfo.stCrop.sInImageSize.u32Height = SAL_align(vdecPrm.decHeight, 64);
                    s32Ret = pVdecTskChn->decModCtrl.dupChanHandle[i]->dupOps.OpDupSetBlitPrm((Ptr)pVdecTskChn->decModCtrl.dupChanHandle[i], &stParamInfo);
                    if (SAL_SOK != s32Ret)
                    {
                       DISP_LOGE("uiVoDevChn error !!\n");
                       SAL_mutexUnlock(pVdecTskCtl->mutex);
                       return SAL_FAIL;
                    }
                    
                }
                
                if (pVdecTskCtl->dWPrm.bind == 1)
                {
                     if (pVdecTskCtl->dWPrm.enable[i] == 0)
                     {
                         VDEC_LOGI("pVdecTskCtl->dWPrm chn%d disable !!!\n",i);
                         memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
                         stParamInfo.enType = IMAGE_SIZE_CFG;
                         stParamInfo.stImgSize.u32Width = VDEC_WIDTH_DEF;
                         stParamInfo.stImgSize.u32Height = VDEC_HEIGH_DEF;
                         for(j = 0; j < 4; j++)
                         {
                             if (pVdecTskCtl->dWPrm.enable[j])
                             {
                                 stParamInfo.stImgSize.u32Width = pVdecTskCtl->dWPrm.rect[j].uiWidth;
                                 stParamInfo.stImgSize.u32Height = pVdecTskCtl->dWPrm.rect[j].uiHeight;
                                 break;
                             }
                         }
                         VDEC_LOGI("pVdecTskCtl->dWPrm.w*h = %u*%u !!!\n",stParamInfo.stImgSize.u32Width, stParamInfo.stImgSize.u32Height);
                     }
                     else
                     {
                         memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
                         stParamInfo.enType = IMAGE_SIZE_CFG;
                         stParamInfo.stImgSize.u32Width = pVdecTskCtl->dWPrm.rect[i].uiWidth;
                         stParamInfo.stImgSize.u32Height = pVdecTskCtl->dWPrm.rect[i].uiHeight;
                         VDEC_LOGI("pVdecTskCtl->dWPrm.h = %d !!!\n",stParamInfo.stImgSize.u32Height);
                     }
                     s32Ret = pVdecTskChn->decModCtrl.dupChanHandle[i]->dupOps.OpDupSetBlitPrm((Ptr)pVdecTskChn->decModCtrl.dupChanHandle[i], &stParamInfo);
                     if(SAL_SOK != s32Ret)
                     {
                         SAL_WARN("OpDupSetBlitPrm failed !!!\n");
                         SAL_mutexUnlock(pVdecTskCtl->mutex);
                         return SAL_FAIL;
                     }
                }
                
                s32Ret =pVdecTskChn->decModCtrl.dupChanHandle[i]->dupOps.OpDupStartBlit((Ptr)pVdecTskChn->decModCtrl.dupChanHandle[i]);
                if(SAL_SOK != s32Ret)
                {
                    SAL_WARN("OpDupStartBlit failed !!!\n");
                    SAL_mutexUnlock(pVdecTskCtl->mutex);
                    return SAL_FAIL;
                }
            }
        }
        else
        {
            pSrcBuf.mod = SYSTEM_MOD_ID_VDEC;
            pSrcBuf.chn = u32VdecChn;
            if (SAL_SOK != (s32Ret = dup_task_bindToDup(&pSrcBuf, pVdecTskChn->decModCtrl.frontDupHandle, SAL_TRUE)))
            {
                VDEC_LOGE("chn %d create handle failed ret 0x%x\n", u32VdecChn, s32Ret);
                
                /*绑定失败释放dup句柄防止dup句柄泄漏被申请完*/
                dup_task_vdecDupDestroy(pVdecTskChn->decModCtrl.frontDupHandle, pVdecTskChn->decModCtrl.rearDupHandle, u32VdecChn);
            }
            
    
            /*为了降低性能减少解码到显示的缩放性能*/
            //if (pVdecTskCtl->dWPrm.bind == 1)  /* fixme: 此处临时注释，规避智能未开启解码显示时解码器绑定的vpss通道未开启的问题 */
            {
                for(i = 0;i < 4;i++)
                {
                    if (pVdecTskCtl->dWPrm.enable[i] == 0)
                    {
                         VDEC_LOGI("pVdecTskCtl->dWPrm chn%d disable !!!\n",i);
                         memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
                         stParamInfo.enType = IMAGE_SIZE_CFG;
                         stParamInfo.stImgSize.u32Width = VDEC_WIDTH_DEF;
                         stParamInfo.stImgSize.u32Height = VDEC_HEIGH_DEF;
                         for(j = 0; j < 4; j++)
                         {
                             if (pVdecTskCtl->dWPrm.enable[j])
                             {
                                 stParamInfo.stImgSize.u32Width = pVdecTskCtl->dWPrm.rect[j].uiWidth;
                                 stParamInfo.stImgSize.u32Height = pVdecTskCtl->dWPrm.rect[j].uiHeight;
                                 break;
                             }
                         }
                         VDEC_LOGI("pVdecTskCtl->dWPrm.w*h = %u*%u !!!\n",stParamInfo.stImgSize.u32Width, stParamInfo.stImgSize.u32Height);
                    }
                    else
                    {
                        stParamInfo.enType = IMAGE_SIZE_CFG;
                        stParamInfo.stImgSize.u32Width = pVdecTskCtl->dWPrm.rect[i].uiWidth;
                        stParamInfo.stImgSize.u32Height = pVdecTskCtl->dWPrm.rect[i].uiHeight;
                       
                    }
                    s32Ret = pVdecTskChn->decModCtrl.dupChanHandle[i]->dupOps.OpDupSetBlitPrm((Ptr)pVdecTskChn->decModCtrl.dupChanHandle[i], &stParamInfo);
                    if(SAL_SOK != s32Ret)
                    {
                        SAL_WARN("OpDupSetBlitPrm failed !!!\n");
                        SAL_mutexUnlock(pVdecTskCtl->mutex);
                        return SAL_FAIL;
                    }

                    s32Ret =pVdecTskChn->decModCtrl.dupChanHandle[i]->dupOps.OpDupStartBlit((Ptr)pVdecTskChn->decModCtrl.dupChanHandle[i]);
                    if(SAL_SOK != s32Ret)
                    {
                        SAL_WARN("OpDupStartBlit failed !!!\n");
                        SAL_mutexUnlock(pVdecTskCtl->mutex);
                        return SAL_FAIL;
                    }
                }
            
            }
        }
    }
    if (SAL_SOK != (s32Ret = vdec_hal_VdecStart(u32VdecChn)))
    {
        SAL_mutexUnlock(pVdecTskCtl->mutex);
        VDEC_LOGE("chn %d start failed ret 0x%x\n", u32VdecChn, s32Ret);
        return SAL_FAIL;
    }
    SAL_mutexUnlock(pVdecTskCtl->mutex);
    DISP_LOGI("uiVoDevChn %d vdec_tsk_UpdateVdec end !\n", u32VdecChn);

    return s32Ret;
}

/**
 * @function    vdec_tsk_DemuxThread
 * @brief   解复用线程
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
static void *vdec_tsk_DemuxThread(void *prm)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32Variable = 0;
    UINT32 u32VdecChn = 0;
    DEC_SHARE_BUF *pDecShare = NULL;
    DEC_MOD_CTRL *pDemuxCtrl = NULL;
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    VDEC_TASK_CTL *pVdecCtl = NULL;
    DEM_CALC_PRM calcPrm;
    DEM_PRCO_OUT procOut;
    DEM_BUFFER pDemBuf;
    SAL_VideoFrameBuf decFrameIn;
    SAL_VideoFrameBuf decFrameOut;
    UINT32 u32pbTmieDel = 0, u32norTimeDel = 0;
    UINT32 u32skipDecFlg = 0;
    UINT32 u32SpeedCtrl = 0;
    UINT32 u32CurFramePts = 0;
    UINT32 u32RtpFrameFps = 0;
    UINT32 u32Dec_One_frame = 0;
    VDEC_HAL_STATUS pvdecStatus = {0};
    DSPINITPARA *pDspInitPara = NULL;
    UINT8 *pAdtsData = NULL;
    INT32 *pHandle = NULL;
    INT32  s32Status = 0;
    ALLOC_VB_INFO_S stVbInfo = {0};
    UINT64 Lastpts[2] = {0};
    UINT64 LastSystime = 0;
	UINT32 u32IFrmTimeStamp = 0;
	UINT64 u64IFrmGlbRelateTime = 0;

    if (prm == NULL)
    {
        VDEC_LOGE("prm null err\n");
        return NULL;
    }

    memset(&pDemBuf, 0, sizeof(pDemBuf));
    pDspInitPara = SystemPrm_getDspInitPara();

    pVdecTskChn = (VDEC_TSK_CHN *)prm;

    u32VdecChn = pVdecTskChn->u32VdecChn;
    pDemuxCtrl = &pVdecTskChn->decModCtrl;

    pDecShare = &pDspInitPara->decShareBuf[u32VdecChn];

    if (pVdecTskChn->vdec_chn.packType == DEM_RTP)
    {
        pVdecTskChn->decModCtrl.chnHandle = pVdecTskChn->decModCtrl.rtpHandle;
    }
    else if (pVdecTskChn->vdec_chn.packType == DEM_PS)
    {
        pVdecTskChn->decModCtrl.chnHandle = pVdecTskChn->decModCtrl.psHandle;
    }

    pHandle = &pVdecTskChn->decModCtrl.chnHandle;

    pVdecCtl = &g_stVecTaskCtl[u32VdecChn];

    pVdecCtl->u32PlaySpeed = DECODE_SPEED_1x;
    pVdecCtl->bUpdataDecTime = SAL_TRUE;
    pVdecCtl->vdecMode = DECODE_MODE_BULL;
    pVdecCtl->decodeStandardTime = 0;   /* SAL_getTimeOfJiffies(); */
    pVdecCtl->decodeAbsTime = 0;
    pVdecCtl->decodePlayBackStdTime = 0xffffffff; /* SAL_getTimeOfJiffies(); */

    pVdecCtl->frameNum = 0;

    pVdecCtl->syncPrm.mode = SYNC_MODE_NON;
    pVdecCtl->skipVdec = SAL_FALSE;
    memset(&decFrameIn, 0x0, sizeof(SAL_VideoFrameBuf));
    memset(&decFrameOut, 0x0, sizeof(SAL_VideoFrameBuf));

    pDemuxCtrl->chnStatus = VDEC_STATE_STOPPED;
    VDEC_LOGI("u32VdecChn %d thread create\n", u32VdecChn);

    SAL_SET_THR_NAME();


    while (1)
    {
        if (pDemuxCtrl->chnStatus != VDEC_STATE_RUNNING)
        {
            u32Dec_One_frame = 0;
            if (pDemuxCtrl->chnStatus == VDEC_STATE_PREPARING)
            {
                pDemuxCtrl->chnStatus = VDEC_STATE_PREPARED;
            }

            if (pDemuxCtrl->chnStatus == VDEC_STATE_PAUSE)
            {
                /* 检测是否已经保存完当前视频帧 */
                if (pVdecCtl->isCopyOver == 1)
                {
                    s32Ret = vdec_tsk_GetVdecFrameForPausFromVdec(u32VdecChn, -1);
                    if (s32Ret != SAL_SOK)
                    {
                        u32Dec_One_frame = 1;
                    }
                    else
                    {
                        /* save_frame */
                        pVdecCtl->isCopyOver = 0;
                    }
                }
            }

            if (pVdecCtl->u32VdecPrmReset == 1)
            {
                /*解码复位清空解码器缓存*/
                s32Ret = vdec_hal_VdecClear(u32VdecChn);
                if (SAL_SOK != s32Ret)
                {
                    pDspInitPara->VdecStatus[u32VdecChn].decodedW = 0;
                    pDspInitPara->VdecStatus[u32VdecChn].decodedH = 0;
                    pDspInitPara->VdecStatus[u32VdecChn].streamVfps = 0;
                    pDspInitPara->VdecStatus[u32VdecChn].streamVtype = 0;
                    VDEC_LOGE("Reset vdecchn %u failed. s32Ret:%d\n", u32VdecChn, s32Ret);
                }


                pDecShare->decodeAbsTime = 0;
                pDecShare->decodeStandardTime = 0;
                pDecShare->decodePlayBackStdTime = 0;

                pVdecCtl->bUpdataDecTime = SAL_TRUE;
                pVdecCtl->timestampLast = 0;
                pVdecCtl->timestampFirst = 0;
                pVdecCtl->isNewDec = 1;

                u32norTimeDel = 0;
                u32CurFramePts = 0;
                u32RtpFrameFps = 0;

                pVdecCtl->u32VdecPrmReset = 0;
                VDEC_LOGI("u32VdecChn %d reset vdec prm\n", u32VdecChn);
            }

            if (!(u32Dec_One_frame == 1 || pDemuxCtrl->chnStatus == VDEC_STATE_STOPPED))
            {
               
                usleep(10 * 1000);
                continue;
            }

            VDEC_LOGI("u32VdecChn %d state %d\n", u32VdecChn, pDemuxCtrl->chnStatus);

        }
        
        /*阻塞等待模块开关*/
        vdec_tsk_DemuxWait(u32VdecChn);

        if (pVdecCtl->vdecMode == DECODE_MODE_BULL)
        {
            SAL_msleep(5);
            continue;
        }

        calcPrm.demHdl = *pHandle;
        calcPrm.orgWIdx = pDecShare->writeIdx;
        s32Ret = DemuxControl(DEM_CALC_DATA, &calcPrm, &u32Variable);
        if (u32Variable <= 255)
        {
            usleep(2 * 1000);
            continue;
        }

        s32Ret = DemuxControl(DEM_PROC_DATA, pHandle, &procOut);
        pDecShare->readIdx = procOut.orgRIdx;
        if ((!procOut.newFrm) || (DEMUX_S_OK != s32Ret))
        {
            SAL_dbPrintf(demDbLevel, DEBUG_LEVEL2, "u32VdecChn %d demuxchan %d newFrm %d s32Ret %d ridx %d widx %d\n", u32VdecChn, pVdecTskChn->decModCtrl.chnHandle, procOut.newFrm, s32Ret, pDecShare->readIdx, pDecShare->writeIdx);
            continue;
        }
        else
        {
            if (STREAM_FRAME_TYPE_AUDIO == procOut.frmType)
            {
                pDemBuf.bufType = STREAM_FRAME_TYPE_AUDIO;
                pDemBuf.addr = NULL;
                pDemBuf.length = 0;
                pDemBuf.datalen = 0;

                s32Ret = DemuxControl(DEM_FULL_BUF, pHandle, &pDemBuf);
                if (s32Ret == SAL_SOK)
                {
                    /* 不播放语音 */
                    if (0 == pVdecTskChn->vdec_chn.audPlay)
                    {
                        continue;
                    }

                    #if 1
                    if (pDemBuf.addr == NULL)
                    {
                        VDEC_LOGE("pDemBuf.addr = %p %u, %u \n", pDemBuf.addr, pDemBuf.datalen, pDemBuf.audEncType);
                        continue;
                    }

                    if (pVdecTskChn->vdec_chn.packType == DEM_PS)
                    {
                        s32Ret = audio_tsk_decPreviewWriteData(pDemBuf.addr, pDemBuf.datalen, pDemBuf.audEncType, aacAdtsHeader[u32VdecChn], 0);
                    }
                    else
                    {
                        /* pDemBuf.datalen包含了4字节的AAC音频RTP打包数据头，丢弃后加上7字节aacAdtsHeader，所以AAC音频大小增加7-4=3字节 */
                        pAdtsData = aacAdtsHeader[u32VdecChn];
                        pAdtsData[3] &= 0xfc;
                        pAdtsData[3] |= (((pDemBuf.datalen + 3) >> 11) & 0x03);
                        pAdtsData[4] &= 0x00;
                        pAdtsData[4] |= (((pDemBuf.datalen + 3) >> 3) & 0xff);
                        pAdtsData[5] &= 0x1f;
                        pAdtsData[5] |= ((((pDemBuf.datalen + 3) & 0x07) << 5) & 0xe0);
                        s32Ret = audio_tsk_decPreviewWriteData(pDemBuf.addr, pDemBuf.datalen, pDemBuf.audEncType, aacAdtsHeader[u32VdecChn], 7);
                    }

                    if (SAL_SOK != s32Ret)
                    {
                        VDEC_LOGW("warnning\n");
                        continue;
                    }

                    #endif
                }
                else
                {

                }
            }
            else if (STREAM_FRAME_TYPE_VIDEO == procOut.frmType)
            {
                u32skipDecFlg = 0;
                pDemBuf.bufType = STREAM_FRAME_TYPE_VIDEO;
                /*NT/RK平台单步运行时需要使用mmz内存，不能使用malloc出来的内存。解封装根据pDemBuf.addr地址是否为空进行拷贝*/
                if(pDemuxCtrl->dupBindType == NODE_BIND_TYPE_GET )
                {

                    if(s32Status == 0)
                    {
                        s32Ret = mem_hal_vbAlloc(MAX_VDEC_HEIGHT*MAX_PPM_VDEC_WIDTH/2,  "VDEC", "vdec_in", NULL, SAL_FALSE,&stVbInfo);
                        if (s32Ret != SAL_SOK) 
                        {
                            VDEC_LOGE("mem_hal_vbAlloc fail\n");
                            continue;
                        }
                        
                        s32Status = 1;

                    }
                    pDemBuf.length =  MAX_VDEC_HEIGHT*MAX_PPM_VDEC_WIDTH/2;
                    pDemBuf.addr = (unsigned char *)stVbInfo.pVirAddr;
                    pDemBuf.datalen = 0;
                }
                else
                {
                    pDemBuf.addr = NULL;
                    pDemBuf.length = 0;
                    pDemBuf.datalen = 0;
                }
               
                s32Ret = DemuxControl(DEM_FULL_BUF, pHandle, &pDemBuf);
                if (SAL_SOK != s32Ret)
                {
                    VDEC_LOGE("u32VdecChn %u new frame error. s32Ret:%d, %u\n", u32VdecChn, s32Ret, pDemBuf.frmType);
                }
                else
                {
                    if ((pDemBuf.width < MIN_VDEC_WIDTH) || (pDemBuf.height < MIN_VDEC_HEIGHT)
                        || (pDemBuf.width > MAX_PPM_VDEC_WIDTH) || (pDemBuf.height > MAX_VDEC_HEIGHT)
                        || ((pDemBuf.streamType != H264) && (pDemBuf.streamType != H265)) 
                        || (pDspInitPara->VdecStatus[u32VdecChn].streamVtype != pDemBuf.streamType))    /*比较类型为了外部码流切换编码类型时重新初始化解码器*/
                    {

                        VDEC_LOGW("u32VdecChn = %u streamType = %u height*width = %u * %u!!!\n", u32VdecChn, pDemBuf.streamType, pDemBuf.height, pDemBuf.width);
                        if(pDspInitPara->VdecStatus[u32VdecChn].decodedW != 0 || pDspInitPara->VdecStatus[u32VdecChn].decodedH != 0)
                        {
                             /*宽高为0时判断为第一次出化正常开启解码器送解码*/
                             u32skipDecFlg = 1;
                        }
                    }

                    if (pDemBuf.addr == NULL)
                    {
                        VDEC_LOGE("pDemBuf.addr = %p %u X %u \n", pDemBuf.addr, pDemBuf.height, pDemBuf.width);
                        continue;
                    }

                    if (pVdecTskChn->vdec_chn.packType == DEM_PS)
                    {
                        if (((pVdecCtl->frameNum + 1) != pDemBuf.frame_num) && ((pVdecCtl->u32PlaySpeed > DECODE_SPEED_8x) && ((pVdecCtl->vdecMode == DECODE_MODE_MULT)
                                                                                                                               || (pVdecCtl->vdecMode == DECODE_MODE_NORMAL))))
                        {
                            VDEC_LOGW("u32VdecChn %u old frameNum %u,cur frame_num is %u,fram_type %u\n", u32VdecChn, pVdecCtl->frameNum, pDemBuf.frame_num, vdec_tsk_DemuxisKeyFrame(pDemBuf.addr));
                        }

                        pVdecCtl->frameNum = pDemBuf.frame_num;
                        if (pDemBuf.time_info > 0)
                        {
                            pVdecCtl->frameFps = (90000 / pDemBuf.time_info);
                        }
                    }
                    else
                    {
                        if (pDemBuf.time_info > 0)
                        {
                            pVdecCtl->frameFps = (90000 / pDemBuf.time_info);
                        }
                        else
                        {
                            if ((u32CurFramePts == 0) || (u32CurFramePts > pDemBuf.timestamp))
                            {
                                u32CurFramePts = pDemBuf.timestamp;
                                u32RtpFrameFps = 1;
                            }
                            else
                            {
                                if ((pDemBuf.timestamp / 90 - u32CurFramePts / 90) > 990)
                                {
                                    pVdecCtl->frameFps = u32RtpFrameFps;
                                    u32CurFramePts = 0;
                                    u32RtpFrameFps = 0;
                                }
                                else
                                {
                                    u32RtpFrameFps++;
                                }
                            }
                        }
                    }

                    vdec_hal_VdecChnStatus(u32VdecChn, &pvdecStatus);
                   
                    if (!u32skipDecFlg && (SAL_TRUE == vdec_tsk_DemuxisKeyFrame(pDemBuf.addr))
                        && (pDspInitPara->VdecStatus[u32VdecChn].decodedW != pDemBuf.width
                        || pDspInitPara->VdecStatus[u32VdecChn].decodedH != pDemBuf.height
                        || pDspInitPara->VdecStatus[u32VdecChn].streamVtype != pDemBuf.streamType || pvdecStatus.bcreate == SAL_FALSE))
                    {
                        VDEC_LOGI("chn %d update dec old [w %d h %d type %d] new [w %d h %d type %d]\n",
                                  u32VdecChn, pDspInitPara->VdecStatus[u32VdecChn].decodedW, pDspInitPara->VdecStatus[u32VdecChn].decodedH, pDspInitPara->VdecStatus[u32VdecChn].streamVtype,
                                  pDemBuf.width, pDemBuf.height, pDemBuf.streamType);
                        s32Ret = vdec_tsk_UpdateVdec(u32VdecChn, &pDemBuf);
                        if (s32Ret != SAL_SOK)
                        {
                            u32skipDecFlg = 1;
                        }
                    }
                    
                    pDspInitPara->VdecStatus[u32VdecChn].decodedW = pDemBuf.width;
                    pDspInitPara->VdecStatus[u32VdecChn].decodedH = pDemBuf.height;
                    pDspInitPara->VdecStatus[u32VdecChn].streamVfps = pVdecCtl->frameFps;
                    pDspInitPara->VdecStatus[u32VdecChn].streamVtype = pDemBuf.streamType;
                    decFrameIn.frameParam.encodeType = pDemBuf.streamType;

                    /*码流中 可能出现重复帧 要丢弃*/
                    if (pVdecCtl->timestampLast != 0)
                    {
                        if (pVdecCtl->timestampLast >= pDemBuf.timestamp)
                        {
                            if (pVdecCtl->timestampFirst != pDemBuf.timestamp)
                            {
                                /* 拖放模式可能有重复帧 */
                                if (pVdecCtl->vdecMode != DECODE_MODE_DRAG_PLAY)
                                {
                                    VDEC_LOGI("u32VdecChn %u, frame same time [old %u,cur %u],frame type %u\n", u32VdecChn, pVdecCtl->timestampLast, pDemBuf.timestamp, vdec_tsk_DemuxisKeyFrame(pDemBuf.addr));
                                    if (vdec_tsk_DemuxisKeyFrame(pDemBuf.addr))
                                    {
                                        pVdecCtl->timestampLast = 0;
                                        pVdecCtl->timestampFirst = 0;
                                    }

                                    continue;
                                }
                            }
                        }
                    }

                    if (pVdecCtl->timestampLast == 0)
                    {
                        pVdecCtl->timestampFirst = pDemBuf.timestamp;
                    }

                    pVdecCtl->glb_time.year = pDemBuf.glb_time.year;
                    pVdecCtl->glb_time.month = pDemBuf.glb_time.month;
                    pVdecCtl->glb_time.date = pDemBuf.glb_time.date;
                    pVdecCtl->glb_time.hour = pDemBuf.glb_time.hour;
                    pVdecCtl->glb_time.minute = pDemBuf.glb_time.minute;
                    pVdecCtl->glb_time.second = pDemBuf.glb_time.second;
                    pVdecCtl->glb_time.msecond = pDemBuf.glb_time.msecond;

                    if (vdec_tsk_DemuxisKeyFrame(pDemBuf.addr))
                    {
                        pVdecCtl->milliSecond = pVdecCtl->glb_time.msecond;
                    }
                    else
                    {

                        if (pVdecTskChn->vdec_chn.packType == DEM_PS)
                        {
                            pVdecCtl->milliSecond += ((pDemBuf.timestamp - pVdecCtl->timestampLast) / 45);

                            if (((pDemBuf.timestamp - pVdecCtl->timestampLast) / 45) > 1000)
                                VDEC_LOGW("u32VdecChn %d ,de[%u,%u]\n", u32VdecChn, (pDemBuf.timestamp - pVdecCtl->timestampLast),
                                          ((pDemBuf.timestamp - pVdecCtl->timestampLast) / 45));
                        }
                        else
                        {
                            pVdecCtl->milliSecond += ((pDemBuf.timestamp - pVdecCtl->timestampLast) / 90);
                            if (((pDemBuf.timestamp - pVdecCtl->timestampLast) / 90) > 1000)
                                VDEC_LOGW("u32VdecChn %d ,de[%u,%u]\n", u32VdecChn, (pDemBuf.timestamp - pVdecCtl->timestampLast),
                                          ((pDemBuf.timestamp - pVdecCtl->timestampLast) / 90));
                        }
                    }

                    pVdecCtl->glb_time.msecond = pVdecCtl->milliSecond;

                    VDEC_LOGD("u32VdecChn %d time y:%u,m:%u,d:%u,h:%u,mi:%u,s:%u,ms:%u,milliSecond %u,ms %llu\n", u32VdecChn,
                              pDemBuf.glb_time.year, pDemBuf.glb_time.month, pDemBuf.glb_time.date,
                              pDemBuf.glb_time.hour, pDemBuf.glb_time.minute, pDemBuf.glb_time.second, pDemBuf.glb_time.msecond,
                              pVdecCtl->milliSecond, vdec_tsk_GetAabsTime(&pVdecCtl->glb_time));

                    if (((pVdecCtl->u32PlaySpeed <= DECODE_SPEED_8x) || (pVdecCtl->vdecMode == DECODE_MODE_ONLY_I_FREAM)
                         || (pVdecCtl->vdecMode == DECODE_MODE_I_TO_BMP) || (pVdecCtl->vdecMode == DECODE_MODE_DRAG_PLAY)
                         || (1 == pVdecCtl->isNewDec))
                        && (SAL_TRUE != vdec_tsk_DemuxisKeyFrame(pDemBuf.addr))) /* (*(pDemBuf.addr + 4) == 0x61)) */
                    {
                        if (1 == pVdecCtl->isNewDec)
                        {
                            VDEC_LOGD("skipFlg 1 u32VdecChn %u, nor key frame,isCopyOver %u\n", u32VdecChn, pVdecCtl->isCopyOver);
                        }

                        u32skipDecFlg = 1;
                    }

                    pVdecCtl->curFrmTimeTru = (pVdecTskChn->vdec_chn.packType == DEM_PS) ? (pDemBuf.timestamp / 45) : (pDemBuf.timestamp / 90);
                    u32SpeedCtrl = 1;
                    if (0 == u32skipDecFlg && SAL_SOK != vdec_tsk_DecSyncCtrl(u32VdecChn))
                    {
                        u32SpeedCtrl = 0;
                    }

                    if (pVdecCtl->bigDiff != 1 && SAL_TRUE == vdec_tsk_DemuxisKeyFrame(pDemBuf.addr))
                    {
                        pVdecCtl->skipVdec = SAL_FALSE;
                    }

                    if (pVdecCtl->skipVdec)
                    {
                        u32skipDecFlg = 1;
                    }

                    /*解决8倍速播放暂停失败，获取一帧暂停帧时不跳帧直接获取*/
                    if ( (1 == u32Dec_One_frame) && (pVdecCtl->u32PlaySpeed <= DECODE_SPEED_8x))
                    {
                        u32SpeedCtrl = 0;
                        u32skipDecFlg = 0;
                    }

                    if ((1 == u32SpeedCtrl) && (u32skipDecFlg == 0) && (0 == pVdecCtl->isCopyOver) && (pVdecCtl->vdecMode != DECODE_MODE_DRAG_PLAY)
                        && (pVdecCtl->vdecMode != DECODE_MODE_NORMAL))
                    {
                        /* UINT64 time0 = vdec_tsk_getTimeMilli(); */
                        if (SAL_SOK != vdec_tsk_DemuxSpeedCtrlCalc(u32VdecChn, pVdecCtl, 0))
                        {
                            VDEC_LOGW("u32VdecChn %u\n", u32VdecChn);
                            continue;
                        }

                        /* UINT64 time1 = vdec_tsk_getTimeMilli(); */
                        /* VDEC_LOGI("u32VdecChn %d time use %lld speed %d mode %d\n",u32VdecChn,time1 - time0,pVdecCtl->u32PlaySpeed,pVdecCtl->vdecMode); */

                        VDEC_LOGD("u32VdecChn %d end ctl\n", u32VdecChn);
                    }

                    if (pVdecCtl->vdecMode == DECODE_MODE_REVERSE)
                    {
                        decFrameIn.virAddr[0] = (PhysAddr)pDemBuf.addr;
                        decFrameIn.phyAddr[0] = stVbInfo.u64PhysAddr;
                        decFrameIn.bufLen = pDemBuf.datalen;
                        decFrameIn.frameParam.width = pDemBuf.width;
                        decFrameIn.frameParam.height = pDemBuf.height;

                        if (pDemBuf.height && pDemBuf.width)
                        {
                            if (u32skipDecFlg == 0)
                            {
                                pDemBuf.isKeyFrame = vdec_tsk_DemuxisKeyFrame(pDemBuf.addr);
                                s32Ret = vdec_tsk_VdecDecframe(u32VdecChn, &decFrameIn, &decFrameOut);
                                if (SAL_SOK != s32Ret)
                                {
                                    VDEC_LOGW("warnning\n");
                                    continue;
                                }
                            }

                            if (0 != pVdecCtl->timestampLast)
                            {
                                u32pbTmieDel = pDemBuf.timestamp - pVdecCtl->timestampLast;
                                if (pDemBuf.timestamp <= pVdecCtl->timestampLast)
                                {
                                    u32pbTmieDel = 0;
                                    VDEC_LOGW("u32VdecChn %d.. cur time %u,old time %u\n", u32VdecChn, pDemBuf.timestamp, pVdecCtl->timestampLast);
                                }
                            }
                            else
                            {
                                u32pbTmieDel = 0;
                            }

                            pVdecCtl->timestampLast = pDemBuf.timestamp;
                            pVdecCtl->decodePlayBackStdTime -= u32pbTmieDel;
                            pVdecCtl->decodeStandardTime = pDemBuf.timestamp;

                            pDecShare->decodeStandardTime = pVdecCtl->decodeStandardTime;
                            pDecShare->decodePlayBackStdTime = pVdecCtl->decodePlayBackStdTime;
                            /* VDEC_LOGI("dpbt %u,abst %u\n",pVdecCtl->decodePlayBackStdTime,pVdecCtl->decodeAbsTime); */
                            if (u32skipDecFlg == 0)
                            {
                                if (SAL_SOK != vdec_tsk_DemuxPlayBackProcess(u32VdecChn, &pDemBuf))
                                {
                                    continue;
                                }
                            }
                        }

                        continue;
                    }
                    else if (pVdecCtl->vdecMode == DECODE_MODE_I_TO_BMP)
                    {
                        decFrameIn.virAddr[0] = (PhysAddr)pDemBuf.addr;
                        decFrameIn.phyAddr[0] = stVbInfo.u64PhysAddr;
                        decFrameIn.bufLen = pDemBuf.datalen;
                        decFrameIn.frameParam.width = pDemBuf.width;
                        decFrameIn.frameParam.height = pDemBuf.height;

                        if (pDemBuf.height && pDemBuf.width)
                        {
                            if (u32skipDecFlg == 0)
                            {
                                if (pDemBuf.addr == NULL)
                                {
                                    VDEC_LOGE("pDemBuf.addr = %p %u X %u \n", pDemBuf.addr, pDemBuf.height, pDemBuf.width);
                                    continue;
                                }

                                /* decFrameIn.frameParam.privData = 1; */
                                s32Ret = vdec_tsk_VdecDecframe(u32VdecChn, &decFrameIn, &decFrameOut);
                                if (SAL_SOK != s32Ret)
                                {
                                    VDEC_LOGW("warnning\n");
                                    continue;
                                }
                            }

                            if (0 != pVdecCtl->timestampLast)
                            {
                                u32norTimeDel = pDemBuf.timestamp - pVdecCtl->timestampLast;
                                if (pDemBuf.timestamp <= pVdecCtl->timestampLast)
                                {
                                    u32norTimeDel = 0;
                                    VDEC_LOGW("u32VdecChn %d..cur time %u,old time %u\n", u32VdecChn, pDemBuf.timestamp, pVdecCtl->timestampLast);
                                }
                            }
                            else
                            {
                                u32norTimeDel = 0;
                            }

                            pVdecCtl->timestampLast = pDemBuf.timestamp;
                            pVdecCtl->decodeStandardTime = pDemBuf.timestamp;
                            pVdecCtl->decodeAbsTime += u32norTimeDel;

                            pDecShare->decodeAbsTime = pVdecCtl->decodeAbsTime;
                            pDecShare->decodeStandardTime = pVdecCtl->decodeStandardTime;
                            /* VDEC_LOGI("dst %u,abst %u\n",pVdecCtl->decodeStandardTime,pVdecCtl->decodeAbsTime); */
                            if (u32skipDecFlg == 0)
                            {
                                if (SAL_SOK != vdec_tsk_DemuxiFrame2BmpProcess(u32VdecChn, &pDemBuf))
                                {
                                    continue;
                                }
                            }

                            continue;

                        }
                        else
                        {
                            continue;
                        }
                    }
                    else
                    {
                        decFrameIn.virAddr[0] = (PhysAddr)pDemBuf.addr;
                        decFrameIn.phyAddr[0] = stVbInfo.u64PhysAddr;
                        decFrameIn.bufLen = pDemBuf.datalen;
                        decFrameIn.frameParam.width = pDemBuf.width;
                        decFrameIn.frameParam.height = pDemBuf.height;

						/* 人包模块计算视频流同步时戳 */
                        if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_PPM))
                        {
                            if (10 == u32VdecChn || 11 == u32VdecChn)                            
							{
                            	/* 记录I帧对应的采样时间和全局时间(ms) */
                                if (vdec_tsk_DemuxisKeyFrame(pDemBuf.addr))
                                {
                                    u32IFrmTimeStamp = pDemBuf.timestamp;
									u64IFrmGlbRelateTime = vdec_tskGetTimepFromCalender(&pDemBuf.glb_time)*1000 + pDemBuf.glb_time.msecond;
									
									VDEC_LOGD("===+++ chan %d, get I frame! pDemBuf.timestamp %d, glb: %d-%d-%d %d:%d:%d.%d \n", u32VdecChn, pDemBuf.timestamp,
										       pDemBuf.glb_time.year, pDemBuf.glb_time.month, pDemBuf.glb_time.date,
									           pDemBuf.glb_time.hour, pDemBuf.glb_time.minute, pDemBuf.glb_time.second, pDemBuf.glb_time.msecond);
                                }

                                /* 累加偏移时间，90k */
                                decFrameIn.pts = u64IFrmGlbRelateTime + \
                                                 (0 == u32IFrmTimeStamp ? 0 : (pDemBuf.timestamp - u32IFrmTimeStamp)/90);

								VDEC_LOGD("===+++ chan %d, glb: %d-%d-%d %d:%d:%d.%d \n",u32VdecChn,
									      pDemBuf.glb_time.year, pDemBuf.glb_time.month, pDemBuf.glb_time.date,
									      pDemBuf.glb_time.hour, pDemBuf.glb_time.minute, pDemBuf.glb_time.second, pDemBuf.glb_time.msecond
									      );
                            }
                            UINT64 ullTmp = vdec_tskGetTimeOfDay();

                            if ((10 == u32VdecChn && Lastpts[0] >= decFrameIn.pts)
                               || (11 == u32VdecChn && Lastpts[1] >= decFrameIn.pts))
                            {
                                SAL_ERROR("dec chan %d, cur pts %llu - last %llu = %lld \n",
                                          u32VdecChn, decFrameIn.pts, Lastpts[u32VdecChn], decFrameIn.pts-Lastpts[u32VdecChn]);
                                SAL_ERROR("demBuf time [%d-%d-%d] [%d:%d:%d.%d], timep %llu, gap %d, chan %d, stamp %d, iF %u \n", 
                                          pDemBuf.glb_time.year, pDemBuf.glb_time.month, pDemBuf.glb_time.date,
                                          pDemBuf.glb_time.hour, pDemBuf.glb_time.minute, pDemBuf.glb_time.second, pDemBuf.glb_time.msecond,
                                          decFrameIn.pts, pDemBuf.time_info, u32VdecChn, pDemBuf.timestamp, u32IFrmTimeStamp);
                                SAL_ERROR("cur sys time %llu - last %llu = %lld\n",ullTmp, LastSystime, ullTmp-LastSystime);
                            }
                            if (LastSystime > ullTmp)
                            {
                                SAL_ERROR("sys time changed!!! chan %d, cur %llu - last %llu = %lld\n",u32VdecChn, ullTmp, LastSystime, ullTmp-LastSystime);
                            }
                            LastSystime = ullTmp;

                            if(10 == u32VdecChn)                            {
                                Lastpts[0] = decFrameIn.pts;
                            }
                            if(11 == u32VdecChn)                            {
                                Lastpts[1] = decFrameIn.pts;
                            }
                        }

                        if (pDemBuf.height && pDemBuf.width)
                        {
                            if (u32skipDecFlg == 0)
                            {
                                if (pDemBuf.addr == NULL)
                                {
                                    VDEC_LOGE("pDemBuf.addr = %p %u X %u \n", pDemBuf.addr, pDemBuf.height, pDemBuf.width);
                                    continue;
                                }

                                s32Ret = vdec_tsk_VdecDecframe(u32VdecChn, &decFrameIn, &decFrameOut);
                                /* VDEC_LOGI("u32VdecChn %d dec success \n",u32VdecChn); */
                                pVdecCtl->isNewDec = 0;
                            }

                            if (0 != pVdecCtl->timestampLast)
                            {
                                u32norTimeDel = pDemBuf.timestamp - pVdecCtl->timestampLast;
                                if (pDemBuf.timestamp <= pVdecCtl->timestampLast)
                                {
                                    u32norTimeDel = 0;
                                    if (pVdecCtl->vdecMode != DECODE_MODE_DRAG_PLAY)
                                    {
                                        VDEC_LOGW("u32VdecChn %d..cur time %u,old time %u,skipflg %u,is Key frame %u\n", u32VdecChn, pDemBuf.timestamp, pVdecCtl->timestampLast, u32skipDecFlg, vdec_tsk_DemuxisKeyFrame(pDemBuf.addr));
                                    }
                                }
                            }
                            else
                            {
                                u32norTimeDel = 0;
                            }

                            pVdecCtl->timestampLast = pDemBuf.timestamp;
                            pVdecCtl->decodeStandardTime = pDemBuf.timestamp;
                            pVdecCtl->decodeAbsTime += u32norTimeDel;
                            pDecShare->decodeAbsTime = pVdecCtl->decodeAbsTime;
                            pDecShare->decodeStandardTime = pVdecCtl->decodeStandardTime;

                            if ((SAL_SOK != s32Ret) && (u32skipDecFlg == 0))
                            {
                                VDEC_LOGE("u32VdecChn [%u] dec cur frame failed\n", u32VdecChn);
                                continue;
                            }
                            else
                            {
                                continue;
                            }
                        }
                        else
                        {
                            continue;
                        }
                    }
                }
            }
            else
            {
                pDemBuf.bufType = STREAM_FRAME_TYPE_PRIVT;
                pDemBuf.addr = NULL;
                pDemBuf.length = 0;
                pDemBuf.datalen = 0;
                s32Ret = DemuxControl(DEM_FULL_BUF, pHandle, &pDemBuf);
                if (s32Ret)
                {
                }
                else
                {
                    if ((pVdecCtl->vdecMode == DECODE_MODE_DRAG_PLAY) || (pVdecCtl->vdecMode == DECODE_MODE_MULT))
                    {
                        vca_unpack_process(u32VdecChn, &pDemBuf);
                    }
                    else
                    {
                        vca_unpack_clearResult(u32VdecChn);
                    }
                }
            }
        }

        usleep(2 * 1000);
    }

    return NULL;

}

/**
 * @function    vdec_tsk_MemQuickCpoy
 * @brief   快速拷贝
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_MemQuickCpoy(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    CHECK_PTR_NULL(prm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    return s32Ret;
}

/**
 * @function    vdec_tsk_SetDbLeave
 * @brief   设置log等级
 * @param[in] int level 等级
 * @param[out] None
 * @return  static INT32
 */
void vdec_tsk_SetDbLeave(int level)
{
    demDbLevel = (level > 0) ? level : 0;
    VDEC_LOGI("vdecDbLevel %d\n", demDbLevel);
}

/**
 * @function   vdec_tsk_SetOutChnDataFormat
 * @brief      更新解码器绑定的vpss输出通道的图像格式
 * @param[in]  UINT32 u32VdecChn                 
 * @param[in]  UINT32 u32OutChn                  
 * @param[in]  SAL_VideoDataFormat enDataFormat  
 * @param[out] None
 * @return     INT32
 */
INT32 vdec_tsk_SetOutChnDataFormat(UINT32 u32VdecChn, UINT32 u32OutChn, SAL_VideoDataFormat enDataFormat)
{
    INT32 s32Ret = SAL_SOK;
	
    VDEC_HAL_STATUS stVdecStatus = {0};
    PARAM_INFO_S stParamInfo = {0};
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    DUP_ChanHandle *pDupChanHandle = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));

    s32Ret = vdec_hal_VdecJpegSupport();
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("vdec not support jpeg dec\n");
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NOT_SUPPORT);
    }

    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];

    vdec_hal_VdecChnStatus(u32VdecChn, &stVdecStatus);
    if (stVdecStatus.bcreate == 0)
    {
    	/* 解码器未创建则返回失败 */
		VDEC_LOGW("vdec %d not created! \n", u32VdecChn);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_UNEXIST);
    }

    pDupChanHandle = pVdecTskChn->decModCtrl.dupChanHandle[u32OutChn];
    CHECK_PTR_NULL(pDupChanHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    stParamInfo.enType = PIX_FORMAT_CFG;
    stParamInfo.enPixFormat = enDataFormat;

    s32Ret = pDupChanHandle->dupOps.OpDupSetBlitPrm(pDupChanHandle, &stParamInfo);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d set pixel %d err ret 0x%x\n", u32VdecChn, stParamInfo.enPixFormat, s32Ret);
        return s32Ret;
    }

    return SAL_SOK;
}

/**
 * @function	vdec_tsk_SetOutChnResolution
 * @brief	配置解码输出通道宽高
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] UINT32 u32OutChn 子通道
 * @param[in] UINT32 width 宽
 * @param[in] UINT32 height 高
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_SetOutChnResolution(UINT32 u32VdecChn, UINT32 u32OutChn, UINT32 width, UINT32 height)
{
    INT32 s32Ret = SAL_SOK;
	
    VDEC_HAL_STATUS stVdecStatus = {0};
    PARAM_INFO_S stParamInfo = {0};
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    DUP_ChanHandle *pDupChanHandle = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));

    if (width > MAX_PPM_VDEC_WIDTH || width < MIN_VDEC_WIDTH || height > MAX_VDEC_HEIGHT || height < MIN_VDEC_HEIGHT)
    {
        VDEC_LOGE("chn %d w %d h %d err\n", u32VdecChn, width, height);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM);
    }

    s32Ret = vdec_hal_VdecJpegSupport();
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("vdec not support jpeg dec\n");
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NOT_SUPPORT);
    }

    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];

    vdec_hal_VdecChnStatus(u32VdecChn, &stVdecStatus);
    if (stVdecStatus.bcreate == 0)
    {
    	/* 解码器未创建则返回失败 */
		VDEC_LOGW("vdec %d not created! \n", u32VdecChn);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_UNEXIST);
    }

    pDupChanHandle = pVdecTskChn->decModCtrl.dupChanHandle[u32OutChn];
    CHECK_PTR_NULL(pDupChanHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
	
    stParamInfo.enType = IMAGE_SIZE_CFG;
    stParamInfo.stImgSize.u32Width = width;
    stParamInfo.stImgSize.u32Height = height;

    s32Ret = pDupChanHandle->dupOps.OpDupSetBlitPrm(pDupChanHandle, &stParamInfo);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d set prm width %d height %d err ret 0x%x\n", u32VdecChn, width, height, s32Ret);
        return s32Ret;
    }

    return SAL_SOK;
}

/**
 * @function    vdec_tsk_ScaleJpegChn
 * @brief   jpeg缩放
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] UINT32 chn 子通道
 * @param[in] UINT32 width 宽
 * @param[in] UINT32 height 高
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_ScaleJpegChn(UINT32 u32VdecChn, UINT32 chn, UINT32 width, UINT32 height)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_STATUS pvdecStatus = {0};
    DEM_BUFFER pDemBuf = {0};
    PARAM_INFO_S pstParamInfo = {0};
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    DUP_ChanHandle *pDupChanHandle = NULL;
    DSPINITPARA *pstDspInfo = NULL;
    
    pstDspInfo = SystemPrm_getDspInitPara();
    if (pstDspInfo == NULL)
    {
        VDEC_LOGE("error\n");
        return SAL_FAIL;
    }

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));

    if (width > MAX_PPM_VDEC_WIDTH || width < MIN_VDEC_WIDTH || height > MAX_VDEC_HEIGHT || height < MIN_VDEC_HEIGHT)
    {
        VDEC_LOGE("chn %d w %d h %d err\n", u32VdecChn, width, height);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM);
    }

    s32Ret = vdec_hal_VdecJpegSupport();
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("vdec not support jpeg dec\n");
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NOT_SUPPORT);
    }

    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];

	/* 
	   sunzelin add@2022.8.31

	   !!!注意视频解码通道不能调用此接口!!!
	   此接口目前仅用于应用触发的jpeg转bmp使用，解码器类型固定为mjpeg类型
	   关于此处为何需要调用update接口频繁销毁、创建解码通道，目前暂无明确解释原因 
	*/
	
    vdec_hal_VdecChnStatus(u32VdecChn, &pvdecStatus);
    if (pvdecStatus.bcreate == 0)
    {
        pDemBuf.width = MAX_VDEC_WIDTH;
        pDemBuf.height = MAX_VDEC_HEIGHT;
        pDemBuf.streamType = MJPEG;
        s32Ret = vdec_tsk_UpdateVdec(u32VdecChn, &pDemBuf);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("chn %d update vdec failed ret 0x%x\n", u32VdecChn, s32Ret);
            return s32Ret;
        }
    }

    pDupChanHandle = pVdecTskChn->decModCtrl.dupChanHandle[chn];
    CHECK_PTR_NULL(pDupChanHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pstParamInfo.enType = IMAGE_SIZE_CFG;
    pstParamInfo.stImgSize.u32Width = width;
    pstParamInfo.stImgSize.u32Height = height;

    s32Ret = pDupChanHandle->dupOps.OpDupSetBlitPrm(pDupChanHandle, &pstParamInfo);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d set prm width %d height %d err ret 0x%x\n", u32VdecChn, width, height, s32Ret);
        return s32Ret;
    }
	
    if(pstDspInfo->dspCapbPar.dev_tpye == PRODUCT_TYPE_ISM)
    {
        DupHandle dupHandle;
        PARAM_INFO_S stParamInfo = {0};
        dupHandle = pVdecTskChn->decModCtrl.rearDupHandle;
        stParamInfo.enType = MIRROR_CFG;
        stParamInfo.stMirror.u32Mirror = SAL_FALSE;
        stParamInfo.stMirror.u32Flip = SAL_TRUE;
        s32Ret = dup_task_setDupParam(dupHandle, &stParamInfo);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("chn %d set stMirror u32Flip err\n", u32VdecChn);
            //return s32Ret;
        }
    }

    return SAL_SOK;
}

/**
 * @function    vdec_tsk_SendJpegFrame
 * @brief   解码jpeg图片
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] VOID *pBuf 数据
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_SendJpegFrame(UINT32 u32VdecChn, VOID *pAddr, UINT32 u32StrLen)
{
    INT32 s32Ret = SAL_SOK;
    SAL_VideoFrameBuf pstSrcframe = {0};
    SAL_VideoFrameBuf pstDstframe = {0};
    VDEC_HAL_STATUS pvdecStatus = {0};
    DEM_BUFFER pDemBuf = {0};

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(pAddr, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    s32Ret = vdec_hal_VdecJpegSupport();
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("vdec not support jpeg dec\n");
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NOT_SUPPORT);
    }

    vdec_hal_VdecChnStatus(u32VdecChn, &pvdecStatus);
    if (pvdecStatus.bcreate == 0)
    {
        pDemBuf.width = MAX_VDEC_WIDTH;
        pDemBuf.height = MAX_VDEC_HEIGHT;
        pDemBuf.streamType = MJPEG;
        s32Ret = vdec_tsk_UpdateVdec(u32VdecChn, &pDemBuf);
        if (s32Ret != SAL_SOK)
        {
            VDEC_LOGE("chn %d update vdec failed ret 0x%x\n", u32VdecChn, s32Ret);
            return s32Ret;
        }
    }

    pstSrcframe.encodeType = MJPEG;
    pstSrcframe.bufLen = u32StrLen;
    pstSrcframe.virAddr[0] = (PhysAddr)pAddr;

    s32Ret = vdec_tsk_VdecDecframe(u32VdecChn, &pstSrcframe, &pstDstframe);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d dec jpeg frame failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    return SAL_SOK;
}

/**
 * @function    vdec_tsk_GetJpegYUVFrame
 * @brief   解码jpeg后的yuv数据
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] UINT32 chn 子通道
 * @param[in] void *pBuf 数据
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_GetJpegYUVFrame(UINT32 u32VdecChn, UINT32 chn, void *pBuf)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 needFree = 0;
    static SYSTEM_FRAME_INFO pstFrame[MAX_VDEC_PLAT_CHAN] = {0};

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(pBuf, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pstFrame[u32VdecChn].uiAppData = (PhysAddr)pBuf;

    s32Ret = vdec_hal_VdecJpegSupport();
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("vdec not support jpeg dec\n");
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NOT_SUPPORT);
    }

    s32Ret = vdec_tsk_GetVideoFrmaeMeth(u32VdecChn, chn, &needFree, &pstFrame[u32VdecChn]);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d get frame err ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

/**
 * @function    vdec_tsk_JpgDecBuffInit
 * @brief   
 * @param[in] 
 * @param[in] 
 * @param[out] None
 * @return  static INT32
 */

INT32 vdec_tsk_JpgDecBuffInit()
{
    INT32 s32Ret = 0;
    UINT32 uiBlkSize = 0;
    ALLOC_VB_INFO_S stVbInfo = {0};
    VDEC_TSK_JPGTOPMP_PRM *pstJgb2BmpPrm = NULL;
    VDEC_JPG_BUFF_ST *pstJdecBuf = NULL;
    VDEC_JPG_BUFF_ST *pstJpg2YuvBuf = NULL;
    SYSTEM_FRAME_INFO *pstSystemFrmInfo = NULL;

    pstJgb2BmpPrm = &gJpgToBmpPrm;

    pstSystemFrmInfo = &pstJgb2BmpPrm->stSystemFrmInfo;
    memset(pstSystemFrmInfo, 0x00, sizeof(SYSTEM_FRAME_INFO));
    if (pstSystemFrmInfo->uiAppData == 0x00)
    {
        if (SAL_SOK != sys_hal_allocVideoFrameInfoSt(pstSystemFrmInfo))
        {
            VDEC_LOGE("Disp_halGetFrameMem error !\n");
            return SAL_FAIL;
        }
    }

    pstJdecBuf = &pstJgb2BmpPrm->stJpgDecBuff;

    uiBlkSize = VDEC_JPG_W_MAX * VDEC_JPG_H_MAX * 4;
    s32Ret = mem_hal_vbAlloc(uiBlkSize, "vdec", "vdec_jpg", NULL, SAL_TRUE,  &stVbInfo);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("Alloc Vb Buf For Jdec Failed! uiBlkSize %d\n", uiBlkSize);
        return SAL_FAIL;
    }
    pstJdecBuf->u32Pool = stVbInfo.u32PoolId;
    pstJdecBuf->uiUseFlag = SAL_FALSE;
    pstJdecBuf->uiBufSize = uiBlkSize;
    pstJdecBuf->PhyAddr = (UINT64)stVbInfo.u64PhysAddr;
    pstJdecBuf->VirAddr = (UINT64)stVbInfo.pVirAddr;
    

    pstJpg2YuvBuf = &pstJgb2BmpPrm->stJpg2YuvBuf;
    uiBlkSize = VDEC_JPG_W_MAX * VDEC_JPG_H_MAX * 1.5;
    
    memset(&stVbInfo, 0, sizeof(ALLOC_VB_INFO_S));
    s32Ret = mem_hal_vbAlloc(uiBlkSize, "vdec", "vdec_jpg", NULL, SAL_FALSE, &stVbInfo);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("Alloc Vb Buf For Jdec Failed! uiBlkSize %d\n", uiBlkSize);
        return SAL_FAIL;
    }

    pstJpg2YuvBuf->u32Pool = stVbInfo.u32PoolId;
    pstJpg2YuvBuf->uiUseFlag = SAL_FALSE;
    pstJpg2YuvBuf->uiBufSize = uiBlkSize;
    pstJpg2YuvBuf->PhyAddr = (UINT64)stVbInfo.u64PhysAddr;
    pstJpg2YuvBuf->VirAddr = (UINT64)stVbInfo.pVirAddr;
    pstJpg2YuvBuf->vbBlk = stVbInfo.u64VbBlk;

    return SAL_SOK;
}


/**
 * @function    Vdec_halJpgToYuv
 * @brief   
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] DEM_BUFFER *pDemBuf 数据
 * @param[out] None
 * @return  static INT32
 */

INT32 vdec_tskJpgToYuv(DECODER_JPG_BMP_DATA *pstJpgData, void *pFrame)
{
    UINT64 time0 = 0, time1 = 0, time2 = 0, time3 = 0;

    UINT32 uiJpegLen = 0;
    UINT32 uiWidth = 0;
    UINT32 uiHeight = 0;
    UINT32 uiProgressiveMode = 0;
    UINT32 u32Pool = 0;
    PhysAddr uiPhyAddr = 0;

    UINT32 s32Ret = SAL_FAIL;

    SYSTEM_FRAME_INFO SrcVpssFrame = {0};
    HKAJPGD_ABILITY stAbility = {0};
    HKAJPGD_STREAM stJpegData = {0};

    SYSTEM_FRAME_INFO *pstFrameInfo = NULL;
    VDEC_TSK_JPGTOPMP_PRM *pstJgb2BmpPrm = NULL;
    VDEC_JPG_BUFF_ST *pstJdecBufData = NULL;
    VDEC_JPG_BUFF_ST *pstJpg2YuvBuff = NULL;
    SAL_VideoFrameBuf stDstFrame;
    SAL_VideoFrameBuf stSrcFrame;
    void *pVirAddr = NULL;
    UINT32 VdecChn = 0;
    UINT32 vpssChn = 0;
    DSPINITPARA *pstDspInfo = NULL;
    
    pstDspInfo = SystemPrm_getDspInitPara();
    if (pstDspInfo == NULL)
    {
        VDEC_LOGE("error\n");
        return SAL_FAIL;
    }

    pstFrameInfo = (SYSTEM_FRAME_INFO *)pFrame;


    pstJgb2BmpPrm = &gJpgToBmpPrm;
    if (NULL == pstJgb2BmpPrm)
    {
        VDEC_LOGE("Get jpg2bmp global prm Failed!\n");
        return SAL_FAIL;
    }

    uiJpegLen = pstJpgData->u32DataSize;

    if (pstJpgData->u32DataSize > VDEC_JPG_H_MAX * VDEC_JPG_W_MAX * 4 || uiJpegLen > pstJpgData->u32BuffSize)
    {
        VDEC_LOGE("Jpeg size %d > Max Size %d or buff size %d \n", pstJpgData->u32DataSize, VDEC_JPG_H_MAX * VDEC_JPG_W_MAX * 4, pstJpgData->u32BuffSize);
        return SAL_FAIL;
    }

    time0 =sal_get_tickcnt();
    pstJdecBufData = &pstJgb2BmpPrm->stJpgDecBuff;
    memset((void *)pstJdecBufData->VirAddr, 0, pstJdecBufData->uiBufSize);

    /* 拷贝用户态数据到Jdec Buf */
    memcpy((void *)pstJdecBufData->VirAddr, pstJpgData->pDataBuff, pstJpgData->u32DataSize);

    /* 对获取的解码图片信息进行判断，硬解还是软解 */
    stJpegData.stream_buf = (UINT8 *)pstJdecBufData->VirAddr;
    stJpegData.stream_len = uiJpegLen;

    s32Ret = HKAJPGD_GetImageInfo(&stJpegData, &stAbility.image_info);
    if (HIK_VIDEO_DEC_LIB_S_OK != s32Ret)
    {
        VDEC_LOGE(" HKAJPGD_GetImageInfo ERR! s32Ret:0x%x\n", s32Ret);
        return SAL_FAIL;
    }

    time1 = sal_get_tickcnt();
    
    uiWidth = stAbility.image_info.img_size.width;
    uiHeight = stAbility.image_info.img_size.height;
    uiProgressiveMode = stAbility.image_info.progressive_mode;

    if (uiWidth > 1920 || uiHeight > 1080 )
    {
        VDEC_LOGE(" uiWidth [%d %d] uiHeight[%d %d] is error\n", uiWidth, pstJpgData->u32Width, uiHeight, pstJpgData->u32Height);
        return SAL_FAIL;
    }

    VDEC_LOGI("ability: progressive_mode %d num_components %d w %d h %d pix_format %d\n",
             stAbility.image_info.progressive_mode,
             stAbility.image_info.num_components,
             stAbility.image_info.img_size.width,
             stAbility.image_info.img_size.height,
             stAbility.image_info.pix_format);

    VDEC_LOGI("Get Jpeg cost %llu ms, w %d h %d mode %d \n",
             time1 - time0, uiWidth, uiHeight, uiProgressiveMode);

    if (uiProgressiveMode)
    {
        VDEC_LOGE("jpg dec not support the pic!\n");
        return SAL_FAIL;
    }


    time2 = sal_get_tickcnt();
    VdecChn = pstDspInfo->decChanCnt -1;
    /* 更新解码输出通道宽高 */
    uiWidth = SAL_align(uiWidth, 2);
    uiHeight = SAL_align(uiHeight, 2);
    
    s32Ret = vdec_tsk_ScaleJpegChn(VdecChn, vpssChn, uiWidth, uiHeight); 
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("VdecChn %d Update Jpeg Output Chn Failed!\n", VdecChn);
        return SAL_FAIL;
    }

    /* jpg数据送解码 */
    s32Ret = vdec_tsk_SendJpegFrame(VdecChn, (void *)pstJdecBufData->VirAddr,uiJpegLen);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("VdecChn %d Send Frame Failed!\n", VdecChn);
        return SAL_FAIL;
    }

    usleep(20*1000);
    if (SrcVpssFrame.uiAppData == 0x00)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&SrcVpssFrame);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("vdec_tskJpgToYuv error !\n");
            return SAL_FAIL;
        }

    }

    /* 获取解码数据 */
    s32Ret = vdec_tsk_GetJpegYUVFrame(VdecChn, vpssChn, (void *)SrcVpssFrame.uiAppData);
    if (SAL_SOK != s32Ret)
    {
        sys_hal_rleaseVideoFrameInfoSt(&SrcVpssFrame);
        VDEC_LOGE(" VdecChn %d Get Dec Yuv Failed s32Ret 0x%x!\n", VdecChn, s32Ret);
       return SAL_FAIL;
    }

    memset(&stSrcFrame,0x00,sizeof(SAL_VideoFrameBuf));
    sys_hal_getVideoFrameInfo(&SrcVpssFrame, &stSrcFrame);

    pstJpg2YuvBuff = &pstJgb2BmpPrm->stJpg2YuvBuf;
    uiPhyAddr = pstJpg2YuvBuff->PhyAddr;
    pVirAddr = (void *)pstJpg2YuvBuff->VirAddr;
    u32Pool = pstJpg2YuvBuff->u32Pool;

    memset(&stDstFrame,0x00,sizeof(SAL_VideoFrameBuf));
    SAL_halMakeFrame(pVirAddr,uiPhyAddr,u32Pool,uiWidth, uiHeight, uiWidth,SAL_VIDEO_DATFMT_YUV420SP_UV,&stDstFrame);
    sys_hal_buildVideoFrame(&stDstFrame,pstFrameInfo);

	sys_hal_getVideoFrameInfo(pstFrameInfo, &stDstFrame);

    TDE_HAL_SURFACE srcSurface = {0};
    TDE_HAL_SURFACE dstSurface = {0};
    TDE_HAL_RECT srcRect = {0};
    TDE_HAL_RECT dstRect = {0};

    srcSurface.enColorFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    dstSurface.enColorFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;

    srcSurface.u32Width = uiWidth;
    srcSurface.u32Height = uiHeight;
    srcSurface.u32Stride = uiWidth;
    srcSurface.PhyAddr = stSrcFrame.phyAddr[0];
	srcSurface.vbBlk = stSrcFrame.vbBlk;
    srcRect.s32Xpos = 0;
    srcRect.s32Ypos = 0;
    srcRect.u32Width = uiWidth;
    srcRect.u32Height = uiHeight;

    dstSurface.u32Width = uiWidth;
    dstSurface.u32Height = uiHeight;
    dstSurface.u32Stride = uiWidth;
    dstSurface.PhyAddr = stDstFrame.phyAddr[0];
	dstSurface.vbBlk = stDstFrame.vbBlk;
    dstRect.s32Xpos = 0;
    dstRect.s32Ypos = 0;
    dstRect.u32Width = uiWidth;
    dstRect.u32Height = uiHeight;

    s32Ret = tde_hal_QuickCopy(&srcSurface, &srcRect, &dstSurface, &dstRect, SAL_FALSE);
    if (SAL_SOK != s32Ret)
    {
        sys_hal_rleaseVideoFrameInfoSt(&SrcVpssFrame);
        VDEC_LOGE(" VdecChn %d Get Dec Yuv Failed s32Ret 0x%x!\n", VdecChn, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = vdec_tsk_DupPutfreame(VdecChn, 0, &SrcVpssFrame);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("Vdec Put Vdec Frame Failed VdecChn %d ! \n", VdecChn);
        return SAL_FAIL;
    }
    sys_hal_rleaseVideoFrameInfoSt(&SrcVpssFrame);

    time3 = sal_get_tickcnt();
    VDEC_LOGI("Get HW Jdec Frame cost %llu ms, w %d h %d\n", time3 - time2, uiWidth,uiHeight);

    return SAL_SOK;
}

/**
 * @function    Vdec_drvJpgToBmp
 * @brief   jpg转bmp
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] SAL_VideoFrameBuf *pstSrcframe 数据源
 * @param[in]  SAL_VideoFrameBuf *pstDstframe 解码后数据
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tskYuvToBmp(DECODER_JPG_BMP_DATA *pstBmpData, void *pFrame)
{
    INT32 s32Ret = 0;
    UINT32 uiWidth = 0;
    UINT32 uiHeight = 0;
    UINT32 uiStride = 0;
    UINT32 offset = 10; /* BMP的头是54个字节的，默认偏移10个字节，16对齐，用于硬件加速地址16对齐 */
    SYSTEM_FRAME_INFO *pstFrameInfo = NULL;
    VDEC_TSK_JPGTOPMP_PRM *pstJgb2BmpPrm = NULL;
    UINT64 time_yuv2bgra_start = 0;
    UINT64 time_yuv2bgra_end = 0;
    UINT64 time3 = 0;
    UINT64 bgrPhyAddr = 0;
    UINT64 bgrVirAddr = 0;
    UINT8 *pData = NULL;
    SAL_VideoFrameBuf stVideoFrame = {0};
    BITMAPFILEHEADER stFHdr;
    BITMAPINFOHEADER stInfoHdr;

    IVE_HAL_IMAGE stIveYuv = {0};
    IVE_HAL_IMAGE stIveRgb = {0};
    IVE_HAL_MODE_CTRL stCscCtrl = {0};

    pstFrameInfo = (SYSTEM_FRAME_INFO *)pFrame;
    sys_hal_getVideoFrameInfo(pstFrameInfo, &stVideoFrame);
    uiWidth = stVideoFrame.frameParam.width;
    uiHeight = stVideoFrame.frameParam.height;
    uiStride = stVideoFrame.stride[0];

    if (0 == stVideoFrame.phyAddr[0] || 0 == stVideoFrame.phyAddr[1])
    {
        VDEC_LOGE("y_phyAddr or uv_phyAddr NULL! \n");
        return SAL_FAIL;
    }

    time_yuv2bgra_start = sal_get_tickcnt();

    pstJgb2BmpPrm = &gJpgToBmpPrm;;
    if (NULL == pstJgb2BmpPrm)
    {
        VDEC_LOGE("Get Bgra Data Tab Failed! \n");
        return SAL_FAIL;
    }

    bgrPhyAddr = pstJgb2BmpPrm->stJpgDecBuff.PhyAddr;
    bgrVirAddr = pstJgb2BmpPrm->stJpgDecBuff.VirAddr;

    stFHdr.bfType = 0x4D42;
    stFHdr.bfSize = 54 + uiWidth * uiHeight * 3; /* file header + info header + bmp data */
    stFHdr.bfReserved1 = 0;
    stFHdr.bfReserved2 = 0;
    stFHdr.bfOffBits = 54;        /* file header + info header + color palette. */

    stInfoHdr.biSize = 0x28;
    stInfoHdr.biWidth = uiWidth;
    stInfoHdr.biHeight = uiHeight;
    stInfoHdr.biPlanes = 1;        /* always 1 */
    stInfoHdr.biBitCount = 24;     /* RGB888 */
    stInfoHdr.biCompression = 0;
    stInfoHdr.biSizeImage = uiWidth * uiHeight * 3;       /* size of actual bmp data, Bytes. */
    stInfoHdr.biXPelsPerMeter = 0;
    stInfoHdr.biYPelsPerMeter = 0;
    stInfoHdr.biClrUsed = 0;        /* number of color used. 0: all colors may be used. */
    stInfoHdr.biClrImportant = 0;

    /* bmp data storage : B->G->R->A */
    pData = (UINT8 *)(bgrVirAddr + offset);
    memcpy(pData, &stFHdr, sizeof(BITMAPFILEHEADER));
    offset += sizeof(BITMAPFILEHEADER);

    pData = (UINT8 *)bgrVirAddr + offset;
    memcpy(pData, &stInfoHdr, sizeof(BITMAPINFOHEADER));
    offset += sizeof(BITMAPINFOHEADER);

    stIveYuv.enColorType = SAL_VIDEO_DATFMT_YUV420SP_UV;       /*YUV420 SemiPlanar*/
    stIveYuv.u64VirAddr[0] = stVideoFrame.virAddr[0];
    stIveYuv.u64VirAddr[1] = stVideoFrame.virAddr[1];
    stIveYuv.u64PhyAddr[0] = stVideoFrame.phyAddr[0];
    stIveYuv.u64PhyAddr[1] = stVideoFrame.phyAddr[1];
    stIveYuv.u32Stride[0] = uiStride;  /*需要16对齐*/
    stIveYuv.u32Stride[1] = uiStride;
    stIveYuv.u32Width = uiWidth;
    stIveYuv.u32Height = uiHeight;

    stIveRgb.enColorType = SAL_VIDEO_DATFMT_RGB24_888;       
    stIveRgb.u64VirAddr[0] = (UINT64)(bgrVirAddr);
    stIveRgb.u64VirAddr[1] = stIveRgb.u64VirAddr[0] + uiStride * uiHeight;
    stIveRgb.u64VirAddr[2] = stIveRgb.u64VirAddr[1] + uiStride * uiHeight;
    stIveRgb.u64PhyAddr[0] = bgrPhyAddr;
    stIveRgb.u64PhyAddr[1] = stIveRgb.u64PhyAddr[0] + uiStride * uiHeight;
    stIveRgb.u64PhyAddr[2] = stIveRgb.u64PhyAddr[1] + uiStride * uiHeight;
    stIveRgb.u32Stride[0] = uiStride;
    stIveRgb.u32Stride[1] = uiStride;
    stIveRgb.u32Stride[2] = uiStride;
    stIveRgb.u32Width = uiWidth;
    stIveRgb.u32Height = uiHeight;

    /* 海思ive模块：输出B G        R模式，地址16对齐*/
    stCscCtrl.u32enMode = IVE_CSC_PIC_BT709_YUV2RGB,
    s32Ret = ive_hal_CSC(&stIveYuv, &stIveRgb, &stCscCtrl);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("HI_MPI_IVE_CSC is error s32Ret with %x!\n ", s32Ret);
        return SAL_FAIL;
    }
/*
    该处是规避的位置，需要注意
 */
    bgrVirAddr += 10; /* BMP的起始地址默认向后偏10个字节，原内存的前10个字节被跳过用于16字节对齐了 */
    pstBmpData->u32DataSize = stFHdr.bfSize;
    pstBmpData->u32Width = uiWidth;
    pstBmpData->u32Height = uiHeight;
    time_yuv2bgra_end = sal_get_tickcnt();
    if (pstBmpData->u32DataSize > pstBmpData->u32BuffSize)
    {
        VDEC_LOGE("bmp u32BuffSize %d < u32DataSize %d\n",pstBmpData->u32BuffSize, pstBmpData->u32DataSize);
        return SAL_FAIL;
    }
    memcpy(pstBmpData->pDataBuff, (UINT8 *)bgrVirAddr, pstBmpData->u32DataSize);

    time3 = sal_get_tickcnt();
    pstJgb2BmpPrm->stJpgDecBuff.uiUseFlag = SAL_FALSE;
    SAL_INFO("Register: yuv to bgra cost %llu ms memcpy %llu ms, ret %d\n", time_yuv2bgra_end - time_yuv2bgra_start,time3 - time_yuv2bgra_end, s32Ret);
    return SAL_SOK;
}



/**
 * @function    Vdec_drvJpgToBmp
 * @brief   jpg转bmp
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] SAL_VideoFrameBuf *pstSrcframe 数据源
 * @param[in]  SAL_VideoFrameBuf *pstDstframe 解码后数据
 * @param[out] None
 * @return  static INT32
 */

INT32 vdec_tsk_JpgToBmp(DECODER_JPGTOBMP_PRM *pstDecJpg2BmpPrm)
{
    INT32 s32Ret = SAL_FAIL;
    static UINT8 JpgDecBuffInitstaue = 0;
    SYSTEM_FRAME_INFO *pstSystemFrmInfo = NULL;
    DECODER_JPG_BMP_DATA *stJpgDataPrm = NULL;
    DECODER_JPG_BMP_DATA *stBmpDataPrm = NULL;
    VDEC_TSK_JPGTOPMP_PRM *pstJgb2BmpPrm = NULL;
    DSPINITPARA *pstDspInfo = SystemPrm_getDspInitPara();

    if (NULL == pstDecJpg2BmpPrm)
    {
        VDEC_LOGE("error \n");
        return SAL_FAIL;
    }

    stJpgDataPrm = &pstDecJpg2BmpPrm->stJpgData;
    stBmpDataPrm = &pstDecJpg2BmpPrm->stBmpData;
    if (0 != pstDspInfo->decChanCnt && JpgDecBuffInitstaue == 0)
    {
        if (SAL_SOK != vdec_tsk_JpgDecBuffInit())
        {
            XPACK_LOGE("vdec_tsk_JpgDecBuffInit !!!\n");
            return SAL_FAIL; 
        }
        JpgDecBuffInitstaue = 1;
    }

    pstJgb2BmpPrm = &gJpgToBmpPrm;
    pstSystemFrmInfo = &pstJgb2BmpPrm->stSystemFrmInfo;
    if (pstSystemFrmInfo->uiAppData == 0x00)
    {
        VDEC_LOGE("pstJgb2BmpPrm global buff init Failed!\n");
        return SAL_FAIL;
    }
     
    s32Ret = vdec_tskJpgToYuv(stJpgDataPrm, (void *)pstSystemFrmInfo);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("Vdec_halJpgToYuv error \n");
        return SAL_FAIL;
    }

    s32Ret = vdec_tskYuvToBmp(stBmpDataPrm, (void *)pstSystemFrmInfo);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function    vdec_tsk_SetDispPrm
 * @brief   解码jpeg后的yuv数据
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] DISP_WINDOW_PRM prm 显示参数
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_SetDispPrm(UINT32 u32VdecChn,DISP_WINDOW_PRM *prm)
{
    VDEC_TASK_CTL *pVdecTskCtl = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));
    CHECK_PTR_NULL(prm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecTskCtl = &g_stVecTaskCtl[u32VdecChn];

    memcpy(&pVdecTskCtl->dWPrm,prm,sizeof(DISP_WINDOW_PRM));

    return SAL_SOK;
}

/**
 * @function    Vdec_task_PpmDupChnHandleCreate
 * @brief       创建用于跟解码器绑定使用的dup
 * @param[in]   stInstCfg 绑定时需要用到的实例配置信息，在link_task.c里配置
                u32VdecChn 解码通道
 * @param[out]  pFrontDupHandle 新创建的前级dup对应的handle
                pRearDupHandle  新创建的后级dup对应的handle
 * @return
 */
INT32 Vdec_task_PpmDupChnHandleCreate(UINT32 u32VdecChn, void* pFrontDupHandle)
{
    UINT32 s32Ret = 0;
    INST_INFO_S *pstInst = NULL;
    CHAR szOutNodeNm[NAME_LEN] = {0};
    CHAR szInstPreName[NAME_LEN] = "DUP_VDEC";
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    DUP_BIND_PRM pSrcBuf = {0};
    DupHandle* pstDupTmp = NULL;
    pstDupTmp = (DupHandle*)(pFrontDupHandle);

    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];

    snprintf(szInstPreName, NAME_LEN, "DUP_VDEC_%d", u32VdecChn);
    pstInst = link_drvGetInst(szInstPreName);
    CHECK_PTR_NULL(pstInst, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    int i =0;
    for (i = 0; i < NODE_NUM_MAX; i++)
    {
        LINK_LOGD("ywn_log:i %d, sz_name %s\n", i, pstInst->stNode[i].szName);
    }

    snprintf(szOutNodeNm, NAME_LEN, "out_0");

    pVdecTskChn->decModCtrl.dupChanHandle[0] = link_drvGetHandleFromNode(pstInst, szOutNodeNm);
    CHECK_PTR_NULL(pVdecTskChn->decModCtrl.dupChanHandle[0], DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR))

    snprintf(szOutNodeNm, NAME_LEN, "out_1");
    pVdecTskChn->decModCtrl.dupChanHandle[1] = link_drvGetHandleFromNode(pstInst, szOutNodeNm);
    CHECK_PTR_NULL(pVdecTskChn->decModCtrl.dupChanHandle[1], DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR))

    snprintf(szOutNodeNm, NAME_LEN, "out_2");
    pVdecTskChn->decModCtrl.dupChanHandle[2] = link_drvGetHandleFromNode(pstInst, szOutNodeNm);
    CHECK_PTR_NULL(pVdecTskChn->decModCtrl.dupChanHandle[2], DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR))

    snprintf(szOutNodeNm, NAME_LEN, "out_3");
    pVdecTskChn->decModCtrl.dupChanHandle[3] = link_drvGetHandleFromNode(pstInst, szOutNodeNm);
    CHECK_PTR_NULL(pVdecTskChn->decModCtrl.dupChanHandle[3], DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR))

    VDEC_LOGI("chn %d dup handle [%p %p %p %p]\n", u32VdecChn, pVdecTskChn->decModCtrl.dupChanHandle[0],
              pVdecTskChn->decModCtrl.dupChanHandle[1], pVdecTskChn->decModCtrl.dupChanHandle[2], pVdecTskChn->decModCtrl.dupChanHandle[3]);

    pSrcBuf.mod = SYSTEM_MOD_ID_VDEC;
    pSrcBuf.chn = u32VdecChn;

    if (SAL_SOK != (s32Ret = dup_task_bindToDup(&pSrcBuf, *pstDupTmp, SAL_TRUE)))
    {
        VDEC_LOGE("chn %d create handle failed ret 0x%x\n", u32VdecChn, s32Ret);

        /*绑定失败释放dup句柄防止dup句柄泄漏被申请完*/
        dup_task_vdecDupDestroy(pVdecTskChn->decModCtrl.frontDupHandle, pVdecTskChn->decModCtrl.rearDupHandle, u32VdecChn);
    }
    s32Ret =pVdecTskChn->decModCtrl.dupChanHandle[0]->dupOps.OpDupStartBlit((Ptr)pVdecTskChn->decModCtrl.dupChanHandle[0]);
    if(SAL_SOK != s32Ret)
    {
        SAL_ERROR("OpDupStartBlit failed !!!\n");
        return SAL_FAIL;
    }
    return SAL_SOK;
}

/**
 * @function	vdec_tsk_SetOutChnResolution
 * @brief	配置解码输出通道宽高
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] UINT32 u32OutChn 子通道
 * @param[in] UINT32 width 宽
 * @param[in] UINT32 height 高
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_PpmSetOutChnResolution(UINT32 u32VdecChn, UINT32 u32OutChn, UINT32 width, UINT32 height)
{
    INT32 s32Ret = SAL_SOK;
	
    VDEC_HAL_STATUS stVdecStatus = {0};
    PARAM_INFO_S stParamInfo = {0};
    VDEC_TSK_CHN *pVdecTskChn = NULL;
    DUP_ChanHandle *pDupChanHandle = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN));

    if (width > MAX_PPM_VDEC_WIDTH || width < MIN_VDEC_WIDTH || height > MAX_VDEC_HEIGHT || height < MIN_VDEC_HEIGHT)
    {
        VDEC_LOGE("chn %d w %d h %d err\n", u32VdecChn, width, height);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM);
    }

    s32Ret = vdec_hal_VdecJpegSupport();
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("vdec not support jpeg dec\n");
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NOT_SUPPORT);
    }

    pVdecTskChn = &g_stVdecObjCommon.stObjTsk[u32VdecChn];

    vdec_hal_VdecChnStatus(u32VdecChn, &stVdecStatus);
    if (stVdecStatus.bcreate == 0)
    {
    	/* 解码器未创建则返回失败 */
		VDEC_LOGW("vdec %d not created! \n", u32VdecChn);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_UNEXIST);
    }

    pDupChanHandle = pVdecTskChn->decModCtrl.dupChanHandle[u32OutChn];
    CHECK_PTR_NULL(pDupChanHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
	
    stParamInfo.enType = IMAGE_SIZE_CFG;
    stParamInfo.stImgSize.u32Width = width;
    stParamInfo.stImgSize.u32Height = height;

    s32Ret = pDupChanHandle->dupOps.OpDupSetBlitPrm(pDupChanHandle, &stParamInfo);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d set prm width %d height %d err ret 0x%x\n", u32VdecChn, width, height, s32Ret);
        return s32Ret;
    }

    stParamInfo.enType = PIX_FORMAT_CFG;
    stParamInfo.enPixFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;

    s32Ret = pDupChanHandle->dupOps.OpDupSetBlitPrm(pDupChanHandle, &stParamInfo);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d set prm width %d height %d err ret 0x%x\n", u32VdecChn, width, height, s32Ret);
        return s32Ret;
    }

    return SAL_SOK;
}


#if 0

/******************************************************************
   Function:   vdec_test
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 vdec_test(UINT32 vdecChn, FACE_DSP_REGISTER_JPG_PARAM *jpeprm)
{
    INT32 s32Ret = SAL_SOK;
    static int cnt = 0;
    char name[128];
    static SYSTEM_FRAME_INFO pFrame;
    PIC_FRAME_PRM pPicprm;

    SAL_WARN("vdec_test\n");
    if (jpeprm == NULL)
    {
        VDEC_LOGE("buf null err\n");
    }

    if (pFrame.uiAppData == 0x00)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&pFrame);
        if (s32Ret != SAL_SOK)
        {
            XPACK_LOGE("chn %d alloc struct video frame failed \n", vdecChn);
            return s32Ret;
        }
    }

    vdecChn = vdecChn + 1;
    s32Ret = vdec_tsk_ScaleJpegChn(vdecChn, 0, 1280, 720);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d vdec_tsk_ScaleJpegChn ret 0x%x\n", vdecChn, s32Ret);
        return s32Ret;
    }

    s32Ret = vdec_tsk_SendJpegFrame(vdecChn, jpeprm);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d vdec_tsk_SendJpegFrame err ret 0x%x\n", vdecChn, s32Ret);
        return s32Ret;
    }

    s32Ret = vdec_tsk_GetJpegYUVFrame(vdecChn, 0, (void *)pFrame.uiAppData);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d vdec_tsk_GetJpegYUVFrame err ret 0x%x\n", vdecChn, s32Ret);
        return s32Ret;
    }

    vdec_hal_Mmap((void *)pFrame.uiAppData, &pPicprm);
    pFrame.uiDataAddr = (PhysAddr)pPicprm.addr[0];

    sprintf(name, "/mnt/out/ydata/serverlogo%d_%d_%d.yuv", cnt, 1280, 720);

    FILE *file = NULL;
    file = fopen(name, "wb");
    if (file != NULL)
    {
        for (int i = 0; i < pPicprm.height; i++)
        {
            fwrite(pPicprm.addr[0] + i * pPicprm.stride, 1, pPicprm.width, file);
        }

        for (int i = 0; i < pPicprm.height / 2; i++)
        {
            fwrite(pPicprm.addr[1] + i * pPicprm.stride, 1, pPicprm.width, file);
        }

        fclose(file);
        cnt++;
    }

    vdec_tsk_PutVdecFrame(vdecChn, 0, &pFrame);

    return s32Ret;
}

#endif

