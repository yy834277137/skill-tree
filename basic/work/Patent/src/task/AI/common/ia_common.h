/*******************************************************************************
* ia_common.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2019年11月25日 Create
*
* Description :
* Modification:
*******************************************************************************/

#ifndef _IA_COMMON_H_
#define _IA_COMMON_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <platform_hal.h>
#include "encrypt_proc.h"
#include "hka_types.h"
#include "cJSON.h"
#include "capbility.h"

/* ========================================================================== */
/*                             宏定义区                                       */
/* ========================================================================== */
#define IA_MEM_TYPE (5)     /* szl_todo: unused, delete later */

#define INPUT_MAX_NUM	(2)
#define AI_MODEL_NUM	(2)    /* Ai 开发平台最多数量 */
#define ASM_MODEL_NUM	(2)    /* 安检机模型最大数量 ，双模型为2 */


/* BA Module Mmz Mem */
#define BA_MMZ_CACHE_SIZE (50 * 1024 * 1024)

#ifdef DEBUG_SVA_DUMP
#define SVA_DEBUG_DUMP_MEM (500 * 1024 * 1024)
#else
#define SVA_DEBUG_DUMP_MEM (0)
#endif

/* FACE Module Mmz Mem */
#define FACE_MMZ_CACHE_SIZE (0)

#define IA_MAX_MODEL_NUM (SVA_XSI_MODEL_MAX_NUM > SVA_AI_MODEL_MAX_NUM ? SVA_XSI_MODEL_MAX_NUM : SVA_AI_MODEL_MAX_NUM)

/* ========================================================================== */
/*                             数据结构区                                     */
/* ========================================================================== */
typedef enum tagIaMemType
{
	IA_MALLOC = 0,
    IA_HISI_MMZ_CACHE,
    IA_HISI_MMZ_NO_CACHE,
    IA_HISI_MMZ_CACHE_PRIORITY,
    IA_HISI_MMZ_CACHE_NO_PRIORITY,
	IA_HISI_VB_REMAP_NONE,
	IA_HISI_VB_REMAP_NO_CACHED,
	IA_HISI_VB_REMAP_CACHED,	
	IA_NT_MEM_FEATURE,
	IA_RK_CACHE_CMA,
	IA_RK_CACHE_IOMMU,
    IA_MEM_TYPE_NUM,
} IA_MEM_TYPE_E;

#if 0
typedef enum tagIaProductType
{
    IA_ANALYZER_TYPE = 0,
    IA_PEOPLE_PACKAGE_MATCH_TYPE,
    IA_SECURTIY_MACHINE_TYPE,

    IA_PRODUCT_TYPE_NUM,
} IA_PRODUCT_TYPE_E;
#endif

typedef enum tagDeviceType
{
    ANALYZER_SINGLE_INPUT = 0,                  /* 分析仪单输入设备(单芯片) */
    ANALYZER_SINGLE_INPUT_MAIN,                 /* 分析仪单输入设备(双芯片-主) */
    ANALYZER_SINGLE_INPUT_SLAVE,                /* 分析仪单输入设备(双芯片-从) */
    ANALYZER_DOUBLE_INPUT,                      /* 分析仪双输入设备(单芯片) */
    ANALYZER_DOUBLE_INPUT_MAIN,                 /* 分析仪双输入设备(双芯片-主) */
    ANALYZER_DOUBLE_INPUT_SLAVE,                /* 分析仪双输入设备(双芯片-从) */

    SECURITY_MACHINE_TYPE_SINGLE,               /* 安检机设备单视角*/
    SECURITY_MACHINE_TYPE_DOUBLE,               /* 安检机设备双视角*/

    PEOPLE_PACKAGE_MATCH_SINGLE_INPUT_MAIN,     /* 人包关联单输入(主) */
    PEOPLE_PACKAGE_MATCH_SINGLE_INPUT_SLAVE,    /* 人包关联单输入(从) */
    PEOPLE_PACKAGE_MATCH_DOUBLE_INPUT_MAIN,     /* 人包关联双输入(主) */
	PEOPLE_PACKAGE_MATCH_DOUBLE_INPUT_SLAVE, 	/* 人包关联双输入(从) */
	
    DEVICE_TYPE_NUM,
} IA_DEVICE_TYPE_E;

typedef struct _IA_ALG_MODEL_CFG_
{
    UINT32 uiModelNum;

    CHAR acModelPath[IA_MAX_MODEL_NUM][SVA_MODEL_NAME_MAX_LEN];
} IA_ALG_MODEL_CFG_S;

typedef struct _IA_ALG_CFG_
{
    UINT32 uiModelW;
    UINT32 uiModelH;

    IA_ALG_MODEL_CFG_S stIaAlgModelCfg;
} IA_ALG_CFG_S;

typedef struct _IA_MEM_PRM_T_
{
    PhysAddr PhyAddr;
    char *VirAddr;

    UINT32 u32PoolId;               /* VB类型使用参数，mmz不涉及 */
	UINT64 u64VbBlk;
	INT32 s32BlkCnt;
	INT64L llBlkSize;
} IA_MEM_PRM_S;

typedef struct _IA_MEM_ALLOC_T
{
    UINT32 uiOffSet;
    UINT32 uiSize;
    IA_MEM_TYPE_E uiMemType;
    IA_MOD_IDX_E uiMemMode;
    IA_MEM_PRM_S stMemPrm;
    CHAR *cName;
    void *mutex;
} IA_MEM_INFO;

typedef struct _IA_SVA_MEM_INFO_T
{
    IA_MEM_INFO stMemInfo[IA_MEM_TYPE_NUM];
} SVA_MEM_INFO;

typedef struct _IA_BA_MEM_INFO_T
{
    IA_MEM_INFO stMemInfo[IA_MEM_TYPE_NUM];
} BA_MEM_INFO;

typedef struct _IA_FACE_MEM_INFO_T
{
    IA_MEM_INFO stMemInfo[IA_MEM_TYPE_NUM];
} FACE_MEM_INFO;

typedef struct _IA_PPM_MEM_INFO_T
{
    IA_MEM_INFO stMemInfo[IA_MEM_TYPE_NUM];
} PPM_MEM_INFO;

typedef struct tagIaOutChnResolution
{
    UINT32 uiWidth;                 /* 输出通道宽度 */
    UINT32 uiHeight;                /* 输出通道高度 */
    UINT32 uiVdecChn;               /* 解码通道号 */
    UINT32 uiVpssChn;               /* VPSS输出通道 */
    IA_MOD_IDX_E enModule;        /* 智能模块ID */
} IA_UPDATE_OUTCHN_PRM;

typedef INT32 (*CFG_CB_FUNC)(cJSON *pstItem);

/* ========================================================================== */
/*                             函数声明区                                     */
/* ========================================================================== */

/**
 * @function:   IA_RegModCfgCbFunc
 * @brief:      模块注册回调函数用于修改获取的配置文件
 * @param[in]:  IA_MOD_IDX_E enModId
 * @param[in]:  CFG_CB_FUNC pCbFunc
 * @param[out]: None
 * @return:     INT32
 */
INT32 IA_RegModCfgCbFunc(IA_MOD_IDX_E enModId, CFG_CB_FUNC pCbFunc);

/**
 * @function:   IA_InitCfgPrm
 * @brief:      读取应用层配置文件，初始化相关配置参数
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 IA_InitCfgPrm(VOID);

/**
 * @function:   IA_InitCfgPath
 * @brief:      确定初始化的默认配置文件名称
 * @param[in]:  None
 * @param[out]: None
 * @return:     static INT32
 */
INT32 IA_InitCfgPath(VOID);

/*******************************************************************************
* 函数名  : IA_GetScheHndl
* 描  述  : 返回调度器句柄
* 输  入  : 无:
* 输  出  : 无
* 返回值  : 调度句柄指针
*******************************************************************************/
void *IA_GetScheHndl(void);

/*******************************************************************************
* 函数名  : IA_GetScheHndl
* 描  述  : 返回BA调度器句柄
* 输  入  : 无:
* 输  出  : 无
* 返回值  : 调度句柄指针
*******************************************************************************/

void *IA_GetBaScheHndl(void);


/*******************************************************************************
* 函数名  : Ia_MemDeInit
* 描  述  : 算法引擎内存去初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 注意点  : 算法引擎对内存起始地址有要求，需要在3.2G以内
*******************************************************************************/
INT32 Ia_MemDeInit(void);

/*******************************************************************************
* 函数名  : Ia_MemInit
* 描  述  : 算法引擎内存初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 注意点  : 算法引擎对内存起始地址有要求，需要在3.2G以内
*******************************************************************************/
INT32 Ia_MemInit(void);

/**
 * @function:   Ia_PrintMemInfo
 * @brief:      打印模块使用的内存信息
 * @param[in]:  IA_MOD_IDX_E mode
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ia_PrintMemInfo(IA_MOD_IDX_E mode);

/*******************************************************************************
* 函数名  : Ia_ResetModMem
* 描  述  : 重置特定智能子模块的内存
* 输  入  : - mode: 内存所属智能模块
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Ia_ResetModMem(IA_MOD_IDX_E mode);

/*******************************************************************************
* 函数名  : Ia_GetXsiFreeMem
* 描  述  : 分配算法引擎内存
* 输  入  : - mode        : 内存所属智能模块
*         : - type        : 内存类型
*         : - uBufSize    : 内存块大小
*         : - pstMemBuffer: 内存指针
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Ia_GetXsiFreeMem(IA_MOD_IDX_E mode, IA_MEM_TYPE_E type, UINT32 uBufSize, IA_MEM_PRM_S *pstMemBuffer);

/**
 * @function:   IA_GetModEnableFlag
 * @brief:      获取智能模块使能标记
 * @param[in]:  IA_MOD_IDX_E enModIdx
 * @param[out]: None
 * @return:     BOOL
 */
BOOL IA_GetModEnableFlag(IA_MOD_IDX_E enModIdx);

/**
 * @function:   IA_GetModTransFlag
 * @brief:      获取智能模块是否需要跨芯片传输标记
 * @param[in]:  IA_MOD_IDX_E enModIdx
 * @param[out]: None
 * @return:     BOOL
 */
BOOL IA_GetModTransFlag(IA_MOD_IDX_E enModIdx);

/**
 * @function:   IA_GetModTransChipId
 * @brief:      获取智能模块运行芯片标识
 * @param[in]:  IA_MOD_IDX_E enModIdx
 * @param[out]: None
 * @return:     BOOL
 */
INT32 IA_GetModTransChipId(IA_MOD_IDX_E enModIdx);

/*******************************************************************************
* 函数名  : IA_GetModMemInitSize
* 描  述  : 初始化模块内存大小
* 输  入  : mod: 模块
*         : SizeArr : 初始化内存大小
* 输  出  : 无
* 返回值  : SAL_SOK : 成功
*         : SAL_FAIL: 失败
* 注意点  : 当前在初始化根据设备型号进行区分
*******************************************************************************/
UINT32 IA_GetModMemInitSize(IA_MOD_IDX_E mod, UINT32 uiSizeArr[IA_MEM_TYPE_NUM]);

/*******************************************************************************
* 函数名  : IA_UpdateOutChnResolution
* 描  述  : 更新输出通道分辨率
* 输  入  : pPrm: 输入参数
* 输  出  : 无
* 返回值  : SAL_SOK : 成功
*         : SAL_FAIL: 失败
*******************************************************************************/
INT32 IA_UpdateOutChnResolution(void *pPrm);
/**
 * @function   IA_InitHwCore
 * @brief      初始化平台层硬核资源
 * @param[in]  VOID  
 * @param[out] None
 * @return     INT32
 */
INT32 IA_InitHwCore(VOID);

/*******************************************************************************
* 函数名  : IA_InitEncrypt
* 描  述  : 解密资源初始化
* 输  入  : - pHandle: 解密句柄
* 输  出  : - pHandle: 解密句柄
* 返回值  : SAL_SOK  : 成功
*			SAL_FAIL : 失败
*******************************************************************************/
INT32 IA_InitEncrypt(void **ppHandle);

/**
 * @function   IA_DeinitEncrypt
 * @brief      去初始化解密
 * @param[in]  void *pHandle  
 * @param[out] None
 * @return     INT32
 */
INT32 IA_DeinitEncrypt(void *pHandle);

/**
 * @function:   Ia_TdeQuickCopy
 * @brief:      tde拷贝，智能模块封装
 * @param[in]:  SYSTEM_FRAME_INFO *pSrcSysFrame
 * @param[in]:  SYSTEM_FRAME_INFO *pDstSysFrame
 * @param[in]:  UINT32 x
 * @param[in]:  UINT32 y
 * @param[in]:  UINT32 w
 * @param[in]:  UINT32 h
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ia_TdeQuickCopy(SYSTEM_FRAME_INFO *pSrcSysFrame, SYSTEM_FRAME_INFO *pDstSysFrame,
                      UINT32 x, UINT32 y,
                      UINT32 w, UINT32 h, UINT32 bCached);

/**
 * @function:   Ia_TdeQuickCopyTmp
 * @brief:      tde拷贝，智能模块封装
 * @param[in]:  SYSTEM_FRAME_INFO *pSrcSysFrame
 * @param[in]:  SYSTEM_FRAME_INFO *pDstSysFrame
 * @param[in]:  UINT32 x
 * @param[in]:  UINT32 y
 * @param[in]:  UINT32 w
 * @param[in]:  UINT32 h
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ia_TdeQuickCopyTmp(SYSTEM_FRAME_INFO *pSrcSysFrame, SYSTEM_FRAME_INFO *pDstSysFrame,
                      UINT32 x, UINT32 y,
                      UINT32 uiDstX, UINT32 uiDstY, UINT32 w, UINT32 h, UINT32 bCached);

/**
 * @function:   IA_GetAlgRunFlag
 * @brief:      获取智能模块算法资源是否运行标识
 * @param[in]:  IA_MOD_IDX_E enModIdx
 * @param[out]: None
 * @return:     BOOL
 */
BOOL IA_GetAlgRunFlag(IA_MOD_IDX_E enModIdx);

/**
 * @function:   IA_PrIaInitMapInfo
 * @brief:      打印智能模块初始化映射信息
 * @param[in]:  IA_MOD_IDX_E enModIdx
 * @param[out]: None
 * @return:     VOID
 */
VOID IA_PrIaInitMapInfo(IA_MOD_IDX_E enModIdx);

VOID Ia_DumpYuvData(CHAR *pStrPath, void *pVir, UINT32 uiSize);

#endif

