
#ifndef _XPACK_DRV_H_
#define _XPACK_DRV_H_

/*****************************************************************************
                                头文件
*****************************************************************************/
#include "sal.h"
#include "plat_com_inter_hal.h"
#include "system_common_api.h"
#include "system_plat_common.h"
#include "dspcommon.h"
#include "platform_hal.h"
#include "sva_out.h"
#include "vgs_drv_api.h"
#include "svp_dsp_drv_api.h"
#include "disp_tsk_api.h"
#include "ximg_proc.h"
#include "xpack_ring_buf.h"


/*****************************************************************************
                                宏定义
*****************************************************************************/
#define XPACK_CHN_MAX				(MAX_XRAY_CHAN)
#define XSP_PACK_LINE_INFINITE      (0xFFFFFFFF)        /*包裹起始结束行的特殊标志，表示该起始结束行并不是包裹真实的，真实起始在该帧数据之前，真实结束在之后*/
#define XPACK_DUP_TIME_NUM          600

/*****************************************************************************
                                结构体定义
*****************************************************************************/

/**
 * @enum    XRAY_PROC_STATUS_E
 * @brief   XRAY的处理状态，实时预览、回拉、停止
 */
typedef enum
{
    XRAY_PS_NONE                = 0,
    XRAY_PS_RTPREVIEW           = 1,
    XRAY_PS_PLAYBACK_MASK       = 0x10, 
    XRAY_PS_PLAYBACK_SLICE_FAST = 0x11, 
    XRAY_PS_PLAYBACK_SLICE_XTS  = 0x12, 
    XRAY_PS_PLAYBACK_FRAME      = 0x13,

    XRAY_PS_NUM
} XRAY_PROC_STATUS_E;

typedef struct
{
    XIMAGE_DATA_ST stRawData;       // tip-raw数据
    XIMAGE_DATA_ST stBlendData;     // blend灰度数据
    XIMAGE_DATA_ST stDispData;      // 显示数据
    UINT32 u32DispBgColor;          // 背景色
    BOOL bIfRefresh;                // 是否是刷新当前屏幕
    XRAY_PROC_DIRECTION enDispDir;  // 显示方向
} XPACK_DATA4DISPLAY;

/* xpack jpg map参数结构体*/
typedef struct tagXpackJpgSvaOutTransPrm
{
    UINT32 u32PackWidth;         /* 源包裹宽*/
    UINT32 u32PackHeight;
    UINT32 u32JpgCropedWidth;   /* jpg抓图包裹宽*/
    UINT32 u32JpgCropedHeight;
    UINT32 u32JpgScaleWidth;    /* jpg抓图缩放后宽 */
    UINT32 u32JpgScaleHeight;   /* jpg抓图缩放后高 */
    UINT32 u32BackWidth;        /* jpg背板宽*/
    UINT32 u32BackHeight;
    UINT32 u32CropTop;          /* jpg抠图上边界 */
    UINT32 u32CropBottom;       /* jpg抠图下边界, 预留，不使用 */
    UINT32 u32PackTop;          /* 原图包裹上边界 */
    UINT32 u32PackBottom;       /* 原图包裹下边界，预留，不使用 */
} XPACK_COOR_TRANS;

/* 包裹相关类数据 */
typedef struct
{
    XRAY_PROC_DIRECTION enDir;  // 出图方向
    BOOL bVMirror;              // 是否垂直镜像
    XSP_PACKAGE_TAG enPkgTag;   // 包裹开始、结束等标识，实时预览仅支持NONE、START、END和MIDDLE
    UINT32 u32Top;              // 已出包裹上边沿，基于Display Image
    UINT32 u32Bottom;           // 已出包裹下边沿，基于Display Image
    UINT32 u32SliceNrawH;       // 超分后条带NRaw高度
    XIMAGE_DATA_ST stAiDgrBuf;  // 用于危险品识别的AI-YUV图像
    XIMAGE_DATA_ST stXrIdtBuf;  // 用于XSP难穿透、可疑有机物识别和回调给应用的的NRaw图像，包裹显示图像的宽高也从该参数中获取，非超分
} XPACK_DATA4PACKAGE_RT;

typedef struct
{
    UINT32 u32StartLoc;     /* 包裹起始位置，-1表示包裹起始不在该段数据内，而是在此之前，当与EndLoc同为0时表示全空白无包裹 */
    UINT32 u32EndLoc;       /* 包裹结束位置，-1表示包裹结束不在该段数据内，而是在此之后，当与StartLoc同为0时表示全空白无包裹 */
    UINT32 u32RawWidth;     /* 包裹宽度，基于NRaw */
    UINT32 u32RawHeight;    /* 包裹高度，基于NRaw */
    UINT32 u32Top;          /* 包裹上边沿，基于Display Image */
    UINT32 u32Bottom;       /* 包裹下边沿，基于Display Image */
    SVA_DSP_OUT *pstAiDgrResult; /* 包裹危险品识别结果 */
    XSP_RESULT_DATA *pstXrIdtResult; /* XSP输出的图像识别信息，包括难穿透、可疑有机物等 */
} XPACK_DATA4PACKAGE_PB;

/* 包裹标签，比如TIP、条形码等 */
typedef struct
{
    CHAR szName[SVA_ALERT_NAME_LEN]; // 标签名
    XSP_RECT stRect; // 标签矩形区域，基于X扫描图，起点：L2R为右上，R2L为左上，即X表示与包裹起始列的距离
    UINT64 u64DelayTime; // 延时显示时间，延时多少ms后才开始显示，0表示立即显示，单位：ms
    UINT32 u32ShowTime; // 显示总时长，显示多少ms后自动消失，0xFFFFFFFF表示永久显示，单位：ms
} XPACK_PACKAGE_LABEL;

typedef struct
{
    UINT32 u32Id;              // 包裹ID，仅调试使用
    XIMAGE_DATA_ST *pstImgOsd; // 用于画OSD的图像数据，其中横向区域内中有且仅有一个包裹（半个包裹也算一个，其他区域会认为空白），否则OSD会画到其他包裹上
    XIMG_RECT stToiLimitedArea; // 危险品、难穿透、可疑有机物等Toi的归一化坐标与整型坐标转换时的限定区域
    XIMG_BORDER stProfileBorder; // 包裹外轮廓边界，基于stImgOsd，边界不超过stImgOsd中的Width和Height
    XSP_RESULT_DATA *pstXrIdtResult; // XSP输出的图像识别信息，包括难穿透、可疑有机物等
    XPACK_SVA_RESULT_OUT *pstAiDgrResult; // 包裹危险品识别结果
    XPACK_PACKAGE_LABEL *pstLabelTip; // 包裹的Tip标签信息
}XPACK_PACKAGE_OSD;

/*****************************************************************************
                            函数声明
*****************************************************************************/
/**
 * @fn      Xpack_DrvInit
 * @brief   XPack业务初始化 
 * @note    初始化顺序：①XPack后级（显示、编码等），②XPack，③XPack前级（XSP、XRay） 
 * 
 * @return  SAL_STATUS SAL_SOK：初始化成功，SAL_FAIL：失败
 */
SAL_STATUS Xpack_DrvInit(VOID);

/**
 * @function:       Xpack_DrvGetScreenRaw
 * @brief:          获取缓存的当前屏幕raw数据，仅获取地址，不做数据拷贝
 * @param[IN]:      UINT32       chan            视角通道号
 * @param[OUT]:     SEG_BUF_DATA pstRawBuffPrm   Raw数据信息,过包时为tipraw,回拉时为raw
 * @param[OUT]:     SEG_BUF_DATA pstBlendBuffPrm blend数据信息
 * @param[IN/OUT]:  UINT32       *pu32Length     获取的raw数据高度，非空时传入期望获取的高度，返回实际获取的高度，该值介于屏幕宽度和分段缓存长度之间
 * @return:         SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS Xpack_DrvGetScreenRaw(UINT32 chan, SEG_BUF_DATA *pstRawBuffPrm, SEG_BUF_DATA *pstBlendBuffPrm, UINT32 *pu32Length);

/**
 * @fn      Xpack_DrvGetDispBuffStatus
 * @brief   获取实时过包时显示 Buffer中还未送显的数据量
 * @param   [IN] chan 通道号
 * @return  INT32 SAL_FAIL：获取失败，其他：显示 Buffer中还未送显的数据量
 */
INT32 Xpack_DrvGetDispBuffStatus(UINT32 chan);

/**
 * @function   Xpack_DrvClearScreen
 * @brief      清空当前屏幕所有显示和缓存包裹信息，修改帧率和过包数据刷新速度
 * @param[IN]  UINT32              chan            视角通道号
 * @param[IN]  UINT32              u32RefreshCol   每帧刷新数据的列数
 * @param[IN]  XRAY_PROC_DIRECTION enDispDir       预览过包出图方向
 * @param[OUT] None
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS Xpack_DrvClearScreen(UINT32 chan, UINT32 u32RefreshCol, XRAY_PROC_DIRECTION enDispDir);

/**
 * @function   Xpack_DrvChangeProcType
 * @brief      修改xpack处理状态，进入条带回拉会将预览全屏raw数据缓存拷贝到回拉全屏raw数据缓存中，且若过包不足一屏，会将回拉raw数据缓存移到屏幕另一侧位置
 * @param[IN]  UINT32             chan          视角通道号
 * @param[IN]  XRAY_PROC_STATUS_E enProcStat    目标处理状态
 * @param[IN]  UINT32             u32RefreshCol 每帧刷新数据的列数
 * @param[IN]  UINT32             u32RtLineNum  清屏后预览过包的数据列数
 * @param[OUT] None
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS Xpack_DrvChangeProcType(UINT32 chan, XRAY_PROC_STATUS_E enProcStat, UINT32 u32RefreshCol, UINT32 u32RtLineNum);

/**
 * @fn      Xpack_DrvSetSegCbPrm
 * @brief   包裹分片回调参数设置
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstSegPrm 包裹分片参数
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：失败
 */
SAL_STATUS Xpack_DrvSetSegCbPrm(UINT32 chan, XPACK_DSP_SEGMENT_SET *pstSegPrm);

/**
 * @function   Xpack_DrvSetDispSync
 * @brief      设置为了双视角显示同步的等待超时时间，仅双视角机型有效
 * @param[IN]  UINT32 u32SyncSetTime 同步超时时间
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS Xpack_DrvSetDispSync(UINT32 u32SyncSetTime);

/**
 * @fn      Xpack_DrvSetPackJpgCbPrm
 * @brief   设置包裹回调的JPG图片参数
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstJpgCbPrm 包裹回调的JPG图片参数
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：失败
 */
SAL_STATUS Xpack_DrvSetPackJpgCbPrm(UINT32 chan, XPACK_JPG_SET_ST *pstJpgCbPrm);

/**
 * @fn      Xpack_DrvSaveScreen
 * @brief   保存当前屏幕图像为jpeg、bmp或raw格式数据
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstSavePrm 保存设置参数
 * 
 * @return  SAL_STATUS SAL_SOK：SAVE成功，SAL_FAIL：失败
 */
SAL_STATUS Xpack_DrvSaveScreen(UINT32 chan, XPACK_SAVE_PRM *pstSavePrm);

/**
 * @fn      Xpack_DrvSetAiXrOsdShow
 * @brief   设置危险品识别与XSP难穿透&可疑有机物的OSD是否显示 
 * @note    开关TIP考核时调用，开启TIP考核时不显示危险品等辅助信息
 * 
 * @param   [IN] UINT32          chan        XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] DISP_SVA_SWITCH *pstDispOsd 仅dispSvaSwitch有效，OSD是否显示，0-不显示，1-显示
 * @param   [IN] BOOL            bShowTip    tip是否显示，0-不显示，1-显示
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：失败
 */
SAL_STATUS Xpack_DrvSetAiXrOsdShow(UINT32 chan, DISP_SVA_SWITCH *pstDispOsd, BOOL bShowTip);

/**
 * @fn      Xpack_DrvClearOsdOnScreen
 * @brief   清空屏幕上所有包裹的OSD，包括实时过包与回拉
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 */
VOID Xpack_DrvClearOsdOnScreen(UINT32 chan);

/**
 * @fn      Xpack_DrvSetAidgrLinePrm
 * @brief   设置危险品目标的画线参数
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstAiDgrLine 危险品目标的画线参数
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：失败
 */
SAL_STATUS Xpack_DrvSetAidgrLinePrm(UINT32 chan, DISPLINE_PRM *pstAiDgrLine);

/**
 * @fn      Xpack_DrvSetDispVertOffset
 * @brief   设置显示纵向的偏移，该偏移是基于整个显示图像的，即disp_yuv_h_max
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstDispVertOffset reset：重置为默认值，即纵向偏移h_top_blanking 
 *                                 offset：基于当前位置的纵向偏移量，小于0则往上偏，大于0则往下偏
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS Xpack_DrvSetDispVertOffset(UINT32 chan, XPACK_YUVSHOW_OFFSET *pstDispVertOffset);

/**
 * @fn      Xpack_DrvSendRtSlice
 * @brief   发送包裹条带数据给XPack，用于智能识别与包裹回调存储
 * 
 * @param   [IN] u32Chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] XPACK_DATA4DISPLAY *pstDataSrc  显示数据
 * @param   [IN] pstData4Pkg 包裹条带数据与一些必要信息
 * @param   [IN] pstLabelTip Tip标签，仅在为包裹条带时有效，可为NULL
 * @param   [IN] pstPackDiv 包裹分割信息，仅在包裹结束时有效，可为NULL
 * 
 * @return  SAL_STATUS SAL_SOK：发送成功，SAL_FAIL：发送失败
 */
SAL_STATUS Xpack_DrvSendRtSlice(UINT32 u32Chan, XPACK_DATA4DISPLAY *pstData4Disp, XPACK_DATA4PACKAGE_RT *pstData4Pkg, XPACK_PACKAGE_LABEL *pstLabelTip, XSP_TRANSFER_INFO *pstPackDiv);

/**
 * @function    Xpack_DrvSendPbSegORefresh
 * @brief       发送回拉段或全屏成像处理的显示数据
 * @param[in]   UINT32              chan        显示通道号
 * @param[in]   XPACK_DATA4DISPLAY  *pstDataSrc 源数据
 * @param[in]   XPACK_DATA4PACKAGE_PB *pstData4Pkg    包裹信息
 * @param[out]  NONE
 * @return      SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS Xpack_DrvSendPbSegORefresh(UINT32 chan, XPACK_DATA4DISPLAY *pstData4Disp, XPACK_DATA4PACKAGE_PB *pstData4Pkg);

/**
 * @fn      Xpack_DrvDrawOsdOnPackage
 * @brief   基于包裹画目标的OSD
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN/OUT] pstPackOsd 输入：包裹的图像及其目标信息 
 *                              输出：当bAutoExpandToShowAll为TRUE时，自动扩展图像区域，pstPackOsd->stImgOsd中的width会变大
 * @param   [IN] bAutoExpandToShowAll 是否自动扩展图像以显示所有的目标，OSD文本超过图像宽度时才会起作用 
 *                                    而允许扩展必须满足pstPackOsd->stImgOsd的stride大于width 
 * @param   [IN] u32DispBgColor 自动扩展图像时，图像扩展区域被重置的背景色
 * 
 * @return  INT32 <0：Draw失败，其他：布局所有OSD目标需要的扩展宽度（图像太窄OSD文本太长导致无法画）
 */
INT32 Xpack_DrvDrawOsdOnPackage(UINT32 chan, XPACK_PACKAGE_OSD *pstPackOsd, BOOL bAutoExpandToShowAll, UINT32 u32DispBgColor);

/**
 * @fn      Xpack_GetSvaResultForPos
 * @brief   根据预览/回拉包裹队列获取当前屏幕智能信息
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [OUT] pstSvaOut 输出：码流显示区域违禁品POS信息 
 */
SAL_STATUS Xpack_GetSvaResultForPos(UINT32 chan, SVA_PROCESS_OUT *pstSvaOut);

#endif /* _XPACK_DRV_H_*/

