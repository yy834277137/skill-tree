/**
 * @file   dup_tsk_api.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  DUP是图像帧数据分发及处理模块，内部包含如下功能:
                 1. 从VI获取数据之后分发给后级多路消费者；
                 2. 从解码器获取数据之后分发给后级多路消费者；
                 3. 图像处理功能，送入DUP进行镜像、缩放、图像格式转换等处理。

                 流程:
                 1. DUP模块创建( 输入VI的视频参数 ，一个模块调用一次接口)
                 2. 获取一个dup通道(获取到dup通道及数据接口)
                 3. 拿到该dup通道的数据接口能力
                 4. 循环使用 OpDupGetBlit/OpDupPutBlit 或者 OpDupCopyBlit
                 5. 销毁整个DUP模块
 * @author yeyanzhong
 * @date   2021.6.3
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */

#ifndef _DUP_TSK_API_H_
#define _DUP_TSK_API_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal.h"
//#include "hal_inc_exter/sys_hal.h"
//#include "hal_inc_exter/dup_common.h"
#include "platform_hal.h"
#include "link_task_api.h"
/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

#define DUP_MAX_BUFFER (6)


/* 句柄类型 */
typedef unsigned int DupHandle;               /* 统用句柄类型 */


/* 指针类型定义 */
typedef void *Ptr;                      /* 指针类型 */

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

typedef struct
{
    UINT32 mod;    /* 模块ID，对应SYSTEM_MOD_ID_E */
    UINT32 modChn; /* 某个模块ID内的模块编号，对应到平台的设备号 */
    UINT32 chn;    /* 模块内的子通道号*/
} DUP_BIND_PRM;

/* 需要的数据的信息 */
typedef struct tagDupDataCopyPrm
{
    UINT32 uiNeedCrop;   /* 是否需要裁剪 */
    UINT32 uiNeedScale;  /* 是否需要缩放 */
    UINT32 uiRotate;    /*0 : 0  1 : 90  2 :180 3 : 270*/

    UINT32 uiX;
    UINT32 uiY;
    UINT32 uiDstWidth;
    UINT32 uiDstHeight;
} DUP_DATA_COPY_PRM;

/* 拷贝数据信息 */
typedef struct tagDupCopyDataBuff
{
    DUP_DATA_COPY_PRM stDupDataCopyPrm;   /* 需要的数据的属性 */
    SYSTEM_FRAME_INFO *pstDstSysFrame;    /* 返回的数据       */
} DUP_COPY_DATA_BUFF;

#define DUP_MNG_FRM_CNT 10
typedef struct tagDupCopyDataBuffPool
{
    UINT32 w_idx;
    UINT32 r_idx;
    UINT32 new_cnt;
    DUP_COPY_DATA_BUFF stDupFrm[DUP_MNG_FRM_CNT];
} DUP_COPY_DATA_BUFF_POOL;

#define DISP_BUF_FRM_CNT 6

typedef struct tagDupDispDataBuffPool
{
    UINT32 w;
    UINT32 r;
    SYSTEM_FRAME_INFO stSysFrm[DISP_BUF_FRM_CNT];
} DUP_DISP_DATA_BUFF_POOL;

#define DISP_COPY_FRM_CNT 3
typedef struct tagDupCopyDispDataBuffPool
{
    UINT32 w;
    UINT32 r;
    SYSTEM_FRAME_INFO stSysFrm[DISP_COPY_FRM_CNT];
    UINT32 hasBeenUpdated;  //该通道的缓冲区是否有更新过数据。通道刚开启时刻置为0，在从底层获取到一帧数据之后置为1，主要用于解决通道重新开启之后有前一次的残留
    UINT32 u32IsNew[DISP_COPY_FRM_CNT];
} DUP_COPY_DISP_DATA_BUFF_POOL;

/* DUP数据分发时的相关操作函数 */
typedef struct
{
    /*
        以下两个回调函数是在向DUP模块取数据时使用，两者同时存在时，成对调用，通过
        createFlags指示是否调用这两个函数，一旦指定了，如果函数指针无效，则会警告。
     */
    INT32 (*OpDupGetBlit)(Ptr pUsrArgs, Ptr pBuf);
    INT32 (*OpDupPutBlit)(Ptr pUsrArgs, Ptr pBuf);

    /*
        此回调函数被外界调用一次，会收到一帧DUP分发拷贝出来的数据；DUP内部实现原理:
        线程使用硬件加速API按照调用时pUsrArgs参数把一帧视频数据拷贝一次到对方提供过
        来的地址空间上，此接口调用返回速度非常快控制在0-10ms以内。
     */
    INT32 (*OpDupCopyBlit)(Ptr pUsrArgs, Ptr pDstBuf);

    /* 采用独占绑定的方式长期占用一路图像流，数据从底层SDK/DDR传输，消费者不需要频繁的调用接口 */
    INT32 (*OpDupBindBlit)(Ptr pUsrArgs, Ptr pDstBuf, UINT32 isBind);

    /* 配置或者更新dup通道的码流信息 */
    INT32 (*OpDupSetBlitPrm)(Ptr pUsrArgs, PARAM_INFO_S *pstParamInfo);
    INT32 (*OpDupGetBlitPrm)(Ptr pUsrArgs, PARAM_INFO_S *pstParamInfo);
    INT32 (*OpDupStartBlit)(Ptr pUsrArgs);
    INT32 (*OpDupStopBlit)(Ptr pUsrArgs);

    INT32 reserved[4];
} DUP_ChanOps;

/* DUP通道创建时指定的参数，和获取到的dup通道及接口能力。*/
typedef struct
{
    /* 使用dup通道消费者的名字 */
    const INT8 *pName;

    /* 需要取帧的帧率，dup通道接口调用的频率，dup模块根据此接口分配数据复用情况和输出接口类型 */
    UINT32 fps;

    /* DUP模块给本通道数据分发时提供的接口类型 */
    UINT32 createFlags;
    /* 用户自定义参数，暂时用不到 */
    Ptr pUsrArgs;

    /* DUP模块的句柄，初始化DUP模块通道时得到，外面使用dup通道时不需要更新此成员 */
    DupHandle dupModule;

    INT32 reserved[4];
} DUP_ChanCreate;


/* DUP通道创建成功后获取到的dup通道及接口能力。 */
typedef struct
{
    /* 魔数,用于校验句柄有效性。*/
    UINT32 nMgicNum;
    UINT32 u32Grp;
    UINT32 u32Chn;

    /* 输出DUP通道的参数: 分辨率、帧率、视频矩阵信息等 */
    Ptr pDupArgs;

    /* 用户自定义参数 */
    DupHandle dupModule;

    UINT32 fps;

    /* DUP模块给本通道数据分发时提供的接口类型 */
    UINT32 createFlags;

    /* 输出数据的回调接口 */
    DUP_ChanOps dupOps;

    /* 输出数据缓存数组 */
    UINT32 uiDupWIdx;

    SYSTEM_FRAME_INFO *pastInQue[DUP_MAX_BUFFER];

} DUP_ChanHandle;

typedef struct tagDupCreateAttr
{
    UINT32 uiChn;
} DUP_CREATE_ATTR;

/* Dup Mod 创建属性 */
typedef struct tagDupModCreatePrm
{
    PhysAddr captHandle;

    /* DUP_CREATE_ATTR stDupCreateAttr; */
} DUP_MOD_CREATE_PRM;

void dup_task_setDbLeave(INT32 level);

/**
 * @function    dup_task_bindToDup
 * @brief       绑定到dup group，比如可以把解码器vdec绑定到group
 * @param[in]   pSrcBuf 绑定源
                dupHandle 目标dupHandle
                isBind，1为绑定，0为解绑
 * @param[out]
 * @return
 */
INT32 dup_task_bindToDup(DUP_BIND_PRM *pSrcBuf, DupHandle dupHandle, UINT32 isBind);

/**
 * @function    dup_task_sendToDup
 * @brief       发送图像帧到DUP group
 * @param[in]   dupHandle 发送图像帧的目的dup
                pstFrame 图像帧
 * @param[out]
 * @return
 */
INT32 dup_task_sendToDup(DupHandle dupHandle, SYSTEM_FRAME_INFO *pstFrame);

/**
 * @function    dup_task_setDupParam
 * @brief       设置dup group的参数
 * @param[in]   dupHandle 需要配置参数的dup handle
                pstParamInfo 参数结构体，具体配置什么参数通过CFG_TYPE_E区别
 * @param[out]
 * @return
 */
INT32 dup_task_setDupParam(DupHandle dupHandle, PARAM_INFO_S *pstParamInfo);

/**
 * @function    dup_task_getDupParam
 * @brief       获取dup group的参数
 * @param[in]   dupHandle 需要获取参数的dup handle
 * @param[out]  pstParamInfo 参数结构体，具体获取什么参数通过CFG_TYPE_E区别
 * @return
 */
INT32 dup_task_getDupParam(DupHandle dupHandle, PARAM_INFO_S *pstParamInfo);

/**
 * @function    dup_task_vdecDupDestroy
 * @brief       销毁dup
 * @param[in]   frontDupHandle dup创建时的前级handle
 *              frontDupHandle dup创建时的后级handle
 *              u32VdecChn     解码通道号
 * @param[out]  
 * @return
 */
INT32 dup_task_vdecDupDestroy( DupHandle frontDupHandle, DupHandle rearDupHandle, UINT32 u32VdecChn);


/**
 * @function    dup_task_vdecDupCreate
 * @brief       创建用于跟解码器绑定使用的dup
 * @param[in]   stInstCfg 绑定时需要用到的实例配置信息，在link_task.c里配置
                u32VdecChn 解码通道
 * @param[out]  pFrontDupHandle 新创建的前级dup对应的handle
                pRearDupHandle  新创建的后级dup对应的handle
 * @return
 */
INT32 dup_task_vdecDupCreate(INST_CFG_S *stInstCfg, UINT32 u32VdecChn, DupHandle *pFrontDupHandle, DupHandle *pRearDupHandle,VDEC_PRM *pstVdecPrm);


/**
 * @function    dup_task_vpDupCreate
 * @brief       创建用于做图像处理的dup
 * @param[in]   stInstCfg 绑定时需要用到的实例配置信息，在link_task.c里配置
                u32Chn 相同实例名字时对应的通道号，不同实例名通道号可以重复
 * @param[out]  pDupHandle 新创建的dup对应的handle

 * @return
 */
INT32 dup_task_vpDupCreate(INST_CFG_S *stInstCfg, UINT32 u32Chn, DupHandle *pDupHandle);

/**
 * @function    dup_task_PpmvpDupCreate
 * @brief       创建人包用于做2560x1440图像处理的dup
 * @param[in]   stInstCfg 绑定时需要用到的实例配置信息，在link_task.c里配置
                u32Chn 相同实例名字时对应的通道号，不同实例名通道号可以重复
 * @param[out]  pDupHandle 新创建的dup对应的handle

 * @return
 */
INT32 dup_task_PpmvpDupCreate(INST_CFG_S *stInstCfg, UINT32 u32Chn, DupHandle *pDupHandle);

/**
 * @function    dup_task_multDupCreate
 * @brief       创建安检数据进来时需要用到的两个dup，在分析仪里两个dup进行绑定；安检机里两个dup不需要绑定，往各自的dup送数据即可
 * @param[in]   pCreate创建dup时需要用到的参数
                u32Chn 第几路安检数据
 * @param[out]  pDupHandle1 创建的第一个dup handle
                pDupHandle2 创建的第二个dup handle
 * @return
 */
INT32 dup_task_multDupCreate(DUP_MOD_CREATE_PRM *pCreate, UINT32 u32Chn, DupHandle *pDupHandle1, DupHandle *pDupHandle2);


#endif

