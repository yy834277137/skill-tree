/**
 * @file   sts.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  智能维护模块
 * @author sunzelin
 * @date   2021.12.23
 * @note
 * @note \n History
   1.Date        : 2021.12.23
     Author      : sunzelin
     Modification: create
 */
#ifndef _STS_H_
#define _STS_H_

#include <stdio.h>
#include <string.h>

#include "../../../base/common/sal_type.h"


#if 0  // unused
#define MOD_STS_MAX_TYPE			(32) /* 当前sts模块支持的最大查询类别个数 */
#define MOD_STS_MAX_SUB_TYPE		(32) /* 当前sts模块支持的最大查询子类别个数 */
#define MOD_STS_MAX_FIRST_LAYER_NUM (8) /* 支持最大第一层级个数 */
#endif

#define MOD_STS_MAX_STRING_LEN		(64) /* 字符串最大长度 */
#define MOD_STS_MAX_LAYER_NUM		(32) /* 单个节点支持最大的子节点个数 */

#define STS_OK              (0x0)      /* 成功 */

/* 错误码 */
#define STS_ERR_CODE_BASE	(0x77770000)
#define STS_PTR_NULL		(STS_ERR_CODE_BASE + 0x1)      /* 空指针 */
#define STS_MALLOC_FAILED	(STS_ERR_CODE_BASE + 0x2)      /* 内存申请失败 */
#define STS_MOD_NUM_FULL	(STS_ERR_CODE_BASE + 0x3)      /* 状态维护模块注册的模块已经满 */
#define STS_HANDLE_ERROR	(STS_ERR_CODE_BASE + 0x4)      /* 全局handle指针异常 */
#define STS_INVALID_IDX		(STS_ERR_CODE_BASE + 0x5)      /* 异常模块索引值 */
#define STS_INPUT_PARAM		(STS_ERR_CODE_BASE + 0x6)      /* 用户入参异常 */

/*
   类别层级说明:

   一级类别---
       |
       |------二级子类_1------
       |          |
       |          |---三级子类_1    ...
       |          |
       |------二级子类_2   
       |          |
       |          |---三级子类_2
       |    ...   |
       |          |
       |          |    ...
       |------二级子类_N   
                  |
                  |---三级子类_K

  一级类别---
       |
       |------二级子类_1   ...
       |
       |------二级子类_2
       |
       |   ...
       |
       |
       |------二级子类_N
 */

/* 维护模块信息输出方式 */
typedef enum _STS_OUTPUT_TYPE_
{
    STS_OUTPUT_FILE = 0,                  /* 写文件方式输出维护信息 */
    STS_OUTPUT_STDOUT,                    /* 串口终端方式输出维护信息 */
    STS_OUTPUT_TYPE_NUM,
} STS_OUTPUT_TYPE_E;

/* 状态变量类型 */
typedef enum _STS_NODE_VAL_TYPE_
{
    STS_VAL_TYPE_NONE = 0,                /* 无需关注变量类型 */
    STS_VAL_TYPE_CHAR = 1,                /* 字符类型 */
    STS_VAL_TYPE_INT = 2,                 /* 整形 */
    STS_VAL_TYPE_FLOAT = 3,               /* 浮点型 */
    STS_VAL_TYPE_STRING = 4,              /* 字符串 */

    STS_VAL_TYPE_NUM,
} STS_NODE_VAL_TYPE_E;

typedef void *(*sts_get_outer_val)(void);    /* 回调函数: 用于维护子模块获取外部注册模块的节点数据 */

typedef struct _STS_NODE_VAL_INFO_
{
    sts_get_outer_val pGetValCb;                     /* 回调函数 */
} STS_NODE_VAL_INFO_S;

typedef struct _STS_LAYER_NODE_PRM_
{
    STS_NODE_VAL_TYPE_E enValType;                  /* 节点变量类型，后续统一使用字符串，不需要区分变量类型 */
    STS_NODE_VAL_INFO_S stNodeValInfo;              /* 节点变量信息 */
    char acNodeName[MOD_STS_MAX_STRING_LEN];        /* 节点名称 */
    void *pPrvt;                                    /* 私有信息, 外部模块不关注 */
} STS_LAYER_NODE_PRM_S;

typedef struct _STS_MOD_NODE_INFO_
{
    int layer_id;                               /* 层级id */
    STS_LAYER_NODE_PRM_S stNodePrm;             /* 当前节点参数 */

    int child_node_cnt;                         /* 子节点个数 */
    void *pNextNode[MOD_STS_MAX_LAYER_NUM];     /* 子节点 */
} STS_MOD_NODE_INFO_S;

#if 0 /* todo: 更新外部可见的结构体，封装起来便于维护 */
/* 模块通用结构体 */
typedef struct _STS_MOD_COMM_INFO_
{
    void *pHandle;                 /* 模块通用句柄 */

    FUNC_P pRegModule;             /* 外部能力接口1: 注册模块 */
    FUNC_P pUnRegModule;           /* 外部能力接口2: 反注册模块 */
    FUNC_P pPrintInfo;             /* 外部能力接口1: 打印模块维护信息 */

    void *pUserPrvtInfo;           /* 用户私有信息 */
} STS_MOD_COMM_INFO_S;
#endif

/**
 * @function    sts_reg_entry
 * @brief       模块注册接口(外部接口)
 * @param[in]   STS_MOD_NODE_INFO_S *pstModLayerPrm  节点信息
 * @param[out]  void **ppHandle  模块通道句柄
 * @return
 */
int sts_reg_entry(STS_MOD_NODE_INFO_S *pstModLayerPrm, void **ppHandle);

/**
 * @function    sts_unreg_entry
 * @brief       模块反注册接口(外部接口)
 * @param[in]   void *pHandle  模块通道句柄
 * @param[out]
 * @return
 */
int sts_unreg_entry(void *pHandle);

/**
 * @function    sts_pr_node_info_by_name
 * @brief       打印模块指定节点的子树信息(外部接口)
 * @param[in]   char *mod_name   模块名
 *              char *node_name  节点名
 * @param[out]
 * @return
 */
void sts_pr_node_info_by_name(char *mod_name, char *node_name);

/**
 * @function    sts_set_output_type
 * @brief       设置维护信息输出类型(外部接口)
 * @param[in]   STS_OUTPUT_TYPE_E enIptType  维护信息输出方式
 * @param[out]
 * @return
 */
int sts_set_output_type(STS_OUTPUT_TYPE_E enIptType);

#endif

