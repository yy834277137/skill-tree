/**
 * @file   dual_pkg_match.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  双视角包裹匹配
 * @author sunzelin
 * @date   2022/5/30
 * @note   DPM: dual package matching
 * @note \n History
   1.日    期: 2022/5/30
     作    者: sunzelin
     修改历史: 创建文件
 */

#ifndef _DUAL_PKG_MATCH_H_
#define _DUAL_PKG_MATCH_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include <stdio.h>
#include <stdbool.h>

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
/* 返回值成功 */
#define DPM_OK (0)
/* 返回值失败 */
#define DPM_FAIL (-1)

/* 包裹匹配模块支持的最大通道个数 */
#define DUAL_PKG_MATCH_MAX_SUPT_CHN_NUM (2)
/* 包裹匹配缓存队列深度 */
#define DUAL_PKG_MATCH_BUF_QUEUE_DEPTH (16)

/*----------------------------------------------*/
/*                 错误码定义                   */
/*----------------------------------------------*/
/* 模块通用性错误码，目前分配16个，如有需要可以扩增 */
#define DUAL_PKG_MATCH_ERR_CODE_BASE (0x77770000)
/* 空指针 */
#define DUAL_PKG_MATCH_ERR_NULL_PTR	(DUAL_PKG_MATCH_ERR_CODE_BASE + 0x1)
/* 用户输入参数异常 */
#define DUAL_PKG_MATCH_ERR_ILLEGAL_VIEW_TYPE (DUAL_PKG_MATCH_ERR_CODE_BASE + 0x2)
/* 静态参数异常 */
#define DUAL_PKG_MATCH_ERR_STATIC_PRM (DUAL_PKG_MATCH_ERR_CODE_BASE + 0x3)
/* 内存申请失败 */
#define DUAL_PKG_MATCH_ERR_ALLOC_MEM (DUAL_PKG_MATCH_ERR_CODE_BASE + 0x4)

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
typedef enum _DUAL_PKG_MATCH_VIEW_TYPE_
{
    /* 异常视角类型，默认视角序列从1计数 */
    INVALID_VIEW_TYPE = 0,
    /* 主视角 */
    DUAL_PKG_MATCH_MAIN_VIEW = 1,
    /* 侧视角 */
    DUAL_PKG_MATCH_SIDE_VIEW = 2,
} DUAL_PKG_MATCH_VIEW_TYPE_E;

typedef struct _DUAL_PKG_MATCH_INIT_CHN_QUE_PRM_
{
    /* 包裹缓存深度 */
    unsigned int uiQueDepth;
} DUAL_PKG_MATCH_INIT_CHN_QUE_PRM_S;

typedef struct _DUAL_PKG_MATCH_INIT_CHN_PRM_
{
    /* 视角类型 */
    DUAL_PKG_MATCH_VIEW_TYPE_E enViewType;
    /* 包裹缓存队列参数 */
    DUAL_PKG_MATCH_INIT_CHN_QUE_PRM_S stQuePrm;
} DUAL_PKG_MATCH_INIT_CHN_PRM_S;

typedef struct _DUAL_PKG_MATCH_INIT_PRM_
{
    /* 初始化的通道个数 */
    int iInitChnCnt;
    /* 初始化的通道参数 */
    DUAL_PKG_MATCH_INIT_CHN_PRM_S astInitChnPrm[DUAL_PKG_MATCH_MAX_SUPT_CHN_NUM];
} DUAL_PKG_MATCH_INIT_PRM_S;

typedef struct _DUAL_PKG_MATCH_OUTER_PKG_INFO_
{
    /* 包裹ID */
    unsigned int uiPkgId;
} DUAL_PKG_MATCH_OUTER_PKG_INFO_S;

typedef struct _DUAL_PKG_MATCH_RECT_INFO_
{
    float x;
    float y;
    float w;
    float h;
} DUAL_PKG_MATCH_RECT_INFO_S;

/* 回调函数，由外层调用模块实现并传入 */
typedef int (*dpm_get_pkg_rect_from_id_f)(void *pXsiRawOutInfo,
                                          DUAL_PKG_MATCH_VIEW_TYPE_E enViewType,
                                          int iPkgId,
                                          DUAL_PKG_MATCH_RECT_INFO_S *pstRect);

typedef struct _DUAL_PKG_MATCH_CTRL_PRM_
{
    /* 外部XSIE模块获取到的原始算法结果，对应XSIE_SECURITY_ALERT_T */
    void *pRawOutInfo;
    /* 回调函数，用于获取特定视角特定包裹id的rect信息，由外部XSIE调用模块实现 */
    dpm_get_pkg_rect_from_id_f pProcFunc;
} DUAL_PKG_MATCH_CTRL_PRM_S;

typedef struct _DUAL_PKG_MATCH_RESULT_
{
    /* 是否存在匹配结果 */
    bool bHasMatch;
    /* 包裹匹配对的id，0表示主视角包裹id，1为侧视角包裹id */
    int auiMatchId[2];
} DUAL_PKG_MATCH_RESULT_S;

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/
/**
 * @function   dpm_init
 * @brief      包裹匹配模块初始化
 * @param[in]  DUAL_PKG_MATCH_INIT_PRM_S *pstInitPrm
 * @param[out] None
 * @return     int
 */
int dpm_init(const DUAL_PKG_MATCH_INIT_PRM_S *pstInitPrm);

/**
 * @function   dpm_deinit
 * @brief      包裹匹配模块去初始化
 * @param[in]  void
 * @param[out] None
 * @return     int
 */
int dpm_deinit(void);

/**
 * @function   dpm_insert_pkg_queue
 * @brief      向包裹匹配队列插入新包裹
 * @param[in]  DUAL_PKG_MATCH_VIEW_TYPE_E enViewType
 * @param[in]  DUAL_PKG_MATCH_OUTER_PKG_INFO_S *pstPkgInfo
 * @param[out] None
 * @return     int
 */
int dpm_insert_pkg_queue(DUAL_PKG_MATCH_VIEW_TYPE_E enViewType,
                         DUAL_PKG_MATCH_OUTER_PKG_INFO_S *pstPkgInfo);

/**
 * @function   dpm_do_pkg_match
 * @brief      进行包裹匹配处理
 * @param[in]  DUAL_PKG_MATCH_CTRL_PRM_S *pstCtrlPrm
 * @param[out] None
 * @return     int
 */
int dpm_do_pkg_match(DUAL_PKG_MATCH_CTRL_PRM_S *pstCtrlPrm, DUAL_PKG_MATCH_RESULT_S *pstMatchResult);

/**
 * @function   dpm_set_match_thresh
 * @brief      设置包裹匹配阈值
 * @param[in]  float thresh  
 * @param[out] None
 * @return     int
 */
int dpm_set_match_thresh(float thresh);

/**
 * @function   dpm_print_debug_status
 * @brief      打印匹配模块调试信息
 * @param[in]  void  
 * @param[out] None
 * @return     int
 */
int dpm_print_debug_status(void);

#ifdef __cplusplus
}
#endif

#endif

