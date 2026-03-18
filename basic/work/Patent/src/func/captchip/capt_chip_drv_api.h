
#ifndef __CAPT_CHIP_H_
#define __CAPT_CHIP_H_
#include <sal.h>
#include <platform_hal.h>


#define CAPT_FPS_ERROR      (2)     // 输入帧率的误差
#define CAPT_WIDTH_MAX      (1920)  // 输入分辨率的最大宽度
#define CAPT_HEIGHT_MAX     (1440)  // 输入分辨率的最大高度
#define CAPT_FPS_MAX        (144)  // 输入分辨率的最大帧率

/* 降帧信息 */
typedef struct
{
    UINT32 u32Fps;                  // 降帧前的帧率
    UINT32 u32ReducedFps;           // 降帧后的帧率
} CAPT_REDUCE_FPS_MAP_S;


/* 极性 */
typedef enum
{
    CAPT_POL_NEG = 0,
    CAPT_POL_POS,
    CAPT_POL_BUTT,
} CAPT_POL_E;

/* 方向 */
typedef enum
{
    CAPT_POS_UP,
    CAPT_POS_DOWN,
    CAPT_POS_LEFT,
    CAPT_POS_RIGHT,
    CAPT_POS_BUTT,
} CAPT_POS_E;

/* CSC参数 */
typedef enum
{
    CAPT_CSC_LUMA,
    CAPT_CSC_CONTRAST,
    CAPT_CSC_HUE,
    CAPT_CSC_SATUATURE,
    CAPT_CSC_BUTT,
} CAPT_CSC_E;

/*******************************************************************************
* 函数名  : capt_func_chipUpdateByFile
* 描  述  : 固件通过文件升级
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
BOOL capt_func_chipIsFpgaNeedReset(VOID);

/*******************************************************************************
* 函数名  : capt_func_chipUpdateByFile
* 描  述  : 固件通过文件升级
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipUpdateByFile(const VOID *pvArg1, const VOID *pvArg2, const VOID *pvArg3, const VOID *pvArg4);

/*******************************************************************************
* 函数名  : capt_func_chipEdidToFile
* 描  述  : 保存edid文件到文件
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipEdidToFile(const VOID *pvArg1, const VOID *pvArg2, const VOID *pvArg3, const VOID *pvArg4);

/*******************************************************************************
* 函数名  : capt_func_chipAutoAdjust
* 描  述  : 自动调整图像
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipDebugAutoAdjust(const VOID *pvArg1, const VOID *pvArg2, const VOID *pvArg3, const VOID *pvArg4);

/*******************************************************************************
* 函数名  : capt_func_chipFpgaSetResolution
* 描  述  : 写fpga寄存器的值
* 输  入  : UINT32 u32Reg : 寄存器地址
* 输  出  : UINT32 u32Val : 值
* 返回值  : 
*******************************************************************************/
INT32 capt_func_chipFpgaSetResolution(UINT32 u32Chn, UINT32 u32Width, UINT32 u32Height, UINT32 u32Fps);

/*******************************************************************************
* 函数名  : capt_func_chipFpgaDectedAndReset
* 描  述  : 检测FPGA统计的宽高并复位
* 输  入  : 
* 输  出  : 
* 返回值  : 
*******************************************************************************/
INT32 capt_func_chipFpgaDectedAndReset(UINT32 u32Chn);

/*******************************************************************************
* 函数名  : capt_func_chipGetAdjustStatus
* 描  述  : 自动调整图像
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipGetAdjustStatus(UINT32 u32Chn);

/*******************************************************************************
* 函数名  : capt_func_chipAutoAdjust
* 描  述  : 自动调整图像
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipAutoAdjust(UINT32 u32Chn, CAPT_CABLE_E enCable);


/*******************************************************************************
* 函数名  : capt_func_chipHotPlug
* 描  述  : 拉一次hot plug
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipHotPlug(const VOID *pvArg1, const VOID *pvArg2, const VOID *pvArg3, const VOID *pvArg4);

/*******************************************************************************
* 函数名  : capt_func_chipReloadEdid
* 描  述  : 从文件中重新加载一次EDID
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipReloadEdid(const VOID *pvArg1, const VOID *pvArg2, const VOID *pvArg3, const VOID *pvArg4);

/*******************************************************************************
* 函数名  : capt_func_chipGetEdidInfo
* 描  述  : 获取EDID信息
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipGetEdidInfo(UINT32 u32Chn, CAPT_EDID_INFO_S *pstEdidInfo);

/**
 * @function   capt_func_chipSetDispEdid
 * @brief      将输出EDID回写到输入
 * @param[in]  UINT32 u32Chn             通道号
 * @param[in]  UINT32 u32ViNum           采集创建数量
 * @param[in]  CAPT_EDID_DISP_S *pstEdid 设置指令
 * @param[out] None
 * @return     INT32 SAL_SOK : 成功
 *                  SAL_FAIL : 失败
 */
INT32 capt_func_chipSetDispEdid(UINT32 u32Chn,UINT32 u32ViNum, CAPT_EDID_DISP_S *pstEdid);

/**
 * @function   capt_func_chipSetEdid
 * @brief      设置EDID
 * @param[in]  UINT32 u32Chn                   采集通道号
 * @param[in]  UINT32 u32ViNum                 采集创建数
 * @param[in]  CAPT_EDID_INFO_ST *pstEdidInfo  edid信息
 * @param[out] None
 * @return     INT32 SAL_SOK : 成功
 *                  SAL_FAIL : 失败
 */
INT32 capt_func_chipSetEdid(UINT32 u32Chn,UINT32 u32ViNum, CAPT_EDID_INFO_ST *pstEdidInfo);

/*******************************************************************************
* 函数名  : capt_func_chipSetCsc
* 描  述  : 设置CSC
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipSetCsc(UINT32 u32Chn, VIDEO_CSC_S *pstCsc);


/*******************************************************************************
* 函数名  : CaptHal_AdvModeInit
* 描  述  : 用于adv7842模块初始化
* 输  入  : - chnNum   : 通道号 
* 输  出  : - 无
* 返回值  : 0        : 成功
*          -1        : 失败
*******************************************************************************/
INT32 capt_func_chipCheckStatus(UINT32 chnId);

/*******************************************************************************
* 函数名  : capt_func_chipGetStatus
* 描  述  : 用于视频输入状态
* 输  入  : - chnId    : vi 通道 
* 输  出  : - outPrm
* 返回值  : 0        : 成功
*          -1        : 失败
*******************************************************************************/
INT32 capt_func_chipSetStatus(UINT32 chnId, CAPT_STATUS_S *pstStatus);

/*******************************************************************************
* 函数名  : capt_func_chipGetStatus
* 描  述  : 用于视频输入状态
* 输  入  : - chnId    : vi 通道 
* 输  出  : - outPrm
* 返回值  : 0        : 成功
*          -1        : 失败
*******************************************************************************/
INT32 capt_func_chipGetStatus(UINT32 chnId, CAPT_STATUS_S *pstStatus);


/*****************************************************************************
 函 数 名  : VencHal_drvGetViSrcRate
 功能描述  : 获取采集源帧率
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史       :
  1.日      期   : 2019年03月06日
    作      者   : 
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_func_chipGetViSrcRate(UINT32 viChn);

/*******************************************************************************
* 函数名  : capt_func_chipDetect
* 描  述  : 检测输入并配置
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipDetect(UINT32 chnId, CAPT_STATUS_S *pstStatus);

/*******************************************************************************
* 函数名  : capt_func_chipScale
* 描  述  : 输入分辨率大于面板分辨率时，会对输入分辨率进行缩放，计算缩放后的分辨率
* 输  入  : CAPT_RESOLUTION_S *pstSrc
* 输  出  : CAPT_RESOLUTION_S *pstDst
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipScale(UINT32 chnId, CAPT_RESOLUTION_S *pstDst, CAPT_RESOLUTION_S *pstSrc);

/*******************************************************************************
* 函数名  : capt_func_chipSetOutputRes
* 描  述  : 设置输出分辨率
* 输  入  : UINT32 u32Chn
          CAPT_RESOLUTION_S *pstRes
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipSetOutputRes(UINT32 chnId, CAPT_RESOLUTION_S *pstRes);


/*******************************************************************************
* 函数名  : capt_func_chipSetOutputRes
* 描  述  : 获取输出分辨率，当前作用判断是否支持设置输出分辨率
* 输  入  : UINT32 u32Chn
          CAPT_RESOLUTION_S *pstRes
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_func_chipGetOutputRes(UINT32 chnId, CAPT_RESOLUTION_S *pstRes);

/**
 * @function   capt_func_chipMcuheartBeatStart
 * @brief      开始发送心跳
 * @param[in]  UINT32 u32Chn  
 * @param[out] None
 * @return     INT32 SAL_SOK : 成功
*              SAL_FAIL : 失败
 */
INT32 capt_func_chipMcuheartBeatStart(UINT32 u32Chn);

/**
 * @function   CaptHal_drvHeartBeatStop
 * @brief      停止发送心跳
 * @param[in]  UINT32 u32Chn  
 * @param[out] None
 * @return     INT32 SAL_SOK : 成功
 *                  SAL_FAIL : 失败
 */
INT32 capt_func_chipMcuheartBeatStop(UINT32 u32Chn);

/**
 * @function   capt_func_chipGetBuildTime
 * @brief      获取对应芯片固件的编译时间
 * @param[in]  UINT32 u32Chn  
 * @param[out] None
 * @return     INT32 SAL_SOK : 成功
 *                  SAL_FAIL : 失败
 */
INT32 capt_func_chipGetBuildTime(UINT32 u32Chn, VIDEO_CHIP_BUILD_TIME_S *pstTime);

/**
 * @function   capt_func_chipReset
 * @brief      按顺序先后复位FPGA,MSTAR,MCU
 * @param[in]  UINT32 u32Chn  
 * @param[out] None
 * @return     INT32 SAL_SOK : 成功
 *                  SAL_FAIL : 失败
 */
INT32 capt_func_chipReset(UINT32 u32Chn);

/**
 * @function   capt_func_chipMcuFirmwareUpdate
 * @brief      单片机固件包升级
 * @param[in]  UINT32 u32Chn         采集通道
 * @param[in]  const UINT8 *pu8Buff  升级固件
 * @param[in]  UINT32 u32Len         数长度
 * @param[out] None
 * @return     INT32
 */
INT32 capt_func_chipMcuFirmwareUpdate(UINT32 u32Chn, const UINT8 *pu8Buff, UINT32 u32Len);



/*******************************************************************************
* 函数名  : CaptHal_AdvModeInit
* 描  述  : 用于adv7842模块初始化
* 输  入  : - chnNum   : 通道号 
* 输  出  : - 无
* 返回值  : 0        : 成功
*          -1        : 失败
*******************************************************************************/
INT32 capt_func_chipInit(UINT32 chnId);


/*******************************************************************************
* 函数名  : capt_func_chipCreate
* 描  述  : 创建外接输入芯片
* 输  入  : - chnNum   : 通道号 
* 输  出  : - 无
* 返回值  : 0        : 成功
*          -1        : 失败
*******************************************************************************/
INT32 capt_func_chipCreate(UINT32 uiChn);




#endif


