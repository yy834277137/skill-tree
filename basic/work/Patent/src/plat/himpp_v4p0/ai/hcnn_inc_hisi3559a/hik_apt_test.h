/***************************************************************************************************
* 版权信息：版权所有(c) , 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称: hik_apt_test.h
* 文件标识: __HIK_APT_TEST_H__
* 摘    要: 平台通用测试接口
*
* 当前版本: 0.1.0
* 作    者: 崔莫磊
* 日    期: 2019-11-21
* 备    注: 新建
*
***************************************************************************************************/
#ifndef __HIK_APT_TEST_H__
#define __HIK_APT_TEST_H__
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "vca_base.h"
#include "hik_apt_err.h"
#include "hik_apt_dbg.h"

#ifndef HIK_APT_TEST_CHECK_GOTO
#define HIK_APT_TEST_CHECK_GOTO(sts, err_code, to)                                            \
    do                                                                                        \
    {                                                                                         \
        if (sts)                                                                              \
        {                                                                                     \
            ret_val = err_code;                                                               \
            HIK_APT_ERR("\r\nERROR(%x) %s %d: %s\n", err_code, __FUNCTION__, __LINE__, #sts); \
            goto to;                                                                          \
        }                                                                                     \
    } while (0)
#endif

#ifndef VCA_MAX
#define VCA_MAX(a, b) (((a) < (b)) ? (b) : (a)) // 最大值
#endif
#ifndef VCA_MIN
#define VCA_MIN(a, b) (((a) > (b)) ? (b) : (a)) // 最小值
#endif
#ifndef VCA_CLIP
#define VCA_CLIP(v, minv, maxv) VCA_MIN((maxv), VCA_MAX((v), (minv))) // 饱和
#endif

#define HIK_APT_TEST_NT98528_POOL_SIZE (80 * 1024 * 1024)
static int32_t first_init = 0;

typedef struct _HIK_APT_TEST_RAW_HEADER
{
    uint16_t width;
    uint16_t height;
    uint8_t  data[1];
}HIK_APT_TEST_RAW_HEADER;

//==============================第一部分 基础内存分配===========================

//aligned malloc
static int hik_apt_test_aligned_alloc(size_t alignment, size_t malloc_size, void **ret_ptr)
{
    size_t max_align = VCA_MAX(alignment, 2 * sizeof(void *));
    size_t align_size = max_align + malloc_size;
    void *raw_malloc_ptr = (void *)malloc(align_size);
    *ret_ptr = NULL;
    if (NULL == raw_malloc_ptr)
    {
        HIK_APT_ERR("malloc ARM_MEM failed\n");
        return -1;
    }

    //清空分配内存
    memset(raw_malloc_ptr, 0, align_size);
    *ret_ptr = (void *)((((size_t)raw_malloc_ptr + (max_align - 1)) / max_align) * max_align);
    *ret_ptr = *ret_ptr == raw_malloc_ptr ? (void *)((size_t)raw_malloc_ptr + max_align) : *ret_ptr;
    *(void **)((size_t)*ret_ptr - sizeof(void *)) = raw_malloc_ptr;
    return HIK_APT_S_OK;
}

static void hik_apt_test_aligned_free(void *ptr)
{
    if (ptr)
    {
        //free(((void **)ptr)[-1]);
        free(*(void **)((size_t)ptr - sizeof(void *)));
    }
}

#ifndef _WIN32
//in windows this functions had been defiend
static void *_aligned_malloc(size_t size, size_t align)
{
    void *ret_ptr;
    int32_t ret = -1;
    ret = hik_apt_test_aligned_alloc(align, size, &ret_ptr);
    if (ret != HIK_APT_S_OK)
    {
        HIK_APT_DUMP_VAL(ret, "%08x");
        return NULL;
    }
    return ret_ptr;
}

static void _aligned_free(void *ptr)
{
    hik_apt_test_aligned_free(ptr);
}
#endif

//内存池分配
static int hik_apt_test_alloc_pool(void **buf_ptr, size_t *buf_size, size_t new_buf_size)
{
    void *p_buf = *buf_ptr;
    if (new_buf_size > *buf_size)
    {
        if (*buf_ptr)
        {
            p_buf = realloc(*buf_ptr, new_buf_size);
        }
        else
        {
            p_buf = malloc(new_buf_size);
        }

        if (p_buf == NULL)
        {
            return -1;
        }
        *buf_size = new_buf_size;
        *buf_ptr = p_buf;
    }

    return HIK_APT_S_OK;
}

static void hik_apt_test_free_pool(void *ptr)
{
    free(ptr);
}



#ifdef _WIN32
#include "hik_apt_test_x86.h"
#elif defined(NT98528)
#include "hik_apt_test_nt98528.h"
#elif defined(HISI_PLT)
#include "hik_apt_test_hisi.h"
#else
#error "no support platform"
#endif


static void hik_apt_test_free_memtab_array(VCA_MEM_TAB_V3 *mem_tab, int num)
{
    int i;

    for (i = 0; i < num; i++)
    {
        if (mem_tab[i].base || mem_tab[i].phy_base)
        {
            hik_apt_test_free_memtab(&mem_tab[i]);
        }
    }
}

/***********************************************************************************************************************
* 功  能: 显示mem_tab内存
* 参  数:
*   mem_tab   -I/O   记录单个内存块的所有信息
* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/

static int hik_apt_test_dump_memtab_array(VCA_MEM_TAB_V3 *mem_tab, int num, char *module_name)
{
    int ret = HIK_APT_S_OK, i;
    static size_t total_mmz = 0;
    static size_t total_mem = 0;

    // for (i = 0; i < num; i++)
    // {
    //     HIK_APT_DUMP("mem_tab[%d].size = %d;\r\n", i, mem_tab[i].size);
    //     HIK_APT_DUMP("mem_tab[%d].alignment = %d;\r\n", i, mem_tab[i].alignment);
    //     HIK_APT_DUMP("mem_tab[%d].attrs = %d;\r\n", i, mem_tab[i].attrs);
    //     HIK_APT_DUMP("mem_tab[%d].space = %d;\r\n", i, mem_tab[i].space);
    //     HIK_APT_DUMP("mem_tab[%d].plat = %d;\r\n", i, mem_tab[i].plat);
    // }

    //一次分派每一个NNIE_MEM_TAB
    HIK_APT_DUMP("================= %s Memory Start ===============\r\n", module_name);
    for (i = 0; i < num; i++)
    {
        if (mem_tab[i].space == VCA_HCNN_MEM_MMZ_NO_CACHE ||
            mem_tab[i].space == VCA_HCNN_MEM_MMZ_NO_CACHE_PRIORITY ||
            mem_tab[i].space == VCA_HCNN_MEM_MMZ_WITH_CACHE ||
            mem_tab[i].space == VCA_HCNN_MEM_MMZ_WITH_CACHE_PRIORITY)
        {
            HIK_APT_DUMP("%s[%d](scratch:%d, space %d, plat %d) = %fMB MMZ\r\n",
                         module_name, i,
                         mem_tab[i].attrs == VCA_MEM_SCRATCH,
                         mem_tab[i].space,
                         mem_tab[i].plat,
                         mem_tab[i].size * (1.f / 1024 / 1024));

            total_mmz += mem_tab[i].size;
        }
        else
        {
            HIK_APT_DUMP("%s[%d](scratch:%d, space %d, plat %d) = %fMB DDR\r\n",
                         module_name, i,
                         mem_tab[i].attrs == VCA_MEM_SCRATCH,
                         mem_tab[i].space,
                         mem_tab[i].plat,
                         mem_tab[i].size * (1.f / 1024 / 1024));

            total_mem += mem_tab[i].size;
        }
    }
    HIK_APT_DUMP("================= %s Memory End ===============\r\n", module_name);
    HIK_APT_DUMP("Total memory %f mmz %f mem\r\n",
                 total_mmz * (1.f / 1024 / 1024),
                 total_mem * (1.f / 1024 / 1024));
    return ret;
}

/***********************************************************************************************************************
* 功  能: 分配nnie模块内需要的内存
* 参  数:
*   mem_tab  -I/O   nnie需要的所有内存信息
* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/
static int hik_apt_test_alloc_memtab_array(VCA_MEM_TAB_V3 *mem_tab, int num)
{
    int ret = HIK_APT_S_OK, i;

    //一次分派每一个NNIE_MEM_TAB
    for (i = 0; i < num; i++)
    {
        if (mem_tab[i].size == 0)
        {
            continue;
        }
        ret = hik_apt_test_alloc_memtab(&mem_tab[i]);

        if (ret != HIK_APT_S_OK)
        {
            HIK_APT_DUMP_VAL(i, "%d");
            HIK_APT_DUMP_VAL(mem_tab[i].alignment, "%d");
            HIK_APT_DUMP_VAL(mem_tab[i].attrs, "%d");
            HIK_APT_DUMP_VAL(mem_tab[i].plat, "%d");
            HIK_APT_DUMP_VAL(mem_tab[i].space, "%d");
            HIK_APT_DUMP_VAL((uint32_t)mem_tab[i].size, "%d");
        }

        HIK_APT_CHECK_ERR(ret != HIK_APT_S_OK, ret);
    }

    return ret;
}

//文件读写接口
//read data from file
static int hik_apt_test_load_bin(const char *file_path, void **p_buf, size_t *p_size, int b_alloc)
{
    int ret_val = HIK_APT_S_OK;
    FILE* fp = fopen(file_path, "rb");
    HIK_APT_CHECK_ERR(NULL == fp, -1);
    fseek(fp, 0, SEEK_END);
    uint32_t file_sz = (unsigned int)ftell(fp);
    void * p_alloc_buffer = NULL;
    fseek(fp, 0, SEEK_SET);

    if(b_alloc)
    {
        ret_val = hik_apt_test_aligned_alloc(128, file_sz, &p_alloc_buffer);
        HIK_APT_TEST_CHECK_GOTO(ret_val != HIK_APT_S_OK, -3, FAILED);
    }
    else
    {
        p_alloc_buffer = *p_buf;
        if(p_size)
        {
            HIK_APT_TEST_CHECK_GOTO(*p_size < file_sz, HIK_APT_MEM_LACK_OF_MEM, FAILED);
        }
    }
    HIK_APT_TEST_CHECK_GOTO(p_alloc_buffer == NULL, HIK_APT_MEM_LACK_OF_MEM, FAILED);
    fread(p_alloc_buffer, 1, file_sz, fp);
    *p_buf = p_alloc_buffer;
    if(p_size)
    {
        *p_size = file_sz;
    }
FAILED:
    fclose(fp);
    return ret_val;
}

static int hik_apt_test_load_txt(const char *file_path, void **p_buf, size_t *p_size, int b_alloc)
{
    int ret_val = HIK_APT_S_OK;
    FILE* fp = fopen(file_path, "r");
    HIK_APT_CHECK_ERR(NULL == fp, -1);
    fseek(fp, 0, SEEK_END);
    uint32_t file_sz = (unsigned int)ftell(fp);
    void * p_alloc_buffer = NULL;
    fseek(fp, 0, SEEK_SET);

    if(b_alloc)
    {
        ret_val = hik_apt_test_aligned_alloc(128, file_sz + 1, &p_alloc_buffer);
        HIK_APT_TEST_CHECK_GOTO(ret_val != HIK_APT_S_OK, -3, FAILED);
    }
    else
    {
        p_alloc_buffer = *p_buf;
        if(p_size)
        {
            HIK_APT_TEST_CHECK_GOTO(*p_size < file_sz + 1, HIK_APT_MEM_LACK_OF_MEM, FAILED);
        }
    }
    HIK_APT_TEST_CHECK_GOTO(p_alloc_buffer == NULL, HIK_APT_MEM_LACK_OF_MEM, FAILED);
    fread(p_alloc_buffer, 1, file_sz, fp);
    *p_buf = p_alloc_buffer;
    *((uint8_t*)p_alloc_buffer + file_sz) = 0; // end of str
    if(p_size)
    {
        *p_size = file_sz;
    }
FAILED:
    fclose(fp);
    return ret_val;
}


/***********************************************************************************************************************
* 功  能: 读取模型到VCA_MODEL_LIST中
* 参  数:
*       buf          -I/O 空闲的mem_tab,用于内存集中释放
*       im_w         -I/O 空闲mem_tab的数量free_mem_tab_num
*       im_h         -I   申请的空闲mem_tab数量
* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/

static int hik_apt_test_load_models(VCA_MODEL_LIST *model_list,
                                    int list_len,
                                    char *model_path_li[],
                                    int *model_type_li,
                                    int dl_proc_type)
{
    int ret = HIK_APT_S_OK;
    int i;
    void *p_file_buf;
    size_t file_len;

    HIK_APT_CHECK_ERR(model_list == NULL, ret);
    HIK_APT_CHECK_ERR(model_path_li == NULL, ret);

    model_list->model_num = list_len;
    for (i = 0; i < list_len; i++)
    {
        HIK_APT_CHECK_ERR(model_path_li[i] == NULL, ret);
        ret = hik_apt_test_load_bin(model_path_li[i],
                                     &p_file_buf,
                                     &file_len, 1);

        HIK_APT_CHECK_ERR(ret != HIK_APT_S_OK, ret);

        model_list->model_info[i].model_size = (unsigned int)file_len;
        model_list->model_info[i].model_buf = p_file_buf;
        if (model_type_li)
        {
            model_list->model_info[i].model_type = model_type_li[i];
        }
        else
        {
            model_list->model_info[i].model_type = 0;
        }
        model_list->model_info[i].dl_proc_type = dl_proc_type;
    }
    return ret;
}

/***********************************************************************************************************************
* 功  能: 实现nv21图像到nv12的转换，效率较低
* 参  数:
*       buf          -I/O 空闲的mem_tab,用于内存集中释放
*       im_w         -I/O 空闲mem_tab的数量free_mem_tab_num
*       im_h         -I   申请的空闲mem_tab数量
* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/
static int hik_apt_test_nv21_2_nv12(void *buf, int im_w, int im_h)
{
    int i, j;
    int res = HIK_APT_S_OK;
    uint8_t *im_buf = (uint8_t *)buf;
    uint8_t *uv_buf = (uint8_t *)(im_buf + im_w * im_h);
    uint8_t tmp = 0;

    for (i = 0; i < im_h / 2; i++)
    {
        for (j = 0; j < im_w; j += 2)
        {
            tmp = uv_buf[i * im_w + j];
            uv_buf[i * im_w + j] = uv_buf[i * im_w + j + 1];
            uv_buf[i * im_w + j + 1] = tmp;
        }
    }
    return res;
}

/***********************************************************************************************************************
* 功  能: 从mem_tab list中分配空间的mem_tab，用于内存的集中释放
* 参  数:
*       free_mem_tab_list          -I/O 空闲的mem_tab,用于内存集中释放
*       free_mem_tab_len           -I/O 空闲mem_tab的数量free_mem_tab_num
*       alloc_tab_len              -I   申请的空闲mem_tab数量
* 返回值: 成功返回申请成功mem_tab地址，错误返回NULL
* 备  注: 无
***********************************************************************************************************************/
static VCA_MEM_TAB_V3 *hik_apt_test_get_free_mem_tab(VCA_MEM_TAB_V3 **free_mem_tab_list,
                                                     int *free_mem_tab_len,
                                                     int alloc_tab_len)
{
    VCA_MEM_TAB_V3 *ret = NULL;
    if ((*free_mem_tab_len) < alloc_tab_len)
    {
        HIK_APT_DUMP_VAL((*free_mem_tab_len), "%d");
        HIK_APT_DUMP_VAL(alloc_tab_len, "%d");
        return NULL;
    }
    ret = *free_mem_tab_list;
    *free_mem_tab_list += alloc_tab_len;
    *free_mem_tab_len -= alloc_tab_len;
    return ret;
}

/***********************************************************************************************************************
* 功  能: demo中用于分配某一块内存的接口
* 参  数:
*       free_mem_tab          -I/O 空闲的mem_tab,用于内存集中释放
*       free_mem_tab_num      -I/O 空闲mem_tab的数量free_mem_tab_num
*       p_schedular_hdl       -O   调度器句柄，在PC上则是cuda句柄
* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/

static int hik_apt_test_alloc_mem(VCA_MEM_TAB_V3 **free_mem_tab,
                                  int *free_mem_tab_num,
                                  size_t alloc_size,
                                  VCA_MEM_ATTRS attrs,
                                  VCA_MEM_ALIGNMENT aligment,
                                  VCA_MEM_PLAT plat,
                                  void **p_alloc_buf)
{
    int res;
    VCA_MEM_TAB_V3 *mem_tab_input = hik_apt_test_get_free_mem_tab(free_mem_tab, free_mem_tab_num, 1);
    HIK_APT_CHECK_ERR(mem_tab_input == NULL, HIK_APT_S_FAIL);
    mem_tab_input->size = alloc_size;
    mem_tab_input->attrs = attrs;        // VCA_MEM_PERSIST;
    mem_tab_input->alignment = aligment; // VCA_MEM_ALIGN_128BYTE;
    mem_tab_input->plat = plat;          // VCA_MEM_PLAT_CPU;
    res = hik_apt_test_alloc_memtab(mem_tab_input);
    HIK_APT_CHECK_ERR(res != HIK_APT_S_OK, res);
    *p_alloc_buf = mem_tab_input->base;

    return HIK_APT_S_OK;
}

/***********************************************************************************************************************
* 功  能: demo中用于从文件句柄中读取一行的api
* 参  数:
*       fp          -I/O 文件句柄
*       str_buf     -O   用于存储行内容的buffer
*       buf_size    -I   用于存储行内容的buffer长度
* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/
static int hik_apt_test_read_line(FILE *fp, char *str_buf, size_t buf_size)
{
    int read_size = 0;
    int i = 0;
    long file_index = ftell(fp);
    read_size = (int)fread(str_buf, 1, buf_size, fp);
    //support \r\n
    if (str_buf[0] == '\n')
    {
        fseek(fp, file_index + 1, SEEK_SET);
        file_index = ftell(fp);
        read_size = (int)fread(str_buf, 1, buf_size, fp);
    }

    if (read_size <= 1)
    {
        return HIK_APT_S_FAIL;
    }

    for (i = 1; i < read_size; i++)
    {
        if (str_buf[i] == '\n') //find \n
        {
            str_buf[i] = 0;
            if (str_buf[i - 1] == '\r') //support \r\n
            {
                str_buf[i - 1] = 0;
                fseek(fp, file_index + i + 1, SEEK_SET);
            }
            else
            {
                fseek(fp, file_index + i + 1, SEEK_SET);
            }
            break;
        }
    }

    //could not find \n in a line
    HIK_APT_CHECK_ERR(read_size == i, HIK_APT_MEM_LACK_OF_MEM);

    return HIK_APT_S_OK;
}

#endif // TEST_HEADER_H
