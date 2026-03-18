/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   FileName: link_drv_api.h
   Description:链接模块，主要目的是不同产品初始化时实现数据流管道的自动搭建，
                    运行过程中也可重新绑定。
   Author: yeyanzhong
   Date: 2020.12.10
   Modification History:
******************************************************************************/
#ifndef _BIND_DRV_H
#define _BIND_DRV_H

#include "sal_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 节点绑定的最大个数。节点与节点之间绑定可一对一，一对多，
  * 目前只支持一对一绑定。
  * 如果要支持一对多绑定，则解绑时需要外部传入解绑的源节点以及目标节点。
  * 目前一对一绑定的情况下，解绑时只需要传入源节点或者目标节点，自动把对应的目标节点或源节点解绑
 */
#define NODE_BIND_NUM_MAX 1

#define NAME_LEN 32

/* 节点索引 */
typedef enum
{
    IN_NODE_0 = 0,
    IN_NODE_1,
    IN_NODE_2,
    IN_NODE_3,
    IN_NODE_4,
    IN_NODE_5,
    IN_NODE_6,
    IN_NODE_7,
    OUT_NODE_0,
    OUT_NODE_1,
    OUT_NODE_2,
    OUT_NODE_3,
    OUT_NODE_4,
    OUT_NODE_5,
    OUT_NODE_6,
    OUT_NODE_7,
    NODE_NUM_MAX,
} NODE_IDX;

typedef enum
{
    NODE_BIND_TYPE_SDK_BIND = 0,    /* SDK bind */
    NODE_BIND_TYPE_GET,             /* 接口需要成对调用get/put */
    NODE_BIND_TYPE_COPY,            /* 接口只需要调一次 */
    NODE_BIND_TYPE_SEND,            /* 采用送帧方式 */
    
} NODE_BIND_TYPE_E;


/* 节点配置信息，初始化节点时使用 */
typedef struct _NODE_CFG_S
{
    INT32 s32NodeIdx;
    CHAR szName[NAME_LEN];
    NODE_BIND_TYPE_E enBindType;
} NODE_CFG_S;

typedef struct _INST_CFG_S
{
    CHAR szInstPreName[NAME_LEN];
    UINT32 u32NodeNum;
    NODE_CFG_S stNodeCfg[];
} INST_CFG_S;

/* 节点绑定信息 */
typedef struct _NODE_BIND_S
{
    void *pInst;
    INT32 s32NodeIdx;
} NODE_BIND_S;

/* 节点信息，主要有名字、绑定关系等 */
typedef struct _NODE_CTRL_S
{
    CHAR szName[NAME_LEN];
    INT32 s32NodeIdx;
    NODE_BIND_TYPE_E enBindType; /*绑定类型，HI3559a里vdec与vpss之间用sdk绑定，NT98336里手动从vdec获取送入vpss。NODE_BIND_NUM_MAX如果不为1，这里需要采用数组*/
    UINT32 u32DataType;
    void *pHandle;
    UINT32 u32BindDstCnt;
    UINT32 u32BindSrcCnt;
    NODE_BIND_S *pstCurBindDst;
    NODE_BIND_S *pstCurBindSrc;
    NODE_BIND_S stBindDst[NODE_BIND_NUM_MAX]; 
    NODE_BIND_S stBindSrc[NODE_BIND_NUM_MAX]; 
} NODE_CTRL_S;

/* 实例信息 */
typedef struct _INST_INFO_S
{
    CHAR szInstName[NAME_LEN];
    UINT32 u32IsUsed;
    pthread_mutex_t instMutex;
    UINT32 u32InNodeNum;
    UINT32 u32OutNodeNum;
    NODE_CTRL_S stNode[NODE_NUM_MAX];
} INST_INFO_S;

/******************************************************************
   Function:   link_drvInit
   Description: 初始化link模块
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvInit(void);

/******************************************************************
   Function:   link_drvDeInit
   Description: 反初始化link模块
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvDeInit(void);

/******************************************************************
   Function:   link_drvReqstInst
   Description: 申请实例
   Input:      无
   Output:   实例指针 
   Return:   OK or ERR Information
 *******************************************************************/
INST_INFO_S *link_drvReqstInst();

/******************************************************************
   Function:   link_drvFreeInst
   Description: 释放实例
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvFreeInst(INST_INFO_S *pstInst);

/******************************************************************
   Function:   link_drvInitInst
   Description: 初始化实例
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvInitInst(INST_INFO_S *pstInst, const CHAR *pszName);

/******************************************************************
   Function:   link_drvGetInst
   Description: 根据名字获取实例
   Input: 实例名字
   Output: 实例指针
   Return:   OK or ERR Information
 *******************************************************************/
INST_INFO_S *link_drvGetInst(const CHAR *pszName);

/******************************************************************
   Function:   link_drvInitNode
   Description: 初始化实例内的节点
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvInitNode(INST_INFO_S *pstInst, NODE_CFG_S *pstNodeCfg);

/******************************************************************
   Function:   link_drvGetSrcNode
   Description: 获取源节点
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvGetSrcNode(CHAR *pszDstInstName, CHAR *pszDstNodeName, INST_INFO_S **ppstSrcInst, CHAR **ppszSrcNodeName);

/******************************************************************
   Function:   link_drvAttachToNode
   Description: 把handle attach到节点
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvAttachToNode(NODE_CTRL_S *stNode, void *pHandle);

/******************************************************************
   Function:   link_drvGetHandleFromNode
   Description: 从对应节点获取handle指针
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
void *link_drvGetHandleFromNode(INST_INFO_S *pstInst, CHAR *pszNodeName);

/******************************************************************
   Function:   link_drvCheckCurBind
   Description: 检查源节点和目标节点之间是否已绑定
   Input:
   Output: SAL_SOK，已绑定；SAL_FAIL，还未绑定
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvCheckCurBind(INST_INFO_S *pstSrc, CHAR *pszOutNodeNm, INST_INFO_S *pstDst, CHAR *pszInNodeNm);

/******************************************************************
   Function:   link_drvBind
   Description: 绑定接口
   Input: pstSrc源实例；pszOutNodeNm输出节点；
             pstDst目标实例；pszInNodeNm输入节点；
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvBind(INST_INFO_S *pstSrc, CHAR *pszOutNodeNm, INST_INFO_S *pstDst, CHAR *pszInNodeNm);

/******************************************************************
   Function:   link_drvUnbind
   Description: 解绑接口
   Input: pstSrc源实例；pszOutNodeNm输出节点；
             pstDst目标实例；pszInNodeNm输入节点；
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvUnbind(INST_INFO_S *pstSrc, CHAR *pszOutNodeNm, INST_INFO_S *pstDst, CHAR *pszInNodeNm);



#ifdef __cplusplus
}
#endif

#endif

