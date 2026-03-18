/*******************************************************************************
* sva_hal.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : huangshuxin <huangshuxin@hikvision.com>
* Version: V1.0.0  2019年2月14日 Create
*
* Description :
* Modification:
*******************************************************************************/


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
/* #include <platform_sdk.h> */
#include <platform_hal.h>
#include "cJSON.h"
#include "sva_hal.h"
#include "capbility.h"
#include "osd_drv_api.h"
#include "drawChar.h"
#include "vdec_tsk_api.h"
#include "vgs_drv_api.h"
#include "color_space.h"
#include "jpeg_drv_api.h"
#include "sys_tsk.h"
#include "system_prm_api.h"
#include "disp_tsk_inter.h"



/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define SVA_HAL_CHECK_CHAN(chan, value)		{if (chan > (XSI_DEV_MAX - 1)) {SVA_LOGE("Chan (Illegal parameters)\n"); return (value); }}
#define SVA_HAL_CHECK_PRM(ptr, value)		{if (!ptr) {SVA_LOGE("Ptr (The address is empty or Value is 0 )\n"); return (value); }}
#define SVA_HAL_CHECK_RETURN(ptr, value)	{if (ptr) {SVA_LOGE("ptr 0x%x\n", ptr); return (value); }}

#define DIFF_ABS(A, B) ((A > B) ? (A - B) : (B - A))
#define SVA_HAL_CHECK_ENG_CHN(chan, value)  \
    { \
        if (chan > (ENG_DEV_MAX - 1)) \
        { \
            SVA_LOGE("Eng Chn invalid! %d \n", chan); \
            return (value); \
        } \
    }

#define SVA_CFG_FILE_NUM	(4)           /* 配置文件数量 */
#define SVA_CFG_PATH_LENGTH (64)          /* 配置文件路径长度 */

#define SVA_ICF_XSI_NODE_ID (9)           /* 引擎XSI节点号 */
#define SVA_ICF_OBD_NODE_ID (5)           /* 引擎OBD节点号 */
#define SVA_ICF_OBD_NUM		(6)           /* 引擎OBD帧数 */

#define SVA_MAX_OFFSET_TH (-0.00024)
/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
typedef enum _SVA_JSON_CLASS_TYPE_
{
    SVA_JSON_FIRST_CLASS = 0,
    SVA_JSON_SECOND_CLASS,
    SVA_JSON_CLASS_TYPE_NUM,
} SVA_JSON_CLASS_TYPE_E;

typedef enum _SVA_JSON_CLASS_DEV_TYPE_
{
    SVA_JSON_ISA_CLASS = 0,
    SVA_JSON_ISM_CLASS,
    SVA_JSON_CLASS_DEV_TYPE_NUM,
} SVA_JSON_CLASS_DEV_TYPE_E;

typedef struct _SVA_JSON_MOD_PRM_
{
    SVA_JSON_CLASS_TYPE_E enClassType;
    SVA_JSON_CLASS_DEV_TYPE_E uiClassDevType;
    UINT32 uiClassId;

    union
    {
        UINT32 uiVal;
        CHAR *pcVal;
    };
} SVA_JSON_MOD_PRM_S;

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */
extern INT32 capt_func_chipGetViSrcRate(UINT32 viChn);  /*待重构后移动到task层*/

static DEBUG_LEVEL_E svaDbLevel = DEBUG_LEVEL3;

/* 调试参数: 后续考虑统一维护起来 */
static INT32 svaDbgCnt = 30;
static UINT32 uiSvaAlgDbgFlag = SAL_FALSE;
static UINT32 uiSvaAlgDbgLevel = 50;              /* 调试使用；算法耗时打印阈值，单位ms */
static UINT32 g_SlaveProcessGap = 60;             /* 从片使用，非双芯片设备无需关注 */

static XSI_COMMON gXsiCommon = {0};
static SVA_PROC_MODE_E enInitMode = SVA_PROC_MODE_IMAGE;
/* vgs模块宏定义修改后，解决coverity堆栈告警产物，后续模块整理一并修改，下同 */
static VGS_DRAW_AI_PRM *gpastSvaLinePrm[XSI_DEV_MAX] = {NULL};
static VGS_ADD_OSD_PRM *gpastSvaOsdPrm[XSI_DEV_MAX] = {NULL};
static UINT32 gau32SvaVarOsdChn[XSI_DEV_MAX] = {0};

static VGS_RECT_ARRAY_S *gpastJpegRectPrm[XSI_DEV_MAX] = {NULL};
static VGS_ADD_OSD_PRM *gpastJpegOsdPrm[XSI_DEV_MAX] = {NULL};
static UINT32 gau32JpegVarOsdChn[XSI_DEV_MAX] = {0};

/* dump yuv图片的个数，安检机使用,目前设置4组，分别对应1.从xpack中copy出来的yuv图片个数，2.送入引擎的yuv图片个数,3.从xpack中直接获取的yuv图片,4.从片接收的YUV图像(后续可添加) */
static INT32 g_s32DumpDataCnt[SVA_DUMP_YUV_NUM][XSI_DEV_MAX] = {0};

static DISP_SVA_RECT_F g_SvaRect[XSI_DEV_MAX][SVA_XSI_MAX_ALARM_NUM] = {0};

/* 默认置信度表(普通模式下和pk模式下)*/
static INT32 g_s32DefNormalConfTab[SVA_MAX_ALARM_TYPE] = {0};
static INT32 g_s32DefPkConfTab[SVA_MAX_ALARM_TYPE] = {0};


#if 0
static VGS_DRAW_LINE_PRM stSvaVgsDrawLine[XSI_DEV_MAX] = {0};
static VGS_DRAW_LINE_PRM stSvaVgsDrawVLine[XSI_DEV_MAX] = {0};
static VGS_ADD_OSD_PRM stSvaVgsOsdPrm[XSI_DEV_MAX] = {0};

static VGS_DRAW_LINE_PRM stJpegVgsDrawLine[XSI_DEV_MAX] = {0};
static VGS_ADD_OSD_PRM stJpegVgsOsdPrm[XSI_DEV_MAX] = {0};

static VGS_DRAW_LINE_PRM stXpackVgsDrawLine[XSI_DEV_MAX] = {0};
static VGS_ADD_OSD_PRM stXpackVgsOsdPrm[XSI_DEV_MAX] = {0};
#endif

static CHAR g_cfg_path[SVA_CFG_FILE_NUM][SVA_CFG_PATH_LENGTH] =
{
    {"./sva/config/AlgCfgTest.json"},             /* AlgCfgPath */
    {"./sva/config/TASK.json"},                   /* TaskCfgPath */
    {"./sva/config/ToolkitCfg.json"},             /* ToolkitCfgPath */
    {"./sva/config/icf_secdet_param.json"},       /* AppParamCfgPath */
};

/* json配置文件解析一级键值名称 */
static CHAR *g_jSonMainClassKeyTab[SVA_JSON_CLASS_NUM] =
{
    /* 第一级键值名称 */
    "Security_Detect_Param",
};

/* json配置文件解析二级键值名称 */
static CHAR *g_jSonSubIsaClassKeyTab[SVA_JSON_SUB_CLASS_NUM] =
{
    /* 第一级包含的子键值名称 */
    "work_type", "ai_type",
    "pack_conf_thresh", "pd_proc_gap",
    "small_pack_threshold", "pd_split_thresh",
    "cls_batch_num",
    "main_obd_model_num", "main_obd_model_path",
    "side_obd_model_num", "side_obd_model_path",
    "main_pd_model_num", "main_pd_model_path",
    "side_pd_model_num", "side_pd_model_path",
    "ai_main_model_num", "ai_main_model_path",
    "ai_side_model_num", "ai_side_model_path",
    "cls_model_num", "cls_model_path",
    "pos_write_flag",
};

/* json配置文件解析二级键值名称 */
static CHAR *g_jSonSubIsmClassKeyTab[SVA_JSON_SUB_CLASS_NUM] =
{
    /* 第一级包含的子键值名称 */
    "width", "height",
    "work_type", "ai_type",
    "main_model_num", "main_model_path",
    "side_model_num", "side_model_path",
    "ai_main_model_num", "ai_main_model_path",

    /* 第二级，暂时没有，若有需求往后面增加 */

};

#if 0
static CHAR *ai_modelpath[2] = {"./sva/model/AI OBD/zhihu.bin", "./sva/model/AI OBD/zhihu.bin"};  /*原始开放平台AI 模型初始化路径*/
static CHAR *single_modelPath[2] = {"./sva/model/OBD/xsi_bin_fxy_single_energy_model1_pack.bin", "./sva/model/OBD/xsi_bin_fxy_single_energy_model2_pack.bin"};  /*单能模型路径*/
static CHAR *mul_modelPath[2] = {"./sva/model/OBD/xsi_bin_ajj_mul_energy_model1_pack_1152_1152_1109.bin", "./sva/model/OBD/xsi_bin_ajj_mul_energy_model2_pack_1152_1152_1109.bin"};  /*多能模型路径*/
static CHAR *pcAna_modelPath[2] =
{
    "./sva/model/OBD/xsi_bin_fxy_mul_energy_model1_pack_1600_1280_1109.bin",
    "./sva/model/OBD/xsi_bin_fxy_mul_energy_model2_pack_1600_1280_1109.bin"
};  /*多能模型路径*/
#endif

void *pCbFunc = NULL;                  /* 结果回调函数 */

#if 0
OSD_HAL_OBJ_INFO g_SvaObjOsdInfo[XSI_DEV_MAX][DISP_OSD_MAX_OBJ_NUM] = {0};       /* 保存非数字的osd字符信息 */

OSD_HAL_OBJ_INFO g_SvaObjNumOsdInfo[XSI_DEV_MAX][DISP_OSD_MAX_OBJ_NUM] = {0};    /* 保存数字osd字符信息，如自信度 */

extern OSD_HAL_DRAW g_drawOsdPrm[DISP_OSD_MAX_CHAN_NUM];
#endif

static ENGINE_COMM_FUNC_P *g_pEngineCommFunc = NULL;

extern INT32 Sva_DrvCallBackDealOut(UINT32 chan, SVA_PROCESS_OUT *pstHalOut, SYSTEM_FRAME_INFO *pstSysFrame);
extern INT32 Sva_DrvSaveProcMode(void *prm);

/**
 * @function:   Sva_HalGetIcfCommFuncP
 * @brief:      获取icf回调函数指针
 * @param[in]:  None
 * @param[out]: None
 * @return:     VOID
 */
ENGINE_COMM_FUNC_P *Sva_HalGetIcfCommFuncP(VOID)
{
    return g_pEngineCommFunc;
}

/*******************************************************************************
* 函数名  : Sva_HalGetIcfParaJson
* 描  述  : 获取相应的配置文件路径
* 输  入  : - cfg_num: 文件num
* 输  出  : 无
* 返回值  : 对应的json文件路径
*******************************************************************************/
CHAR *Sva_HalGetIcfParaJson(SVA_CFG_JSONFILE_E cfg_num)
{
    /*input check*/
    if (cfg_num > APPPARAM_CFG_PATH)
    {
        SVA_LOGE("Input json file num ERROR ! \n");
        return NULL;
    }

    return g_cfg_path[cfg_num];

}

/*******************************************************************************
* 函数名  : Sva_HalRegCbFunc
* 描  述  : 注册结果回调函数
* 输  入  : SVACALLBACK p   结果回调函数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalRegCbFunc(SVACALLBACK p)
{
    pCbFunc = p;
    return SAL_SOK;
}

/**
 * @function:   Sva_HalGetEngchnPrm
 * @brief:      获取引擎通道全局参数
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     ENG_CHANNEL_PRM_S *
 */
ENG_CHANNEL_PRM_S *Sva_HalGetEngchnPrm(UINT32 chan)
{
    SVA_HAL_CHECK_ENG_CHN(chan, NULL);
    return &gXsiCommon.stEngChnPrm[chan];
}

/*******************************************************************************
* 函数名  : Sva_HalGetDev
* 描  述  : 获取算法结果全局变量
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
XSI_COMMON *Sva_HalGetXsiCommon(void)
{
    /* converity检测到此处必然不为空，所以后面不需要加return NULL */
    return &gXsiCommon;
}

/*******************************************************************************
* 函数名  : Sva_HalGetDev
* 描  述  : 获取设备全局变量
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : 设备全局变量
*******************************************************************************/
XSI_DEV *Sva_HalGetDev(UINT32 chan)
{
    SVA_HAL_CHECK_CHAN(chan, NULL);

    XSI_COMMON *pstXsiCommon = Sva_HalGetXsiCommon();

    SVA_HAL_CHECK_PRM(pstXsiCommon, NULL);

    return &pstXsiCommon->xsi_dev[chan];
}

/*******************************************************************************
* 函数名  : Sva_HalGetRandomId
* 描  述  : 获取随机数
* 输  入  : 无
* 输  出  : 无
* 返回值  : 随机数
*******************************************************************************/
static UINT32 Sva_HalGetRandomId(void)
{
    INT32 ret = 0;
    INT32 fd = 0;
    UINT32 seed = 0;
    static UINT32 initialized = 0;

    if (!initialized)
    {
        fd = open("/dev/urandom", 0);
        if (fd < 0)
        {
            goto err;
        }

        ret = read(fd, &seed, sizeof(seed));
        if (ret < 0)
        {
            goto err;
        }

        close(fd);

        srand(seed);     /* 设置随机种子 */

        initialized++;   /* 下次取同样的随机数 */
    }

    goto exit;

err:
    if (fd >= 0)
    {
        close(fd);
    }

    SVA_LOGE("Could not load seed from /dev/urandom: %s",
             strerror(errno));
    seed = time(0);

exit:
    return rand();
}

/*******************************************************************************
* 函数名  : Sva_HalJudgeForwPull
* 描  述  : 判断是否是前拉状态
* 输  入  : - chan : 通道号
*         : - pPrm : 帧数据
*         : - pFlag: 标志位(判断是否回拉)
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 判断是否为前拉状态，若是返回1的标志位
*******************************************************************************/
INT32 Sva_HalJudgeForwPull(UINT32 chan, void *pPrm, void *pFpFlag)
{
    /* 入参有效性判断 */
    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pPrm, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pFpFlag, SAL_FAIL);

    /* 变量定义 */
    UINT32 uiLastIdx = 0;
    UINT32 uiCurFlag = 0;
    float fOffsetGap = 0;

    static UINT32 uiCurIdx[2] = {0};
    static UINT32 uiLastFlag[2] = {0};
    static float fData[2][3] = {{0.0}, {0.0}};

    UINT32 *puiFpFlag = NULL;
    SVA_PROCESS_OUT *pstOut = NULL;

    /* 入参本地化 */
    pstOut = (SVA_PROCESS_OUT *)pPrm;
    puiFpFlag = (UINT32 *)pFpFlag;

    /* 若偏移量为负值，无需进行正拉判断 */
    if (pstOut->frame_offset < 0.0)
    {
        return SAL_SOK;
    }

    fData[chan][uiCurIdx[chan]] = pstOut->frame_offset;

    uiLastIdx = (uiCurIdx[chan] + 2) % 3;
    fOffsetGap = fData[chan][uiCurIdx[chan]] - fData[chan][uiLastIdx];

    uiCurFlag = uiLastFlag[chan];
    if (0 == uiCurFlag)
    {
        if (fOffsetGap > 0.05)   /* 前拉标准为偏移超过画面的15% */
        {
            *puiFpFlag = SAL_TRUE;
            uiCurFlag = 1;
        }
    }
    else if (1 == uiCurFlag)
    {
        *puiFpFlag = SAL_TRUE;   /* 从前拉状态恢复正常前，均不显示目标 */
        if (fData[chan][uiCurIdx[chan]] <= 0.02
            && fData[chan][uiLastIdx] <= 0.02)
        {
            uiCurFlag = 0;
            *puiFpFlag = SAL_FALSE;
        }
    }
    else
    {
        /* DO NOTHING.. */
        SVA_LOGE("invalid flag %d, chan %d \n", uiLastFlag[chan], chan);
    }

    uiLastFlag[chan] = uiCurFlag;

    uiCurIdx[chan] = (uiCurIdx[chan] + 1) % 3;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalJudgeBackPull
* 描  述  : 判断是否是回拉状态
* 输  入  : - chan : 通道号
*         : - pPrm : 帧数据
*         : - pFlag: 标志位(判断是否回拉)
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 判断是否为回拉状态，若是返回1的标志位
*******************************************************************************/
INT32 Sva_HalJudgeBackPull(UINT32 chan, void *pPrm, void *pBpFlag)
{
    /* 入参有效性判断 */
    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pPrm, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pBpFlag, SAL_FAIL);

    /* 变量定义 */
    UINT32 uiChgSts = 0;
    UINT32 uiLastIdx = 0;
    UINT32 uiCurFlag = 0;
    static float fData[2][3] = {{0.0}, {0.0}};
    static UINT32 uiLastFlag[2] = {0};   /* 0: 正常过包，1: 回拉状态 */
    static UINT32 uiCurIdx[2] = {0};     /* 当前索引值 */

    UINT32 *puiBpFlag = NULL;
    SVA_PROCESS_OUT *pstOut = NULL;

    /* 入参本地化 */
    pstOut = (SVA_PROCESS_OUT *)pPrm;
    puiBpFlag = (UINT32 *)pBpFlag;

    if (pstOut->frame_offset <= SVA_MAX_OFFSET_TH)
    {
        SVA_LOGW("frm_offset %f < 0 \n", pstOut->frame_offset);
        pstOut->frame_offset = 0;
    }

    fData[chan][uiCurIdx[chan]] = pstOut->frame_offset;   /* 保存当前帧间隔 */

    uiCurFlag = uiLastFlag[chan];
    uiLastIdx = (uiCurIdx[chan] + 2) % 3;

    if (0 == uiLastFlag[chan])
    {
        /* 若上次为正常过包 */
        if (fData[chan][uiCurIdx[chan]] < 0.0 && fData[chan][uiLastIdx] < 0.0)
        {
            uiChgSts = 1;
        }
    }
    else if (1 == uiLastFlag[chan])
    {
        /* 若上次为回拉状态 */
        if (fData[chan][uiCurIdx[chan]] > 0.0 && fData[chan][uiLastIdx] > 0.0)
        {
            uiChgSts = 1;
        }
    }
    else
    {
        /* DO NOTHING.. */
        SVA_LOGE("chan %d invalid flag %d \n", chan, uiLastFlag[chan]);
    }

    if (1 == uiChgSts)
    {
        uiCurFlag = 1 - uiLastFlag[chan];
    }

    uiLastFlag[chan] = uiCurFlag;   /* 保留当前过包状态，作为下一次判断标准 */

    /* 静止后需要更新算法结果 */
    if (0.0 == fData[chan][uiCurIdx[chan]] && 0.0 == fData[chan][uiLastIdx])
    {
        uiCurFlag = 0;
    }

    uiCurIdx[chan] = (uiCurIdx[chan] + 1) % 3;

    /* 保存当前判断结果 */
    *puiBpFlag = uiCurFlag;

    return SAL_SOK;
}

/**
 * @function   Sva_HalMemPoolSystemMallocCb
 * @brief      申请系统内存的回调函数
 * @param[in]  IA_MEM_TYPE_E enMemType
 * @param[in]  UINT32 u32MemSize
 * @param[in]  IA_MEM_PRM_S *pstMemBuf
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_HalMemPoolSystemMallocCb(IA_MEM_TYPE_E enMemType, /* TODO: 后续取消使用ia_common.c中的数据结构 */
                                          UINT32 u32MemSize,
                                          IA_MEM_PRM_S *pstMemBuf)
{
    INT32 s32Ret = SAL_FAIL;
    UINT64 pa = 0;
    VOID *va = NULL;
    ALLOC_VB_INFO_S stVbInfo = {0};

    switch (enMemType)
    {
        case IA_MALLOC:
        {
            va = SAL_memZalloc(u32MemSize, "sva", "engine");
            if (NULL == va)
            {
                SVA_LOGE("malloc failed! \n");
                return SAL_FAIL;
            }

            pstMemBuf->VirAddr = (char *)va;
            pstMemBuf->PhyAddr = (PhysAddr)pstMemBuf->VirAddr;
            break;
        }
        case IA_HISI_MMZ_CACHE:
        case IA_HISI_MMZ_NO_CACHE:
        case IA_HISI_MMZ_CACHE_PRIORITY:
        case IA_HISI_MMZ_CACHE_NO_PRIORITY:
        case IA_HISI_VB_REMAP_NONE:
        case IA_HISI_VB_REMAP_NO_CACHED:
        case IA_HISI_VB_REMAP_CACHED:
        {
            if (SAL_SOK != Ia_GetXsiFreeMem(IA_MOD_SVA, enMemType, u32MemSize, pstMemBuf))
            {
                SVA_LOGE("Get Xsi Free Mem Failed! size %d, type %d \n", u32MemSize, enMemType);
                return SAL_FAIL;
            }

            goto exit;
        }
        case IA_NT_MEM_FEATURE:
        {
            SVA_LOGI("NT MEM ALLOC, lBufSize13 = %d\n", u32MemSize);
            s32Ret = mem_hal_mmzAllocDDR0(u32MemSize, "sva", "sva_engine", NULL, SAL_TRUE, (UINT64 *)&pa, (VOID **)&va);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("nt feature mem alloc err!!\n");
                return SAL_FAIL;
            }

            pstMemBuf->VirAddr = va;
            pstMemBuf->PhyAddr = pa;

            goto exit;
        }
        case IA_RK_CACHE_CMA:
        {
            SVA_LOGI("RK MEM ALLOC, lBufSize13 = %d\n", u32MemSize);
            s32Ret = mem_hal_iommuMmzAlloc(u32MemSize, "sva", "sva_engine", NULL, SAL_TRUE, &stVbInfo);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("rk cma Malloc err!!\n");
                return SAL_FAIL;
            }

            pstMemBuf->VirAddr = stVbInfo.pVirAddr;
            pstMemBuf->PhyAddr = (PhysAddr)stVbInfo.pVirAddr;

            goto exit;
        }
        default:
        {
            SVA_LOGE("invalid mem type %d \n", enMemType);
            return SAL_FAIL;
        }
    }

exit:
    return SAL_SOK;
}

/**
 * @function   Sva_HalMemPoolSystemFreeCb
 * @brief      释放系统内存的回调函数
 * @param[in]  IA_MEM_TYPE_E enMemType
 * @param[in]  UINT32 u32MemSize
 * @param[in]  IA_MEM_PRM_S *pstMemBuf
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_HalMemPoolSystemFreeCb(IA_MEM_TYPE_E enMemType, /* TODO: 后续取消使用ia_common.c中的数据结构 */
                                        UINT32 u32MemSize,
                                        IA_MEM_PRM_S *pstMemBuf)

{
    INT32 s32Ret = SAL_FAIL;

    switch (enMemType)
    {
        case IA_MALLOC:
        {
            SAL_memfree(pstMemBuf->VirAddr, "sva", "engine");
            break;
        }
        case IA_HISI_MMZ_CACHE:
        case IA_HISI_MMZ_NO_CACHE:
        case IA_HISI_MMZ_CACHE_PRIORITY:
        case IA_HISI_MMZ_CACHE_NO_PRIORITY:
        case IA_HISI_VB_REMAP_NONE:
        case IA_HISI_VB_REMAP_NO_CACHED:
        case IA_HISI_VB_REMAP_CACHED:
        {
            return SAL_SOK;
        }
        case IA_NT_MEM_FEATURE:
        {
            uintptr_t va = 0;
            va = (uintptr_t) pstMemBuf->VirAddr;
            s32Ret = mem_hal_mmzFree((void *)va, "sva", "sva_engine");
            if (s32Ret != SAL_SOK)
            {
                SVA_LOGE("mmz free Err! sts %x\n", s32Ret);
                return SAL_FAIL;
            }

            return SAL_SOK;
        }
        case IA_RK_CACHE_CMA:
        {
            s32Ret = mem_hal_iommuMmzFree(u32MemSize, "sva", "sva_engine", pstMemBuf->PhyAddr, pstMemBuf->VirAddr, 0);
            if (s32Ret != SAL_SOK)
            {
                SVA_LOGE("cma free Err! sts %x\n", s32Ret);
                return SAL_FAIL;
            }

            return SAL_SOK;
        }

        default:
        {
            SVA_LOGE("invalid mem type %d \n", enMemType);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalBuffCreate
* 描  述  : 创建送引擎的帧Buff
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalBuffCreate(UINT32 chan)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 i = 0;

    XSI_DEV *pstXsi = NULL;
    XSI_DEV_BUFF *pstDevBuff = NULL;
    XSI_COMMON *pstXsiCommon = NULL;

    ALLOC_VB_INFO_S stVbInfo = {0};

    pstXsi = Sva_HalGetDev(chan);
    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    pstXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pstXsiCommon, SAL_FAIL);
    for (i = 0; i < SVA_INPUT_BUF_LEN; i++)
    {
        pstDevBuff = &pstXsi->stBuff[i];

        if (NULL == pstDevBuff->stMemBuf.VirAddr)
        {
#ifdef DSP_ISM

            /* 当前需要兼容NT平台送引擎处理的buf必须在DDR0，且mem_hal_mmzAllocDDR0中平台内存申请接口仅在NT有定义
               故此处先调用mem_hal_mmzAllocDDR0接口，若成功则为NT平台，否则调用mem_hal_vbAlloc平台通用接口 */
            if (SAL_SOK != mem_hal_mmzAllocDDR0(SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2,
                                                "sva", "buff_create", NULL,
                                                SAL_TRUE,
                                                (UINT64 *)&pstDevBuff->stMemBuf.PhyAddr,
                                                (VOID **)&pstDevBuff->stMemBuf.VirAddr))
            {
                s32Ret = mem_hal_vbAlloc(SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2 + 
                                         (SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 6),
                                         "sva", "buff_create", NULL,
                                         SAL_TRUE,
                                         &stVbInfo);
                if (SAL_SOK != s32Ret)
                {
                    SVA_LOGE("mmz alloc failed! chan %d \n", chan);
                    return SAL_FAIL;
                }

                pstDevBuff->stMemBuf.PhyAddr = (PhysAddr)stVbInfo.u64PhysAddr;
                pstDevBuff->stMemBuf.VirAddr = (char *)stVbInfo.pVirAddr;
                pstDevBuff->stMemBuf.u64VbBlk = stVbInfo.u64VbBlk;
                pstDevBuff->stMemBuf.u32PoolId = stVbInfo.u32PoolId;
            }

#elif defined DSP_ISA
            s32Ret = mem_hal_vbAlloc(SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2,
                                     "sva",
                                     "buff_create",
                                     NULL,
                                     SAL_TRUE,
                                     &stVbInfo);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("MemPoolMemAlloc error, ret: 0x%x \n", s32Ret);
                return SAL_FAIL;
            }

            pstDevBuff->stMemBuf.VirAddr = (void *)stVbInfo.pVirAddr;
            pstDevBuff->stMemBuf.PhyAddr = (PhysAddr)stVbInfo.u64PhysAddr;
            pstDevBuff->stMemBuf.u64VbBlk = stVbInfo.u64VbBlk;
            pstDevBuff->stMemBuf.u32PoolId = stVbInfo.u32PoolId;

            pstDevBuff->buffUse = SAL_FALSE;
            pstDevBuff->devChn = chan;
#endif
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalBuffDelete
* 描  述  : 删除送引擎的帧Buff
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalBuffDelete(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;

    XSI_DEV *pstXsi = NULL;
    XSI_DEV_BUFF *pstDevBuff = NULL;
    XSI_COMMON *pstXsiCommon = NULL;

    pstXsi = Sva_HalGetDev(chan);
    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    pstXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pstXsiCommon, SAL_FAIL);

    for (i = 0; i < SVA_INPUT_BUF_LEN; i++)
    {
        pstDevBuff = &pstXsi->stBuff[i];

        if (pstDevBuff->stMemBuf.VirAddr)
        {
            SVA_LOGW("dev buff is released! i %d, chan %d \n", i, chan);
            continue;
        }

#ifdef DSP_ISM
        if (0 == pstDevBuff->stMemBuf.u64VbBlk)
        {
            s32Ret = mem_hal_mmzFree(pstDevBuff->stMemBuf.VirAddr, "sva", "buff_create");
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("mem_hal_mmzFree failed! chan %d, i %d \n", chan, i);
            }
        }
        else
        {
            s32Ret = mem_hal_vbFree(pstDevBuff->stMemBuf.VirAddr,
                                    "sva",
                                    "buff_create",
                                    SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2 +
                                    (SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 6),
                                    pstDevBuff->stMemBuf.u64VbBlk,
                                    pstDevBuff->stMemBuf.u32PoolId);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("mem_hal_vbFree failed! chan %d, i %d \n", chan, i);
            }
        }

#elif defined DSP_ISA
        s32Ret = mem_hal_vbFree(pstDevBuff->stMemBuf.VirAddr,
                                "sva",
                                "buff_create",
                                SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2,
                                pstDevBuff->stMemBuf.u64VbBlk,
                                pstDevBuff->stMemBuf.u32PoolId);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("mem_hal_vbFree error, ret: 0x%x \n", s32Ret);
            return SAL_FAIL;
        }

#endif

        pstDevBuff->stMemBuf.VirAddr = NULL;
        pstDevBuff->stMemBuf.PhyAddr = 0;
        pstDevBuff->stMemBuf.u64VbBlk = 0;
        pstDevBuff->stMemBuf.u32PoolId = 0;
        pstDevBuff->buffUse = SAL_FALSE;
        pstDevBuff->devChn = 0;
    }

    return SAL_SOK;
}

/**
 * @function    Sva_HalGetViSrcRate
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Sva_HalGetViSrcRate(VOID)
{
    return (capt_func_chipGetViSrcRate(1)     \
            ? capt_func_chipGetViSrcRate(1)   \
            : (capt_func_chipGetViSrcRate(0) ? capt_func_chipGetViSrcRate(0) : 60));
}

/**
 * @function:   Sva_HalGetProcGap
 * @brief:      获取OBD处理间隔
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     UINT32
 */
static UINT32 Sva_HalGetMasterProcGap(VOID)
{
    INT32 iViSrcRate = 0;
    UINT32 uiProcessGap = 60;

    DSPINITPARA *pstInitPrm = NULL;

    pstInitPrm = SystemPrm_getDspInitPara();

    iViSrcRate = Sva_HalGetViSrcRate();
    if (iViSrcRate < 0)
    {
        SVA_LOGE("Get Vi Src Frame Rate Failed! \n");
        goto exit;
    }

    SVA_LOGD("szl_dbg: get vi src rate %d \n", iViSrcRate);

    if (SINGLE_MODEL_TYPE == pstInitPrm->modelType)
    {
        uiProcessGap = iViSrcRate / Sva_DrvGetInputGapNum() * 2 / 3;
    }
    else if (DOUBLE_MODEL_TYPE == pstInitPrm->modelType)
    {
        uiProcessGap = iViSrcRate / Sva_DrvGetInputGapNum();
    }
    else
    {
        SVA_LOGW("Invalid model type %d \n", pstInitPrm->modelType);
    }

exit:
    return uiProcessGap;
}

/**
 * @function:   Sva_HalSetSlaveProcGap
 * @brief:      设置从片处理帧间隔
 * @param[in]:  UINT32 uiProcGap
 * @param[out]: None
 * @return:     VOID
 */
VOID Sva_HalSetSlaveProcGap(UINT32 uiProcGap)
{
    g_SlaveProcessGap = uiProcGap;
    return;
}

/**
 * @function:   Sva_HalGetSlaveProcGap
 * @brief:      获取从片处理帧间隔
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     UINT32
 */
static UINT32 Sva_HalGetSlaveProcGap(VOID)
{
    return g_SlaveProcessGap;
}

/**
 * @function:   Sva_HalGetViInputFrmRate
 * @brief:      获取VI输入帧率
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     UINT32
 */
UINT32 Sva_HalGetViInputFrmRate(VOID)
{
    UINT32 uiViFrmRate = 60;

    DSPINITPARA *pstInitPrm = NULL;

    pstInitPrm = SystemPrm_getDspInitPara();

    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_SVA))
    {
        if (SAL_TRUE != IA_GetModTransFlag(IA_MOD_SVA) || DOUBLE_CHIP_MASTER_TYPE == pstInitPrm->deviceType)
        {
            uiViFrmRate = Sva_HalGetMasterProcGap();
        }
        else if (DOUBLE_CHIP_SLAVE_TYPE == pstInitPrm->deviceType)
        {
            uiViFrmRate = Sva_HalGetSlaveProcGap();
        }
        else
        {
            /* DO NOTHING... */
        }
    }

    /* checker */
    if (uiViFrmRate > 75 || uiViFrmRate < 60)
    {
        uiViFrmRate = 60;
    }

    return uiViFrmRate;
}

/**
 * @function:   Sva_HalGetPdProcGap
 * @brief:      获取pd处理间隔
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     UINT32
 */
UINT32 Sva_HalGetPdProcGap(VOID)
{
    UINT32 uiProcessGap = 6;

    /* todo: 获取算法配置文件中pd proc gap，当前默认是6帧 */



    return uiProcessGap;
}

/*******************************************************************************
* 函数名  : Sva_HalInitInputDataPrm
* 描  述  : 初始化输入帧数据固化参数
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalInitInputDataPrm(SVA_PROC_MODE_E mode, UINT32 aiFlag)
{
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 uiChanNum = 0;

    XSI_COMMON *pstXsiCommon = NULL;
    ENGINE_COMM_INPUT_DATA_INFO *pstIcfCommIptDataAddInfo = NULL;
    ENGINE_COMM_INPUT_DATA_IMG_INFO *pstIcfCommIptDataImg = NULL;
    ENGINE_COMM_INPUT_DATA_IMG_PRM *pstIcfCommInputDataViewImg = NULL;

    /* Input Args Checker */
    if (mode >= SVA_PROC_MODE_NUM || aiFlag > 1)
    {
        SVA_LOGE("Invalid input args: mode[%d], AiFlag[%d], Pls Check! \n", mode, aiFlag);
        return SAL_FAIL;
    }

    /* Localize Input Args */
    pstXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pstXsiCommon, SAL_FAIL);

    /* Get Initialized Channel Number */
    uiChanNum = pstXsiCommon->uiChannelNum;

    for (i = 0; i < uiChanNum; i++)
    {
        for (j = 0; j < SVA_INPUT_BUF_LEN; j++)
        {
            pstIcfCommIptDataAddInfo = (ENGINE_COMM_INPUT_DATA_INFO *)&pstXsiCommon->stEngChnPrm[i].astSvaInputData[j].stInputDataAddInfo;

            /* Initial Input Data Param */
            pstIcfCommIptDataAddInfo->stIaImgPrvtCtrlPrm.u32InputDataImgNum = 1;
            pstIcfCommIptDataImg = &pstIcfCommIptDataAddInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0];

            pstIcfCommIptDataImg->u32W = SVA_MODULE_WIDTH;
            pstIcfCommIptDataImg->u32H = SVA_MODULE_HEIGHT;
            pstIcfCommIptDataImg->enImgType = YUV_NV21;
            pstIcfCommIptDataImg->fYuvRectX = 0.0;
            pstIcfCommIptDataImg->fYuvRectY = 0.0;
            pstIcfCommIptDataImg->fYuvRectH = 1.0;
            pstIcfCommIptDataImg->fYuvRectW = 1.0;

            /* 此处需要区分平台，引擎内部内存类型与平台层绑定 */
#ifdef NT98336
            pstIcfCommIptDataImg->enInputDataMemType = NT_FEATURE;
#elif defined HI3559A_SPC030
            pstIcfCommIptDataImg->enInputDataMemType = HISI_MMZ_CACHE;
#elif defined RK3588
            pstIcfCommIptDataImg->enInputDataMemType = RK_MALLOC;
#endif

            /* Main View: Initial Input Data Param with Default Values */
            pstIcfCommIptDataImg->u32MainViewImgNum = 1;
            pstIcfCommInputDataViewImg = &pstIcfCommIptDataImg->stMainViewImg[0];
            pstIcfCommInputDataViewImg->u32W = SVA_MODULE_WIDTH;
            pstIcfCommInputDataViewImg->u32H = SVA_MODULE_HEIGHT;
            pstIcfCommInputDataViewImg->u32S[0] = SVA_MODULE_WIDTH;
            pstIcfCommInputDataViewImg->u32S[1] = SVA_MODULE_WIDTH;
            pstIcfCommInputDataViewImg->u32S[2] = SVA_MODULE_WIDTH;

            /* Side View: Initial Input Data Param with Default Values */
            pstIcfCommIptDataImg->u32SideViewImgNum = 1;
            pstIcfCommInputDataViewImg = &pstIcfCommIptDataImg->stSideViewImg[0];
            pstIcfCommInputDataViewImg->u32W = SVA_MODULE_WIDTH;
            pstIcfCommInputDataViewImg->u32H = SVA_MODULE_HEIGHT;
            pstIcfCommInputDataViewImg->u32S[0] = SVA_MODULE_WIDTH;
            pstIcfCommInputDataViewImg->u32S[1] = SVA_MODULE_WIDTH;
            pstIcfCommInputDataViewImg->u32S[2] = SVA_MODULE_WIDTH;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalIcfMemFreeInputData
* 描  述  : 释放引擎输入帧数据内存
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalIcfMemFreeInputData(void)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    UINT32 uiChnNum = XSI_DEV_MAX;

    /* Free Mmz for ICF Input Data, default Free mmz for two channels */
    for (i = 0; i < uiChnNum; i++)
    {
        s32Ret = Sva_HalBuffDelete(i);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Delete Circle Buf Failed! chan %d ", i);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalIcfMemAllocInputData
* 描  述  : 申请引擎输入帧数据内存
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalIcfMemAllocInputData(void)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    UINT32 uiChnNum = XSI_DEV_MAX;

    /* Malloc Mmz for ICF Input Data, default malloc mmz for two channels */
    for (i = 0; i < uiChnNum; i++)
    {
        s32Ret = Sva_HalBuffCreate(i);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Create Circle Buf Failed! chan %d \n", i);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalGetIcfVersion
* 描  述  : 创建引擎句柄
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalGetIcfVersion(UINT32 chan, SVA_PROC_MODE_E mode)
{
    INT32 s32Ret = SAL_FAIL;

    ENGINE_COMM_VERSION_S stIcfAppVer = {0};
    XSIE_GET_MODEL_VERSION_T stObdVersion = {0};
    XSIE_SECURITY_VERSION_T stXsiVersion = {0};
    XSIE_GET_MODEL_VERSION_T stModelInfo = {0};
    ENGINE_COMM_USER_CONFIG_PRM stIcfCommCfgPrm = {0};

    XSI_COMMON *pstXsiCommon = Sva_HalGetXsiCommon();

    /* ICF APP version */
    s32Ret = g_pEngineCommFunc->get_icf_app_version(&stIcfAppVer, sizeof(ENGINE_COMM_VERSION_S));
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("get icf app version failed! \n");
        goto exit;
    }

    /* OBD version */
    stIcfCommCfgPrm.pChnHandle = pstXsiCommon->stEngChnPrm[chan].pHandle;
    stIcfCommCfgPrm.u32NodeId = XSIE_OBD_NODE_1;
    stIcfCommCfgPrm.u32CfgKey = XSIE_SECURITY_GET_OBD_VERSION;
    stIcfCommCfgPrm.pData = &stObdVersion;
    stIcfCommCfgPrm.u32DataSize = sizeof(XSIE_GET_MODEL_VERSION_T);

    s32Ret = g_pEngineCommFunc->get_config(&stIcfCommCfgPrm);
    if (0 != s32Ret)
    {
        SVA_LOGE("icf get obd version failed! ret= 0x%x\n", s32Ret);
        goto exit;
    }

    /* XSI version */
    stIcfCommCfgPrm.pChnHandle = pstXsiCommon->stEngChnPrm[chan].pHandle;
    stIcfCommCfgPrm.u32NodeId = (mode == SVA_PROC_MODE_DUAL_CORRELATION ? XSIE_XSI_NODE : XSIE_XSI_NODE_1);
    stIcfCommCfgPrm.u32CfgKey = XSIE_SECURITY_GET_XSI_VERSION;
    stIcfCommCfgPrm.pData = &stXsiVersion;
    stIcfCommCfgPrm.u32DataSize = sizeof(XSIE_SECURITY_VERSION_T);

    s32Ret = g_pEngineCommFunc->get_config(&stIcfCommCfgPrm);
    if (0 != s32Ret)
    {
        SVA_LOGE("icf get obd version failed! ret= 0x%x\n", s32Ret);
        goto exit;
    }

    /* Project Analyser Only Get Main-View Model Version */
    stModelInfo.ModelIndex = 0;

    /* Model version */
    stIcfCommCfgPrm.pChnHandle = pstXsiCommon->stEngChnPrm[chan].pHandle;
    stIcfCommCfgPrm.u32NodeId = XSIE_OBD_NODE_1;
    stIcfCommCfgPrm.u32CfgKey = XSIE_SECURITY_GET_OBD_MODEL_VERSION;
    stIcfCommCfgPrm.pData = &stModelInfo;
    stIcfCommCfgPrm.u32DataSize = sizeof(XSIE_GET_MODEL_VERSION_T);

    s32Ret = g_pEngineCommFunc->get_config(&stIcfCommCfgPrm);
    if (0 != s32Ret)
    {
        SVA_LOGE("icf get obd version failed! ret= 0x%x\n", s32Ret);
        goto exit;
    }

    /* Save Version Info */
    s32Ret = snprintf((CHAR *)pstXsiCommon->version,
                      sizeof(pstXsiCommon->version),
                      "ICF Version: V_%d.%d.%d build %d-%d-%d, MODEL Version: V_%d.%d.%d build 20%d-%d-%d, OBD Version: V_%d.%d.%d build 20%d-%d-%d, XSI Version: V_%d.%d.%d build 20%d-%d-%d",
                      stIcfAppVer.u32MajorVersion, stIcfAppVer.u32SubVersion, stIcfAppVer.u32RevisVersion,
                      stIcfAppVer.u32VersionYear, stIcfAppVer.u32VersionMonth, stIcfAppVer.u32VersionDay,
                      stModelInfo.ModelVersion.nMajorVersion, stModelInfo.ModelVersion.nSubVersion, stModelInfo.ModelVersion.nRevisVersion,
                      stModelInfo.ModelVersion.nVersionYear, stModelInfo.ModelVersion.nVersionMonth, stModelInfo.ModelVersion.nVersionDay,
                      stObdVersion.ModelVersion.nMajorVersion, stObdVersion.ModelVersion.nSubVersion, stObdVersion.ModelVersion.nRevisVersion,
                      stObdVersion.ModelVersion.nVersionYear, stObdVersion.ModelVersion.nVersionMonth, stObdVersion.ModelVersion.nVersionDay,
                      stXsiVersion.nMajorVersion, stXsiVersion.nSubVersion, stXsiVersion.nRevisVersion,
                      stXsiVersion.nVersionYear, stXsiVersion.nVersionMonth, stXsiVersion.nVersionDay);
    if (s32Ret <= 0 || s32Ret > sizeof(pstXsiCommon->version))
    {
        SVA_LOGE("snprintf errrrr! \n");
        return SAL_FAIL;
    }

    SVA_LOGI("Version_Info: %s \n", pstXsiCommon->version);
    s32Ret = SAL_SOK;

exit:
    return s32Ret;
}

/*******************************************************************************
* 函数名  : Sva_HalEngineDeInit
* 描  述  : 引擎资源去初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 注意点  : 此接口用于引擎资源退出+引擎句柄销毁
*******************************************************************************/
INT32 Sva_HalEngineDeInit(void)
{
    INT32 s32Ret = SAL_SOK;

    XSI_COMMON *pstXsiCommon = Sva_HalGetXsiCommon();

    SVA_HAL_CHECK_PRM(pstXsiCommon, SAL_FAIL);

    /* Free Icf Input Data Memory */
    s32Ret = Sva_HalIcfMemFreeInputData();
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Free Mem for Icf Failed! \n");
        return SAL_FAIL;
    }

    s32Ret = g_pEngineCommFunc->deinit();
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("engine deinit Failed! \n");
        return SAL_FAIL;
    }

    SVA_LOGI("Vcae DeInit Success!\n");
    return SAL_SOK;
}

/**
 * @function:   Sva_HalSetTarSizeFilterCfg
 * @brief:      设置目标尺寸过滤参数
 * @param[in]:  VOID *pHandle
 * @param[in]:  SVA_ALERT_SIZE_FILTER_PRM_S *pstSizeFilterPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_HalSetTarSizeFilterCfg(VOID *pChnHandle, SVA_ALERT_SIZE_FILTER_PRM_S *pstSizeFilterPrm)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 uiType = 0;

    ENGINE_COMM_USER_CONFIG_PRM stConfigPrm = {0};

    SVA_ALERT_SIZE_FILTER_CONFIG_S *pstFilterCfg = NULL;
    SVA_SIZE_FILTER_CONFIG_INFO_S *pstFilterCfgInfo = NULL;

    XSIE_SIZE_CONFIG_T stSizeFilterConfig = {0};

    SVA_LOGI("form app:prmcnt %d \n ", pstSizeFilterPrm->uiPrmCnt);

    for (i = 0; i < pstSizeFilterPrm->uiPrmCnt; i++)
    {
        pstFilterCfg = &pstSizeFilterPrm->stSizeFilterCfg[i];
        pstFilterCfgInfo = &pstFilterCfg->stFilterCfgInfo;

        SVA_LOGI("filter type %d, get target num %d \n", pstFilterCfg->enType, pstFilterCfgInfo->uiCnt);

        switch (pstFilterCfg->enType)
        {
            case TARGET_MIN_SIZE_FILTER:
            {
                memset(&stSizeFilterConfig, 0x00, sizeof(XSIE_SIZE_CONFIG_T));
                memset(&stConfigPrm, 0x00, sizeof(ENGINE_COMM_USER_CONFIG_PRM));

                stSizeFilterConfig.sub_config_type = XSIE_CONFIG_GPARAM_FLTSIZE_LOWER_WIDTH;
                for (j = 0; j < pstFilterCfgInfo->uiCnt; j++)
                {					
                    uiType = pstFilterCfgInfo->stFilterInfo[j].uiType;
					
                    stSizeFilterConfig.filter_size[uiType].enable_flg = 1; /*设置下来违禁品类别的尺寸过滤，默认有效*/
					stSizeFilterConfig.filter_size[uiType].class_type = uiType;
					stSizeFilterConfig.filter_size[uiType].value = pstFilterCfgInfo->stFilterInfo[j].uFilterSizeInfo.stRectInfo.uiW;

                    SVA_LOGI("filter type %d, j %d, target type %d, w %d \n", 
						     pstFilterCfg->enType, j, pstFilterCfgInfo->stFilterInfo[j].uiType, pstFilterCfgInfo->stFilterInfo[j].uFilterSizeInfo.stRectInfo.uiW);
                }

                stConfigPrm.pChnHandle = pChnHandle;
                stConfigPrm.u32NodeId = pstSizeFilterPrm->reserved[0];
                stConfigPrm.u32CfgKey = XSIE_XSI_CONFIG_TYPE_GPARAM;
                stConfigPrm.pData = (VOID *)&stSizeFilterConfig;
                stConfigPrm.u32DataSize = sizeof(XSIE_SIZE_CONFIG_T);

                s32Ret = g_pEngineCommFunc->set_config(&stConfigPrm);
                if (SAL_SOK != s32Ret)
                {
                    SVA_LOGE(" XSIE_CONFIG_GPARAM_FLTSIZE_LOWER_WIDTH fail, ret: 0x%x \n", s32Ret);
                    goto exit;
                }

                memset(&stSizeFilterConfig, 0x00, sizeof(XSIE_SIZE_CONFIG_T));

                stSizeFilterConfig.sub_config_type = XSIE_CONFIG_GPARAM_FLTSIZE_LOWER_HEIGHT;
                for (j = 0; j < pstFilterCfgInfo->uiCnt; j++)
                {			
                    uiType = pstFilterCfgInfo->stFilterInfo[j].uiType;
					
                    stSizeFilterConfig.filter_size[uiType].enable_flg = 1; /*设置下来违禁品类别的尺寸过滤，默认有效*/
					stSizeFilterConfig.filter_size[uiType].class_type = uiType;
					stSizeFilterConfig.filter_size[uiType].value = pstFilterCfgInfo->stFilterInfo[j].uFilterSizeInfo.stRectInfo.uiH;

                    SVA_LOGI("filter type %d, j %d, target type %d, h %d \n", 
						     pstFilterCfg->enType, j, pstFilterCfgInfo->stFilterInfo[j].uiType, pstFilterCfgInfo->stFilterInfo[j].uFilterSizeInfo.stRectInfo.uiH);
                }

                stConfigPrm.pChnHandle = pChnHandle;
                stConfigPrm.u32NodeId = pstSizeFilterPrm->reserved[0];
                stConfigPrm.u32CfgKey = XSIE_XSI_CONFIG_TYPE_GPARAM;
                stConfigPrm.pData = (VOID *)&stSizeFilterConfig;
                stConfigPrm.u32DataSize = sizeof(XSIE_SIZE_CONFIG_T);

                s32Ret = g_pEngineCommFunc->set_config(&stConfigPrm);
                if (SAL_SOK != s32Ret)
                {
                    SVA_LOGE(" XSIE_CONFIG_GPARAM_FLTSIZE_LOWER_WIDTH fail, ret: 0x%x \n", s32Ret);
                    goto exit;
                }

                break;
            }
            case TARGET_MAX_SIZE_FILTER:
            {
                memset(&stSizeFilterConfig, 0x00, sizeof(XSIE_SIZE_CONFIG_T));
                memset(&stConfigPrm, 0x00, sizeof(ENGINE_COMM_USER_CONFIG_PRM));

                stSizeFilterConfig.sub_config_type = XSIE_CONFIG_GPARAM_FLTSIZE_UPPER;
                for (j = 0; j < pstFilterCfgInfo->uiCnt; j++)
                {			
                    uiType = pstFilterCfgInfo->stFilterInfo[j].uiType;
					
                    stSizeFilterConfig.filter_size[uiType].enable_flg = 1; /*设置下来违禁品类别的尺寸过滤，默认有效*/
					stSizeFilterConfig.filter_size[uiType].class_type = uiType;
					stSizeFilterConfig.filter_size[uiType].value = pstFilterCfgInfo->stFilterInfo[j].uFilterSizeInfo.stDiagInfo.uiDiagLen;

                    SVA_LOGI("filter type %d, j %d, target type %d, get uiDiagLen:%d \n",
                             pstFilterCfg->enType, j, pstFilterCfgInfo->stFilterInfo[j].uiType, pstFilterCfgInfo->stFilterInfo[j].uFilterSizeInfo.stDiagInfo.uiDiagLen);
                }

                stConfigPrm.pChnHandle = pChnHandle;
                stConfigPrm.u32NodeId = pstSizeFilterPrm->reserved[0];
                stConfigPrm.u32CfgKey = XSIE_XSI_CONFIG_TYPE_GPARAM;
                stConfigPrm.pData = (VOID *)&stSizeFilterConfig;
                stConfigPrm.u32DataSize = sizeof(XSIE_SIZE_CONFIG_T);

                s32Ret = g_pEngineCommFunc->set_config(&stConfigPrm);
                if (SAL_SOK != s32Ret)
                {
                    SVA_LOGE(" XSIE_CONFIG_GPARAM_FLTSIZE_LOWER_WIDTH fail, ret: 0x%x \n", s32Ret);
                    goto exit;
                }

                break;
            }
            default:
            {
                SVA_LOGW("invalid size filter type %d, DO NOTHING... \n", pstFilterCfg->enType);
                break;
            }
        }
    }

    SVA_LOGI("Set Tar Size Filter end, node %d \n", pstSizeFilterPrm->reserved[0]);

exit:
    return s32Ret;
}

/**
 * @function:   Sva_HalSetDefSizeFilterPrm
 * @brief:      设置默认的目标尺寸过滤参数(像素)
 * @param[in]:  VOID *pHandle
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_HalSetDefSizeFilterPrm(UINT32 u32EngChnIdx, SVA_PROC_MODE_E enMode)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 uiTmpIdx = 0;

    XSI_COMMON *pXsiCommon = NULL;

    SVA_ALERT_SIZE_FILTER_PRM_S stSizeFilterPrm = {0};

    UINT32 uiDefMinCfgArr[14][3] =
    {
        /* type, w, h */
        {0, 40, 20},
        {3, 60, 33},
        {8, 40, 40},
        {11, 40, 20},
        {15, 33, 28},
        {16, 40, 32},
        {18, 55, 29},
        {20, 70, 40},
        {22, 40, 20},
        {23, 40, 20},
        {24, 40, 20},
        {30, 70, 40},
        {31, 70, 40},
        {32, 70, 40},
    };

    UINT32 uiDefMaxCfgArr[5][2] =
    {
        /* type, diaglen */
        {5, 329},
        {13, 91},
        {15, 191},
        {55, 329},
        {56, 329},
    };

    /* checker */
    if (u32EngChnIdx >= ENG_DEV_MAX)
    {
        SVA_LOGE("invalid engine channel index %d >= max %d \n", u32EngChnIdx, ENG_DEV_MAX);
        return SAL_FAIL;
    }

    uiTmpIdx = 0;

#if 1  /* 小尺寸过滤 */
    stSizeFilterPrm.stSizeFilterCfg[uiTmpIdx].enType = TARGET_MIN_SIZE_FILTER;

    for (i = 0; i < 14; i++)
    {
        stSizeFilterPrm.stSizeFilterCfg[uiTmpIdx].stFilterCfgInfo.stFilterInfo[i].uiType = uiDefMinCfgArr[i][0];
        stSizeFilterPrm.stSizeFilterCfg[uiTmpIdx].stFilterCfgInfo.stFilterInfo[i].uFilterSizeInfo.stRectInfo.uiW = uiDefMinCfgArr[i][1];
        stSizeFilterPrm.stSizeFilterCfg[uiTmpIdx].stFilterCfgInfo.stFilterInfo[i].uFilterSizeInfo.stRectInfo.uiH = uiDefMinCfgArr[i][2];
		//uiTarIdx++;
    }

    stSizeFilterPrm.stSizeFilterCfg[uiTmpIdx++].stFilterCfgInfo.uiCnt = i;
#endif

#if 1  /* 大尺寸过滤 */
    stSizeFilterPrm.stSizeFilterCfg[uiTmpIdx].enType = TARGET_MAX_SIZE_FILTER;

    for (i = 0; i < 5; i++)
    {
        stSizeFilterPrm.stSizeFilterCfg[uiTmpIdx].stFilterCfgInfo.stFilterInfo[i].uiType = uiDefMaxCfgArr[i][0];
        stSizeFilterPrm.stSizeFilterCfg[uiTmpIdx].stFilterCfgInfo.stFilterInfo[i].uFilterSizeInfo.stDiagInfo.uiDiagLen = uiDefMaxCfgArr[i][1];
    }

    stSizeFilterPrm.stSizeFilterCfg[uiTmpIdx++].stFilterCfgInfo.uiCnt = i;
#endif

    stSizeFilterPrm.uiPrmCnt = uiTmpIdx;

#ifdef SIZEFILTER
    if (SVA_PROC_MODE_IMAGE != enMode)
    {
        stSizeFilterPrm.reserved[0] = XSIE_XSI_NODE + u32EngChnIdx;
    }
    else
    {
        stSizeFilterPrm.reserved[0] = XSIE_XSI_NODE_1 + u32EngChnIdx;     /* 海思平台图片模式下用33 */
    }

#else
    if (SVA_PROC_MODE_DUAL_CORRELATION == enMode)
    {
        stSizeFilterPrm.reserved[0] = XSIE_XSI_NODE + u32EngChnIdx;
    }
    else
    {
        stSizeFilterPrm.reserved[0] = XSIE_XSI_NODE_1 + u32EngChnIdx;     /* RK平台图片和视频单路模式下用33 */
    }

#endif

    pXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pXsiCommon, SAL_FAIL);

    s32Ret = Sva_HalSetTarSizeFilterCfg(pXsiCommon->stEngChnPrm[u32EngChnIdx].pHandle, &stSizeFilterPrm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("set tar size filter failed! idx %d, node %d  \n", u32EngChnIdx, stSizeFilterPrm.reserved[0]);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   Sva_DrvSetPkgScaleRatio
 * @brief      设置包裹放缩最大倍数
 * @param[in]  UINT32 u32EngChnIdx
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvSetPkgScaleRatio(UINT32 u32EngChnIdx)
{
    INT32 s32Ret = SAL_FAIL;

    XSI_COMMON *pXsiCommon = NULL;

    XSIE_MAX_RESIZE_RATIO_CONFIG_INFO_T stResizeRatioConfig = {0};
    ENGINE_COMM_USER_CONFIG_PRM stConfigPrm = {0};

#if 1  /* 当前使用算法内部默认的包裹放缩阈值，且当前仅NT支持，RK暂不支持，外部暂不配置。 */
    return SAL_SOK;
#endif

    /* checker */
    if (u32EngChnIdx >= ENG_DEV_MAX)
    {
        SVA_LOGE("invalid engine channel index %d >= max %d \n", u32EngChnIdx, ENG_DEV_MAX);
        return SAL_FAIL;
    }

    pXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pXsiCommon, SAL_FAIL);

    stResizeRatioConfig.resize_ratio_contrl_flag = SAL_TRUE;
    stResizeRatioConfig.max_resize_ratio = 2.0f;

    stConfigPrm.pChnHandle = pXsiCommon->stEngChnPrm[u32EngChnIdx].pHandle;
    stConfigPrm.u32NodeId = XSIE_OBD_NODE_1 + u32EngChnIdx;
    stConfigPrm.u32CfgKey = XSIE_MAX_RESIZE_RATIO_CONFIG;
    stConfigPrm.pData = (VOID *)&stResizeRatioConfig;
    stConfigPrm.u32DataSize = sizeof(XSIE_MAX_RESIZE_RATIO_CONFIG_INFO_T);

    s32Ret = g_pEngineCommFunc->set_config(&stConfigPrm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("XSIE_MAX_RESIZE_RATIO_CONFIG fail ret=0x%x, node %d \n", s32Ret, stConfigPrm.u32NodeId);
        return SAL_FAIL;
    }

    return s32Ret;
}

/**
 * @function   Sva_DrvSetImgPdRgnThresh
 * @brief      图片模式下设置包裹割图检测阈值
 * @param[in]  UINT32 u32EngChnIdx
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvSetImgPdRgnThresh(UINT32 u32EngChnIdx)
{
    INT32 s32Ret = SAL_FAIL;

    XSI_COMMON *pXsiCommon = NULL;

    XSIE_PASTE_CONFIG_INFO_T stPasteConfig = { 0 };
    ENGINE_COMM_USER_CONFIG_PRM stConfigPrm = {0};

    /* 当前包裹割图检测阈设置功能默认开启，默认设置宽高阈值为640(当引擎输入数据为1280*1024时)，外部暂不配置。 */
    /* checker */
    if (u32EngChnIdx >= ENG_DEV_MAX)
    {
        SVA_LOGE("invalid engine channel index %d >= max %d \n", u32EngChnIdx, ENG_DEV_MAX);
        return SAL_FAIL;
    }

    pXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pXsiCommon, SAL_FAIL);

    stPasteConfig.filter_flag = 1;
    stPasteConfig.paste_rect.x = 0.0f;
    stPasteConfig.paste_rect.y = 0.0f;
    stPasteConfig.paste_rect.width = 0.5f;
    stPasteConfig.paste_rect.height = 0.625f;

    stConfigPrm.pChnHandle = pXsiCommon->stEngChnPrm[u32EngChnIdx].pHandle;
    stConfigPrm.u32NodeId = XSIE_PD_NODE_1 + u32EngChnIdx;
    stConfigPrm.u32CfgKey = XSIE_PASTE_THRS_CONFIG;
    stConfigPrm.pData = (VOID *)&stPasteConfig;
    stConfigPrm.u32DataSize = sizeof(XSIE_PASTE_CONFIG_INFO_T);

    s32Ret = g_pEngineCommFunc->set_config(&stConfigPrm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("XSIE_PASTE_THRS_CONFIG fail ret=0x%x, node %d \n", s32Ret, stConfigPrm.u32NodeId);
        return SAL_FAIL;
    }

    return s32Ret;
}

/**
 * @function   Sva_HalSetDefForceSplitFrmNum
 * @brief      设置默认强制分割帧数
 * @param[in]  UINT32 u32EngChnIdx
 * @param[in]  SVA_PROC_MODE_E enMode
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_HalSetDefForceSplitFrmNum(UINT32 u32EngChnIdx, SVA_PROC_MODE_E enMode)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 u32NodeId = 0;

    XSI_COMMON *pXsiCommon = NULL;

    VCA_KEY_PARAM stXsiStopValidFrmPrm = {0};
    ENGINE_COMM_USER_CONFIG_PRM stConfigPrm = {0};

    /* checker */
    if (u32EngChnIdx >= ENG_DEV_MAX)
    {
        SVA_LOGE("invalid engine channel index %d >= max %d \n", u32EngChnIdx, ENG_DEV_MAX);
        return SAL_FAIL;
    }

    pXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pXsiCommon, SAL_FAIL);

    if (SVA_PROC_MODE_DUAL_CORRELATION == enMode)
    {
        u32NodeId = XSIE_XSI_NODE;
    }
    else
    {
        u32NodeId = XSIE_XSI_NODE_1 + u32EngChnIdx;
    }

    stXsiStopValidFrmPrm.value = 600;
    stXsiStopValidFrmPrm.index = XSIE_CONFIG_STOPVALID_FRM;

    stConfigPrm.pChnHandle = pXsiCommon->stEngChnPrm[u32EngChnIdx].pHandle;;
    stConfigPrm.u32NodeId = u32NodeId;
    stConfigPrm.u32CfgKey = XSIE_SECURITE_SET_STOPVALID_FRM;
    stConfigPrm.pData = (VOID *)&stXsiStopValidFrmPrm;
    stConfigPrm.u32DataSize = sizeof(VCA_KEY_PARAM);

    s32Ret = g_pEngineCommFunc->set_config(&stConfigPrm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE(" XSIE_CONFIG_GPARAM_FLTSIZE_LOWER_WIDTH fail, ret: 0x%x \n", s32Ret);
        goto exit;
    }

    SVA_LOGI("xsi_stop_valid_frm end: %d \n", stXsiStopValidFrmPrm.value);

exit:
    return s32Ret;
}

/**
 * @function   Sva_HalSetFFTtemplateGap
 * @brief      设置fft模板帧更新间隔
 * @param[in]  UINT32 u32EngChnIdx
 * @param[in]  SVA_PROC_MODE_E enMode
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_HalSetFFTtemplateGap(UINT32 u32EngChnIdx, SVA_PROC_MODE_E enMode)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 u32NodeId = 0;

    XSI_COMMON *pXsiCommon = NULL;

    VCA_KEY_PARAM stXsiFftTemplateGap = {0};
    ENGINE_COMM_USER_CONFIG_PRM stConfigPrm = {0};

    /* checker */
    if (u32EngChnIdx >= ENG_DEV_MAX)
    {
        SVA_LOGE("invalid engine channel index %d >= max %d \n", u32EngChnIdx, ENG_DEV_MAX);
        return SAL_FAIL;
    }

    if (SVA_PROC_MODE_IMAGE == enMode)
    {
        SVA_LOGI("enmode %d not support set FFT templateGap \n", SVA_PROC_MODE_IMAGE);
        return SAL_SOK;
    }

    pXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pXsiCommon, SAL_FAIL);

    /* FFT模板帧更新间隔配置（若不设置默认为5，高速可略微减少，低速可略微提高，但必须大于0） */
    /* FFT模板帧更新间隔，视频模式下图像位移估计FFT模板帧每 xsi_fft_template_gap.value 更新一次 */
    /* 该设置仅在视频模式下生效，在图像模式无效，图像模式下请勿设置 */
    if (SVA_PROC_MODE_DUAL_CORRELATION == enMode)
    {
        u32NodeId = XSIE_XSI_NODE;
    }
    else if (SVA_PROC_MODE_PACK_DIV == enMode)
    {
        u32NodeId = XSIE_XSI_NODE_3;
    }
    else
    {
        u32NodeId = XSIE_XSI_NODE_1;
    }

    stXsiFftTemplateGap.value = 5;
    stXsiFftTemplateGap.index = XSIE_CONFIG_TEMPALTE_GAP;

    stConfigPrm.pChnHandle = pXsiCommon->stEngChnPrm[u32EngChnIdx].pHandle;
    stConfigPrm.u32NodeId = u32NodeId;
    stConfigPrm.u32CfgKey = XSIE_SECURITY_SET_TEMPALTE_GAP;
    stConfigPrm.pData = (VOID *)&stXsiFftTemplateGap;
    stConfigPrm.u32DataSize = sizeof(VCA_KEY_PARAM);

    s32Ret = g_pEngineCommFunc->set_config(&stConfigPrm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE(" XSIE_CONFIG_GPARAM_FLTSIZE_LOWER_WIDTH fail, ret: 0x%x \n", s32Ret);
        goto exit;
    }

    SVA_LOGI("success! xsi_fft_template_gap: %d \n", stXsiFftTemplateGap.value);

exit:
    return s32Ret;
}

/* 基线未使用，保留用于后续扩展，兼容安检盒子 */
#ifdef RK3588 /*当前只有rk3588支持该功能，该配置项仅在包裹分割模式使用，默认关闭*/

/**
 * @function   Sva_HalSetAllForwardPrm
 * @brief      设置过包方向，配置完后位移估计一直为正
 * @param[in]  VOID *pHandle
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_HalSetAllForwardPrm(UINT32 u32EngChnIdx, SVA_PROC_MODE_E enMode)
{
    INT32 s32Ret = SAL_FAIL;

    XSI_COMMON *pXsiCommon = NULL;

    VCA_KEY_PARAM stXsiAllForwardPrm = {0};
    ENGINE_COMM_USER_CONFIG_PRM stConfigPrm = {0};


    if (SVA_PROC_MODE_PACK_DIV != enMode)
    {
        SVA_LOGI("enmode %d not support packdiv \n", SVA_PROC_MODE_PACK_DIV);
        return SAL_SOK;
    }

    /* checker */
    if (u32EngChnIdx >= ENG_DEV_MAX)
    {
        SVA_LOGE("invalid engine channel index %d >= max %d \n", u32EngChnIdx, ENG_DEV_MAX);
        return SAL_FAIL;
    }

    pXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pXsiCommon, SAL_FAIL);

    stXsiAllForwardPrm.value = 1;
    stXsiAllForwardPrm.index = XSIE_CONFIG_ALLFORWARD_FLAG;

    stConfigPrm.pChnHandle = pXsiCommon->stEngChnPrm[u32EngChnIdx].pHandle;
    stConfigPrm.u32NodeId = XSIE_XSI_NODE_3;
    stConfigPrm.u32CfgKey = XSIE_SECURITE_SET_ALLFORWARD_FLAG;
    stConfigPrm.pData = (VOID *)&stXsiAllForwardPrm;
    stConfigPrm.u32DataSize = sizeof(VCA_KEY_PARAM);

    s32Ret = g_pEngineCommFunc->set_config(&stConfigPrm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE(" XSIE_SECURITE_SET_ALLFORWARD_FLAG fail, ret: 0x%x \n", s32Ret);
        goto exit;
    }

    SVA_LOGI("XSIE_SET_ALL_FORWARD SUCESS\n");

exit:
    return s32Ret;
}

#endif

#ifdef RK3588

/**
 * @function   Sva_DrvSetImgPkgCombine
 * @brief      图片模式下设置包裹合并处理个数
 * @param[in]  UINT32 u32EngChnIdx
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvSetImgPkgCombine(UINT32 u32EngChnIdx)
{
    INT32 s32Ret = SAL_FAIL;

    XSI_COMMON *pXsiCommon = NULL;

    ENGINE_COMM_USER_CONFIG_PRM stConfigPrm = {0};
    XSIE_MAX_PACK_NUM_SET_INFO_T stPackCombNumPrm = {0};

    /* checker */
    if (u32EngChnIdx >= ENG_DEV_MAX)
    {
        SVA_LOGE("invalid engine channel index %d >= max %d \n", u32EngChnIdx, ENG_DEV_MAX);
        return SAL_FAIL;
    }

    pXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pXsiCommon, SAL_FAIL);

    stPackCombNumPrm.filter_flag = 1;         /* 开关，设置1为开，0为关*/
    stPackCombNumPrm.max_proc_pack_num = 2;   /* 包裹个数达到合并的条件 */

    stConfigPrm.pChnHandle = pXsiCommon->stEngChnPrm[u32EngChnIdx].pHandle;
    stConfigPrm.u32NodeId = XSIE_OBD_NODE_1 + u32EngChnIdx;
    stConfigPrm.u32CfgKey = XSIE_SET_MAX_PACK_PROC_NUM;
    stConfigPrm.pData = (VOID *)&stPackCombNumPrm;
    stConfigPrm.u32DataSize = sizeof(XSIE_MAX_PACK_NUM_SET_INFO_T);

    s32Ret = g_pEngineCommFunc->set_config(&stConfigPrm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("XSIE_PASTE_THRS_CONFIG fail ret=0x%x, node %d \n", s32Ret, stConfigPrm.u32NodeId);
        return SAL_FAIL;
    }

    return s32Ret;
}

#endif

/**
 * @function:   Sva_HalGetDefConf
 * @brief:      设置算法检测置信度
 * @param[in]:  UINT32 chan
 * @param[out]: SVA_ALERT_CONF *pstSetAlertConf
 * @return:     INT32
 */
INT32 Sva_HalSetAlertConf(UINT32 chan, SVA_ALERT_CONF *pstSetAlertConf)
{
	INT32 s32Ret = SAL_SOK;
	UINT32 i = 0;
	
	VOID *pHandle = NULL;
	XSI_COMMON *pXsiCommon = NULL;
	
	ENGINE_COMM_USER_CONFIG_PRM stConfigPrm = {0};
	XSIE_CONF_CONFIG_T conf_param_cfg = {0};  
	
	pXsiCommon = Sva_HalGetXsiCommon();
	pHandle = pXsiCommon->stEngChnPrm[chan].pHandle;

	for (i = 0; i < pstSetAlertConf->sva_cnt; i++)
	{
		if (pstSetAlertConf->sva_type[i] > (SVA_MAX_ALARM_TYPE - 1))
		{
			SVA_LOGE("Type (%d) or conf (%d) is Illegal !\n", pstSetAlertConf->sva_type[i], pstSetAlertConf->sva_conf[i]);
			return SAL_FAIL;
		}
		/*超过引擎配置的范围(6,99)，强制修正置信度在(6,99)内，由于引擎内部只能拿到0.15置信度以上的危险品*/
		if (pstSetAlertConf->sva_conf[i] <= 5)
		{
			pstSetAlertConf->sva_conf[i] = 6;
		}
		if (pstSetAlertConf->sva_conf[i] >= 100)
		{
			pstSetAlertConf->sva_conf[i] = 99;	
		}
		conf_param_cfg.conf_list[i].enable_flg = 1; /*配置为1时生效*/
        conf_param_cfg.conf_list[i].class_type = pstSetAlertConf->sva_type[i];
		conf_param_cfg.conf_list[i].value= pstSetAlertConf->sva_conf[i];
		
		SVA_LOGI("chan %d Alert Type %d conf %d sva_cnt %d \n", chan, pstSetAlertConf->sva_type[i], pstSetAlertConf->sva_conf[i], pstSetAlertConf->sva_cnt);
	}

    stConfigPrm.pChnHandle = pHandle;
	stConfigPrm.u32NodeId = XSIE_XSI_NODE_1 + chan;
	stConfigPrm.u32CfgKey = XSIE_SET_CONFIG_TYPE_SENSITY;
	stConfigPrm.pData = &conf_param_cfg;
	stConfigPrm.u32DataSize = sizeof(XSIE_CONF_CONFIG_T);

	s32Ret = g_pEngineCommFunc->set_config(&stConfigPrm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE(" XSIE_SET_CONFIG_TYPE_SENSITY fail, ret: 0x%x \n", s32Ret);
        return SAL_FAIL;
    }
	return s32Ret;
}

/**
 * @function:   Sva_HalGetNormalConf
 * @brief:      获取当前置信度(0~100)
 * @param[in]:  UINT32 chan
 * @param[out]: INT32 *uiDefNormalConfTab
 * @return:     INT32
 */
INT32 Sva_HalGetNormalConf(UINT32 chan, INT32 *uiDefNormalConfTab)
{
    INT32 s32Ret = SAL_SOK; 
	
	VOID *pHandle = NULL;
	XSI_COMMON *pXsiCommon = NULL;
	ENGINE_COMM_USER_CONFIG_PRM stConfigPrm = {0};
	XSIE_CONF_CONFIG_T conf_param_cfg = {0};      

	pXsiCommon = Sva_HalGetXsiCommon();

	/* 双路模式默认置信度都一样，获取一路的就可以 */
	pHandle = pXsiCommon->stEngChnPrm[chan].pHandle;
	
    /*获取当前置信度*/
	memset(&conf_param_cfg, 0, sizeof(XSIE_CONF_CONFIG_T));

	stConfigPrm.pChnHandle = pHandle;
	stConfigPrm.u32NodeId = XSIE_XSI_NODE_1 + chan;
	stConfigPrm.u32CfgKey = XSIE_SECURITE_GET_TYPE_SESITY;
	stConfigPrm.pData = &conf_param_cfg;
	stConfigPrm.u32DataSize = sizeof(XSIE_CONF_CONFIG_T);
	
	s32Ret = g_pEngineCommFunc->get_config(&stConfigPrm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE(" XSIE_SECURITE_GET_TYPE_SESITY fail, ret: 0x%x \n", s32Ret);
        return SAL_FAIL;
    }
	
	for (int i = 0; i < SVA_MAX_ALARM_TYPE; i++)
	{
	    /*未启用的直接跳过*/
		if (0 == conf_param_cfg.conf_list[i].enable_flg)
		{
			continue;
		}
	    uiDefNormalConfTab[conf_param_cfg.conf_list[i].class_type] = conf_param_cfg.conf_list[i].value;
		SVA_LOGD("type_%d, conf_%d enable_flg_%d\n", conf_param_cfg.conf_list[i].class_type, conf_param_cfg.conf_list[i].value,conf_param_cfg.conf_list[i].enable_flg);
	}
    return s32Ret;
}

/**
 * @function:   Sva_HalGetDefPkConf
 * @brief:      获取默认pk置信度(0~100)
 * @param[in]:  UINT32 chan
 * @param[out]: INT32 *uiDefPkConfTab
 * @return:     INT32
 */
INT32 Sva_HalGetDefPkConf(UINT32 chan, INT32 *uiDefPkConfTab)
{
    INT32 s32Ret = SAL_SOK; 
	
	VOID *pHandle = NULL;
	XSI_COMMON *pXsiCommon = NULL;
	ENGINE_COMM_USER_CONFIG_PRM stConfigPrm = {0};
	XSIE_CONF_CONFIG_T conf_param_cfg = {0};      

	pXsiCommon = Sva_HalGetXsiCommon();

	/* 双路模式默认置信度都一样，获取一路的就可以 */
	pHandle = pXsiCommon->stEngChnPrm[chan].pHandle;
	
	/*获取默认PK置信度*/
	memset(&conf_param_cfg, 0, sizeof(XSIE_CONF_CONFIG_T));

	stConfigPrm.pChnHandle = pHandle;
	stConfigPrm.u32NodeId = XSIE_XSI_NODE_1 + chan;
	stConfigPrm.u32CfgKey = XSIE_SECURITE_GET_PK_SESITY;
	stConfigPrm.pData = &conf_param_cfg;
	stConfigPrm.u32DataSize = sizeof(XSIE_CONF_CONFIG_T);
	
	s32Ret = g_pEngineCommFunc->get_config(&stConfigPrm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE(" XSIE_SECURITE_GET_PK_SESITY fail, ret: 0x%x \n", s32Ret);
        return SAL_FAIL;
    }
	
	for (int i = 0; i < SVA_MAX_ALARM_TYPE; i++)
	{
	    /*未启用的直接跳过*/
		if (0 == conf_param_cfg.conf_list[i].enable_flg)
		{
			continue;
		}
	    uiDefPkConfTab[conf_param_cfg.conf_list[i].class_type] = conf_param_cfg.conf_list[i].value;
		SVA_LOGD("type_%d, conf_%d enable_flg_%d\n", conf_param_cfg.conf_list[i].class_type, conf_param_cfg.conf_list[i].value,conf_param_cfg.conf_list[i].enable_flg);
	}
    return s32Ret;
}

/**
 * @function:   Sva_HalGetDefConf
 * @brief:      从默认置信度表中获取默认置信度(0~100)
 * @param[in]:  UINT32 chan
 * @param[out]: SVA_DSP_DEF_CONF_OUT *pstXsiDefConf
 * @return:     INT32
 */
INT32 Sva_HalGetDefConf(UINT32 chan, SVA_DSP_DEF_CONF_OUT *pstXsiDefConf)
{
    INT32 s32Ret = SAL_SOK; 
    UINT32 i = 0;
	
	/*此处从默认置信度表格(引擎完成初始化后获得)获取，目前只返回普通XSI支持的前128类*/
	if (SVA_CONF_MODE_NORMAL == pstXsiDefConf->enConfMode)
	{
		for (i = 0; i < SVA_XSI_MAX_ALARM_TYPE; i++)
		{
			pstXsiDefConf->s32DefConf[i] = g_s32DefNormalConfTab[i];
		}
	}
	if (SVA_CONF_MODE_PK == pstXsiDefConf->enConfMode)
	{
		for (i = 0; i < SVA_XSI_MAX_ALARM_TYPE; i++)
		{
			pstXsiDefConf->s32DefConf[i] = g_s32DefPkConfTab[i];
		}
	} 
	
	SVA_LOGI("chan %d, get def conf! mode(0:normal, 1:pk) %d! \n", chan, pstXsiDefConf->enConfMode);
	for (i = 0; i < SVA_XSI_MAX_ALARM_TYPE; i++)
	{
		if (0 == pstXsiDefConf->s32DefConf[i])
		{
			continue;
		}
		
		SVA_LOGI("type %d, conf %d \n", i, pstXsiDefConf->s32DefConf[i]);
	}
    return s32Ret;
}

/**
 * @function:   Sva_HalSetIcfDefaultPrm
 * @brief:      设置引擎使用的默认参数
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_HalSetIcfDefaultPrm(SVA_PROC_MODE_E enMode)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 u32AiChnNum = 0;

    CAPB_AI *pstAiCapb = NULL;

    if (NULL != (pstAiCapb = capb_get_ai()))
    {
        u32AiChnNum = pstAiCapb->ai_chn;
    }

    for (i = 0; i < u32AiChnNum; i++)
    {
        /* 设置目标大小过滤参数，单位像素 */
        s32Ret = Sva_HalSetDefSizeFilterPrm(i, enMode);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Sva_HalSetDefSizeFilterPrm failed! ret: 0x%x \n", s32Ret);
            return SAL_FAIL;
        }

        /* 设置OBD最大放缩阈值，默认是2.0，各个模式均可使用 */
        s32Ret = Sva_DrvSetPkgScaleRatio(i);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Sva_DrvSetImgPkgScaleRatio failed! ret: 0x%x \n", s32Ret);
            return SAL_FAIL;
        }

        /* 图片模式下支持对xpack填充的包裹割图进行检测阈值校验，若大于配置的割图分辨率则引擎内部对割图进行pd+obd，否则以传入的包裹割图为包裹单位跳过pd直接obd。
           默认包裹割图检测开启，阈值为宽640高640，仅支持图片模式配置 */
        if (SVA_PROC_MODE_IMAGE == enMode)
        {
            s32Ret = Sva_DrvSetImgPdRgnThresh(i);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("Sva_DrvSetImgPdRgnThresh failed! ret: 0x%x \n", s32Ret);
                return SAL_FAIL;
            }

#ifdef RK3588
            s32Ret = Sva_DrvSetImgPkgCombine(i);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("Sva_DrvSetImgPkgCombine failed! ret: 0x%x \n", s32Ret);
                return SAL_FAIL;
            }
	
			/*从引擎获取当前置信度，即正常状态下的默认置信度(两个通道相同，默认用0通道)*/
			s32Ret = Sva_HalGetNormalConf(0, g_s32DefNormalConfTab);
		    if (SAL_SOK != s32Ret)
		    {
		        SVA_LOGE("Sva_HalGetDefConf Failed!!! \n");
		        return SAL_FAIL;
		    }  
			/*从引擎获取默认pk置信度*/			
		    s32Ret = Sva_HalGetDefPkConf(0, g_s32DefPkConfTab);
		    if (SAL_SOK != s32Ret)
		    {
		        SVA_LOGE("Sva_HalGetDefConf Failed!!! \n");
		        return SAL_FAIL;
		    }  
#endif
        }

        /* 强制分割帧数和FFT模板帧间隔两个配置仅在视频模式和双路关联模式下使用，图片模式不适用 */
        if (SVA_PROC_MODE_IMAGE != enMode)
        {
            /* 设置强制分割帧数 */
            s32Ret = Sva_HalSetDefForceSplitFrmNum(i, enMode);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("Sva_HalSetDefForceSplitFrmNum failed! ret: 0x%x \n", s32Ret);
                return SAL_FAIL;
            }

            /* 设置fft模板帧更新间隔 */
            s32Ret = Sva_HalSetFFTtemplateGap(i, enMode);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("Sva_HalSetFFTtemplateGap failed! ret: 0x%x \n", s32Ret);
                return SAL_FAIL;
            }

#ifdef RK3588 /* 当前只有rk平台支持，设置过包方向一直前向计算，位移估计一直为正 */
            s32Ret = Sva_HalSetAllForwardPrm(i, enMode);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("Sva_HalSetAllForwardPrm failed! ret: 0x%x \n", s32Ret);
                return SAL_FAIL;
            }

#endif
        }
    }

    SVA_LOGI("%s: end! \n", __FUNCTION__);
    return SAL_SOK;
}

/**
 * @function:   Sva_HalModeGraphMap
 * @brief:      处理模式映射到graph
 * @param[in]:  None
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_HalModeGraphMap(SVA_PROC_MODE_E enMode, UINT32 *pu32GraphType)
{
    UINT32 uiGraphType = 2;

    /* checker */
    if (enMode >= SVA_PROC_MODE_NUM)
    {
        SVA_LOGE("Invalid Proc Mode %d, Pls Check! \n", enMode);
        return SAL_FAIL;
    }

    if (SVA_PROC_MODE_IMAGE == enMode)
    {
        uiGraphType = 1;
    }
    else if (SVA_PROC_MODE_VIDEO == enMode)
    {
        uiGraphType = 2;
    }
    else if (SVA_PROC_MODE_DUAL_CORRELATION == enMode)
    {
        uiGraphType = 3;
    }
    else
    {
        SVA_LOGE("Invalid mode %d!!! \n", enMode);
        return SAL_FAIL;
    }

    *pu32GraphType = uiGraphType;
    return SAL_SOK;
}

/**
 * @function    Sva_HalInitEngine
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Sva_HalInitEngine(const SVA_PROC_MODE_E enMode)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;
    UINT32 u32GraphType = 0;
    ENGINE_COMM_INIT_PRM_S stIcfCommInitPrm = {0};
    ENGINE_COMM_INIT_OUTPUT_PRM stIcfCommInitOut = {0};

    ENGINE_COMM_CHN_ATTR *pstIcfCommChnAttr = NULL;
    CAPB_AI *pstCapbAi = NULL;

    XSI_COMMON *pXsiCommon = Sva_HalGetXsiCommon();

    /* 结果处理回调函数 */
    stIcfCommInitPrm.pRsltCbFunc = (ENGINE_COMM_RSLT_CB_FUNC)pCbFunc;

    /* 配置文件 */
    stIcfCommInitPrm.stCfgFileInfo.pAlgCfgPath = g_cfg_path[0];
    stIcfCommInitPrm.stCfgFileInfo.pTaskCfgPath = g_cfg_path[1];
    stIcfCommInitPrm.stCfgFileInfo.pToolkitCfgPath = g_cfg_path[2];

    pstCapbAi = capb_get_ai();
    if (NULL == pstCapbAi)
    {
        SVA_LOGE("capb_get_ai null! \n");
        return SAL_FAIL;
    }

    /* 引擎通道配置信息 */
    stIcfCommInitPrm.stInitChnCfg.u32ChnNum = pstCapbAi->ai_chn; /* szl_dbg: 根据业务层的能力进行判断 */
    for (i = 0; i < stIcfCommInitPrm.stInitChnCfg.u32ChnNum; i++)
    {
        pstIcfCommChnAttr = &stIcfCommInitPrm.stInitChnCfg.astChnAttr[i];

        s32Ret = Sva_HalModeGraphMap(enMode, &u32GraphType);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Decrypt proc fail ret: 0x%x \n", s32Ret);
            goto exit;
        }

        /*如果是双路,具体的u32GraphId 和u32NodeId结合对应的icf heade文件进行计算*/
        pstIcfCommChnAttr->u32GraphType = u32GraphType;
        pstIcfCommChnAttr->u32GraphId = i + 1;
        pstIcfCommChnAttr->pAppParamCfgPath = g_cfg_path[3];
        pstIcfCommChnAttr->u32NodeId = XSIE_POST_NODE_1 + i;
        pstIcfCommChnAttr->u32CallBackType = ENGINE_CALLBACK_OUTPUT;
    }

    /* 内存配置信息 */
    IA_GetModMemInitSize(IA_MOD_SVA, stIcfCommInitPrm.stInitMemCfg.u32MemSize);
    stIcfCommInitPrm.stInitMemCfg.pMemAllocFunc = (ENGINE_COMM_MEM_ALLOC_CB_FUNC)Sva_HalMemPoolSystemMallocCb;
    stIcfCommInitPrm.stInitMemCfg.pMemRlsFunc = (ENGINE_COMM_MEM_RLS_CB_FUNC)Sva_HalMemPoolSystemFreeCb;

    /* xsie入参结构体大小 */
    stIcfCommInitPrm.u32IaPrvtDataSize = sizeof(XSIE_SECURITY_INPUT_T);

    s32Ret = g_pEngineCommFunc->init(&stIcfCommInitPrm, &stIcfCommInitOut);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("engine init fail ret: 0x%x \n", s32Ret);
        goto exit;
    }

    if (stIcfCommInitOut.s32InitStatus || stIcfCommInitPrm.stInitChnCfg.u32ChnNum != stIcfCommInitOut.stInitChnOutput.u32ChnNum)
    {
        SAL_LOGE("icf common init err! sts %d, init chn %d != out chn %d \n",
                 stIcfCommInitOut.s32InitStatus,
                 stIcfCommInitPrm.stInitChnCfg.u32ChnNum,
                 stIcfCommInitOut.stInitChnOutput.u32ChnNum);
        goto exit;
    }

    pXsiCommon->uiChannelNum = stIcfCommInitPrm.stInitChnCfg.u32ChnNum;
    for (i = 0; i < stIcfCommInitPrm.stInitChnCfg.u32ChnNum; i++)
    {
        pXsiCommon->stEngChnPrm[i].pHandle = stIcfCommInitOut.stInitChnOutput.pChnHandle[i];
    }

exit:
    return s32Ret;
}

/*******************************************************************************
* 函数名  : Sva_HalFreeInputAllIndex
* 描  述  : 清空所有占用Input索引项
* 输  入  : - chan : 通道号
*         : - mode : 模式
* 输  出  : - pIdx : 索引值指针
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalFreeInputAllIndex(UINT32 chan, SVA_PROC_MODE_E mode)
{
    UINT32 uiChnId = 0;
    UINT32 i = 0;

    /* XSI_DEV *pstXsi = NULL; */
    ENG_CHANNEL_PRM_S *pstEngChnPrm = NULL;
    SVA_INPUT_DATA *pstInputDataInfo = NULL;

    /* Input Args Checker */
    SVA_HAL_CHECK_ENG_CHN(chan, SAL_FAIL);

    /* TODO: 目前仅有图片模式支持双通道独立检测，关联模式和视频模式均是单通道处理，使用通道0 */
    uiChnId = (mode == SVA_PROC_MODE_IMAGE ? chan : 0);

    pstEngChnPrm = Sva_HalGetEngchnPrm(uiChnId);
    SVA_HAL_CHECK_PRM(pstEngChnPrm, SAL_FAIL);

    for (i = 0; i < SVA_INPUT_BUF_LEN; i++)
    {
        pstInputDataInfo = (SVA_INPUT_DATA *)&pstEngChnPrm->astSvaInputData[i];

        if (SAL_TRUE == pstInputDataInfo->bUseFlag)
        {
            pstInputDataInfo->bUseFlag = SAL_FALSE;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalSetVcaePrm
* 描  述  : 设置引擎参数
* 输  入  : - void:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 注意点  : 模式切换需要退出引擎资源(句柄等)，然后设置模式后重新创建引擎句柄
            该接口不可重入，为避免上层调用错误，此处添加简单保护
*******************************************************************************/
INT32 Sva_HalSetVcaePrm(void *prm)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 time0 = 0;
    UINT32 time1 = 0;
    INT32 i = 0;

    /*需要修改的配置文件参数结构体*/
    IA_ISA_UPDATA_CFG_JSON_PARA stUpJsonPrm = {0};
    SVA_VCAE_PRM_ST *pstVacePrm = NULL;

    SVA_HAL_CHECK_PRM(prm, SAL_FAIL);

    time0 = SAL_getCurMs();

    /* Localize Input Args */
    pstVacePrm = (SVA_VCAE_PRM_ST *)prm;

    SVA_LOGI("Set Proc Mode %d entering! %p, %d \n", pstVacePrm->enProcMode, (void *)pstVacePrm, pstVacePrm->uiAiEnable);

    for (i = 0; i < XSI_DEV_MAX; i++)
    {
        if (SAL_SOK != Sva_HalVcaeStop(i))
        {
            SVA_LOGE("Stop Vcae Failed! \n");
            return SAL_FAIL;
        }

        SVA_LOGI("Stop Module %d End \n", i);
    }

    /* Free Input Data Mem */
    s32Ret = Sva_HalIcfMemFreeInputData();
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Icf Free Input Data Mem Failed! \n");
        goto EXIT;
    }

    s32Ret = g_pEngineCommFunc->deinit();
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("engine deinit Failed! \n");
        goto EXIT;
    }

    /* Sva Mem Deinit */
    s32Ret = Ia_ResetModMem(IA_MOD_SVA);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Sva Mem Deinit Failed! \n");
        goto EXIT;
    }

    /* Modify Cfg Json Mode pro */
    stUpJsonPrm.enChgType = CHANGE_WORK_TYPE;
    stUpJsonPrm.enWorkType = pstVacePrm->enProcMode;
    if (SAL_SOK != Sva_IsaHalModifyCfgFile(&stUpJsonPrm))
    {
        SVA_LOGE("Modify Cfg File Failed! mode %d\n", stUpJsonPrm.enWorkType);
        goto EXIT;
    }

    /* Modify Cfg Json Mode Enbale Switch */
    stUpJsonPrm.enChgType = CHANGE_AI_TYPE;
    stUpJsonPrm.uiAiEnable = pstVacePrm->uiAiEnable;
    if (SAL_SOK != Sva_IsaHalModifyCfgFile(&stUpJsonPrm))
    {
        SVA_LOGE("Modify Cfg File Failed! ai_flag %d \n", stUpJsonPrm.uiAiEnable);
        goto EXIT;
    }

    if (SAL_SOK != Sva_HalInitEngine(pstVacePrm->enProcMode))
    {
        SVA_LOGE("Sva: init engine Failed! \n");
        goto EXIT;
    }

    /* get version info */
    if (SAL_SOK != Sva_HalGetIcfVersion(0, pstVacePrm->enProcMode))
    {
        SVA_LOGE("Sva: get version info Failed! \n");
        goto EXIT;
    }

    /* Set engine default param */
    if (SAL_SOK != Sva_HalSetIcfDefaultPrm(pstVacePrm->enProcMode))
    {
        SVA_LOGE("Sva: Set ICF Default Param Failed! \n");
        goto EXIT;
    }

    /* Malloc Memory for Icf Input Data */
    if (SAL_SOK != Sva_HalIcfMemAllocInputData())
    {
        SVA_LOGE("Sva: Icf Alloc Mem for Input Data Failed! \n");
        goto EXIT;
    }

    /* Initialize Input Data Param, including img-info and img-format, etc */
    if (SAL_SOK != Sva_HalInitInputDataPrm(pstVacePrm->enProcMode, pstVacePrm->uiAiEnable))
    {
        SVA_LOGE("Sva: Init Input Data Param Failed! \n");
        goto EXIT;
    }

    /* 每次切换引擎模式时清空所有空闲Input索引项 */
    for (i = 0; i < XSI_DEV_MAX; i++)
    {
        if (SAL_SOK != Sva_HalFreeInputAllIndex(i, pstVacePrm->enProcMode))
        {
            SVA_LOGE("Sva: free Input All index Failed! \n");
            goto EXIT;
        }
    }

    time1 = SAL_getCurMs();

    SVA_LOGI("Set Proc Mode %d, Ai Flag %d End! %d \n", pstVacePrm->enProcMode, pstVacePrm->uiAiEnable, time1 - time0);
    return SAL_SOK;

EXIT:
    (VOID)Sva_HalPrintCfgInfo();
    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Sva_HalGetInitMode
* 描  述  :
* 输  入  : - void:
* 输  出  : 无
* 返回值  : 初始化模式类型
*******************************************************************************/
static SVA_PROC_MODE_E Sva_HalGetInitMode(void)
{
    return enInitMode;
}

/*******************************************************************************
* 函数名  : Sva_HalSetInitMode
* 描  述  : 设置初始化模式
* 输  入  : - mode: 模式
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
void Sva_HalSetInitMode(SVA_PROC_MODE_E mode)
{
    enInitMode = mode >= SVA_PROC_MODE_NUM ? SVA_PROC_MODE_DUAL_CORRELATION : mode;
    return;
}

/*******************************************************************************
* 函数名  : Sva_HalEngineInit
* 描  述  : 引擎资源初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalEngineInit(void)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 time0 = 0;
    UINT32 time1 = 0;
    SVA_PROC_MODE_E enMode = Sva_HalGetInitMode();
    /* CAPB_PRODUCT *pstProduct = capb_get_product(); */
    XSI_COMMON *pXsiCommon = NULL;
    CAPB_AI *pstAiCapPrm = NULL;

    /* IA_UPDATA_CFG_JSON_PARA stUpJsonPrm = {0}; */

    pXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pXsiCommon, SAL_FAIL);
    time0 = SAL_getCurMs();

    /*获取能力集*/
    pstAiCapPrm = capb_get_ai();
    if (NULL == pstAiCapPrm)
    {
        SVA_LOGE("NULL err\n");
        return SAL_FAIL;
    }

    /* Initialize Encrypt Related Resource */
    s32Ret = IA_InitEncrypt(&pXsiCommon->alg_encrypt_hdl);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("IA_InitEncrypt fail ret: 0x%x \n", s32Ret);
        goto EXIT;
    }

    /* 初始化平台层智能硬核资源，包括npu、cpu、ipu等 */
    if (SAL_SOK != IA_InitHwCore())
    {
        SVA_LOGE("Sva: Init Dsp Core Failed!\n");
        goto EXIT;
    }

    /* init icf common func cb */
    g_pEngineCommFunc = engine_comm_register();
    if (NULL == g_pEngineCommFunc)
    {
        SVA_LOGE("icf comm register failed! \n");
        goto EXIT;
    }

    /* init engine */
    if (SAL_SOK != Sva_HalInitEngine(enMode))
    {
        SVA_LOGE("Sva: init engine Failed! \n");
        goto EXIT;
    }

    /* get version info */
    if (SAL_SOK != Sva_HalGetIcfVersion(0, enMode))
    {
        SVA_LOGE("Sva: get version info Failed! \n");
        goto EXIT;
    }

    /* Set engine default param */
    if (SAL_SOK != Sva_HalSetIcfDefaultPrm(enMode))
    {
        SVA_LOGE("Sva: Set ICF Default Param Failed! \n");
        goto EXIT;
    }

    /* Malloc Memory for Icf Input Data */
    if (SAL_SOK != Sva_HalIcfMemAllocInputData())
    {
        SVA_LOGE("Sva: Icf Alloc Mem for Input Data Failed! \n");
        goto EXIT;
    }

    /* Initialize Input Data Param, including img-info and img-format, etc */
    if (SAL_SOK != Sva_HalInitInputDataPrm(enMode, SAL_FALSE))
    {
        SVA_LOGE("Sva: Init Input Data Param Failed! \n");
        goto EXIT;
    }

    time1 = SAL_getCurMs();

    SVA_LOGI("Sva: Init proc mode %d End! %d \n", enMode, time1 - time0);
    return SAL_SOK;

EXIT:
    (VOID)Sva_HalPrintCfgInfo();

    return SAL_FAIL;

}

/*******************************************************************************
* 函数名  : Sva_HalVcaeStart
* 描  述  : 开启引擎通道
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalVcaeStart(UINT32 chan)
{
    XSI_DEV *pstXsi = NULL;

    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);

    pstXsi = Sva_HalGetDev(chan);

    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    SAL_mutexLock(pstXsi->mChnMutexHdl);

    if (pstXsi->xsi_status == SAL_FALSE)
    {
        pstXsi->xsi_status = SAL_TRUE;
    }
    else
    {
        SVA_LOGI("chan[%d] is started! \n", chan);
    }

    SAL_mutexUnlock(pstXsi->mChnMutexHdl);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalVcaeStop
* 描  述  : 停止引擎
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalVcaeStop(UINT32 chan)
{
    XSI_DEV *pstXsi = NULL;

    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);

    pstXsi = Sva_HalGetDev(chan);
    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    SAL_mutexLock(pstXsi->mChnMutexHdl);

    if (pstXsi->xsi_status == SAL_TRUE)
    {
        pstXsi->xsi_status = SAL_FALSE;
        pstXsi->reaultCnt = 0;
    }
    else
    {
        SVA_LOGI("engine is stopped! chan[%d] \n", chan);
    }

    pstXsi->uiFrameNum = 0;

    SAL_mutexUnlock(pstXsi->mChnMutexHdl);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalGetVersion
* 描  述  : 获取版本信息
* 输  入  : - prm: 版本信息
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalGetVersion(VOID *prm)
{
    INT32 len = 0;
    XSI_COMMON *pXsiCommon = NULL;

    SVA_HAL_CHECK_PRM(prm, SAL_FAIL);

    pXsiCommon = Sva_HalGetXsiCommon();

    SVA_HAL_CHECK_PRM(pXsiCommon, SAL_FAIL);
    len = strlen((char *)pXsiCommon->version);
    if (SAL_SOK == len)
    {
        return SAL_FAIL;
    }

    sal_memcpy_s((unsigned char *)prm, sizeof(pXsiCommon->version),
                 pXsiCommon->version, strlen((char *)pXsiCommon->version));

    SVA_LOGI("version : %s\n", pXsiCommon->version);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalUpDateSvaResult
* 描  述  : 更新算法结果
* 输  入  : - pstSrc: 拷贝源变量
*         : - pstDst: 拷贝目的变量
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalCpySvaResult(SVA_PROCESS_OUT *pstSrc, SVA_PROCESS_OUT *pstDst)
{
    UINT32 i = 0;

    SVA_HAL_CHECK_PRM(pstSrc, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstDst, SAL_FAIL);

#if 0
    /* 上层应用相关配置参数，动态更新 */
    pstDst->stTargetPrm.confidence = pstSrc->stTargetPrm.confidence;
    pstDst->stTargetPrm.scaleLevel = pstSrc->stTargetPrm.scaleLevel;
    pstDst->stTargetPrm.name_cnt = pstSrc->stTargetPrm.name_cnt;
    pstDst->stTargetPrm.color_cnt = pstSrc->stTargetPrm.color_cnt;
#endif

    /* 算法分析结果，随目标物信息更新 */
    pstDst->frame_num = pstSrc->frame_num;
    /* pstDst->drawType = pstSrc->drawType; */
    pstDst->frame_offset = pstSrc->frame_offset;
    pstDst->single_frame_offset = pstSrc->single_frame_offset;
    pstDst->frame_stamp = pstSrc->frame_stamp;
    pstDst->uiTimeRef = pstSrc->uiTimeRef;
    /* pstDst->direction = pstSrc->direction; */
    pstDst->xsi_out_type = pstSrc->xsi_out_type;
    pstDst->target_num = pstSrc->target_num;
    pstDst->frame_sync_idx = pstSrc->frame_sync_idx;

    pstDst->packbagAlert.candidate_flag = pstSrc->packbagAlert.candidate_flag;
    pstDst->packbagAlert.package_valid = pstSrc->packbagAlert.package_valid;
    pstDst->packbagAlert.package_id = pstSrc->packbagAlert.package_id;
    pstDst->packbagAlert.package_match_index = pstSrc->packbagAlert.package_match_index;
    pstDst->packbagAlert.package_loc.x = pstSrc->packbagAlert.package_loc.x;
    pstDst->packbagAlert.package_loc.y = pstSrc->packbagAlert.package_loc.y;
    pstDst->packbagAlert.package_loc.width = pstSrc->packbagAlert.package_loc.width;
    pstDst->packbagAlert.package_loc.height = pstSrc->packbagAlert.package_loc.height;
    pstDst->packbagAlert.MainViewPackageNum = pstSrc->packbagAlert.MainViewPackageNum;
    for (i = 0; i < pstDst->packbagAlert.MainViewPackageNum; i++)
    {
        pstDst->packbagAlert.MainViewPackageLoc[i].PackageRect.x = pstSrc->packbagAlert.MainViewPackageLoc[i].PackageRect.x;
        pstDst->packbagAlert.MainViewPackageLoc[i].PackageRect.y = pstSrc->packbagAlert.MainViewPackageLoc[i].PackageRect.y;
        pstDst->packbagAlert.MainViewPackageLoc[i].PackageRect.width = pstSrc->packbagAlert.MainViewPackageLoc[i].PackageRect.width;
        pstDst->packbagAlert.MainViewPackageLoc[i].PackageRect.height = pstSrc->packbagAlert.MainViewPackageLoc[i].PackageRect.height;
        pstDst->packbagAlert.MainViewPackageLoc[i].PakageID = pstSrc->packbagAlert.MainViewPackageLoc[i].PakageID;
    }

    pstDst->packbagAlert.OriginalPackageNum = pstSrc->packbagAlert.OriginalPackageNum;
    for (i = 0; i < pstDst->packbagAlert.OriginalPackageNum; i++)
    {
        pstDst->packbagAlert.OriginalPackageLoc[i].PackageRect.x = pstSrc->packbagAlert.OriginalPackageLoc[i].PackageRect.x;
        pstDst->packbagAlert.OriginalPackageLoc[i].PackageRect.y = pstSrc->packbagAlert.OriginalPackageLoc[i].PackageRect.y;
        pstDst->packbagAlert.OriginalPackageLoc[i].PackageRect.width = pstSrc->packbagAlert.OriginalPackageLoc[i].PackageRect.width;
        pstDst->packbagAlert.OriginalPackageLoc[i].PackageRect.height = pstSrc->packbagAlert.OriginalPackageLoc[i].PackageRect.height;
        pstDst->packbagAlert.OriginalPackageLoc[i].PakageID = pstSrc->packbagAlert.OriginalPackageLoc[i].PakageID;
    }

    pstDst->packbagAlert.fPkgFwdLoc = pstSrc->packbagAlert.fPkgFwdLoc;
    pstDst->uiOffsetX = pstSrc->uiOffsetX;
    pstDst->uiImgH = pstSrc->uiImgH;
    pstDst->uiImgW = pstSrc->uiImgW;
    pstDst->aipackinfo.packBottom = pstSrc->aipackinfo.packBottom;
    pstDst->aipackinfo.packTop = pstSrc->aipackinfo.packTop;
    pstDst->aipackinfo.packIndx = pstSrc->aipackinfo.packIndx;
    pstDst->aipackinfo.completePackMark = pstSrc->aipackinfo.completePackMark;

    for (i = 0; i < pstSrc->target_num; i++)
    {
        sal_memcpy_s(&pstDst->target[i], sizeof(SVA_TARGET), &pstSrc->target[i], sizeof(SVA_TARGET));
    }

    pstDst->pos_info.xsi_pos_size = pstSrc->pos_info.xsi_pos_size;

    sal_memcpy_s(pstDst->pos_info.xsi_pos_buf, SVA_POST_MSG_LEN, pstSrc->pos_info.xsi_pos_buf, pstSrc->pos_info.xsi_pos_size);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalUpDateSvaResult
* 描  述  : 更新算法结果
* 输  入  : - pstSrc: 拷贝源变量
*         : - pstDst: 拷贝目的变量
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalAdjustSvaResult(SVA_PROCESS_OUT *pstSrc, SVA_PROCESS_OUT *pstDst, INT64L timeRef_diff)
{
    UINT32 i = 0;

    SVA_HAL_CHECK_PRM(pstSrc, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstDst, SAL_FAIL);

#if 0
    /* 上层应用相关配置参数，动态更新 */
    pstDst->stTargetPrm.confidence = pstSrc->stTargetPrm.confidence;
    pstDst->stTargetPrm.scaleLevel = pstSrc->stTargetPrm.scaleLevel;
    pstDst->stTargetPrm.name_cnt = pstSrc->stTargetPrm.name_cnt;
    pstDst->stTargetPrm.color_cnt = pstSrc->stTargetPrm.color_cnt;
#endif

    /* 算法分析结果，随目标物信息更新 */
    pstDst->frame_num = pstSrc->frame_num;
/*  pstDst->drawType       = pstSrc->drawType; */
    pstDst->frame_offset = pstSrc->frame_offset;
    pstDst->single_frame_offset = pstSrc->single_frame_offset;
    pstDst->frame_stamp = pstSrc->frame_stamp;
    pstDst->uiTimeRef = pstSrc->uiTimeRef;
/*  pstDst->direction      = pstSrc->direction; */
    pstDst->xsi_out_type = pstSrc->xsi_out_type;
    pstDst->target_num = pstSrc->target_num;
    pstDst->frame_sync_idx = pstSrc->frame_sync_idx;

    pstDst->packbagAlert.candidate_flag = pstSrc->packbagAlert.candidate_flag;
    pstDst->packbagAlert.package_valid = pstSrc->packbagAlert.package_valid;
    pstDst->packbagAlert.package_loc.x = pstSrc->packbagAlert.package_loc.x - timeRef_diff * pstSrc->single_frame_offset;
    pstDst->packbagAlert.package_loc.y = pstSrc->packbagAlert.package_loc.y;
    pstDst->packbagAlert.package_loc.width = pstSrc->packbagAlert.package_loc.width;
    pstDst->packbagAlert.package_loc.height = pstSrc->packbagAlert.package_loc.height;
    pstDst->packbagAlert.MainViewPackageNum = pstSrc->packbagAlert.MainViewPackageNum;

    for (i = 0; i < pstDst->packbagAlert.MainViewPackageNum; i++)
    {
        pstDst->packbagAlert.MainViewPackageLoc[i].PackageRect.x = pstSrc->packbagAlert.MainViewPackageLoc[i].PackageRect.x;
        pstDst->packbagAlert.MainViewPackageLoc[i].PackageRect.y = pstSrc->packbagAlert.MainViewPackageLoc[i].PackageRect.y;
        pstDst->packbagAlert.MainViewPackageLoc[i].PackageRect.width = pstSrc->packbagAlert.MainViewPackageLoc[i].PackageRect.width;
        pstDst->packbagAlert.MainViewPackageLoc[i].PackageRect.height = pstSrc->packbagAlert.MainViewPackageLoc[i].PackageRect.height;
        pstDst->packbagAlert.MainViewPackageLoc[i].PakageID = pstSrc->packbagAlert.MainViewPackageLoc[i].PakageID;
    }

    pstDst->packbagAlert.OriginalPackageNum = pstSrc->packbagAlert.OriginalPackageNum;

    for (i = 0; i < pstDst->packbagAlert.OriginalPackageNum; i++)
    {
        pstDst->packbagAlert.OriginalPackageLoc[i].PackageRect.x = pstSrc->packbagAlert.OriginalPackageLoc[i].PackageRect.x;
        pstDst->packbagAlert.OriginalPackageLoc[i].PackageRect.y = pstSrc->packbagAlert.OriginalPackageLoc[i].PackageRect.y;
        pstDst->packbagAlert.OriginalPackageLoc[i].PackageRect.width = pstSrc->packbagAlert.OriginalPackageLoc[i].PackageRect.width;
        pstDst->packbagAlert.OriginalPackageLoc[i].PackageRect.height = pstSrc->packbagAlert.OriginalPackageLoc[i].PackageRect.height;
        pstDst->packbagAlert.OriginalPackageLoc[i].PakageID = pstSrc->packbagAlert.OriginalPackageLoc[i].PakageID;
    }

    SAL_LOGD("need adjust loc! frame_offset %f, frmNum %d, timeRef_diff %lld \n",
             pstDst->frame_offset, pstDst->frame_num, timeRef_diff);

    for (i = 0; i < pstSrc->target_num; i++)
    {
        sal_memcpy_s(&pstDst->target[i], sizeof(SVA_TARGET), &pstSrc->target[i], sizeof(SVA_TARGET));
    }

    pstDst->pos_info.xsi_pos_size = pstSrc->pos_info.xsi_pos_size;

    sal_memcpy_s(pstDst->pos_info.xsi_pos_buf, SVA_POST_MSG_LEN, pstSrc->pos_info.xsi_pos_buf, pstSrc->pos_info.xsi_pos_size);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalClearSvaOut
* 描  述  : 清空智能结果信息
* 输  入  : - chan : 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalClearSvaOut(UINT32 chan)
{
    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);

    XSI_DEV *pstXsi = NULL;

    pstXsi = Sva_HalGetDev(chan);
    if (NULL == pstXsi)
    {
        SVA_LOGE("xsi dev == NULL! chan %d \n ", chan);
        return SAL_FAIL;
    }

    /* 清空全局变量中保存的叠框信息，避免下次开启智能分析结果残留 */
    SAL_clear(&pstXsi->stSvaOut);

    SVA_LOGI("Clear Sva Out End! chan %d \n", chan);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalGetSvaOut
* 描  述  : 获取智能结果信息
* 输  入  : - chan     : 通道号
*         : - pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalGetSvaOut(UINT32 chan, SVA_PROCESS_OUT *pstSvaOut)
{
    INT32 ret = SAL_FALSE;
    UINT32 viUserPic = 0;

    SVA_HAL_CHECK_PRM(pstSvaOut, SAL_FAIL);

    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);

    XSI_DEV *pstXsi = Sva_HalGetDev(chan);

    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    viUserPic = capt_hal_getCaptUserPicStatue(chan);
    if (!viUserPic)
    {
        if (pstXsi->xsi_status == SAL_TRUE)
        {
            SAL_mutexLock(pstXsi->mOutMutexHdl);
            ret = Sva_HalCpySvaResult(&pstXsi->stSvaOut, pstSvaOut);
            if (ret != SAL_SOK)
            {
                SVA_LOGW("warn\n");
            }

            pstSvaOut->reaultCnt = pstXsi->reaultCnt;
            SAL_mutexUnlock(pstXsi->mOutMutexHdl);
        }
        else
        {
            pstSvaOut->reaultCnt = pstXsi->reaultCnt;
        }
    }
    else
    {

        pstXsi->stSvaOut.target_num = 0;
        pstSvaOut->target_num = 0;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalUpDateOut
* 描  述  : 更新智能结果
* 输  入  : - chan     : 通道号
*         : - pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalUpDateOut(UINT32 chan, SVA_PROCESS_OUT *pstDrvOut)
{
    INT32 ret = SAL_FALSE;

    SVA_HAL_CHECK_PRM(pstDrvOut, SAL_FAIL);
    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);

    XSI_DEV *pstXsi = Sva_HalGetDev(chan);

    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    SAL_mutexLock(pstXsi->mOutMutexHdl);

    if (pstXsi->xsi_status == SAL_TRUE)
    {
        ret = Sva_HalCpySvaResult(pstDrvOut, &pstXsi->stSvaOut);
        if (ret != SAL_SOK)
        {
            SVA_LOGW("Copy Sva Result Failed! chan %d \n", chan);
        }

        if (pstDrvOut->target_num > 0)
        {
            pstXsi->reaultCnt++;
        }
    }

    SAL_mutexUnlock(pstXsi->mOutMutexHdl);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名:   Sva_HalGetFromPool
* 描  述   :   获取智能结果信息
* 输  入   :   - chan     : 通道号
*                 :   - uiTimeRef: 用于匹配的帧号
*                 :   - pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*                      SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalGetFromPool(UINT32 chan, UINT32 uiTimeRef, SVA_PROCESS_OUT *pstSvaOut)
{
    INT32 ret = SAL_FALSE;
    UINT32 r_idx = 0, uiMinDiff_idx = 0;
    UINT32 i = 0;
    UINT32 viUserPic = 0;
    SVA_PROCESS_OUT *pstSvaSrc = NULL;
    UINT32 uiSvaTimeRef = 0;
    UINT32 uiMinDiff = 0;

    /* static UINT32 count = 0; */
    SVA_HAL_CHECK_PRM(pstSvaOut, SAL_FAIL);

    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);

    XSI_DEV *pstXsi = Sva_HalGetDev(chan);

    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    viUserPic = capt_hal_getCaptUserPicStatue(chan);
    if (!viUserPic)
    {
        if (pstXsi->xsi_status == SAL_TRUE)
        {

            r_idx = pstXsi->stSvaOutPool.r_idx;
            pstSvaSrc = &(pstXsi->stSvaOutPool.svaOut[r_idx]);
            uiSvaTimeRef = pstSvaSrc->uiTimeRef;
            uiMinDiff_idx = r_idx;
            uiMinDiff = DIFF_ABS(uiSvaTimeRef, uiTimeRef);

            /* i从r_idx开始比较，是为了判断如果r_idx的timeRef相等，就直接break，提高效率 */
            for (i = r_idx; i < SVAOUT_CNT + r_idx; i++)
            {
                pstSvaSrc = &(pstXsi->stSvaOutPool.svaOut[i % SVAOUT_CNT]);
                uiSvaTimeRef = pstSvaSrc->uiTimeRef;

                if (uiSvaTimeRef == uiTimeRef)
                {
                    uiMinDiff_idx = i % SVAOUT_CNT;
                    break;
                }

                if (DIFF_ABS(uiSvaTimeRef, uiTimeRef) < uiMinDiff)
                {
                    uiMinDiff = DIFF_ABS(uiSvaTimeRef, uiTimeRef);
                    uiMinDiff_idx = i % SVAOUT_CNT;
                }
                else if (uiMinDiff == DIFF_ABS(uiSvaTimeRef, uiTimeRef)) /* 如果有两个uiSvaTimeRef与要搜索的uiTimeRef的差值相等，取较大那一个 */
                {
                    if (uiSvaTimeRef > uiTimeRef)
                    {
                        uiMinDiff_idx = i % SVAOUT_CNT;
                    }
                }
                else
                {
                    ;
                }

                #if 0
                static int count = 0;
                /* debug ，后续去掉*/
                if (i == (SVAOUT_CNT + r_idx - 1))
                {
                    count++;
                    if (count % 10 == 0)
                    {
                        SAL_WARN(" w_idx %d ; search for %d, result %d \n", pstXsi->stSvaOutPool.w_idx, uiTimeRef, pstXsi->stSvaOutPool.svaOut[uiMinDiff_idx].uiTimeRef);
                        for (i = r_idx; i < SVAOUT_CNT + r_idx; i++)
                        {
                            pstSvaSrc = &(pstXsi->stSvaOutPool.svaOut[i % SVAOUT_CNT]);
                            SAL_WARN("    idx %d, uiTimeRef in pool %u, frm_num %u\n", i % SVAOUT_CNT, pstSvaSrc->uiTimeRef, pstSvaSrc->frame_num);
                        }
                    }
                }

                #endif
            }

            pstXsi->stSvaOutPool.r_idx = (uiMinDiff_idx + 1) % SVAOUT_CNT;

            pstSvaSrc = &(pstXsi->stSvaOutPool.svaOut[uiMinDiff_idx]);
            SAL_mutexLock(pstXsi->mOutMutexHdl);

            if (pstSvaSrc->uiTimeRef != uiTimeRef)
            {
                /* debug 信息，完全调好之后再删除
                         if (uiTimeRef % 121 == 0)
                         {
                             SAL_WARN(" ref_diff %lld, frame_offset %f, offset %f \n", ((INT64L)uiTimeRef - (INT64L)pstSvaSrc->uiTimeRef) / 2, \
                             pstSvaSrc->frame_offset, pstSvaSrc->target[0].offset);
                         }
                 */
                ret = Sva_HalAdjustSvaResult(pstSvaSrc, pstSvaOut, ((INT64L)uiTimeRef - (INT64L)pstSvaSrc->uiTimeRef) / 2);
            }
            else
            {
                ret = Sva_HalCpySvaResult(pstSvaSrc, pstSvaOut);
            }

            if (ret != SAL_SOK)
            {
                SVA_LOGW("warn\n");
            }

            pstSvaOut->reaultCnt = pstXsi->reaultCnt;
            SAL_mutexUnlock(pstXsi->mOutMutexHdl);
        }
        else
        {
            pstSvaOut->reaultCnt = pstXsi->reaultCnt;
        }
    }
    else
    {
        pstXsi->stSvaOut.target_num = 0;
        pstSvaOut->target_num = 0;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名:   Sva_HalGetFromPoolWithPts
* 描  述   :   根据PTS从缓存池获取智能结果信息
* 输  入   :   - chan     : 通道号
*          :   - pRidx: 读索引指针
*          :   - u64Pts: 用于匹配的PTS
*          :   - pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*                      SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalGetFromPoolWithPts(UINT32 chan, UINT32 *pRidx, UINT64 u64Pts, SVA_PROCESS_OUT *pstSvaOut)
{
    INT32 ret = SAL_FAIL;
    UINT32 r_idx = 0, uiMinDiff_idx = 0;
    UINT32 i = 0;
    UINT32 viUserPic = 0;
    SVA_PROCESS_OUT *pstSvaSrc = NULL;
    UINT64 u64SvaPts = 0;
    UINT64 u64MinDiff = 0;

    /* static UINT32 count = 0; */
    SVA_HAL_CHECK_PRM(pstSvaOut, SAL_FAIL);

    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);

    XSI_DEV *pstXsi = Sva_HalGetDev(chan);

    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    r_idx = *pRidx;
    if (r_idx >= SVAOUT_CNT)
    {
        SVA_LOGE("r_idx %d, illegal \n", r_idx);
        return SAL_FAIL;
    }

    viUserPic = capt_hal_getCaptUserPicStatue(chan);
    if (!viUserPic)
    {
        if (pstXsi->xsi_status == SAL_TRUE)
        {
            pstSvaSrc = &(pstXsi->stSvaOutPool.svaOut[r_idx]);
            u64SvaPts = pstSvaSrc->frame_stamp / 1000;
            /* 20201229: 目前第二路pts为0，发去从片处理时需要把第二路pts加上*/
            if (0 == u64SvaPts)
            {
                return SAL_FAIL;
            }

            uiMinDiff_idx = r_idx;
            u64MinDiff = DIFF_ABS(u64SvaPts, u64Pts);

            /* i从r_idx开始比较，是为了判断如果r_idx的frame_stamp相等，就直接break，提高效率 */
            for (i = r_idx; i < SVAOUT_CNT + r_idx; i++)
            {
                pstSvaSrc = &(pstXsi->stSvaOutPool.svaOut[i % SVAOUT_CNT]);
                u64SvaPts = pstSvaSrc->frame_stamp / 1000;

                if (u64SvaPts == u64Pts)
                {
                    uiMinDiff_idx = i % SVAOUT_CNT;
                    break;
                }

                if (DIFF_ABS(u64SvaPts, u64Pts) < u64MinDiff)
                {
                    u64MinDiff = DIFF_ABS(u64SvaPts, u64Pts);
                    uiMinDiff_idx = i % SVAOUT_CNT;
                }
                else if (u64MinDiff == DIFF_ABS(u64SvaPts, u64Pts)) /* 如果有两个u64SvaPts与要搜索的u64Pts的差值相等，取较大那一个 */
                {
                    if (u64SvaPts > u64Pts)
                    {
                        uiMinDiff_idx = i % SVAOUT_CNT;
                    }
                }
                else
                {
                    ;
                }

                #if 0
                static int count = 0;
                /* debug ，后续去掉*/
                if (i == (SVAOUT_CNT + r_idx - 1))
                {
                    count++;
                    if (count % 180 == 0)
                    {
                        SVA_LOGW(" w_idx %d ; search for %llu, result %llu, diff %lld \n", pstXsi->stSvaOutPool.w_idx, u64Pts, pstXsi->stSvaOutPool.svaOut[uiMinDiff_idx].frame_stamp / 1000, \
                                 (INT64T)u64Pts - (INT64T)pstXsi->stSvaOutPool.svaOut[uiMinDiff_idx].frame_stamp / 1000);
                        for (i = r_idx; i < SVAOUT_CNT + r_idx; i = i + 5)
                        {
                            pstSvaSrc = &(pstXsi->stSvaOutPool.svaOut[i % SVAOUT_CNT]);
                            SVA_LOGW("    idx %d, frame_stamp in pool %llu \n", i % SVAOUT_CNT, pstSvaSrc->frame_stamp / 1000);
                        }
                    }
                }

                #endif
            }

            *pRidx = (uiMinDiff_idx + 1) % SVAOUT_CNT;
            pstSvaSrc = &(pstXsi->stSvaOutPool.svaOut[uiMinDiff_idx]);
            SAL_mutexLock(pstXsi->mOutMutexHdl);

            ret = Sva_HalCpySvaResult(pstSvaSrc, pstSvaOut);
            if (ret != SAL_SOK)
            {
                SVA_LOGW("warn\n");
            }

            SAL_mutexUnlock(pstXsi->mOutMutexHdl);
        }
    }
    else
    {
        pstXsi->stSvaOut.target_num = 0;
        pstSvaOut->target_num = 0;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名:  Sva_HalClrPoolResult
* 描  述   :  把SVA结果保持到缓冲池
* 输  入  : - chan     : 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalClrPoolResult(UINT32 chan)
{
    UINT32 i = 0;
    UINT32 w_idx = 0;
    SVA_PROCESS_OUT *pstSvaDst = NULL;

    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);

    XSI_DEV *pstXsi = Sva_HalGetDev(chan);

    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    SAL_mutexLock(pstXsi->mOutMutexHdl);

    w_idx = pstXsi->stSvaOutPool.w_idx;
    for (i = 0; i < SVAOUT_CNT; i++)
    {
        pstSvaDst = &(pstXsi->stSvaOutPool.svaOut[(w_idx + i) % SVAOUT_CNT]);
        SAL_clear(pstSvaDst);
    }

    SAL_mutexUnlock(pstXsi->mOutMutexHdl);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名:  Sva_HalSaveToPool
* 描  述   :  把SVA结果保持到缓冲池
* 输  入  : - chan     : 通道号
*                  - pstDrvOutSrc : 需要保存的SVA信息
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalSaveToPool(UINT32 chan, SVA_PROCESS_OUT *pstDrvOutSrc)
{
    INT32 ret = SAL_SOK;
    UINT32 w_idx = 0;
    SVA_PROCESS_OUT *pstSvaDst = NULL;

    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstDrvOutSrc, SAL_FAIL);

    XSI_DEV *pstXsi = Sva_HalGetDev(chan);

    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    SAL_mutexLock(pstXsi->mOutMutexHdl);

    w_idx = pstXsi->stSvaOutPool.w_idx;
    pstSvaDst = &(pstXsi->stSvaOutPool.svaOut[w_idx]);

    if (pstXsi->xsi_status == SAL_TRUE)
    {
        ret = Sva_HalCpySvaResult(pstDrvOutSrc, pstSvaDst);
        if (ret != SAL_SOK)
        {
            SVA_LOGW("Copy Sva Result Failed! chan %d \n", chan);
        }
        else
        {
            pstXsi->stSvaOutPool.w_idx = (w_idx + 1) % SVAOUT_CNT;
        }
    }

    SAL_mutexUnlock(pstXsi->mOutMutexHdl);

    return ret;
}

/*******************************************************************************
* 函数名  : Sva_HalFindFreeInputIndex
* 描  述  : 寻找空闲Input索引项
* 输  入  : - chan : 通道号
*         : - mode : 模式
* 输  出  : - pIdx : 索引值指针
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalFindFreeInputIndex(UINT32 chan, SVA_PROC_MODE_E mode, UINT32 *pIdx)
{
    UINT32 i = 0;
    UINT32 uiChnId = 0;

    /* XSI_DEV *pstXsi = NULL; */
    ENG_CHANNEL_PRM_S *pstEngChnPrm = NULL;
    SVA_INPUT_DATA *pstInputDataInfo = NULL;

    /* Input Args Checker */
    SVA_HAL_CHECK_ENG_CHN(chan, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pIdx, SAL_FAIL);

    /* TODO: 目前仅有图片模式支持双通道独立检测，关联模式和视频模式均是单通道处理，使用通道0 */
    uiChnId = (mode == SVA_PROC_MODE_IMAGE ? chan : 0);

    pstEngChnPrm = Sva_HalGetEngchnPrm(uiChnId);
    SVA_HAL_CHECK_PRM(pstEngChnPrm, SAL_FAIL);

    if (1 == chan && SVA_PROC_MODE_DUAL_CORRELATION == mode)
    {
        pIdx[1] = pIdx[0];
        return SAL_SOK;
    }

    for (i = 0; i < SVA_INPUT_BUF_LEN; i++)
    {
        pstInputDataInfo = (SVA_INPUT_DATA *)&pstEngChnPrm->astSvaInputData[i];

        if (SAL_TRUE == pstInputDataInfo->bUseFlag)
        {
            SVA_LOGD("i %d, pstInputDataInfo %p \n", i, pstInputDataInfo);
            continue;
        }

        pstInputDataInfo->bUseFlag = SAL_TRUE;
        break;
    }

    if (i >= SVA_INPUT_BUF_LEN)
    {
        SVA_LOGE("Input Add Info Buf Full! chan %d, mode %d \n", chan, mode);
        return SAL_FAIL;
    }

    pIdx[uiChnId] = i;
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalFreeInputIndex
* 描  述  : 清空占用Input索引项
* 输  入  : - chan : 通道号
*         : - mode : 模式
* 输  出  : - pIdx : 索引值指针
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalFreeInputIndex(UINT32 chan, SVA_PROC_MODE_E mode, UINT32 *pIdx)
{
    UINT32 uiChnId = 0;

    /* XSI_DEV *pstXsi = NULL; */
    ENG_CHANNEL_PRM_S *pstEngChnPrm = NULL;
    SVA_INPUT_DATA *pstInputDataInfo = NULL;

    /* Input Args Checker */
    SVA_HAL_CHECK_ENG_CHN(chan, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pIdx, SAL_FAIL);

    /* TODO: 目前仅有图片模式支持双通道独立检测，关联模式和视频模式均是单通道处理，使用通道0 */
    uiChnId = (mode == SVA_PROC_MODE_IMAGE ? chan : 0);

    pstEngChnPrm = Sva_HalGetEngchnPrm(uiChnId);
    SVA_HAL_CHECK_PRM(pstEngChnPrm, SAL_FAIL);

    pstInputDataInfo = (SVA_INPUT_DATA *)&pstEngChnPrm->astSvaInputData[pIdx[uiChnId]];

    if (SAL_TRUE == pstInputDataInfo->bUseFlag)
    {
        pstInputDataInfo->bUseFlag = SAL_FALSE;
    }

    return SAL_SOK;
}

#define DEBUG_DUMP_YUV_SIZE			(1024 * 1024 * 1024) /* 1G */
#define DEBUG_DUMP_YUV_DATA_SIZE	(1280 * 1024 * 3 / 2)
#define DEBUG_DUMP_FRAME_NUM		(1500)    /* number of frames to be dumped */
#define DEBUG_DUMP_PATH				("./dump_yuv.nv21")

static int do_flag = 0;

/**
 * @function    debug_dump_set_flag
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
void debug_dump_set_flag(int flag)
{
    do_flag = (int)(!!flag);
    SVA_LOGI("get debug dump flag %d end! \n", do_flag);
}

/**
 * @function    debug_dump_yuv
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
void debug_dump_yuv(char *data)
{
    int r = 0;
    static int offset = 0;
    static char *va = NULL;
    UINT64 pa = 0;

    static int path_idx = 0;
    char path[64] = {0};

    static int cnt = 0;
    static int over_size_flag = 0;

    /* exit */
    if (1 != do_flag)
    {
        return;
    }

    if (!va)
    {
        r = mem_hal_mmzAlloc(DEBUG_DUMP_YUV_SIZE, "sva", "debug", NULL, 0, (UINT64 *)&pa, (VOID **)&va);
        if (SAL_SOK != r)
        {
            SVA_LOGE("dbg_yuv: mmz alloc failed! \n");
            return;
        }

        SVA_LOGI("");
    }

    if (cnt < DEBUG_DUMP_FRAME_NUM && 1 != over_size_flag)
    {
        /* dump ONE yuv each THREE frame */
        if (offset + DEBUG_DUMP_YUV_DATA_SIZE <= DEBUG_DUMP_YUV_SIZE)
        {
            memcpy(va + offset, data, DEBUG_DUMP_YUV_DATA_SIZE);
            offset += DEBUG_DUMP_YUV_DATA_SIZE;

            cnt++;
        }
        else
        {
            over_size_flag = 1;
        }
    }
    else
    {
        /* write file */
        FILE *fp = NULL;

        sprintf(path, "%s_%d", DEBUG_DUMP_PATH, path_idx);
        fp = fopen(path, "w+");
        if (NULL == fp)
        {
            SVA_LOGE("fopen failed! \n");
            return;
        }

        int time0 = SAL_getCurMs();

        fwrite(va, offset, 1, fp);
        fflush(fp);
        fclose(fp);

        path_idx++;

        int time1 = SAL_getCurMs();

        SVA_LOGE("================== write cost %d ms! \n", time1 - time0);

        /* free memory */
        /* mem_hal_mmzFree(va, "sva", "debug"); */
        /* va = NULL; */

        /* mark as finished */
        /* do_flag = 0; */

        /* reset static variables, prepared for the next dump action */
        if (cnt == DEBUG_DUMP_FRAME_NUM)
        {
            /* cnt = do_flag = path_idx = 0; */
            cnt = 0;
        }

        offset = over_size_flag = 0;

        SVA_LOGW("write dump data end! path: %s, offset %d, cnt %d \n", path, offset, cnt);
    }

    return;

}

/**
 * @function    Sva_HalFillInputData
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_HalFillInputData(IN BOOL bDoubleView,
                                  IN BOOL bYuvRectValid,
                                  IN SVA_RECT_F *pstRect,
                                  IN UINT64 u64FrameNum,
                                  IN XSI_DEV_BUFF *pstMainBuf,
                                  IN XSI_DEV_BUFF *pstSideBuf,
                                  OUT ENGINE_COMM_INPUT_DATA_INFO *pstInputDataInfo)
{
    /* checker */
    if (NULL == pstMainBuf
        || NULL == pstSideBuf
        || NULL == pstInputDataInfo)
    {
        SVA_LOGE("input null! main %p, side %p, input data %p \n", pstMainBuf, pstSideBuf, pstInputDataInfo);
        return SAL_FAIL;
    }

    if (NULL == pstMainBuf->stMemBuf.VirAddr)
    {
        SVA_LOGE("Invalid Buff Addr! pls Check! \n");
        return SAL_FAIL;
    }

    if (1 != pstInputDataInfo->stIaImgPrvtCtrlPrm.u32InputDataImgNum)
    {
        SVA_LOGE("invalid blob num %d \n", pstInputDataInfo->stIaImgPrvtCtrlPrm.u32InputDataImgNum);
        return SAL_FAIL;
    }

    /* 帧序号 */
    pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].u64FrameNum = u64FrameNum;

    /* 图像是否通知引擎内部抠图+resize，默认不开启，当前仅图片模式需要使用到 */
    pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].u32SourYuvRectValid = bYuvRectValid;

    if (bYuvRectValid)
    {
        /* 当前仅支持一维blob处理 */
        if (NULL == pstRect)
        {
            /* 默认整幅画面抠图 */
            pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].fYuvRectX = 0.0f;
            pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].fYuvRectY = 0.0f;
            pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].fYuvRectW = 1.0f;
            pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].fYuvRectH = 1.0f;
        }
        else
        {
            pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].fYuvRectX = pstRect->x;
            pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].fYuvRectY = pstRect->y;
            pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].fYuvRectW = pstRect->width;
            pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].fYuvRectH = pstRect->height;
        }
    }

    /* 主视角帧数据填充 */
    pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].u32MainViewImgNum = 1;

    pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stMainViewImg[0].pY_CompVa = (VOID *)pstMainBuf->stMemBuf.VirAddr;
    pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stMainViewImg[0].pU_CompVa = (VOID *)((CHAR *)pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stMainViewImg[0].pY_CompVa + SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT);
    pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stMainViewImg[0].pV_CompVa = pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stMainViewImg[0].pU_CompVa;

	/* LCD输入特征分配地址,Le\He\Zmap分别连续地址，单帧feat长度为SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 6 */
    pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stInLcdData[0].InFeatRaw = ((CHAR *)pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stMainViewImg[0].pY_CompVa + SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT*3/2);

    /* 默认侧视角帧数据个数为0 */
    pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].u32SideViewImgNum = 0;

    /* 侧视角帧数据填充 */
    if (bDoubleView)
    {
        pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].u32SideViewImgNum = 1;

        if (NULL == pstSideBuf->stMemBuf.VirAddr)
        {
            SVA_LOGE("Invalid Buff Addr! pls Check! \n");
            return SAL_FAIL;
        }

        pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stSideViewImg[0].pY_CompVa = (VOID *)pstSideBuf->stMemBuf.VirAddr;
        pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stSideViewImg[0].pU_CompVa = (VOID *)((CHAR *)pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stSideViewImg[0].pY_CompVa + SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT);
        pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stSideViewImg[0].pV_CompVa = pstInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stSideViewImg[0].pU_CompVa;

    }

    /* debug_dump_yuv(pstMainBuf->stMemBuf.VirAddr);*/

#ifdef DEBUG_LOG
    SVA_LOGW("input--bDoubleView %d, bYuvRectValid %d, frameNum %d \n", bDoubleView, bYuvRectValid, u64FrameNum);
#endif

    return SAL_SOK;
}

/* 将外部智能图像数据和私有参数填充到icf框架的结构体中 */
static INT32 xsie_proc_alg_img(VOID *pImgInfo, VOID *pXsiePrvtOutput)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 j = 0;

    ENGINE_COMM_INPUT_DATA_IMG_INFO *pstXsieImgInfo = NULL;
    XSIE_SECURITY_INPUT_T *pstYuvDataInfo = NULL;

    if (NULL == pImgInfo
        || NULL == pXsiePrvtOutput)
    {
        SAL_LOGE("ptr null! %p, %p \n", pImgInfo, pXsiePrvtOutput);
        goto exit;
    }

    pstXsieImgInfo = (ENGINE_COMM_INPUT_DATA_IMG_INFO *)pImgInfo;
    pstYuvDataInfo = (XSIE_SECURITY_INPUT_T *)pXsiePrvtOutput;

	/* LCD输入特征分配地址,Le\He\Zmap分别连续地址 */
    pstYuvDataInfo->InDataInfo.InLcdData.InFeatRaw     = pstXsieImgInfo->stInLcdData[0].InFeatRaw;
    pstYuvDataInfo->InDataInfo.InLcdData.FeatW     = pstXsieImgInfo->stInLcdData[0].FeatW;
    pstYuvDataInfo->InDataInfo.InLcdData.FeatH     = pstXsieImgInfo->stInLcdData[0].FeatH;

    pstYuvDataInfo->InDataInfo.MainNum = pstXsieImgInfo->u32MainViewImgNum;

    for (j = 0; j < pstXsieImgInfo->u32MainViewImgNum; j++)
    {
        pstYuvDataInfo->InDataInfo.MainYuvData[j].image_w = pstXsieImgInfo->stMainViewImg[j].u32W;
        pstYuvDataInfo->InDataInfo.MainYuvData[j].image_h = pstXsieImgInfo->stMainViewImg[j].u32H;
        pstYuvDataInfo->InDataInfo.MainYuvData[j].pitch_y = pstXsieImgInfo->stMainViewImg[j].u32S[0];
        pstYuvDataInfo->InDataInfo.MainYuvData[j].pitch_uv = pstXsieImgInfo->stMainViewImg[j].u32S[1];
        pstYuvDataInfo->InDataInfo.MainYuvData[j].format = VCA_YVU420;  /* fixme: xsie此处写死为420 */

        pstYuvDataInfo->InDataInfo.MainYuvData[j].y = pstXsieImgInfo->stMainViewImg[j].pY_CompVa;
        pstYuvDataInfo->InDataInfo.MainYuvData[j].u = pstXsieImgInfo->stMainViewImg[j].pU_CompVa;
        pstYuvDataInfo->InDataInfo.MainYuvData[j].v = pstXsieImgInfo->stMainViewImg[j].pV_CompVa;
        SVA_LOGD("main---j %d, img_w %d, img_h %d, pitch_y %d, pitch_uv %d, format %d, y %p, u %p, v %p \n",
                 j, pstYuvDataInfo->InDataInfo.MainYuvData[j].image_w,
                 pstYuvDataInfo->InDataInfo.MainYuvData[j].image_h,
                 pstYuvDataInfo->InDataInfo.MainYuvData[j].pitch_y,
                 pstYuvDataInfo->InDataInfo.MainYuvData[j].pitch_uv,
                 pstYuvDataInfo->InDataInfo.MainYuvData[j].format,
                 pstYuvDataInfo->InDataInfo.MainYuvData[j].y,
                 pstYuvDataInfo->InDataInfo.MainYuvData[j].u,
                 pstYuvDataInfo->InDataInfo.MainYuvData[j].v);
    }

    pstYuvDataInfo->InDataInfo.SideNum = pstXsieImgInfo->u32SideViewImgNum;

    for (j = 0; j < pstXsieImgInfo->u32SideViewImgNum; j++)
    {
        pstYuvDataInfo->InDataInfo.SideYuvData[j].image_w = pstXsieImgInfo->stSideViewImg[j].u32W;
        pstYuvDataInfo->InDataInfo.SideYuvData[j].image_h = pstXsieImgInfo->stSideViewImg[j].u32H;
        pstYuvDataInfo->InDataInfo.SideYuvData[j].pitch_y = pstXsieImgInfo->stSideViewImg[j].u32S[0];
        pstYuvDataInfo->InDataInfo.SideYuvData[j].pitch_uv = pstXsieImgInfo->stSideViewImg[j].u32S[1];
        pstYuvDataInfo->InDataInfo.SideYuvData[j].format = VCA_YVU420;  /* fixme: xsie此处写死为420 */

        pstYuvDataInfo->InDataInfo.SideYuvData[j].y = pstXsieImgInfo->stSideViewImg[j].pY_CompVa;
        pstYuvDataInfo->InDataInfo.SideYuvData[j].u = pstXsieImgInfo->stSideViewImg[j].pU_CompVa;
        pstYuvDataInfo->InDataInfo.SideYuvData[j].v = pstXsieImgInfo->stSideViewImg[j].pV_CompVa;

        SVA_LOGD("side---j %d, img_w %d, img_h %d, pitch_y %d, pitch_uv %d, format %d, y %p, u %p, v %p \n",
                 j, pstYuvDataInfo->InDataInfo.SideYuvData[j].image_w,
                 pstYuvDataInfo->InDataInfo.SideYuvData[j].image_h,
                 pstYuvDataInfo->InDataInfo.SideYuvData[j].pitch_y,
                 pstYuvDataInfo->InDataInfo.SideYuvData[j].pitch_uv,
                 pstYuvDataInfo->InDataInfo.SideYuvData[j].format,
                 pstYuvDataInfo->InDataInfo.SideYuvData[j].y,
                 pstYuvDataInfo->InDataInfo.SideYuvData[j].u,
                 pstYuvDataInfo->InDataInfo.SideYuvData[j].v);

    }

    pstYuvDataInfo->InDataInfo.SourYuvRectValid = pstXsieImgInfo->u32SourYuvRectValid;

    /* 当前该标记仅用于图片模式，告知引擎有效图像区域进行抠图及resize。当前默认只使用idx_0 */
    if (pstYuvDataInfo->InDataInfo.SourYuvRectValid)
    {
        pstYuvDataInfo->InDataInfo.SourYuvRect[0].x = pstXsieImgInfo->fYuvRectX;
        pstYuvDataInfo->InDataInfo.SourYuvRect[0].y = pstXsieImgInfo->fYuvRectY;
        pstYuvDataInfo->InDataInfo.SourYuvRect[0].width = pstXsieImgInfo->fYuvRectW;
        pstYuvDataInfo->InDataInfo.SourYuvRect[0].height = pstXsieImgInfo->fYuvRectH;

        SAL_LOGD("yuv rect---valid %d, [%f, %f] [%f, %f] \n",
                 pstYuvDataInfo->InDataInfo.SourYuvRectValid,
                 pstYuvDataInfo->InDataInfo.SourYuvRect[0].x,
                 pstYuvDataInfo->InDataInfo.SourYuvRect[0].y,
                 pstYuvDataInfo->InDataInfo.SourYuvRect[0].width,
                 pstYuvDataInfo->InDataInfo.SourYuvRect[0].height);
    }

    s32Ret = SAL_SOK;
exit:
    return s32Ret;
}

/* 将外部智能图像数据和私有参数填充到icf框架的结构体中 */
static INT32 xsie_proc_alg_prm(VOID *pXsiePrvtPrm, VOID *pXsiePrvtOutput)
{
    INT32 s32Ret = SAL_FAIL;

    XSIE_SECURITY_INPUT_PARAM_T *pstXsiePrvtPrm = NULL;
    XSIE_SECURITY_INPUT_T *pstYuvDataInfo = NULL;

    if (NULL == pXsiePrvtPrm
        || NULL == pXsiePrvtOutput)
    {
        SAL_LOGE("ptr null! %p, %p \n", pXsiePrvtPrm, pXsiePrvtOutput);
        goto exit;
    }

    pstXsiePrvtPrm = (XSIE_SECURITY_INPUT_PARAM_T *)pXsiePrvtPrm;
    pstYuvDataInfo = (XSIE_SECURITY_INPUT_T *)pXsiePrvtOutput;

    /* 拷贝外部参数 */
    sal_memcpy_s(&pstYuvDataInfo->InDataInfo.ParamIn,
                 sizeof(XSIE_SECURITY_INPUT_PARAM_T),
                 pstXsiePrvtPrm,
                 sizeof(XSIE_SECURITY_INPUT_PARAM_T));

    SAL_LOGD("paramIn---main roi[%f, %f] [%f, %f], side roi[%f, %f] [%f, %f] \n",
             pstYuvDataInfo->InDataInfo.ParamIn.MainRoiRect.x, pstYuvDataInfo->InDataInfo.ParamIn.MainRoiRect.y,
             pstYuvDataInfo->InDataInfo.ParamIn.MainRoiRect.width, pstYuvDataInfo->InDataInfo.ParamIn.MainRoiRect.height,
             pstYuvDataInfo->InDataInfo.ParamIn.SideRoiRect.x, pstYuvDataInfo->InDataInfo.ParamIn.SideRoiRect.y,
             pstYuvDataInfo->InDataInfo.ParamIn.SideRoiRect.width, pstYuvDataInfo->InDataInfo.ParamIn.SideRoiRect.height);

    SAL_LOGD("PackOverX2 %f, Orientation %d, MainViewDelayTimeSeconds %d, SideViewDelayTimeSeconds %d, ObdDownsampleScale %f, ZoomInOutThres %f, PackageSensity %d, PK_Switch %d, PDProcessGap %d, SizeFilterSwitch %d, ColorFilterSwitch %d \n",
             pstYuvDataInfo->InDataInfo.ParamIn.PackOverX2,
             pstYuvDataInfo->InDataInfo.ParamIn.Orientation,
             pstYuvDataInfo->InDataInfo.ParamIn.MainViewDelayTimeSeconds,
             pstYuvDataInfo->InDataInfo.ParamIn.SideViewDelayTimeSeconds,
             pstYuvDataInfo->InDataInfo.ParamIn.ObdDownsampleScale,
             pstYuvDataInfo->InDataInfo.ParamIn.ZoomInOutThres,
             pstYuvDataInfo->InDataInfo.ParamIn.PackageSensity,
             pstYuvDataInfo->InDataInfo.ParamIn.PkSwitch,
             pstYuvDataInfo->InDataInfo.ParamIn.PDProcessGap,
             pstYuvDataInfo->InDataInfo.ParamIn.SizeFilterSwitch,
             pstYuvDataInfo->InDataInfo.ParamIn.ColorFilterSwitch);

    s32Ret = SAL_SOK;
exit:
    return s32Ret;
}

#if 0

/*******************************************************************************
* 函数名  : Sva_HalPrintInputInfo
* 描  述  : 打印输入信息(调试使用)
* 输  入  : pstIcfInputDataInfo: 输入数据信息
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
void Sva_HalPrintInputInfo(ICF_INPUT_DATA_V2 *pstIcfInputDataInfo)
{
    XSIE_SECURITY_INPUT_T *pstInputData = NULL;

    pstInputData = (XSIE_SECURITY_INPUT_T *)pstIcfInputDataInfo->stBlobData[0].pData;

    SVA_LOGW("===========================================================\n");
    SVA_LOGW("szl_dbg: blobNum %d \n", pstIcfInputDataInfo->nBlobNum);
    SVA_LOGW("szl_dbg: blob_format %d \n", pstIcfInputDataInfo->stBlobData[0].eBlobFormat);
    SVA_LOGW("szl_dbg: data_type %d \n", pstIcfInputDataInfo->stBlobData[0].nDataType);
    SVA_LOGW("szl_dbg: frame_num %llu \n", pstIcfInputDataInfo->stBlobData[0].nFrameNum);
    SVA_LOGW("szl_dbg: shape_0 %d \n", pstIcfInputDataInfo->stBlobData[0].nShape[0]);
    SVA_LOGW("szl_dbg: shape_1 %d \n", pstIcfInputDataInfo->stBlobData[0].nShape[1]);
    SVA_LOGW("szl_dbg: space %d \n", pstIcfInputDataInfo->stBlobData[0].nSpace);
    SVA_LOGW("szl_dbg: stride_0 %d \n", pstIcfInputDataInfo->stBlobData[0].nStride[0]);
    SVA_LOGW("szl_dbg: stride_1 %d \n", pstIcfInputDataInfo->stBlobData[0].nStride[1]);
    SVA_LOGW("szl_dbg: pts %llu \n", pstIcfInputDataInfo->stBlobData[0].nTimeStamp);
    SVA_LOGW("szl_dbg: pData %p \n", pstIcfInputDataInfo->stBlobData[0].pData);
    SVA_LOGW("szl_dbg: valid %d \n", pstInputData->InDataInfo.SourYuvRectValid);
    SVA_LOGW("============ Main  ==================\n");
    SVA_LOGW("szl_dbg: Main--format %d \n", pstInputData->InDataInfo.MainYuvData[0].format);
    SVA_LOGW("szl_dbg: Main--w %d, h %d \n", pstInputData->InDataInfo.MainYuvData[0].image_w, pstInputData->InDataInfo.MainYuvData[0].image_h);
    SVA_LOGW("szl_dbg: Main--pitch_y %d, pitch_uv %d \n", pstInputData->InDataInfo.MainYuvData[0].pitch_y, pstInputData->InDataInfo.MainYuvData[0].pitch_uv);
    SVA_LOGW("szl_dbg: Main--scale_rate %d \n", pstInputData->InDataInfo.MainYuvData[0].scale_rate);
    SVA_LOGW("szl_dbg: Main--y %p, u %p, v %p \n", pstInputData->InDataInfo.MainYuvData[0].y, pstInputData->InDataInfo.MainYuvData[0].u, pstInputData->InDataInfo.MainYuvData[0].v);
    SVA_LOGW("============ Side  ==================\n");
    SVA_LOGW("szl_dbg: Side--format %d \n", pstInputData->InDataInfo.SideYuvData[0].format);
    SVA_LOGW("szl_dbg: Side--w %d, h %d \n", pstInputData->InDataInfo.SideYuvData[0].image_w, pstInputData->InDataInfo.SideYuvData[0].image_h);
    SVA_LOGW("szl_dbg: Side--pitch_y %d, pitch_uv %d \n", pstInputData->InDataInfo.SideYuvData[0].pitch_y, pstInputData->InDataInfo.SideYuvData[0].pitch_uv);
    SVA_LOGW("szl_dbg: Side--scale_rate %d \n", pstInputData->InDataInfo.SideYuvData[0].scale_rate);
    SVA_LOGW("szl_dbg: Side--y %p, u %p, v %p \n", pstInputData->InDataInfo.SideYuvData[0].y, pstInputData->InDataInfo.SideYuvData[0].u, pstInputData->InDataInfo.SideYuvData[0].v);
    SVA_LOGW("============ Param =================\n");
    SVA_LOGW("szl_dbg: main_view_delay_time_seconds %d \n", pstInputData->InDataInfo.ParamIn.MainViewDelayTimeSeconds);
    SVA_LOGW("szl_dbg: main_x %.4f, main_y %.4f, main_w %.4f, main_h %.4f \n",
             pstInputData->InDataInfo.ParamIn.MainRoiRect.x, pstInputData->InDataInfo.ParamIn.MainRoiRect.y,
             pstInputData->InDataInfo.ParamIn.MainRoiRect.width, pstInputData->InDataInfo.ParamIn.MainRoiRect.height);
    SVA_LOGW("szl_dbg: side_view_delay_time_seconds %d \n", pstInputData->InDataInfo.ParamIn.SideViewDelayTimeSeconds);
    SVA_LOGW("szl_dbg: side_x %.4f, side_y %.4f, side_w %.4f, side_h %.4f \n",
             pstInputData->InDataInfo.ParamIn.SideRoiRect.x, pstInputData->InDataInfo.ParamIn.SideRoiRect.y,
             pstInputData->InDataInfo.ParamIn.SideRoiRect.width, pstInputData->InDataInfo.ParamIn.SideRoiRect.height);

    SVA_LOGW("szl_dbg: scale_rate %.4f \n", pstInputData->InDataInfo.ParamIn.ObdDownsampleScale);
    SVA_LOGW("szl_dbg: orientation %d \n", pstInputData->InDataInfo.ParamIn.Orientation);
    SVA_LOGW("szl_dbg: PDProcessGap %d \n", pstInputData->InDataInfo.ParamIn.PDProcessGap);
    SVA_LOGW("szl_dbg: ZoomInOutThres %f \n", pstInputData->InDataInfo.ParamIn.ZoomInOutThres);
    SVA_LOGW("szl_dbg: PackageSensity %d \n", pstInputData->InDataInfo.ParamIn.PackageSensity);
    SVA_LOGW("szl_dbg: Pk Mode %d \n", pstInputData->InDataInfo.ParamIn.PkSwitch);
    SVA_LOGW("szl_dbg: Spray Filter Switch %d \n", pstInputData->InDataInfo.ParamIn.SizeFilterSwitch);

    SVA_LOGW("============ conf ===============\n");
    for (UINT32 i = 0; i < XSIE_SECURITY_CLASS_NUM; i++)
    {
        SVA_LOGW("szl_dbg: i %d, Main-sensity %d, Side-sensity %d \n",
                 i, pstInputData->InDataInfo.ParamIn.MainDetSensity[i], pstInputData->InDataInfo.ParamIn.SideDetSensity[i]);
    }

    SVA_LOGW("===========================================================\n");

    return;
}

#endif

/**
 * @function   Sva_DrvPrintInputDataInfo
 * @brief      打印inputdata信息(调试接口)
 * @param[in]  ENGINE_COMM_INPUT_DATA_INFO *pstInputAddInfo
 * @param[out] None
 * @return     VOID
 */
VOID Sva_DrvPrintInputDataInfo(ENGINE_COMM_INPUT_DATA_INFO *pstInputAddInfo)
{
    /* 默认关闭 */
    return;

    UINT32 i, j;

    SVA_LOGW("pChnHandle %p \n", pstInputAddInfo->pChnHandle);

    ENGINE_COMM_IA_IMG_PRVT_CTRL_PRM *pstImgPrm = &pstInputAddInfo->stIaImgPrvtCtrlPrm;
    SVA_LOGW("u32InputDataImgNum %d \n", pstImgPrm->u32InputDataImgNum);

    for (i = 0; i < pstImgPrm->u32InputDataImgNum; i++)
    {
        SVA_LOGW("i %d \n", i);
        SVA_LOGW("u64FrameNum %llu, enInputDataMemType %d, enImgType %d \n",
                 pstImgPrm->stInputDataImg[i].u64FrameNum,
                 pstImgPrm->stInputDataImg[i].enInputDataMemType,
                 pstImgPrm->stInputDataImg[i].enImgType);
        SVA_LOGW("u32SourYuvRectValid %d \n", pstImgPrm->stInputDataImg[i].u32SourYuvRectValid);
        if (pstImgPrm->stInputDataImg[i].u32SourYuvRectValid)
        {
            SVA_LOGW("fYuvRect: [%f, %f] [%f, %f] \n",
                     pstImgPrm->stInputDataImg[i].fYuvRectX, pstImgPrm->stInputDataImg[i].fYuvRectY,
                     pstImgPrm->stInputDataImg[i].fYuvRectW, pstImgPrm->stInputDataImg[i].fYuvRectH);
        }

        SVA_LOGW("yuv: w %d, h %d, s[%d, %d, %d] \n",
                 pstImgPrm->stInputDataImg[i].u32W, pstImgPrm->stInputDataImg[i].u32H,
                 pstImgPrm->stInputDataImg[i].u32S[0], pstImgPrm->stInputDataImg[i].u32S[1], pstImgPrm->stInputDataImg[i].u32S[2]);

        SVA_LOGW("main: u32MainViewImgNum %d \n", pstImgPrm->stInputDataImg[i].u32MainViewImgNum);
        for (j = 0; j < pstImgPrm->stInputDataImg[i].u32MainViewImgNum; j++)
        {
            SVA_LOGW("main: j %d, w %d, h %d, s[%d, %d, %d], vir[%p, %p, %p] \n", j,
                     pstImgPrm->stInputDataImg[i].stMainViewImg[j].u32W,
                     pstImgPrm->stInputDataImg[i].stMainViewImg[j].u32H,
                     pstImgPrm->stInputDataImg[i].stMainViewImg[j].u32S[0],
                     pstImgPrm->stInputDataImg[i].stMainViewImg[j].u32S[1],
                     pstImgPrm->stInputDataImg[i].stMainViewImg[j].u32S[2],
                     pstImgPrm->stInputDataImg[i].stMainViewImg[j].pY_CompVa,
                     pstImgPrm->stInputDataImg[i].stMainViewImg[j].pU_CompVa,
                     pstImgPrm->stInputDataImg[i].stMainViewImg[j].pV_CompVa);
        }

        SVA_LOGW("side: u32SideViewImgNum %d \n", pstImgPrm->stInputDataImg[i].u32SideViewImgNum);
        for (j = 0; j < pstImgPrm->stInputDataImg[i].u32SideViewImgNum; j++)
        {
            SVA_LOGW("side: j %d, w %d, h %d, s[%d, %d, %d], vir[%p, %p, %p] \n", j,
                     pstImgPrm->stInputDataImg[i].stSideViewImg[j].u32W,
                     pstImgPrm->stInputDataImg[i].stSideViewImg[j].u32H,
                     pstImgPrm->stInputDataImg[i].stSideViewImg[j].u32S[0],
                     pstImgPrm->stInputDataImg[i].stSideViewImg[j].u32S[1],
                     pstImgPrm->stInputDataImg[i].stSideViewImg[j].u32S[2],
                     pstImgPrm->stInputDataImg[i].stSideViewImg[j].pY_CompVa,
                     pstImgPrm->stInputDataImg[i].stSideViewImg[j].pU_CompVa,
                     pstImgPrm->stInputDataImg[i].stSideViewImg[j].pV_CompVa);
        }
    }
}

/**
 * @function    Sva_HalSetDumpYuvCnt
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID Sva_HalSetDumpYuvCnt(UINT32 chan, UINT32 cnt, UINT32 mode)
{
    if (chan >= XSI_DEV_MAX)
    {
        SVA_LOGE("invalid chan %d \n", chan);
        return;
    }

    if ((0x1 << SVA_DUMP_YUV_COPY_XPACK) & mode)
    {
        g_s32DumpDataCnt[SVA_DUMP_YUV_COPY_XPACK][chan] = cnt;
    }

    if ((0x1 << SVA_DUMP_YUV_ENGINE) & mode)
    {
        g_s32DumpDataCnt[SVA_DUMP_YUV_ENGINE][chan] = cnt;
    }

    if ((0x1 << SVA_DUMP_YUV_XPACK) & mode)
    {
        g_s32DumpDataCnt[SVA_DUMP_YUV_XPACK][chan] = cnt;
    }

    if ((0x1 << SVA_DUMP_YUV_SLAVE_REC) & mode)
    {
        g_s32DumpDataCnt[SVA_DUMP_YUV_SLAVE_REC][chan] = cnt;
    }

    SVA_LOGW("=== set chan %d, dump yuv cnt %d \n", chan, cnt);
}

/**
 * @function   Sva_HalDumpInputImg
 * @brief      dump送入引擎的图像(当前仅安检机图片模式使用)
 * @param[in]  VOID *pImgVir
 * @param[in]  CHAR *pHead
 * @param[in]  UINT32 frmNum
 * @param[out] None
 * @return     static VOID
 */
VOID Sva_HalDumpInputImg(VOID *pImgVir, UINT32 wihgt, UINT32 hight, CHAR *pHead, UINT32 chan, UINT64 frmNum, SVA_DUMP_YUV_MODE mode)
{
    CHAR acPath[64] = {0};

    if (chan >= XSI_DEV_MAX)
    {
        SVA_LOGE("invalid chan %d \n", chan);
        return;
    }

    /*dump从xpack中copy出来的yuv图片*/
    if (g_s32DumpDataCnt[SVA_DUMP_YUV_COPY_XPACK][chan] > 0 && mode == SVA_DUMP_YUV_COPY_XPACK)
    {
        snprintf(acPath, 64, "%s/copy_xpack_frm_%llu_ch_%d_w_%d_h_%d.nv21", pHead, frmNum, chan, wihgt, hight);

        Ia_DumpYuvData(acPath, pImgVir, wihgt * hight * 3 / 2);

        g_s32DumpDataCnt[SVA_DUMP_YUV_COPY_XPACK][chan]--;

        SVA_LOGW("========= dump copy_xpack yuv end, left %d \n", g_s32DumpDataCnt[SVA_DUMP_YUV_COPY_XPACK][chan]);
    }

    /*dump送入引擎的yuv图片*/
    if (g_s32DumpDataCnt[SVA_DUMP_YUV_ENGINE][chan] > 0 && mode == SVA_DUMP_YUV_ENGINE)
    {
        snprintf(acPath, 64, "%s/engine_frm_%llu_ch_%d_w_%d_h_%d.nv21", pHead, frmNum, chan, wihgt, hight);

        Ia_DumpYuvData(acPath, pImgVir, wihgt * hight * 3 / 2);

        g_s32DumpDataCnt[SVA_DUMP_YUV_ENGINE][chan]--;

        SVA_LOGW("========= dump engine yuv end, left %d \n", g_s32DumpDataCnt[SVA_DUMP_YUV_ENGINE][chan]);
    }

    /*dump从xpack中获取的yuv图片*/
    if (g_s32DumpDataCnt[SVA_DUMP_YUV_XPACK][chan] > 0 && mode == SVA_DUMP_YUV_XPACK)
    {
        snprintf(acPath, 64, "%s/xpack_frm_%llu_ch_%d_w_%d_h_%d.nv21", pHead, frmNum, chan, wihgt, hight);

        Ia_DumpYuvData(acPath, pImgVir, wihgt * hight * 3 / 2);

        g_s32DumpDataCnt[SVA_DUMP_YUV_XPACK][chan]--;

        SVA_LOGW("========= dump xpack yuv end, left %d \n", g_s32DumpDataCnt[SVA_DUMP_YUV_XPACK][chan]);
    }

    /*dump获取从片接收的YUV图像 */
    if (g_s32DumpDataCnt[SVA_DUMP_YUV_SLAVE_REC][chan] > 0 && mode == SVA_DUMP_YUV_SLAVE_REC)
    {
        snprintf(acPath, 64, "%s/slave_rec_frm_%llu_ch_%d_w_%d_h_%d.nv21", pHead, frmNum, chan, wihgt, hight);

        Ia_DumpYuvData(acPath, pImgVir, wihgt * hight * 3 / 2);

        g_s32DumpDataCnt[SVA_DUMP_YUV_SLAVE_REC][chan]--;

        SVA_LOGW("========= dump slave_rec yuv end, left %d \n", g_s32DumpDataCnt[SVA_DUMP_YUV_SLAVE_REC][chan]);
    }
}

/**
 * @function    Sva_HalRlsPrvtUserInfo
 * @brief       释放输入的用户附加信息
 * @param[in]   VOID *pXsieUserPrvtInfo
 * @param[out]  none
 * @return
 */
INT32 Sva_HalRlsPrvtUserInfo(VOID *pXsieUserPrvtInfo)
{
    if (!pXsieUserPrvtInfo || NULL == ((XSIE_USER_PRVT_INFO *)pXsieUserPrvtInfo)->pstInputAddInfo)
    {
        SVA_LOGE("ptr null! %p \n", pXsieUserPrvtInfo);
        return SAL_FAIL;
    }

    ((SVA_INPUT_DATA *)(((XSIE_USER_PRVT_INFO *)pXsieUserPrvtInfo)->pstInputAddInfo))->bUseFlag = SAL_FALSE;

    SVA_LOGD("rls user prvt info end! \n");
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalInputDataToEngine
* 描  述  : 推帧接口
* 输  入  : - chan : 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalInputDataToEngine(UINT32 *puiChnSts, UINT32 *puiInputIdx, SVA_PROC_MODE_E mode)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;

    UINT32 time0 = 0;
    UINT32 time1 = 0;

    ENG_CHANNEL_PRM_S *pstEngChnPrm = NULL;
    XSI_COMMON *pstXsiCommon = NULL;
    ENGINE_COMM_INPUT_DATA_INFO *pstInputAddInfo = NULL;
    XSIE_USER_PRVT_INFO *pstUsrPrvtInfo = NULL;
    ENGINE_COMM_USER_PRVT_CTRL_PRM *pstUserPrvtCtrl = NULL;
    SVA_INPUT_DATA *pstSvaInputData = NULL;

    SVA_HAL_CHECK_PRM(puiChnSts, SAL_FAIL);

    if (mode >= SVA_PROC_MODE_NUM)
    {
        SVA_LOGE("Invalid mode %d >= max %d \n", mode, SVA_PROC_MODE_NUM);
        return SAL_FAIL;
    }

    /* print debug info */
    SVA_LOGD("send data to engine enter! mode %d \n", mode);
    for (i = 0; i < XSI_DEV_MAX; i++)
    {
        SVA_LOGD("i %d, chn_sts %d \n", i, puiChnSts[i]);
    }

    pstXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pstXsiCommon, SAL_FAIL);

    if (SVA_PROC_MODE_DUAL_CORRELATION == mode)
    {
        if (SAL_TRUE != puiChnSts[0] || SAL_TRUE != puiChnSts[1])
        {
            /* Xsi not Enable, No need Send Input Data */
            return SAL_SOK;
        }

        SVA_LOGD("mode %d entering! \n", mode);

        pstEngChnPrm = &pstXsiCommon->stEngChnPrm[0];

        /* sva模块全局输入数据 */
        pstSvaInputData = &pstEngChnPrm->astSvaInputData[puiInputIdx[0]];

        /* 引擎层通用输入数据结构体 */
        pstInputAddInfo = &pstSvaInputData->stInputDataAddInfo;

        /* 存放引擎通道句柄 */
        pstInputAddInfo->pChnHandle = pstEngChnPrm->pHandle;

        /* 填充私有数据回调函数 */
        pstInputAddInfo->stIaImgPrvtCtrlPrm.pFuncProcIaImg = (ENGINE_COMM_PROC_IA_IMG_FUNC)xsie_proc_alg_img;
        pstInputAddInfo->stIaPrmPrvtCtrlPrm.pFuncProcIaPrm = (ENGINE_COMM_PROC_IA_PRM_FUNC)xsie_proc_alg_prm;

        /* 填充双视角帧数据 */
        if (SAL_SOK != Sva_HalFillInputData(SAL_TRUE,
                                            SAL_FALSE,
                                            NULL,
                                            pstXsiCommon->xsi_dev[0].uiFrameNum,
                                            &pstXsiCommon->xsi_dev[0].stBuff[puiInputIdx[0]],
                                            &pstXsiCommon->xsi_dev[1].stBuff[puiInputIdx[0]],
                                            pstInputAddInfo))
        {
            SVA_LOGE("fill input data failed! mode %d \n", mode);
            return SAL_FAIL;
        }

        /* 用于存放用户透传信息的全局变量 */
        pstUsrPrvtInfo = &pstSvaInputData->stXsieUsrPrvtInfo;

        pstUsrPrvtInfo->pstInputAddInfo = (VOID *)pstSvaInputData;   /* 全局参数放入用户透传信息中，用于释放 */
        pstUsrPrvtInfo->mode = SVA_PROC_MODE_DUAL_CORRELATION;        /* 当前帧处理模式 */
        pstUsrPrvtInfo->nRealFrameNum = (long long)pstXsiCommon->xsi_dev[0].stSysFrame.uiDataLen;

        pstUsrPrvtInfo->stUsrPrm[0].uiInputTime = SAL_getCurMs();
        pstUsrPrvtInfo->stUsrPrm[0].devChn = 0;
        pstUsrPrvtInfo->stUsrPrm[0].uiEngHdlIdx = 0;

        pstUsrPrvtInfo->stUsrPrm[1].uiInputTime = SAL_getCurMs();
        pstUsrPrvtInfo->stUsrPrm[1].devChn = 1;

        /* 用户私有信息控制参数，主要存放私有信息透明化参数和回调处理函数指针 */
        pstUserPrvtCtrl = &pstSvaInputData->stInputDataAddInfo.stInputUserInfo.stUsrPrvtCtrlPrm;
        pstUserPrvtCtrl->pUsrData = pstUsrPrvtInfo;
        pstUserPrvtCtrl->pFuncRlsUserPrvtInfo = Sva_HalRlsPrvtUserInfo;
        pstUserPrvtCtrl->bUsed = SAL_TRUE;

        time0 = SAL_getCurMs();

        (VOID)Sva_DrvPrintInputDataInfo(pstInputAddInfo);

        s32Ret = g_pEngineCommFunc->input_data(pstInputAddInfo);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("ICF_Input_data error, ret: 0x%x, mode %d, pHandle %p\n", s32Ret, mode, pstEngChnPrm->pHandle);
            return SAL_FAIL;
        }

        time1 = SAL_getCurMs();

        if (uiSvaAlgDbgFlag && (time1 - time0 > 15))
        {
            SZL_DBG_LOG("input cost %d ms. mode %d \n", time1 - time0, mode);
        }

        pstXsiCommon->xsi_dev[0].uiFrameNum++;
        pstXsiCommon->xsi_dev[1].uiFrameNum++;
    }
    else if (SVA_PROC_MODE_VIDEO == mode)
    {
        for (i = 0; i < XSI_DEV_MAX; i++)
        {
            if (SAL_TRUE != puiChnSts[i])
            {
                continue;
            }

            SVA_LOGD("mode %d entering! i %d \n", mode, i);

            pstEngChnPrm = &pstXsiCommon->stEngChnPrm[0];

            /* sva模块全局输入数据 */
            pstSvaInputData = &pstEngChnPrm->astSvaInputData[puiInputIdx[0]];

            /* 引擎层通用输入数据结构体 */
            pstInputAddInfo = &pstSvaInputData->stInputDataAddInfo;

            /* 保存引擎通道句柄 */
            pstInputAddInfo->pChnHandle = pstEngChnPrm->pHandle;

            /* 填充私有数据回调函数 */
            pstInputAddInfo->stIaImgPrvtCtrlPrm.pFuncProcIaImg = (ENGINE_COMM_PROC_IA_IMG_FUNC)xsie_proc_alg_img;
            pstInputAddInfo->stIaPrmPrvtCtrlPrm.pFuncProcIaPrm = (ENGINE_COMM_PROC_IA_PRM_FUNC)xsie_proc_alg_prm;

            /* 填充一路视角帧数据 */
            if (SAL_SOK != Sva_HalFillInputData(SAL_FALSE,
                                                SAL_FALSE,
                                                NULL,
                                                pstXsiCommon->xsi_dev[i].uiFrameNum,
                                                &pstXsiCommon->xsi_dev[i].stBuff[puiInputIdx[0]],
                                                &pstXsiCommon->xsi_dev[1 - i].stBuff[puiInputIdx[0]],
                                                pstInputAddInfo))
            {
                SVA_LOGE("fill input data failed! mode %d \n", mode);
                return SAL_FAIL;
            }

            /* 用于存放用户透传信息的全局变量 */
            pstUsrPrvtInfo = &pstSvaInputData->stXsieUsrPrvtInfo;

            pstUsrPrvtInfo->pstInputAddInfo = (VOID *)pstSvaInputData;   /* 全局参数放入用户透传信息中，用于释放 */
            pstUsrPrvtInfo->mode = SVA_PROC_MODE_VIDEO;        /* 当前帧处理模式 */
            pstUsrPrvtInfo->nRealFrameNum = (long long)pstXsiCommon->xsi_dev[i].stSysFrame.uiDataLen;

            pstUsrPrvtInfo->stUsrPrm[0].uiInputTime = SAL_getCurMs();
            pstUsrPrvtInfo->stUsrPrm[0].devChn = i;
            pstUsrPrvtInfo->stUsrPrm[0].uiEngHdlIdx = 0;

            /* 用户私有信息控制参数，主要存放私有信息透明化参数和回调处理函数指针 */
            pstUserPrvtCtrl = &pstSvaInputData->stInputDataAddInfo.stInputUserInfo.stUsrPrvtCtrlPrm;
            pstUserPrvtCtrl->pUsrData = (VOID *)pstUsrPrvtInfo;
            pstUserPrvtCtrl->pFuncRlsUserPrvtInfo = Sva_HalRlsPrvtUserInfo;
            pstUserPrvtCtrl->bUsed = SAL_TRUE;

            SVA_LOGD("input--i %d, mode %d, frame_num %d, inputId %d, %p \n",
                     i, SVA_PROC_MODE_VIDEO, pstXsiCommon->xsi_dev[i].uiFrameNum, puiInputIdx[0], pstInputAddInfo);

            time0 = SAL_getCurMs();

            s32Ret = g_pEngineCommFunc->input_data(pstInputAddInfo);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("ICF_Input_data error, ret: 0x%x, mode %d, pHandle %p\n", s32Ret, mode, pstEngChnPrm->pHandle);
                return SAL_FAIL;
            }

            time1 = SAL_getCurMs();

            if (uiSvaAlgDbgFlag && (time1 - time0 > 15))
            {
                SVA_LOGW("input cost %d ms. i %d, mode %d \n", time1 - time0, i, mode);
            }

            pstXsiCommon->xsi_dev[i].uiFrameNum++;
        }
    }
    else
    {
        for (i = 0; i < XSI_DEV_MAX; i++)
        {
            if (SAL_TRUE != puiChnSts[i])
            {
                continue;
            }

            pstEngChnPrm = &pstXsiCommon->stEngChnPrm[i];

            SVA_LOGD("mode %d entering! chan(%d)\n", mode, i);

            /* sva模块全局输入数据 */
            pstSvaInputData = &pstEngChnPrm->astSvaInputData[puiInputIdx[i]];

            /* 引擎层通用输入数据结构体 */
            pstInputAddInfo = &pstSvaInputData->stInputDataAddInfo;

            /* 保存引擎通道句柄 */
            pstInputAddInfo->pChnHandle = pstEngChnPrm->pHandle;

            /* 填充私有数据回调函数 */
            pstInputAddInfo->stIaImgPrvtCtrlPrm.pFuncProcIaImg = (ENGINE_COMM_PROC_IA_IMG_FUNC)xsie_proc_alg_img;
            pstInputAddInfo->stIaPrmPrvtCtrlPrm.pFuncProcIaPrm = (ENGINE_COMM_PROC_IA_PRM_FUNC)xsie_proc_alg_prm;

            /* 用于存放用户透传信息的全局变量 */
            pstUsrPrvtInfo = &pstSvaInputData->stXsieUsrPrvtInfo;

            pstInputAddInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stInLcdData[0].FeatW   = pstUsrPrvtInfo->stUsrPrm[0].imgW;
            pstInputAddInfo->stIaImgPrvtCtrlPrm.stInputDataImg[0].stInLcdData[0].FeatH   = pstUsrPrvtInfo->stUsrPrm[0].imgH;

#if defined NT98336 || defined RK3588
            /* 图像模式使用，需要配置实际包裹在背景buffer上的位置，用于引擎内部单个违禁品识别流程优化，当前仅NT支持 */
            SVA_RECT_F stPkgRect = {0};

            stPkgRect.x = 0.0f;
            stPkgRect.y = 0.0f;
            stPkgRect.width = (float)pstUsrPrvtInfo->stUsrPrm[0].imgW / SVA_MODULE_WIDTH;
            stPkgRect.height = (float)pstUsrPrvtInfo->stUsrPrm[0].imgH / SVA_MODULE_HEIGHT;

            /*若输入的坐标及有效宽高超出范围，则报错返回，该接口外部会释放送入引擎的buff*/
            if(stPkgRect.x < 0.0f || stPkgRect.x > 1.0f || stPkgRect.y < 0.0f || stPkgRect.y > 1.0f || \
                stPkgRect.width <= 0.0f || (stPkgRect.x + stPkgRect.width) > 1.0f || stPkgRect.height <= 0.0f || (stPkgRect.x + stPkgRect.height) > 1.0f)
            {
                SVA_LOGE("Vaild w,h to engine is ERR!!!: stPkgRect.x_%f stPkgRect.y_%f stPkgRect.width_%f stPkgRect.height_%f \n", stPkgRect.x, stPkgRect.y, stPkgRect.width, stPkgRect.height);
                return SAL_FAIL;
            }
            
            SVA_LOGI("Vaild w,h to engine: stPkgRect.x_%f stPkgRect.y_%f stPkgRect.width_%f stPkgRect.height_%f \n", stPkgRect.x, stPkgRect.y, stPkgRect.width, stPkgRect.height);
#endif

            /* 填充一路视角帧数据 */
            if (SAL_SOK != Sva_HalFillInputData(SAL_FALSE,
                                                SAL_TRUE,
#if defined NT98336 || defined RK3588 /* 当前仅NT和RK支持图片模式内部流程优化贴图修改 */
                                                &stPkgRect,
#else
                                                NULL,
#endif
                                                pstXsiCommon->xsi_dev[i].uiFrameNum,
                                                &pstXsiCommon->xsi_dev[i].stBuff[puiInputIdx[i]],
                                                &pstXsiCommon->xsi_dev[1 - i].stBuff[puiInputIdx[i]],
                                                pstInputAddInfo))
            {
                SVA_LOGE("fill input data failed! mode %d \n", mode);
                return SAL_FAIL;
            }

            /* 调试接口，dump送入引擎的图像 */
            Sva_HalDumpInputImg(pstXsiCommon->xsi_dev[i].stBuff[puiInputIdx[i]].stMemBuf.VirAddr,
                                SVA_MODULE_WIDTH,
                                SVA_MODULE_HEIGHT,
                                INPUT_IMG_DUMP_PATH, i,
                                pstXsiCommon->xsi_dev[i].stBuff[puiInputIdx[i]].stUsrPrm.pts,
                                SVA_DUMP_YUV_ENGINE);

            pstUsrPrvtInfo->pstInputAddInfo = (VOID *)pstSvaInputData;   /* 全局参数放入用户透传信息中，用于释放 */
            pstUsrPrvtInfo->mode = SVA_PROC_MODE_IMAGE;    /* 当前帧处理模式 */
            pstUsrPrvtInfo->nRealFrameNum = (long long)pstXsiCommon->xsi_dev[i].stSysFrame.uiDataLen;

            pstUsrPrvtInfo->stUsrPrm[0].uiInputTime = SAL_getCurMs();
            pstUsrPrvtInfo->stUsrPrm[0].devChn = i;
            pstUsrPrvtInfo->stUsrPrm[0].uiEngHdlIdx = i;

            SVA_LOGD("chan %d completePackMark %d packIndx %d packTop %d packBottom %d  packOffsetX %d\n",
                     i, pstUsrPrvtInfo->stUsrPrm[i].stAiPrm.completePackMark, \
                     pstUsrPrvtInfo->stUsrPrm[i].stAiPrm.packIndx, pstUsrPrvtInfo->stUsrPrm[i].stAiPrm.packTop, \
                     pstUsrPrvtInfo->stUsrPrm[i].stAiPrm.packBottom, pstUsrPrvtInfo->stUsrPrm[i].stAiPrm.packOffsetX);
            /* 用户私有信息控制参数，主要存放私有信息透明化参数和回调处理函数指针 */
            pstUserPrvtCtrl = &pstSvaInputData->stInputDataAddInfo.stInputUserInfo.stUsrPrvtCtrlPrm;
            pstUserPrvtCtrl->pUsrData = (VOID *)pstUsrPrvtInfo;
            pstUserPrvtCtrl->pFuncRlsUserPrvtInfo = Sva_HalRlsPrvtUserInfo;
            pstUserPrvtCtrl->bUsed = SAL_TRUE;

            time0 = SAL_getCurMs();

            s32Ret = g_pEngineCommFunc->input_data(pstInputAddInfo);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("ICF_Input_data error, ret: 0x%x, mode %d, pHandle %p\n", s32Ret, mode, pstEngChnPrm->pHandle);
                return SAL_FAIL;
            }

            time1 = SAL_getCurMs();

            if (uiSvaAlgDbgFlag && (time1 - time0 > 15))
            {
                SVA_LOGW("input cost %d ms. mode %d \n", time1 - time0, mode);
            }

            SVA_LOGD("i %d, mode %d, frame_num %d puiInputIdx %d \n", i, mode, pstXsiCommon->xsi_dev[i].uiFrameNum, puiInputIdx[0]);

            pstXsiCommon->xsi_dev[i].uiFrameNum++;
        }
    }

    return s32Ret;
}

/**
 * @function:   Sva_HalFillInputPrm
 * @brief:      填充引擎内部配置参数
 * @param[in]:  BOOL bDoubleView
 * @param[in]:  SVA_PROCESS_IN *pstInMain
 * @param[in]:  SVA_PROCESS_IN *pstInSide
 * @param[out]: XSIE_SECURITY_INPUT_PARAM_T *pstInputPrm
 * @return:     ENGINE_COMM_FUNC_P *
 */
static INT32 Sva_HalFillInputPrm(IN BOOL bDoubleView,
                                 IN SVA_PROCESS_IN *pstInMain,
                                 IN SVA_PROCESS_IN *pstInSide,
                                 OUT XSIE_SECURITY_INPUT_PARAM_T *pstInputPrm)
{
    pstInputPrm->MainRoiRect.x = pstInMain->enDirection ? 1 - pstInMain->rect.x - pstInMain->rect.width : pstInMain->rect.x;
    pstInputPrm->MainRoiRect.y = pstInMain->rect.y;
    pstInputPrm->MainRoiRect.width = pstInMain->rect.width;
    pstInputPrm->MainRoiRect.height = pstInMain->rect.height;
    pstInputPrm->MainViewDelayTimeSeconds = 30;   /* TODO: need confirmation */

    if (bDoubleView)
    {
        pstInputPrm->SideRoiRect.x = pstInSide->enDirection ? 1 - pstInSide->rect.x - pstInSide->rect.width : pstInSide->rect.x;;
        pstInputPrm->SideRoiRect.y = pstInSide->rect.y;
        pstInputPrm->SideRoiRect.width = pstInSide->rect.width;
        pstInputPrm->SideRoiRect.height = pstInSide->rect.height;
        pstInputPrm->SideViewDelayTimeSeconds = 30;   /* TODO: need confirmation */
    }
    /*
        为优化从左到右过包时引擎内部镜像的资源消耗，故当在从左到右过包时外部DSP将图像镜像后送入引擎，即引擎内部认为图像均为从右向左.
        当前下述通用参数不区分视角，两个视角公用同一套参数
     */
    pstInputPrm->Orientation = 0; /* pstInMain->enDirection; */
    pstInputPrm->PDProcessGap = 6; /*Sva_HalGetProcGap();*/
#if 0
    pstInputPrm->OBDProcessGap = 6;
#endif
    pstInputPrm->ObdDownsampleScale = 1.0f; /* pstInMain->fScale; */
    pstInputPrm->PackageSensity = pstInMain->uiPkgSensity;
    pstInputPrm->ZoomInOutThres = pstInMain->fZoomInOutVal;
    pstInputPrm->PackOverX2 = 0.998;
    pstInputPrm->PkSwitch = pstInMain->uiPkSwitch;   /* szl_todo: 参数与icf版本一致 */
    /* pstInputPrm->PackageSensity = pstInMain->uiSprayFilterSwitch; */
    pstInputPrm->ColorFilterSwitch = 1;
    pstInputPrm->SizeFilterSwitch = 1;

    SVA_LOGD("PkSwitch %d \n", pstInputPrm->PkSwitch);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalUpdateInputParam
* 描  述  : 更新引擎输入参数
* 输  入  : - mode : 引擎模式
          : - uiMainChn:主通道号
          : - *puiChnSts:引擎开关状态
          : - *puiInputIdx:引擎输入空闲Input索引项
          : - *pstIn:引擎功能参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalUpdateInputParam(SVA_PROC_MODE_E mode, UINT32 uiMainChn, UINT32 *puiChnSts, UINT32 *puiInputIdx, SVA_PROCESS_IN *pstIn)
{
    UINT32 i = 0;

    XSI_COMMON *pstXsiCommon = NULL;
    SVA_INPUT_DATA *pstSvaInputData = NULL;
    ENGINE_COMM_INPUT_DATA_INFO *pstInputAddInfo = NULL;
    XSIE_USER_PRVT_INFO *pstXsieUsrPrvtInfo = NULL;

    /* Input Args Checker */
    SVA_HAL_CHECK_CHAN(uiMainChn, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstIn, SAL_FAIL);
    SVA_HAL_CHECK_PRM(puiChnSts, SAL_FAIL);
    SVA_HAL_CHECK_PRM(puiInputIdx, SAL_FAIL);

    pstXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pstXsiCommon, SAL_FAIL);

    /* Print Debug Info */
    SVA_LOGD("Update Input param enter! mode %d \n", mode);

    for (i = 0; i < XSI_DEV_MAX; i++)
    {
        SVA_LOGD("i %d, chn_sts %d \n", i, puiChnSts[i]);
        SVA_LOGD("x,y [%.4f, %.4f] w,h [%.4f, %.4f] \n", pstIn[i].rect.x, pstIn[i].rect.y, pstIn[i].rect.width, pstIn[i].rect.height);
    }

    /* main or side decided by chan and mode */
    if (SVA_PROC_MODE_DUAL_CORRELATION == mode)
    {
        if (SAL_TRUE != puiChnSts[0] || SAL_TRUE != puiChnSts[1])
        {
            SVA_LOGE("mode %d, chan[%d, %d] \n", mode, puiChnSts[0], puiChnSts[1]);
            return SAL_FAIL;
        }

        pstSvaInputData = &pstXsiCommon->stEngChnPrm[0].astSvaInputData[puiInputIdx[0]];

        pstInputAddInfo = &pstSvaInputData->stInputDataAddInfo;
        pstXsieUsrPrvtInfo = &pstSvaInputData->stXsieUsrPrvtInfo;

        /* 填充用户透传参数 */
        for (i = 0; i < XSI_DEV_MAX; i++)
        {
            sal_memcpy_s(&pstXsieUsrPrvtInfo->stUsrPrm[i],
                         sizeof(XSI_DEV_USR_PRM_S),
                         &pstXsiCommon->xsi_dev[i].stBuff[puiInputIdx[i]].stUsrPrm,
                         sizeof(XSI_DEV_USR_PRM_S));
        }

        /* 填充送引擎的配置参数 */
        (VOID)Sva_HalFillInputPrm(SAL_TRUE,
                                  &pstIn[uiMainChn],
                                  &pstIn[1 - uiMainChn],
                                  &pstSvaInputData->stXsieAlgPrm);

        /* 将结果放入后级发送engine的通用结构体中 */
        pstInputAddInfo->stIaPrmPrvtCtrlPrm.pIaAlgPrm = (VOID *)&pstSvaInputData->stXsieAlgPrm;

        /* 实际的过包方向通过用户私有参数传输 */
        pstXsieUsrPrvtInfo->uiDirection = pstIn[uiMainChn].enDirection;
    }
    else if (SVA_PROC_MODE_VIDEO == mode)
    {
        for (i = 0; i < XSI_DEV_MAX; i++)
        {
            if (SAL_TRUE != puiChnSts[i])
            {
                continue;
            }

            SVA_LOGD("video mode enter! i %d \n", i);

            pstSvaInputData = &pstXsiCommon->stEngChnPrm[0].astSvaInputData[puiInputIdx[0]];

            pstInputAddInfo = &pstSvaInputData->stInputDataAddInfo;
            pstXsieUsrPrvtInfo = &pstSvaInputData->stXsieUsrPrvtInfo;

            /* 填充用户透传参数 */
            sal_memcpy_s(&pstXsieUsrPrvtInfo->stUsrPrm[0],
                         sizeof(XSI_DEV_USR_PRM_S),
                         &pstXsiCommon->xsi_dev[i].stBuff[puiInputIdx[i]].stUsrPrm,
                         sizeof(XSI_DEV_USR_PRM_S));

            /* 填充送引擎的配置参数 */
            (VOID)Sva_HalFillInputPrm(SAL_FALSE,
                                      &pstIn[uiMainChn],
                                      &pstIn[1 - uiMainChn],
                                      &pstSvaInputData->stXsieAlgPrm);

            /* 将结果放入后级发送engine的通用结构体中 */
            pstInputAddInfo->stIaPrmPrvtCtrlPrm.pIaAlgPrm = (VOID *)&pstSvaInputData->stXsieAlgPrm;

            /* 实际的过包方向通过用户私有参数传输 */
            pstXsieUsrPrvtInfo->uiDirection = pstIn[uiMainChn].enDirection;
        }
    }
    else if (SVA_PROC_MODE_IMAGE == mode)
    {
        for (i = 0; i < XSI_DEV_MAX; i++)
        {
            if (SAL_TRUE != puiChnSts[i])
            {
                continue;
            }

            pstSvaInputData = &pstXsiCommon->stEngChnPrm[i].astSvaInputData[puiInputIdx[i]];

            pstInputAddInfo = &pstSvaInputData->stInputDataAddInfo;
            pstXsieUsrPrvtInfo = &pstSvaInputData->stXsieUsrPrvtInfo;

            sal_memcpy_s(&pstXsieUsrPrvtInfo->stUsrPrm[0],
                         sizeof(XSI_DEV_USR_PRM_S),
                         &pstXsiCommon->xsi_dev[i].stBuff[puiInputIdx[i]].stUsrPrm,
                         sizeof(XSI_DEV_USR_PRM_S));

            /* 填充用户透传参数 */
            sal_memcpy_s(&pstXsieUsrPrvtInfo->stUsrPrm[0].stAiPrm,
                         sizeof(AI_PACK_PRM),
                         &pstIn[0].stAiPrm[i],
                         sizeof(AI_PACK_PRM));

            /* 填充送引擎的配置参数 */
            (VOID)Sva_HalFillInputPrm(SAL_FALSE,
                                      &pstIn[i],
                                      &pstIn[1 - i],
                                      &pstSvaInputData->stXsieAlgPrm);

            /* 将结果放入后级发送engine的通用结构体中 */
            pstInputAddInfo->stIaPrmPrvtCtrlPrm.pIaAlgPrm = (VOID *)&pstSvaInputData->stXsieAlgPrm;
            /* 实际的过包方向通过用户私有参数传输 */
            pstXsieUsrPrvtInfo->uiDirection = pstIn[uiMainChn].enDirection;
        }
    }
    else
    {
        /* DO NOTHING... */
        SVA_LOGE("Invalid Proc Mode %d >= Max %d \n", mode, SVA_PROC_MODE_NUM);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    Ia_CpuQuickCopyYuv   //szl_todo: cpu拷贝yuv接口后续删除
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Ia_CpuQuickCopyYuv(SYSTEM_FRAME_INFO *pSrcFrame, SYSTEM_FRAME_INFO *pDstFrame, TDE_HAL_RECT *pSrcRect, TDE_HAL_RECT *pDstRect)
{
    /*入参校验*/
    SVA_HAL_CHECK_PRM(pSrcFrame, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pDstFrame, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pSrcRect, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pDstRect, SAL_FAIL);

    INT32 i = 0;
    CHAR *srcY = NULL;
    CHAR *srcVU = NULL;
    CHAR *dstY = NULL;
    CHAR *dstVU = NULL;
    INT32 uBufSize = SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT;
    SAL_VideoFrameBuf stSrcVideoFrmBuf = {0};
    SAL_VideoFrameBuf stDstVideoFrmBuf = {0};
    TDE_HAL_SURFACE srcSurface = {0};
    TDE_HAL_SURFACE dstSurface = {0};
    TDE_HAL_RECT srcRect = {0};
    TDE_HAL_RECT dstRect = {0};

    INT32 uiStride = 0;

    /* 此处单双芯片送入推帧接口的数据进行统一，所以此处跨距统一为SVA_MODULE_WIDTH*/
    uiStride = SVA_MODULE_WIDTH;

    (VOID)sys_hal_getVideoFrameInfo(pSrcFrame, &stSrcVideoFrmBuf);
    (VOID)sys_hal_getVideoFrameInfo(pDstFrame, &stDstVideoFrmBuf);

    srcSurface.u32Width = stSrcVideoFrmBuf.frameParam.width;
    srcSurface.u32Height = stSrcVideoFrmBuf.frameParam.height;
    srcSurface.u32Stride = stSrcVideoFrmBuf.stride[0];
    srcSurface.enColorFmt = stSrcVideoFrmBuf.frameParam.dataFormat;
    srcSurface.PhyAddr = stSrcVideoFrmBuf.phyAddr[0];
    srcRect.s32Xpos = pSrcRect->s32Xpos;
    srcRect.s32Ypos = pSrcRect->s32Ypos;
    srcRect.u32Width = pSrcRect->u32Width;
    srcRect.u32Height = pSrcRect->u32Height;

    dstSurface.u32Width = stDstVideoFrmBuf.frameParam.width;
    dstSurface.u32Height = stDstVideoFrmBuf.frameParam.height;
    dstSurface.u32Stride = stDstVideoFrmBuf.stride[0];
    dstSurface.enColorFmt = stDstVideoFrmBuf.frameParam.dataFormat;
    dstSurface.PhyAddr = stDstVideoFrmBuf.phyAddr[0];
    dstRect.s32Xpos = pDstRect->s32Xpos;
    dstRect.s32Ypos = pDstRect->s32Ypos;
    dstRect.u32Width = pDstRect->u32Width;
    dstRect.u32Height = pDstRect->u32Height;


    SVA_LOGD("src: virAddr %llx, phyAddr %llx, w %u, h %u, Rect x %d, y %d, w %u, h %u \n", stSrcVideoFrmBuf.virAddr[0], srcSurface.PhyAddr, srcSurface.u32Width, srcSurface.u32Height,  \
             srcRect.s32Xpos, srcRect.s32Ypos, srcRect.u32Width, srcRect.u32Height);

    SVA_LOGD("dst: Addr %llx, w %u, h %u, Rect x %d, y %d, w %u, h %u \n", dstSurface.PhyAddr, dstSurface.u32Width, dstSurface.u32Height,  \
             dstRect.s32Xpos, dstRect.s32Ypos, dstRect.u32Width, dstRect.u32Height);

    srcY = (CHAR *)stSrcVideoFrmBuf.virAddr[0];
    srcVU = (CHAR *)(srcY + uiStride * srcRect.u32Height);

    dstY = (CHAR *)stDstVideoFrmBuf.virAddr[0];
    dstVU = (CHAR *)(dstY + uBufSize);

    sal_memset_s(dstY, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT, 0xeb, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT);   /*默认底色为白色*/
    /*sal_memset_s(dstY, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT, 0xFF, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT);     默认底色为白色*/
    sal_memset_s(dstVU, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT / 2, 0x80, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT / 2);
    for (i = 0; i < srcRect.u32Height; i++)
    {
        sal_memcpy_s(dstY + dstRect.s32Xpos, srcRect.u32Width, srcY, srcRect.u32Width);
        srcY += uiStride;
        dstY += SVA_MODULE_WIDTH;
        if (i % 2 == 0)
        {
#if 0
            /*NT 专用 (NV21 转NV12)*/
            for (int j = 0; j < srcRect.u32Width; j++)
            {
                dstVU[j + 1 + dstRect.s32Xpos] = srcVU[j];
            }

#endif
            sal_memcpy_s(dstVU + dstRect.s32Xpos, srcRect.u32Width, srcVU, srcRect.u32Width);
            srcVU += uiStride;
            dstVU += SVA_MODULE_WIDTH;
        }
    }

#if 0
    static int cnt1 = 0;
    char path1[64] = {0};
    SAL_clear(path1);
    sprintf((CHAR *)path1, "./sva_out/dst_%d.yuv", cnt1);
    FILE *fp = NULL;
    fp = fopen(path1, "wb");
    fwrite(dstY, uBufSize, 1, fp);
    fclose(fp);
    fp = NULL;
    cnt1++;
#endif

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalQuickCopyData
* 描  述  : 将VB数据拷贝到缓存
* 输  入  : - chan : 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalQuickCopyData(UINT32 chan, SVA_PROC_MODE_E enProcMode, SYSTEM_FRAME_INFO *pstSrcFrame, UINT32 *puiInputIdx)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiInputIdx = 0;

    TDE_HAL_RECT srcPos = {0};
    TDE_HAL_RECT dstPos = {0};
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    XSI_DEV *pstXsi = NULL;
    XSI_DEV_BUFF *pstDevBuff = NULL;
    XSI_COMMON *pstXsiCommon = NULL;
    CAPB_PRODUCT *pstPlatCap = NULL;

    /* Input Args Checker */
    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstSrcFrame, SAL_FAIL);

    if (0x00 == pstSrcFrame->uiAppData)
    {
        SVA_LOGE("Src Frame Err! chan %d \n", chan);
        return SAL_FAIL;
    }

    pstXsi = Sva_HalGetDev(chan);
    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    pstXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pstXsiCommon, SAL_FAIL);

#if 1
    uiInputIdx = (SVA_PROC_MODE_IMAGE == enProcMode ? puiInputIdx[chan] : puiInputIdx[0]);
#endif

    pstDevBuff = &pstXsi->stBuff[uiInputIdx];

    if (0x00 == pstXsi->stSysFrame.uiAppData)
    {
        if (SAL_SOK != sys_hal_allocVideoFrameInfoSt(&pstXsi->stSysFrame))
        {
            SVA_LOGE("alloc frame info st failed! \n");
            return SAL_FAIL;
        }
    }

    stVideoFrmBuf.vbBlk = pstDevBuff->stMemBuf.u64VbBlk;
    stVideoFrmBuf.frameParam.width = SVA_MODULE_WIDTH;
    stVideoFrmBuf.frameParam.height = SVA_MODULE_HEIGHT;
    stVideoFrmBuf.frameParam.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;

    stVideoFrmBuf.stride[0] = SVA_MODULE_WIDTH;
    stVideoFrmBuf.stride[1] = SVA_MODULE_WIDTH;
    stVideoFrmBuf.stride[2] = SVA_MODULE_WIDTH;

    stVideoFrmBuf.phyAddr[0] = (PhysAddr)pstDevBuff->stMemBuf.PhyAddr;
    stVideoFrmBuf.phyAddr[1] = stVideoFrmBuf.phyAddr[0] + SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT;
    stVideoFrmBuf.phyAddr[2] = stVideoFrmBuf.phyAddr[1];


    stVideoFrmBuf.virAddr[0] = (PhysAddr)pstDevBuff->stMemBuf.VirAddr;
    stVideoFrmBuf.virAddr[1] = (PhysAddr)((CHAR *)stVideoFrmBuf.virAddr[0] + SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT);
    stVideoFrmBuf.virAddr[2] = stVideoFrmBuf.virAddr[1];

    s32Ret = sys_hal_buildVideoFrame(&stVideoFrmBuf, &pstXsi->stSysFrame);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("build video frame failed! \n");
        return SAL_FAIL;
    }

    s32Ret = sys_hal_getVideoFrameInfo(pstSrcFrame, &stVideoFrmBuf);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("get video frame info failed! \n");
        return SAL_FAIL;
    }

    pstPlatCap = capb_get_product();
    if (NULL == pstPlatCap)
    {
        SVA_LOGE("get platform capbility fail! \n");
        return SAL_FAIL;
    }
    if (VIDEO_INPUT_INSIDE == pstPlatCap->enInputType)
    {
        /* Save Frame Ref Index */
        srcPos.s32Xpos = 0;
        srcPos.s32Ypos = 0;
        srcPos.u32Width = stVideoFrmBuf.frameParam.width;
        srcPos.u32Height = stVideoFrmBuf.frameParam.height;
        dstPos.s32Xpos = 0;
        dstPos.s32Ypos = 0;
        dstPos.u32Width = stVideoFrmBuf.frameParam.width;
        dstPos.u32Height = stVideoFrmBuf.frameParam.height;
        pstDevBuff->stUsrPrm.syncPts = stVideoFrmBuf.pts;
        pstDevBuff->stUsrPrm.timeRef = stVideoFrmBuf.frameNum;
        SVA_LOGI("before Yuv Cpy---chan %d, src_w %d, src_h %d x %d, syncPts %llu Input TimeRef is %d\n",
                 chan, srcPos.u32Width, srcPos.u32Height, dstPos.s32Xpos, pstDevBuff->stUsrPrm.syncPts, pstDevBuff->stUsrPrm.timeRef);

        /*将拷贝的高设置为推帧图片的高，避免出现uv位置错误的问题*/
        srcPos.u32Height = SVA_MODULE_HEIGHT;

        /*copy 之前先清空缓存，copy中会将背景置为白色*/
        memset((char *)pstDevBuff->stMemBuf.VirAddr, 0x00, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2);

#if 0    /*copy前的图片*/
        INT8 framename[64] = {0};
        sys_hal_getVideoFrameInfo(pstSrcFrame, &stVideoFrmBuf);
        sprintf((CHAR *)framename, "./out/sva_copy_before/%d-%d slaveputengine-b-s.yuv", stVideoFrmBuf.frameNum, chan);
        Sva_HalDebugDumpData((CHAR *)stVideoFrmBuf.virAddr[0], (CHAR *)framename, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2, 0);
#endif

        /*copy yuv data*/
        (VOID)Ia_CpuQuickCopyYuv(pstSrcFrame, &pstXsi->stSysFrame, &srcPos, &dstPos);


#if 0   /*copy后的图片*/
        INT8 framename1[64] = {0};
        sys_hal_getVideoFrameInfo(pstSrcFrame, &stVideoFrmBuf);
        sprintf((CHAR *)framename1, "./out/sva_copy/%d-%d slaveputengine-a-s", stVideoFrmBuf.frameNum, chan);
        sys_hal_getVideoFrameInfo(&pstXsi->stSysFrame, &stVideoFrmBuf);
        Sva_HalDebugDumpData((CHAR *)stVideoFrmBuf.virAddr[0], (CHAR *)framename1, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2, 0);
#endif

    }
    else if (VIDEO_INPUT_OUTSIDE == pstPlatCap->enInputType)
    {
        SAL_VideoFrameBuf stVideoFrmBuf_1 = {0};

        if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSrcFrame, &stVideoFrmBuf_1))
        {
            SVA_LOGE("get video frame info failed! \n");
            return SAL_FAIL;
        }

        SVA_LOGD("src: w %d, h %d, vb %p, s [%d, %d, %d], vir [%p, %p, %p], phy [%p, %p, %p], fmt %d, \n",
                 stVideoFrmBuf_1.frameParam.width, stVideoFrmBuf_1.frameParam.height, (VOID *)stVideoFrmBuf_1.vbBlk,
                 stVideoFrmBuf_1.stride[0], stVideoFrmBuf_1.stride[1], stVideoFrmBuf_1.stride[2],
                 (VOID *)stVideoFrmBuf_1.virAddr[0], (VOID *)stVideoFrmBuf_1.virAddr[1], (VOID *)stVideoFrmBuf_1.virAddr[2],
                 (VOID *)stVideoFrmBuf_1.phyAddr[0], (VOID *)stVideoFrmBuf_1.phyAddr[1], (VOID *)stVideoFrmBuf_1.phyAddr[2],
                 stVideoFrmBuf_1.frameParam.dataFormat);


/*		SAL_VideoFrameBuf stVideoFrmBuf_1 = {0}; */
        memset(&stVideoFrmBuf_1, 0x00, sizeof(SAL_VideoFrameBuf));

        if (SAL_SOK != sys_hal_getVideoFrameInfo(&pstXsi->stSysFrame, &stVideoFrmBuf_1))
        {
            SVA_LOGE("get video frame info failed! \n");
            return SAL_FAIL;
        }

        SVA_LOGD("dst: w %d, h %d, vb %p, s [%d, %d, %d], vir [%p, %p, %p], phy [%p, %p, %p], fmt %d, \n",
                 stVideoFrmBuf_1.frameParam.width, stVideoFrmBuf_1.frameParam.height, (VOID *)stVideoFrmBuf_1.vbBlk,
                 stVideoFrmBuf_1.stride[0], stVideoFrmBuf_1.stride[1], stVideoFrmBuf_1.stride[2],
                 (VOID *)stVideoFrmBuf_1.virAddr[0], (VOID *)stVideoFrmBuf_1.virAddr[1], (VOID *)stVideoFrmBuf_1.virAddr[2],
                 (VOID *)stVideoFrmBuf_1.phyAddr[0], (VOID *)stVideoFrmBuf_1.phyAddr[1], (VOID *)stVideoFrmBuf_1.phyAddr[2],
                 stVideoFrmBuf_1.frameParam.dataFormat);

        SVA_LOGD("s %d, h %d \n", stVideoFrmBuf.stride[0], stVideoFrmBuf.frameParam.height);

        s32Ret = Ia_TdeQuickCopy(pstSrcFrame,
                                 &pstXsi->stSysFrame,
                                 0, 0,
                                 stVideoFrmBuf.stride[0], stVideoFrmBuf.frameParam.height,
                                 SAL_TRUE);
    }
    else
    {
        SVA_LOGE("Invalid video input type[%d] \n", pstPlatCap->enInputType);
        return SAL_FAIL;
    }

    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Quick Copy Failed! chan %d, ret: 0x%x \n", chan, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalFindFreeBuf
* 描  述  : 获取空闲缓存
* 输  入  : - chan : 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalFindFreeBuf(UINT32 chan, UINT32 *pFreeBufId)
{
    UINT32 i = 0;
    UINT32 uiStartIdx = 0;

    XSI_DEV *pstXsi = NULL;
    UINT32 *puiFreeBufIdx = NULL;
    XSI_DEV_BUFF *pstDevBuff = NULL;
    XSI_COMMON *pstXsiCommon = NULL;

    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pFreeBufId, SAL_FAIL);

    pstXsi = Sva_HalGetDev(chan);
    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    pstXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pstXsiCommon, SAL_FAIL);

    uiStartIdx = pstXsi->uiFrameNum;

    puiFreeBufIdx = pFreeBufId;

    SVA_LOGD("chan %d, uiStartIdx %d, \n", chan, uiStartIdx);
    for (i = uiStartIdx; i < uiStartIdx + SVA_INPUT_BUF_LEN; i++)
    {
        pstDevBuff = &pstXsi->stBuff[i % SVA_INPUT_BUF_LEN];

        SVA_LOGD("chan %d, bufId %d, use %d, %p \n", chan, i % SVA_INPUT_BUF_LEN, pstDevBuff->buffUse, pstDevBuff);

        if (SAL_TRUE == pstDevBuff->buffUse)
        {
            SVA_LOGD("chan %d, bufId %d, use %d \n", chan, i % SVA_INPUT_BUF_LEN, pstDevBuff->buffUse);
            continue;
        }

        break;
    }

    /* 队列满，通知引擎释放当前Buf */
    if (uiStartIdx + SVA_INPUT_BUF_LEN == i)
    {
        *puiFreeBufIdx = 0xffffffff;
        SVA_LOGD("Data Buf Que Failed! chan %d \n", chan);
        return SAL_FAIL;
    }

    /* 调试打印，查看缓存队列中Buf使用情况 */
    if (svaDbLevel == DEBUG_LEVEL4)
    {
        for (i = 0; i < SVA_INPUT_BUF_LEN; i++)
        {
            pstDevBuff = &pstXsi->stBuff[i];

            SVA_LOGW("sva chan %d buff %d status %d !\n", chan, i, pstDevBuff->buffUse);
        }
    }

    *puiFreeBufIdx = i % SVA_INPUT_BUF_LEN;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalCalculateSyncIndex
* 描  述  : 计算同步帧序号用于上层同步通道数据
* 输  入  : - chan : 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalCalculateSyncIndex(UINT32 chan, SVA_PROC_MODE_E mode)
{
    UINT32 i = 0;
    UINT32 uiRandNum = 0;
    SVA_PROC_MODE_E enProcMode = 0;

    XSI_COMMON *pstXsiCommon = NULL;

    /* Input Args Checker */
    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);

    if (mode >= SVA_PROC_MODE_NUM)
    {
        SVA_LOGE("Invalid mode %d >= Max %d, Pls Check! chan %d \n", mode, SVA_PROC_MODE_NUM, chan);
        return SAL_FAIL;
    }

    enProcMode = mode;
    pstXsiCommon = Sva_HalGetXsiCommon();

    if (SVA_PROC_MODE_DUAL_CORRELATION == enProcMode)
    {
        if (0 == chan)
        {
            uiRandNum = Sva_HalGetRandomId();

            pstXsiCommon->xsi_dev[0].uiSyncId = uiRandNum % 100000000;
            pstXsiCommon->xsi_dev[1].uiSyncId = pstXsiCommon->xsi_dev[0].uiSyncId;
        }
    }
    else
    {
        pstXsiCommon->xsi_dev[0].uiSyncId = 0;
        pstXsiCommon->xsi_dev[1].uiSyncId = 0;
    }

    /* Print Debug Info */
    for (i = 0; i < XSI_DEV_MAX; i++)
    {
        SVA_LOGD("i %d sync_num %d \n", i, pstXsiCommon->xsi_dev[i].uiSyncId);
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_HalFillFrmNum
 * @brief:      填充帧序号
 * @param[in]:  UINT32 chan
 * @param[in]:  SVA_PROC_MODE_E enProcMode
 * @param[in]:  SYSTEM_FRAME_INFO *pstSrcFrame
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_HalFillFrmNum(UINT32 chan, SVA_PROC_MODE_E enProcMode, SYSTEM_FRAME_INFO *pstSrcFrame)
{
    XSI_COMMON *pstXsiCommon = NULL;
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstSrcFrame, SAL_FAIL);

    pstXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pstXsiCommon, SAL_FAIL);

    if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSrcFrame, &stVideoFrmBuf))
    {
        SVA_LOGE("get video frame info failed! \n");
        return SAL_FAIL;
    }

    /* sva需要跨芯片传输需要填充帧序号 */
    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_SVA) && SAL_TRUE == IA_GetModTransFlag(IA_MOD_SVA)
        && SVA_PROC_MODE_IMAGE != enProcMode)
    {
        /* SVA_LOGE("============== fill error frame num! \n"); */
        if (SVA_PROC_MODE_DUAL_CORRELATION == enProcMode)
        {
            if (0 == chan)
            {
                /* 存放虚拟的帧号 */
                pstXsiCommon->xsi_dev[0].uiFrameNum = (UINT32)pstSrcFrame->uiDataLen;
                pstXsiCommon->xsi_dev[1].uiFrameNum = pstXsiCommon->xsi_dev[0].uiFrameNum;
                /* 存放真实的帧号 */
                pstXsiCommon->xsi_dev[0].stSysFrame.uiDataLen = (UINT32)stVideoFrmBuf.privateDate;
                pstXsiCommon->xsi_dev[1].stSysFrame.uiDataLen = (UINT32)stVideoFrmBuf.privateDate;
            }
        }
        else
        {
            /* 存放虚拟的帧号 */
            pstXsiCommon->xsi_dev[chan].uiFrameNum = (UINT32)pstSrcFrame->uiDataLen;
            /* 存放真实的帧号 */
            pstXsiCommon->xsi_dev[chan].stSysFrame.uiDataLen = (UINT32)stVideoFrmBuf.privateDate;

        }
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_HalFillBufInfo
 * @brief:      填充缓存参数
 * @param[in]:  UINT32 chan
 * @param[in]:  SVA_PROC_MODE_E enProcMode
 * @param[in]:  SYSTEM_FRAME_INFO *pstSrcFrame
 * @param[in]:  UINT32 uiInputIdx
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Sva_HalFillBufInfo(UINT32 chan, SVA_PROC_MODE_E enProcMode, SYSTEM_FRAME_INFO *pstSrcFrame, UINT32 uiInputIdx)
{
    XSI_DEV *pstXsi = NULL;
    XSI_DEV_BUFF *pstDevBuff = NULL;
    XSI_COMMON *pstXsiCommon = NULL;
    DSPINITPARA *pstInit = NULL;
    CAPB_PRODUCT *pstPlatCap = NULL;

    static UINT32 uiSideViewFrmGap = 0;
    static UINT32 uiFrmGroupIdx = 0;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    pstInit = SystemPrm_getDspInitPara();

    /* Input Args Checker */
    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstSrcFrame, SAL_FAIL);

    if (0x00 == pstSrcFrame->uiAppData)
    {
        SVA_LOGE("Src Frame Err! chan %d \n", chan);
        return SAL_FAIL;
    }

    pstXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pstXsiCommon, SAL_FAIL);

    pstXsi = Sva_HalGetDev(chan);
    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    pstDevBuff = &pstXsi->stBuff[uiInputIdx];

    if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSrcFrame, &stVideoFrmBuf))
    {
        SVA_LOGE("get video frame info failed! \n");
        return SAL_FAIL;
    }

    pstDevBuff->stUsrPrm.timeRef = stVideoFrmBuf.frameNum;
    pstDevBuff->stUsrPrm.pts = stVideoFrmBuf.pts;
    pstDevBuff->stUsrPrm.syncIdx = pstXsi->uiSyncId;

    if (SINGLE_CHIP_TYPE != pstInit->deviceType)
    {
        if (SVA_PROC_MODE_DUAL_CORRELATION == enProcMode)
        {
            if (0 != chan)
            {
                /* 侧视角帧序号需要手动增加 */
                if (1 != pstXsiCommon->xsi_dev[1].uiFrameNum % Sva_HalGetPdProcGap())
                {
                    pstDevBuff->stUsrPrm.timeRef = uiFrmGroupIdx + uiSideViewFrmGap * 2;
                }
                else
                {
                    uiSideViewFrmGap = 0;
                    uiFrmGroupIdx = pstDevBuff->stUsrPrm.timeRef;
                }

                uiSideViewFrmGap++;
            }
        }
    }

    pstPlatCap = capb_get_product();
    SVA_HAL_CHECK_PRM(pstPlatCap, SAL_FAIL);

    if (VIDEO_INPUT_INSIDE == pstPlatCap->enInputType)
    {
        /* pstDevBuff->stUsrPrm.timeRef = stVideoFrmBuf.pts; */
        pstDevBuff->stUsrPrm.syncPts = stVideoFrmBuf.privateDate;
        pstDevBuff->stUsrPrm.imgW = stVideoFrmBuf.frameParam.width;
        pstDevBuff->stUsrPrm.imgH = stVideoFrmBuf.frameParam.height;
        pstDevBuff->stUsrPrm.offsetX = (UINT64)pstSrcFrame->uiDataAddr; /* pstDevBuff->stUsrPrm.stAiPrm.packStartX; */
        /*SVA_LOGE("Wxl_dbg:syncPts %llu,imgW %d,imgH %d,offsetX %llu \n", pstDevBuff->stUsrPrm.syncPts,pstDevBuff->stUsrPrm.imgW,pstDevBuff->stUsrPrm.imgH,pstDevBuff->stUsrPrm.offsetX);*/
    }
    else if (VIDEO_INPUT_OUTSIDE == pstPlatCap->enInputType)
    {
        pstDevBuff->stUsrPrm.imgW = SVA_MODULE_WIDTH;
        pstDevBuff->stUsrPrm.imgH = SVA_MODULE_HEIGHT;
    }
    else
    {
        SVA_LOGE("Invalid video input type[%d] \n", pstPlatCap->enInputType);
        return SAL_FAIL;
    }

    SVA_LOGD("chan %d, get frame num %d, timeref %d \n",
             chan, pstXsiCommon->xsi_dev[chan].uiFrameNum, pstDevBuff->stUsrPrm.timeRef);
    return SAL_SOK;
}

/**
 * @function:   Sva_HalFillBufInfo
 * @brief:      填充缓存参数
 * @param[in]:  UINT32 chan
 * @param[in]:  SVA_PROC_MODE_E enProcMode
 * @param[in]:  SYSTEM_FRAME_INFO *pstSrcFrame
 * @param[in]:  UINT32 uiInputIdx
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Sva_HalImgModeFillBufInfo(UINT32 chan, SYSTEM_FRAME_INFO *pstSrcFrame, UINT32 uiInputIdx, UINT32 uiW, UINT32 uiH)
{
    XSI_DEV *pstXsi = NULL;
    XSI_DEV_BUFF *pstDevBuff = NULL;
    XSI_COMMON *pstXsiCommon = NULL;
    CAPB_PRODUCT *pstPlatCap = NULL;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    /* Input Args Checker */
    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstSrcFrame, SAL_FAIL);

    if (0x00 == pstSrcFrame->uiAppData)
    {
        SVA_LOGE("Src Frame Err! chan %d \n", chan);
        return SAL_FAIL;
    }

    pstXsiCommon = Sva_HalGetXsiCommon();
    SVA_HAL_CHECK_PRM(pstXsiCommon, SAL_FAIL);

    pstXsi = Sva_HalGetDev(chan);
    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);

    pstDevBuff = &pstXsi->stBuff[uiInputIdx];

    if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSrcFrame, &stVideoFrmBuf))
    {
        SVA_LOGE("get video frame info failed! \n");
        return SAL_FAIL;
    }

    pstDevBuff->stUsrPrm.timeRef = stVideoFrmBuf.frameNum;
    pstDevBuff->stUsrPrm.pts = stVideoFrmBuf.pts;
    pstDevBuff->stUsrPrm.syncIdx = pstXsi->uiSyncId;

    pstPlatCap = capb_get_product();
    SVA_HAL_CHECK_PRM(pstPlatCap, SAL_FAIL);

    if (VIDEO_INPUT_INSIDE == pstPlatCap->enInputType)
    {
        /* pstDevBuff->stUsrPrm.timeRef = stVideoFrmBuf.pts; */
        pstDevBuff->stUsrPrm.syncPts = stVideoFrmBuf.privateDate;
        pstDevBuff->stUsrPrm.imgW = stVideoFrmBuf.frameParam.width;
        pstDevBuff->stUsrPrm.imgH = stVideoFrmBuf.frameParam.height;
        pstDevBuff->stUsrPrm.offsetX = (UINT64)pstSrcFrame->uiDataAddr; /* pstDevBuff->stUsrPrm.stAiPrm.packStartX; */
        /* SVA_LOGE("Wxl_dbg:syncPts %llu,imgW %d,imgH %d,offsetX %llu \n", pstDevBuff->stUsrPrm.syncPts,pstDevBuff->stUsrPrm.imgW,pstDevBuff->stUsrPrm.imgH,pstDevBuff->stUsrPrm.offsetX); */
    }
    else if (VIDEO_INPUT_OUTSIDE == pstPlatCap->enInputType)
    {
        pstDevBuff->stUsrPrm.imgW = uiW;
        pstDevBuff->stUsrPrm.imgH = uiH;
    }
    else
    {
        SVA_LOGE("Invalid video input type[%d] \n", pstPlatCap->enInputType);
        return SAL_FAIL;
    }

    SVA_LOGW("chan %d, get frame num %d, timeref %d, imgw %d, imgh %d \n",
             chan, pstXsiCommon->xsi_dev[chan].uiFrameNum, pstDevBuff->stUsrPrm.timeRef, pstDevBuff->stUsrPrm.imgW, pstDevBuff->stUsrPrm.imgH);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalGetAlgDbgLevel
* 描  述  : 获取算法调试等级
* 输  入  : 无
* 输  出  : 无
* 返回值  : 成功: SAL_SOK
            失败: SAL_FAIL
*******************************************************************************/
static UINT32 Sva_HalGetAlgDbgLevel(void)
{
    return uiSvaAlgDbgLevel;
}

#if 1  /* 用于dump yuv */

static BOOL bDumpYuvFlag = SAL_FALSE;                   /* dump使能开关 */
static UINT32 uiDumpYuvOffset = 0;                      /* 当前dump内存的offset */
static CHAR *pDumpYuvVirMem = NULL;                     /* 用于dump yuv数据的虚拟地址 */
static UINT64 u64DumpYuvPhyMem = 0;                     /* 用于dump yuv数据的物理地址 */

/**
 * @function    Sva_HalSetDumpYuvFlag
 * @brief       设置yuv的dump开关
 * @param[in]
 * @param[out]
 * @return
 */
VOID Sva_HalSetDumpYuvFlag(BOOL bFlag)
{
    if (bDumpYuvFlag)
    {
        SVA_LOGE("other thread is dumping data! pls try later! \n");
        return;
    }

    bDumpYuvFlag = !!bFlag;
    SVA_LOGI("set recv yuv dump flag %d end! \n", bDumpYuvFlag);
}

/**
 * @function    Sva_DrvDumpRecvYuvData
 * @brief       dump从片接收到的yuv数据(默认仅主视角)
 * @param[in]
 * @param[out]
 * @return
 */
VOID Sva_HalDumpRecvYuvData(VOID *pVir, UINT32 uiSize, CHAR *pDumpPath)
{
    if (!bDumpYuvFlag)
    {
        uiDumpYuvOffset = 0;

        if (pDumpYuvVirMem)
        {
            mem_hal_mmzFree(pDumpYuvVirMem, "sva", "dump_yuv");
            pDumpYuvVirMem = NULL;
        }

        return;
    }

    if (uiDumpYuvOffset >= DUMP_YUV_MEM_SIZE)
    {
        bDumpYuvFlag = SAL_FALSE;

        if (!pDumpYuvVirMem)
        {
            if (SAL_SOK != mem_hal_mmzAlloc(DUMP_YUV_MEM_SIZE, "sva", "dump_yuv", NULL, SAL_TRUE, &u64DumpYuvPhyMem, (VOID **)&pDumpYuvVirMem))
            {
                SVA_LOGE("mmz alloc failed! size %d \n", DUMP_YUV_MEM_SIZE);
            }

            return;
        }

        Sva_HalDebugDumpData(pDumpYuvVirMem, pDumpPath, uiSize, 0);
        SVA_LOGE("dump yuv mem full! offset %d \n", uiDumpYuvOffset);
        return;
    }

    if (!pDumpYuvVirMem)
    {
        if (SAL_SOK != mem_hal_mmzAlloc(DUMP_YUV_MEM_SIZE, "sva", "dump_yuv", NULL, SAL_TRUE, &u64DumpYuvPhyMem, (VOID **)&pDumpYuvVirMem))
        {
            SVA_LOGE("mmz alloc failed! size %d \n", DUMP_YUV_MEM_SIZE);
            return;
        }
    }

    memcpy((CHAR *)pDumpYuvVirMem + uiDumpYuvOffset, pVir, uiSize);
    uiDumpYuvOffset += uiSize;
}

#endif

/*******************************************************************************
* 函数名  : Sva_HalVcaeSyncPutData
* 描  述  : 给引擎推帧接口
* 输  入  : - pstSysFrameInfo: 帧信息
*         : - pstSvaIn : 算法使用的参数
*         : - puiChnSts: 智能分析通道状态
*         : - uiMainViewChn: 主视角通道号
*         : - enProcMode: 处理模式
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalVcaeSyncPutData(SYSTEM_FRAME_INFO *pstSysFrameInfo[XSI_DEV_MAX],
                             SVA_PROCESS_IN *pstSvaIn,
                             UINT32 *puiChnSts,
                             UINT32 *puiXsiSts,
                             UINT32 uiMainViewChn,
                             SVA_PROC_MODE_E enProcMode)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 uiChnStatus = 0;
    UINT32 uiXsiStatus = 0;
    UINT32 uiTmpChn = 0;
    UINT32 uiInputIdx[2] = {0};

    static UINT32 uiLastTime = 0;

    UINT32 time_0 = 0;
    UINT32 time_1 = 0;
    UINT32 time_2 = 0;
    UINT32 time_3 = 0;
    UINT32 uiThrshold = 0;     /* 算法耗时打印阈值，单位ms */

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    SVA_HAL_CHECK_PRM(pstSysFrameInfo, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstSvaIn, SAL_FAIL);
    SVA_HAL_CHECK_PRM(puiChnSts, SAL_FAIL);

    /* Print Debug Info */
    for (i = 0; i < 2; i++)
    {
        SVA_LOGD("chan %d, sts %d \n", i, puiChnSts[i]);
    }

    time_0 = SAL_getCurMs();

    if ((SVA_PROC_MODE_DUAL_CORRELATION == enProcMode) && (SAL_TRUE != puiChnSts[0] || SAL_TRUE != puiChnSts[1]))
    {
        SVA_LOGE("ProcMode %d, Chn Status Errrr, chn_0[%d], chn_1[%d] \n",
                 enProcMode, puiChnSts[0], puiChnSts[1]);
        return SAL_FAIL;
    }

    for (i = 0; i < XSI_DEV_MAX; i++)
    {
        uiChnStatus = puiChnSts[i];
        uiXsiStatus = puiXsiSts[i];
        if ((SAL_TRUE != uiChnStatus) || (SAL_TRUE != uiXsiStatus))
        {
            continue;
        }

        /* channel Number used in following interfaces, Main-View channel is chan 0 !!!*/
        if (SVA_PROC_MODE_DUAL_CORRELATION == enProcMode)
        {
            uiTmpChn = (1 == uiMainViewChn ? 1 - i : i);
        }
        else
        {
            uiTmpChn = i;
        }

        if (0x00 == pstSysFrameInfo[uiTmpChn]->uiAppData)
        {
            SVA_LOGE("chan %d is null \n", i);
            return SAL_FAIL;
        }

        /* Calculate Sync Index */
        if (SAL_SOK != Sva_HalCalculateSyncIndex(i, enProcMode))
        {
            SVA_LOGE("Cal Sync Index Failed! chan %d, mode %d \n", i, enProcMode);
            return SAL_FAIL;
        }

        if (SAL_SOK != Sva_HalFillFrmNum(i, enProcMode, pstSysFrameInfo[uiTmpChn]))
        {
            return SAL_FAIL;
        }

        /* dump yuv数据用于问题定位 */
        if ((SVA_PROC_MODE_DUAL_CORRELATION == enProcMode && 0 == i)
            || (SVA_PROC_MODE_VIDEO == enProcMode))
        {
            if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSysFrameInfo[uiTmpChn], &stVideoFrmBuf))
            {
                SVA_LOGE("get video frame failed! \n");
            }
            else
            {
                Sva_HalDumpRecvYuvData((VOID *)stVideoFrmBuf.virAddr[0],
                                       SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT,
                                       DUMP_ALG_YUV_PATH);
            }
        }

        /*由于引擎输入需要使用到帧缓存数据的buffer，为方便理解，所以两者统一使用引擎输入的空闲Input索引项*/
        if (SAL_SOK != Sva_HalFindFreeInputIndex(i, enProcMode, &uiInputIdx[0]))
        {
            SVA_LOGE("Input Index Full! \n");
            return SAL_FAIL;
        }

        if (SAL_SOK != Sva_HalFillBufInfo(i, enProcMode, pstSysFrameInfo[uiTmpChn], uiInputIdx[i]))
        {
            SVA_LOGE("%s failed! \n", __func__);
            return SAL_FAIL;
        }

        if ((SAL_TRUE == IA_GetModEnableFlag(IA_MOD_SVA) && SAL_TRUE == IA_GetModTransFlag(IA_MOD_SVA))
            && SVA_PROC_MODE_DUAL_CORRELATION == enProcMode
            && 1 == i
            && 1 != gXsiCommon.xsi_dev[0].uiFrameNum % Sva_HalGetPdProcGap())
        {
            continue;
        }

        time_1 = SAL_getCurMs();

        if (SAL_SOK != Sva_HalQuickCopyData(i, enProcMode, pstSysFrameInfo[uiTmpChn], &uiInputIdx[0]))
        {
            SVA_LOGE("Quick Copy Failed! chan %d mode %d \n", i, enProcMode);
            goto err;
        }

        time_2 = SAL_getCurMs();
        if (uiSvaAlgDbgFlag && ((time_2 - time_1) > 14))
        {
            SZL_DBG_LOG("Quick Copy, chan %d time spent %d \n", i, (time_2 - time_1));
        }
    }

    time_1 = SAL_getCurMs();

    if (SAL_SOK != Sva_HalUpdateInputParam(enProcMode, uiMainViewChn, puiChnSts, &uiInputIdx[0], pstSvaIn))
    {
        SVA_LOGE("Update Input Param Failed! \n");
        goto err;
    }

    time_2 = SAL_getCurMs();

    gXsiCommon.uiIptEngStuckFlag = 1;     /* 引擎送帧接口卡住标记，调试使用用于定位接口是否卡住 */

    gXsiCommon.uiInputEngDataNum++;

    s32Ret = Sva_HalInputDataToEngine(puiChnSts, &uiInputIdx[0], enProcMode);
    if (SAL_SOK != s32Ret)
    {
        gXsiCommon.uiInputEngDataFailNum++;
    }
    else
    {
        gXsiCommon.uiInputEngDataSuccNum++;
    }

    gXsiCommon.uiIptEngStuckFlag = 0;     /* 引擎送帧接口卡住标记 */

    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Input Data Failed! \n");
        goto err;
    }

    gXsiCommon.uiInputEngCnt++;

    time_3 = SAL_getCurMs();

    uiThrshold = Sva_HalGetAlgDbgLevel();

    if (uiSvaAlgDbgFlag && (0 != uiLastTime) && (time_3 - uiLastTime > uiThrshold))
    {
        SZL_DBG_LOG("gap time %d, %d, %d, %d, total: %d. \n",
                    time_3 - uiLastTime, time_1 - time_0, time_2 - time_1, time_3 - time_2, time_3 - time_0);
    }

    uiLastTime = time_3;

    return SAL_SOK;
err:
    for (i = 0; i < XSI_DEV_MAX; i++)
    {
        uiChnStatus = puiChnSts[i];
        uiXsiStatus = puiXsiSts[i];

        if ((SAL_TRUE != uiChnStatus) || (SAL_TRUE != uiXsiStatus))
        {
            continue;
        }

        Sva_HalFreeInputIndex(i, enProcMode, &uiInputIdx[0]);
    }

    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Sva_HalVcaeSyncImgmodePutData
* 描    述    : 给引擎推帧接口
* 输    入    : - pstSysFrameInfo: 帧信息
*             : - pstSvaIn : 算法使用的参数
*             : - puiChnSts: 智能分析通道状态
*             : - uiMainViewChn: 主视角通道号
*             : - enProcMode: 处理模式
* 输    出    : 无
* 返回值  : SAL_SOK  : 成功
*               SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalVcaeSyncImgmodePutData(SYSTEM_FRAME_INFO *pstSysFrameInfo,
                                    SVA_PROCESS_IN *pstSvaIn,
                                    UINT32 *puiChnSts,
                                    UINT32 uiMainViewChn,
                                    SVA_PROC_MODE_E enProcMode,
                                    UINT32 uiW, UINT32 uiH)
{
    UINT32 i = 0;
    UINT32 uiChnStatus = 0;
    UINT32 uiTmpChn = 0;
    UINT32 uiInputIdx[2] = {0};  /*单帧bmp图片不给uiInputIdx赋值，即uiInputIdx一直为0*/

    static UINT32 uiLastTime = 0;

    UINT32 time_0 = 0;
    UINT32 time_1 = 0;
    UINT32 time_2 = 0;
    UINT32 time_3 = 0;
    UINT32 uiThrshold = 0;   /* 算法耗时打印阈值，单位ms */

    XSI_DEV *pstXsi = NULL;

    SVA_HAL_CHECK_PRM(pstSysFrameInfo, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstSvaIn, SAL_FAIL);
    SVA_HAL_CHECK_PRM(puiChnSts, SAL_FAIL);

    SAL_LOGD("Sync Put Data Entering! mode %d \n", enProcMode);
    for (i = 0; i < XSI_DEV_MAX; i++)
    {
        SAL_LOGD("chan %d, sts %d \n", i, puiChnSts[i]);
    }

    time_0 = SAL_getCurMs();

    if (((SVA_PROC_MODE_DUAL_CORRELATION == enProcMode)
         && (SAL_TRUE != puiChnSts[0] || SAL_TRUE != puiChnSts[1])))
    {
        SAL_ERROR("ProcMode %d, Chn Status Errrr, chn_0[%d], chn_1[%d] \n",
                  enProcMode, puiChnSts[0], puiChnSts[1]);
        return SAL_FAIL;
    }

    for (i = 0; i < XSI_DEV_MAX; i++)
    {
        uiChnStatus = puiChnSts[i];
        pstXsi = Sva_HalGetDev(i);

        if ((SAL_TRUE != uiChnStatus) || (SAL_TRUE != pstXsi->xsi_status))/*安检机若使用bmp单帧图片送智能，此处需修改*/
        {
            SAL_LOGD("chan %d close! %d %d \n", i, uiChnStatus, pstXsi->xsi_status);
            continue;
        }

        uiTmpChn = 0;

        if (0x00 == pstSysFrameInfo[uiTmpChn].uiAppData)
        {
            SAL_ERROR("chan %d is null \n", i);
            return SAL_FAIL;
        }

        /* Calculate Sync Index */
        if (SAL_SOK != Sva_HalCalculateSyncIndex(i, enProcMode))
        {
            SAL_ERROR("Cal Sync Index Failed! chan %d, mode %d \n", i, enProcMode);
            return SAL_FAIL;
        }
        time_1 = SAL_getCurMs();
        if (SAL_SOK != Sva_HalImgModeFillBufInfo(i, &pstSysFrameInfo[uiTmpChn], uiInputIdx[i], uiW, uiH))
        {
            SVA_LOGE("%s failed! \n", __func__);
            return SAL_FAIL;
        }

        /* Copy VB Data into no-cache Memory */
        if (SAL_SOK != Sva_HalQuickCopyData(i, enProcMode, &pstSysFrameInfo[uiTmpChn], &uiInputIdx[0]))
        {
            SAL_ERROR("Quick Copy Failed! chan %d mode %d \n", i, enProcMode);
            return SAL_FAIL;
        }

        time_2 = SAL_getCurMs();
        if (uiSvaAlgDbgFlag && ((time_2 - time_1) > 14))
        {
            SAL_ERROR("Quick Copy, chan %d time spent %d \n", i, (time_2 - time_1));
        }
    }

    time_1 = SAL_getCurMs();

    /* Update Input Param */
    if (SAL_SOK != Sva_HalUpdateInputParam(enProcMode, uiMainViewChn, puiChnSts, &uiInputIdx[0], pstSvaIn))
    {
        SAL_ERROR("Update Input Param Failed! \n");
        return SAL_FAIL;
    }

    time_2 = SAL_getCurMs();

    /* Send Copy Data To Engine and Process with IA */
    if (SAL_SOK != Sva_HalInputDataToEngine(puiChnSts, &uiInputIdx[0], enProcMode))
    {
        SAL_ERROR("Input Data Failed! \n");
        return SAL_FAIL;
    }

    time_3 = SAL_getCurMs();

    uiThrshold = Sva_HalGetAlgDbgLevel();

    if (uiSvaAlgDbgFlag && (0 != uiLastTime) && (time_3 - uiLastTime > uiThrshold))
    {
        SAL_WARN("gap time %d, %d, %d, %d, total: %d. \n",
                 time_3 - uiLastTime, time_1 - time_0, time_2 - time_1, time_3 - time_2, time_3 - time_0);
    }

    uiLastTime = time_3;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalGetFramePTS
* 描  述  : 获取帧信息中的时间戳
* 输  入  : - pu64Pts: 时间戳
*         : - pstSystemFrameInfo: 帧数据
* 输  出  : - pu64Pts: 时间戳
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalGetFramePTS(UINT64 *pu64Pts, SYSTEM_FRAME_INFO *pstSysFrameInfo)
{
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    if (NULL == pstSysFrameInfo)
    {
        SVA_LOGE("is null!!!\n");
        return SAL_FAIL;
    }

    if (NULL == pu64Pts)
    {
        SVA_LOGE("is null!!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSysFrameInfo, &stVideoFrmBuf))
    {
        SVA_LOGE("get video frame info failed! \n");
        return SAL_FAIL;
    }

    *pu64Pts = stVideoFrmBuf.pts;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalGetFrame
* 描  述  : 获取帧数据
* 输  入  : - uiVpssGrp: vpss组ID
*         : - uiVpssChn: vpss通道号
*         : - pstSysFrameInfo: 帧信息
* 输  出  : - pstSysFrameInfo: 帧信息
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalGetFrame(DUP_ChanHandle *pDupChnHandle, SYSTEM_FRAME_INFO *pstSysFrameInfo)
{
    INT32 s32Ret = SAL_SOK;

    DUP_COPY_DATA_BUFF stDupCpyDataBuf = {0};

    if (NULL == pstSysFrameInfo || NULL == pDupChnHandle)
    {
        SVA_LOGE("ptr null!!! %p, %p \n", pstSysFrameInfo, pDupChnHandle);
        return SAL_FAIL;
    }

    if (0x00 == pstSysFrameInfo->uiAppData)
    {
        SVA_LOGE("is null\n");
        return SAL_FAIL;
    }

    stDupCpyDataBuf.pstDstSysFrame = pstSysFrameInfo;
    s32Ret = pDupChnHandle->dupOps.OpDupGetBlit((VOID *)pDupChnHandle, &stDupCpyDataBuf);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Get Out Chn Frame Failed, 0x%x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalPutFrame
* 描  述  : 获取帧数据
* 输  入  : - chan: 通道号
*         : - pstSysFrameInfo: 帧信息
* 输  出  : - pstSysFrameInfo: 帧信息
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalPutFrame(DUP_ChanHandle *pDupChnHandle, SYSTEM_FRAME_INFO *pstSysFrameInfo)
{
    INT32 s32Ret = 0;

    DUP_COPY_DATA_BUFF stDupCpyDataBuf = {0};

    if (NULL == pstSysFrameInfo || NULL == pDupChnHandle)
    {
        SVA_LOGE("ptr is null!!! %p, %p \n", pDupChnHandle, pstSysFrameInfo);
        return SAL_FAIL;
    }

    if (0x00 == pstSysFrameInfo->uiAppData)
    {
        SVA_LOGE("is null\n");
        return SAL_FAIL;
    }

    stDupCpyDataBuf.pstDstSysFrame = pstSysFrameInfo;
    s32Ret = pDupChnHandle->dupOps.OpDupPutBlit((VOID *)pDupChnHandle, &stDupCpyDataBuf);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Get Out Chn Frame Failed, 0x%x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

#if 0 /* 安检机使用给引擎推帧接口，暂不修改 */

/*******************************************************************************
* 函数名  : Sva_HalVcaePutData
* 描  述  : 送帧给引擎
* 输  入  : - chan     : 通道号
*         : - pFrame   : 帧信息
*         : - pstSvaIn : 算法需要使用的参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalVcaePutData(UINT32 chan, void *pFrame, SVA_PROCESS_IN *pstSvaIn)
{
    SVA_HAL_CHECK_PRM(pFrame, SAL_FAIL);

    HAL_BUFFTRANS_POS_INFO srcPos = {0};
    HAL_BUFFTRANS_POS_INFO dstPos = {0};
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    UINT32 uiIdx = 0;

    XSI_DEV_BUFF *pstDevBuff = NULL;

    SYSTEM_FRAME_INFO *pstFrame = NULL;

    VIDEO_FRAME_INFO_S *pstFrameInfo = NULL;

    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);

    XSI_DEV *pstXsi = Sva_HalGetDev(chan);

    SVA_HAL_CHECK_PRM(pstXsi, SAL_FAIL);
    pstFrame = (SYSTEM_FRAME_INFO *)pFrame;
    pstFrameInfo = (VIDEO_FRAME_INFO_S *)pstFrame->uiAppData;

    uiIdx = pstXsi->frameCnt;

    for (i = uiIdx; i < uiIdx + SVA_INPUT_BUF_LEN; i++)
    {
        pstDevBuff = &pstXsi->stBuff[i % SVA_INPUT_BUF_LEN];

        if (SAL_TRUE == pstDevBuff->buffUse)
        {
            continue;
        }

        /* pstDevBuff->syncIdx = uiSyncId[chan]; */

        /* HAL_bufQuickCopy(&stFrame, &pstDevBuff->stSysFrame, 0, 0, pstFrameInfo->stVFrame.u32Width, pstFrameInfo->stVFrame.u32Height); */
        srcPos.uiWidth = pstFrameInfo->stVFrame.u32Width;
        srcPos.uiHeight = pstFrameInfo->stVFrame.u32Height;
        dstPos.uiX = (UINT32)pstFrame->uiDataAddr;
        HAL_bufQuickCopyYuv(pstFrame, &pstDevBuff->stSysFrame, &srcPos, &dstPos);

        SAL_mutexLock(pstXsi->mStatusMutexHdl);

        if (SAL_TRUE == pstXsi->xsi_status)
        {
            s32Ret = Sva_HalVcaeInputData(chan, pstDevBuff, pstSvaIn, 1);
            if (SAL_SOK == s32Ret)
            {
                pstXsi->frameCnt++;
                pstDevBuff->buffUse = SAL_TRUE;
            }
            else
            {
                SAL_dbPrintf(svaDbLevel, DEBUG_LEVEL2, "%s|%d--sva input failed! chan %d FrmCnt %d \n",
                             __func__, __LINE__, chan, pstXsi->frameCnt);
            }
        }
        else
        {
            SAL_dbPrintf(svaDbLevel, DEBUG_LEVEL2, "%s|%d--sva chan %d i %d xsi_status is %d!\n",
                         __func__, __LINE__, chan, i, pstXsi->xsi_status);
        }

        SAL_mutexUnlock(pstXsi->mStatusMutexHdl);

        break;
    }

    /* 队列满，通知引擎释放当前Buf */
    if (uiIdx + SVA_INPUT_BUF_LEN == i)
    {
        pstDevBuff = &pstXsi->stBuff[uiIdx % SVA_INPUT_BUF_LEN];
        s32Ret = Sva_HalVcaeInputData(chan, pstDevBuff, pstSvaIn, 0);
        if (SAL_SOK != s32Ret)
        {
            SAL_dbPrintf(svaDbLevel, DEBUG_LEVEL2, "%s|%d--sva QueBuf is Full! chan %d FrmCnt %d\n",
                         __func__, __LINE__, chan, pstXsi->frameCnt);
        }
    }

    /* 调试打印，查看缓存队列中Buf使用情况 */
    for (i = 0; i < SVA_INPUT_BUF_LEN; i++)
    {
        pstDevBuff = &pstXsi->stBuff[i];

        SAL_dbPrintf(svaDbLevel, DEBUG_LEVEL4, "%s|%d--sva chan %d buff %d status %d !\n",
                     __func__, __LINE__, chan, i, pstDevBuff->buffUse);
    }

    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);

    return SAL_SOK;
}

#endif

/**
 * @function:   Sva_HalYuv2Bmp
 * @brief:      yuv转bmp
 * @param[in]:  SYSTEM_FRAME_INFO *pstSrcFrameInfo
 * @param[in]:  SYSTEM_FRAME_INFO *pstDstFrameInfo
 * @param[in]:  SVA_PROCESS_IN *pstIn
 * @param[in]:  SVA_PROCESS_OUT *pstOut
 * @param[in]:  UINT32 bSampleCollect
 * @param[in]:  INT8 *pcBmp
 * @param[in]:  UINT32 *pBmpSize
 * @param[in]:  UINT32 uiWidth
 * @param[in]:  UINT32 uiHeight
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_HalYuv2Bmp(SYSTEM_FRAME_INFO *pstSrcFrameInfo,
                     SYSTEM_FRAME_INFO *pstDstFrameInfo,
                     SVA_PROCESS_IN *pstIn,
                     SVA_PROCESS_OUT *pstOut,
                     UINT32 bSampleCollect,
                     INT8 *pcBmp,
                     UINT32 *pBmpSize,
                     UINT32 uiWidth,
                     UINT32 uiHeight,
                     UINT32 bCached)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiX = 0;
    UINT32 uiY = 0;
    UINT32 uiW = 0;
    UINT32 uiH = 0;
    UINT32 uiTmp = 0;

    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 time3 = 0;

    BMP_RESULT_ST stBmpRslt = {0};
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    SVA_HAL_CHECK_PRM(pstSrcFrameInfo, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstDstFrameInfo, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstOut, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pcBmp, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pBmpSize, SAL_FAIL);

    uiX = 0;
    uiY = 0;
    uiW = SVA_MODULE_WIDTH;
    uiH = SVA_MODULE_HEIGHT;

    /* 若开启素材采集，则对包裹进行裁剪并转换成bmp */
    if (bSampleCollect)
    {
        uiX = SAL_align((UINT32)(pstOut->packbagAlert.package_loc.x * (float)uiWidth), 2);
        uiY = SAL_align((UINT32)(pstOut->packbagAlert.package_loc.y * (float)uiHeight), 2);
        uiW = SAL_align((UINT32)(pstOut->packbagAlert.package_loc.width * (float)uiWidth), 2);
        uiH = SAL_align((UINT32)(pstOut->packbagAlert.package_loc.height * (float)uiHeight), 2);

        uiTmp = uiX;

        /* 算法要求包裹区域左右外扩50个像素 */
        uiX = uiTmp > 50 + 8 ? uiTmp - 50 : 8;
        uiW = SAL_align(uiTmp + uiW + 50 < uiWidth - 8 ? uiTmp + uiW + 50 - uiX : uiWidth - 8 - uiX, 4);    /* bmp宽度需要4对齐 */

        uiW = uiX + uiW > uiWidth ? uiWidth - uiX : uiW;
        if (uiW < pstIn->uiPkgSplitFilter)
        {
            SVA_LOGW("pkg w[%d] < split thr[%d], return success! \n", uiW, pstIn->uiPkgSplitFilter);

            *pBmpSize = 0;
            return SAL_SOK;
        }
    }

    s32Ret = sys_hal_getVideoFrameInfo(pstDstFrameInfo, &stVideoFrmBuf);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("get video frame info failed! \n");
        return SAL_FAIL;
    }

    /* Clear Old Data */
    sal_memset_s((VOID *)stVideoFrmBuf.virAddr[0],    \
                 SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2,            \
                 0x80,                                             \
                 SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2);

    time0 = SAL_getCurMs();
#if 0
    /* IVE DMA QUICK COPY */
    srcRect.s32Xpos = uiX;
    srcRect.s32Ypos = uiY;
    srcRect.u32Width = uiW;
    srcRect.u32Height = uiH;

    DstRect.s32Xpos = 0;
    DstRect.s32Ypos = 0;
    DstRect.u32Width = uiW;
    DstRect.u32Height = uiH;
    if (SAL_SOK != ive_hal_QuickCopy(pstSrcFrameInfo, pstDstFrameInfo, &srcRect, &DstRect, SAL_TRUE))
    {
        SVA_LOGE("Ive Quick Copy Failed! \n");
        return SAL_FAIL;
    }

#endif

    s32Ret = Ia_TdeQuickCopy(pstSrcFrameInfo, pstDstFrameInfo, uiX, uiY, uiW, uiH, bCached);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("Tde Copy fail! ret: 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    time1 = SAL_getCurMs();

    /* Save Target Image Region Info */
    pstDstFrameInfo->uiDataWidth = uiW;
    pstDstFrameInfo->uiDataHeight = uiH;

    time2 = SAL_getCurMs();

    /* YUV 2 RGB */
    stBmpRslt.cBmpAddr = (UINT8 *)pcBmp;
    s32Ret = vdec_tsk_SaveYuv2Bmp(pstDstFrameInfo, &stBmpRslt);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("YUV 2 RGB Failed! \n");
        return SAL_FAIL;
    }

    *pBmpSize = stBmpRslt.uiBmpSize;

    time3 = SAL_getCurMs();

    if (time1 - time0 > 100 || time3 - time2 > 100)
    {
        SVA_LOGI("ive copy cost %d ms, yuv2rgb cost %d ms. \n", time1 - time0, time3 - time2);
    }

    SVA_LOGD("ive copy yuv2rgb cost %d ms, size %d \n", time1 - time0, stBmpRslt.uiBmpSize);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_halDrawInit
* 描  述  : 初始化画框和OSD
* 输  入  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalDrawInit(UINT32 u32Dev, DRAW_MOD_E enLineMode, DRAW_MOD_E enOsdMode)
{
    INT32 s32Ret = SAL_SOK;
    OSD_VAR_BLOCK_S stBlock;

    if (DRAW_MOD_VGS == enOsdMode)
    {
        stBlock.u32StringLenMax = strlen("(  %)");
        stBlock.u32LatSizeMax = 16 * 3;     /* 按照大号字体来申请内存 */
        stBlock.u32FontSizeMax = 16 * 3;
        stBlock.u32LatNum = 0;
        stBlock.u32FontNum = 128;

        /* SVA */
        s32Ret = osd_func_getFreeVarBlock(&stBlock, &gau32SvaVarOsdChn[u32Dev]);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("sva[%u] get osd block fail\n", u32Dev);
            return SAL_FAIL;
        }

        gpastSvaOsdPrm[u32Dev] = SAL_memMalloc(sizeof(VGS_ADD_OSD_PRM), "SVA", "sva_vgs_osdprm");
        if (NULL == gpastSvaOsdPrm[u32Dev])
        {
            SVA_LOGE("sva[%u] malloc fail\n", u32Dev);
            return SAL_FAIL;
        }

        /* JPEG */
        s32Ret = osd_func_getFreeVarBlock(&stBlock, &gau32JpegVarOsdChn[u32Dev]);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("sva[%u] get osd block fail\n", u32Dev);
            return SAL_FAIL;
        }

        gpastJpegOsdPrm[u32Dev] = SAL_memMalloc(sizeof(VGS_ADD_OSD_PRM), "SVA", "sva_vgs_osdprm");
        if (NULL == gpastJpegOsdPrm[u32Dev])
        {
            SVA_LOGE("sva[%u] malloc fail\n", u32Dev);
            SAL_memfree(gpastSvaOsdPrm[u32Dev], "SVA", "sva_vgs_osdprm");
            return SAL_FAIL;
        }
    }

    if (DRAW_MOD_CPU == enLineMode)
    {
        s32Ret = vgs_func_drawLineSoftInit(&gpastSvaLinePrm[u32Dev]);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("sva[%u] init line fail\n", u32Dev);
            return SAL_FAIL;
        }

        gpastJpegRectPrm[u32Dev] = SAL_memMalloc(sizeof(VGS_RECT_ARRAY_S), "SVA", "sva_vgs_array");
        if (NULL == gpastJpegRectPrm[u32Dev])
        {
            SVA_LOGE("malloc fail\n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvTransTarLoc
 * @brief:      目标坐标转换
 * @param[in]:  SVA_PROCESS_OUT *pstOut
 * @param[in]:  UINT32 *puiRealTarNum
 * @param[in]:  SVA_TARGET *pstRealTarArr
 * @param[out]: None
 * @return:     static VOID
 */
VOID Sva_HalTransTarLoc(SVA_PROCESS_OUT *pstOut, UINT32 *puiRealTarNum, SVA_TARGET *pstRealTarArr)
{
    UINT32 j = 0;
    UINT32 uiRealTarNum = 0;
    UINT32 uiGapPixel = 0;

    float fPkgLeftX = 0.0;
    float fPkgRightX = 0.0;
    float fPkgUpY = 0.0;
    float fPkgDownY = 0.0;

    float fPkgRealUpY = 0.0;
    float fPkgRealDownY = 0.0;

    float fTarX = 0.0;
    float fTarY = 0.0;
    float fTarMidX = 0.0;
    float fTarMidY = 0.0;

    SVA_RECT_F *pstPkgLoc = NULL;
    SVA_TARGET *pstTarInfo = NULL;
    SVA_TARGET *pstRealTarInfo = NULL;
    SVA_RECT_F stRealPkgLoc = {0};

    if (NULL == pstOut || NULL == puiRealTarNum || NULL == pstRealTarArr)
    {
        SVA_LOGE("ptr null! %p, %p, %p \n", pstOut, puiRealTarNum, pstRealTarArr);
        return;
    }

    pstPkgLoc = &pstOut->packbagAlert.package_loc;

    stRealPkgLoc.y = pstOut->packbagAlert.fRealPkgY;
    stRealPkgLoc.height = pstOut->packbagAlert.fRealPkgH;

    for (j = 0; j < pstOut->target_num; j++)
    {
        pstRealTarInfo = pstRealTarArr + uiRealTarNum;
        pstTarInfo = &pstOut->target[j];

        fTarMidX = pstTarInfo->rect.x + pstTarInfo->rect.width / 2.0;
        fTarMidY = pstTarInfo->rect.y + pstTarInfo->rect.height / 2.0;

        fPkgLeftX = pstPkgLoc->x - (float)uiGapPixel / (float)SVA_MODULE_WIDTH;
        fPkgRightX = pstPkgLoc->x + pstPkgLoc->width + (float)uiGapPixel / (float)SVA_MODULE_WIDTH;
        fPkgUpY = pstPkgLoc->y;   /* pstPkgLoc->y - (float)uiGapPixel / (float)SVA_MODULE_HEIGHT; */
        fPkgDownY = pstPkgLoc->y + pstPkgLoc->height; /* pstPkgLoc->y + pstPkgLoc->height + (float)uiGapPixel / (float)SVA_MODULE_HEIGHT; */

        fPkgLeftX = fPkgLeftX <= 0.0 ? 0.001 : fPkgLeftX;
        fPkgRightX = fPkgRightX >= 1.0 ? 0.999 : fPkgRightX;
        fPkgUpY = fPkgUpY <= 0.0 ? 0.001 : fPkgUpY;
        fPkgDownY = fPkgDownY >= 1.0 ? 0.999 : fPkgDownY;

        fPkgRealUpY = stRealPkgLoc.y;   /* pstPkgLoc->y - (float)uiGapPixel / (float)SVA_MODULE_HEIGHT; */
        fPkgRealDownY = stRealPkgLoc.y + stRealPkgLoc.height; /* pstPkgLoc->y + pstPkgLoc->height + (float)uiGapPixel / (float)SVA_MODULE_HEIGHT; */

        fPkgRealUpY = fPkgRealUpY <= 0.0 ? 0.001 : fPkgRealUpY;
        fPkgRealDownY = fPkgRealDownY >= 1.0 ? 0.999 : fPkgRealDownY;

        if (fTarMidX < fPkgLeftX || fTarMidX > fPkgRightX
            || fTarMidY < fPkgRealUpY || fTarMidY > fPkgRealDownY)
        {
            SVA_LOGD("target:j %d, fPkgLeftX %f, fPkgRightX %f, fPkgUpY %f, fPkgDownY %f, fTarMidX %f, fTarMidY %f \n", j,
                     fPkgLeftX, fPkgRightX, fPkgRealUpY, fPkgRealDownY, fTarMidX, fTarMidY);
            SVA_LOGD("target111:j %d, fPkgLeftX %f, fPkgRightX %f, fPkgUpY %f, fPkgDownY %f  \n", j,
                     pstTarInfo->rect.x,
                     pstTarInfo->rect.y,
                     pstTarInfo->rect.width,
                     pstTarInfo->rect.height);
            continue;
        }

        memcpy(pstRealTarInfo, pstTarInfo, sizeof(SVA_TARGET));

        fTarX = pstTarInfo->rect.x <= fPkgLeftX ? fPkgLeftX + 0.001 : pstTarInfo->rect.x;
        fTarY = pstTarInfo->rect.y <= fPkgUpY ? fPkgUpY + 0.001 : pstTarInfo->rect.y;

#if 1
        /* 更新目标在包裹内的相对坐标 */
        pstRealTarInfo->rect.x = (pstTarInfo->rect.x - fPkgLeftX) / (fPkgRightX - fPkgLeftX);
        pstRealTarInfo->rect.y = (pstTarInfo->rect.y - fPkgUpY) / (fPkgDownY - fPkgUpY);
        pstRealTarInfo->rect.width = (pstTarInfo->rect.x + pstTarInfo->rect.width >= fPkgRightX) \
                                     ? (fPkgRightX - fTarX) / (fPkgRightX - fPkgLeftX) \
                                     : (pstTarInfo->rect.x + pstTarInfo->rect.width - fTarX) / (fPkgRightX - fPkgLeftX);
        pstRealTarInfo->rect.height = (pstTarInfo->rect.y + pstTarInfo->rect.height >= fPkgDownY) \
                                      ? (fPkgDownY - fTarY) / (fPkgDownY - fPkgUpY) \
                                      : (pstTarInfo->rect.y + pstTarInfo->rect.height - fTarY) / (fPkgDownY - fPkgUpY);

        pstRealTarInfo->rect.x = pstRealTarInfo->rect.x <= 0.0 ? 0.001 : pstRealTarInfo->rect.x;
        pstRealTarInfo->rect.y = pstRealTarInfo->rect.y <= 0.0 ? 0.001 : pstRealTarInfo->rect.y;
        pstRealTarInfo->rect.width = pstRealTarInfo->rect.width + pstRealTarInfo->rect.x >= 1.0 \
                                     ? 0.999 - pstRealTarInfo->rect.x \
                                     : pstRealTarInfo->rect.width;
        pstRealTarInfo->rect.height = pstRealTarInfo->rect.height + pstRealTarInfo->rect.y >= 1.0 \
                                      ? 0.999 - pstRealTarInfo->rect.y \
                                      : pstRealTarInfo->rect.height;
#endif

        uiRealTarNum++;
    }

    *puiRealTarNum = uiRealTarNum;
    return;
}

/**
 * @function:   Sva_HalTransTarLocJpg
 * @brief:      基于1280x1024的图片进行目标坐标转换
 * @param[in]:  SVA_PROCESS_OUT *pstOut
 * @param[in]:  UINT32 *puiRealTarNum
 * @param[in]:  SVA_TARGET *pstRealTarArr
 * @param[out]: None
 * @return:     static VOID
 */
VOID Sva_HalTransTarLocJpg(SVA_PROCESS_OUT *pstOut, UINT32 *puiRealTarNum, SVA_TARGET *pstRealTarArr)
{
    UINT32 j = 0;
    UINT32 uiRealTarNum = 0;
    UINT32 uiGapPixel = 0;

    float fPkgLeftX = 0.0;
    float fPkgRightX = 0.0;
    float fPkgUpY = 0.0;
    float fPkgDownY = 0.0;

    /* float fTarX = 0.0; */
    /* float fTarY = 0.0; */
    float fTarMidX = 0.0;
    float fTarMidY = 0.0;

    SVA_RECT_F *pstPkgLoc = NULL;
    SVA_TARGET *pstTarInfo = NULL;
    SVA_TARGET *pstRealTarInfo = NULL;

    if (NULL == pstOut || NULL == puiRealTarNum || NULL == pstRealTarArr)
    {
        SVA_LOGE("ptr null! %p, %p, %p \n", pstOut, puiRealTarNum, pstRealTarArr);
        return;
    }

    pstPkgLoc = &pstOut->packbagAlert.package_loc;
    SVA_LOGD("ywn: pkgloc xy[%f, %f]\n", pstPkgLoc->x, pstPkgLoc->y);

    for (j = 0; j < pstOut->target_num; j++)
    {
        pstRealTarInfo = pstRealTarArr + uiRealTarNum;
        pstTarInfo = &pstOut->target[j];

        fTarMidX = pstTarInfo->rect.x + pstTarInfo->rect.width / 2.0;
        fTarMidY = pstTarInfo->rect.y + pstTarInfo->rect.height / 2.0;

        fPkgLeftX = pstPkgLoc->x - (float)uiGapPixel / (float)SVA_MODULE_WIDTH;
        fPkgRightX = pstPkgLoc->x + pstPkgLoc->width + (float)uiGapPixel / (float)SVA_MODULE_WIDTH;
        fPkgUpY = pstPkgLoc->y;   /* pstPkgLoc->y - (float)uiGapPixel / (float)SVA_MODULE_HEIGHT; */
        fPkgDownY = pstPkgLoc->y + pstPkgLoc->height; /* pstPkgLoc->y + pstPkgLoc->height + (float)uiGapPixel / (float)SVA_MODULE_HEIGHT; */

        fPkgLeftX = fPkgLeftX <= 0.0 ? 0.001 : fPkgLeftX;
        fPkgRightX = fPkgRightX >= 1.0 ? 0.999 : fPkgRightX;
        fPkgUpY = fPkgUpY <= 0.0 ? 0.001 : fPkgUpY;
        fPkgDownY = fPkgDownY >= 1.0 ? 0.999 : fPkgDownY;

        if (fTarMidX < fPkgLeftX || fTarMidX > fPkgRightX
            || fTarMidY < fPkgUpY || fTarMidY > fPkgDownY)
        {
            continue;
        }

        memcpy(pstRealTarInfo, pstTarInfo, sizeof(SVA_TARGET));

        /* fTarX = pstTarInfo->rect.x <= fPkgLeftX ? fPkgLeftX + 0.001 : pstTarInfo->rect.x; */
        /* fTarY = pstTarInfo->rect.y <= fPkgUpY ? fPkgUpY + 0.001 : pstTarInfo->rect.y; */

#if 0
        /* 更新目标在包裹内的相对坐标 */
        pstRealTarInfo->rect.x = (pstTarInfo->rect.x - fPkgLeftX) / (fPkgRightX - fPkgLeftX);
        pstRealTarInfo->rect.y = (pstTarInfo->rect.y - fPkgUpY) / (fPkgDownY - fPkgUpY);
        pstRealTarInfo->rect.width = (pstTarInfo->rect.x + pstTarInfo->rect.width >= fPkgRightX) \
                                     ? (fPkgRightX - fTarX) / (fPkgRightX - fPkgLeftX) \
                                     : (pstTarInfo->rect.x + pstTarInfo->rect.width - fTarX) / (fPkgRightX - fPkgLeftX);
        pstRealTarInfo->rect.height = (pstTarInfo->rect.y + pstTarInfo->rect.height >= fPkgDownY) \
                                      ? (fPkgDownY - fTarY) / (fPkgDownY - fPkgUpY) \
                                      : (pstTarInfo->rect.y + pstTarInfo->rect.height - fTarY) / (fPkgDownY - fPkgUpY);

        pstRealTarInfo->rect.x = pstRealTarInfo->rect.x <= 0.0 ? 0.001 : pstRealTarInfo->rect.x;
        pstRealTarInfo->rect.y = pstRealTarInfo->rect.y <= 0.0 ? 0.001 : pstRealTarInfo->rect.y;
        pstRealTarInfo->rect.width = pstRealTarInfo->rect.width + pstRealTarInfo->rect.x >= 1.0 \
                                     ? 0.999 - pstRealTarInfo->rect.x \
                                     : pstRealTarInfo->rect.width;
        pstRealTarInfo->rect.height = pstRealTarInfo->rect.height + pstRealTarInfo->rect.y >= 1.0 \
                                      ? 0.999 - pstRealTarInfo->rect.y \
                                      : pstRealTarInfo->rect.height;
#endif
        pstRealTarInfo->rect.x = pstTarInfo->rect.x - pstPkgLoc->x;
        pstRealTarInfo->rect.y = pstTarInfo->rect.y - pstPkgLoc->y;
        pstRealTarInfo->rect.width = pstTarInfo->rect.width;
        pstRealTarInfo->rect.height = pstTarInfo->rect.height;

        uiRealTarNum++;
    }

    *puiRealTarNum = uiRealTarNum;
    return;
}

/*******************************************************************************
* 函数名  : disp_osdPstOutSort
* 描  述  : 非叠框模式智能信息排序
* 输  入  : - pstOut:
*         : - pSvaRect:
*         : - uiPicW:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 Sva_HalOsdPstOutSort(SVA_PROCESS_IN *pstIn, SVA_PROCESS_OUT *pstOut, DISP_SVA_RECT_F *pSvaRect)
{
    INT32 i = 0;
    INT32 j = 0;
    INT32 alert_num = 0; /* 智能信息个数 */
    SVA_TARGET tmpTarget = {0};
    DISP_SVA_RECT_F tmpRect = {0};

    FLOAT32 f32TmpX1 = 0.0f;   /*第1个矩形框X坐标*/
    FLOAT32 f32TmpX2 = 0.0f;   /*第2个矩形框X坐标*/

    if (NULL == pstIn || NULL == pstOut || NULL == pSvaRect)
    {
        SVA_LOGE("pstOut NULL err\n");
        return SAL_FAIL;
    }

    alert_num = pstOut->target_num;

    for (i = 0; i < alert_num - 1; i++)
    {
        for (j = 0; j < alert_num - 1 - i; j++)
        {
            /*采用算法吐出的浮点值计算矩形框X坐标进行排序，防止精度丢失导致画框排序抖动*/
            f32TmpX1 = (FLOAT32)pSvaRect[j].x + pSvaRect[j].f32StartXFract;
            f32TmpX2 = (FLOAT32)pSvaRect[j + 1].x + pSvaRect[j + 1].f32StartXFract;

            /*按照目标顶点X坐标进行排序*/
            if (f32TmpX1 > f32TmpX2)
            {
                memcpy(&tmpTarget, &pstOut->target[j], sizeof(SVA_TARGET));
                memcpy(&pstOut->target[j], &pstOut->target[j + 1], sizeof(SVA_TARGET));
                memcpy(&pstOut->target[j + 1], &tmpTarget, sizeof(SVA_TARGET));

                memcpy(&tmpRect, &pSvaRect[j], sizeof(DISP_SVA_RECT_F));
                memcpy(&pSvaRect[j], &pSvaRect[j + 1], sizeof(DISP_SVA_RECT_F));
                memcpy(&pSvaRect[j + 1], &tmpRect, sizeof(DISP_SVA_RECT_F));
            }
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_osdPstOutFloattoINT
* 描  述  : 获取智能信息的检测区域
* 输  入  : - pDispChn:
*         : - pstFrameInfo:
*         : - pstOut        :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 Sva_HalPstOutFloattoINT(SVA_PROCESS_OUT *pstOut, UINT32 uiW, UINT32 uiH, UINT32 uiChan)
{
    UINT32 uiPicW = 0;
    UINT32 uiPicH = 0;
    INT32 alert_num = 0; /* 智能信息个数 */
    INT32 i = 0;
    DISP_SVA_RECT_F *pSvaRect = NULL;
    BOOL bUseFract = capb_get_line()->bUseFract;
    FLOAT32 f32StartX = 0.0;
    FLOAT32 f32EndX = 0.0;

    if (pstOut == NULL)
    {
        SVA_LOGE("pstOut NULL err\n");
        return SAL_FAIL;
    }

    uiPicW = uiW;
    uiPicH = uiH;
    alert_num = pstOut->target_num;

    pSvaRect = g_SvaRect[uiChan]; /* 智能信息浮点型转整形存储结构体 */

    for (i = 0; i < alert_num; i++)
    {
        f32StartX = pstOut->target[i].rect.x * (float)uiPicW;

        /*画框使用整型，会导致精度丢失*/
        pSvaRect[i].x = (UINT32)f32StartX;
        pSvaRect[i].y = pstOut->target[i].rect.y * uiPicH;
        f32EndX = (pstOut->target[i].rect.x + pstOut->target[i].rect.width) * (float)uiPicW;
        pSvaRect[i].width = (UINT32)(f32EndX - f32StartX);
        pSvaRect[i].height = pstOut->target[i].rect.height * uiPicH;
        pSvaRect[i].f32EndX = f32EndX;

        pSvaRect[i].y = SAL_align(pSvaRect[i].y, 2);
        pSvaRect[i].height = SAL_align(pSvaRect[i].height, 2);

        if (SAL_TRUE == bUseFract)
        {
            pSvaRect[i].f32StartXFract = f32StartX - (UINT32)f32StartX;
            pSvaRect[i].f32EndXFract = f32EndX - (UINT32)f32EndX;
        }
        else
        {
            pSvaRect[i].x = SAL_align(pSvaRect[i].x, 2);
            pSvaRect[i].width = SAL_align(pSvaRect[i].width, 2);
            pSvaRect[i].f32StartXFract = -1;
            pSvaRect[i].f32EndXFract = -1;
        }
    }

    return SAL_SOK;
}

/**
 * @function    Sva_HalJpegCalcTargetNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Sva_HalJpegCalcTargetNum(UINT32 chan, SVA_PROCESS_IN *pstIn,
                               SVA_PROCESS_OUT *pstOut, SVA_OSD_JPG_PRM_S *stSvaPicTarPrm)
{
    INT32 s32Ret = SAL_SOK;
    INT32 j = 0;
    INT32 k = 0;
    UINT32 uiPicW = 0;
    UINT32 uiPicH = 0;
    UINT32 uiConfidence = 0;
    UINT32 uiSubTypeIdx = 0;
    UINT32 reliability = 0;  /* 0~100 */
    UINT32 indx = 0;
    UINT32 u32LatSize = 18;
    UINT32 u32FontSize = 18;
    UINT32 u32Argb1555 = 255;
    UINT32 u32BgColor = 255;

    DISP_OSD_COORDINATE_CORRECT *pstOsd_corct = NULL;
    DISP_OSD_CORRECT_PRM_S stOsdCorrectPrm;
    DISP_SVA_RECT_F *pSvaRect = NULL;
    UINT32 uiTmpRealTarNum = 0;
    SVA_PROCESS_OUT stSvaOutTmp = {0};
    SVA_TARGET stSvaTarget[SVA_XSI_MAX_ALARM_NUM] = {0};
    DISP_OSD_COORDINATE_CORRECT osd_correct[SVA_MAX_PACKAGE_BUF_NUM] = {0};
    UINT32 u32NameLen = 0;
    UINT32 u32NumLen = 0;
    UINT32 uiUpOsdMaxY = 0;
    UINT32 uiDownOsdMaxY = 0;

    OSD_SET_PRM_S *pstOsdPrm = NULL;
    OSD_REGION_S *pstOsdRgn = NULL;
    OSD_REGION_S *pstOsdLatRgn = NULL;
    OSD_REGION_S *pstNumLatRgn = NULL;
    OSD_REGION_S *pstNumFontRgn = NULL;

    /* 违禁品坐标转换 */
    sal_memset_s(&stSvaOutTmp, sizeof(SVA_PROCESS_OUT), 0x00, sizeof(SVA_PROCESS_OUT));
    sal_memcpy_s(&stSvaOutTmp, sizeof(SVA_PROCESS_OUT), pstOut, sizeof(SVA_PROCESS_OUT));
    sal_memset_s(&stSvaTarget, SVA_XSI_MAX_ALARM_NUM * sizeof(SVA_TARGET), 0x00, SVA_XSI_MAX_ALARM_NUM * sizeof(SVA_TARGET));

    stSvaOutTmp.packbagAlert.package_loc.y = stSvaOutTmp.packbagAlert.fRealPkgY;
    stSvaOutTmp.packbagAlert.package_loc.height = stSvaOutTmp.packbagAlert.fRealPkgH;

    (VOID)Sva_HalTransTarLoc(&stSvaOutTmp, &uiTmpRealTarNum, &stSvaTarget[0]);

    k = 0;
    for (j = 0; j < uiTmpRealTarNum; j++)
    {
        if (SAL_TRUE == stSvaTarget[j].bAnotherViewTar || stSvaTarget[j].type >= SVA_XSI_MAX_ALARM_NUM)
        {
            continue;
        }

        sal_memcpy_s(&stSvaOutTmp.target[k], sizeof(SVA_TARGET), &stSvaTarget[j], sizeof(SVA_TARGET));
        k++;
    }

    stSvaOutTmp.target_num = k;

    uiPicW = (UINT32)(stSvaOutTmp.packbagAlert.package_loc.width * (float)SVA_MODULE_WIDTH);
    uiPicH = (UINT32)(stSvaOutTmp.packbagAlert.package_loc.height * (float)SVA_MODULE_HEIGHT);

    pSvaRect = g_SvaRect[chan];
    sal_memset_s(pSvaRect, SVA_XSI_MAX_ALARM_NUM * sizeof(DISP_SVA_RECT_F), 0x00, SVA_XSI_MAX_ALARM_NUM * sizeof(DISP_SVA_RECT_F));

    /* 将违禁品浮点型坐标转换成整型并赋值给全局结构体 */
    s32Ret = Sva_HalPstOutFloattoINT(&stSvaOutTmp, uiPicW, uiPicH, chan);

    /* 按照目标顶点X坐标对所有违禁品根据索引ID升序排列进行排序 */
    s32Ret = Sva_HalOsdPstOutSort(pstIn, &stSvaOutTmp, pSvaRect);
    if (s32Ret != SAL_SOK)
    {
        SVA_LOGE("chn %d pstOutSort failed\n", chan);
        return SAL_FAIL;
    }

    (VOID)disp_osdInitCocrtInfo(NULL, SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT, pstIn, &stSvaOutTmp, &osd_correct[0]);

    /* 初始化用VGS画OSD */
    pstNumFontRgn = osd_func_getVarFontRegionSet(gau32SvaVarOsdChn[chan], stSvaOutTmp.target_num);
    if (NULL == pstNumFontRgn)
    {
        SVA_LOGE("please alloc memory first[]\n");
        return SAL_FAIL;
    }

    /* 用户参数判断 */
    if (SVA_ALERT_EXT_PERCENT == pstIn->stTargetPrm.enOsdExtType)
    {
        uiConfidence = SAL_TRUE;
    }

    uiConfidence = SAL_TRUE;

    (VOID)osd_func_readStart();

    for (j = 0; j < stSvaOutTmp.target_num; j++)
    {
        if (j >= SVA_XSI_MAX_ALARM_NUM)
        {
            SVA_LOGE("chn %d\n", chan);
            break;
        }

        indx = stSvaOutTmp.target[j].type;                      /* 目标类型 */
        reliability = stSvaOutTmp.target[j].visual_confidence;  /* osd 置信度 */
        uiSubTypeIdx = stSvaOutTmp.target[j].u32SubTypeIdx;     /* 小类种类 */

        if (NULL == (pstOsdPrm = osd_func_getOsdPrm(OSD_BLOCK_IDX_STRING, indx)))
        {
            SVA_LOGW("get osd fail param, idx[%u]\n", indx);
            continue;
        }

        if (pSvaRect[j].width && pSvaRect[j].height)
        {
            /* 数据校验防止画出图 */
            if (pSvaRect[j].x >= uiPicW || pSvaRect[j].y >= uiPicH)
            {
                continue;
            }

            /* 违禁品名称OSD的点阵 */
            if (NULL == (pstOsdLatRgn = osd_func_getLatRegion(OSD_BLOCK_IDX_STRING, indx, u32LatSize)))
            {
                DISP_LOGW("get alert name osd lattice fail, idx[%u], lat[%u]\n", indx, u32LatSize);
                continue;
            }

            /* 违禁品名称的OSD */
            if (NULL == (pstOsdRgn = osd_func_getFontRegion(OSD_BLOCK_IDX_STRING, indx, u32LatSize, u32FontSize, SAL_FALSE)))
            {
                SVA_LOGW("get alert name osd fail, idx[%u], lat[%u] font[%u]\n", indx, u32LatSize, u32FontSize);
                continue;
            }

            u32NameLen = pstOsdRgn->u32Width;
            if (NULL == stSvaPicTarPrm)
            {
                SVA_LOGW(" stSvaPicTarPrm == NULL \n");
                return SAL_FAIL;
            }

            stSvaPicTarPrm->uiOsdHeight = pstOsdRgn->u32Height;
        }

        u32NumLen = 0;

        if (uiConfidence == SAL_TRUE)
        {
            if ((SVA_OSD_TYPE_CROSS_NO_RECT_TYPE != pstIn->drawType) && (SVA_OSD_TYPE_CROSS_RECT_TYPE != pstIn->drawType))
            {
                if (SVA_NO_SUB_TYPE_IDX == uiSubTypeIdx)
                {
                    if (NULL == (pstNumLatRgn = osd_func_getNumLatRegion(OSD_BLOCK_IDX_NUM_PAREN, reliability, 0, u32LatSize)))
                    {
                        SVA_LOGW("get num osd fail, num[%u], lat[%u]\n", reliability, u32LatSize);
                        continue;
                    }
                }
                else
                {
                    if (NULL == (pstNumLatRgn = osd_func_getNumLatRegion(OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT, reliability, uiSubTypeIdx, u32LatSize)))
                    {
                        SVA_LOGW("get num osd fail, num[%u], lat[%u]\n", reliability, u32LatSize);
                        continue;
                    }
                }
            }
            else
            {
                if (NULL == (pstNumLatRgn = osd_func_getNumLatRegion(OSD_BLOCK_IDX_NUM_PAREN, reliability, 0, u32LatSize)))
                {
                    SVA_LOGW("get num osd fail, num[%u], lat[%u]\n", reliability, u32LatSize);
                    continue;
                }
            }

            if (SAL_SOK != osd_func_FillFont(pstNumLatRgn, u32LatSize, pstNumFontRgn, u32FontSize, u32Argb1555, u32BgColor))
            {
                SVA_LOGW("fill osd font fail, lat[%u] font[%u]\n", reliability, u32LatSize);
                continue;
            }

            u32NumLen = pstNumFontRgn->u32Width;
        }

        /* 根据画框和OSD方式重新计算框和OSD的坐标 */
        stOsdCorrectPrm.u32Width = u32NameLen + u32NumLen;
        stSvaPicTarPrm->uiOsdWidth = stOsdCorrectPrm.u32Width;
        if (pstOsdLatRgn == NULL)
        {
            SVA_LOGE("pstOsdLatRgn == NULL\n");
        }

        stOsdCorrectPrm.u32Height = stSvaPicTarPrm->uiOsdHeight;
        stOsdCorrectPrm.enBorderType = SVA_OSD_LINE_RECT_TYPE;

        pstOsd_corct = &osd_correct[0];

        pstOsd_corct->uiPicW = uiPicW;
        pstOsd_corct->uiPicH = uiPicH;
        pstOsd_corct->uiSourceW = uiPicW;
        pstOsd_corct->uiSourceH = uiPicH;
        pstOsd_corct->uiUpTarNum = 10;
        pstOsd_corct->bOsdPstOutFlag = SAL_TRUE;
        pstOsd_corct->uiChan = chan;
        s32Ret = disp_osdPstOutcorrect(pstOsd_corct, &stOsdCorrectPrm, \
                                       pSvaRect + j, &stSvaOutTmp, &stSvaOutTmp.target[j], NULL);
        if (s32Ret != SAL_SOK)
        {
            SVA_LOGE("j %d PstOutcorrect fail\n", j);
            return SAL_FAIL;
        }

        /* 竖线 */
        if (pstOsd_corct->uiUpOsdFlag)
        {
            /* OSD在框的上方 */
            SVA_LOGD("target: j %d, %d+%d < %d  \n", j, stOsdCorrectPrm.u32Y, stOsdCorrectPrm.u32Height, pSvaRect[j].y);
            stSvaPicTarPrm->uiUpTarNum++;
            uiUpOsdMaxY = pstOsd_corct->uiTmpOsdY > uiUpOsdMaxY ? pstOsd_corct->uiTmpOsdY : uiUpOsdMaxY;
        }
        else
        {
            /* OSD在框的下方 */
            SVA_LOGD("target: j %d, %d > %d + %d  \n", j, stOsdCorrectPrm.u32Y, pSvaRect[j].y, pSvaRect[j].height);
            stSvaPicTarPrm->uiDownTarNum++;
            uiDownOsdMaxY = pstOsd_corct->uiTmpOsdY > uiDownOsdMaxY ? pstOsd_corct->uiTmpOsdY : uiDownOsdMaxY;
        }
    }

    (VOID)osd_func_readEnd();

    /* TODO:筛选出有效的包裹上下方高度 */
    if (uiUpOsdMaxY == 0)
    {
        stSvaPicTarPrm->uiUpLen = 0;
    }
    else
    {
        stSvaPicTarPrm->uiUpLen = uiUpOsdMaxY - pstOsd_corct->uiUpStartY;
    }

    if (uiDownOsdMaxY == 0)
    {
        stSvaPicTarPrm->uiDownLen = 0;
    }
    else
    {
        stSvaPicTarPrm->uiDownLen = uiDownOsdMaxY - pstOsd_corct->uiUpStartY  \
                                    - stSvaPicTarPrm->uiUpLen - pstOsd_corct->uiSourceH;
    }

    SVA_LOGW("calc rslt:chan %d, uptar %d, uplen %d, downtar %d, downlen %d, uposdMax %d, downosdMax %d, osdh %d, srcH %d \n", chan,
             stSvaPicTarPrm->uiUpTarNum, stSvaPicTarPrm->uiUpLen, stSvaPicTarPrm->uiDownTarNum, stSvaPicTarPrm->uiDownLen,
             uiUpOsdMaxY, uiDownOsdMaxY, stSvaPicTarPrm->uiOsdHeight, uiPicH);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalJpegDrawLine
* 描  述  : jpeg画线, 默认cpu画线，VGS画框
* 输  入  : - chan        : 通道号
*         : - pstJpegFrame: jpeg帧数据
*         : - pstIn       : 智能配置参数
*         : - pstOut      : 智能结果信息
*         : - uiJpegEncChn: 编码通道
*         : - pcJpeg      : jpeg数据
*         : - pJpegSize   : jpeg大小
*         : - uiWidth     : 图片宽度
*         : - uiHeight    : 图片高度
*         : - mode        : 引擎模式
* 输  出  : - pcJpeg      : jpeg数据
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalJpegDrawLine(UINT32 chan,
                          SYSTEM_FRAME_INFO *pstJpegFrame,
                          CROP_S *pstCropPrm,
                          SVA_PROCESS_IN *pstIn,
                          SVA_PROCESS_OUT *pstOut,
                          void *jpgHdl,
                          INT8 *pcJpeg,
                          UINT32 *pJpegSize,
                          UINT32 uiWidth,
                          UINT32 uiHeight,
                          SVA_PROC_MODE_E mode,
                          SVA_OSD_JPG_PRM_S *pstOsdJpgPrm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 j = 0;
    UINT32 k = 0;
    UINT32 uiPicW = 0;
    UINT32 uiPicH = 0;
    UINT32 uiSourceW = 0;
    UINT32 uiSourceH = 0;
    UINT32 uiColor = 0;
    UINT32 uiConfidence = 0;
    UINT32 uiSubTypeIdx = 0;
    UINT32 scaleLevel = 0;
    UINT32 reliability = 0;  /* 0~100 */
    UINT32 indx = 0;
    UINT32 alert_num = 0;
    UINT32 bNeedMap = 0;
    UINT32 u32LatSize = 0;
    UINT32 u32FontSize = 0;
    UINT32 u32MaxTarW = 0;
    UINT32 uiLocMaxHFlag;

    SVA_BORDER_TYPE_E drawType = SVA_OSD_LINE_RECT_TYPE;              /* 叠框方式，默认为点线模式 */

    UINT32 u32Argb1555 = 0;
    UINT32 u32BgColor = 0;
    UINT32 u32up = 0;
    UINT32 u32down = 0;

    /* 违禁品相关参数 */
    UINT32 uiTmpRealTarNum = 0;
    SVA_PROCESS_OUT stSvaOutTmp = {0};
    SVA_TARGET stSvaTarget[SVA_XSI_MAX_ALARM_NUM] = {0};
    DISP_SVA_RECT_F *pSvaRect = NULL;

    /* 能力集相关 */
    /* BOOL bDrawOsd = SAL_TRUE;            R7项目需求，有框jpeg需要按照包裹进行裁剪，不需要叠加OSD和上拉线，只需要目标框 */
    UINT32 u32Thick = 2;                /* capb_get_line()->u32LineWidth; */
    CAPB_OSD *pstCapbOsd = capb_get_osd();

    SVA_HAL_CHECK_PRM(pstCapbOsd, SAL_FAIL);
    OSD_FONT_TYPE_E enFontType = pstCapbOsd->enFontType;
    BOOL bOsdUsrMod = SAL_TRUE;                 /* OSD背景填色开关 */

    /* 索引值 */
    UINT32 u32LineNum = 0;          /* 线数 */

    /* 使用CPU画框和线数据结构 */
    VGS_DRAW_AI_PRM *pstCpuLinePrm = NULL;
    VGS_DRAW_LINE_PRM *pstCpuLineArray = NULL;
    VGS_RECT_ARRAY_S *pstCpuRectArray = NULL;
    VGS_DRAW_RECT_S *pstCpuRect = NULL;

    /* 使用VGS画OSD数据结构 */
    VGS_ADD_OSD_PRM *pstVgsOsdPrm = NULL;
    VGS_OSD_PRM *pstOsd = NULL;
    VGS_OSD_PRM *pstNumOsd = NULL;
    OSD_REGION_S *pstOsdRgn = NULL;
    OSD_REGION_S *pstOsdLatRgn = NULL;
    OSD_REGION_S *pstNumLatRgn = NULL;
    OSD_REGION_S *pstNumFontRgn = NULL;
    OSD_SET_PRM_S *pstOsdPrm = NULL;
    SAL_VideoFrameBuf salVideoFrame = {0};
    UINT32 u32NameLen = 0;
    UINT32 u32NumLen = 0;
    UINT32 u32OsdHeightTmp = 0;

    DISP_OSD_COORDINATE_CORRECT *pstOsd_corct = NULL;
    DISP_OSD_COORDINATE_CORRECT osd_correct[SVA_MAX_PACKAGE_BUF_NUM] = {0};
    DISP_OSD_CORRECT_PRM_S stOsdCorrectPrm;

    UINT64 u64StartTime = 0;
    UINT64 u64EndTime = 0;
    UINT64 u64GapTime = 0;

    VGS_ARTICLE_LINE_TYPE ailinetype;
    JPEG_COMMON_ENC_PRM_S jpegEncPrm = {0};

    sal_memset_s(&ailinetype, sizeof(DISPLINE_PRM), 0x00, sizeof(DISPLINE_PRM));

    SVA_HAL_CHECK_PRM(pstIn, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstOut, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pcJpeg, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pJpegSize, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstJpegFrame, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstOsdJpgPrm, SAL_FAIL);

    memset(&salVideoFrame, 0, sizeof(SAL_VideoFrameBuf));
    s32Ret = sys_hal_getVideoFrameInfo(pstJpegFrame, &salVideoFrame);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGW("sys_hal_getVideoFrameInfo failed !\n");
    }

    /* 违禁品坐标转换 */
    sal_memset_s(&stSvaOutTmp, sizeof(SVA_PROCESS_OUT), 0x00, sizeof(SVA_PROCESS_OUT));
    sal_memcpy_s(&stSvaOutTmp, sizeof(SVA_PROCESS_OUT), pstOut, sizeof(SVA_PROCESS_OUT));
    sal_memset_s(&stSvaTarget, SVA_XSI_MAX_ALARM_NUM * sizeof(SVA_TARGET), 0x00, SVA_XSI_MAX_ALARM_NUM * sizeof(SVA_TARGET));

    uiSourceW = (UINT32)(stSvaOutTmp.packbagAlert.package_loc.width * (float)SVA_MODULE_WIDTH);
    uiSourceH = (UINT32)(stSvaOutTmp.packbagAlert.fRealPkgH * (float)SVA_MODULE_HEIGHT);

    stSvaOutTmp.packbagAlert.package_loc.y = stSvaOutTmp.packbagAlert.fRealPkgY - (float)pstOsdJpgPrm->uiUpLen / SVA_MODULE_HEIGHT;
    stSvaOutTmp.packbagAlert.package_loc.height = stSvaOutTmp.packbagAlert.fRealPkgH + (float)pstOsdJpgPrm->uiUpLen / SVA_MODULE_HEIGHT + (float)pstOsdJpgPrm->uiDownLen / SVA_MODULE_HEIGHT;

    uiPicW = (UINT32)(stSvaOutTmp.packbagAlert.package_loc.width * (float)SVA_MODULE_WIDTH);
    uiPicH = (UINT32)(stSvaOutTmp.packbagAlert.package_loc.height * (float)SVA_MODULE_HEIGHT);

    /* 大包裹的特殊处理: 违禁品坐标转换为相对于原始包裹的坐标，再将各违禁品的y坐标下移 图片上方的空白区域高度    */
    if (stSvaOutTmp.packbagAlert.package_loc.y < 0.0)
    {
        uiLocMaxHFlag = SAL_TRUE;
        SVA_LOGD("loc y %f < 0, height %f \n", stSvaOutTmp.packbagAlert.package_loc.y, stSvaOutTmp.packbagAlert.package_loc.height);
        stSvaOutTmp.packbagAlert.package_loc.y = stSvaOutTmp.packbagAlert.fRealPkgY;
        stSvaOutTmp.packbagAlert.package_loc.height = stSvaOutTmp.packbagAlert.fRealPkgH;
        uiPicW = SVA_MODULE_WIDTH;
        uiPicH = SVA_MODULE_HEIGHT;

        (VOID)Sva_HalTransTarLocJpg(&stSvaOutTmp, &uiTmpRealTarNum, &stSvaTarget[0]);
    }
    else
    {
        uiLocMaxHFlag = SAL_FALSE;
        (VOID)Sva_HalTransTarLoc(&stSvaOutTmp, &uiTmpRealTarNum, &stSvaTarget[0]);
    }

    k = 0;
    for (j = 0; j < uiTmpRealTarNum; j++)
    {
        if (SAL_TRUE == stSvaTarget[j].bAnotherViewTar || stSvaTarget[j].type >= SVA_XSI_MAX_ALARM_NUM)
        {
            continue;
        }

        sal_memcpy_s(&stSvaOutTmp.target[k], sizeof(SVA_TARGET), &stSvaTarget[j], sizeof(SVA_TARGET));
        if (SAL_TRUE == uiLocMaxHFlag)
        {
            stSvaOutTmp.target[k].rect.y += (float)pstOsdJpgPrm->uiUpLen / SVA_MODULE_HEIGHT;
        }

        k++;
    }

    stSvaOutTmp.target_num = k;
    SVA_LOGD("Draw: chan %d, source_tarnum %d, tmp_targetnum %d\n", chan, pstOut->target_num, stSvaOutTmp.target_num);

    if (SVA_PROC_MODE_IMAGE != mode && NULL != pstCropPrm)
    {
        pstCropPrm->u32X = 0;
        pstCropPrm->u32Y = 0;
        pstCropPrm->u32CropEnable = SAL_TRUE;
        SVA_LOGD("Targetnum %d, uplen %d, downlen %d \n", uiTmpRealTarNum, pstOsdJpgPrm->uiUpLen, pstOsdJpgPrm->uiDownLen);
        if (uiTmpRealTarNum != 0)
        {
            pstCropPrm->u32H += pstOsdJpgPrm->uiUpLen + pstOsdJpgPrm->uiDownLen;
        }
    }

    /* 初始化CPU画框参数 */
    pstCpuLinePrm = gpastSvaLinePrm[chan];
    if ((NULL == pstCpuLinePrm) || (NULL == jpgHdl))
    {
        SVA_LOGE("please alloc memory first\n");
        return SAL_FAIL;
    }

    pstCpuLineArray = &pstCpuLinePrm->VgsDrawVLine;
    pstCpuLineArray->uiLineNum = 0;
    pstCpuRectArray = &pstCpuLinePrm->VgsDrawRect;
    pstCpuRectArray->u32RectNum = 0;
    pstCpuRectArray->enExceedType = RECT_EXCEED_TYPE_CLOSE;

    if (SVA_PROC_MODE_IMAGE == mode)
    {
        sal_memcpy_s(&stSvaOutTmp, sizeof(SVA_PROCESS_OUT), pstOut, sizeof(SVA_PROCESS_OUT));
        sal_memset_s(&stSvaOutTmp.target[j], SVA_XSI_MAX_ALARM_NUM * sizeof(SVA_TARGET), 0x00, SVA_XSI_MAX_ALARM_NUM * sizeof(SVA_TARGET));
        for (j = 0; j < pstOut->target_num; j++)
        {
            if (SAL_TRUE == pstOut->target[j].bAnotherViewTar)
            {
                continue;
            }

            sal_memcpy_s(&stSvaOutTmp.target[k], sizeof(SVA_TARGET), &pstOut->target[j], sizeof(SVA_TARGET));
            k++;
        }

        uiPicW = SVA_MODULE_WIDTH;
        uiPicH = SVA_MODULE_HEIGHT;
    }

    /* 初始化用VGS画OSD */
    pstNumFontRgn = osd_func_getVarFontRegionSet(gau32SvaVarOsdChn[chan], stSvaOutTmp.target_num);
    pstVgsOsdPrm = gpastSvaOsdPrm[chan];
    if ((NULL == pstVgsOsdPrm) || (NULL == pstNumFontRgn))
    {
        SVA_LOGE("please alloc memory first[]\n");
        return SAL_FAIL;
    }

    sal_memset_s(pstVgsOsdPrm, sizeof(VGS_ADD_OSD_PRM), 0x00, sizeof(VGS_ADD_OSD_PRM));

#if 0  /* 画黄色包裹框 */
    if ((uiW > pstIn->uiPkgSplitFilter) && (uiH > 0))
    {
        /*竖线*/
        pstCpuLineArray->uiLineNum = 4;
        pstCpuLineArray->stVgsLinePrm[0].stStartPoint.s32X = uiX;
        pstCpuLineArray->stVgsLinePrm[0].stStartPoint.s32X = uiY;
        pstCpuLineArray->stVgsLinePrm[0].stEndPoint.s32X = uiX + uiW;
        pstCpuLineArray->stVgsLinePrm[0].stEndPoint.s32X = uiY;

        /*横线*/
        pstCpuLineArray->stVgsLinePrm[1].stStartPoint.s32X = uiX;
        pstCpuLineArray->stVgsLinePrm[1].stStartPoint.s32X = uiY;
        pstCpuLineArray->stVgsLinePrm[1].stEndPoint.s32X = uiX;
        pstCpuLineArray->stVgsLinePrm[1].stEndPoint.s32X = uiY + uiH;

        pstCpuLineArray->stVgsLinePrm[2].stStartPoint.s32X = uiX + uiW;
        pstCpuLineArray->stVgsLinePrm[2].stStartPoint.s32X = uiY + uiH;
        pstCpuLineArray->stVgsLinePrm[2].stEndPoint.s32X = uiX + uiW;
        pstCpuLineArray->stVgsLinePrm[2].stEndPoint.s32X = uiY;

        pstCpuLineArray->stVgsLinePrm[3].stStartPoint.s32X = uiX + uiW + 4;
        pstCpuLineArray->stVgsLinePrm[3].stStartPoint.s32X = uiY + uiH;
        pstCpuLineArray->stVgsLinePrm[3].stEndPoint.s32X = uiX;
        pstCpuLineArray->stVgsLinePrm[3].stEndPoint.s32X = uiY + uiH;

        for (i = 0; i < pstCpuLineArray->uiLineNum; i++)
        {
            pstCpuLineArray->stVgsLinePrm[i].stStartPoint.s32X = (pstCpuLineArray->stVgsLinePrm[i].stStartPoint.s32X + 1) / 2 * 2;
            pstCpuLineArray->stVgsLinePrm[i].stStartPoint.s32Y = (pstCpuLineArray->stVgsLinePrm[i].stStartPoint.s32Y + 1) / 2 * 2;
            pstCpuLineArray->stVgsLinePrm[i].stEndPoint.s32X = (pstCpuLineArray->stVgsLinePrm[i].stEndPoint.s32X + 1) / 2 * 2;
            pstCpuLineArray->stVgsLinePrm[i].stEndPoint.s32Y = (pstCpuLineArray->stVgsLinePrm[i].stEndPoint.s32Y + 1) / 2 * 2;
            pstCpuLineArray->stVgsLinePrm[i].u32Color = 0xEEEE00;
            pstCpuLineArray->stVgsLinePrm[i].u32Thick = 4;
            /* pstCpuLineArray->linetype[i] = ??; */
            /* pstCpuLineArray->af32XFract[i] = -1; */
        }

        pstCpuLineArray->uiLineNum = 0;  /* no pkg region */
        /* vgs_hal_drawLineArray(pstJpegFrame, pstCpuLineArray); */
    }

#endif

    /* 十字叠框类型需要对重复目标进行过滤 */
    if (SVA_OSD_TYPE_CROSS_NO_RECT_TYPE == pstIn->drawType
        || SVA_OSD_TYPE_CROSS_RECT_TYPE == pstIn->drawType)
    {
        (VOID)disp_osdGetRealOut(chan, pstIn, &stSvaOutTmp, NULL);
        DISP_LOGD("222:alert_num %d pstOut->target_num[%d] \n", alert_num, pstOut->target_num);
    }

    /* 初始化矫正参数，按照画面中所有的包裹进行初始化 */
    (VOID)disp_osdInitCocrtInfo(NULL, SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT, pstIn, &stSvaOutTmp, &osd_correct[0]);


    /* 用户参数判断 */
    if (SVA_ALERT_EXT_PERCENT == pstIn->stTargetPrm.enOsdExtType)
    {
        uiConfidence = SAL_TRUE;
    }
    else
    {
        uiConfidence = SAL_FALSE;
    }

    alert_num = stSvaOutTmp.target_num;
    /* 抓图使用默认字体大小 */
    scaleLevel = 2;

    /* truetype点阵大小和字体大小保持一致，hzk16点阵大小固定为16 */
    if (OSD_FONT_TRUETYPE == enFontType)
    {
        u32LatSize = osd_func_getFontSize(scaleLevel);
        u32LatSize = 18;
        u32FontSize = u32LatSize;
    }
    else
    {
        u32LatSize = 16;
        u32FontSize = scaleLevel * 16;
    }

    bNeedMap = SAL_FALSE;

    memset(&salVideoFrame, 0, sizeof(SAL_VideoFrameBuf));
    s32Ret = sys_hal_getVideoFrameInfo(pstJpegFrame, &salVideoFrame);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGW("sys_hal_getVideoFrameInfo failed !\n");
    }

    pSvaRect = g_SvaRect[chan];
    sal_memset_s(pSvaRect, SVA_XSI_MAX_ALARM_NUM * sizeof(DISP_SVA_RECT_F), 0x00, SVA_XSI_MAX_ALARM_NUM * sizeof(DISP_SVA_RECT_F));

    /* 将违禁品浮点型坐标转换成整型并赋值给全局结构体 */
    s32Ret = Sva_HalPstOutFloattoINT(&stSvaOutTmp, uiPicW, uiPicH, chan);

    /* 按照目标顶点X坐标对所有违禁品根据索引ID升序排列进行排序 */
    s32Ret = Sva_HalOsdPstOutSort(pstIn, &stSvaOutTmp, pSvaRect);
    if (s32Ret != SAL_SOK)
    {
        SVA_LOGE("chn %d pstOutSort failed\n", chan);
        return SAL_FAIL;
    }

    for (k = 0; k < pstOut->target_num; k++)
    {
        SVA_LOGD("111:target:chan %d, j %d, id %d, type %d, [%f, %f][%f, %f] ,bAnotherViewTar %d\n", chan,
                 k, pstOut->target[k].ID, pstOut->target[k].type,
                 pstOut->target[k].rect.x,
                 pstOut->target[k].rect.y,
                 pstOut->target[k].rect.width,
                 pstOut->target[k].rect.height,
                 pstOut->target[k].bAnotherViewTar);
    }

    for (k = 0; k < stSvaOutTmp.target_num; k++)
    {
        SVA_LOGD("222:target:chan %d, j %d, id %d, type %d, [%f, %f][%f, %f] ,bAnotherViewTar %d\n", chan,
                 k, stSvaOutTmp.target[k].ID, stSvaOutTmp.target[k].type,
                 stSvaOutTmp.target[k].rect.x,
                 stSvaOutTmp.target[k].rect.y,
                 stSvaOutTmp.target[k].rect.width,
                 stSvaOutTmp.target[k].rect.height,
                 stSvaOutTmp.target[k].bAnotherViewTar);
    }

    for (k = 0; k < stSvaOutTmp.target_num; k++)
    {
        SVA_LOGD("333:target:chan %d, j %d, [%d, %d][%d, %d], uiPicW %d, Pich %d\n", chan,
                 k,
                 pSvaRect[k].x,
                 pSvaRect[k].y,
                 pSvaRect[k].width,
                 pSvaRect[k].height, uiPicW, uiPicH);
    }

    (VOID)osd_func_readStart();

    /* 报警物 */
    for (j = 0; j < alert_num; j++)
    {
        if (j >= SVA_XSI_MAX_ALARM_NUM)
        {
            SVA_LOGE("chn %d alert_num %d\n", chan, alert_num);
            break;
        }

        /* 任何包裹展示方式时，双视角目标合并的target直接忽略 */
        if (SAL_TRUE == stSvaOutTmp.target[j].bAnotherViewTar)
        {
            continue;
        }

        /* 异常条件保护 */
        if (0 == pSvaRect[j].width || 0 == pSvaRect[j].height)
        {
            SVA_LOGW("chn %d, id %d, type %d, width %d, height %d, alert_num %d\n", chan,
                     stSvaOutTmp.target[j].ID, stSvaOutTmp.target[j].type,
                     pSvaRect[j].width, pSvaRect[j].height, alert_num);
            continue;
        }

        SVA_LOGD("444:target:chan %d, j %d, id %d, [%d, %d][%d, %d]\n", chan,
                 j,
                 stSvaOutTmp.target[j].ID,
                 pSvaRect[j].x,
                 pSvaRect[j].y,
                 pSvaRect[j].width,
                 pSvaRect[j].height);

        indx = stSvaOutTmp.target[j].type;
        reliability = stSvaOutTmp.target[j].visual_confidence;
        reliability = (reliability >= 100) ? (99) : (reliability);
        uiSubTypeIdx = stSvaOutTmp.target[j].u32SubTypeIdx;
        if (NULL == (pstOsdPrm = osd_func_getOsdPrm(OSD_BLOCK_IDX_STRING, indx)))
        {
            SVA_LOGW("get osd fail param, idx[%u]\n", indx);
            continue;
        }

        uiColor = pstOsdPrm->u32Color;
        SAL_RGB24ToARGB1555(uiColor, &u32Argb1555, 1);
        u32BgColor = OSD_BACK_COLOR;
        if (SAL_TRUE == bOsdUsrMod)
        {
            u32BgColor = u32Argb1555;
            u32Argb1555 = OSD_COLOR_WHITE;
        }

        /* 数据校验防止画出图 */
        if (pSvaRect[j].x >= uiPicW || pSvaRect[j].y >= uiPicH)
        {
            continue;
        }

        /* 违禁品名称OSD的点阵 */
        if (NULL == (pstOsdLatRgn = osd_func_getLatRegion(OSD_BLOCK_IDX_STRING, indx, u32LatSize)))
        {
            DISP_LOGW("get alert name osd lattice fail, idx[%u], lat[%u]\n", indx, u32LatSize);
            continue;
        }

        /* 违禁品名称的OSD */
        if (NULL == (pstOsdRgn = osd_func_getFontRegion(OSD_BLOCK_IDX_STRING, indx, u32LatSize, u32FontSize, (0 == bOsdUsrMod) ? SAL_FALSE : SAL_TRUE)))
        {
            SVA_LOGW("get alert name osd fail, idx[%u], lat[%u] font[%u]\n", indx, u32LatSize, u32FontSize);
            continue;
        }

        u32NameLen = pstOsdRgn->u32Width;
        u32OsdHeightTmp = pstOsdLatRgn->u32Height;
        u32NumLen = 0;

        if (uiConfidence == SAL_TRUE)
        {
            if ((SVA_OSD_TYPE_CROSS_NO_RECT_TYPE != pstIn->drawType) && (SVA_OSD_TYPE_CROSS_RECT_TYPE != pstIn->drawType))
            {
                if (SVA_NO_SUB_TYPE_IDX == uiSubTypeIdx)
                {
                    if (NULL == (pstNumLatRgn = osd_func_getNumLatRegion(OSD_BLOCK_IDX_NUM_PAREN, reliability, 0, u32LatSize)))
                    {
                        SVA_LOGW("get num osd fail, num[%u], lat[%u]\n", reliability, u32LatSize);
                        continue;
                    }
                }
                else
                {
                    if (NULL == (pstNumLatRgn = osd_func_getNumLatRegion(OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT, reliability, uiSubTypeIdx, u32LatSize)))
                    {
                        SVA_LOGW("get num osd fail, num[%u], lat[%u]\n", reliability, u32LatSize);
                        continue;
                    }
                }
            }
            else
            {
                if (NULL == (pstNumLatRgn = osd_func_getNumLatRegion(OSD_BLOCK_IDX_NUM_PAREN, reliability, 0, u32LatSize)))
                {
                    SVA_LOGW("get num osd fail, num[%u], lat[%u]\n", reliability, u32LatSize);
                    continue;
                }
            }

            if (SAL_SOK != osd_func_FillFont(pstNumLatRgn, u32LatSize, pstNumFontRgn, u32FontSize, u32Argb1555, u32BgColor))
            {
                SVA_LOGW("fill osd font fail, lat[%u] font[%u]\n", reliability, u32LatSize);
                continue;
            }

            u32NumLen = pstNumFontRgn->u32Width;
        }

        /* 根据画框和OSD方式重新计算框和OSD的坐标 */
        stOsdCorrectPrm.u32Width = u32NameLen + u32NumLen;
        stOsdCorrectPrm.u32Height = u32OsdHeightTmp * ((OSD_FONT_TRUETYPE == enFontType) ? 1 : scaleLevel);
        stOsdCorrectPrm.enBorderType = drawType;
        pstOsd_corct = &osd_correct[0];

        pstOsd_corct->uiPicW = uiPicW;
        pstOsd_corct->uiPicH = uiPicH;
        pstOsd_corct->uiSourceW = uiSourceW;
        pstOsd_corct->uiSourceH = uiSourceH;
        pstOsd_corct->uiUpTarNum = 10;
        pstOsd_corct->bOsdPstOutFlag = SAL_TRUE;
        pstOsd_corct->uiChan = chan;

        if (SVA_PROC_MODE_IMAGE == mode)
        {
            pstOsd_corct->uiPicW = SVA_MODULE_WIDTH;
            pstOsd_corct->uiPicH = SVA_MODULE_HEIGHT;
            pstOsd_corct->uiSourceW = SVA_MODULE_WIDTH;
            pstOsd_corct->uiSourceH = SVA_MODULE_HEIGHT;
            pstOsd_corct->bOsdPstOutFlag = SAL_FALSE;
        }

        s32Ret = disp_osdPstOutcorrect(pstOsd_corct, &stOsdCorrectPrm, \
                                       pSvaRect + j, &stSvaOutTmp, &stSvaOutTmp.target[j], NULL);
        if (s32Ret != SAL_SOK)
        {
            SVA_LOGE("chn %d j %d PstOutcorrect fail\n", chan, j);
            continue;
        }

        if ((stOsdCorrectPrm.u32X >= uiPicW)) /* || (stOsdCorrectPrm.u32Y >= uiPicH) */
        {
            SVA_LOGD("invalid correct prm x*y[%d, %d], Pic W*h [%d, %d] \n",
                     stOsdCorrectPrm.u32X, stOsdCorrectPrm.u32Y, uiPicW, uiPicH);
            continue;
        }

        /* 框和线使用同一模块画 */
        /* 矩形 */
        pstCpuRect = pstCpuRectArray->astRect + pstCpuRectArray->u32RectNum++;

        pstCpuRect->s32X = pSvaRect[j].x;
        pstCpuRect->s32Y = pSvaRect[j].y;
        pstCpuRect->u32Width = pSvaRect[j].width;
        pstCpuRect->u32Height = pSvaRect[j].height;
        pstCpuRect->f32StartXFract = pSvaRect[j].f32StartXFract;
        pstCpuRect->f32EndXFract = pSvaRect[j].f32EndXFract;
        pstCpuRect->u32Color = uiColor;
        pstCpuRect->u32Thick = u32Thick;
        memcpy(&pstCpuRect->stLinePrm, &ailinetype.ailine, sizeof(ailinetype.ailine));

        /* 竖线 */
        u32LineNum = pstCpuLineArray->uiLineNum;
        if (pstOsd_corct->uiUpOsdFlag)
        {
            if (stOsdCorrectPrm.u32Y + stOsdCorrectPrm.u32Height < pSvaRect[j].y)
            {
                /* OSD在框的上方 */
                SVA_LOGD("target:chan %d, j %d, %d+%d < %d  \n", chan, j, stOsdCorrectPrm.u32Y, stOsdCorrectPrm.u32Height, pSvaRect[j].y);
                pstCpuLineArray->stVgsLinePrm[u32LineNum].stStartPoint.s32X = pSvaRect[j].x;
                pstCpuLineArray->stVgsLinePrm[u32LineNum].stStartPoint.s32Y = stOsdCorrectPrm.u32Y + stOsdCorrectPrm.u32Height;
                pstCpuLineArray->stVgsLinePrm[u32LineNum].stEndPoint.s32X = pSvaRect[j].x;
                pstCpuLineArray->stVgsLinePrm[u32LineNum].stEndPoint.s32Y = pSvaRect[j].y;
                pstCpuLineArray->stVgsLinePrm[u32LineNum].u32Thick = u32Thick;
                pstCpuLineArray->stVgsLinePrm[u32LineNum].u32Color = uiColor;
                pstCpuLineArray->af32XFract[u32LineNum] = pSvaRect[j].f32StartXFract;
                memcpy(pstCpuLineArray->linetype + u32LineNum, &ailinetype.ailine, sizeof(ailinetype.ailine));

                pstCpuLineArray->uiLineNum++;
                u32up++;
            }
        }
        else
        {
            if (stOsdCorrectPrm.u32Y > pSvaRect[j].y + pSvaRect[j].height)
            {
                /* OSD在框的下方 */
                SVA_LOGD("target222:  j %d, %d > %d + %d  \n", j, stOsdCorrectPrm.u32Y, pSvaRect[j].y, pSvaRect[j].height);
                pstCpuLineArray->stVgsLinePrm[u32LineNum].stStartPoint.s32X = pSvaRect[j].x;
                pstCpuLineArray->stVgsLinePrm[u32LineNum].stStartPoint.s32Y = pSvaRect[j].y + pSvaRect[j].height;
                pstCpuLineArray->stVgsLinePrm[u32LineNum].stEndPoint.s32X = pSvaRect[j].x;
                pstCpuLineArray->stVgsLinePrm[u32LineNum].stEndPoint.s32Y = stOsdCorrectPrm.u32Y;
                pstCpuLineArray->stVgsLinePrm[u32LineNum].u32Thick = u32Thick;
                pstCpuLineArray->stVgsLinePrm[u32LineNum].u32Color = uiColor;
                pstCpuLineArray->af32XFract[u32LineNum] = pSvaRect[j].f32StartXFract;
                memcpy(pstCpuLineArray->linetype + u32LineNum, &ailinetype.ailine, sizeof(ailinetype.ailine));

                pstCpuLineArray->uiLineNum++;

                u32down++;
            }
        }

        /* 汉字OSD */
        pstOsd = pstVgsOsdPrm->stVgsOsdPrm + pstVgsOsdPrm->uiOsdNum++;
        pstOsd->u64PhyAddr = pstOsdRgn->stAddr.u64PhyAddr;
        pstOsd->pVirAddr = pstOsdRgn->stAddr.pu8VirAddr;
        pstOsd->enPixelFmt = SAL_VIDEO_DATFMT_ARGB16_1555;
        pstOsd->s32X = stOsdCorrectPrm.u32X;
        pstOsd->s32Y = stOsdCorrectPrm.u32Y;
        pstOsd->u32Width = pstOsdRgn->u32Width;
        pstOsd->u32Height = pstOsdRgn->u32Height;
        pstOsd->u32BgColor = u32BgColor;
        pstOsd->u32BgAlpha = 0;
        pstOsd->u32FgAlpha = 255;
        pstOsd->u32Stride = pstOsdRgn->u32Stride;

        /* 数字OSD */
        if ((uiConfidence == SAL_TRUE) && (NULL != pstNumFontRgn))
        {
            pstNumOsd = pstVgsOsdPrm->stVgsOsdPrm + pstVgsOsdPrm->uiOsdNum++;
            pstNumOsd->u64PhyAddr = pstNumFontRgn->stAddr.u64PhyAddr;
            pstNumOsd->pVirAddr = pstNumFontRgn->stAddr.pu8VirAddr;
            pstNumOsd->enPixelFmt = SAL_VIDEO_DATFMT_ARGB16_1555;
            pstNumOsd->s32X = pstOsd->s32X + pstOsd->u32Width;
            pstNumOsd->s32Y = pstOsd->s32Y;
            pstNumOsd->u32Width = pstNumFontRgn->u32Width;
            pstNumOsd->u32Height = pstNumFontRgn->u32Height;
            pstNumOsd->u32BgColor = u32BgColor;
            pstNumOsd->u32BgAlpha = 0;
            pstNumOsd->u32FgAlpha = 255;
            pstNumOsd->u32Stride = pstNumFontRgn->u32Stride;
            pstNumFontRgn++;
        }

        if (SAL_TRUE == uiConfidence && (NULL != pstNumOsd))
        {
            /* TODO:遍历所有包裹，判断x+osd长度+置信度长度 >图片的宽，是则增大裁剪图片的宽度；否则不变    */
            if (pstOsd->s32X + pstOsd->u32Width + pstNumOsd->u32Width > pstCropPrm->u32W)
            {
                u32MaxTarW = u32MaxTarW > (pstNumOsd->s32X + pstNumOsd->u32Width - pstCropPrm->u32W) ? u32MaxTarW : (pstNumOsd->s32X + pstNumOsd->u32Width - pstCropPrm->u32W);

                SVA_LOGD("ywn:x %d + numosd %d = %d, uiSourceW %d  \n", pstOsd->s32X, pstOsd->u32Width, pstNumOsd->s32X + pstNumOsd->u32Width, u32MaxTarW);
            }
        }
        else
        {
            /* TODO:遍历所有包裹，判断x+osd长度+置信度长度 >图片的宽，是则增大裁剪图片的宽度；否则不变    */
            if (pstOsd->s32X + pstOsd->u32Width > pstCropPrm->u32W)
            {
                u32MaxTarW = u32MaxTarW > (pstOsd->s32X + pstOsd->u32Width - pstCropPrm->u32W) ? u32MaxTarW : (pstOsd->s32X + pstOsd->u32Width - pstCropPrm->u32W);

                SVA_LOGD("ywn:x %d + numosd %d = %d, uiSourceW %d  \n", pstOsd->s32X, pstOsd->u32Width, pstOsd->s32X + pstOsd->u32Width, u32MaxTarW);
            }
        }
    }

    if (SVA_PROC_MODE_IMAGE != mode && NULL != pstCropPrm)
    {
        pstCropPrm->u32W += u32MaxTarW;
    }

    u64StartTime = SAL_getCurUs();

    vgs_func_drawRectArraySoft(&salVideoFrame, pstCpuRectArray, bNeedMap, SAL_TRUE);
    vgs_func_drawLineSoft(&salVideoFrame, pstCpuLineArray, bNeedMap, SAL_TRUE);
    if (pstVgsOsdPrm->uiOsdNum > 0)
    {
        vgs_hal_drawLineOSDArray(pstJpegFrame, pstVgsOsdPrm, NULL);
    }
    else
    {
        SVA_LOGW("uiOsdNum error [%d], alert_num [%d]\n", pstVgsOsdPrm->uiOsdNum, alert_num);
    }

    u64EndTime = SAL_getCurUs();
    u64GapTime = u64EndTime - u64StartTime;
    if (0)
    {
        SVA_LOGE("dsp run cost %llu us\n", u64GapTime);
    }

    (VOID)osd_func_readEnd();

#if 0  /* bmp数据不叠框，故提前处理 */
       /* 将画完线的yuv数据拷贝到cached_mmz内存中，提高bmp转格式的处理速度，23ms左右 */
    pstOutFrameInfo = (VIDEO_FRAME_INFO_S *)stJpegFrame.uiAppData;
    sal_memcpy_s((char *)pstFrameInfo, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2,
                 (char *)pstOutFrameInfo->stVFrame.u64VirAddr[0], SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2);
    pstTmpFrameInfo = (VIDEO_FRAME_INFO_S *)pstSysFrameInfo->uiAppData;
    pstTmpFrameInfo->stVFrame.u64VirAddr[0] = (HI_U64)pstFrameInfo;

    /* YUV转RGB */
    time0 = SAL_getCurMs();
    stBmpRslt.cBmpAddr = pcBmp;
    s32Ret = vdec_tsk_SaveYuv2Bmp(pstSysFrameInfo, &stBmpRslt);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("YUV 2 RGB Failed! \n");
        return SAL_FAIL;
    }

    time1 = SAL_getCurMs();
    *pBmpSize = stBmpRslt.uiBmpSize;
#endif

    jpegEncPrm.stSysFrame = *pstJpegFrame;
    jpegEncPrm.pOutJpeg = pcJpeg;

    /* szl_todo: 增加传入的裁剪参数 */
    if (SVA_PROC_MODE_IMAGE != mode && NULL != pstCropPrm)
    {
        pstCropPrm->u32CropEnable = SAL_TRUE;
        pstCropPrm->u32W = SAL_align(pstCropPrm->u32W, 2);
        pstCropPrm->u32H = SAL_align(pstCropPrm->u32H, 2);
        pstCropPrm->u32W = pstCropPrm->u32W > SVA_MODULE_WIDTH ? SVA_MODULE_WIDTH : pstCropPrm->u32W;
        pstCropPrm->u32H = pstCropPrm->u32H > SVA_MODULE_HEIGHT ? SVA_MODULE_HEIGHT : pstCropPrm->u32H;
        SVA_LOGW("chan %d, crop x %d, y %d, w %d ,h %d, type %d, up %d, down %d \n",
                 chan, pstCropPrm->u32X, pstCropPrm->u32Y, pstCropPrm->u32W, pstCropPrm->u32H, pstCropPrm->enCropType, u32up, u32down);
    }

    if (u32up != pstOsdJpgPrm->uiUpTarNum || u32down != pstOsdJpgPrm->uiDownTarNum)
    {
        SVA_LOGW("jpg:chan %d, upTar %d != calc upTar %d || downTar %d != calc downTar %d \n", chan,
                 u32up, pstOsdJpgPrm->uiUpTarNum, u32down, pstOsdJpgPrm->uiDownTarNum);
    }

    (VOID)jpeg_drv_cropEnc(jpgHdl, &jpegEncPrm, pstCropPrm);
    *pJpegSize = jpegEncPrm.outSize;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalJpegDrawAiInfo
* 描  述  : 将智能信息画在yuv上
* 输  入  : - chan        : 通道号
*         : - pstJpegFrame: jpeg帧数据
*         : - pstOut      : 智能结果信息
*         : - uiWith      : 宽度
*         : - uiHeight    : 高度
*         : - uiModule    : 模块
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalJpegDrawAiInfo(UINT32 chan,
                            SYSTEM_FRAME_INFO *pstJpegFrame,
                            SVA_PROCESS_IN *pstIn,
                            SVA_PROCESS_OUT *pstOut,
                            UINT32 uiWith,
                            UINT32 uiHeight,
                            UINT32 uiModule)
{
    UINT32 j = 0;
    UINT32 uiX = 0;
    UINT32 uiY = 0;
    UINT32 uiW = 0;
    UINT32 uiH = 0;
    UINT32 uiColor = 0;
    UINT32 u32BgColor = 0;
    UINT32 uiConfidence = 0;
    UINT32 scaleLevel = 0;
    UINT32 reliability = 0;  /* 0~100 */
    UINT32 indx = 0;
    UINT32 alert_num = 0;
    UINT32 yOffset = 18;
    UINT32 u32LatSize = 0;
    UINT32 u32FontSize = 0;
    INT32 s32Ret = SAL_FAIL;
    VGS_ARTICLE_LINE_TYPE ailinetype;
    UINT32 u32Thick = capb_get_line()->u32LineWidth;

    CAPB_OSD *pstCapbOsd = capb_get_osd();

    SVA_HAL_CHECK_PRM(pstCapbOsd, SAL_FAIL);

    OSD_FONT_TYPE_E enFontType = pstCapbOsd->enFontType;

    /* 使用CPU画框和线数据结构 */
    VGS_RECT_ARRAY_S *pstCpuRectArray = NULL;
    VGS_DRAW_RECT_S *pstCpuRect = NULL;

    /* 使用VGS画OSD数据结构 */
    VGS_ADD_OSD_PRM *pstVgsOsdPrm = NULL;
    VGS_OSD_PRM *pstOsd = NULL;
    VGS_OSD_PRM *pstNumOsd = NULL;
    OSD_REGION_S *pstOsdRgn = NULL;
    OSD_REGION_S *pstNumLatRgn = NULL;
    OSD_REGION_S *pstNumFontRgn = NULL;
    SAL_VideoFrameBuf salVideoFrame = {0};

    /* TODO: wait till vgs module modified */
    /* float fStartPointXFract[SVA_XSI_MAX_ALARM_NUM] = {0};  / * 开始点的x坐标的小数部分，即左竖线x坐标的小数部分 * / */
    /* float fEndPointXFract[SVA_XSI_MAX_ALARM_NUM] = {0};    / * 结束点的x坐标的小数部分，即右竖线x坐标的小数部分 * / */

    SVA_HAL_CHECK_PRM(pstIn, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstOut, SAL_FAIL);
    SVA_HAL_CHECK_PRM(pstJpegFrame, SAL_FAIL);

    memset(&salVideoFrame, 0, sizeof(SAL_VideoFrameBuf));
    s32Ret = sys_hal_getVideoFrameInfo(pstJpegFrame, &salVideoFrame);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGW("sys_hal_getVideoFrameInfo failed !\n");
    }

    /* 初始化CPU画框参数 */
    pstCpuRectArray = gpastJpegRectPrm[chan];
    if (NULL == pstCpuRectArray)
    {
        SVA_LOGE("please alloc memory first\n");
        return SAL_FAIL;
    }

    pstCpuRectArray->u32RectNum = 0;
    pstCpuRectArray->enExceedType = RECT_EXCEED_TYPE_CLOSE;

    /* 初始化用VGS画OSD */
    pstNumFontRgn = osd_func_getVarFontRegionSet(gau32JpegVarOsdChn[chan], pstOut->target_num);
    pstVgsOsdPrm = gpastJpegOsdPrm[chan];
    if ((NULL == pstVgsOsdPrm) || (NULL == pstNumFontRgn))
    {
        SVA_LOGE("please alloc memory first\n");
        return SAL_FAIL;
    }

    pstVgsOsdPrm->uiOsdNum = 0;

    sal_memset_s(&ailinetype, sizeof(VGS_ARTICLE_LINE_TYPE), 0x00, sizeof(VGS_ARTICLE_LINE_TYPE));
    sal_memset_s(pstVgsOsdPrm, sizeof(VGS_ADD_OSD_PRM), 0x00, sizeof(VGS_ADD_OSD_PRM));

    /* 包裹 */
    uiX = (UINT32)(pstOut->packbagAlert.package_loc.x * (float)uiWith);
    uiY = (UINT32)(pstOut->packbagAlert.package_loc.y * (float)uiHeight);
    uiW = (UINT32)(pstOut->packbagAlert.package_loc.width * (float)uiWith);
    uiH = (UINT32)(pstOut->packbagAlert.package_loc.height * (float)uiHeight);

    alert_num = pstOut->target_num;
    uiConfidence = pstIn->stTargetPrm.enOsdExtType;
    scaleLevel = pstIn->stTargetPrm.scaleLevel;
    /* if (0 == scaleLevel) //抓图使用默认字体大小 */
    {
        scaleLevel = 1;
    }

    /* truetype点阵大小和字体大小保持一致，hzk16点阵大小固定为16 */
    if (OSD_FONT_TRUETYPE == enFontType)
    {
        u32LatSize = osd_func_getFontSize(scaleLevel);;
        u32FontSize = u32LatSize;
    }
    else
    {
        u32LatSize = 16;
        u32FontSize = scaleLevel * 16;
    }

    (VOID)osd_func_readStart();

    for (j = 0; j < alert_num; j++)
    {
        /* 报警物 */
        /*******************************************************************************/
        /* 目标框体已经换算成vpss通道的分辨率上，此处需要换算回检测数据上的坐标 */
        /*******************************************************************************/
        uiX = (UINT32)(pstOut->target[j].rect.x * (float)uiWith);
        uiY = (UINT32)(pstOut->target[j].rect.y * (float)uiHeight);
        uiW = (UINT32)(pstOut->target[j].rect.width * (float)uiWith);
        uiH = (UINT32)(pstOut->target[j].rect.height * (float)uiHeight);

        uiX = (uiX + 1) / 2 * 2;
        uiY = (uiY + 1) / 2 * 2;
        uiW = (uiW + 1) / 2 * 2;
        uiH = (uiH + 1) / 2 * 2;

        uiColor = pstOut->target[j].color;
        indx = pstOut->target[j].type;
        reliability = pstOut->target[j].visual_confidence;

        reliability = (reliability >= 100) ? (99) : (reliability);

        if (uiW != 0 && uiH != 0)
        {
            pstCpuRect = pstCpuRectArray->astRect + pstCpuRectArray->u32RectNum++;
            pstCpuRect->s32X = uiX;
            pstCpuRect->s32Y = uiY;
            pstCpuRect->u32Width = uiW;
            pstCpuRect->u32Height = uiH;
            pstCpuRect->f32StartXFract = -1;
            pstCpuRect->f32EndXFract = -1;
            pstCpuRect->u32Color = uiColor;
            pstCpuRect->u32Thick = u32Thick;
            memcpy(&pstCpuRect->stLinePrm, &ailinetype.ailine, sizeof(ailinetype.ailine));

            if (scaleLevel == 2)
            {
                yOffset = 16 * 2 + 2;
            }
            else if (scaleLevel == 3)
            {
                yOffset = 16 * 3 + 2;
            }
            else if (scaleLevel == 4)
            {
                yOffset = 16 * 4 + 2;
            }
            else
            {
                yOffset = 18;
            }

            /* 填充OSD参数 */
            pstOsd = &pstVgsOsdPrm->stVgsOsdPrm[pstVgsOsdPrm->uiOsdNum];
            if (NULL == (pstOsdRgn = osd_func_getFontRegion(OSD_BLOCK_IDX_STRING, indx, u32LatSize, u32FontSize, SAL_FALSE)))
            {
                continue;
            }

            SAL_RGB24ToARGB1555(uiColor, &uiColor, 0);
            u32BgColor = OSD_BACK_COLOR;
            pstOsd->u64PhyAddr = pstOsdRgn->stAddr.u64PhyAddr;
            pstOsd->enPixelFmt = SAL_VIDEO_DATFMT_ARGB16_1555;
            pstOsd->s32X = uiX;
            pstOsd->s32Y = uiY > yOffset ? (uiY - yOffset) : (uiY + 8);
            pstOsd->u32Width = pstOsdRgn->u32Width;
            pstOsd->u32Height = pstOsdRgn->u32Height;
            pstOsd->u32BgColor = u32BgColor;
            pstOsd->u32BgAlpha = 255;
            pstOsd->u32FgAlpha = 0;
            pstOsd->u32Stride = pstOsdRgn->u32Stride;
            pstVgsOsdPrm->uiOsdNum++;

            if (uiConfidence == SAL_TRUE)
            {
                if (NULL == (pstNumLatRgn = osd_func_getNumLatRegion(OSD_BLOCK_IDX_NUM_PAREN, reliability, 0, u32LatSize)))
                {
                    continue;
                }

                if (SAL_SOK != osd_func_FillFont(pstNumLatRgn, (OSD_FONT_TRUETYPE == enFontType) ? scaleLevel * 16 : 16,
                                                 pstNumFontRgn, scaleLevel * 16, uiColor, u32BgColor))
                {
                    continue;
                }

                pstNumOsd = pstVgsOsdPrm->stVgsOsdPrm + pstVgsOsdPrm->uiOsdNum++;
                pstNumOsd->u64PhyAddr = pstNumFontRgn->stAddr.u64PhyAddr;
                pstNumOsd->enPixelFmt = SAL_VIDEO_DATFMT_ARGB16_1555;
                pstNumOsd->s32X = pstOsd->s32X + pstOsd->u32Width;
                pstNumOsd->s32Y = pstOsd->s32Y;
                pstNumOsd->u32Width = pstNumFontRgn->u32Width;
                pstNumOsd->u32Height = pstNumFontRgn->u32Height;
                pstNumOsd->u32BgColor = u32BgColor;
                pstNumOsd->u32BgAlpha = 255;
                pstNumOsd->u32FgAlpha = 0;
                pstNumOsd->u32Stride = pstNumFontRgn->u32Stride;
                pstNumFontRgn++;
            }
        }
    }

    vgs_func_drawRectArraySoft(&salVideoFrame, pstCpuRectArray, SAL_TRUE, SAL_TRUE);
    vgs_hal_drawLineOSDArray(pstJpegFrame, pstVgsOsdPrm, NULL);

    (VOID)osd_func_readEnd();

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_HalGetDbgLevel
* 描  述  :
* 输  入  : - void:
* 输  出  : 无
* 返回值  : sva模块的调试打印级别
*******************************************************************************/
UINT32 Sva_HalGetDbgLevel(void)
{
    return svaDbLevel;
}

/*******************************************************************************
* 函数名  : Sva_HalSetDbLevel
* 描  述  : 设置日志打印级别
* 输  入  : - level: 日志级别
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
void Sva_HalSetDbLevel(UINT32 level)
{
    svaDbLevel = (level >= DEBUG_LEVEL_BUll) ? DEBUG_LEVEL4 : level;
    SVA_LOGI("sva debugLevel %d\n", svaDbLevel);

    return;
}

/*******************************************************************************
* 函数名  : Sva_HalSetDbgCnt
* 描  述  : 设置框滞留时间(算法内部使用)
* 输  入  : - level: 日志级别
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
void Sva_HalSetDbgCnt(UINT32 dbgCnt)
{
    svaDbgCnt = (dbgCnt > 0) ? dbgCnt : 30;
    SVA_LOGI("sva debugCnt %d\n", svaDbgCnt);

    return;
}

/*******************************************************************************
* 函数名  : Sva_HalPrintCfgInfo
* 描  述  : 异常返回时调试打印
* 输  入  : 无
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
void Sva_HalPrintCfgInfo(VOID)
{
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 size = 10 * 1024;
    UINT32 ret = 0;
    UINT32 uiCfgSize = 0;

    FILE *fp = NULL;
    CHAR *pCfgFile = NULL;
    INT8 *pTmpBuf = NULL;

    if (NULL == pTmpBuf)
    {
        pTmpBuf = SAL_memMalloc(size, "SVA", "sva_debug_print");
        if (NULL == pTmpBuf)
        {
            SVA_LOGE("malloc failed! \n");
            goto EXIT;
        }
    }

    for (i = 0; i < SVA_CFG_FILE_NUM; i++)
    {
        sal_memset_s(pTmpBuf, size, 0x00, size);
        pCfgFile = g_cfg_path[i];

        fp = fopen(pCfgFile, "rb");
        if (NULL == fp)
        {
            SVA_LOGE("fopen failed! i %d, cfg %s, error %s \n", i, pCfgFile, strerror(errno));
            continue;
        }

        if (0 != fseek(fp, 0L, SEEK_END))
        {
            SVA_LOGE("fseek failed! \n");
            if (NULL != fp)
            {
                fclose(fp);
                fp = NULL;
            }

            continue;
        }

        uiCfgSize = ftell(fp);
        if (0 == uiCfgSize)
        {
            SVA_LOGE("ftell failed! \n");
            if (NULL != fp)
            {
                fclose(fp);
                fp = NULL;
            }

            continue;
        }

        uiCfgSize = uiCfgSize > size ? size : uiCfgSize;

        if (0 != fseek(fp, 0L, SEEK_SET))
        {
            SVA_LOGE("fseek failed! \n");
            if (NULL != fp)
            {
                fclose(fp);
                fp = NULL;
            }

            continue;
        }

        ret = fread(pTmpBuf, 1, uiCfgSize, fp);
        if (0 == ret || 0 != feof(fp))
        {
            SVA_LOGE("fread err! \n");
            if (NULL != fp)
            {
                fclose(fp);
                fp = NULL;
            }

            continue;
        }

        if (NULL != fp)
        {
            fclose(fp);
            fp = NULL;
        }

        SVA_LOGI("===============================================================\n");
        SVA_LOGI("Sva_Dbg: i %d, cfg %s \n", i, pCfgFile);
        SVA_LOGI("===============================================================\n");

        for (j = 0; j < uiCfgSize; j++)
        {
            printf("%c", pTmpBuf[j]);
        }

        printf("\n");

    }

EXIT:
    if (NULL != pTmpBuf)
    {
        SAL_memfree(pTmpBuf, "SVA", "sva_debug_print");
        pTmpBuf = NULL;
    }

    return;
}

/*******************************************************************************
* 函数名  : Sva_HalGetAlgDbgFlag
* 描  述  : 获取算法调试开关
* 输  入  : 无
* 输  出  : 无
* 返回值  : 成功: SAL_SOK
            失败: SAL_FAIL
*******************************************************************************/
UINT32 Sva_HalGetAlgDbgFlag(void)
{
    return uiSvaAlgDbgFlag;
}

/*******************************************************************************
* 函数名  : Sva_HalSetAlgDbgFlag
* 描  述  : 设置算法调试开关
* 输  入  : UINT32 : uiFlag
* 输  出  : 无
* 返回值  : 成功: SAL_SOK
            失败: SAL_FAIL
* 说  明  : 该接口用于调试，入参有效性部分由外部保证，故此处不进行严格校验
*******************************************************************************/
void Sva_HalSetAlgDbgFlag(UINT32 uiFlag)
{
    uiSvaAlgDbgFlag = uiFlag > SAL_TRUE ? SAL_TRUE : uiFlag;

    SVA_LOGI("Set Alg Dbg Flag %d End! \n", uiSvaAlgDbgFlag);
    return;
}

/*******************************************************************************
* 函数名  : Sva_HalSetAlgDbgLevel
* 描  述  : 设置算法调试等级
* 输  入  : UINT32 : uiDbgLevel
* 输  出  : 无
* 返回值  : 成功: SAL_SOK
            失败: SAL_FAIL
*******************************************************************************/
void Sva_HalSetAlgDbgLevel(UINT32 uiDbgLevel)
{
    uiSvaAlgDbgLevel = uiDbgLevel > 100 ? 100 : uiDbgLevel;

    SVA_LOGI("Set Alg Dbg Level %d End! \n", uiSvaAlgDbgLevel);
    return;
}

/**
 * @function:   Sva_HalDebugDumpData
 * @brief:      sva模块dump数据接口
 * @param[in]:  INT8 *pData
 * @param[in]:  INT8 *pPath
 * @param[in]:  UINT32 uiSize
 * @param[out]: None
 * @return:     void
 */
void Sva_HalDebugDumpData(CHAR *pData, CHAR *pPath, UINT32 uiSize, UINT32 uiFileCtl)
{
    INT32 ret = 0;
    FILE *fp = NULL;

    CHAR *pStrFileCtl[] = {"w+", "a+"};

    fp = fopen(pPath, pStrFileCtl[uiFileCtl]);
    if (!fp)
    {
        SVA_LOGE("fopen %s failed! \n", pPath);
        goto exit;
    }

    ret = fwrite(pData, uiSize, 1, fp);
    if (ret < 0)
    {
        SVA_LOGE("fwrite err! \n");
        goto exit;
    }

    fflush(fp);

exit:
    if (NULL != fp)
    {
        fclose(fp);
        fp = NULL;
    }

    return;
}

/*******************************************************************************
* 函数名  : Sva_HalGetCjsonFile
* 描  述  : 获取现有Json文件字符串
* 输  入  : pFileName : Json路径
*
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static cJSON *Sva_HalGetCjsonFile(char *pFileName)
{
    FILE *f = NULL;
    long len = 0;
    char *data = NULL;
    cJSON *ret = NULL;
    int result = 0;

    /*check the filename*/
    if (NULL == pFileName)
    {
        SVA_LOGE("Cjson file is  NULL! \n");
        return NULL;
    }

    /*open file*/
    f = fopen(pFileName, "rb");
    if (NULL == f)
    {
        SVA_LOGE("fopen file %s fail!, error %s \n", pFileName, strerror(errno));
        return NULL;
    }

    /*file seek_end*/
    if (0 != fseek(f, 0, SEEK_END))
    {
        SVA_LOGE("fseek failed! \n");
        if (NULL != f)
        {
            fclose(f);
            f = NULL;
        }

        return NULL;
    }

    /*Get file length*/
    len = ftell(f);
    if (len <= 0)
    {
        SVA_LOGE("file data is null! \n");
        ret = NULL;
        goto EXIT;
    }

    /*file seek_set*/
    if (0 != fseek(f, 0, SEEK_SET))
    {
        SVA_LOGE("fseek failed! \n");
        ret = NULL;
        goto EXIT;
    }

    /*malloc*/
    data = (char *)SAL_memMalloc(len + 1, "SVA", "sva_json_data");
    if (NULL == data)
    {
        SVA_LOGE("Data Malloc ERROR! \n");
        ret = NULL;
        goto EXIT;
    }

    /*read file to data*/
    result = fread(data, 1, len, f);
    if (result == 0 || feof(f))
    {
        SVA_LOGE("fread err! \n");
        ret = NULL;
        goto EXIT;
    }

    data[len] = '\0';

    cJSON *json = NULL;

    json = cJSON_Parse(data);
    if (!json)
    {
        SVA_LOGE("Error before: [%s]\n", cJSON_GetErrorPtr());
        ret = NULL;
        goto EXIT;
    }
    else
    {
        ret = json;
    }

EXIT:
    if (NULL != data)
    {
        SAL_memfree(data, "SVA", "sva_json_data");
        data = NULL;
    }

    if (NULL != f)
    {
        fclose(f);
        f = NULL;
    }

    return ret;
}

/*******************************************************************************
* 函数名  : Sva_HalWriteJsonFile
* 描  述  : 写Json文件
* 输  入  : pFile : Json路径
*            : pData : 需要写入的内容
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 Sva_HalWriteJsonFile(char *pFile, char *pData)
{
    FILE *fp = NULL;
    INT32 ret = SAL_SOK;

    /*check input parameter*/
    if (NULL == pData || NULL == pFile)
    {
        SVA_LOGE("Input Parameter Error!! \n");
        return SAL_FAIL;
    }

    /*open  file*/
    fp = fopen(pFile, "w");
    if (fp == NULL)
    {
        SVA_LOGE("Write Json File ERROR !!filename is %s\n", pFile);
        return SAL_FAIL;
    }

    /*write pData to the fp file*/
    fprintf(fp, "%s", pData);

    if (fp != NULL)
    {
        fclose(fp);
        fp = NULL;
    }

    return ret;
}

/**
 * @function:   Sva_HalGetJsonKeyStr
 * @brief:      获取JSON键值名称
 * @param[in]:  SVA_JSON_MOD_PRM_S *pstSvaJsonPrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_HalGetJsonKeyStr(SVA_JSON_MOD_PRM_S *pstSvaJsonPrm)
{
    CHAR *pcString = NULL;

    SVA_HAL_CHECK_PRM(pstSvaJsonPrm, SAL_FAIL);

    switch (pstSvaJsonPrm->enClassType)
    {
        case SVA_JSON_FIRST_CLASS:
            if (pstSvaJsonPrm->uiClassId > SVA_JSON_CLASS_NUM - 1)
            {
                SVA_LOGE("Invalid Main Class Id %d \n", pstSvaJsonPrm->uiClassId);
                goto exit;
            }

            pcString = g_jSonMainClassKeyTab[pstSvaJsonPrm->uiClassId];
            break;
        case SVA_JSON_SECOND_CLASS:
            if (pstSvaJsonPrm->uiClassId > SVA_JSON_SUB_CLASS_NUM - 1)
            {
                SVA_LOGE("Invalid Sub Class Id %d \n", pstSvaJsonPrm->uiClassId);
                goto exit;
            }

            if (pstSvaJsonPrm->uiClassDevType)
            {
                pcString = g_jSonSubIsmClassKeyTab[pstSvaJsonPrm->uiClassId];
            }
            else
            {
                pcString = g_jSonSubIsaClassKeyTab[pstSvaJsonPrm->uiClassId];
            }

            break;
        default:
            SVA_LOGW("Invalid Main Class Id %d \n", pstSvaJsonPrm->uiClassId);
            break;
    }

    pstSvaJsonPrm->pcVal = pcString;

exit:
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_IsaHalModifyCfgFile
* 描  述  : 修改配置文件中特定配置项
* 输  入  : pUpdataJson     需要修改的json内容结构体
*
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_IsaHalModifyCfgFile(IA_ISA_UPDATA_CFG_JSON_PARA *pstUpJsonPrm)
{
    UINT32 i = 0;
    INT32 uiAiModelNum = 1;      /*AI模型数量，默认为1*/
    INT32 uiObdModelNum = 1;   /*基础模型数量，默认为1*/
    INT32 uiPdModelNum = 1;   /*基础模型数量，默认为1*/
    INT32 uiClsModelNum = 1;   /*分类模型数量，默认为1*/

    cJSON *pstRootPara = NULL;
    cJSON *pstObjItem = NULL;
    cJSON *pstArrItem = NULL;
    CHAR *pstOutPara = NULL;

    SVA_JSON_MOD_PRM_S stSvaJsonPrm = {0};

    SVA_HAL_CHECK_PRM(pstUpJsonPrm, SAL_FAIL);

    /*Get cjson filepath*/
    pstRootPara = Sva_HalGetCjsonFile(Sva_HalGetIcfParaJson(APPPARAM_CFG_PATH));
    SVA_HAL_CHECK_PRM(pstRootPara, SAL_FAIL);

    /*use cJson interface to get the json object data to the outPara(char *),注意:cJSON_Print函数已经释放root内存，后面不需要重复释放*/
    pstOutPara = cJSON_Print(pstRootPara);
    free(pstOutPara);
    pstOutPara = NULL;

    stSvaJsonPrm.enClassType = SVA_JSON_FIRST_CLASS;
    stSvaJsonPrm.uiClassId = SVA_JSON_CLASS_SECURITY_DETECT;
    stSvaJsonPrm.uiClassDevType = SVA_JSON_ISA_CLASS;
    (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);

    cJSON *pstBasicPara = NULL;

    pstBasicPara = cJSON_GetObjectItem(pstRootPara, stSvaJsonPrm.pcVal);
    SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

    stSvaJsonPrm.enClassType = SVA_JSON_SECOND_CLASS;

    switch (pstUpJsonPrm->enChgType)
    {
        /*change Model Type*/
        case CHANGE_WORK_TYPE:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_WORK_TYPE;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            pstObjItem->valuedouble = pstUpJsonPrm->enWorkType;
            SVA_LOGI("Chane Cfg Json WorkType ,Work type is %d \n", pstUpJsonPrm->enWorkType);
            break;

        /*change Model EnableSwitch*/
        case CHANGE_AI_TYPE:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_AI_TYPE;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            pstObjItem->valuedouble = pstUpJsonPrm->uiAiEnable;
            SVA_LOGI("Chane Cfg Json ai_type ,ai_type is %d \n", pstUpJsonPrm->uiAiEnable);
            break;

        case CHANGE_PACK_CONF_THRESH:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_PACK_CONG_TH;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            pstObjItem->valuedouble = pstUpJsonPrm->uiPackConfTh;

            SVA_LOGI("modify packconfth %lf end! \n", pstUpJsonPrm->uiPackConfTh);
            break;

        case CHANGE_PD_GAP:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_PD_GAP;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            pstObjItem->valuedouble = pstUpJsonPrm->uiPackGap;

            SVA_LOGI("modify packgap %d end! \n", pstUpJsonPrm->uiPackGap);
            break;

        case CHANGE_SMALL_PACK_THRESH:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_SMALL_PACK_TH;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            pstObjItem->valuedouble = pstUpJsonPrm->uiSmallPackTh;

            SVA_LOGI("modify smallpackth %lf end! \n", pstUpJsonPrm->uiSmallPackTh);
            break;

        case CHANGE_PD_SPLIT_THRESH:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_PD_SPLIT_THRESH;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            pstObjItem->valuedouble = pstUpJsonPrm->uiPdSplitTh;

            SVA_LOGI("modify Pd split th %d end! \n", pstUpJsonPrm->uiPdSplitTh);
            break;

        case CHANGE_CLS_BATCH_NUM:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_CLS_BATCH_NUM;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            pstObjItem->valuedouble = pstUpJsonPrm->uiClsBatchNum;

            SVA_LOGI("modify cls batch num %d end! \n", pstUpJsonPrm->uiClsBatchNum);
            break;

        /*change AI Model Path*/
        case CHANGE_MAIN_AI_MODEL_PATH:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_MAIN_AI_MODEL_NUM;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            uiAiModelNum = pstObjItem->valueint;

            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_MAIN_AI_MODEL_PATH;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            for (i = 0; i < uiAiModelNum; i++)
            {
                pstArrItem = cJSON_GetArrayItem(pstObjItem, i);
                SVA_HAL_CHECK_PRM(pstArrItem, SAL_FAIL);

                pstArrItem->valuestring = pstUpJsonPrm->pcAiModelPath[0];

                SVA_LOGI("Chane Cfg Json main Ai Model Path, ai_model_num(%d). i %d, Model Path is %s \n", \
                         uiAiModelNum, i, pstUpJsonPrm->pcAiModelPath[0]);
            }

            break;

        /*change AI Model Path*/
        case CHANGE_SIDE_AI_MODEL_PATH:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_SIDE_AI_MODEL_NUM;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            uiAiModelNum = pstObjItem->valueint;

            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_SIDE_AI_MODEL_PATH;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            for (i = 0; i < uiAiModelNum; i++)
            {
                pstArrItem = cJSON_GetArrayItem(pstObjItem, i);
                SVA_HAL_CHECK_PRM(pstArrItem, SAL_FAIL);

                pstArrItem->valuestring = pstUpJsonPrm->pcAiModelPath[0];

                SVA_LOGI("Chane Cfg Json side Ai Model Path, ai_model_num(%d). i %d, Model Path is %s \n", \
                         uiAiModelNum, i, pstUpJsonPrm->pcAiModelPath[0]);
            }

            break;
        /*change OBD Model Path*/
        case CHANGE_MAIN_OBD_MODEL_PATH:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_MAIN_OBD_MODEL_NUM;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            uiObdModelNum = pstUpJsonPrm->uiObdModelNum ? pstUpJsonPrm->uiObdModelNum : pstObjItem->valueint;
            SVA_LOGI("uiObdModelNum %d \n", uiObdModelNum);

            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_MAIN_OBD_MODEL_PATH;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            for (i = 0; i < uiObdModelNum; i++)
            {
                pstArrItem = cJSON_GetArrayItem(pstObjItem, i);
                SVA_HAL_CHECK_PRM(pstArrItem, SAL_FAIL);

                pstArrItem->valuestring = pstUpJsonPrm->pcObdModelPath[i];

                SVA_LOGI("Chane Cfg Json main obd Model Path, main_model_num(%d). i %d, Model Path is %s \n", \
                         uiObdModelNum, i, pstUpJsonPrm->pcObdModelPath[i]);
            }

            break;
        case CHANGE_SIDE_OBD_MODEL_PATH:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_SIDE_OBD_MODEL_NUM;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            uiObdModelNum = pstUpJsonPrm->uiObdModelNum ? pstUpJsonPrm->uiObdModelNum : pstObjItem->valueint;
            SVA_LOGI("uiObdModelNum %d \n", uiObdModelNum);

            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_SIDE_OBD_MODEL_PATH;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            for (i = 0; i < uiObdModelNum; i++)
            {
                pstArrItem = cJSON_GetArrayItem(pstObjItem, i);
                SVA_HAL_CHECK_PRM(pstArrItem, SAL_FAIL);

                pstArrItem->valuestring = pstUpJsonPrm->pcObdModelPath[i];

                SVA_LOGI("Chane Cfg Json side obd Model Path, main_model_num(%d). i %d, Model Path is %s \n", \
                         uiObdModelNum, i, pstUpJsonPrm->pcObdModelPath[i]);
            }

            break;

        /*change CLS Model Path*/
        case CHANGE_CLS_MODEL_PATH:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_CLS_MODEL_NUM;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            uiClsModelNum = pstUpJsonPrm->uiClsModelNum ? pstUpJsonPrm->uiClsModelNum : pstObjItem->valueint;
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_CLS_MODEL_PATH;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            for (i = 0; i < uiClsModelNum; i++)
            {
                pstArrItem = cJSON_GetArrayItem(pstObjItem, i);
                SVA_HAL_CHECK_PRM(pstArrItem, SAL_FAIL);

                pstArrItem->valuestring = pstUpJsonPrm->pcClsModelPath[i];

                SVA_LOGI("Chane Cfg Json main cls Model Path i %d, %s \n", i, pstUpJsonPrm->pcClsModelPath[i]);
            }

            break;

        /*change PD Model Path*/
        case CHANGE_MAIN_PD_MODEL_PATH:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_MAIN_PD_MODEL_NUM;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            uiPdModelNum = pstUpJsonPrm->uiPdModelNum ? pstUpJsonPrm->uiPdModelNum : pstObjItem->valueint;

            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_MAIN_PD_MODEL_PATH;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            for (i = 0; i < uiPdModelNum; i++)
            {
                pstArrItem = cJSON_GetArrayItem(pstObjItem, i);
                SVA_HAL_CHECK_PRM(pstArrItem, SAL_FAIL);

                pstArrItem->valuestring = pstUpJsonPrm->pcPdModelPath[i];

                SVA_LOGI("Chane Cfg Json main pd Model Path, main_model_num(%d). i %d, Model Path is %s \n", \
                         uiPdModelNum, i, pstUpJsonPrm->pcPdModelPath[i]);
            }

            break;

        /*change PD Model Path*/
        case CHANGE_SIDE_PD_MODEL_PATH:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_SIDE_PD_MODEL_NUM;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            uiPdModelNum = pstUpJsonPrm->uiPdModelNum ? pstUpJsonPrm->uiPdModelNum : pstObjItem->valueint;

            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_SIDE_PD_MODEL_PATH;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            for (i = 0; i < uiPdModelNum; i++)
            {
                pstArrItem = cJSON_GetArrayItem(pstObjItem, i);
                SVA_HAL_CHECK_PRM(pstArrItem, SAL_FAIL);

                pstArrItem->valuestring = pstUpJsonPrm->pcPdModelPath[i];

                SVA_LOGI("Chane Cfg Json side pd Model Path, main_model_num(%d). i %d, Model Path is %s \n", \
                         uiPdModelNum, i, pstUpJsonPrm->pcPdModelPath[i]);
            }

            break;

        /*change Model Type*/
        case CHANGE_POS_WRITE_FLAG:
            stSvaJsonPrm.uiClassId = SVA_JSON_SUB_CLASS_POS_WRITE_FLAG;
            (VOID)Sva_HalGetJsonKeyStr(&stSvaJsonPrm);
            SVA_HAL_CHECK_PRM(stSvaJsonPrm.pcVal, SAL_FAIL);

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stSvaJsonPrm.pcVal);
            SVA_HAL_CHECK_PRM(pstObjItem, SAL_FAIL);

            pstObjItem->valuedouble = pstUpJsonPrm->uiPosWriteFlag;
            SVA_LOGI("Chane Cfg Json Pos Write Flag, Flag is %d \n", pstUpJsonPrm->uiPosWriteFlag);

            break;

        /*default*/
        default:
            SVA_LOGE("Change Type ERROR!!! \n");
            break;
    }

    /*copy the root data to the out,注意:cJSON_Print函数已经释放root内存，后面不需要重复释放*/
    pstOutPara = cJSON_Print(pstRootPara);

    /*write the json file*/
    if (Sva_HalWriteJsonFile(Sva_HalGetIcfParaJson(APPPARAM_CFG_PATH), pstOutPara) != SAL_SOK)
    {
        SVA_LOGE("Write Json  Faile !! \n");
        return SAL_FAIL;
    }

    free(pstOutPara);
    pstOutPara = NULL;

    /* cJSON_Delete(pstRootPara); */
    /* pstRootPara = NULL; */

    return SAL_SOK;
}

/* 将单双能数量转换为数组索引 */
#define SVA_HAL_GET_ENERGY_INDEX(num) (num - 1)

/**
 * @function   Sva_HalGetXrayEnergyNum
 * @brief      获取xray高低能数量
 * @param[in]  VOID
 * @param[out] None
 * @return     static UINT32
 */
static UINT32 Sva_HalGetXrayEnergyNum(VOID)
{
    UINT32 u32XrayEnergyNum = 0;

    DSPINITPARA *pstInit = NULL;

    /* 初始化全局参数 */
    pstInit = SystemPrm_getDspInitPara();
    SVA_HAL_CHECK_RETURN(NULL == pstInit, SAL_FAIL);

    /* 未定义或者异常能量数，默认双能 */
    switch (pstInit->dspCapbPar.xray_energy_num)
    {
        case XRAY_ENERGY_SINGLE:
        {
            u32XrayEnergyNum = XRAY_ENERGY_SINGLE;
            break;
        }
        case XRAY_ENERGY_DUAL:
        {
            u32XrayEnergyNum = XRAY_ENERGY_DUAL;
            break;
        }
        default:
        {
            u32XrayEnergyNum = XRAY_ENERGY_DUAL;
            break;
        }
    }

    SVA_LOGI("get xray energy num: %d \n", u32XrayEnergyNum);

    return u32XrayEnergyNum;
}

/**
 * @function:   Sva_HalSetCfgPrm
 * @brief:      设置安检智能配置参数
 * @param[in]:  cJSON *pstItem
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_HalSetCfgPrm(cJSON *pstItem)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 u32EnergyNum = 0;

    cJSON *pstSubItem = NULL;
    cJSON *pstArrItem = NULL;

    /* OBD和PD模型根据单双能进行区分，在json配置中寻找不同的解析字段 */
    CHAR *pcMainObdModelPath[2] = {"main_single_energy_obd_model_path", "main_double_energy_obd_model_path"};
    CHAR *pcSideObdModelPath[2] = {"side_single_energy_obd_model_path", "side_double_energy_obd_model_path"};
    CHAR *pcMainPdModelPath[2] = {"main_single_energy_pd_model_path", "main_double_energy_pd_model_path"};
    CHAR *pcSidePdModelPath[2] = {"side_single_energy_pd_model_path", "side_double_energy_pd_model_path"};

    IA_ISA_UPDATA_CFG_JSON_PARA stUpJsonPrm = {0};

    SVA_HAL_CHECK_PRM(pstItem, SAL_FAIL);

    /* work_type */
    pstSubItem = cJSON_GetObjectItem(pstItem, "work_type");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    stUpJsonPrm.enWorkType = pstSubItem->valueint;
    stUpJsonPrm.enChgType = CHANGE_WORK_TYPE;
    s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);


    /* ai_type */
    pstSubItem = cJSON_GetObjectItem(pstItem, "ai_type");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    stUpJsonPrm.uiAiEnable = pstSubItem->valueint;
    stUpJsonPrm.enChgType = CHANGE_AI_TYPE;
    s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);

    /* pack_conf_thresh */
    pstSubItem = cJSON_GetObjectItem(pstItem, "pack_conf_thresh");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    stUpJsonPrm.uiPackConfTh = pstSubItem->valuedouble;
    stUpJsonPrm.enChgType = CHANGE_PACK_CONF_THRESH;
    s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);


    /* pd_proc_gap */
    pstSubItem = cJSON_GetObjectItem(pstItem, "pd_proc_gap");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    stUpJsonPrm.uiPackGap = pstSubItem->valueint;
    stUpJsonPrm.enChgType = CHANGE_PD_GAP;
    s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);


    /* small_pack_threshold */
    pstSubItem = cJSON_GetObjectItem(pstItem, "small_pack_threshold");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    stUpJsonPrm.uiSmallPackTh = pstSubItem->valuedouble;
    stUpJsonPrm.enChgType = CHANGE_SMALL_PACK_THRESH;
    s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);


    /* pd_split_thresh */
    pstSubItem = cJSON_GetObjectItem(pstItem, "pd_split_thresh");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    stUpJsonPrm.uiPdSplitTh = pstSubItem->valueint;
    stUpJsonPrm.enChgType = CHANGE_PD_SPLIT_THRESH;
    s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);


    /* cls_batch_num */
    pstSubItem = cJSON_GetObjectItem(pstItem, "cls_batch_num");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    stUpJsonPrm.uiClsBatchNum = pstSubItem->valueint;
    stUpJsonPrm.enChgType = CHANGE_CLS_BATCH_NUM;
    s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);

    /* 高低能数量 */
    u32EnergyNum = Sva_HalGetXrayEnergyNum();

    /* main_obd_model */
    pstSubItem = cJSON_GetObjectItem(pstItem, "main_obd_model_num");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    stUpJsonPrm.uiObdModelNum = pstSubItem->valueint;

    pstSubItem = cJSON_GetObjectItem(pstItem, pcMainObdModelPath[SVA_HAL_GET_ENERGY_INDEX(u32EnergyNum)]);
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    for (i = 0; i < stUpJsonPrm.uiObdModelNum; i++)
    {
        pstArrItem = cJSON_GetArrayItem(pstSubItem, i);
        SVA_HAL_CHECK_PRM(pstArrItem, SAL_FAIL);

        stUpJsonPrm.pcObdModelPath[i] = pstArrItem->valuestring;
    }

    stUpJsonPrm.enChgType = CHANGE_MAIN_OBD_MODEL_PATH;
    s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);


    /* side_obd_model */
    pstSubItem = cJSON_GetObjectItem(pstItem, "side_obd_model_num");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    stUpJsonPrm.uiObdModelNum = pstSubItem->valueint;

    pstSubItem = cJSON_GetObjectItem(pstItem, pcSideObdModelPath[SVA_HAL_GET_ENERGY_INDEX(u32EnergyNum)]);
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    for (i = 0; i < stUpJsonPrm.uiObdModelNum; i++)
    {
        pstArrItem = cJSON_GetArrayItem(pstSubItem, i);
        SVA_HAL_CHECK_PRM(pstArrItem, SAL_FAIL);

        stUpJsonPrm.pcObdModelPath[i] = pstArrItem->valuestring;
    }

    stUpJsonPrm.enChgType = CHANGE_SIDE_OBD_MODEL_PATH;
    s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);

    /* main_pd_model */
    pstSubItem = cJSON_GetObjectItem(pstItem, "main_pd_model_num");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    stUpJsonPrm.uiPdModelNum = pstSubItem->valueint;

    pstSubItem = cJSON_GetObjectItem(pstItem, pcMainPdModelPath[SVA_HAL_GET_ENERGY_INDEX(u32EnergyNum)]);
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    for (i = 0; i < stUpJsonPrm.uiPdModelNum; i++)
    {
        pstArrItem = cJSON_GetArrayItem(pstSubItem, i);
        SVA_HAL_CHECK_PRM(pstArrItem, SAL_FAIL);

        stUpJsonPrm.pcPdModelPath[i] = pstArrItem->valuestring;
    }

    stUpJsonPrm.enChgType = CHANGE_MAIN_PD_MODEL_PATH;
    s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);

    /* side_pd_model */
    /* 当前侧视角使用和主视角一样的算法模型 */
    pstSubItem = cJSON_GetObjectItem(pstItem, "side_pd_model_num");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    stUpJsonPrm.uiPdModelNum = pstSubItem->valueint;

    pstSubItem = cJSON_GetObjectItem(pstItem, pcSidePdModelPath[SVA_HAL_GET_ENERGY_INDEX(u32EnergyNum)]);
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    for (i = 0; i < stUpJsonPrm.uiPdModelNum; i++)
    {
        pstArrItem = cJSON_GetArrayItem(pstSubItem, i);
        SVA_HAL_CHECK_PRM(pstArrItem, SAL_FAIL);

        stUpJsonPrm.pcPdModelPath[i] = pstArrItem->valuestring;
    }

    stUpJsonPrm.enChgType = CHANGE_SIDE_PD_MODEL_PATH;
    s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);


    /* ai_main_model */
    pstSubItem = cJSON_GetObjectItem(pstItem, "ai_main_model_num");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    stUpJsonPrm.uiAiModelNum = pstSubItem->valueint;

    pstSubItem = cJSON_GetObjectItem(pstItem, "ai_main_model_path");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    for (i = 0; i < stUpJsonPrm.uiAiModelNum; i++)
    {
        pstArrItem = cJSON_GetArrayItem(pstSubItem, i);
        SVA_HAL_CHECK_PRM(pstArrItem, SAL_FAIL);

        stUpJsonPrm.pcAiModelPath[i] = pstArrItem->valuestring;
    }

    stUpJsonPrm.enChgType = CHANGE_MAIN_AI_MODEL_PATH;
    s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);

    /* ai_side_model */
    pstSubItem = cJSON_GetObjectItem(pstItem, "ai_side_model_num");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    stUpJsonPrm.uiAiModelNum = pstSubItem->valueint;

    pstSubItem = cJSON_GetObjectItem(pstItem, "ai_side_model_path");
    SVA_HAL_CHECK_PRM(pstSubItem, SAL_FAIL);
    for (i = 0; i < stUpJsonPrm.uiAiModelNum; i++)
    {
        pstArrItem = cJSON_GetArrayItem(pstSubItem, i);
        SVA_HAL_CHECK_PRM(pstArrItem, SAL_FAIL);

        stUpJsonPrm.pcAiModelPath[i] = pstArrItem->valuestring;
    }

    stUpJsonPrm.enChgType = CHANGE_SIDE_AI_MODEL_PATH;
    s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
    SVA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);

    return SAL_SOK;
}

/**
 * @function    Sva_HalPrintGapTimeMs
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
UINT32 Sva_HalPrintGapTimeMs(UINT32 t0, UINT32 t1)
{
    return t1 > t0 ? t1 - t0 : 0;
}

/**
 * @function:   Sva_DrvJudgeOffsetEqualZero
 * @brief:      判断包裹偏移量是否等于0
 * @param[in]:  float fIn
 * @param[out]: None
 * @return:     BOOL
 */
BOOL Sva_DrvJudgeOffsetEqualZero(float fIn)
{
    BOOL bFlag = SAL_FALSE;

    if (fIn > -0.0005 && fIn < 0.0005)
    {
        bFlag = SAL_TRUE;
    }

    return bFlag;
}

/**
 * @function:   Sva_HalJudgeStill
 * @brief:      判断包裹是否静止
 * @param[in]:  UINT32 chan
 * @param[in]:  SVA_PROCESS_OUT *pstOut
 * @param[in]:  BOOL *pStillFlag
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_HalJudgeStill(UINT32 chan, SVA_PROCESS_OUT *pstOut, BOOL *pStillFlag)
{
    SVA_HAL_CHECK_CHAN(chan, SAL_FAIL);

    static UINT32 uiCurIdx[2] = {0};
    static float fData[2][3] = {{0.0}, {0.0}};
    static UINT32 uiFrmNum[2][3] = {{0}, {0}};

    BOOL *pbFpFlag = NULL;

    /* 入参本地化 */
    pbFpFlag = (BOOL *)pStillFlag;

    /* 若偏移量为负值，无需进行正拉判断 */
    if (pstOut->frame_offset <= -0.0005 || pstOut->frame_offset >= 0.0005)
    {
        SVA_LOGD("not still! offset %f, frmNum %d, chan %d \n", pstOut->frame_offset, pstOut->frame_num, chan);
        return SAL_SOK;
    }

    fData[chan][uiCurIdx[chan]] = pstOut->frame_offset;
    uiFrmNum[chan][uiCurIdx[chan]] = pstOut->frame_num;

    if (Sva_DrvJudgeOffsetEqualZero(fData[chan][uiCurIdx[chan]])
        && Sva_DrvJudgeOffsetEqualZero(fData[chan][(uiCurIdx[chan] + 2) % 3])
        && Sva_DrvJudgeOffsetEqualZero(fData[chan][(uiCurIdx[chan] + 1) % 3]))
    {
        *pbFpFlag = SAL_TRUE;
        SVA_LOGD("%d: %f, %d: %f, %d: %f \n",
                 uiFrmNum[chan][0], fData[chan][0], uiFrmNum[chan][1], fData[chan][1], uiFrmNum[chan][2], fData[chan][2]);
    }

    uiCurIdx[chan] = (uiCurIdx[chan] + 1) % 3;
    return SAL_SOK;
}

/**
 * @function    Sva_HalGetIptEngStuckFlag
 * @brief       获取引擎送帧接口是否卡住标记
 * @param[in]
 * @param[out]
 * @return
 */
UINT32 Sva_HalGetIptEngStuckFlag(void)
{
    return gXsiCommon.uiIptEngStuckFlag;
}

/**
 * @function    Sva_HalGetIptEngNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
UINT32 Sva_HalGetIptEngNum(void)
{
    return gXsiCommon.uiInputEngDataNum;
}

/**
 * @function    Sva_HalGetIptEngSuccNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
UINT32 Sva_HalGetIptEngSuccNum(void)
{
    return gXsiCommon.uiInputEngDataSuccNum;
}

/**
 * @function    Sva_HalGetIptEngFailNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
UINT32 Sva_HalGetIptEngFailNum(void)
{
    return gXsiCommon.uiInputEngDataFailNum;
}

/**
 * @function    Sva_HalGetInputEngCnt
 * @brief       获取引擎送帧个数
 * @param[in]
 * @param[out]
 * @return
 */
UINT32 Sva_HalGetInputEngCnt(void)
{
    return gXsiCommon.uiInputEngCnt;
}

/**
 * @function    Sva_HalResetDbgInfo
 * @brief       清空调试信息
 * @param[in]
 * @param[out]
 * @return
 */
VOID Sva_HalResetDbgInfo(void)
{
    gXsiCommon.uiInputEngCnt = 0;
    gXsiCommon.uiIptEngStuckFlag = 0;
    gXsiCommon.uiInputEngDataFailNum = 0;
    gXsiCommon.uiInputEngDataNum = 0;
    gXsiCommon.uiInputEngDataSuccNum = 0;
    gXsiCommon.uiInputEngDataFailNum = 0;
    gXsiCommon.uiInputEngDataNum = 0;
    gXsiCommon.uiInputEngDataSuccNum = 0;

    return;
}

