/**
 * @file   sts.c
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

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "sts.h"
#include "malloc.h"

#define STS_MAGIC_NUM			(0x535A4C)         /* 魔数 */
#define STS_MAX_SUPT_MOD_NUM	(32)               /* sts模块支持最大的外部模块个数 */

typedef int HANDLE;

#define GET_MOD_IDX(h)		(char)(*(HANDLE *)h & 0xff)
#define GET_STS_MAGIC(h)	(int)(((*(HANDLE *)h) >> 8) & 0xffffff)

#define FORMAT_MIN_GAP	(16)      /* 格式化输出时的最小长度，单位为字符 */
#define OUTPUT_DIR_PATH "/tmp"
#define OUTPUT_BUF_SIZE (1024 * 1024)  /* 输出buf，1M */
#define OUTPUT_STRING_TMP_LEN (10240)  /* 维护信息输出中间数组长度 */
#define OUTPUT_FOTMAT_LEN (256)        /* 维护信息输出格式单行最大长度，单位字符 */

#define HANDLE_CHECKER(h) \
    { \
        if (NULL == h) \
        { \
            return STS_PTR_NULL; \
        } \
        if (STS_MAGIC_NUM != GET_STS_MAGIC(h)) \
        { \
            return STS_HANDLE_ERROR; \
        } \
    }

#define STS_GET_STR_BUF_LEFT_SIZE(str,len) ((len >= strlen(str)) ? len - strlen(str) : 0)        /* 用于字符串拼接时计算剩余大小 */

extern Ptr SAL_memMalloc(UINT32 size, CHAR *szModName, CHAR *szMemName);/*内存申请函数，包含模块名和内存名，方便统计内存*/
extern INT32 SAL_memfree(Ptr pPtr, CHAR *szModName, CHAR *szMemName);/*内存释放函数,与上方内存申请函数对应*/



typedef struct _STS_NODE_FORMAT_PRM_
{
    char iNodeNameLen;                       /* 层级宽度 */
} STS_NODE_FORMAT_PRM_S;

/* 获取节点字符串的回调函数 */
typedef int (*sts_get_val_string)(STS_NODE_VAL_TYPE_E enValType, STS_NODE_VAL_INFO_S *pstValInfo, char *pOutString, int iOutBufSize);

typedef struct _STS_NODE_PRVT_INFO_
{
    int bPrVal;                              /* 是否需要打印节点值 */
    sts_get_val_string pFuncGetValString;    /* 获取节点变量值的回调函数 */

    STS_NODE_FORMAT_PRM_S stNodeFormatPrm;   /* 格式化输出控制参数 */
} STS_NODE_PRVT_INFO_S;

typedef struct _STS_CHN_PRM_
{
    int useFlag;                               /* 模块使用标记 */

    HANDLE *pHandle;                           /* 模块句柄 */
    STS_MOD_NODE_INFO_S stModLayerPrm;         /* 模块层级信息-多叉树 */
} STS_CHN_PRM_S;

static char g_acTmp[OUTPUT_STRING_TMP_LEN] = {'\0'};        /* 中间变量，避免局部大堆栈 */
static char *g_acOutputString = NULL;                       /* 存放模块维护信息字符串，动态申请malloc */
static STS_OUTPUT_TYPE_E enOutputType = STS_OUTPUT_FILE;    /* 默认输出方式为写文件，因终端输出可能存在串口格式冲突 */

/* 单个模块支持的状态查询数组，通过过快内部定义的枚举个数MOD_STS_MAX_NUM和idx进行索引 */
static STS_CHN_PRM_S *pastModStsArr[STS_MAX_SUPT_MOD_NUM] = {0};
static pthread_mutex_t g_modArrLock = PTHREAD_MUTEX_INITIALIZER;		 /* 用户态互斥锁变量，用于模块写互斥 */

/**
 * @function    sts_mod_lock
 * @brief       模块上锁
 * @param[in]   void **ppMem  存放动态申请内存的指针变量
 *              int size      申请内存大小
 * @param[out]
 * @return
 */
static void sts_mod_lock(void)
{
    pthread_mutex_lock(&g_modArrLock);
    return;
}

/**
 * @function    sts_mod_unlock
 * @brief       模块释放锁
 * @param[in]   void **ppMem  存放动态申请内存的指针变量
 *              int size      申请内存大小
 * @param[out]
 * @return
 */
static void sts_mod_unlock(void)
{
    pthread_mutex_unlock(&g_modArrLock);
    return;
}

/**
 * @function    sts_malloc
 * @brief       系统内存申请
 * @param[in]   void **ppMem  存放动态申请内存的指针变量
 *              int size      申请内存大小
 * @param[out]
 * @return
 */
static int sts_malloc(void **ppMem, int size)
{
    if (NULL == ppMem || NULL != *ppMem)
    {
        return STS_PTR_NULL;
    }

    *ppMem = SAL_memMalloc(size, "STS", "sts_malloc");
    if (NULL == *ppMem)
    {
        printf("malloc failed! \n");
        return STS_MALLOC_FAILED;
    }

    memset(*ppMem, 0x00, size);

    return STS_OK;
}

/**
 * @function    sts_free
 * @brief       系统内存释放
 * @param[in]   void **ppMem  待释放的内存指针
 * @param[out]
 * @return
 */
static __attribute__((unused)) int sts_free(void **ppMem)
{
    if (NULL == ppMem)
    {
        return STS_PTR_NULL;
    }

    if (NULL == *ppMem)
    {
        return STS_OK;
    }

    SAL_memfree(*ppMem, "STS", "sts_malloc");
    *ppMem = NULL;

    return STS_OK;
}

/**
 * @function    sts_get_node_str_val
 * @brief       获取字符串
 * @param[in]   STS_NODE_VAL_INFO_S *pstValInfo  节点信息
 *              char *pOutString  存放节点字符串的外部缓存
 *              int iOutBufSize  外部缓存大小
 * @param[out]
 * @return
 */
static int sts_get_node_str_val(STS_NODE_VAL_INFO_S *pstValInfo, char *pOutString, int iOutBufSize)
{
    void *pTmpVal = NULL;

	if (NULL == pstValInfo || NULL == pOutString)
	{
	    return STS_PTR_NULL;
	}

    pTmpVal = pstValInfo->pGetValCb();
    if (NULL == pTmpVal)
    {
        return STS_PTR_NULL;
    }

    snprintf(pOutString, iOutBufSize, "%s", (char *)pTmpVal);

    return STS_OK;
}

/**
 * @function    sts_get_node_float_val
 * @brief       获取浮点
 * @param[in]   STS_NODE_VAL_INFO_S *pstValInfo  节点信息
 *              char *pOutString  存放节点字符串的外部缓存
 *              int iOutBufSize  外部缓存大小
 * @param[out]
 * @return
 */
static int sts_get_node_float_val(STS_NODE_VAL_INFO_S *pstValInfo, char *pOutString, int iOutBufSize)
{
    float *pfTmpVal = NULL;

	if (NULL == pstValInfo || NULL == pOutString)
	{
	    return STS_PTR_NULL;
	}

    pfTmpVal = pstValInfo->pGetValCb();
    if (NULL == pfTmpVal)
    {
        return STS_PTR_NULL;
    }

    snprintf(pOutString, iOutBufSize, "%f", *pfTmpVal);

    return STS_OK;
}

/**
 * @function    sts_get_node_int_val
 * @brief       获取整型
 * @param[in]   STS_NODE_VAL_INFO_S *pstValInfo  节点信息
 *              char *pOutString  存放节点字符串的外部缓存
 *              int iOutBufSize  外部缓存大小
 * @param[out]
 * @return
 */
static int sts_get_node_int_val(STS_NODE_VAL_INFO_S *pstValInfo, char *pOutString, int iOutBufSize)
{
    int *piTmpVal = NULL;

	if (NULL == pstValInfo || NULL == pOutString)
	{
	    return STS_PTR_NULL;
	}

    piTmpVal = pstValInfo->pGetValCb();
    if (NULL == piTmpVal)
    {
        return STS_PTR_NULL;
    }

    snprintf(pOutString, iOutBufSize, "%d", *piTmpVal);

    return STS_OK;
}

/**
 * @function    sts_get_node_char_val
 * @brief       获取字符
 * @param[in]   STS_NODE_VAL_INFO_S *pstValInfo  节点信息
 *              char *pOutString  存放节点字符串的外部缓存
 *              int iOutBufSize  外部缓存大小
 * @param[out]
 * @return
 */
static int sts_get_node_char_val(STS_NODE_VAL_INFO_S *pstValInfo, char *pOutString, int iOutBufSize)
{
    char *pcTmpVal = NULL;

	if (NULL == pstValInfo || NULL == pOutString)
	{
	    return STS_PTR_NULL;
	}
	
    pcTmpVal = pstValInfo->pGetValCb();
    if (NULL == pcTmpVal)
    {        
        return STS_PTR_NULL;
    }

    snprintf(pOutString, iOutBufSize, "%c", *pcTmpVal);

    return STS_OK;
}

/**
 * @function    sts_get_node_val_string
 * @brief       获取全局变量字符串
 * @param[in]   STS_NODE_VAL_TYPE_E enValType    节点变量类型
 *              STS_NODE_VAL_INFO_S *pstValInfo  节点信息
 *              char *pOutString  存放节点字符串的外部缓存
 *              int iOutBufSize  外部缓存大小
 * @param[out]
 * @return
 */
static int sts_get_node_val_string(STS_NODE_VAL_TYPE_E enValType, STS_NODE_VAL_INFO_S *pstValInfo, char *pOutString, int iOutBufSize)
{
    int r = STS_OK;

    /* todo: 后续考虑将获取不同类型变量的接口统一掉 */
    /* char *pacValType2FmtMap[STS_VAL_TYPE_NUM] = {"  ", "%c", "%d", "%f", "%s"}; */

    if (STS_VAL_TYPE_NONE == enValType)
    {
        printf("no need get val! return success! \n");
        return STS_OK;
    }

    if (NULL == pstValInfo || NULL == pOutString)
    {
        printf("val info null! %p, %p \n", pstValInfo, pOutString);
        return STS_PTR_NULL;
    }

    switch (enValType)
    {
        case STS_VAL_TYPE_CHAR:
        {
            r = sts_get_node_char_val(pstValInfo, pOutString, iOutBufSize);
            break;
        }
        case STS_VAL_TYPE_INT:
        {
            r = sts_get_node_int_val(pstValInfo, pOutString, iOutBufSize);
            break;
        }
        case STS_VAL_TYPE_FLOAT:
        {
            r = sts_get_node_float_val(pstValInfo, pOutString, iOutBufSize);
            break;
        }
        case STS_VAL_TYPE_STRING:
        {
            r = sts_get_node_str_val(pstValInfo, pOutString, iOutBufSize);
            break;
        }
        default:
        {
            printf("invalid val type %d \n", enValType);
            break;
        }
    }

    return r;
}

/**
 * @function    sts_init_prvt_by_dfs
 * @brief       深度优先遍历初始化节点私有信息+获取格式化信息
 * @param[in]   unsigned int uiLen  长度
 *              STS_MOD_NODE_INFO_S *pstModLayerPrm  节点信息
 * @param[out]
 * @return
 */
static int sts_init_prvt_by_dfs(unsigned int uiLen, STS_MOD_NODE_INFO_S *pstModLayerPrm)
{
    int r = STS_OK;

    int i;

    STS_NODE_PRVT_INFO_S *pstNodePrvtInfo = NULL;

    if (NULL == pstModLayerPrm)
    {
        return 0;
    }

    if (NULL == pstModLayerPrm->stNodePrm.pPrvt)
    {
        r = sts_malloc(&pstModLayerPrm->stNodePrm.pPrvt, sizeof(STS_NODE_PRVT_INFO_S));
        if (STS_OK != r)
        {
            printf("malloc failed! \n");
            return STS_MALLOC_FAILED;
        }
    }

    pstNodePrvtInfo = (STS_NODE_PRVT_INFO_S *)pstModLayerPrm->stNodePrm.pPrvt;

    if (STS_VAL_TYPE_NONE != pstModLayerPrm->stNodePrm.enValType)
    {
        pstNodePrvtInfo->bPrVal = 1;
        pstNodePrvtInfo->pFuncGetValString = (sts_get_val_string)sts_get_node_val_string;
    }

    for (i = 0; i < pstModLayerPrm->child_node_cnt; i++)
    {
        sts_init_prvt_by_dfs(uiLen / 2, pstModLayerPrm->pNextNode[i]);
    }

    /* 如果子节点长度和为0，表示当前节点为叶子节点，那么需要返回当前节点名称的长度，两边还需要预留两倍gap值。e.g. |  %NODE_NAME%  | */
    pstNodePrvtInfo->stNodeFormatPrm.iNodeNameLen = uiLen; /* ((len < FORMAT_MIN_GAP) ? FORMAT_MIN_GAP : (len > 128 ? 128 : len)); */

    return STS_OK; /* pstNodePrvtInfo->stNodeFormatPrm.iNodeNameLen; */
}

/**
 * @function    sts_init_node_prvt_info
 * @brief       初始化节点私有信息
 * @param[in]   int iModIdx  节点索引
 * @param[out]
 * @return
 */
static int sts_init_node_prvt_info(int iModIdx)
{
    (void)sts_init_prvt_by_dfs(OUTPUT_FOTMAT_LEN, &pastModStsArr[iModIdx]->stModLayerPrm);
    return STS_OK;
}

/**
 * @function    sts_init_mod_info
 * @brief       初始化通道全局参数
 * @param[in]   STS_MOD_NODE_INFO_S *pstModLayerPrm  节点信息
 *              char cModIdx  模块索引
 * @param[out]
 * @return
 */
static int sts_init_mod_info(STS_MOD_NODE_INFO_S *pstModLayerPrm, char cModIdx)
{
    if (cModIdx >= STS_MAX_SUPT_MOD_NUM)
    {
        return STS_INVALID_IDX;
    }

    int iModIdxTmp = cModIdx;

    if (NULL == pastModStsArr[iModIdxTmp])
    {
        if (STS_OK != sts_malloc((void **)&pastModStsArr[iModIdxTmp], sizeof(STS_CHN_PRM_S)))
        {
            printf("malloc failed! \n");
            return STS_MALLOC_FAILED;
        }
    }

    memcpy(&pastModStsArr[iModIdxTmp]->stModLayerPrm, pstModLayerPrm, sizeof(STS_MOD_NODE_INFO_S));

    /* 使能 */
    pastModStsArr[iModIdxTmp]->useFlag = 1;
    return STS_OK;
}

/**
 * @function    sts_get_free_mod_idx
 * @brief       获取空闲模块索引
 * @param[in]   
 * @param[out]  char *cModIdx  节点索引
 * @return
 */
static int sts_get_free_mod_idx(char *cModIdx)
{
    int r = STS_OK;
    int i = 0;

	sts_mod_lock();
	
    for (i = 0; i < STS_MAX_SUPT_MOD_NUM; i++)
    {
        if (NULL != pastModStsArr[i] && 1 == pastModStsArr[i]->useFlag)
        {
            continue;
        }

        break;
    }

    if (i >= STS_MAX_SUPT_MOD_NUM)
    {
        r = STS_MOD_NUM_FULL;
        goto exit;
    }

    *cModIdx = (char)i;

exit:
	sts_mod_unlock();
    return r;
}

/**
 * @function    sts_create_handle
 * @brief       创建句柄
 * @param[in]   char *mod_idx     模块索引
 * @param[out]  void **ppHandle   模块通道句柄
 * @return
 */
static int sts_create_handle(char *mod_idx, void **ppHandle)
{
    int r = STS_OK;
	
    int iModIdxTmp = 0;
    HANDLE *pHandle = NULL;

    if (NULL == ppHandle)
    {
        printf("create handle failed! \n");
        return STS_PTR_NULL;
    }

    if (*ppHandle != NULL && (STS_MAGIC_NUM != (*(HANDLE *)(*ppHandle)) >> 8))
    {
        printf("magic invalid! \n");
        return STS_HANDLE_ERROR;
    }

    r = sts_get_free_mod_idx((char *)&iModIdxTmp);
    if (STS_OK != r)
    {
        printf("get free mod idx failed! \n");
        return r;
    }

    if (NULL == pastModStsArr[iModIdxTmp])
    {
        if (STS_OK != sts_malloc((void **)&pastModStsArr[iModIdxTmp], sizeof(STS_CHN_PRM_S)))
        {
            printf("malloc failed! \n");
            return STS_MALLOC_FAILED;
        }
    }

	pHandle = SAL_memMalloc(sizeof(HANDLE), "STS", "sts_phandle");
	if (NULL == pHandle)
	{
		printf("malloc failed! \n");
		return STS_MALLOC_FAILED;
	}
	
	memset(pHandle, 0x00, sizeof(HANDLE));

    *pHandle = (HANDLE)((STS_MAGIC_NUM << 8) | (iModIdxTmp & 0xff));

    pastModStsArr[iModIdxTmp]->pHandle = pHandle;

    /* 导出外部使用的通道句柄 */
    *ppHandle = pHandle;
	*mod_idx = (char)(iModIdxTmp & 0xff);

    printf("create handle success! \n");
    return STS_OK;
}

/**
 * @function    sts_destroy_handle
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static __attribute__((unused)) int sts_destroy_handle(void *pHandle)
{
    int mod_idx = 0;

    HANDLE_CHECKER(pHandle);

    mod_idx = GET_MOD_IDX(pHandle);
    if (mod_idx >= STS_MAX_SUPT_MOD_NUM)
    {
        return STS_INVALID_IDX;
    }

    pastModStsArr[mod_idx]->useFlag = 0;
    return STS_OK;
}

/**
 * @function    sts_put_mod_idx
 * @brief       释放模块索引
 * @param[in]   char cModIdx  模块索引
 * @param[out]
 * @return
 */
static int sts_put_mod_idx(char cModIdx)
{
    int r = STS_OK;

    if (cModIdx >= STS_MAX_SUPT_MOD_NUM)
    {
        r = STS_INVALID_IDX;
        goto exit;
    }

    int iModIdxTmp = cModIdx;

    if (NULL != pastModStsArr[iModIdxTmp])
    {
        pastModStsArr[iModIdxTmp]->useFlag = 0;
    }

exit:
    return r;
}

/**
 * @function    sts_reg_entry
 * @brief       模块注册接口(外部接口)
 * @param[in]   STS_MOD_NODE_INFO_S *pstModLayerPrm  节点信息
 * @param[out]  void **ppHandle  模块通道句柄
 * @return
 */
int sts_reg_entry(STS_MOD_NODE_INFO_S *pstModLayerPrm, void **ppHandle)
{
    int r = STS_OK;
    char cModIdx = 0;

    if (NULL == pstModLayerPrm || NULL == ppHandle)
    {
        printf("ptr null! \n");
        return STS_PTR_NULL;
    }
	
    r = sts_create_handle(&cModIdx, ppHandle);
    if (STS_OK != r)
    {
        printf("create handle failed! \n");
        return r;
    }

    r = sts_init_mod_info(pstModLayerPrm, cModIdx);
    if (STS_OK != r)
    {
        printf("init mod info failed! \n");
        return r;
    }

    /* 初始化通道全局节点私有信息 */
    (void)sts_init_node_prvt_info(cModIdx);

    return STS_OK;
}

/**
 * @function    sts_unreg_entry
 * @brief       模块反注册接口(外部接口)
 * @param[in]   void *pHandle  模块通道句柄
 * @param[out]
 * @return
 */
int sts_unreg_entry(void *pHandle)
{
    int r = STS_OK;

    char cModIdx = 0;

    HANDLE_CHECKER(pHandle);

    cModIdx = (char)GET_MOD_IDX(pHandle);

    r = sts_put_mod_idx(cModIdx);
    if (STS_OK != r)
    {
        /* 失败不异常返回，仅产生error打印 */
        printf("put mod idx failed! \n");
    }

    return STS_OK;
}

/**
 * @function    sts_output
 * @brief
 * @param[in]   HANDLE *pHandle  模块通道句柄
 * @param[out]
 * @return
 */
static void sts_output(HANDLE *pHandle)
{
    int end_idx = 0;
	
    if (NULL == g_acOutputString)
    {
        (void)sts_malloc((void **)&g_acOutputString, OUTPUT_BUF_SIZE);
        g_acOutputString[0] = '\0';
    }

	end_idx = (strlen(g_acOutputString) >= OUTPUT_BUF_SIZE-1) ? OUTPUT_BUF_SIZE-1 : strlen(g_acOutputString);

    /* 添加字符串终止符 */
    g_acOutputString[end_idx] = '\0';

    if (STS_OUTPUT_FILE == enOutputType)
    {
        int iTmpIdx = GET_MOD_IDX(pHandle);
        char acFile[128] = {'\0'};
        FILE *fp = NULL;

        if (iTmpIdx >= STS_MAX_SUPT_MOD_NUM
            || NULL == pastModStsArr[iTmpIdx]
            || 0 == pastModStsArr[iTmpIdx]->useFlag)
        {
            printf("invalid idx %d \n", iTmpIdx);
            return;
        }

        snprintf(acFile, 128, "%s/mod_%s_info.txt", OUTPUT_DIR_PATH, pastModStsArr[iTmpIdx]->stModLayerPrm.stNodePrm.acNodeName);

        fp = fopen(acFile, "w+");
        if (NULL == fp)
        {
            printf("fopen failed! %s \n", acFile);
            return;
        }

        fwrite(g_acOutputString, strlen(g_acOutputString), 1, fp);
        fflush(fp);

        fclose(fp);
        fp = NULL;
    }
    else if (STS_OUTPUT_STDOUT == enOutputType)
    {
        printf("%s", g_acOutputString);
    }
    else
    {
        printf("invalid output type %d \n", enOutputType);
    }

    return;
}

/**
 * @function    sts_pr_by_dfs
 * @brief       打印维护信息(dfs)
 * @param[in]   STS_MOD_NODE_INFO_S *pstNodeInfo  节点信息
 * @param[out]
 * @return
 */
static int sts_pr_by_dfs(STS_MOD_NODE_INFO_S *pstNodeInfo)
{
    int i;
    int tmp_gap = 0;

    STS_NODE_PRVT_INFO_S *pstNodePrvtInfo = NULL;

    if (NULL == pstNodeInfo->stNodePrm.pPrvt)
    {
        printf("pr: get node prvt info failed! layer_id %d \n", pstNodeInfo->layer_id);
        return STS_PTR_NULL;
    }

    pstNodePrvtInfo = (STS_NODE_PRVT_INFO_S *)pstNodeInfo->stNodePrm.pPrvt;

    tmp_gap = (pstNodePrvtInfo->stNodeFormatPrm.iNodeNameLen - strlen(pstNodeInfo->stNodePrm.acNodeName)) / 2;

    if (STS_VAL_TYPE_NONE != pstNodeInfo->stNodePrm.enValType)
    {
        memset(g_acTmp, '\0', OUTPUT_STRING_TMP_LEN);
        pstNodePrvtInfo->pFuncGetValString(pstNodeInfo->stNodePrm.enValType, \
                                           &pstNodeInfo->stNodePrm.stNodeValInfo, \
                                           g_acTmp, \
                                           OUTPUT_STRING_TMP_LEN);
        snprintf(g_acOutputString + strlen(g_acOutputString), STS_GET_STR_BUF_LEFT_SIZE(g_acOutputString, OUTPUT_BUF_SIZE), "%-24s: %s\n", pstNodeInfo->stNodePrm.acNodeName, g_acTmp);
    }
    else
    {
        /* 打印上横条 */
        for (i = 0; i < pstNodePrvtInfo->stNodeFormatPrm.iNodeNameLen; i++)
        {
            snprintf(g_acOutputString + strlen(g_acOutputString), STS_GET_STR_BUF_LEFT_SIZE(g_acOutputString, OUTPUT_BUF_SIZE), "%s", "-");
        }

        snprintf(g_acOutputString + strlen(g_acOutputString), STS_GET_STR_BUF_LEFT_SIZE(g_acOutputString, OUTPUT_BUF_SIZE), "%s", "\n");

        /* 打印节点名称 */
        snprintf(g_acOutputString + strlen(g_acOutputString), STS_GET_STR_BUF_LEFT_SIZE(g_acOutputString, OUTPUT_BUF_SIZE), "%-*c%-s%*c\n", tmp_gap + 1, '|', pstNodeInfo->stNodePrm.acNodeName, tmp_gap + 1, '|');

        /* 打印下横条 */
        for (i = 0; i < pstNodePrvtInfo->stNodeFormatPrm.iNodeNameLen; i++)
        {
            snprintf(g_acOutputString + strlen(g_acOutputString), STS_GET_STR_BUF_LEFT_SIZE(g_acOutputString, OUTPUT_BUF_SIZE), "%s", "-");
        }

        snprintf(g_acOutputString + strlen(g_acOutputString), STS_GET_STR_BUF_LEFT_SIZE(g_acOutputString, OUTPUT_BUF_SIZE), "%s", "\n");
    }

    for (i = 0; i < pstNodeInfo->child_node_cnt; i++)
    {
        if (NULL == pstNodeInfo->pNextNode[i])
        {
            printf("get next node ptr null! i %d, layer_id %d, total %d \n", i, pstNodeInfo->layer_id, pstNodeInfo->child_node_cnt);
            continue;
        }

        sts_pr_by_dfs((STS_MOD_NODE_INFO_S *)pstNodeInfo->pNextNode[i]);
    }

    return STS_OK;
}

/**
 * @function    sts_pr_mod_info
 * @brief       打印模块维护信息
 * @param[in]   void *pHandle  模块通道句柄
 * @param[out]
 * @return
 */
static int sts_pr_mod_info(void *pHandle)
{
    int i = 0;
    int mod_idx = 0;

    STS_MOD_NODE_INFO_S *pstNodeInfo = NULL;

    HANDLE_CHECKER(pHandle);

    mod_idx = GET_MOD_IDX(pHandle);
    if (mod_idx >= STS_MAX_SUPT_MOD_NUM)
    {
        return STS_INVALID_IDX;
    }

    if (NULL == pastModStsArr[mod_idx])
    {
        return STS_PTR_NULL;
    }

    if (1 != pastModStsArr[mod_idx]->useFlag)
    {
        printf("mod %d, not using! \n", mod_idx);
        return STS_OK;
    }

    if (NULL == g_acOutputString)
    {
        (void)sts_malloc((void **)&g_acOutputString, OUTPUT_BUF_SIZE);
    }

    /* 清空+初始化输出缓存 */
    snprintf(g_acOutputString, OUTPUT_BUF_SIZE, "%s", "\n");

    pstNodeInfo = &pastModStsArr[mod_idx]->stModLayerPrm;

    /* 从layer_id为1的层级开始打印 */
    for (i = 0; i < pstNodeInfo->child_node_cnt; i++)
    {
        if (NULL == pstNodeInfo->pNextNode[i])
        {
            printf("get next node ptr null! i %d, layer_id %d, total %d \n", i, pstNodeInfo->layer_id, pstNodeInfo->child_node_cnt);
            continue;
        }

        sts_pr_by_dfs(pstNodeInfo->pNextNode[i]);
        printf("\n\n");
    }

    sts_output(pHandle);

    return STS_OK;
}

/**
 * @function    sts_find_mod_handle
 * @brief       寻找模块句柄
 * @param[in]   char *mod_name  模块名
 * @param[out]
 * @return
 */
static void *sts_find_mod_handle(char *mod_name)
{
    int i = 0;
    HANDLE *pHandle = NULL;

    if (NULL == mod_name)
    {
        return NULL;
    }

    for (i = 0; i < STS_MAX_SUPT_MOD_NUM; i++)
    {
        if ((NULL == pastModStsArr[i] || 1 != pastModStsArr[i]->useFlag)
            || (NULL == strstr(pastModStsArr[i]->stModLayerPrm.stNodePrm.acNodeName, mod_name)))
        {
            continue;
        }

        pHandle = pastModStsArr[i]->pHandle;
        break;
    }

    return pHandle;
}

/**
 * @function    sts_find_node_info_by_dfs
 * @brief       寻找指定节点名称的子树(递归)
 * @param[in]   char *pNodeName  节点名称
 *              STS_MOD_NODE_INFO_S *pstNodeInfo  节点信息
 * @param[out]
 * @return
 */
static STS_MOD_NODE_INFO_S *sts_find_node_info_by_dfs(char *pNodeName, STS_MOD_NODE_INFO_S *pstNodeInfo)
{
    int i;
    STS_MOD_NODE_INFO_S *pstTmpNode = NULL;

    if (NULL == pstNodeInfo)
    {
        printf("ptr null! \n");
        return NULL;
    }

    if (NULL != strstr(pstNodeInfo->stNodePrm.acNodeName, pNodeName))
    {
        return pstNodeInfo;
    }

    for (i = 0; i < pstNodeInfo->child_node_cnt; i++)
    {
        if (NULL != (pstTmpNode = sts_find_node_info_by_dfs(pNodeName, (STS_MOD_NODE_INFO_S *)pstNodeInfo->pNextNode[i])))
        {
            return pstTmpNode;
        }
    }

    return NULL;
}

/**
 * @function    sts_pr_node_info_by_name
 * @brief       打印模块指定节点的子树信息(外部接口)
 * @param[in]   char *mod_name   模块名
 *              char *node_name  节点名
 * @param[out]
 * @return
 */
void sts_pr_node_info_by_name(char *mod_name, char *node_name)
{
    int iTmpModIdx = 0;
    int i;

    HANDLE *pHandle = NULL;
    STS_CHN_PRM_S *pstModChnPrm = NULL;
    STS_MOD_NODE_INFO_S *pstNodeInfo = NULL;

    pHandle = sts_find_mod_handle(mod_name);
    if (NULL == pHandle)
    {
        printf("find mod %s handle failed! \n", mod_name);
        return;
    }

    iTmpModIdx = GET_MOD_IDX(pHandle);
    if (NULL == (pstModChnPrm = pastModStsArr[iTmpModIdx]))
    {
        printf("mod info null \n");
        return;
    }

    if (1 != pstModChnPrm->useFlag)
    {
        printf("mod not enable! pls check! \n");
        return;
    }

    /* 若外部传入的节点名称不为空，则递归查找对应节点子树信息。若传入节点为空，则打印全部根节点信息 */
    if (NULL != node_name)
    {
        for (i = 0; i < pstModChnPrm->stModLayerPrm.child_node_cnt; i++)
        {
            if (NULL != (pstNodeInfo = sts_find_node_info_by_dfs(node_name, pstModChnPrm->stModLayerPrm.pNextNode[i])))
            {
                break;
            }
        }

        if (NULL == pstNodeInfo)
        {
            printf("not found! mod %s, node %s \n", mod_name, node_name);
            return;
        }

        if (NULL == g_acOutputString)
        {
            printf("!!!! need malloc !!!!! \n");
            (void)sts_malloc((void **)&g_acOutputString, OUTPUT_BUF_SIZE);
        }

        snprintf(g_acOutputString, OUTPUT_BUF_SIZE, "%s", "\n");
		
        sts_pr_by_dfs(pstNodeInfo);
		sts_output(pHandle);
    }
    else
    {
        if (NULL == g_acOutputString)
        {
            printf("!!!! need malloc !!!!! \n");
            (void)sts_malloc((void **)&g_acOutputString, OUTPUT_BUF_SIZE);
        }

        snprintf(g_acOutputString, OUTPUT_BUF_SIZE, "%s", "\n");

        sts_pr_mod_info(pHandle);
    }

    return;
}

/**
 * @function    sts_set_output_type
 * @brief       设置维护信息输出类型(外部接口)
 * @param[in]   STS_OUTPUT_TYPE_E enIptType  维护信息输出方式
 * @param[out]
 * @return
 */
int sts_set_output_type(STS_OUTPUT_TYPE_E enIptType)
{
    if (enIptType >= STS_OUTPUT_TYPE_NUM)
    {
        printf("set output type failed! %d \n", enIptType);
        return STS_INPUT_PARAM;
    }

    enOutputType = enIptType;
    return STS_OK;
}

#if 1 /* 维护模块demo代码 */

/*
   demo初始化的维护信息如下:

   -----------------------------------------------------------
   |                         dbg                             |
   -----------------------------------------------------------
   test_1       : 77
   test_11      : test_string
   test_12      : t
   test_13      : 77.000000


 */

static STS_MOD_NODE_INFO_S stDemoNodeInfo = {0};

static int demo_int_val = 77;

/**
 * @function    demo_int_val_cb
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *demo_int_val_cb(void)
{
    return &demo_int_val;
}

static char demo_char_val = 't';

/**
 * @function    demo_char_val_cb
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *demo_char_val_cb(void)
{
    return &demo_char_val;
}

#define STS_DEMO_STRING "test_string"

static char *demo_str_val = STS_DEMO_STRING;

/**
 * @function    demo_str_val_cb
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *demo_str_val_cb(void)
{
    return demo_str_val;
}

static float demo_float_val = 77.0;

/**
 * @function    demo_float_val_cb
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *demo_float_val_cb(void)
{
    return &demo_float_val;
}

/**
 * @function    demo_init_tree
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void demo_init_tree(void)
{
    STS_MOD_NODE_INFO_S *pstTmpNode = NULL;
    STS_MOD_NODE_INFO_S *pstTmpNode_1 = NULL;

    /* 根节点信息不需要初始化 */
    stDemoNodeInfo.layer_id = 0;
    stDemoNodeInfo.stNodePrm.enValType = STS_VAL_TYPE_NONE;
    snprintf(stDemoNodeInfo.stNodePrm.acNodeName, MOD_STS_MAX_STRING_LEN, "%s", "dbg");
    stDemoNodeInfo.child_node_cnt = 1; /* 2; */
    (void)sts_malloc((void **)&stDemoNodeInfo.pNextNode[0], sizeof(STS_MOD_NODE_INFO_S));
    /* (void)sts_malloc((void **)&stDemoNodeInfo.pNextNode[1], sizeof(STS_MOD_NODE_INFO_S)); */

    /* 初始化第一个子树---debug_0 */
    pstTmpNode = (STS_MOD_NODE_INFO_S *)stDemoNodeInfo.pNextNode[0];

    pstTmpNode->layer_id = 0;
    snprintf(pstTmpNode->stNodePrm.acNodeName, MOD_STS_MAX_STRING_LEN, "%s", "comm");
    pstTmpNode->stNodePrm.enValType = STS_VAL_TYPE_NONE;           /* 非叶子节点不需要打印变量 */

    pstTmpNode->child_node_cnt = 4;
    (void)sts_malloc((void **)&pstTmpNode->pNextNode[0], sizeof(STS_MOD_NODE_INFO_S));
    (void)sts_malloc((void **)&pstTmpNode->pNextNode[1], sizeof(STS_MOD_NODE_INFO_S));
    (void)sts_malloc((void **)&pstTmpNode->pNextNode[2], sizeof(STS_MOD_NODE_INFO_S));
    (void)sts_malloc((void **)&pstTmpNode->pNextNode[3], sizeof(STS_MOD_NODE_INFO_S));

#if 1
    pstTmpNode_1 = (STS_MOD_NODE_INFO_S *)pstTmpNode->pNextNode[0];

    pstTmpNode_1->layer_id = 0;
    snprintf(pstTmpNode_1->stNodePrm.acNodeName, MOD_STS_MAX_STRING_LEN, "%s", "test_1");
    pstTmpNode_1->stNodePrm.enValType = STS_VAL_TYPE_INT;           /* 叶子节点需要打印变量 */
    pstTmpNode_1->stNodePrm.stNodeValInfo.pGetValCb = (sts_get_outer_val)demo_int_val_cb;

    pstTmpNode_1->child_node_cnt = 0;
#endif

#if 1
    pstTmpNode_1 = (STS_MOD_NODE_INFO_S *)pstTmpNode->pNextNode[1];

    pstTmpNode_1->layer_id = 1;
    snprintf(pstTmpNode_1->stNodePrm.acNodeName, MOD_STS_MAX_STRING_LEN, "%s", "test_11");
    pstTmpNode_1->stNodePrm.enValType = STS_VAL_TYPE_STRING;            /* 非叶子节点不需要打印变量 */
    pstTmpNode_1->stNodePrm.stNodeValInfo.pGetValCb = (sts_get_outer_val)demo_str_val_cb;

    pstTmpNode_1->child_node_cnt = 0;
#endif

#if 1
    pstTmpNode_1 = (STS_MOD_NODE_INFO_S *)pstTmpNode->pNextNode[2];

    pstTmpNode_1->layer_id = 2;
    snprintf(pstTmpNode_1->stNodePrm.acNodeName, MOD_STS_MAX_STRING_LEN, "%s", "test_12");
    pstTmpNode_1->stNodePrm.enValType = STS_VAL_TYPE_CHAR;          /* 非叶子节点不需要打印变量 */
    pstTmpNode_1->stNodePrm.stNodeValInfo.pGetValCb = (sts_get_outer_val)demo_char_val_cb;

    pstTmpNode_1->child_node_cnt = 0;
#endif

#if 1
    pstTmpNode_1 = (STS_MOD_NODE_INFO_S *)pstTmpNode->pNextNode[3];

    pstTmpNode_1->layer_id = 3;
    snprintf(pstTmpNode_1->stNodePrm.acNodeName, MOD_STS_MAX_STRING_LEN, "%s", "test_13");
    pstTmpNode_1->stNodePrm.enValType = STS_VAL_TYPE_FLOAT;         /* 非叶子节点不需要打印变量 */
    pstTmpNode_1->stNodePrm.stNodeValInfo.pGetValCb = (sts_get_outer_val)demo_float_val_cb;

    pstTmpNode_1->child_node_cnt = 0;
#endif

    return;
}

/**
 * @function    sts_demo
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
int sts_demo(void)
{
    int r = 0;
    void *pHandle = NULL;

    /* 初始化多叉树 */
    (void)demo_init_tree();

    /* 注册外部模块 */
    r = sts_reg_entry(&stDemoNodeInfo, &pHandle);
    if (0 != r)
    {
        printf("reg module entry failed! \n");
        goto exit;
    }

#if 0
    /* 打印模块调试信息 */
    r = sts_pr_mod_info(pHandle);
    if (0 != r)
    {
        printf("prt module info failed! \n");
        goto exit;
    }

    /* 反注册外部模块 */
    r = sts_unreg_entry(pHandle);
    if (0 != r)
    {
        printf("unreg module failed! \n");
        goto exit;
    }

#endif

    printf("sts demo end! \n");
    return 0;
exit:
    return -1;
}

#endif

