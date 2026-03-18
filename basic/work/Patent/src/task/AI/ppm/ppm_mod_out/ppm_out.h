/**
 * @file:   ppm_out.h
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  人包关联模块外部头文件
 * @author: sunzelin
 * @date    2020/12/28
 * @note:
 * @note \n History:
   1.日    期: 2020/12/28
     作    者: sunzelin
     修改历史: 创建文件
 */
#ifndef _PPM_OUT_H_
#define _PPM_OUT_H_

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include "sal.h"
#include "dspcommon.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
#define PPM_MAX_JOINTS_NUM	(256)            /* 引擎那边最大目标数32，单目标最大关节数为32，此处限制为256包含所有目标的关节点 */
#define PPM_MAX_FACE_NUM	(32)

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
typedef struct _PPM_RULE_REGION_INFO_
{
    SVA_RECT_F stFaceRgn;                  /* 人脸配置规则区域 */
    SVA_RECT_F stCapARgn;                  /* 抓拍A区域 */
    SVA_RECT_F stCapBRgn;                  /* 抓拍B区域 */
} PPM_RULE_REGION_INFO_S;

typedef struct _PPM_JOINT_RECT_
{
    UINT32 uiJointsCnt;                                /* 关节点个数 */
    CHAR str[PPM_MAX_FACE_NUM][64];                    /* id信息，通过私有帧字符串输出 */
    SVA_RECT_F stJointsRect[PPM_MAX_JOINTS_NUM];       /* 关节点区域 */
} PPM_JOINT_RECT_S;

typedef struct _PPM_FACE_RECT_
{
    UINT32 uiFaceCnt;                                  /* 人脸个数 */
    CHAR str[PPM_MAX_FACE_NUM][64];                    /* id信息，通过私有帧字符串输出 */
    SVA_RECT_F stFaceRect[PPM_MAX_FACE_NUM];           /* 人脸区域 */
} PPM_FACE_RECT_S;

typedef struct _PPM_POS_INFO_
{
    BOOL bEnable;
    PPM_RULE_REGION_INFO_S stRuleRgnInfo;              /* 规则区域信息 */

    PPM_FACE_RECT_S stFaceRectInfo;                        /* face rect */
    PPM_JOINT_RECT_S stJointsRectInfo;                     /* Joints rect */

    UINT64 u64PTS;
    UINT32 uiReserved[8];                              /* 保留位 */
} PPM_POS_INFO_S;

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/
VOID Ppm_DrvSetMetaCtlFlag(UINT32 flag);

VOID ppm_debug_dump_depth(VOID);

VOID Ppm_DrvParsePos(VOID);

VOID Ppm_DrvSetDepthSyncGap(UINT32 gap);

/**
 * @function   Ppm_DrvGenerateDepthInfo
 * @brief      从json数据中格式化生成深度信息
 * @param[in]  VOID *pJsonData        
 * @param[in]  UINT32 u32DataLen  
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_DrvGenerateDepthInfo(VOID *pJsonData, UINT32 u32DataLen);

/**
 * @function:   Ppm_DrvSetDbgSwitch
 * @brief:      设置通道的同步时间戳阈值，目前固定是300
 * @param[in]:  BOOL bChgFlag, UINT64 in_gap
 * @param[out]: None
 * @return:     VOID
 */
VOID set_sync_gap(BOOL bChgFlag, UINT64 in_gap);

VOID Ppm_DrvSetSyncGapCnt(UINT32 cnt);

VOID Ppm_DrvSetPosInfoFlag(UINT32 flag);

VOID Ppm_DrvSetPackSensity(UINT32 sensity);

/**
 * @function:   Ppm_DrvSetDbgSwitch
 * @brief:      设置调试等级，目前仅用于打印数据个数定位泄露
 * @param[in]:  UINT32 flag
 * @param[out]: None
 * @return:     VOID
 */
VOID Ppm_DrvSetDbgSwitch(UINT32 flag);

/**
 * @function:   Ppm_DrvSetDbgSwitch
 * @brief:      打印推帧个数和回调个数
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     VOID
 */
VOID Ppm_DrvPrintDbgPrm(VOID);

/**
 * @function:   Ppm_DrvGetPosInfo
 * @brief:      获取pos信息
 * @param[in]:  UINT32 chan
 * @param[in]:  PPM_POS_INFO_S *pstPosInfo
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvGetPosInfo(UINT32 chan, PPM_POS_INFO_S *pstPosInfo);

/**
 * @function:   Ppm_TskSetVprocHandle
 * @brief:      设置vproc通道句柄
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_TskSetVprocHandle(UINT32 chan, UINT32 u32OutChn, VOID *pDupChnHandle);

/**
 * @function:   Ppm_TskInit
 * @brief:      任务层初始化接口
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_TskInit(VOID);

/**
 * @function:   CmdProc_ppmCmdProc
 * @brief:      人包关联交互命令处理函数
 * @param[in]:  HOST_CMD cmd
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 CmdProc_ppmCmdProc(HOST_CMD cmd, UINT32 chan, void *prm);

#endif

