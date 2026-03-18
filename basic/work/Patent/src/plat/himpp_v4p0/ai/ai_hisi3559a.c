/**
 * @file   ai_hisi3559a.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  ai模块，实现arm平台相关智能资源初始化
 * @author sunzelin
 * @date   2022/8/5
 * @note
 * @note \n History
   1.日    期: 2022/8/5
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include <sal.h>
#include <platform_sdk.h>
#include "ai_hal_inter.h"
#include "hik_apt_cnn_sched.h"
#include "hik_apt_err.h"
#include "hcnn_scheduler.h"
#include "vca_types.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
/* hisi3559a平台层调度器最多可使用npu个数 */
#define AI_DSP_HISI3559A_MAX_DSP_CORE_NUM (4)
/* hisi3559a平台层调度器最多可使用arm cpu个数 */
#define AI_DSP_HISI3559A_MAX_NNIE_NUM (2)

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
typedef struct _AI_HISI3559A_INNER_INIT_PRM_
{
	/* HISI3559A平台层支持的调度器参数 */
	HIK_APT_CNN_SCHED_PARAM stSchedInitPrm;
} AI_HISI3559A_INNER_INIT_PRM_S;

/* 单个dsp核的属性 */
typedef struct _AI_HISI3559A_DSP_CORE_ATTR_
{
	/* dsp核id */
	UINT32 u32CoreId;
} AI_HISI3559A_DSP_CORE_ATTR_S;

/* 所有dsp核的加载属性 */
typedef struct _AI_HISI3559A_DSP_BIN_LOAD_PRM_
{
	/* 加载的dsp核个数 */
	UINT32 u32LoadNum;
	/* 每个dsp核属性 */
	AI_HISI3559A_DSP_CORE_ATTR_S astDspCoreAttr[AI_DSP_HISI3559A_MAX_DSP_CORE_NUM];
} AI_HISI3559A_DSP_BIN_LOAD_PRM_S;

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/
/* 平台相关的AI硬件平台函数结构体 */
static AI_PLAT_OPS_ST g_stAiPlatOps = {0};

/* 当前H5平台用于智能调度的dsp核参数，静态属性 */
static AI_HISI3559A_DSP_BIN_LOAD_PRM_S const g_stDspCoreLoadPrm = 
{
	/* 当前H5平台分配0、1核给底层算法调度，2、3作为osd使用，故此处仅加载0、1两个核 */
	.u32LoadNum = 2,
	.astDspCoreAttr = 
	{
		{
			.u32CoreId = 0,
		},
		
		{
			.u32CoreId = 1,
		},	
	},
};

/* 行为分析智能调度器创建时的内存表 */
static VCA_MEM_TAB_V3 g_sched_memtab[HKANN_MEM_TAB_NUM] = {0};

/* 行为分析模块的调度句柄 */
static VOID *g_pSbaeSchedHandle = NULL;

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/
/**
 * @function   ai_hisi3559a_ReloadDspBin
 * @brief      加载dspbin
 * @param[in]  VOID  
 * @param[out] None
 * @return     static INT32
 */
static INT32 ai_hisi3559a_ReloadDspBin(VOID)
{
	INT32 s32Ret = SAL_FAIL;
	
	UINT32 u32Idx;

	for (u32Idx = 0; u32Idx < g_stDspCoreLoadPrm.u32LoadNum; ++u32Idx)
	{
		s32Ret = svpdsp_hal_unloadDspBin(u32Idx);
		if (SAL_SOK != s32Ret)
		{
			SAL_LOGE("unload dspbin [%d] failed! ret: 0x%x \n", u32Idx, s32Ret);
			return SAL_FAIL;
		}
	}

	for (u32Idx = 0; u32Idx < g_stDspCoreLoadPrm.u32LoadNum; ++u32Idx)
	{
		s32Ret = svpdsp_hal_loadDspBin(u32Idx);
		if (SAL_SOK != s32Ret)
		{
			SAL_LOGE("load dspbin [%d] failed! ret: 0x%x \n", u32Idx, s32Ret);
			return SAL_FAIL;
		}
	}
	
	return SAL_SOK;
}

/**
 * @function   ai_hisi3559a_Outer2Inner
 * @brief      将hal层参数转换为h5平台层内部参数
 * @param[in]  IN const AI_HAL_INIT_PRM_S *pstOuterInitPrm      
 * @param[in]  OUT AI_HISI3559A_INNER_INIT_PRM_S *pstInnerInitPrm  
 * @param[out] None
 * @return     static INT32
 */
static INT32 ai_hisi3559a_Outer2Inner(IN const AI_HAL_INIT_PRM_S *pstOuterInitPrm,
                                      OUT AI_HISI3559A_INNER_INIT_PRM_S *pstInnerInitPrm)
{
	UINT32 i, u32Idx;

	u32Idx = 0;
	
	/* dsp核分配策略: 0、1初始化作为底层算法使用，2、3核用于业务层画osd使用。若人包使能，0核不参与算法调度需要深度图处理单元 */
	for (i = 0; i < g_stDspCoreLoadPrm.u32LoadNum; ++i)
	{
		if (0 != g_stDspCoreLoadPrm.astDspCoreAttr[i].u32CoreId
			|| SAL_TRUE != pstOuterInitPrm->stOthersInitPrm.bPpmUsed)
		{
            pstInnerInitPrm->stSchedInitPrm.cnn_dev[u32Idx++] = HCNN_DEV_ID_DSP0 + i;
		}
	}

	/* nnie分配 */
	for (i = 0; i < AI_DSP_HISI3559A_MAX_NNIE_NUM; ++i)
	{
		pstInnerInitPrm->stSchedInitPrm.cnn_dev[u32Idx++] = HCNN_DEV_ID_NNIE0 + i;
	}

	/* 保存初始化的dev个数 */
	pstInnerInitPrm->stSchedInitPrm.cnn_dev_num = u32Idx;
	
	return SAL_SOK;
}

/**
 * @function   ai_hisi3559a_AlignedAlloc
 * @brief      内存对齐函数接口
 * @param[in]  size_t alignment   
 * @param[in]  size_t mallocSize  
 * @param[in]  void **pstMem      
 * @param[out] None
 * @return     INT32
 */
static INT32 ai_hisi3559a_AlignedAlloc(size_t alignment, size_t mallocSize, void **pstMem)
{
    size_t maxAlign = VCA_MAX(alignment, 2 * sizeof(void *));
    size_t alignSize = maxAlign + mallocSize;
    void *rawMallocPtr = (void *)malloc(alignSize);

    *pstMem = NULL;
    if (NULL == rawMallocPtr)
    {
        SAL_ERROR("malloc ARM_MEM failed\n");
        return HIK_APT_S_FAIL;
    }

    /* 清空分配内存 */
    memset(rawMallocPtr, 0, alignSize);
    *pstMem = (void *)((((size_t)rawMallocPtr + (maxAlign - 1)) / maxAlign) * maxAlign);
    *pstMem = *pstMem == rawMallocPtr ? (void *)((size_t)rawMallocPtr + maxAlign) : *pstMem;
    *(void **)((size_t)*pstMem - sizeof(void *)) = rawMallocPtr;

    return HIK_APT_S_OK;
}

/**
 * @function   ai_hisi3559a_AllocMemtab
 * @brief      内存申请接口
 * @param[in]  VCA_MEM_TAB_V3 *mem_tab  
 * @param[out] None
 * @return     INT32
 */
static INT32 ai_hisi3559a_AllocMemtab(VCA_MEM_TAB_V3 *mem_tab)
{
    size_t align_size = 0;            /* 对齐后需要的内存大小 */
    void *raw_malloc_ptr = NULL;             /* malloc开辟空间原始首地址 */
    VCA_MEM_SPACE space = mem_tab->space;
    size_t max_align = VCA_MAX(mem_tab->alignment, 2 * sizeof(void *));

    size_t align_off = 0;             /* 对齐非对齐地址偏移量 */
    int32_t hisi_ret = HIK_APT_S_OK;
    PhysAddr u64PhyAddr = 0;
    void *ppVirAddr = NULL;

    if (mem_tab->size == 0)
    {
        return HIK_APT_S_OK;
    }

    if (mem_tab->alignment > 1)  /* 需要对齐 */
    {
        align_size = mem_tab->size + max_align;
    }
    else
    {
        SAL_ERROR("Err %d", mem_tab->alignment);
        return HIK_APT_S_FAIL;   /* 不需要对齐 */
    }

    switch (space)
    {
        case VCA_HCNN_MEM_MMZ_NO_CACHE: /* mmz非缓存内存区域 */
        {
            hisi_ret = mem_hal_mmzAlloc(align_size,
			                      "ia_common",
			                      "mmz_no_cache",
			                      NULL,
			                      SAL_FALSE,
			                      &u64PhyAddr,
			                      (VOID **)&ppVirAddr);
            if (SAL_SOK != hisi_ret)
            {
                SAL_ERROR("error: alloc_memory  HCNN_NO_CACHE_MEM failed\n");
                return HIK_APT_S_FAIL;
            }

            /* 清空分配内存 */
			sal_memset_s((void *)ppVirAddr, align_size, 0, align_size);


            /* 虚拟地址对齐 */
            mem_tab->base = (void *)(((size_t)ppVirAddr + max_align) & (~((size_t)max_align - 1)));

            mem_tab->base = (mem_tab->base == (void *)ppVirAddr) ? (mem_tab->base + max_align) : mem_tab->base;

            /* 记录需要释放的虚拟地址 */
            ((uintptr_t *)mem_tab->base)[-1] = (uintptr_t)ppVirAddr;

            /* 记录需要释放的物理地址 */
            ((uintptr_t *)mem_tab->base)[-2] = (uintptr_t)u64PhyAddr;

            /* 对齐后物理地址 */
            align_off = (size_t)mem_tab->base - (size_t)ppVirAddr;
            mem_tab->phy_base = (void *)((uintptr_t)u64PhyAddr + align_off);

            return HIK_APT_S_OK;
        }
        case VCA_HCNN_MEM_MMZ_NO_CACHE_PRIORITY: /* mmz非缓存内存区域 */
        {
            hisi_ret = mem_hal_mmzAlloc(align_size,
                      "ia_common",
                      "mmz_no_cache",
                      NULL,
                      SAL_FALSE,
                      &u64PhyAddr,
                      (VOID **)&ppVirAddr);
            if (SAL_SOK != hisi_ret)
            {
                SAL_ERROR("error: alloc_memory  HCNN_NO_CACHE_MEM failed\n");
                return HIK_APT_S_FAIL;
            }

            /* 清空分配内存 */
			sal_memset_s((void *)ppVirAddr, align_size, 0, align_size);

            /* 虚拟地址对齐 */
            mem_tab->base = (void *)(((size_t)ppVirAddr + max_align) & (~((size_t)max_align - 1)));

            mem_tab->base = (mem_tab->base == (void *)ppVirAddr) ? (mem_tab->base + max_align) : mem_tab->base;

            /* 记录需要释放的虚拟地址 */
            ((uintptr_t *)mem_tab->base)[-1] = (uintptr_t)ppVirAddr;

            /* 记录需要释放的物理地址 */
            ((uintptr_t *)mem_tab->base)[-2] = (uintptr_t)u64PhyAddr;

            /* 对齐后物理地址 */
            align_off = (size_t)mem_tab->base - (size_t)ppVirAddr;
            mem_tab->phy_base = (void *)((uintptr_t)u64PhyAddr + align_off);

            return HIK_APT_S_OK;
        }
        /* case VCA_HCNN_MEM_MALLOC: */
        case VCA_HCNN_MEM_MMZ_WITH_CACHE_PRIORITY: /* mmz缓存内存区域 */
        {
            hisi_ret = mem_hal_mmzAlloc(align_size,
                      "ia_common",
                      "mmz_with_cache",
                      NULL,
                      SAL_TRUE,
                      &u64PhyAddr,
                      (VOID **)&ppVirAddr);
            if (SAL_SOK != hisi_ret)
            {
                SAL_ERROR("alloc_memory  HCNN_CACHED_MEM failed\n");
                return HIK_APT_S_FAIL;
            }

            /* 清空分配内存 */
			sal_memset_s((void *)ppVirAddr, align_size, 0, align_size);

            /* 虚拟地址对齐 */
            mem_tab->base = (void *)(((size_t)ppVirAddr + max_align) & (~((size_t)max_align - 1)));

            mem_tab->base = (mem_tab->base == (void *)ppVirAddr) ? (mem_tab->base + max_align) : mem_tab->base;

            /* 记录需要释放的虚拟地址 */
            ((uintptr_t *)mem_tab->base)[-1] = (uintptr_t)ppVirAddr;

            /* 记录需要释放的物理地址 */
            ((uintptr_t *)mem_tab->base)[-2] = (uintptr_t)u64PhyAddr;

            /* 对齐后物理地址 */
            align_off = (size_t)mem_tab->base - (size_t)ppVirAddr;
            mem_tab->phy_base = (void *)((uintptr_t)u64PhyAddr + align_off);

            return HIK_APT_S_OK;
        }

        case VCA_HCNN_MEM_MMZ_WITH_CACHE: /* mmz缓存内存区域 */
        {
            hisi_ret = mem_hal_mmzAlloc(align_size,
                      "ia_common",
                      "mmz_with_cache",
                      NULL,
                      SAL_TRUE,
                      &u64PhyAddr,
                      (VOID **)&ppVirAddr);

            if (SAL_SOK != hisi_ret)
            {
                SAL_ERROR("alloc_memory  HCNN_CACHED_MEM failed\n");
                return HIK_APT_S_FAIL;
            }

            /* 清空分配内存 */
			sal_memset_s((void *)ppVirAddr, align_size, 0, align_size);

            /* 虚拟地址对齐 */
            mem_tab->base = (void *)(((size_t)ppVirAddr + max_align) & (~((size_t)max_align - 1)));

            mem_tab->base = (mem_tab->base == (void *)ppVirAddr) ? (mem_tab->base + max_align) : mem_tab->base;

            /* 记录需要释放的虚拟地址 */
            ((uintptr_t *)mem_tab->base)[-1] = (uintptr_t)ppVirAddr;

            /* 记录需要释放的物理地址 */
            ((uintptr_t *)mem_tab->base)[-2] = (uintptr_t)u64PhyAddr;

            /* 对齐后物理地址 */
            align_off = (size_t)mem_tab->base - (size_t)ppVirAddr;
            mem_tab->phy_base = (void *)((uintptr_t)u64PhyAddr + align_off);

            return HIK_APT_S_OK;
        }

        case VCA_MEM_EXTERNAL_UNCACHED_DATA:
        case VCA_MEM_EXTERNAL_CACHED_DATA:
        case VCA_MEM_EXTERNAL_PROG:
        case VCA_HCNN_MEM_MALLOC:
        case VCA_CNN_MEM_CPU:
        case VCA_CNN_MEM_NT:
        case VCA_MEM_INTERNAL_DATA:
        {
            hisi_ret = ai_hisi3559a_AlignedAlloc(mem_tab->alignment, align_size, (void *)raw_malloc_ptr);
            if (SAL_SOK != hisi_ret)
            {
                SAL_ERROR("alloc_memory aligned failed\n");
                return HIK_APT_S_FAIL;
            }

            /* 清空分配内存 */
			sal_memset_s(raw_malloc_ptr, align_size, 0, align_size);
            mem_tab->base = raw_malloc_ptr;

            return HIK_APT_S_OK;
        }
        default:
        {
            SAL_ERROR("%d\n", mem_tab->space);
            return HIK_APT_S_FAIL;
        }
    }

    return HIK_APT_S_OK;
}

/**
 * @function   ai_hisi3559a_AllocMemtabArray
 * @brief      内存Alloc
 * @param[in]  VCA_MEM_TAB_V3 *mem_tab  
 * @param[in]  int num                  
 * @param[out] None
 * @return     INT32
 */
static INT32 ai_hisi3559a_AllocMemtabArray(VCA_MEM_TAB_V3 *mem_tab, int num)
{
    UINT32 ret = HIK_APT_S_OK, i;
    VCA_MEM_SPACE tmp_space;

    for (i = 0; i < num; i++)
    {
        if (mem_tab[i].size == 0)
        {
            continue;
        }

        tmp_space = mem_tab[i].space;
        mem_tab[i].space = VCA_HCNN_MEM_MMZ_WITH_CACHE;
        ret = ai_hisi3559a_AllocMemtab(&mem_tab[i]);
        mem_tab[i].space = tmp_space;

        if (HIK_APT_S_OK != ret)
        {
            SAL_ERROR("Alloc Mem Failed! ret 0x%x\n", ret);
            return HIK_APT_S_FAIL;
        }
    }

    return ret;
}

/**
 * @function   ai_hisi3559a_InitHwCore
 * @brief      初始化hisi3559a硬核资源
 * @param[in]  IN AI_HISI3559A_INNER_INIT_PRM_S *pstInnerInitPrm  
 * @param[in]  OUT VOID **ppHwScheHandle                             
 * @param[out] None
 * @return     static INT32
 */
static INT32 ai_hisi3559a_InitHwCore(IN AI_HISI3559A_INNER_INIT_PRM_S *pstInnerInitPrm, 
	                                 OUT VOID **ppHwScheHandle)  /* TODO: 增加入参，区分不同产品的NN硬核资源分配 */
{
	INT32 s32Ret = SAL_FAIL;

	SAL_LOGI("========hisi3559a, init hw core! \n");
	for (int i = 0; i < pstInnerInitPrm->stSchedInitPrm.cnn_dev_num; i++)
	{
		SAL_LOGI("i %d, dev %d \n", i, pstInnerInitPrm->stSchedInitPrm.cnn_dev[i]);
	}

	s32Ret = HIK_APT_CNN_SCHED_GetMemSize(&pstInnerInitPrm->stSchedInitPrm, g_sched_memtab);
	if (HIK_APT_S_OK != s32Ret)
	{
		printf("GetMemSize fail ret=0x%x\n", s32Ret);
		return s32Ret;
	}
	
	s32Ret = ai_hisi3559a_AllocMemtabArray(g_sched_memtab, HKANN_MEM_TAB_NUM);
	if (HIK_APT_S_OK != s32Ret)
	{
		printf("MEM ALLOC fail ret=0x%x\n", s32Ret);
		return s32Ret;
	}
	
	s32Ret = HIK_APT_CNN_SCHED_Create(&pstInnerInitPrm->stSchedInitPrm, g_sched_memtab, &g_pSbaeSchedHandle);
	if (HIK_APT_S_OK != s32Ret)
	{
		printf("CNN SCHED fail ret=0x%x\n", s32Ret);
		return s32Ret;
	}
	/*获取调度器(xsi&Face公用)*/
	s32Ret = HIK_APT_CNN_SCHED_GetOriginScheduler(g_pSbaeSchedHandle, HIK_APT_CNN_HISI, ppHwScheHandle);
	if (HIK_APT_S_OK != s32Ret)
	{
		SAL_ERROR("CNN Get Originscheduler fail ret=0x%x\n", s32Ret);
		return s32Ret;
	}
	
	SAL_LOGI("hisi3559a Hw core init success!!!!\n");
	return SAL_SOK;
}

/**
 * @function   ai_hisi3559a_Init
 * @brief      初始化接口
 * @param[in]  const AI_HAL_INIT_PRM_S *pstInitPrm  
 * @param[in]  VOID **ppSchedHandle                 
 * @param[out] None
 * @return     INT32
 */
static INT32 ai_hisi3559a_Init(AI_HAL_INIT_PRM_S *pstInitPrm, VOID **ppSchedHandle)   /* TODO: 增加入参，区分不同产品的NN硬核资源分配 */
{
	INT32 s32Ret = SAL_FAIL;

	AI_HISI3559A_INNER_INIT_PRM_S stInnerSchedInitPrm = {0};

	/* checker */
	if (NULL == pstInitPrm || NULL == ppSchedHandle)
	{
		SAL_LOGE("ptr null! %p, %p \n", pstInitPrm, ppSchedHandle);
		return SAL_FAIL;
	}

	/* 重新加载dspbin */
	s32Ret = ai_hisi3559a_ReloadDspBin();
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("ai_rk3588_Outer2Inner failed! \n");
		return SAL_FAIL;
	}
	
	/* 将外部调度器初始化参数，转换为平台层内部参数 */
	s32Ret = ai_hisi3559a_Outer2Inner(pstInitPrm, &stInnerSchedInitPrm);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("ai_rk3588_Outer2Inner failed! \n");
		return SAL_FAIL;
	}

	/* 初始化硬核调度资源 */
	s32Ret = ai_hisi3559a_InitHwCore(&stInnerSchedInitPrm, ppSchedHandle);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("ai_rk3588_InitHwCore failed! \n");
		return SAL_FAIL;
	}
	
    return SAL_SOK;
}

/**
 * @function   ai_hisi3559a_Deinit
 * @brief      去初始化接口
 * @param[in]  VOID  
 * @param[out] None
 * @return     INT32
 */
static INT32 ai_hisi3559a_Deinit(VOID)
{
	/* 保留 */
    return SAL_SOK;
}

/**
 * @function   ai_hisi3559a_GetSbaeSchedHdl
 * @brief      获取行为分析调度句柄
 * @param[in]  VOID  
 * @param[out] None
 * @return     VOID *
 */
static VOID *ai_hisi3559a_GetSbaeSchedHdl(VOID)
{
	return g_pSbaeSchedHandle;
}

/**
 * @function   ai_hal_register
 * @brief      平台相关的AI 接口注册
 * @param[in]  void  
 * @param[out] None
 * @return     AI_PLAT_OPS_ST *
 */
AI_PLAT_OPS_ST *ai_hal_register(void)
{
    g_stAiPlatOps.pAiInit = ai_hisi3559a_Init;
    g_stAiPlatOps.pAiDeInit = ai_hisi3559a_Deinit;
    g_stAiPlatOps.pAiGetSbaeSchedHdl = ai_hisi3559a_GetSbaeSchedHdl;
	
    return &g_stAiPlatOps;
}

