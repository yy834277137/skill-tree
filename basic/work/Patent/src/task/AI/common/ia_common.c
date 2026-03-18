/*******************************************************************************
* ia_common.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2019年11月25日 Create
*
* Description :
* Modification:
*******************************************************************************/


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal.h"

#include "system_prm_api.h"
#include "ia_common.h"
#include "capbility.h"
#include "vdec_tsk_api.h"


/* ========================================================================== */
/*                             宏定义区                                       */
/* ========================================================================== */
#define IA_CHECK_CHAN(chan, value)	{if (chan > 3) {SAL_LOGE("Input FACE Chan[%d] is Invalid!\n", chan); return (value); }}
#define IA_CHECK_PRM(ptr, value)	{if (!ptr) {SAL_LOGE("Ptr (The address is empty or Value is 0 )\n"); return (value); }}
#define IA_CHECK_RETURN(ret, value) {if (ret != 0) {SAL_LOGE("fail\n"); return (value); }}
#define HIK_APT_DUMP_VAL(val, fmt)	HIK_APT_ERR("%s %d: " # val " = "fmt "\r\n", __FUNCTION__, __LINE__, val);
#define CFG_PATH ("./sva/sample.cfg")
#define CFG_PATH_MAX_LEN   (64)

/* ========================================================================== */
/*                             数据结构区                                     */
/* ========================================================================== */
void *scheduler_handle = NULL;  /* 行为分析需要与安检智能共用调度句柄，安检智能默认开启 */
static void *pSbaSchedulerHandle = NULL;               /*行为分析模块的调度句柄*/

typedef struct _IA_CFG_PRM_
{
    UINT32 uiRegModMask;                                          /* 注册模块掩码 */

    cJSON stModCfgPrm[IA_MOD_MAX_NUM];                             /* 各智能子模块的配置参数 */
    INT32(*IA_ModCfgFunc[IA_MOD_MAX_NUM]) (cJSON * pstItem);        /* 安检智能配置项 */
} IA_CFG_PRM_S;

/* fixme: 暂时只适配H5分析仪，其他产品类型暂未适配。
	当前H5在底层处理时存在内存申请需要在mmz起始3.2G以内的限制，否则SVA智能识别不生效
	其他平台暂无此限制，故其他平台直接使用mem_hal封装的平台层内存申请/释放接口动态调用。
*/
/* szl_todo: 智能框架整理，需要将平台层各智能的内存表抽离出来固化 */
static UINT32 gMemSizeTab[IA_MOD_MAX_NUM][INPUT_MAX_NUM][IA_MEM_TYPE_NUM] =
{
    /* SVA模块 */
    {
        /* 单路设备 880M */
        {
            0,                       /* IA_MALLOC */
#ifdef HI3559A_SPC030
            730 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE */
            0 * 1024 * 1024,                        /* IA_HISI_MMZ_NO_CACHE */
            45 * 1024 * 1024,                       /* IA_HISI_MMZ_CACHE_PRIORITY */
            95 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE_NO_PRIORITY */
#else
			0 * 1024 * 1024,						/* IA_HISI_MMZ_CACHE */
			0 * 1024 * 1024,						/* IA_HISI_MMZ_NO_CACHE */
			0 * 1024 * 1024,						/* IA_HISI_MMZ_CACHE_PRIORITY */
			0 * 1024 * 1024,					   /* IA_HISI_MMZ_CACHE_NO_PRIORITY */
#endif
            0, 0, 0, 
            0, /* NT */
            0, 0, /* RK，下同 */
        },

        /* 双路设备 1440M */
        {
            0,                      /* IA_MALLOC */
#ifdef HI3559A_SPC030
            1150 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE */
            0,                                      /* IA_HISI_MMZ_NO_CACHE */
            90 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE_PRIORITY */
            190 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE_NO_PRIORITY */
#else
			0 * 1024 * 1024,						/* IA_HISI_MMZ_CACHE */
			0 * 1024 * 1024,						/* IA_HISI_MMZ_NO_CACHE */
			0 * 1024 * 1024,						/* IA_HISI_MMZ_CACHE_PRIORITY */
			0 * 1024 * 1024,					   /* IA_HISI_MMZ_CACHE_NO_PRIORITY */
#endif
            0, 0, 0, 
            0,
            0, 0
        },
    },

    /* BA模块 */
    {
        /* 单路设备 */
        {
#ifdef HI3559A_SPC030
            80 * 1024 * 1024, 
			150* 1024 * 1024, 
#else
			0, 0,
#endif
			0, 0, 0, 0, 0, 0, 
			0, 
			0, 0
        },

        /* 双路设备 */
        {
#ifdef HI3559A_SPC030
            80 * 1024 * 1024, 
			150* 1024 * 1024,
#else
			0, 0,
#endif
			0, 0, 0, 0, 0, 0, 
			0,
			0, 0
        },
    },

    /* FACE模块 */
    {
        /* 单路设备 */
        {
            0, 0, 0, 0, 0, 0, 0, 0, 
			0, /* nt */
			0, 0/* rk，下同 */ 
        },

        /* 双路设备 */
        {
            0, 0, 0, 0, 0, 0, 0, 0, 
			0,
			0, 0
        },
    },

    /* PPM模块 */
    {
        /* 单路设备 */
        {
#ifdef HI3559A_SPC030
            20 * 1024 * 1024,
            850 * 1024 * 1024,
            50 * 1024 * 1024,
            80 * 1024 * 1024,
            30 * 1024 * 1024,
            80 * 1024 * 1024,                        /* IA_HISI_VB_REMAP_NONE */
#else
			0, 0, 0, 0, 0, 0,
#endif
            0, 0, 
            0,
            0, 0
        },

        /* 双路设备 */
        {
#ifdef HI3559A_SPC030
            20 * 1024 * 1024,
            850 * 1024 * 1024,
            50 * 1024 * 1024,
            80 * 1024 * 1024,
            30 * 1024 * 1024,
            80 * 1024 * 1024,                        /* IA_HISI_VB_REMAP_NONE */
#else
			0, 0, 0, 0, 0, 0,
#endif
            0, 0, 
            0,
            0, 0
        },
    },
};

#if 0
/* 智能模块初始化内存信息表 */
static UINT32 gMemSizeTab[DEVICE_TYPE_NUM][IA_MOD_MAX_NUM][IA_MEM_TYPE_NUM] =
{
    /* 分析仪单输入设备(单芯片) */
    {
        {
            10 * 1024 * 1024,                       /* IA_MALLOC */
            600 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE */
            10 * 1024 * 1024,                       /* IA_HISI_MMZ_NO_CACHE */
            50 * 1024 * 1024,                       /* IA_HISI_MMZ_CACHE_PRIORITY */
            150 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE_NO_PRIORITY */
            0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, FACE_MMZ_CACHE_SIZE, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
    },

    /* 分析仪单输入设备(双芯片-主) */
    {
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, FACE_MMZ_CACHE_SIZE, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
    },

    /* 分析仪单输入设备(双芯片-从) */
    {
        {
            10 * 1024 * 1024,                       /* IA_MALLOC */
            700 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE */
            50 * 1024 * 1024,                       /* IA_HISI_MMZ_NO_CACHE */
            150 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE_PRIORITY */
            200 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE_NO_PRIORITY */
            0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, FACE_MMZ_CACHE_SIZE, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
    },

    /* 分析仪双输入设备(单芯片) */
    {
        {
            100 * 1024 * 1024,                      /* IA_MALLOC */
            700 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE */
            50 * 1024 * 1024 + SVA_DEBUG_DUMP_MEM, /* IA_HISI_MMZ_NO_CACHE */
            150 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE_PRIORITY */
            200 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE_NO_PRIORITY */
            0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,

        },
        {
            0, FACE_MMZ_CACHE_SIZE, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
    },

    /* 分析仪双输入设备(双芯片-主) */
    {
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, FACE_MMZ_CACHE_SIZE, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
    },

    /* 分析仪双输入设备(双芯片-从) */
    {
        {
            10 * 1024 * 1024,                       /* IA_MALLOC */
            700 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE */
            50 * 1024 * 1024 + SVA_DEBUG_DUMP_MEM, /* IA_HISI_MMZ_NO_CACHE */
            150 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE_PRIORITY */
            200 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE_NO_PRIORITY */
            0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, FACE_MMZ_CACHE_SIZE, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
    },

    /*双模型安检机单视角内存表*/
    {
        {
            10 * 1024 * 1024,
            800 * 1024 * 1024,
            50 * 1024 * 1024 + SVA_DEBUG_DUMP_MEM,
            50 * 1024 * 1024,
            200 * 1024 * 1024,
            0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, FACE_MMZ_CACHE_SIZE, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
    },

    /* 安检机设备双视角*/
    {
        /*双模型安检机双视角内存表*/
        {
            20 * 1024 * 1024,
            1400 * 1024 * 1024,
            120 * 1024 * 1024 + SVA_DEBUG_DUMP_MEM,
            120 * 1024 * 1024,
            300 * 1024 * 1024,
            0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, FACE_MMZ_CACHE_SIZE, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
    },

    /* 人包关联设备单视角-主片 */
    {
        {
            10 * 1024 * 1024,                       /* IA_MALLOC */
            600 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE */
            50 * 1024 * 1024,                       /* IA_HISI_MMZ_NO_CACHE */
            70 * 1024 * 1024,                       /* IA_HISI_MMZ_CACHE_PRIORITY */
            100 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE_NO_PRIORITY */
            0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, FACE_MMZ_CACHE_SIZE, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
    },

    /* 人包关联设备单视角-从片 */
    {
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, FACE_MMZ_CACHE_SIZE, 0, 0, 0, 0, 0, 0,
        },
        {
            20 * 1024 * 1024,
            850 * 1024 * 1024,
            50 * 1024 * 1024,
            80 * 1024 * 1024,
            30 * 1024 * 1024,
            80 * 1024 * 1024,                        /* IA_HISI_VB_REMAP_NONE */
            0, 0,
        },
    },

    /* 人包关联设备双视角-主片 */
    {
        {
            100 * 1024 * 1024,                      /* IA_MALLOC */
            700 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE */
            200 * 1024 * 1024 + SVA_DEBUG_DUMP_MEM, /* IA_HISI_MMZ_NO_CACHE */
            300 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE_PRIORITY */
            200 * 1024 * 1024,                      /* IA_HISI_MMZ_CACHE_NO_PRIORITY */
            0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, FACE_MMZ_CACHE_SIZE, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
    },

    /* 人包关联设备双视角-从片 */
    {
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            0, FACE_MMZ_CACHE_SIZE, 0, 0, 0, 0, 0, 0,
        },
        {
            200 * 1024 * 1024,
            1.3 * 1024 * 1024 * 1024,
            250 * 1024 * 1024,
            80 * 1024 * 1024,
            80 * 1024 * 1024,
            80 * 1024 * 1024,                           /* IA_HISI_VB_REMAP_NONE */
            0, 0,
        },
    },

};
#endif

static CHAR gModMemName[IA_MOD_MAX_NUM][IA_MEM_TYPE_NUM][64] =
{
    {
        {"sva_malloc"}, {"sva_hisi_mmz_cache"}, {"sva_hisi_no_cache_mmz"}, {"sva_hisi_cache_pri_mmz"}, {"sva_hisi_no_pri_cache_mmz"}, {"sva_hisi_vb_remap_none"}, {"sva_hisi_vb_remap_no_cached"}, {"sva_hisi_vb_remap_cached"},\
		{"sva_nt_feature"},
		{"sva_rk_cache_cma"}, {"sva_rk_cache_iommu"},
    },
    {
        {"ba_malloc"}, {"ba_mmz_cache"}, {"ba_no_cache_mmz"}, {"ba_cache_pri_mmz"}, {"ba_no_pri_cache_mmz"}, {"ba_vb_remap_none"}, {"ba_vb_remap_no_cached"}, {"ba_vb_remap_cached"},\
		{"ba_nt_feature"},
		{"ba_rk_cache_cma"},{"ba_rk_cache_iommu"},
    },
    {
        {"face_malloc"}, {"face_mmz_cache"}, {"face_no_cache_mmz"}, {"face_cache_pri_mmz"}, {"face_no_pri_cache_mmz"} , {"face_vb_remap_none"}, {"face_vb_remap_no_cached"}, {"face_vb_remap_cached"},\
		{"face_nt_feature"},
		{"face_rk_cache_cma"},{"ba_rk_cache_iommu"},
    },
	{
		{"ppm_malloc"}, {"ppm_mmz_cache"}, {"ppm_no_cache_mmz"}, {"ppm_cache_pri_mmz"}, {"ppm_no_pri_cache_mmz"}, {"ppm_vb_remap_none"}, {"ppm_vb_remap_no_cached"}, {"ppm_vb_remap_cached"},\
		{"ppm_nt_feature"},
		{"ppm_rk_cache_cma"},{"ba_rk_cache_iommu"},
	},
};

#if 0
static IA_DEVICE_TYPE_E gChip2DevTypeTab[IA_PRODUCT_TYPE_NUM][DEVICE_CHIP_TYPE_NUM][INPUT_MAX_NUM] =
{
	{
	    {
	        ANALYZER_SINGLE_INPUT, ANALYZER_DOUBLE_INPUT,
	    },

	    {
	        ANALYZER_SINGLE_INPUT_MAIN, ANALYZER_DOUBLE_INPUT_MAIN
	    },

	    {
	        ANALYZER_SINGLE_INPUT_SLAVE, ANALYZER_DOUBLE_INPUT_SLAVE
	    },
	},
	
	{
		/* 人包关联产品暂时没有单芯片类型 */
		{ },
	
		{
			PEOPLE_PACKAGE_MATCH_SINGLE_INPUT_MAIN, PEOPLE_PACKAGE_MATCH_DOUBLE_INPUT_MAIN,
		},
	
		{
			PEOPLE_PACKAGE_MATCH_SINGLE_INPUT_SLAVE, PEOPLE_PACKAGE_MATCH_DOUBLE_INPUT_SLAVE
		},
	},

	{
		{
			SECURITY_MACHINE_TYPE_SINGLE, SECURITY_MACHINE_TYPE_DOUBLE
		},

		/* 安检机产品暂时没有双芯片类型 */
		{ }, { },
	},

	/* if needed, add new product type... */


};
#endif

/* 配置文件名称, ISM为安检机，ISA为分析仪 */
static CHAR szIsmCfgPathTab[CFG_PATH_MAX_LEN] = "./ia_common/ism/sample.cfg";      /* szl_todo: 资源中添加配置文件 */
static CHAR szIsaCfgPathTab[2][MODEL_TYPE_NUM][CFG_PATH_MAX_LEN] =
{
    /* TODO: 将智能模块通用配置文件整理到外部 */
    {"./ia_common/isa/single_model/sample_single_input.cfg", "./ia_common/isa/single_model/sample_double_input.cfg"},/* 当前没有单模型类型，临时保留兼容 */
	{"./ia_common/isa/sample_single_input.cfg", "./ia_common/isa/sample_double_input.cfg"}, 
};

static UINT32 gInitMemSize[IA_MOD_MAX_NUM][IA_MEM_TYPE_NUM] = {{0}};

static SVA_MEM_INFO gSvaMemInfo = {0};
static BA_MEM_INFO gBaMemInfo = {0};
static FACE_MEM_INFO gFaceMemInfo = {0};
static PPM_MEM_INFO gPpmMemInfo = {0};
static CHAR szDefaultCfgPath[CFG_PATH_MAX_LEN] = {0};

static IA_CFG_PRM_S g_stCfgPrm = {0};                   /* 全局参数，保存各智能子模块注册的信息，目前包括仅包括回调函数 */
static cJSON *g_pstRootItem = NULL;                     /* 该指针用来存储获取的配置文件，在完成模块配置后释放 */

/* ========================================================================== */
/*                             函数定义区                                     */
/* ========================================================================== */

/**
 * @function:   IA_SetModCfg
 * @brief:      配置所有注册的智能模块的应用配置
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
static INT32 IA_SetModCfg(VOID)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;

    for (i = 0; i < IA_MOD_MAX_NUM; i++)
    {
        if (!((g_stCfgPrm.uiRegModMask >> i) & 0x1))
        {
            SAL_LOGI("module %d no need init alg cfg file! \n", i);
            continue;
        }

        IA_CHECK_PRM(g_stCfgPrm.IA_ModCfgFunc[i], SAL_FAIL);

        s32Ret = g_stCfgPrm.IA_ModCfgFunc[i](&g_stCfgPrm.stModCfgPrm[i]);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("proc sub module %d cb func failied! \n", i);
            return SAL_FAIL;
        }
    }

    if (g_pstRootItem)
    {
        cJSON_Delete(g_pstRootItem);
    }

    return SAL_SOK;
}

/**
 * @function:   IA_ReadCfgFile
 * @brief:      读取应用层的配置文件信息
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     cJSON *
 */
static cJSON *IA_ReadCfgFile(VOID)
{
    INT32 s32Ret = SAL_SOK;

    INT32 iSize = 0;

    cJSON *pstItem = NULL;
    CHAR *pcData = NULL;
    FILE *fp = NULL;

    fp = fopen(szDefaultCfgPath, "r+");
    IA_CHECK_PRM(fp, NULL);

    s32Ret = fseek(fp, 0L, SEEK_END);
    if (s32Ret < 0)
    {
        SAL_LOGE("fseek failed! \n");
        goto exit;
    }

    iSize = ftell(fp);
    if (iSize < 0)
    {
        SAL_LOGE("ftell failed! \n");
        goto exit;
    }

    rewind(fp);

    pcData = SAL_memMalloc(iSize, "ia_common", "ia_read_cfg");
    if (!pcData)
    {
        SAL_LOGE("malloc failed! \n");
        goto exit;
    }

    s32Ret = fread(pcData, 1, iSize, fp);
    if (!s32Ret || feof(fp))
    {
        SAL_LOGE("fread err! \n");
        goto exit;
    }

    pstItem = cJSON_Parse(pcData);
    if (!pstItem)
    {
        SAL_LOGE("json parse failed! \n");
        goto exit;
    }

exit:
    if (pcData)
    {
        SAL_memfree(pcData, "ia_common", "ia_read_cfg");
        pcData = NULL;
    }

    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }

    return pstItem;
}

/**
 * @function:   IA_GetMiscCfgInfo
 * @brief:      获取其他配置信息
 * @param[in]:  cJSON *pParItem
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 IA_GetMiscCfgInfo(cJSON *pParItem)
{
    cJSON *pstOtherItem = NULL;

    IA_CHECK_PRM(pParItem, SAL_FAIL);

    pstOtherItem = cJSON_GetObjectItem(pParItem, "other_info");
    IA_CHECK_PRM(pstOtherItem, SAL_FAIL);
	
    return SAL_SOK;
}

/**
 * @function:   IA_GetAlgCfgInfo
 * @brief:      获取算法模块的配置信息
 * @param[in]:  cJSON *pParItem
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 IA_GetAlgCfgInfo(cJSON *pParItem)
{
    UINT32 i = 0;

    cJSON *pstAlgItem = NULL;
    cJSON *pstAlgSubItem = NULL;
	
    CHAR *pcModId2Str[IA_MOD_MAX_NUM] = {"sva", "ba", "face", "ppm"};

    IA_CHECK_PRM(pParItem, SAL_FAIL);

    pstAlgItem = cJSON_GetObjectItem(pParItem, "alg_info");
    IA_CHECK_PRM(pstAlgItem, SAL_FAIL);

    for (i = 0; i < IA_MOD_MAX_NUM; i++)
    {
        pstAlgSubItem = cJSON_GetObjectItem(pstAlgItem, pcModId2Str[i]);
        IA_CHECK_PRM(pstAlgSubItem, SAL_FAIL);

        sal_memcpy_s(&g_stCfgPrm.stModCfgPrm[i], sizeof(cJSON), pstAlgSubItem, sizeof(cJSON));
    }

    return SAL_SOK;
}

/**
 * @function:   IA_GetCfgInfo
 * @brief:      获取整个配置信息
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
static INT32 IA_GetCfgInfo(VOID)
{
    g_pstRootItem = IA_ReadCfgFile();
    IA_CHECK_PRM(g_pstRootItem, SAL_FAIL);

    /* alg info */
    if (SAL_SOK != IA_GetAlgCfgInfo(g_pstRootItem))
    {
        SAL_LOGE("Get Alg Cfg Info Failed! \n");
        return SAL_FAIL;
    }

    /* other info */
    if (SAL_SOK != IA_GetMiscCfgInfo(g_pstRootItem))
    {
        SAL_LOGE("Get Misc Cfg Info Failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   IA_GetModEnableFlag
 * @brief:      获取智能模块使能标记
 * @param[in]:  IA_MOD_IDX_E enModIdx
 * @param[out]: None
 * @return:     BOOL
 */
BOOL IA_GetModEnableFlag(IA_MOD_IDX_E enModIdx)
{
    DSPINITPARA *pstInit = SystemPrm_getDspInitPara();

    if (enModIdx >= IA_MOD_MAX_NUM)
    {
        SAL_LOGE("invalid ia module idx %d > max %d \n", enModIdx, IA_MOD_MAX_NUM);
        return SAL_FALSE;
    }

    return !!(pstInit->stIaInitMapPrm.stIaModCapbPrm[enModIdx].uiEnableFlag);
}

/**
 * @function:   IA_GetModTransFlag
 * @brief:      获取智能模块是否需要跨芯片传输标记
 * @param[in]:  IA_MOD_IDX_E enModIdx
 * @param[out]: None
 * @return:     BOOL
 */
BOOL IA_GetModTransFlag(IA_MOD_IDX_E enModIdx)
{
    DSPINITPARA *pstInit = SystemPrm_getDspInitPara();

    if (enModIdx >= IA_MOD_MAX_NUM)
    {
        SAL_LOGW("invalid ia module idx %d > max %d \n", enModIdx, IA_MOD_MAX_NUM);
        return SAL_FALSE;
    }

	if (SAL_TRUE != IA_GetModEnableFlag(enModIdx))
	{
		SAL_LOGI("mod %d not enable, return false \n", enModIdx);
		return SAL_FALSE;
	}

    return !!(pstInit->stIaInitMapPrm.stIaModCapbPrm[enModIdx].stIaModTransPrm.uiDualChipTransFlag);
}

/**
 * @function:   IA_GetModTransChipId
 * @brief:      获取智能模块运行芯片标识
 * @param[in]:  IA_MOD_IDX_E enModIdx
 * @param[out]: None
 * @return:     BOOL
 */
INT32 IA_GetModTransChipId(IA_MOD_IDX_E enModIdx)
{
    DSPINITPARA *pstInit = SystemPrm_getDspInitPara();

    if (enModIdx >= IA_MOD_MAX_NUM || SAL_TRUE != IA_GetModEnableFlag(enModIdx))
    {
        SAL_LOGE("invalid ia module idx %d > max %d, enable flag %d \n",
                 enModIdx, IA_MOD_MAX_NUM, IA_GetModEnableFlag(enModIdx));
        return SAL_FALSE;
    }

    return pstInit->stIaInitMapPrm.stIaModCapbPrm[enModIdx].stIaModTransPrm.uiRunChipId;
}

/**
 * @function:   IA_GetSvaAlgRunFlag
 * @brief:      获取sva模块算法资源是否运行标识
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     BOOL
 */
static BOOL IA_GetSvaAlgRunFlag(VOID)
{
    DSPINITPARA *pstInit = SystemPrm_getDspInitPara();

    return ((SAL_TRUE == IA_GetModEnableFlag(IA_MOD_SVA) && SAL_TRUE != IA_GetModTransFlag(IA_MOD_SVA))
            || (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_SVA) && SAL_TRUE == IA_GetModTransFlag(IA_MOD_SVA) && DOUBLE_CHIP_SLAVE_TYPE == pstInit->deviceType));
}

/**
 * @function:   IA_GetBaAlgRunFlag
 * @brief:      获取ba模块算法资源是否运行标识
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     BOOL
 */
static BOOL IA_GetBaAlgRunFlag(VOID)
{
    return (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_BA));
}

/**
 * @function:   IA_GetFaceAlgRunFlag
 * @brief:      获取face模块算法资源是否运行标识
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     BOOL
 */
static BOOL IA_GetFaceAlgRunFlag(VOID)
{
    return (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_FACE));
}

/**
 * @function:   IA_GetPpmAlgRunFlag
 * @brief:      获取ppm模块算法资源是否运行标识
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     BOOL
 */
static BOOL IA_GetPpmAlgRunFlag(VOID)
{
    return (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_PPM));
}

/**
 * @function:   IA_GetAlgRunFlag
 * @brief:      获取智能模块算法资源是否运行标识
 * @param[in]:  IA_MOD_IDX_E enModIdx
 * @param[out]: None
 * @return:     BOOL
 */
BOOL IA_GetAlgRunFlag(IA_MOD_IDX_E enModIdx)
{
    BOOL bAlgRunFlag = SAL_FALSE;

    switch (enModIdx)
    {
        case IA_MOD_SVA:
        {
            bAlgRunFlag = IA_GetSvaAlgRunFlag();
            break;
        }
        case IA_MOD_BA:
        {
            bAlgRunFlag = IA_GetBaAlgRunFlag();
            break;
        }
        case IA_MOD_FACE:
        {
            bAlgRunFlag = IA_GetFaceAlgRunFlag();
            break;
        }
        case IA_MOD_PPM:
        {
            bAlgRunFlag = IA_GetPpmAlgRunFlag();
            break;
        }
        default:
        {
            SAL_LOGE("invalid ia mod idx %d \n", enModIdx);
            break;
        }
    }

    return bAlgRunFlag;
}

/**
 * @function:   IA_RegModCfgCbFunc
 * @brief:      模块注册回调函数用于修改配置文件
 * @param[in]:  IA_MOD_IDX_E enModId
 * @param[in]:  CFG_CB_FUNC pCbFunc
 * @param[out]: None
 * @return:     INT32
 */
INT32 IA_RegModCfgCbFunc(IA_MOD_IDX_E enModId, CFG_CB_FUNC pCbFunc)
{
    if (enModId > IA_MOD_MAX_NUM - 1)
    {
        SAL_LOGE("Invalid Module Id %d \n", enModId);
        return SAL_FAIL;
    }

    if (SAL_TRUE != IA_GetModEnableFlag(enModId))
    {
        SAL_LOGW("mod %d no need init! \n", enModId);
        return SAL_SOK;
    }

    if (g_stCfgPrm.IA_ModCfgFunc[enModId])
    {
        SAL_LOGW("Module %d has registered cfg function! return success! \n", enModId);
        return SAL_SOK;
    }

    g_stCfgPrm.IA_ModCfgFunc[enModId] = pCbFunc;

    g_stCfgPrm.uiRegModMask |= 0x1 << enModId;
    return SAL_SOK;
}

/**
 * @function:   IA_InitCfgPrm
 * @brief:      读取应用层配置文件，初始化相关配置参数
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 IA_InitCfgPrm(VOID)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = IA_GetCfgInfo();
    IA_CHECK_RETURN(s32Ret, SAL_FAIL);

    s32Ret = IA_SetModCfg();
    IA_CHECK_RETURN(s32Ret, SAL_FAIL);

    return SAL_SOK;
}

/**
 * @function:   IA_InitCfgPath
 * @brief:      确定初始化的默认配置文件名称
 * @param[in]:  None
 * @param[out]: None
 * @return:     static INT32
 */
INT32 IA_InitCfgPath(VOID)
{
    DSPINITPARA *pstInit = SystemPrm_getDspInitPara();
    CAPB_PRODUCT *pstProduct = capb_get_product();
    
    if(NULL == pstProduct)
    {
        CAPT_LOGE("get platform capbility fail\n");
        return SAL_FAIL;
    }

    if (pstProduct->enInputType > VIDEO_INPUT_BUTT - 1
        || pstInit->stViInitInfoSt.uiViChn > 2
        || pstInit->stViInitInfoSt.uiViChn == 0
        || pstInit->modelType >= MODEL_TYPE_NUM)
    {
        SAL_ERROR("invalid prm! ipt_type %d, viCnt %d, modelType %d \n",
                  pstProduct->enInputType, pstInit->stViInitInfoSt.uiViChn, pstInit->modelType);
        return SAL_FAIL;
    }

    if (VIDEO_INPUT_INSIDE == pstProduct->enInputType)
    {
        snprintf(szDefaultCfgPath, CFG_PATH_MAX_LEN, szIsmCfgPathTab);
    }
    else
    {
        snprintf(szDefaultCfgPath, CFG_PATH_MAX_LEN, szIsaCfgPathTab[pstInit->modelType][pstInit->stViInitInfoSt.uiViChn - 1]);
    }

    SAL_LOGI("input type %d, viCnt %d, set default cfg path %s \n",
             pstProduct->enInputType, pstInit->stViInitInfoSt.uiViChn, szDefaultCfgPath);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : IA_GetScheHndl
* 描  述  : 返回调度器句柄
* 输  入  : 无:
* 输  出  : 无
* 返回值  : 调度句柄指针
*******************************************************************************/
void *IA_GetScheHndl(void)
{
    return scheduler_handle;
}

/*******************************************************************************
* 函数名  : Ia_GetXsiComMem
* 描  述  : 获取算法引擎使用的内存信息
* 输  入  : - mode: 模式
*         : - type: 类型
* 输  出  : 无
* 返回值  : 算法引擎使用的内存信息变量
*******************************************************************************/
static IA_MEM_INFO *Ia_GetXsiComMem(IA_MOD_IDX_E mode, IA_MEM_TYPE_E type)
{
    if (mode >= IA_MOD_MAX_NUM || type >= IA_MEM_TYPE_NUM)
    {
        SAL_LOGE("Mode %d >= Max %d or Mmz Type %d >= Max %d, Please check!\n",
                 mode, IA_MOD_MAX_NUM, type, IA_MEM_TYPE_NUM);
        return NULL;
    }

    if (IA_MOD_SVA == mode)
    {
        if (type >= IA_MEM_TYPE_NUM)
        {
            SAL_LOGE("Invalid Mem Type %d >= Max %d \n", type, IA_MEM_TYPE_NUM);
            return NULL;
        }

        return &gSvaMemInfo.stMemInfo[type];
    }
    else if (IA_MOD_BA == mode)
    {
        if (type >= IA_MEM_TYPE_NUM)
        {
            SAL_LOGE("Invalid Mem Type %d >= Max %d \n", type, IA_MEM_TYPE_NUM);
            return NULL;
        }

        return &gBaMemInfo.stMemInfo[type];
    }
    else if (IA_MOD_FACE == mode)
    {
        if (type >= IA_MEM_TYPE_NUM)
        {
            SAL_LOGE("Invalid Mem Type %d >= Max %d \n", type, IA_MEM_TYPE_NUM);
            return NULL;
        }

        return &gFaceMemInfo.stMemInfo[type];
    }
    else if (IA_MOD_PPM == mode)
    {
        if (type >= IA_MEM_TYPE_NUM)
        {
            SAL_LOGE("Invalid Mem Type %d >= Max %d \n", type, IA_MEM_TYPE_NUM);
            return NULL;
        }

        return &gPpmMemInfo.stMemInfo[type];
    }
    else
    {
        SAL_LOGE("Invalid Mode %d \n", mode);
        return NULL;
    }
}

/*******************************************************************************
* 函数名  : Ia_XsiMemFree
* 描  述  : 释放算法使用内存
* 输  入  : - mode: 内存所属智能模块
*         : - type: 内存类型
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 Ia_XsiMemFree(IA_MOD_IDX_E mode, IA_MEM_TYPE_E type)
{
    IA_MEM_INFO *pstXsiDevMem = NULL;

    pstXsiDevMem = Ia_GetXsiComMem(mode, type);
    if (NULL == pstXsiDevMem)
    {
        SAL_LOGE("get Xsi Local MemInfo Fail!\n");
        return SAL_FAIL;
    }

    if (0 == pstXsiDevMem->uiSize)
    {
        SAL_LOGW("size == 0, no need free. mode %d, type %d \n", mode, type);
        return SAL_SOK;
    }

    if (type <= IA_MALLOC)
    {
        free(pstXsiDevMem->stMemPrm.VirAddr);
    }
    else if (type < IA_NT_MEM_FEATURE)
    {
        if (SAL_SOK != mem_hal_vbFree((Ptr)pstXsiDevMem->stMemPrm.VirAddr, 
			                          "ia_common", 
			                          (CHAR *)gModMemName[mode][type], 
			                          pstXsiDevMem->stMemPrm.llBlkSize * pstXsiDevMem->stMemPrm.s32BlkCnt,
			                          pstXsiDevMem->stMemPrm.u64VbBlk,
			                          pstXsiDevMem->stMemPrm.u32PoolId))
        {
            SAL_LOGE("Free Mmz faield! Mode %d, type %d \n", mode, type);
            return SAL_FAIL;
        }
    }
    else if (type == IA_NT_MEM_FEATURE)
    {
        /* NT平台内存在具体智能模块内向底层注册的内存接口中处理，此处忽略 */
		SAL_LOGW("nt mem proc skip! \n");
    }
    else if (type == IA_RK_CACHE_CMA)
    {
        /* RK平台内存在具体智能模块内向底层注册的内存接口中处理，此处忽略 */
		SAL_LOGW("rk mem proc skip! \n");
    }
	else
	{
		SAL_LOGW("invalid mem type! %d \n", type);
	}

    pstXsiDevMem->uiMemMode = 0;
    pstXsiDevMem->uiMemType = 0;
    pstXsiDevMem->uiOffSet = 0;
    pstXsiDevMem->uiSize = 0;
	sal_memset_s(&pstXsiDevMem->stMemPrm, sizeof(IA_MEM_PRM_S), 0x00, sizeof(IA_MEM_PRM_S));
	
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Ia_ResetTypicalMem
* 描  述  : 重置特定智能子模块特定类型的算法内存
* 输  入  : - mode: 内存所属智能模块
*         : - type: 内存类型
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 Ia_ResetTypicalMem(IA_MOD_IDX_E mode, IA_MEM_TYPE_E type)
{
    IA_MEM_INFO *pstXsiDevMem = NULL;

    if (mode >= IA_MOD_MAX_NUM || type >= IA_MEM_TYPE_NUM)
    {
        SAL_LOGE("Mode %d >= Max %d or Mmz Type %d >= Max %d, Please check!\n",
                 mode, IA_MOD_MAX_NUM, type, IA_MEM_TYPE_NUM);
        return SAL_FAIL;
    }

    pstXsiDevMem = Ia_GetXsiComMem(mode, type);
    if (NULL == pstXsiDevMem)
    {
        SAL_LOGE("get Xsi Local MemInfo Fail! mode %d, type %d \n", mode, type);
        return SAL_FAIL;
    }

    /* 清空分配内存 */
    if (0 != pstXsiDevMem->uiSize)
    {
        sal_memset_s((void *)pstXsiDevMem->stMemPrm.VirAddr, pstXsiDevMem->uiSize, 0x00, pstXsiDevMem->uiSize);
    }

    pstXsiDevMem->uiOffSet = 0;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Ia_ResetModMem
* 描  述  : 重置特定智能子模块的内存
* 输  入  : - mode: 内存所属智能模块
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Ia_ResetModMem(IA_MOD_IDX_E mode)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    if (mode >= IA_MOD_MAX_NUM)
    {
        SAL_LOGE("Mode %d >= Max %d, Please check!\n", mode, IA_MOD_MAX_NUM);
        return SAL_FAIL;
    }

    for (i = 0; i < IA_MEM_TYPE_NUM; i++)
    {
        s32Ret = Ia_ResetTypicalMem(mode, i);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("Module %d reset mem %d failed! \n", mode, i);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Ia_GetXsiFreeMem
* 描  述  : 分配算法引擎内存
* 输  入  : - type        : 内存类型
*         : - uBufSize    : 内存块大小
*         : - pstMemBuffer: 内存指针
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Ia_GetXsiFreeMem(IA_MOD_IDX_E mode, IA_MEM_TYPE_E type, UINT32 uBufSize, IA_MEM_PRM_S *pstMemBuffer)
{
    IA_MEM_INFO *pstXsiDevMem = NULL;

    /* Input Args Checker */
    IA_CHECK_PRM(pstMemBuffer, SAL_FAIL);

    pstXsiDevMem = Ia_GetXsiComMem(mode, type);
    if (NULL == pstXsiDevMem)
    {
        SAL_LOGE("get Xsi Local MemInfo Fail!\n");
        return SAL_FAIL;
    }

    if (pstXsiDevMem->uiOffSet + uBufSize > pstXsiDevMem->uiSize)
    {
        SAL_LOGE("Not Enough Mem Zone for size %d! uiOffSet %d, total %d \n", uBufSize, pstXsiDevMem->uiOffSet, pstXsiDevMem->uiSize);
        return SAL_FAIL;
    }

    pstMemBuffer->PhyAddr = (PhysAddr)(pstXsiDevMem->stMemPrm.PhyAddr + pstXsiDevMem->uiOffSet);
    pstMemBuffer->VirAddr = (void *)(pstXsiDevMem->stMemPrm.VirAddr + pstXsiDevMem->uiOffSet);

    pstXsiDevMem->uiOffSet += uBufSize;

    return SAL_SOK;
}

/**
 * @function:   Ia_PrintMemInfo
 * @brief:      打印模块使用的内存信息
 * @param[in]:  IA_MOD_IDX_E mode
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ia_PrintMemInfo(IA_MOD_IDX_E mode)
{
    UINT32 i = 0;
	
    IA_MEM_INFO *pstModMem = NULL;

    if (mode >= IA_MOD_MAX_NUM)
    {
    	SAL_LOGE("invalid module id %d \n", mode);
		return SAL_FAIL;
    }

    SAL_LOGW("/*================================= Module[%d]MemInfo =================================*/\n", mode);
	for(i = 0; i < IA_MEM_TYPE_NUM; i++)
	{
		pstModMem = Ia_GetXsiComMem(mode, i);
		if (NULL == pstModMem)
		{
			SAL_LOGE("get Local MemInfo Fail! mod %d, type %d \n", mode, i);
			return SAL_FAIL;
		}

	    SAL_LOGW("Mem Type:%-10d\tMem Name:%-16s\tsize:%-16fMBTotal:%-16fMB\n", 
			      i, pstModMem->cName, 
			      pstModMem->uiOffSet / 1024.0 / 1024.0, 
			      pstModMem->uiSize / 1024.0 / 1024.0);
	}
    SAL_LOGW("/*================================= Module[%d]MemInfo =================================*/\n", mode);

    return SAL_SOK;
}

/**
 * @function:   Ia_MemFree
 * @brief:      智能模块内存释放
 * @param[in]:  IA_MOD_MAX_NUM enMod
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ia_MemFree(IA_MOD_IDX_E enMod)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    for (i = 0; i < IA_MEM_TYPE_NUM; i++)
    {
        s32Ret = Ia_XsiMemFree(enMod, i);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("Free Mem Failed! Mode %d, type %d \n", enMod, i);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   Ia_allocHisiVbRemapCacheMem
 * @brief      申请hisi平台remap vb cache
 * @param[in]  IA_MOD_IDX_E enMod  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ia_allocHisiVbRemapCacheMem(IA_MOD_IDX_E enMod)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 uBufSize = 0;

    IA_MEM_INFO *pstXsiDevMem = NULL;
	ALLOC_VB_INFO_S stAllocVbInfo = {0};

    pstXsiDevMem = Ia_GetXsiComMem(enMod, IA_HISI_VB_REMAP_CACHED);
    if (NULL == pstXsiDevMem)
    {
        SAL_LOGE("get Xsi Local MemInfo Fail!\n");
        return SAL_FAIL;
    }
	
    uBufSize = gInitMemSize[enMod][IA_HISI_VB_REMAP_CACHED];

    if (0 != uBufSize)
    {
        /* szl_todo: 待确认是否申请一个VB块可以满足原先4个VB块的功能 */
        s32Ret = mem_hal_vbAlloc(uBufSize, "ia_common", (CHAR *)gModMemName[enMod][IA_HISI_VB_REMAP_CACHED], NULL, SAL_FALSE, &stAllocVbInfo);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("malloc failed \n");
            return SAL_FAIL;
        }
    }

    pstXsiDevMem->uiMemMode = enMod;
    pstXsiDevMem->uiMemType = IA_HISI_VB_REMAP_CACHED;
    pstXsiDevMem->uiSize = uBufSize;
    pstXsiDevMem->uiOffSet = 0;
    pstXsiDevMem->stMemPrm.VirAddr = (char *)stAllocVbInfo.pVirAddr;
    pstXsiDevMem->stMemPrm.PhyAddr = (PhysAddr)stAllocVbInfo.u64PhysAddr;
	pstXsiDevMem->stMemPrm.u32PoolId = stAllocVbInfo.u32PoolId;
	pstXsiDevMem->stMemPrm.u64VbBlk = stAllocVbInfo.u64VbBlk;
	pstXsiDevMem->stMemPrm.s32BlkCnt = 1;
	pstXsiDevMem->stMemPrm.llBlkSize = stAllocVbInfo.u32Size;
	pstXsiDevMem->cName = gModMemName[enMod][IA_HISI_VB_REMAP_CACHED];

	return SAL_SOK;
}

/**
 * @function   Ia_allocHisiVbRemapNoCacheMem
 * @brief      申请hisi平台remap vb noCache
 * @param[in]  IA_MOD_IDX_E enMod  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ia_allocHisiVbRemapNoCacheMem(IA_MOD_IDX_E enMod)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 uBufSize = 0;

    IA_MEM_INFO *pstXsiDevMem = NULL;
	ALLOC_VB_INFO_S stAllocVbInfo = {0};

    pstXsiDevMem = Ia_GetXsiComMem(enMod, IA_HISI_VB_REMAP_NO_CACHED);
    if (NULL == pstXsiDevMem)
    {
        SAL_LOGE("get Xsi Local MemInfo Fail!\n");
        return SAL_FAIL;
    }

    uBufSize = gInitMemSize[enMod][IA_HISI_VB_REMAP_NO_CACHED];

    if (0 != uBufSize)
    {
        /* szl_todo: 待确认是否申请一个VB块可以满足原先4个VB块的功能 */
        s32Ret = mem_hal_vbAlloc(uBufSize, "ia_common", (CHAR *)gModMemName[enMod][IA_HISI_VB_REMAP_NO_CACHED], NULL, SAL_FALSE, &stAllocVbInfo);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("malloc failed \n");
            return SAL_FAIL;
        }
    }

    pstXsiDevMem->uiMemMode = enMod;
    pstXsiDevMem->uiMemType = IA_HISI_VB_REMAP_NO_CACHED;
    pstXsiDevMem->uiSize = uBufSize;
    pstXsiDevMem->uiOffSet = 0;
    pstXsiDevMem->stMemPrm.VirAddr = (char *)stAllocVbInfo.pVirAddr;
    pstXsiDevMem->stMemPrm.PhyAddr = (PhysAddr)stAllocVbInfo.u64PhysAddr;
	pstXsiDevMem->stMemPrm.u32PoolId = stAllocVbInfo.u32PoolId;
	pstXsiDevMem->stMemPrm.u64VbBlk = stAllocVbInfo.u64VbBlk;
	pstXsiDevMem->stMemPrm.s32BlkCnt = 1;
	pstXsiDevMem->stMemPrm.llBlkSize = stAllocVbInfo.u32Size;
	pstXsiDevMem->cName = gModMemName[enMod][IA_HISI_VB_REMAP_NO_CACHED];	

	return SAL_SOK;
}

/**
 * @function   Ia_allocHisiVbRemapNoneMem
 * @brief      申请hisi平台remap vb none
 * @param[in]  IA_MOD_IDX_E enMod  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ia_allocHisiVbRemapNoneMem(IA_MOD_IDX_E enMod)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 uBufSize = 0;

    IA_MEM_INFO *pstXsiDevMem = NULL;
	ALLOC_VB_INFO_S stAllocVbInfo = {0};

	pstXsiDevMem = Ia_GetXsiComMem(enMod, IA_HISI_VB_REMAP_NONE);
    if (NULL == pstXsiDevMem)
    {
        SAL_LOGE("get Xsi Local MemInfo Fail!\n");
        return SAL_FAIL;
    }

    uBufSize = gInitMemSize[enMod][IA_HISI_VB_REMAP_NONE];

    if (0 != uBufSize)
    {
        /* szl_todo: 待确认是否申请一个VB块可以满足原先4个VB块的功能 */
        s32Ret = mem_hal_vbAlloc(uBufSize, "ia_common", (CHAR *)gModMemName[enMod][IA_HISI_VB_REMAP_NONE], NULL, SAL_FALSE, &stAllocVbInfo);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("malloc failed \n");
            return SAL_FAIL;
        }
    }

    pstXsiDevMem->uiMemMode = enMod;
    pstXsiDevMem->uiMemType = IA_HISI_VB_REMAP_NONE;
    pstXsiDevMem->uiSize = uBufSize;
    pstXsiDevMem->uiOffSet = 0;
    pstXsiDevMem->stMemPrm.VirAddr = (char *)stAllocVbInfo.pVirAddr;
    pstXsiDevMem->stMemPrm.PhyAddr = (PhysAddr)stAllocVbInfo.u64PhysAddr;
	pstXsiDevMem->stMemPrm.u32PoolId = stAllocVbInfo.u32PoolId;
	pstXsiDevMem->stMemPrm.u64VbBlk = stAllocVbInfo.u64VbBlk;
	pstXsiDevMem->stMemPrm.s32BlkCnt = 1;
	pstXsiDevMem->stMemPrm.llBlkSize = stAllocVbInfo.u32Size;
	pstXsiDevMem->cName = gModMemName[enMod][IA_HISI_VB_REMAP_NONE];	

	return SAL_SOK;
}

/**
 * @function   Ia_allocHisiCacheNoPrivMem
 * @brief      申请hisi平台cache no priv mmz
 * @param[in]  IA_MOD_IDX_E enMod  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ia_allocHisiCacheNoPrivMem(IA_MOD_IDX_E enMod)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 uBufSize = 0;

    IA_MEM_INFO *pstXsiDevMem = NULL;
	ALLOC_VB_INFO_S stAllocVbInfo = {0};

	pstXsiDevMem = Ia_GetXsiComMem(enMod, IA_HISI_MMZ_CACHE_NO_PRIORITY);
    if (NULL == pstXsiDevMem)
    {
        SAL_LOGE("get Xsi Local MemInfo Fail!\n");
        return SAL_FAIL;
    }

    uBufSize = gInitMemSize[enMod][IA_HISI_MMZ_CACHE_NO_PRIORITY];
    if (0 != uBufSize)
    {
		s32Ret = mem_hal_vbAlloc(uBufSize,
			                     "ia_common",
			                     (CHAR *)gModMemName[enMod][IA_HISI_MMZ_CACHE_NO_PRIORITY],
			                     NULL,
			                     SAL_FALSE,
			                     &stAllocVbInfo);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("Alloc Mem failed! mode %d, type %d \n", enMod, IA_HISI_MMZ_CACHE_NO_PRIORITY);
            return SAL_FAIL;
        }

        /* 清空分配内存 */
        sal_memset_s((void *)stAllocVbInfo.pVirAddr, uBufSize, 0, uBufSize);
    }

    pstXsiDevMem->uiMemMode = enMod;
    pstXsiDevMem->uiMemType = IA_HISI_MMZ_CACHE_NO_PRIORITY;
    pstXsiDevMem->uiSize = uBufSize;
    pstXsiDevMem->uiOffSet = 0;
	pstXsiDevMem->stMemPrm.u64VbBlk = stAllocVbInfo.u64VbBlk;
	pstXsiDevMem->stMemPrm.u32PoolId = stAllocVbInfo.u32PoolId;
    pstXsiDevMem->stMemPrm.VirAddr = (char *)stAllocVbInfo.pVirAddr;
    pstXsiDevMem->stMemPrm.PhyAddr = stAllocVbInfo.u64PhysAddr;
	pstXsiDevMem->stMemPrm.llBlkSize = stAllocVbInfo.u32Size;
	pstXsiDevMem->cName = gModMemName[enMod][IA_HISI_MMZ_CACHE_NO_PRIORITY];	

	return SAL_SOK;
}

/**
 * @function   Ia_allocHisiCachePrivMem
 * @brief      申请hisi平台cache priv mmz
 * @param[in]  IA_MOD_IDX_E enMod  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ia_allocHisiCachePrivMem(IA_MOD_IDX_E enMod)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 uBufSize = 0;

    IA_MEM_INFO *pstXsiDevMem = NULL;
	ALLOC_VB_INFO_S stAllocVbInfo = {0};

    pstXsiDevMem = Ia_GetXsiComMem(enMod, IA_HISI_MMZ_CACHE_PRIORITY);
    if (NULL == pstXsiDevMem)
    {
        SAL_LOGE("get Xsi Local MemInfo Fail!\n");
        return SAL_FAIL;
    }

    uBufSize = gInitMemSize[enMod][IA_HISI_MMZ_CACHE_PRIORITY];
    if (0 != uBufSize)
    {
		s32Ret = mem_hal_vbAlloc(uBufSize,
			                     "ia_common",
			                     (CHAR *)gModMemName[enMod][IA_HISI_MMZ_CACHE_PRIORITY],
			                     NULL,
			                     SAL_TRUE,
			                     &stAllocVbInfo);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("Alloc Mem failed! mode %d, type %d \n", enMod, IA_HISI_MMZ_CACHE_PRIORITY);
            return SAL_FAIL;
        }

        /* 清空分配内存 */
        sal_memset_s((void *)stAllocVbInfo.pVirAddr, uBufSize, 0, uBufSize);
    }

    pstXsiDevMem->uiMemMode = enMod;
    pstXsiDevMem->uiMemType = IA_HISI_MMZ_CACHE_PRIORITY;
    pstXsiDevMem->uiSize = uBufSize;
    pstXsiDevMem->uiOffSet = 0;
	pstXsiDevMem->stMemPrm.u64VbBlk = stAllocVbInfo.u64VbBlk;
	pstXsiDevMem->stMemPrm.u32PoolId = stAllocVbInfo.u32PoolId;
    pstXsiDevMem->stMemPrm.VirAddr = (char *)stAllocVbInfo.pVirAddr;
    pstXsiDevMem->stMemPrm.PhyAddr = stAllocVbInfo.u64PhysAddr;
	pstXsiDevMem->stMemPrm.llBlkSize = stAllocVbInfo.u32Size;
	pstXsiDevMem->cName = gModMemName[enMod][IA_HISI_MMZ_CACHE_PRIORITY];

	return SAL_SOK;
}

/**
 * @function   Ia_allocHisiNoCacheMem
 * @brief      申请hisi平台nocache mmz
 * @param[in]  IA_MOD_IDX_E enMod  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ia_allocHisiNoCacheMem(IA_MOD_IDX_E enMod)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 uBufSize = 0;

    IA_MEM_INFO *pstXsiDevMem = NULL;
	ALLOC_VB_INFO_S stAllocVbInfo = {0};

    pstXsiDevMem = Ia_GetXsiComMem(enMod, IA_HISI_MMZ_NO_CACHE);
    if (NULL == pstXsiDevMem)
    {
        SAL_LOGE("get Xsi Local MemInfo Fail!\n");
        return SAL_FAIL;
    }

    uBufSize = gInitMemSize[enMod][IA_HISI_MMZ_NO_CACHE];
    if (0 != uBufSize)
    {
		s32Ret = mem_hal_vbAlloc(uBufSize,
			                     "ia_common",
			                     (CHAR *)gModMemName[enMod][IA_HISI_MMZ_NO_CACHE],
			                     NULL,
			                     SAL_FALSE,
			                     &stAllocVbInfo);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("Alloc Mem failed! mode %d, type %d \n", enMod, IA_HISI_MMZ_NO_CACHE);
            return SAL_FAIL;
        }

        /* 清空分配内存 */
        sal_memset_s((void *)stAllocVbInfo.pVirAddr, uBufSize, 0, uBufSize);
    }

    pstXsiDevMem->uiMemMode = enMod;
    pstXsiDevMem->uiMemType = IA_HISI_MMZ_NO_CACHE;
    pstXsiDevMem->uiSize = uBufSize;
    pstXsiDevMem->uiOffSet = 0;
	pstXsiDevMem->stMemPrm.u64VbBlk = stAllocVbInfo.u64VbBlk;
	pstXsiDevMem->stMemPrm.u32PoolId = stAllocVbInfo.u32PoolId;
    pstXsiDevMem->stMemPrm.VirAddr = (char *)stAllocVbInfo.pVirAddr;
    pstXsiDevMem->stMemPrm.PhyAddr = stAllocVbInfo.u64PhysAddr;
	pstXsiDevMem->stMemPrm.llBlkSize = stAllocVbInfo.u32Size;
	pstXsiDevMem->cName = gModMemName[enMod][IA_HISI_MMZ_NO_CACHE];	

	return SAL_SOK;
}

/**
 * @function   Ia_allocHisiCacheMem
 * @brief      申请hisi平台cache mmz
 * @param[in]  IA_MOD_IDX_E enMod  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ia_allocHisiCacheMem(IA_MOD_IDX_E enMod)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 uBufSize = 0;

    IA_MEM_INFO *pstXsiDevMem = NULL;
	ALLOC_VB_INFO_S stAllocVbInfo = {0};

    pstXsiDevMem = Ia_GetXsiComMem(enMod, IA_HISI_MMZ_CACHE);
    if (NULL == pstXsiDevMem)
    {
        SAL_LOGE("get Xsi Local MemInfo Fail!\n");
        return SAL_FAIL;
    }

    uBufSize = gInitMemSize[enMod][IA_HISI_MMZ_CACHE];
    if (0 != uBufSize)
    {
		s32Ret = mem_hal_vbAlloc(uBufSize,
			                     "ia_common",
			                     (CHAR *)gModMemName[enMod][IA_HISI_MMZ_CACHE],
			                     NULL,
			                     SAL_TRUE,
			                     &stAllocVbInfo);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("Alloc Mem failed! mode %d, type %d \n", enMod, IA_HISI_MMZ_CACHE);
            return SAL_FAIL;
        }

        /* 清空分配内存 */
        sal_memset_s((void *)stAllocVbInfo.pVirAddr, uBufSize, 0, uBufSize);
    }

    pstXsiDevMem->uiMemMode = enMod;
    pstXsiDevMem->uiMemType = IA_HISI_MMZ_CACHE;
    pstXsiDevMem->uiSize = uBufSize;
    pstXsiDevMem->uiOffSet = 0;
	pstXsiDevMem->stMemPrm.u64VbBlk = stAllocVbInfo.u64VbBlk;
	pstXsiDevMem->stMemPrm.u32PoolId = stAllocVbInfo.u32PoolId;
    pstXsiDevMem->stMemPrm.VirAddr = (char *)stAllocVbInfo.pVirAddr;
    pstXsiDevMem->stMemPrm.PhyAddr = stAllocVbInfo.u64PhysAddr;
	pstXsiDevMem->stMemPrm.llBlkSize = stAllocVbInfo.u32Size;
	pstXsiDevMem->cName = gModMemName[enMod][IA_HISI_MMZ_CACHE];	

	return SAL_SOK;
}

/**
 * @function   Ia_allocSysMem
 * @brief      申请系统内存
 * @param[in]  IA_MOD_IDX_E enMod  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ia_allocSysMem(IA_MOD_IDX_E enMod)
{
    UINT32 uBufSize = 0;

    void *pVirAddr = NULL;
    IA_MEM_INFO *pstXsiDevMem = NULL;
	
	pstXsiDevMem = Ia_GetXsiComMem(enMod, IA_MALLOC);
    if (NULL == pstXsiDevMem)
    {
        SAL_LOGE("get Xsi Local MemInfo Fail!\n");
        return SAL_FAIL;
    }

    pVirAddr = NULL;
    uBufSize = gInitMemSize[enMod][IA_MALLOC];

    if (0 != uBufSize)
    {
        pVirAddr = SAL_memZalloc(uBufSize, "ia_common", (CHAR *)gModMemName[enMod][IA_MALLOC]);//(void *)malloc(uBufSize);
        if (NULL == pVirAddr)
        {
            SAL_LOGE("malloc failed \n");
            return SAL_FAIL;
        }
    }

    pstXsiDevMem->uiMemMode = enMod;
    pstXsiDevMem->uiMemType = IA_MALLOC;
    pstXsiDevMem->uiSize = uBufSize;
    pstXsiDevMem->uiOffSet = 0;
    pstXsiDevMem->stMemPrm.VirAddr = (char *)pVirAddr;
    pstXsiDevMem->stMemPrm.PhyAddr = (PhysAddr)pVirAddr;
	pstXsiDevMem->cName = gModMemName[enMod][IA_MALLOC];	

	return SAL_SOK;
}

/**
 * @function   Ia_MemAllocHisi
 * @brief      申请hisi平台智能模块使用的内存块
 * @param[in]  IA_MOD_IDX_E enMod  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ia_MemAllocHisi(IA_MOD_IDX_E enMod)
{
    INT32 s32Ret = SAL_SOK;

    /* 1. Alloc Mmz with Cache */
	s32Ret = Ia_allocHisiCacheMem(enMod);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("Ia_allocHisiCacheMem failed! mod %d \n", enMod);
		return SAL_FAIL;
	}

    /* 2. Alloc Mmz with No Cache */
	s32Ret = Ia_allocHisiNoCacheMem(enMod);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("Ia_allocHisiNoCacheMem failed! mod %d \n", enMod);
		return SAL_FAIL;
	}

    /* 3. Alloc Mmz with Priority Cache */
	s32Ret = Ia_allocHisiCachePrivMem(enMod);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("Ia_allocHisiCachePrivMem failed! mod %d \n", enMod);
		return SAL_FAIL;
	}

    /* 4. Alloc Mmz with No Priority Cache */
	s32Ret = Ia_allocHisiCacheNoPrivMem(enMod);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("Ia_allocHisiCacheNoPrivMem failed! mod %d \n", enMod);
		return SAL_FAIL;
	}

    /* 5.申请VB remap none */
	s32Ret = Ia_allocHisiVbRemapNoneMem(enMod);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("Ia_allocHisiVbRemapNoneMem failed! mod %d \n", enMod);
		return SAL_FAIL;
	}

    /* 6.申请VB remap no cached */
	s32Ret = Ia_allocHisiVbRemapNoCacheMem(enMod);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("Ia_allocHisiVbRemapNoneMem failed! mod %d \n", enMod);
		return SAL_FAIL;
	}

    /* 7.申请VB remap cached */
	s32Ret = Ia_allocHisiVbRemapCacheMem(enMod);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("Ia_allocHisiVbRemapCacheMem failed! mod %d \n", enMod);
		return SAL_FAIL;
	}

	return SAL_SOK;
}

/**
 * @function   Ia_MemAllocNt
 * @brief      申请nt平台智能模块使用的内存块
 * @param[in]  IA_MOD_IDX_E enMod  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ia_MemAllocNt(IA_MOD_IDX_E enMod)
{
	/* nt内存由各智能模块向底层注册的内存申请释放接口动态处理，无需提前申请 */
	return SAL_SOK;
}

/**
 * @function   Ia_allocRkCacheCmaMem
 * @brief      申请rk平台cache cma mem
 * @param[in]  IA_MOD_IDX_E enMod  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ia_allocRkCacheCmaMem(IA_MOD_IDX_E enMod)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 uBufSize = 0;

    IA_MEM_INFO *pstXsiDevMem = NULL;
	ALLOC_VB_INFO_S stAllocVbInfo = {0};

    pstXsiDevMem = Ia_GetXsiComMem(enMod, IA_RK_CACHE_CMA);
    if (NULL == pstXsiDevMem)
    {
        SAL_LOGE("get Xsi Local MemInfo Fail!\n");
        return SAL_FAIL;
    }
	
    uBufSize = gInitMemSize[enMod][IA_RK_CACHE_CMA];

    if (0 != uBufSize)
    {
        /* szl_todo: 待确认是否申请一个VB块可以满足原先4个VB块的功能 */
        s32Ret = mem_hal_vbAlloc(uBufSize, "ia_common", (CHAR *)gModMemName[enMod][IA_RK_CACHE_CMA], NULL, SAL_FALSE, &stAllocVbInfo);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("malloc failed \n");
            return SAL_FAIL;
        }
    }

    pstXsiDevMem->uiMemMode = enMod;
    pstXsiDevMem->uiMemType = IA_RK_CACHE_CMA;
    pstXsiDevMem->uiSize = uBufSize;
    pstXsiDevMem->uiOffSet = 0;
    pstXsiDevMem->stMemPrm.VirAddr = (char *)stAllocVbInfo.pVirAddr;
    pstXsiDevMem->stMemPrm.PhyAddr = (PhysAddr)stAllocVbInfo.u64PhysAddr;
	pstXsiDevMem->stMemPrm.u32PoolId = stAllocVbInfo.u32PoolId;
	pstXsiDevMem->stMemPrm.u64VbBlk = stAllocVbInfo.u64VbBlk;
	pstXsiDevMem->stMemPrm.s32BlkCnt = 1;
	pstXsiDevMem->stMemPrm.llBlkSize = stAllocVbInfo.u32Size;
	pstXsiDevMem->cName = gModMemName[enMod][IA_RK_CACHE_CMA];

	return SAL_SOK;	
}

/**
 * @function   Ia_allocRkCacheIommuMem
 * @brief      申请rk平台cache iommu mem
 * @param[in]  IA_MOD_IDX_E enMod  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ia_allocRkCacheIommuMem(IA_MOD_IDX_E enMod)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 uBufSize = 0;

    IA_MEM_INFO *pstXsiDevMem = NULL;
	ALLOC_VB_INFO_S stAllocVbInfo = {0};

    pstXsiDevMem = Ia_GetXsiComMem(enMod, IA_RK_CACHE_IOMMU);
    if (NULL == pstXsiDevMem)
    {
        SAL_LOGE("get Xsi Local MemInfo Fail!\n");
        return SAL_FAIL;
    }
	
    uBufSize = gInitMemSize[enMod][IA_RK_CACHE_IOMMU];

    if (0 != uBufSize)
    {
        /* szl_todo: 待确认是否申请一个VB块可以满足原先4个VB块的功能 */
        s32Ret = mem_hal_vbAlloc(uBufSize, "ia_common", (CHAR *)gModMemName[enMod][IA_RK_CACHE_IOMMU], NULL, SAL_FALSE, &stAllocVbInfo);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("malloc failed \n");
            return SAL_FAIL;
        }
    }

    pstXsiDevMem->uiMemMode = enMod;
    pstXsiDevMem->uiMemType = IA_RK_CACHE_IOMMU;
    pstXsiDevMem->uiSize = uBufSize;
    pstXsiDevMem->uiOffSet = 0;
    pstXsiDevMem->stMemPrm.VirAddr = (char *)stAllocVbInfo.pVirAddr;
    pstXsiDevMem->stMemPrm.PhyAddr = (PhysAddr)stAllocVbInfo.u64PhysAddr;
	pstXsiDevMem->stMemPrm.u32PoolId = stAllocVbInfo.u32PoolId;
	pstXsiDevMem->stMemPrm.u64VbBlk = stAllocVbInfo.u64VbBlk;
	pstXsiDevMem->stMemPrm.s32BlkCnt = 1;
	pstXsiDevMem->stMemPrm.llBlkSize = stAllocVbInfo.u32Size;
	pstXsiDevMem->cName = gModMemName[enMod][IA_RK_CACHE_IOMMU];

	return SAL_SOK;	
}

/**
 * @function   Ia_MemAllocRk
 * @brief      申请rk平台智能模块使用的内存块
 * @param[in]  IA_MOD_IDX_E enMod  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ia_MemAllocRk(IA_MOD_IDX_E enMod)
{
    INT32 s32Ret = SAL_FAIL;

	s32Ret = Ia_allocRkCacheCmaMem(enMod);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("Ia_allocRkCacheCmaMem failed! mod %d \n", enMod);
		return SAL_FAIL;
	}

	s32Ret = Ia_allocRkCacheIommuMem(enMod);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("Ia_allocRkCacheIommuMem failed! mod %d \n", enMod);
		return SAL_FAIL;
	}

	return SAL_SOK;
}

/**
 * @function:   Ia_MemAlloc
 * @brief:      智能模块内存申请
 * @param[in]:  IA_MOD_MAX_NUM enMod
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ia_MemAlloc(IA_MOD_IDX_E enMod)
{
    INT32 s32Ret = SAL_SOK;

	/* 申请系统内存 */
	s32Ret = Ia_allocSysMem(enMod);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("Ia_allocSysMem failed! mod %d \n", enMod);
		return SAL_FAIL;
	}

	/* 申请hisi平台相关内存 */
	s32Ret = Ia_MemAllocHisi(enMod);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("Ia_MemAllocHisi failed! mod %d \n", enMod);
		return SAL_FAIL;
	}

	/* 申请nt平台相关内存 */
	s32Ret = Ia_MemAllocNt(enMod);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("Ia_MemAllocNt failed! mod %d \n", enMod);
		return SAL_FAIL;
	}

	/* 申请rk平台相关内存，当前仅有人脸使用 */
	s32Ret = Ia_MemAllocRk(enMod);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("Ia_MemAllocRk failed! mod %d \n", enMod);
		return SAL_FAIL;
	}

    SAL_LOGI("ia Mem Init End! module %d \n", enMod);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Ia_MemDeInit
* 描  述  : 算法引擎内存去初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 注意点  : 算法引擎对内存起始地址有要求，需要在3.2G以内
*******************************************************************************/
INT32 Ia_MemDeInit(void)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    for (i = 0; i < IA_MOD_MAX_NUM; i++)
    {
        s32Ret = Ia_MemFree(i);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("Mod %d: Mem Alloc Failed! \n", i);
            return SAL_FAIL;
        }
    }

#if 0 /* 暂时不需要使用锁 */
    if (SAL_SOK != SAL_mutexDelete((void *)&pstXsiDevMem->mutex))
    {
        SAL_LOGE("Mutex Create Fail!\n");
        return SAL_FAIL;
    }

#endif

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : IA_SetModMemInitSize
* 描  述  : 初始化模块内存大小
* 输  入  : mode: 模块
* 输  出  : 无
* 返回值  : SAL_SOK : 成功
*         : SAL_FAIL: 失败
* 注意点  : 当前在初始化根据设备型号进行区分
*******************************************************************************/
static INT32 IA_SetModMemInitSize(IA_MOD_IDX_E mode, UINT32 uiInputCnt)
{
    UINT32 uiCpySize = 0;

    if (uiInputCnt > INPUT_MAX_NUM || uiInputCnt == 0)
    {
        SAL_LOGE("Invalid input cnt %d, pls check! \n", uiInputCnt);
        return SAL_FAIL;
    }

    uiCpySize = IA_MEM_TYPE_NUM * sizeof(UINT32);

    sal_memcpy_s(gInitMemSize[mode], uiCpySize, gMemSizeTab[mode][uiInputCnt - 1], uiCpySize);

#if 0
    SAL_INFO("%s: mode %d, dev type %d \n", __FUNCTION__, mode, enDevType);
    for (int i = 0; i < IA_MEM_TYPE_NUM; i++)
    {
        SAL_ERROR("!!!!set!!!!mode %d, type %d, size %d \n", mode, i, gInitMemSize[mode][i]);
    }

#endif
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : IA_SetMemInitSize
* 描  述  : 初始化智能相关内存大小
* 输  入  : enDevType: 设备类型(根据设备输入数量进行区分)
* 输  出  : 无
* 返回值  : SAL_SOK : 成功
*         : SAL_FAIL: 失败
* 注意点  : 当前在初始化根据设备型号进行区分
*******************************************************************************/
static INT32 IA_SetMemInitSize(UINT32 uiInputCnt)
{
    UINT32 i = 0;

    for (i = 0; i < IA_MOD_MAX_NUM; i++)
    {
        if (SAL_TRUE != IA_GetAlgRunFlag(i))
        {
            SAL_LOGI("ia module %d not enable! \n", i);
            continue;
        }

        (VOID)IA_SetModMemInitSize(i, uiInputCnt);
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Ia_MemInit
* 描  述  : 算法引擎内存初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 注意点  : 算法引擎对内存起始地址有要求，需要在3.2G以内
*******************************************************************************/
INT32 Ia_MemInit(void)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    DSPINITPARA *pDspInitPara = SystemPrm_getDspInitPara();

    if (SAL_SOK != IA_SetMemInitSize(pDspInitPara->stViInitInfoSt.uiViChn))
    {
        SYS_LOGE("IA_SetMemInitSize err !!!\n");
        return SAL_FAIL;
    }

    for (i = 0; i < IA_MOD_MAX_NUM; i++)
    {
    	/* 若智能模块未使用，则不申请相关内存 */
		if (SAL_TRUE != IA_GetModEnableFlag(i))
		{
			SAL_LOGW("mod %d not use, skip alloc mem! \n", i);
			continue;
		}

        s32Ret = Ia_MemAlloc(i);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("Mod %d: Mem Alloc Failed! \n", i);
            return SAL_FAIL;
        }
    }

#if 0 /* 暂时不需要使用锁 */
    if (SAL_SOK != SAL_mutexCreate(SAL_MUTEX_NORMAL, (void *)&pstXsiDevMem->mutex))
    {
        SAL_LOGE("Mutex Create Fail!\n");
        return SAL_FAIL;
    }

#endif

    SAL_LOGI("Xsi Mem Init End! \n");
    return SAL_SOK;
}

/**
 * @function:   IA_PrPpmInitMapInfo
 * @brief:      打印ppm模块初始化信息
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     VOID
 */
static VOID IA_PrPpmInitMapInfo()
{
    DSPINITPARA *pDspInitPara = SystemPrm_getDspInitPara();

    SAL_LOGI("PPM: enable[%d], trans[%d], chip[%d] \n",
             pDspInitPara->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_PPM].uiEnableFlag,
             pDspInitPara->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_PPM].stIaModTransPrm.uiDualChipTransFlag,
             pDspInitPara->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_PPM].stIaModTransPrm.uiRunChipId);
    return;
}

/**
 * @function:   IA_PrFaceInitMapInfo
 * @brief:      打印face模块初始化信息
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     VOID
 */
static VOID IA_PrFaceInitMapInfo()
{
    DSPINITPARA *pDspInitPara = SystemPrm_getDspInitPara();

    SAL_LOGI("FACE: enable[%d], trans[%d], chip[%d] \n",
             pDspInitPara->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_FACE].uiEnableFlag,
             pDspInitPara->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_FACE].stIaModTransPrm.uiDualChipTransFlag,
             pDspInitPara->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_FACE].stIaModTransPrm.uiRunChipId);
    return;
}

/**
 * @function:   IA_PrBaInitMapInfo
 * @brief:      打印ba模块初始化信息
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     VOID
 */
static VOID IA_PrBaInitMapInfo()
{
    DSPINITPARA *pDspInitPara = SystemPrm_getDspInitPara();

    SAL_LOGI("BA: enable[%d], trans[%d], chip[%d] \n",
             pDspInitPara->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_BA].uiEnableFlag,
             pDspInitPara->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_BA].stIaModTransPrm.uiDualChipTransFlag,
             pDspInitPara->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_BA].stIaModTransPrm.uiRunChipId);
    return;
}

/**
 * @function:   IA_PrSvaInitMapInfo
 * @brief:      打印sva模块初始化信息
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     VOID
 */
static VOID IA_PrSvaInitMapInfo(VOID)
{
    DSPINITPARA *pDspInitPara = SystemPrm_getDspInitPara();

    SAL_LOGI("SVA: enable[%d], trans[%d], chip[%d] \n",
             pDspInitPara->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_SVA].uiEnableFlag,
             pDspInitPara->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_SVA].stIaModTransPrm.uiDualChipTransFlag,
             pDspInitPara->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_SVA].stIaModTransPrm.uiRunChipId);
    return;
}

/**
 * @function:   IA_PrIaInitMapInfo
 * @brief:      打印智能模块初始化映射信息
 * @param[in]:  IA_MOD_IDX_E enModIdx
 * @param[out]: None
 * @return:     VOID
 */
VOID IA_PrIaInitMapInfo(IA_MOD_IDX_E enModIdx)
{
    switch (enModIdx)
    {
        case IA_MOD_SVA:
        {
            IA_PrSvaInitMapInfo();
            break;
        }
        case IA_MOD_BA:
        {
            IA_PrBaInitMapInfo();
            break;
        }
        case IA_MOD_FACE:
        {
            IA_PrFaceInitMapInfo();
            break;
        }
        case IA_MOD_PPM:
        {
            IA_PrPpmInitMapInfo();
            break;
        }
        default:
        {
            SAL_LOGE("invalid ia module idx %d \n", enModIdx);
            break;
        }
    }

    return;
}

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
                      UINT32 w, UINT32 h, UINT32 bCached)
{
    INT32                   s32Ret      = 0;
    TDE_HAL_SURFACE          srcSurface, dstSurface;
    TDE_HAL_RECT             srcRect, dstRect;
	SAL_VideoFrameBuf       stSrcVideoFrmBuf = {0};
	SAL_VideoFrameBuf       stDstVideoFrmBuf = {0};

    (VOID)sys_hal_getVideoFrameInfo(pSrcSysFrame, &stSrcVideoFrmBuf);
    (VOID)sys_hal_getVideoFrameInfo(pDstSysFrame, &stDstVideoFrmBuf);
	
    memset(&srcSurface, 0, sizeof(TDE_HAL_SURFACE));
    memset(&dstSurface, 0, sizeof(TDE_HAL_SURFACE));
    memset(&srcRect, 0, sizeof(TDE_HAL_RECT));
    memset(&dstRect, 0, sizeof(TDE_HAL_RECT));

    srcSurface.u32Width   = stSrcVideoFrmBuf.frameParam.width;
    srcSurface.u32Height  = stSrcVideoFrmBuf.frameParam.height;
    srcSurface.u32Stride  = stSrcVideoFrmBuf.stride[0];
    srcSurface.PhyAddr = stSrcVideoFrmBuf.phyAddr[0];
	srcSurface.enColorFmt = stSrcVideoFrmBuf.frameParam.dataFormat;
	srcSurface.vbBlk = stSrcVideoFrmBuf.vbBlk;
    srcRect.s32Xpos = x;
    srcRect.s32Ypos = y;
    srcRect.u32Width = w;
    srcRect.u32Height = h;

    dstSurface.u32Width   = stDstVideoFrmBuf.frameParam.width;
    dstSurface.u32Height  = stDstVideoFrmBuf.frameParam.height;
    dstSurface.u32Stride  = stDstVideoFrmBuf.stride[0];
    dstSurface.PhyAddr = stDstVideoFrmBuf.phyAddr[0];
	dstSurface.enColorFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
	dstSurface.vbBlk = stDstVideoFrmBuf.vbBlk;
    dstRect.s32Xpos   = 0;
    dstRect.s32Ypos   = 0;
    dstRect.u32Width  = w;
    dstRect.u32Height = h;

    SAL_LOGD("TDE Src:enColorFmt %d, w %d, h %d, rect[%d, %d, %d, %d] \n", srcSurface.enColorFmt, srcSurface.u32Width, srcSurface.u32Height, 
                                           srcRect.s32Xpos, srcRect.s32Ypos, srcRect.u32Width, srcRect.u32Height);
    SAL_LOGD("TDE Dst:enColorFmt %d, w %d, h %d, rect[%d, %d, %d, %d] \n", dstSurface.enColorFmt, dstSurface.u32Width, dstSurface.u32Height, 
                                           dstRect.s32Xpos, dstRect.s32Ypos, dstRect.u32Width, dstRect.u32Height);

	s32Ret = tde_hal_QuickCopy(&srcSurface, &srcRect, &dstSurface, &dstRect, bCached);
    if(s32Ret != SAL_SOK)
    {
        SAL_LOGE("err %#x\n", s32Ret);
        return SAL_FAIL;
    }

    /* 编码器需要时间戳信息 */
    stDstVideoFrmBuf.frameNum      = stSrcVideoFrmBuf.frameNum;
    stDstVideoFrmBuf.pts           = stSrcVideoFrmBuf.pts; 
//    stDstVideoFrmBuf.frameParam.width        = w;
//    stDstVideoFrmBuf.frameParam.height       = h;
	stDstVideoFrmBuf.frameParam.dataFormat = stSrcVideoFrmBuf.frameParam.dataFormat;
//    stDstVideoFrmBuf.stride[0]    = w;
//	stDstVideoFrmBuf.stride[1]    = w;
//	stDstVideoFrmBuf.stride[2]    = w;

    s32Ret = sys_hal_buildVideoFrame(&stDstVideoFrmBuf, pDstSysFrame);
	if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("err %#x\n", s32Ret);
        return SAL_FAIL;
    }   
	
    pDstSysFrame->uiDataWidth           = w;
    pDstSysFrame->uiDataHeight          = h;

    return SAL_SOK;
}
INT32 Ia_TdeQuickCopyTmp(SYSTEM_FRAME_INFO *pSrcSysFrame, SYSTEM_FRAME_INFO *pDstSysFrame,
                      UINT32 x, UINT32 y,
                      UINT32 uiDstX, UINT32 uiDstY, UINT32 w, UINT32 h, UINT32 bCached)
{
  INT32                   s32Ret      = 0;
  TDE_HAL_SURFACE          srcSurface, dstSurface;
  TDE_HAL_RECT             srcRect, dstRect;
  SAL_VideoFrameBuf       stSrcVideoFrmBuf = {0};
  SAL_VideoFrameBuf       stDstVideoFrmBuf = {0};

  (VOID)sys_hal_getVideoFrameInfo(pSrcSysFrame, &stSrcVideoFrmBuf);
  (VOID)sys_hal_getVideoFrameInfo(pDstSysFrame, &stDstVideoFrmBuf);
  
  memset(&srcSurface, 0, sizeof(TDE_HAL_SURFACE));
  memset(&dstSurface, 0, sizeof(TDE_HAL_SURFACE));
  memset(&srcRect, 0, sizeof(TDE_HAL_RECT));
  memset(&dstRect, 0, sizeof(TDE_HAL_RECT));

  srcSurface.u32Width   = stSrcVideoFrmBuf.frameParam.width;
  srcSurface.u32Height  = stSrcVideoFrmBuf.frameParam.height;
  srcSurface.u32Stride  = stSrcVideoFrmBuf.stride[0];
  srcSurface.PhyAddr = stSrcVideoFrmBuf.phyAddr[0];
  srcSurface.enColorFmt = stSrcVideoFrmBuf.frameParam.dataFormat;
  srcSurface.vbBlk = stSrcVideoFrmBuf.vbBlk;
  srcRect.s32Xpos = x;
  srcRect.s32Ypos = y;
  srcRect.u32Width = w;
  srcRect.u32Height = h;

  dstSurface.u32Width   = stDstVideoFrmBuf.frameParam.width;
  dstSurface.u32Height  = stDstVideoFrmBuf.frameParam.height;
  dstSurface.u32Stride  = stDstVideoFrmBuf.stride[0];
  dstSurface.PhyAddr = stDstVideoFrmBuf.phyAddr[0];
  dstSurface.enColorFmt = stSrcVideoFrmBuf.frameParam.dataFormat; 
  dstSurface.vbBlk = stDstVideoFrmBuf.vbBlk;
  dstRect.s32Xpos   = uiDstX;
  dstRect.s32Ypos   = uiDstY;
  dstRect.u32Width  = w;
  dstRect.u32Height = h;

  s32Ret = tde_hal_QuickCopy(&srcSurface, &srcRect, &dstSurface, &dstRect, bCached);
  if(s32Ret != SAL_SOK)
  {
      SAL_LOGE("err %#x\n", s32Ret);
      return SAL_FAIL;
  }

  /* 编码器需要时间戳信息 */
  stDstVideoFrmBuf.frameNum      = stSrcVideoFrmBuf.frameNum;
  stDstVideoFrmBuf.pts           = stSrcVideoFrmBuf.pts; 
  //stDstVideoFrmBuf.frameParam.width        = w;
  //stDstVideoFrmBuf.frameParam.height       = h;
  stDstVideoFrmBuf.frameParam.dataFormat = stSrcVideoFrmBuf.frameParam.dataFormat;
//    stDstVideoFrmBuf.stride[0]    = w;
//  stDstVideoFrmBuf.stride[1]    = w;
//  stDstVideoFrmBuf.stride[2]    = w;

  s32Ret = sys_hal_buildVideoFrame(&stDstVideoFrmBuf, pDstSysFrame);
  if (SAL_SOK != s32Ret)
  {
      SAL_LOGE("err %#x\n", s32Ret);
      return SAL_FAIL;
  }   
  
  pDstSysFrame->uiDataWidth           = w;
  pDstSysFrame->uiDataHeight          = h;

  return SAL_SOK;
}



/**
 * @function    IA_GetBaScheHndl
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
void *IA_GetBaScheHndl(void)
{
#ifdef RK3588
    return scheduler_handle;
#elif defined HI3559A_SPC030
	return pSbaSchedulerHandle;
#else
	return NULL;
#endif
}

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
UINT32 IA_GetModMemInitSize(IA_MOD_IDX_E mod, UINT32 uiSizeArr[IA_MEM_TYPE_NUM])
{
    UINT32 uiCpySize = 0;

    if (mod >= IA_MOD_MAX_NUM)
    {
        SAL_LOGW("Invalid mod %d >= Max %d \n", mod, IA_MOD_MAX_NUM);
        return SAL_FAIL;
    }

    uiCpySize = IA_MEM_TYPE_NUM * sizeof(UINT32);

    sal_memcpy_s(uiSizeArr, uiCpySize, gInitMemSize[mod], uiCpySize);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : IA_UpdateOutChnResolution
* 描  述  : 更新输出通道分辨率
* 输  入  : pPrm: 输入参数
* 输  出  : 无
* 返回值  : SAL_SOK : 成功
*         : SAL_FAIL: 失败
*******************************************************************************/
INT32 IA_UpdateOutChnResolution(void *pPrm)
{
    /* 入参有效性校验 */
    IA_CHECK_PRM(pPrm, SAL_FAIL);

    /* 变量定义 */
    UINT32 uiWidth = 0;
    UINT32 uiHeight = 0;
    UINT32 uiVdecChn = 0;
    UINT32 uiVpssChn = 0;

    IA_UPDATE_OUTCHN_PRM *pstOutChnPrm = NULL;

    /* 入参本地化 */
    pstOutChnPrm = (IA_UPDATE_OUTCHN_PRM *)pPrm;

    /* 模块索引有效性判断 */
    if (pstOutChnPrm->enModule >= IA_MOD_MAX_NUM)
    {
        SAL_LOGE("Invalid Module Id %d > Max %d \n", (UINT32)pstOutChnPrm->enModule, IA_MOD_MAX_NUM);
        return SAL_FAIL;
    }

    uiWidth = pstOutChnPrm->uiWidth;
    uiHeight = pstOutChnPrm->uiHeight;
    uiVdecChn = pstOutChnPrm->uiVdecChn;
    uiVpssChn = pstOutChnPrm->uiVpssChn;

    if (SAL_SOK != vdec_tsk_SetOutChnResolution(uiVdecChn, uiVpssChn, uiWidth, uiHeight))
    {
        SAL_LOGE("Update Jpeg Chn Fail! VdecChn %d\n", uiVdecChn);
        return SAL_FAIL;
    }

    SAL_LOGI("Update Out Chn End! Module:%d, Width:%d, Height:%d \n", pstOutChnPrm->enModule, uiWidth, uiHeight);
    return SAL_SOK;
}
static void *pAIMutexHdl = NULL;
/**
 * @function   IA_InitHwCore
 * @brief      初始化平台层硬核资源
 * @param[in]  VOID  
 * @param[out] None
 * @return     INT32
 */
INT32 IA_InitHwCore(VOID)
{
	INT32 s32Ret = SAL_FAIL;
 
	AI_HAL_INIT_PRM_S stAiInitPrm = {0};

    DSPINITPARA *pstDspInfo = SystemPrm_getDspInitPara();	
	if (NULL == pAIMutexHdl)
	{
		SAL_mutexCreate(SAL_MUTEX_NORMAL, &pAIMutexHdl);

	}
    SAL_mutexLock(pAIMutexHdl);
	if (NULL == scheduler_handle)
	{
		/* 非从片设备才需要给ai-xsp分配单独的npu资源 */
		if (DOUBLE_CHIP_SLAVE_TYPE != pstDspInfo->deviceType)
		{
			/* 目前仅XSP_LIB_RAYIN类型为非AI-XSP，其余均为AI-XSP类型 */
			stAiInitPrm.stAiXspInitPrm.bAiXspUsed = ((XSP_LIB_SRC)pstDspInfo->xspLibSrc != XSP_LIB_RAYIN);	
			
			/* 当前使用xray通道个数进行统计做AI-XSP的通道个数，暂不存在xray的部分通道不做AI-XSP的情况 */
			stAiInitPrm.stAiXspInitPrm.u32EnableChnNum = pstDspInfo->xrayChanCnt;
		}
		
		s32Ret = ai_hal_Init(&stAiInitPrm, &scheduler_handle);
		if (SAL_SOK != s32Ret)
		{
			SAL_LOGE("ai_hal_Init failed! \n");
			goto exit;
		}

		/* 获取行为分析调度器句柄，仅在hisi3559a有实现与使用 */
		pSbaSchedulerHandle = ai_hal_GetSbaeSchedHdl();
	}

	else
	{
		s32Ret = SAL_SOK;
	}
	
exit:
	SAL_mutexUnlock(pAIMutexHdl);

	return s32Ret;
}

/**
 * @function   IA_DeinitEncrypt
 * @brief      去初始化解密
 * @param[in]  void *pHandle  
 * @param[out] None
 * @return     INT32
 */
INT32 IA_DeinitEncrypt(void *pHandle)
{
#ifdef ENABLE_HW_ENCRYPT
    UINT32 s32Ret = SAL_FAIL;

    /* 解密资源 */
    s32Ret = encrypt_deinit(pHandle);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("Decrypt deinit fail ret=0x%x\n", s32Ret);
        return s32Ret;
    }
#endif

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : IA_InitEncrypt
* 描  述  : 解密资源初始化
* 输  入  : - pHandle: 解密句柄
* 输  出  : - pHandle: 解密句柄
* 返回值  : SAL_SOK  : 成功
*			SAL_FAIL : 失败
*******************************************************************************/
INT32 IA_InitEncrypt(void **ppHandle)
{
#ifdef ENABLE_HW_ENCRYPT
    UINT32 s32Ret = SAL_FAIL;

    /* 解密资源 */
    if (NULL == *ppHandle)
    {
        s32Ret = encrypt_init(ppHandle);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("Decrypt init fail ret=0x%x\n", s32Ret);
            return s32Ret;
        }

        s32Ret = encrypt_proc(*ppHandle);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("Decrypt proc fail ret=0x%x\n", s32Ret);
            return s32Ret;
        }
    }
    else
    {
        SAL_LOGI("pHandle is inited already!\n");
    }
#endif

    return SAL_SOK;
}

VOID Ia_DumpYuvData(CHAR *pStrPath, void *pVir, UINT32 uiSize)
{
    FILE *fp = NULL;

    if (NULL == pStrPath || NULL == pVir)
    {
        SAL_LOGE("ptr null! %p, %p \n", pStrPath, pVir);
		return;
    }
	
	fp = fopen(pStrPath, "w+");
    if (NULL == fp)
    {
        SAL_LOGE("fopen failed! \n");
		return;
    }

	fwrite(pVir, uiSize, 1, fp);
	fflush(fp);

	fclose(fp);
	fp = NULL;
	
    return;		
}
