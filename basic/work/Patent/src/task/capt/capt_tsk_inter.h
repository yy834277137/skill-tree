/*******************************************************************************
 * capt_tsk_inter.h
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : heshengyuan <heshengyuan@hikvision.com>
 * Version: V1.0.0  2018年10月22日 Create
 *
 * Description : CAPT是一个摄像头采集后图像管理控制模块，内部包含如下功能:
                 包含: VI数据的采集、数据管理

                 流程:
                 CAPT模块创建( 输入VI的视频参数 ，一个模块调用一次接口)
                    |
                   \|/
                 获取一个capt通道(获取到capt通道及数据接口)
                    |
                   \|/
                 拿到该capt通道的数据接口能力
                    |
                   \|/
                 循环使用 OpCaptGetBlit/OpCaptPutBlit
                    |
                   \|/
                 销毁整个CAPT模块
 * Modification:
*******************************************************************************/

#ifndef _CAPT_TSK_INTERFACE_H_
#define _CAPT_TSK_INTERFACE_H_


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <sal.h>
#include "dspcommon.h"
#include "dup_tsk_api.h"


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

#define CAPT_MAX_BUFFER      (5)


/* 句柄类型 */
typedef unsigned int       CaptHandle;         /* 统用句柄类型 */


/* 指针类型定义 */
typedef void *             Ptr;         /* 指针类型 */

typedef enum
{
    CAPT_OPS_FLAG_NONE       =  0x00,

    /* 拿到该标志后，意味着获取到CAPT通道的数据输出方式是队列方式，需要成对使用capt接口。 */
    CAPT_OPS_FLAG_QUE        =  0x01,

    /* 拿到该标志后，意味着获取到CAPT通道的数据输出方式是块拷贝方式，调用capt接口一次即拿到数据。 */
    CAPT_OPS_FLAG_COPY       =  0x02,

    /* 拿到该标志后，意味着获取到CAPT通道的数据输出方式是海思平台的绑定方式 */
    CAPT_OPS_FLAG_BIND       =  0x03,
} CAPT_CreateFlag;


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

typedef struct
{
    UINT32 mod;
    UINT32 modChn;
    UINT32 chn;
}CAPT_BIND_PRM;


/* CAPT数据分发时的相关操作函数 */
typedef struct
{
    /*
        以下两个回调函数是在向CAPT模块取数据时使用，两者同时存在时，成对调用，通过
        createFlags指示是否调用这两个函数，一旦指定了，如果函数指针无效，则会警告。
     */
    INT32                   (*OpGetBlit)(Ptr pUsrArgs, Ptr pBuf);
    INT32                   (*OpPutBlit)(Ptr pUsrArgs, Ptr pBuf);

    INT32                   (*OpBindBlit)(Ptr pUsrArgs, Ptr pDstBuf, UINT32 isBind);

    /* 配置或者更新capt通道的码流信息 */
    INT32                   (*OpSetBlitPrm)(Ptr pUsrArgs, Ptr pDstBuf);

    INT32                   reserved[4];
} CAPT_ChanOps;

/* CAPT通道创建时指定的参数，和获取到的capt通道及接口能力。*/
typedef struct
{
    /* 使用capt通道消费者的名字 */
    const INT8             *pName;

    /* CAPT模块的句柄，初始化CAPT模块通道时得到，外面使用capt通道时不需要更新此成员 */
    CaptHandle              module;

    INT32                   reserved[4];
} CAPT_ChanCreate;


/* CAPT通道创建成功后获取到的capt通道及接口能力。 */
typedef struct
{
    /* 魔数,用于校验句柄有效性 */
    UINT32                  nMgicNum;

    /* 输出CAPT通道的参数: 分辨率、帧率、视频矩阵信息等 */
    Ptr                     pCaptArgs;

    /* 用户自定义参数 */
    CaptHandle              module;

    UINT32                  fps;

    /* 输出数据的回调接口 */
    CAPT_ChanOps             ops;

    /* 输出数据缓存数组 */
    UINT32             uiCaptWIdx;

    /* DupHandle */
    DupHandle *pfrontDupHandle;
    DupHandle *prearDupHandle;
} CAPT_ChanHandle;

/* 无视频信号图像的信息 */
typedef struct tagCaptChnNoSignalAttr
{
    UINT32 uiNoSignalPicW[MAX_NO_SIGNAL_PIC_CNT];      /* 无视频信号的宽 数组大小用 MAX_NO_SIGNAL_PIC_CNT */
    UINT32 uiNoSignalPicH[MAX_NO_SIGNAL_PIC_CNT];      /* 无视频信号的高   */
    PUINT8 pucNoSignalPicAddr[MAX_NO_SIGNAL_PIC_CNT];  /* 无视频信号的地址 */
    UINT32 uiNoSignalPicSize[MAX_NO_SIGNAL_PIC_CNT];   /* 无视频信号的大小 */
} CAPT_CHN_NOSIGNAL_ATTR;

/* 采集图像信息 */
typedef struct tagCaptChnPicAttr
{
    UINT32 uiCaptWid;  /* 视频宽 */
    UINT32 uiCaptHei;  /* 视频高 */
    UINT32 uiCaptFps;  /* 视频帧率 */
} CAPT_CHN_PIC_ATTR;

/* Capt Chn 创建属性 */
typedef struct tagCaptCreateAttr
{
    CAPT_CHN_PIC_ATTR      stCaptChnPicAttr;       /* 采集图像信息         */
    CAPT_CHN_NOSIGNAL_ATTR stCaptChnNoSignalAttr;  /* 无视频信号图像的信息 */
} CAPT_CREATE_ATTR;

/* Capt Mod 创建属性 */
typedef struct tagCaptModCreatePrm
{
    /* Capt Chn 创建属性 */
    CAPT_CREATE_ATTR   stCaptCreateAttr;
} CAPT_MOD_CREATE_PRM;


/*******************************************************************************
* 函数名  : capt_tsk_askCreateChan
* 描  述  : 向CAPT模块请求一个capt通道
* 输  入  : - pCreate   : 请求通道的参数
*         : - pCaptHandle: 拿到一路capt通道的实现和能力级，外部消费者通过CAPT_ChanHandle提供的接口
                          即可获取到视频数据，不用关心capt内部的实现。
                          超过capt通道数量，将获取失败；不能不限制的通过capt拿到VI的视频数据，消费
                          考虑充分复用该通道码流数据。
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 capt_tsk_askCreateChan(CAPT_ChanCreate *pCreate, CAPT_ChanHandle **pHandle);

/*******************************************************************************
* 函数名  : capt_tsk_moduleCreate
* 描  述  : 初始化一个CAPT模块，创建成果后该CAPT可以提供分发多路YUV数据流处理
* 输  入  : - pCreate   : CAPT模块初始化参数
*         : - pCaptHandle: CAPT模块句柄
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 capt_tsk_moduleCreate(CAPT_MOD_CREATE_PRM * pCreate, CaptHandle *pHandle);

/*******************************************************************************
* 函数名  : capt_tsk_moduleDelete
* 描  述  : 删除采集模块
*         : - pHandle: CAPT模块句柄
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 capt_tsk_moduleDelete(CaptHandle pHandle);


#endif

