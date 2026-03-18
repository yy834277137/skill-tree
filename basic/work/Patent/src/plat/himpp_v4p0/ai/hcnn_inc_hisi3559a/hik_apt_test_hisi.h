/******************************************************************************
* 
* 版权信息：版权所有 (c) 201X, 杭州海康威视数字技术有限公司, 保留所有权利
* 
* 文件名称：hik_apt_test_hisi.h
* 文件标示：__HIK_APT_TEST_HISI_H__
* 摘    要：hisi的alloc/free函数定义
* 
* 当前版本：1.0
* 作    者：秦川6
* 日    期：2020-12-22
* 备    注：
******************************************************************************/
#ifndef __HIK_APT_TEST_HISI_H__
#define __HIK_APT_TEST_HISI_H__
//海思平台内存分配使用mpi_sys.h中的API
#ifdef HISI_PLT
#include "mpi_sys.h"
#include "mpi_dsp.h"
#include "hi_dsp.h"
#include "hcnn_scheduler.h"



/***********************************************************************************************************************
* 功  能: 释放单个HCNN_MEM_TAB内存
* 参  数:
*   mem_tab   -I/O   记录单个内存块的所有信息
* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/
static int hik_apt_test_free_memtab(VCA_MEM_TAB_V3 *mem_tab)
{
    VCA_MEM_SPACE    space = mem_tab->space;
    int32_t          hisi_ret = 0;

    if (mem_tab->plat == VCA_MEM_PLAT_GPU)
    {
        space = VCA_CNN_MEM_GPU;
    }

    if (mem_tab->alignment == 0)
    {
        return -1;
    }

    //根据内存块的所处位置来选择释放操作
    switch (space)
    {
    //mmz管理的cache和非cache区域使用相同的函数进行释放
    case VCA_HCNN_MEM_MMZ_WITH_CACHE_PRIORITY:
    case VCA_HCNN_MEM_MMZ_NO_CACHE_PRIORITY:
    case VCA_HCNN_MEM_MMZ_WITH_CACHE:
    case VCA_HCNN_MEM_MMZ_NO_CACHE:
    {
        HIK_APT_ERR("mem_tab->space = %d\n", mem_tab->space);
        if (mem_tab->phy_base)
        {
            if (mem_tab->alignment > 1)   //释放对齐的内存块
            {
                hisi_ret = HI_MPI_SYS_MmzFree((HI_U64)(((uintptr_t *)mem_tab->base)[-2]),
                        (HI_VOID *)((uintptr_t *)mem_tab->base)[-1]);
            }
            if (hisi_ret != HI_SUCCESS)
            {
                HIK_APT_ERR("-----------MmzFree FAIL!-----------\n");
            }

            mem_tab->phy_base = NULL;
            mem_tab->base   = NULL;
            HIK_APT_ERR("-----------MmzFree OK!-----------\n");

            return HIK_APT_S_OK;
        }

        return -2;
    }
        //释放malloc出来的内存块
    case VCA_MEM_EXTERNAL_PROG:
    case VCA_MEM_EXTERNAL_UNCACHED_DATA:
    case VCA_MEM_EXTERNAL_CACHED_DATA:
    case VCA_HCNN_MEM_MALLOC:
    case VCA_CNN_MEM_CPU:
    {
        if (mem_tab->base)
        {
            _aligned_free(mem_tab->base);
        }
        mem_tab->base = NULL;

        return HIK_APT_S_OK;
    }
    default:
    {
        HIK_APT_DUMP_VAL(space, "%d");
        return -3;
    }
    }
}



/***********************************************************************************************************************
* 功  能: 分配单个HCNN_MEM_TAB内存
* 参  数:
*   mem_tab   -I/O   记录单个内存块的所有信息
* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/
static int hik_apt_test_alloc_memtab(VCA_MEM_TAB_V3 *mem_tab)
{
    size_t		     align_size = 0;  //对齐后需要的内存大小
    void		    *raw_malloc_ptr = NULL;   //malloc开辟空间原始首地址
    VCA_MEM_SPACE    space = mem_tab->space;
    size_t max_align = VCA_MAX(mem_tab->alignment, 2 * sizeof(void *));

    size_t		     align_off = 0;   //对齐非对齐地址偏移量
    int32_t			 hisi_ret   = HIK_APT_S_OK;
    uint64_t		 u64PhyAddr = 0;
    void			*ppVirAddr  = NULL;
	if (mem_tab->size == 0)
	{
		return HIK_APT_S_OK;
	}

    if (mem_tab->alignment > 1)  //需要对齐
    {
        align_size = mem_tab->size + max_align;
    }
    else
    {
        HIK_APT_DUMP_VAL(mem_tab->alignment, "%d");
        return -1;   //不需要对齐
    }

    switch (space)
    {
#if 1
    case VCA_HCNN_MEM_MMZ_NO_CACHE:  //mmz非缓存内存区域
    {
        hisi_ret = HI_MPI_SYS_MmzAlloc((HI_U64 *)&u64PhyAddr, (HI_VOID **)&ppVirAddr, (const HI_CHAR *)"mmz_no_cache", NULL, align_size);
        if (HI_SUCCESS != hisi_ret)
        {
            HIK_APT_ERR("error: alloc_memory  HCNN_NO_CACHE_MEM failed\n");
            HIK_APT_CHECK_ERR(NULL == raw_malloc_ptr, HIK_APT_MEM_LACK_OF_MEM);
        }

        //清空分配内存
        memset((void *)ppVirAddr, 0, align_size);


        //虚拟地址对齐
        mem_tab->base = (void *)(((size_t)ppVirAddr + max_align) & (~((size_t)max_align - 1)));

        mem_tab->base = (mem_tab->base == (void *)ppVirAddr) ? (mem_tab->base + max_align) : mem_tab->base;

        //记录需要释放的虚拟地址
        ((uintptr_t *)mem_tab->base)[-1] = (uintptr_t)ppVirAddr;

        //记录需要释放的物理地址
        ((uintptr_t *)mem_tab->base)[-2] = (uintptr_t)u64PhyAddr;

            //对齐后物理地址
            align_off = (size_t)mem_tab->base - (size_t)ppVirAddr;
            mem_tab->phy_base = (void *)((uintptr_t)u64PhyAddr + align_off);

        return HIK_APT_S_OK;
    }
    case VCA_HCNN_MEM_MMZ_NO_CACHE_PRIORITY:  //mmz非缓存内存区域
    {
        hisi_ret = HI_MPI_SYS_MmzAlloc((HI_U64 *)&u64PhyAddr,
                                       (HI_VOID **)&ppVirAddr,
                                       (const HI_CHAR *)"mmz_no_cache",
                                       NULL,
                                       align_size);
        if (HI_SUCCESS != hisi_ret)
        {
            HIK_APT_ERR("error: alloc_memory  HCNN_NO_CACHE_MEM failed\n");
            HIK_APT_CHECK_ERR(NULL == raw_malloc_ptr, HIK_APT_MEM_LACK_OF_MEM);
        }

        //清空分配内存
        memset((void *)ppVirAddr, 0, align_size);

        //虚拟地址对齐
        mem_tab->base = (void *)(((size_t)ppVirAddr + max_align) & (~((size_t)max_align - 1)));

        mem_tab->base = (mem_tab->base == (void *)ppVirAddr) ? (mem_tab->base + max_align) : mem_tab->base;

        //记录需要释放的虚拟地址
        ((uintptr_t *)mem_tab->base)[-1] = (uintptr_t)ppVirAddr;

        //记录需要释放的物理地址
        ((uintptr_t *)mem_tab->base)[-2] = (uintptr_t)u64PhyAddr;

        //对齐后物理地址
        align_off = (size_t)mem_tab->base - (size_t)ppVirAddr;
        mem_tab->phy_base = (void *)((uintptr_t)u64PhyAddr + align_off);

        return HIK_APT_S_OK;
    }
        //case VCA_HCNN_MEM_MALLOC:
    case VCA_HCNN_MEM_MMZ_WITH_CACHE_PRIORITY: //mmz缓存内存区域
    {
        hisi_ret = HI_MPI_SYS_MmzAlloc_Cached((HI_U64 *)&u64PhyAddr, (HI_VOID **)&ppVirAddr, (const HI_CHAR *)"mmz_with_cache", NULL, align_size);
        if (HI_SUCCESS != hisi_ret)
        {
            HIK_APT_ERR("alloc_memory  HCNN_CACHED_MEM failed\n");
            HIK_APT_CHECK_ERR(NULL == raw_malloc_ptr, HIK_APT_MEM_LACK_OF_MEM);
        }

        //清空分配内存
        memset((void *)ppVirAddr, 0, align_size);

        //虚拟地址对齐
        mem_tab->base = (void *)(((size_t)ppVirAddr + max_align) & (~((size_t)max_align - 1)));

        mem_tab->base = (mem_tab->base == (void *)ppVirAddr) ? (mem_tab->base + max_align) : mem_tab->base;

        //记录需要释放的虚拟地址
        ((uintptr_t *)mem_tab->base)[-1] = (uintptr_t)ppVirAddr;

        //记录需要释放的物理地址
        ((uintptr_t *)mem_tab->base)[-2] = (uintptr_t)u64PhyAddr;

        //对齐后物理地址
        align_off = (size_t)mem_tab->base - (size_t)ppVirAddr;
        mem_tab->phy_base = (void *)((uintptr_t)u64PhyAddr + align_off);

        return HIK_APT_S_OK;
    }

    case VCA_HCNN_MEM_MMZ_WITH_CACHE: //mmz缓存内存区域
    {
        hisi_ret = HI_MPI_SYS_MmzAlloc_Cached((HI_U64 *)&u64PhyAddr, (HI_VOID **)&ppVirAddr, (const HI_CHAR *)"mmz_with_cache", NULL, align_size);
        if (HI_SUCCESS != hisi_ret)
        {
            HIK_APT_ERR("alloc_memory  HCNN_CACHED_MEM failed\n");
            HIK_APT_CHECK_ERR(NULL == raw_malloc_ptr, HIK_APT_MEM_LACK_OF_MEM);
        }

        //清空分配内存
        memset((void *)ppVirAddr, 0, align_size);

        //虚拟地址对齐
        mem_tab->base = (void *)(((size_t)ppVirAddr + max_align) & (~((size_t)max_align - 1)));

        mem_tab->base = (mem_tab->base == (void *)ppVirAddr) ? (mem_tab->base + max_align) : mem_tab->base;

        //记录需要释放的虚拟地址
        ((uintptr_t *)mem_tab->base)[-1] = (uintptr_t)ppVirAddr;

        //记录需要释放的物理地址
        ((uintptr_t *)mem_tab->base)[-2] = (uintptr_t)u64PhyAddr;

        //对齐后物理地址
        align_off = (size_t)mem_tab->base - (size_t)ppVirAddr;
        mem_tab->phy_base = (void *)((uintptr_t)u64PhyAddr + align_off);

        return HIK_APT_S_OK;
    }
#endif

    case VCA_MEM_EXTERNAL_UNCACHED_DATA:
    case VCA_MEM_EXTERNAL_CACHED_DATA:
    case VCA_MEM_EXTERNAL_PROG:
    case VCA_HCNN_MEM_MALLOC:
    case VCA_CNN_MEM_CPU:
    case VCA_CNN_MEM_NT:
    case VCA_MEM_INTERNAL_DATA:
    {
        raw_malloc_ptr = (void *)_aligned_malloc(align_size, mem_tab->alignment);
        if (NULL == raw_malloc_ptr)
        {
            HIK_APT_ERR("alloc_memory ARM_MEM failed\n");
            HIK_APT_CHECK_ERR(NULL == raw_malloc_ptr, HIK_APT_MEM_LACK_OF_MEM);
        }

        //清空分配内存
        memset(raw_malloc_ptr, 0, align_size);
		mem_tab->base = raw_malloc_ptr;

        return HIK_APT_S_OK;
    }
    default:
    {
        HIK_APT_DUMP_VAL(mem_tab->space, "%d");
        HIK_APT_CHECK_ERR_FMT(1, -5, "Not support space type %d!\r\n", space);
    }
    }

    return -5;
}







static HI_S32 hik_apt_test_load_vp6_binary(SVP_DSP_ID_E core_id)
{
    HI_S32 hisi_ret = 0;

#if (defined HCNN_PLT_HI3559A)
#define HIK_APT_DSP_BIN_PATH "../../lib/common_lib/hi3559a/hisi_bin"
    HI_CHAR *aszBin[4][4] = {
        {HIK_APT_DSP_BIN_PATH "/dsp0/hi_sram.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_iram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_dram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_dram1.bin"},
        {HIK_APT_DSP_BIN_PATH "/dsp1/hi_sram.bin",
         HIK_APT_DSP_BIN_PATH "/dsp1/hi_iram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp1/hi_dram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp1/hi_dram1.bin"},
        {HIK_APT_DSP_BIN_PATH "/dsp2/hi_sram.bin",
         HIK_APT_DSP_BIN_PATH "/dsp2/hi_iram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp2/hi_dram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp2/hi_dram1.bin"},
        {HIK_APT_DSP_BIN_PATH "/dsp3/hi_sram.bin",
         HIK_APT_DSP_BIN_PATH "/dsp3/hi_iram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp3/hi_dram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp3/hi_dram1.bin"}};
#elif (defined HCNN_PLT_HI3519A)
#define HIK_APT_DSP_BIN_PATH "../../lib/common_lib/hi3519a/hisi_bin"
    HI_CHAR *aszBin[4][4] = {
        {HIK_APT_DSP_BIN_PATH "/dsp0/hi_sram.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_iram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_dram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_dram1.bin"},
        {HIK_APT_DSP_BIN_PATH "/dsp0/hi_sram.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_iram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_dram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_dram1.bin"},
        {HIK_APT_DSP_BIN_PATH "/dsp0/hi_sram.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_iram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_dram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_dram1.bin"},
        {HIK_APT_DSP_BIN_PATH "/dsp0/hi_sram.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_iram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_dram0.bin",
         HIK_APT_DSP_BIN_PATH "/dsp0/hi_dram1.bin"}};
#endif

#if (defined HCNN_PLT_HI3559A) || (defined HCNN_PLT_HI3519A)
    hisi_ret = HI_MPI_SVP_DSP_PowerOn(core_id);
    HIK_APT_CHECK_ERR(0 != hisi_ret, hisi_ret);

    hisi_ret = HI_MPI_SVP_DSP_LoadBin(aszBin[core_id][1], core_id * 4 + 1);
    HIK_APT_CHECK_ERR(0 != hisi_ret, hisi_ret);

    hisi_ret = HI_MPI_SVP_DSP_LoadBin(aszBin[core_id][0], core_id * 4 + 0);
    HIK_APT_CHECK_ERR(0 != hisi_ret, hisi_ret);

    hisi_ret = HI_MPI_SVP_DSP_LoadBin(aszBin[core_id][2], core_id * 4 + 2);
    HIK_APT_CHECK_ERR(0 != hisi_ret, hisi_ret);

    hisi_ret = HI_MPI_SVP_DSP_LoadBin(aszBin[core_id][3], core_id * 4 + 3);
    HIK_APT_CHECK_ERR(0 != hisi_ret, hisi_ret);

    hisi_ret = HI_MPI_SVP_DSP_EnableCore(core_id);
    HIK_APT_CHECK_ERR(0 != hisi_ret, hisi_ret);
#endif
    return hisi_ret;
}


/***********************************************************************************************************************
* 功  能: demo中用于硬件及调度器初始化的代码
* 参  数:
*       free_mem_tab          -I/O 空闲的mem_tab,用于内存集中释放
*       free_mem_tab_num      -I/O 空闲mem_tab的数量free_mem_tab_num
*       p_schedular_hdl       -O   调度器句柄，在PC上则是cuda句柄
* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/
static int hik_apt_test_init_schedular(VCA_MEM_TAB_V3 **free_mem_tab,
                                       int *free_mem_tab_num,
                                       void **p_schedular_hdl)
{
    int res;
    HCNN_SCHEDULER_MGR_PARAM_T scheduler_param;
    //系统去初始化
    res = HI_MPI_SYS_Exit();
    HIK_APT_CHECK_ERR(res != HI_SUCCESS, res);

    //Step0: 系统初始化
    res = HI_MPI_SYS_Init();
    HIK_APT_CHECK_ERR(res != HI_SUCCESS, res);

    res = hik_apt_test_load_vp6_binary(SVP_DSP_ID_0);
    HIK_APT_CHECK_ERR(res != HI_SUCCESS, res);

    scheduler_param.dev_num = 2;
    scheduler_param.dev_id[0] = HCNN_DEV_ID_DSP0;
    scheduler_param.dev_id[1] = HCNN_DEV_ID_NNIE0;
    res = HCNN_CreateScheduler(&scheduler_param, p_schedular_hdl);
    HIK_APT_CHECK_ERR(res != HCNN_SCHEDULER_ERR_RET_OK, res);
    return HIK_APT_S_OK;
}



static int hik_apt_test_uninit()
{

    return HIK_APT_S_OK;
}

#endif
#endif