/******************************************************************************
   Copyright 2020-2030 Hikvision Co.,Ltd
   FileName: link_drv.c
   Description: 链接模块，主要目的是不同产品初始化时实现数据流管道的自动搭建，
                    运行过程中也可重新绑定。
   Author: yeyanzhong
   Date: 2020.12.10
   Modification History:
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include <unistd.h>

#include "sal.h"

#include "link_drv_api.h"


#define INST_NUM_MAX (128)

static INST_INFO_S *g_stInstInfo = NULL;
static pthread_mutex_t g_mutex;

/******************************************************************
   Function:   link_drvInit
   Description: 初始化link模块
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvInit(void)
{
    g_stInstInfo = SAL_memMalloc(sizeof(INST_INFO_S) * INST_NUM_MAX, "link_drv", "g_stInstInfo");
    if (!g_stInstInfo)
    {
        LINK_LOGE("link_drv fail !!!\n");
        return SAL_FAIL;
    }
    memset(g_stInstInfo, 0, sizeof(INST_INFO_S) * INST_NUM_MAX);
    pthread_mutex_init(&g_mutex, NULL);

    return SAL_SOK;
}

/******************************************************************
   Function:   link_drvDeInit
   Description: 反初始化link模块
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvDeInit(void)
{
    if (g_stInstInfo)
    {
        SAL_memfree(g_stInstInfo, "link_drv", "g_stInstInfo");
        return SAL_SOK;
    }

    return SAL_FAIL;
}

/******************************************************************
   Function:   link_drvReqstInst
   Description: 申请实例
   Input:      无
   Output:   实例指针
   Return:   OK or ERR Information
 *******************************************************************/
INST_INFO_S *link_drvReqstInst()
{
    UINT32 i = 0;
    
    pthread_mutex_lock(&g_mutex);

    for (i = 0; i < INST_NUM_MAX; i++)
    {
        if (0 == g_stInstInfo[i].u32IsUsed)
        {
            g_stInstInfo[i].u32IsUsed = 1;
            pthread_mutex_unlock(&g_mutex);
            
            return &g_stInstInfo[i];
        }
    }
    pthread_mutex_unlock(&g_mutex);
    LINK_LOGE("err: inst is not enough !!!\n");

    return NULL;
}

/******************************************************************
   Function:   link_drvFreeInst
   Description: 释放实例
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvFreeInst(INST_INFO_S *pstInst)
{
    if (!pstInst)
    {
        return SAL_FAIL;
    }

    pstInst->u32IsUsed = 0;

    return SAL_SOK;
}

/******************************************************************
   Function:   link_drvInitInst
   Description: 初始化实例
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvInitInst(INST_INFO_S *pstInst, const CHAR *pszName)
{
    if (!pstInst || !pszName)
    {
        return SAL_FAIL;
    }

    strncpy(pstInst->szInstName, pszName, (sizeof(pstInst->szInstName) - 1));
    pstInst->szInstName[sizeof(pstInst->szInstName) - 1] = '\0';
    pthread_mutex_init(&(pstInst->instMutex), NULL);
    return SAL_SOK;
}

/******************************************************************
   Function:   link_drvGetInst
   Description: 根据名字获取实例
   Input: 实例名字
   Output: 实例指针
   Return:   OK or ERR Information
 *******************************************************************/
INST_INFO_S *link_drvGetInst(const CHAR *pszName)
{
    UINT32 i = 0;

    if (!pszName)
    {
        return NULL;
    }

    for (i = 0; i < INST_NUM_MAX; i++)
    {
        if ((!strcmp(g_stInstInfo[i].szInstName, pszName))
            && (g_stInstInfo[i].u32IsUsed))
        {
            return &g_stInstInfo[i];
        }
    }

    LINK_LOGE("err: can't find inst %s !!!\n", pszName);

    return NULL;
}

/******************************************************************
   Function:   link_drvInitNode
   Description: 初始化实例内的节点
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvInitNode(INST_INFO_S *pstInst, NODE_CFG_S *pstNodeCfg)
{
    if (!pstInst || !pstNodeCfg)
    {
        return SAL_FAIL;
    }

    LINK_LOGD("inst name %s, node name %s, node idx: %d\n", pstInst->szInstName, pstNodeCfg->szName, pstNodeCfg->s32NodeIdx);
    if ((pstNodeCfg->s32NodeIdx >= 0) && (pstNodeCfg->s32NodeIdx < NODE_NUM_MAX))
    {
        pstInst->stNode[pstNodeCfg->s32NodeIdx].s32NodeIdx = pstNodeCfg->s32NodeIdx;
        pstInst->stNode[pstNodeCfg->s32NodeIdx].enBindType = pstNodeCfg->enBindType;
        memcpy(pstInst->stNode[pstNodeCfg->s32NodeIdx].szName, pstNodeCfg->szName, \
                sizeof(pstInst->stNode[pstNodeCfg->s32NodeIdx].szName) - 1);
        pstInst->stNode[pstNodeCfg->s32NodeIdx].szName[NAME_LEN - 1] = '\0';
    }
    else
    {
        LINK_LOGE("pstNodeCfg->s32NodeIdx %d is illegal !!!\n", pstNodeCfg->s32NodeIdx);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
   Function:   link_drvGetNode
   Description: 根据名字获取节点
   Input: pstInst实例指针，ps8Name节点名字
   Output: 节点信息
   Return:   OK or ERR Information
 *******************************************************************/
static NODE_CTRL_S *link_drvGetNode(INST_INFO_S *pstInst, const CHAR *pszName)
{
    UINT32 i = 0;

    if (!pszName || !pstInst)
    {
        LINK_LOGE("%s, input param null\n", __func__);
        return NULL;
    }

    for (i = 0; i < NODE_NUM_MAX; i++)
    {
        if (!strcmp(pstInst->stNode[i].szName, pszName))
        {
            return &pstInst->stNode[i];
        }
    }

    LINK_LOGE("err: can't find node %s in inst %s !!!\n", pszName, pstInst->szInstName);

    return NULL;
}

/******************************************************************
   Function:   link_drvGetNodeIdx
   Description: 根据名字获取对应节点的索引号
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static INT32 link_drvGetNodeIdx(const INST_INFO_S *pstInst, const CHAR *pszName)
{
    UINT32 i = 0;

    if (!pszName || !pstInst)
    {
        return SAL_FAIL;
    }

    for (i = 0; i < NODE_NUM_MAX; i++)
    {
        if (!strcmp(pstInst->stNode[i].szName, pszName))
        {
            return i;
        }
    }

    LINK_LOGE("err: can't find node %s !!!\n", pszName);

    return SAL_FAIL;
}

/******************************************************************
   Function:   link_drvGetNodeIdx
   Description: 获取节点名字
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static INT32 link_drvGetNodeName(INST_INFO_S *pstInst, INT32 s32NodeIdx, CHAR **ppszName)
{
    if (!ppszName || !pstInst || s32NodeIdx < IN_NODE_0 || s32NodeIdx >= NODE_NUM_MAX)
    {
        return SAL_FAIL;
    }

    *ppszName = pstInst->stNode[s32NodeIdx].szName;

    return SAL_SOK;
}

/******************************************************************
   Function:   link_drvGetSrcNode
   Description: 获取源节点
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvGetSrcNode(CHAR *pszDstInstName, CHAR *pszDstNodeName, INST_INFO_S **ppstSrcInst, CHAR **ppszSrcNodeName)
{
    INST_INFO_S *pstDstInst = NULL;
    NODE_CTRL_S *pstDstNode = NULL;
    INT32 s32SrcNodeIdx = -1;
    INT32 s32Ret = SAL_FAIL;
 
    if (!pszDstInstName || !pszDstNodeName || !ppstSrcInst || !ppszSrcNodeName)
    {
        LINK_LOGE("%s, input param null\n", __func__);  
        return SAL_FAIL;
    }

    pstDstInst = link_drvGetInst(pszDstInstName);
    if (!pstDstInst)
    {
        LINK_LOGE("link_drvGetInst failed \n");  
        return SAL_FAIL;
    }

    pstDstNode = link_drvGetNode(pstDstInst, pszDstNodeName);
    if ((pstDstNode) && (pstDstNode->pstCurBindSrc))
    {
        *ppstSrcInst = pstDstNode->pstCurBindSrc->pInst;
        s32SrcNodeIdx = pstDstNode->pstCurBindSrc->s32NodeIdx;
    }
    else
    {
        LINK_LOGD("link_drvGetNode failed \n");  
        return SAL_FAIL;
    }

    s32Ret = link_drvGetNodeName(*ppstSrcInst, s32SrcNodeIdx, ppszSrcNodeName);

    return s32Ret;
    
}

/******************************************************************
   Function:   link_drvAttachToNode
   Description: 把handle attach到节点
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvAttachToNode(NODE_CTRL_S *stNode, void *pHandle)
{
    if (!stNode || !pHandle)
    {
        return SAL_FAIL;
    }

    stNode->pHandle = pHandle;

    return SAL_SOK;
}

/******************************************************************
   Function:   link_drvGetHandleFromNode
   Description: 从对应节点获取handle指针
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
void *link_drvGetHandleFromNode(INST_INFO_S *pstInst, CHAR *pszNodeName)
{
    NODE_CTRL_S *pstNode = NULL;

    if (!pstInst || !pszNodeName)
    {
        LINK_LOGE("%s, input param null\n", __func__);
        return NULL;
    }

    pstNode = link_drvGetNode(pstInst, pszNodeName);
    if (pstNode)
    {
        return pstNode->pHandle;
    }

    return NULL;
}

/******************************************************************
   Function:   link_drvCheckCurBind
   Description: 检查源节点和目标节点之间是否已绑定
   Input:
   Output: SAL_SOK，已绑定；SAL_FAIL，还未绑定
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvCheckCurBind(INST_INFO_S *pstSrc, CHAR *pszOutNodeNm, INST_INFO_S *pstDst, CHAR *pszInNodeNm)
{
    INT32 s32OutIdx = 0;
    INT32 s32InIdx = 0;

    if (!pstSrc || !pszOutNodeNm || !pstDst || !pszInNodeNm)
    {
        LINK_LOGE("input param is null\n");
        return SAL_FAIL;
    }

    s32OutIdx = link_drvGetNodeIdx(pstSrc, pszOutNodeNm);
    s32InIdx = link_drvGetNodeIdx(pstDst, pszInNodeNm);

    if (s32OutIdx < 0 || s32InIdx < 0)
    {
        LINK_LOGE("pszOutNodeNm %s, s32OutIdx %d; pszInNodeNm %s, s32InIdx %d \n", pszOutNodeNm, s32OutIdx, pszInNodeNm, s32InIdx);
        return SAL_FAIL;
    }

    if ((pstSrc->stNode[s32OutIdx].pstCurBindDst) \
        && (pstDst == (pstSrc->stNode[s32OutIdx].pstCurBindDst->pInst)) \
        && (s32InIdx == (pstSrc->stNode[s32OutIdx].pstCurBindDst->s32NodeIdx)))
    {
        LINK_LOGW("already have binded, srcInst %s, pszOutNodeNm %s; detInst %s, pszInNodeNm %s \n", pstSrc->szInstName, pszOutNodeNm, pstDst->szInstName, pszInNodeNm);
        return SAL_SOK;
    }

    return SAL_FAIL;
}

/******************************************************************
   Function:   link_drvUnbindSrcNode
   Description: 解绑源节点，同时需要解绑该源节点的目标节点
   Input:
   Output: 
   Return:   OK or ERR Information
 *******************************************************************/
static INT32 link_drvUnbindSrcNode(INST_INFO_S *pstSrc, INT32 s32OutIdx)
{
    NODE_CTRL_S *pstOutNode = NULL;
    UINT32 i = 0, j = 0;
    INST_INFO_S *pstBindDstInst = NULL;
    INT32 s32BindDstNode = 0;

    if (!pstSrc || s32OutIdx < OUT_NODE_0 || s32OutIdx >= NODE_NUM_MAX)
    {
        LINK_LOGE("err: u32OutIdx %d \n", s32OutIdx);
        return SAL_FAIL;
    }

    pthread_mutex_lock(&pstSrc->instMutex);

    pstOutNode = &pstSrc->stNode[s32OutIdx];
    for (i = 0; i < NODE_BIND_NUM_MAX; i++)
    {
        /* 如果该源有绑定的目标节点，则把对应的目标节点结构体里的绑定源字段清掉 */
        pstBindDstInst = pstOutNode->stBindDst[i].pInst;
        s32BindDstNode = pstOutNode->stBindDst[i].s32NodeIdx;
        if ((pstBindDstInst) && (s32BindDstNode >= IN_NODE_0) && (s32BindDstNode < NODE_NUM_MAX))
        {
            pthread_mutex_lock(&pstBindDstInst->instMutex);
            for (j = 0; j < NODE_BIND_NUM_MAX; j++)
            {
                if ((pstSrc == (pstBindDstInst->stNode[s32BindDstNode].stBindSrc[j].pInst)) \
                    && (s32OutIdx == (pstBindDstInst->stNode[s32BindDstNode].stBindSrc[j].s32NodeIdx)))
                {
                    pstBindDstInst->stNode[s32BindDstNode].stBindSrc[j].pInst = NULL;
                    pstBindDstInst->stNode[s32BindDstNode].stBindSrc[j].s32NodeIdx = -1;
                    pstBindDstInst->stNode[s32BindDstNode].u32BindSrcCnt--;
                    pstBindDstInst->stNode[s32BindDstNode].pstCurBindSrc = NULL;
                }
            }
            pthread_mutex_unlock(&pstBindDstInst->instMutex);

            /* 清除源节点结构体里的绑定目标信息 */
            pstOutNode->stBindDst[i].pInst = NULL;
            pstOutNode->stBindDst[i].s32NodeIdx = -1;
            pstOutNode->u32BindDstCnt--;
            pstOutNode->pstCurBindDst = NULL;
        }

    }

    pthread_mutex_unlock(&pstSrc->instMutex);

    return SAL_SOK;
}

/******************************************************************
   Function:   link_drvCheckCurBind
   Description: 解绑目标节点，同时需要解除该目标节点对应的源节点
   Input:
   Output: 
   Return:   OK or ERR Information
 *******************************************************************/
static INT32 link_drvUnbindDstNode(INST_INFO_S *pstDst, INT32 s32InIdx)
{
    NODE_CTRL_S *pstInNode = NULL;
    UINT32 i = 0, j = 0;
    INST_INFO_S *pstBindSrcInst = NULL;
    INT32 s32BindSrcNode = 0;

    if (!pstDst || s32InIdx < IN_NODE_0 || s32InIdx >= OUT_NODE_0)
    {
        LINK_LOGE("err: u32InIdx %d \n", s32InIdx);
        return SAL_FAIL;
    }
    
    pthread_mutex_lock(&pstDst->instMutex);

    pstInNode = &pstDst->stNode[s32InIdx];
    for (i = 0; i < NODE_BIND_NUM_MAX; i++)
    {
        /* 如果该源有绑定的目标节点，则把对应的目标节点结构体里的绑定源字段清掉 */
        pstBindSrcInst = pstInNode->stBindSrc[i].pInst;
        s32BindSrcNode = pstInNode->stBindSrc[i].s32NodeIdx;
        if ((pstBindSrcInst) && ((s32BindSrcNode >= OUT_NODE_0) && (s32BindSrcNode < NODE_NUM_MAX)))
        {
            pthread_mutex_lock(&pstBindSrcInst->instMutex);
            for (j = 0; j < NODE_BIND_NUM_MAX; j++)
            {
                if ((pstDst == (pstBindSrcInst->stNode[s32BindSrcNode].stBindDst[j].pInst)) \
                    && (s32InIdx == (pstBindSrcInst->stNode[s32BindSrcNode].stBindDst[j].s32NodeIdx)))
                {
                    pstBindSrcInst->stNode[s32BindSrcNode].stBindDst[j].pInst = NULL;
                    pstBindSrcInst->stNode[s32BindSrcNode].stBindDst[j].s32NodeIdx = -1;
                    pstBindSrcInst->stNode[s32BindSrcNode].u32BindDstCnt--;
                    pstBindSrcInst->stNode[s32BindSrcNode].pstCurBindDst = NULL;
                }
            }
            pthread_mutex_unlock(&pstBindSrcInst->instMutex);

            /* 清除源节点结构体里的绑定目标信息 */
            pstInNode->stBindSrc[i].pInst = NULL;
            pstInNode->stBindSrc[i].s32NodeIdx = -1;
            pstInNode->u32BindSrcCnt--;
            pstInNode->pstCurBindSrc = NULL;
        }

    }

    pthread_mutex_unlock(&pstDst->instMutex);

    return SAL_SOK;
}

/******************************************************************
   Function:   link_drvBindToSrc
   Description: 绑定源节点
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static INT32 link_drvBindToSrc(INST_INFO_S *pstSrc, UINT32 u32OutIdx, INST_INFO_S *pstDst, UINT32 u32InIdx)
{
    NODE_CTRL_S *pstInNode = NULL;
    UINT32 i = 0;

    if (!pstSrc || u32OutIdx < OUT_NODE_0 || u32OutIdx >= NODE_NUM_MAX)
    {
        LINK_LOGE("err: u32OutIdx %d \n", u32OutIdx);
        return SAL_FAIL;
    }

    if (!pstDst || u32InIdx >= OUT_NODE_0)
    {
        LINK_LOGE("err: u32InIdx %d \n", u32InIdx);
        return SAL_FAIL;
    }
    pthread_mutex_lock(&pstDst->instMutex);
    pstInNode = &pstDst->stNode[u32InIdx];
    for (i = 0; i < NODE_BIND_NUM_MAX; i++)
    {
        if (NULL == pstInNode->stBindSrc[i].pInst)
        {
            pstInNode->stBindSrc[i].pInst = (void *)pstSrc;
            pstInNode->stBindSrc[i].s32NodeIdx = u32OutIdx;
            pstInNode->u32BindSrcCnt++;
            pstInNode->pstCurBindSrc = &pstInNode->stBindSrc[i];
            pthread_mutex_unlock(&pstDst->instMutex);
            return SAL_SOK;
        }
    }
    pthread_mutex_unlock(&pstDst->instMutex);
    return SAL_FAIL;
}

/******************************************************************
   Function:   link_drvBindToDst
   Description: 绑定目标节点
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static INT32 link_drvBindToDst(INST_INFO_S *pstSrc, INT32 s32OutIdx, INST_INFO_S *pstDst, INT32 s32InIdx)
{
    NODE_CTRL_S *pstOutNode = NULL;
    UINT32 i = 0;

    if (!pstSrc || s32OutIdx < OUT_NODE_0 || s32OutIdx >= NODE_NUM_MAX)
    {
        LINK_LOGE("err: u32OutIdx %d \n", s32OutIdx);
        return SAL_FAIL;
    }

    if (!pstDst || s32InIdx < IN_NODE_0 || s32InIdx >= OUT_NODE_0)
    {
        LINK_LOGE("err: u32InIdx %d \n", s32InIdx);
        return SAL_FAIL;
    }
    pthread_mutex_lock(&pstSrc->instMutex);
    pstOutNode = &pstSrc->stNode[s32OutIdx];
    for (i = 0; i < NODE_BIND_NUM_MAX; i++)
    {
        if (NULL == pstOutNode->stBindDst[i].pInst)
        {
            pstOutNode->stBindDst[i].pInst = (void *)pstDst;
            pstOutNode->stBindDst[i].s32NodeIdx = s32InIdx;
            pstOutNode->u32BindDstCnt++;
            pstOutNode->pstCurBindDst = &pstOutNode->stBindDst[i];
            pthread_mutex_unlock(&pstSrc->instMutex);
            return SAL_SOK;
        }
    }
    pthread_mutex_unlock(&pstSrc->instMutex);
    return SAL_FAIL;
}

#if 0

/******************************************************************
   Function:   link_drvUnbindFromSrc
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvUnbindFromSrc(INST_INFO_S *pstSrc, UINT32 u32OutIdx, INST_INFO_S *pstDst, UINT32 u32InIdx)
{
    NODE_CTRL_S *pstInNode = NULL;
    UINT32 i = 0;

    if (!pstSrc || u32OutIdx < OUT_NODE_0 || u32OutIdx >= NODE_NUM_MAX)
    {
        printf("err: u32OutIdx %d \n", u32OutIdx);
        return -1;
    }

    if (!pstDst || u32InIdx < IN_NODE_0 || u32InIdx >= OUT_NODE_0)
    {
        printf("err: u32InIdx %d \n", u32InIdx);
        return -1;
    }

    pstInNode = &pstDst->stNode[u32InIdx];
    for (i = 0; i < NODE_BIND_NUM_MAX; i++)
    {
        if ((pstSrc == pstInNode->stBindSrc[i].pInst) && ((u32OutIdx == pstInNode->stBindSrc[i].s32NodeIdx)))
        {
            pstInNode->stBindSrc[i].pInst = NULL;
            pstInNode->stBindSrc[i].s32NodeIdx = -1;
            pstInNode->u32BindSrcCnt--;
            return 0;
        }
    }

    return -1;
}

#endif

#if 0

/******************************************************************
   Function:   link_drvUnbindFromDst
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvUnbindFromDst(INST_INFO_S *pstSrc, INT32 s32OutIdx, INST_INFO_S *pstDst, INT32 s32InIdx)
{
    NODE_CTRL_S *pstOutNode = NULL;
    UINT32 i = 0;

    if (!pstSrc || s32OutIdx < OUT_NODE_0 || s32OutIdx >= NODE_NUM_MAX)
    {
        printf("err: u32OutIdx %d \n", s32OutIdx);
        return -1;
    }

    if (!pstDst || s32InIdx < IN_NODE_0 || s32InIdx >= OUT_NODE_0)
    {
        printf("err: u32InIdx %d \n", s32InIdx);
        return -1;
    }

    pstOutNode = &pstSrc->stNode[s32OutIdx];
    for (i = 0; i < NODE_BIND_NUM_MAX; i++)
    {
        if ((pstDst == pstOutNode->stBindDst[i].pInst) && (s32InIdx == pstOutNode->stBindDst[i].s32NodeIdx))
        {
            pstOutNode->stBindDst[i].pInst = NULL;
            pstOutNode->stBindDst[i].s32NodeIdx = -1;
            pstOutNode->u32BindDstCnt--;      /* 这里还需要加锁*/
            return 0;
        }
    }

    return -1;
}

#endif

/******************************************************************
   Function:   link_drvBind
   Description: 绑定接口
   Input: pstSrc源实例；ps8OutNodeNm输出节点；
             pstDst目标实例；ps8InNodeNm输入节点；
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvBind(INST_INFO_S *pstSrc, CHAR *pszOutNodeNm, INST_INFO_S *pstDst, CHAR *pszInNodeNm)
{
    INT32 s32Ret = 0;
    INT32 s32OutIdx = 0;
    INT32 s32InIdx = 0;
    
    if (!pstSrc || !pszOutNodeNm || !pstDst || !pszInNodeNm)
    {
        LINK_LOGE("input param is null\n");
        return SAL_FAIL;
    }

    LINK_LOGI("[srcInst %s, srcNode %s] -------> [dstInst %s, dstNode %s] \n", pstSrc->szInstName, pszOutNodeNm, pstDst->szInstName, pszInNodeNm);
    s32OutIdx = link_drvGetNodeIdx(pstSrc, pszOutNodeNm);
    s32InIdx = link_drvGetNodeIdx(pstDst, pszInNodeNm);

    s32Ret = link_drvBindToDst(pstSrc, s32OutIdx, pstDst, s32InIdx);
    s32Ret |= link_drvBindToSrc(pstSrc, s32OutIdx, pstDst, s32InIdx);

    return s32Ret;
}

/******************************************************************
   Function:   link_drvUnbind
   Description: 解绑接口
   Input: pstSrc源实例；ps8OutNodeNm输出节点；
             pstDst目标实例；ps8InNodeNm输入节点；
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 link_drvUnbind(INST_INFO_S *pstSrc, CHAR *pszOutNodeNm, INST_INFO_S *pstDst, CHAR *pszInNodeNm)
{
    INT32 s32Ret = 0;
    INT32 s32OutIdx = 0;
    INT32 s32InIdx = 0;

    if (!pstSrc || !pszOutNodeNm || !pstDst || !pszInNodeNm)
    {
        LINK_LOGE("input param is null\n");
        return SAL_FAIL;
    }

    s32OutIdx = link_drvGetNodeIdx(pstSrc, pszOutNodeNm);
    s32InIdx = link_drvGetNodeIdx(pstDst, pszInNodeNm);

    s32Ret = link_drvUnbindSrcNode(pstSrc, s32OutIdx);
    s32Ret |= link_drvUnbindDstNode(pstDst, s32InIdx);
    
    LINK_LOGI("unbind, srcInst %s, pszOutNodeNm %s; detInst %s, pszInNodeNm %s \n", pstSrc->szInstName, pszOutNodeNm, pstDst->szInstName, pszInNodeNm);

    return s32Ret;
}




