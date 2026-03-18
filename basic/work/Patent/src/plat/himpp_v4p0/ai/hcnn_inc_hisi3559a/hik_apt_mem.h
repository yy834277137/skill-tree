/***************************************************************************************************
* 版权信息：版权所有(c) , 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称: hik_apt_mem.h
* 文件标识: HIK_APT_MEM_H
* 摘    要: 跨平台通用Memsize计算
*
* 当前版本: 0.1.0
* 作    者: 崔莫磊
* 日    期: 2019-06-18
* 备    注: 新建
* 
***************************************************************************************************/
#ifndef HIK_APT_MEM_H
#define HIK_APT_MEM_H
#include <stdint.h>
#include <string.h>
#include "vca_base.h"
#include "vca_types.h"
#include "hik_apt_dbg.h"
#include "hik_apt_err.h"

#if 0
#define DISABLE_HIK_APT_MEM_SRATCH
#endif

//将数组tab_from中的内存合并至数组tab_to中
//要求tab_from中存在tab_to中对应space, attrs的mem_tab
//并且非可复用内存中，tab_to中的aligment大于数组tab_from
static int32_t hik_apt_mem_merge(VCA_MEM_TAB_V3 *tab_to, VCA_MEM_TAB_V3 *tab_from)
{

    if(tab_from->size == 0)
    {
        return HIK_APT_S_OK;
    }

    if(tab_from->space == tab_to->space
            && tab_from->attrs == tab_to->attrs
            && tab_from->alignment <= tab_to->alignment
			&& tab_from->plat == tab_to->plat)
    {
#ifndef DISABLE_HIK_APT_MEM_SRATCH
        if (tab_from->attrs == VCA_MEM_SCRATCH)
        {
            tab_to->size = VCA_MAX(tab_from->size, tab_to->size);
        }
        else
#endif
        {
            //非复用内存的空间，待分配的需要与被合并的使用相同的aligment（被合并的大于待分配的）
            tab_to->size =  VCA_SIZE_ALIGN(tab_to->size, tab_to->alignment)
                            + VCA_SIZE_ALIGN(tab_from->size, tab_to->alignment);
        }
        return HIK_APT_S_OK;
    }
    return HIK_APT_S_FAIL;
}

//将数组tab_from中的内存合并至数组tab_to中
//要求tab_from中存在tab_to中对应space, attrs的mem_tab，并且数组tab_to中的aligment大于数组tab_from
static int32_t hik_apt_mem_merge_arry(VCA_MEM_TAB_V3 *tab_to, int32_t len_to, VCA_MEM_TAB_V3 *tab_from, int32_t len_from)
{
    int32_t ret;
    int i, j;

    for(i = 0; i < len_from; i++)
    {

        for(j = 0; j < len_to; j++)
        {
            ret = hik_apt_mem_merge(&tab_to[j], &tab_from[i]);
            if(ret == HIK_APT_S_OK)
            {
                break;
            }
        }
#ifndef HIK_APT_SIMPLE_LOG
        if(j >= len_to)
        {
            for(j = 0; j < len_to; j++)
            {
                HIK_APT_DUMP_VAL(j, "%d");
                HIK_APT_DUMP_VAL((uint32_t)tab_to[j].size, "%d");
                HIK_APT_DUMP_VAL(tab_to[j].space, "%d");
                HIK_APT_DUMP_VAL(tab_to[j].attrs, "%d");
                HIK_APT_DUMP_VAL(tab_to[j].plat, "%d");
            }
            HIK_APT_DUMP_VAL(i, "%d");
			HIK_APT_DUMP_VAL((uint32_t)tab_from[i].size, "%d");
            HIK_APT_DUMP_VAL(tab_from[i].space, "%d");
            HIK_APT_DUMP_VAL(tab_from[i].attrs, "%d");
            HIK_APT_DUMP_VAL(tab_from[i].plat, "%d");
        }
#endif
        HIK_APT_CHECK_ERR_FMT(j >= len_to, HIK_APT_S_FAIL, "Merge tab[%d] failed!", i);
    }

    return HIK_APT_S_OK;

}

//从tab_from中分配空间到tab_to中
static int32_t hik_apt_mem_alloc_tab(VCA_MEM_TAB_V3 *tab_from, VCA_MEM_TAB_V3 *tab_to)
{
    tab_to->base = tab_to->phy_base = NULL;
    if(tab_to->size == 0)
    {
        return HIK_APT_S_OK;
    }

	if (tab_from->plat == tab_to->plat 
		    && tab_from->space == tab_to->space
            && tab_from->attrs == tab_to->attrs
            && tab_from->alignment >= tab_to->alignment
            && tab_from->size >= tab_to->size)
    {
#ifndef DISABLE_HIK_APT_MEM_SRATCH
        if (tab_from->attrs == VCA_MEM_SCRATCH)
        {
            tab_to->base = tab_from->base;
            tab_to->phy_base = tab_from->phy_base;
        }
        else
#endif
        {
            //非复用内存的空间，待分配的需要与被合并的使用相同的aligment（被合并的大于待分配的）
            //mem_tab是从前往后分配的
            size_t size = VCA_SIZE_ALIGN(tab_to->size, tab_from->alignment);
            tab_to->base = tab_from->base;
            tab_to->phy_base = tab_from->phy_base;
            tab_from->base = (void *)((size_t)tab_from->base + size);
            tab_from->phy_base = (void *)(tab_from->phy_base == NULL ?
                                          NULL :(void *)((size_t)tab_from->phy_base + size));
            tab_from->size = size > tab_from->size ? 0 : tab_from->size - size;
        }
        return HIK_APT_S_OK;
    }
    return HIK_APT_S_FAIL;
}

//从tab_from数组中分配空间到tab_to数组
static int32_t hik_apt_mem_alloc_tab_arry(VCA_MEM_TAB_V3 *tab_from, int32_t len_from, VCA_MEM_TAB_V3 *tab_to, int32_t len_to)
{

    int32_t ret;
    int i, j;

    for(i = 0; i < len_to; i++)
    {


        for(j = 0; j < len_from; j++)
        {
            ret = hik_apt_mem_alloc_tab(&tab_from[j], &tab_to[i]);
            if(ret == HIK_APT_S_OK)
            {
                break;
            }
        }
#ifndef HIK_APT_SIMPLE_LOG
        if(j >= len_from)
        {
            for(j = 0; j < len_from; j++)
            {
                HIK_APT_DUMP_VAL(j, "%d");
				HIK_APT_DUMP_VAL((uint32_t)tab_from[j].size, "%d");
                HIK_APT_DUMP_VAL(tab_from[j].space, "%d");
                HIK_APT_DUMP_VAL(tab_from[j].attrs, "%d");
                HIK_APT_DUMP_VAL(tab_from[j].plat, "%d");
            }
            HIK_APT_DUMP_VAL(i, "%d");
			HIK_APT_DUMP_VAL((uint32_t)tab_to[i].size, "%d");
            HIK_APT_DUMP_VAL(tab_to[i].space, "%d");
            HIK_APT_DUMP_VAL(tab_to[i].attrs, "%d");
            HIK_APT_DUMP_VAL(tab_to[i].plat, "%d");
        }
#endif
        HIK_APT_CHECK_ERR_FMT(j >= len_from, HIK_APT_S_FAIL, "alloc tab[%d] failed!", i);
    }

    return HIK_APT_S_OK;
}

//计算分配内存
static int32_t hik_apt_mem_alloc(VCA_MEM_TAB_V3    *mem_tab, 
                                 size_t             size,
                                 VCA_MEM_ALIGNMENT  alignment,
                                 VCA_MEM_ATTRS      attrs,
                                 void             **p_vptr,
                                 void             **p_phy_ptr)
{
    void *ptr = NULL;
    if(size == 0)
    {
        return HIK_APT_S_OK;
    }
    HIK_APT_CHECK_ERR(attrs != mem_tab->attrs, HIK_APT_MEM_INVALID_ATTR);
    HIK_APT_CHECK_ERR(size > mem_tab->size, HIK_APT_S_FAIL);

    if(attrs == VCA_MEM_SCRATCH)
    {
        HIK_APT_CHECK_ERR(alignment > mem_tab->alignment, HIK_APT_MEM_INVALID_ATTR);
        ptr = mem_tab->base;
        if(p_phy_ptr)
        {
            *p_phy_ptr = mem_tab->phy_base;
        }
    }
    else
    {
        //直接分配内存是从后往前分配的
        ptr = (void *)((size_t)mem_tab->base + mem_tab->size - size);
        ptr = (void *)((size_t)ptr & (~(size_t)alignment));
        size = (size_t)mem_tab->base + (size_t)mem_tab->size - (size_t)ptr;
        if(p_phy_ptr)
        {
            *p_phy_ptr = (void *)(mem_tab->phy_base == NULL ? (size_t)NULL :
                  (size_t)mem_tab->phy_base + (size_t)mem_tab->size - size);

        }
        mem_tab->size -= size;
    }
    *p_vptr = ptr;

    return HIK_APT_S_OK;
}

//计算mem_size
static int32_t hik_apt_mem_cal_alloc_sz(VCA_MEM_TAB_V3 *mem_tab, size_t size,
                                        VCA_MEM_ALIGNMENT  alignment, VCA_MEM_ATTRS attrs)
{
    if(size == 0)
    {
        return HIK_APT_S_OK;
    }
    HIK_APT_CHECK_ERR(attrs != mem_tab->attrs, HIK_APT_MEM_INVALID_ATTR);

    if(attrs == VCA_MEM_SCRATCH)
    {
        //复用空间记录最大的aligment
        mem_tab->alignment = VCA_MAX(alignment, mem_tab->alignment);
        mem_tab->size = VCA_MAX(size, mem_tab->size);
    }
    else
    {
        //非复用空间要求起始aligment足够大，申请的algiment不能大于最大的aligment
        HIK_APT_CHECK_ERR(alignment < mem_tab->alignment, HIK_APT_MEM_INVALID_ALIGN);
        //允许在非复用空间申请小于aligment的内存
        mem_tab->size += VCA_SIZE_ALIGN(mem_tab->size, alignment) + VCA_SIZE_ALIGN(size, alignment);
    }

    return HIK_APT_S_OK;
}

//初始化mem_tab, 用于计算mem_tab
static int32_t hik_apt_mem_tab_init(VCA_MEM_TAB_V3 *mem_tab, VCA_MEM_SPACE space, VCA_MEM_ALIGNMENT  alignment, VCA_MEM_ATTRS attrs)
{
    memset(mem_tab, 0, sizeof(VCA_MEM_TAB_V3));
    mem_tab->space = space;
    mem_tab->alignment = alignment;
    mem_tab->attrs = attrs;

    return HIK_APT_S_OK;
}



#endif // HIK_APT_MEM_H
