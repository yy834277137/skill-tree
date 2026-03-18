/**
 * @file   mem_hal_inter.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief
 * @author
 * @date
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#ifndef _FB_HAL_INTER_H_

#define _FB_HAL_INTER_H_

#include "sal.h"
#include "platform_hal.h"

/*新平台SD3403和hi3559a图形序号存在差异*/
#define FB_DEV_G0 0
#define FB_DEV_G1 1
#define FB_DEV_G3 2
#define FB_DATA_BITWIDTH_16 16
#define FB_DATA_BITWIDTH_32 32
/* SCREEN 个数   */
#define FB_DEV_NUM_MAX          (3)
#define FB_BUFF_CNT              1 /* 代码重构之后，两个文件使用了static cnt，buffer大于1时会出现应用下发和实际刷的fb不同步 */


typedef struct _FB_APP_T_
{
    UINT32      uiX;            /* APP设置显示的起始坐标X，G0,G1默认0，G3专用*/
    UINT32      uiY;            /* APP设置显示的起始坐标Y，G0,G1默认0，G3专用*/

    UINT32      uiWidth;        /* APP的buff的宽，G3时下层修改此值*/
    UINT32      uiHeight;       /* APP的buff的高，G3时下层修改此值*/

    UINT32      uiSize;
    
    PhysAddr    ui64Phyaddr;    /* APP的buff物理地址*/
    UINT64      vbBlk;          /* RK等平台的BLK信息，硬核需要用到 */
    UINT32      u32PoolId;

    VOID*       Viraddr;        /* APP的buff虚拟地址*/

} FB_APP;

typedef struct _FB_DEV_T_
{
    UINT32      uiX;            /* DSP设置显示的起始坐标X，G0,G1默认0，G3专用*/
    UINT32      uiY;            /* DSP设置显示的起始坐标Y，G0,G1默认0，G3专用*/

    UINT32      uiWidth;
    UINT32      uiHeight;
    UINT32      uiSize;
    UINT32      offset;
    UINT64      vbBlk;          /* RK等平台的BLK信息，硬核需要用到 */
    UINT32      u32PoolId;

    PhysAddr    ui64CanvasAddr; /* G0 G1 时使用,DSP的buff物理地址*/
    VOID*       pBuf;           /* G0 G1 时使用,DSP的buff虚拟地址*/

    UINT8*      pShowScreen;    /* G3模拟鼠标时使用，MMAP出来 */
    UINT32      uiSmem_len;     /* G3模拟鼠标时使用，MMAP出来 */

} FB_DEV;

typedef struct
{
    CHAR sDumpDir[64];          /* 数据存储目录 */
    UINT32 u32DumpDp;           /* 按位表示 */
    UINT32 u32DumpCnt;
    UINT32 u32ProcCnt;
} FB_DUMP_CFG;

typedef struct _FB_COMMON_T_
{
    FB_DEV  fb_dev;             /* 显存信息 */
    FB_APP  fb_app;             /* APP信息 */

    INT32   fd;                 /* 显存设备句柄 */
    INT32   fdcopy;             /* 备用句柄fd close失败时使用 */
    INT32   u32BytePerPixel;    /*  */
    UINT8   fbName[32];          
    
    UINT32  bitWidth;           /* 位数 */
        
    UINT32  AppMMAP;

    UINT32  DevMMAP;
    
    UINT32  mouseNewChn;        /* 鼠标新绑定通道 */

    UINT32  mouseLastChn;       /* 鼠标上次绑定通道 */

    FB_DUMP_CFG stDumpCfg;      /* Dump调试配置 */

    void   *mutex;                    /* 互斥锁 */
} FB_COMMON;


typedef struct _FB_PLAT_OPS
{

    /*******************************************************************************
    * 函数名  : createFbDev
    * 描  述  : 创建图形层显示设备
    * 输  入  : - pDispDev   设备信息
    * 输  出  : -无
    * 返回值  : 成功:SAL_SOK
    *         : 失败:SAL_FAIL
    *******************************************************************************/
    INT32 (*createFb)(UINT32 uiChn, FB_COMMON *pfb_chn, SCREEN_CTRL *pstHalFbAttr, FB_INIT_ATTR_ST *pstFbPrm);
    
    /*******************************************************************************
    * 函数名  : createFbDev
    * 描  述  : 创建图形层显示设备
    * 输  入  : - pDispDev   设备信息
    * 输  出  : -无
    * 返回值  : 成功:SAL_SOK
    *         : 失败:SAL_FAIL
    *******************************************************************************/
    INT32 (*releaseFb)(UINT32 uiChn, FB_COMMON *pfb_chn);

    /*******************************************************************************
    * 函数名  : createFbDev
    * 描  述  : 创建图形层显示设备
    * 输  入  : - pDispDev   设备信息
    * 输  出  : -无
    * 返回值  : 成功:SAL_SOK
    *         : 失败:SAL_FAIL
    *******************************************************************************/
    INT32 (*refreshFb)(UINT32 uiChn,FB_COMMON *pfb_chn, SCREEN_CTRL *pstHalFbAttr);

    /*******************************************************************************
    * 函数名  : createFbDev
    * 描  述  : 创建图形层显示设备
    * 输  入  : - pDispDev   设备信息
    * 输  出  : -无
    * 返回值  : 成功:SAL_SOK
    *         : 失败:SAL_FAIL
    *******************************************************************************/
    INT32 (*setFbChn)(UINT32 uiChn, UINT32 uiBindChn, FB_COMMON *pfb_chn);

    /*******************************************************************************
    * 函数名  : getFbNum
    * 描  述  : 获取fb数量
    * 输  入  : - pDispDev   设备信息
    * 输  出  : -无
    * 返回值  : 成功:SAL_SOK
    *         : 失败:SAL_FAIL
    *******************************************************************************/
    INT32 (*getFbNum)(void);

    /*******************************************************************************
    * 函数名  : createFbDev
    * 描  述  : 设置显示启始坐标
    * 输  入  : - pDispDev   设备信息
    * 输  出  : -无
    * 返回值  : 成功:SAL_SOK
    *         : 失败:SAL_FAIL
    *******************************************************************************/
    INT32 (*setOrgin)(UINT32 uiChn,FB_COMMON *pfb_chn, POS_ATTR *pstPos);

} FB_PLAT_OPS;

/*******************************************************************************
* 函数名  : fb_hisiUMap
* 描  述  : 初始化fb成员函数
* 输  入  : - uiChn:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
FB_PLAT_OPS *fb_halRegister(void);


#endif /* _MEM_HAL_INTER_H_ */


