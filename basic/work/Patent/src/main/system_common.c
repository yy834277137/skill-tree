/**
 * @file   system_common.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  DSP 模块初始化功能模块
 * @author dsp
 * @date   2022/5/7
 * @note
 * @note \n History
   1.日    期: 2022/5/7
     作    者: dsp
     修改历史: 创建文件
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include <sal.h>
#include <platform_hal.h>
#include "dspcommon.h"
#include "system_prm_api.h"
#include "capbility.h"
#include "sys_tsk.h"
#include "dup_tsk_api.h"
#include "link_drv_api.h"

#include "task_ism.h"
#include "capt_tsk_inter.h"
#include "disp_tsk_inter.h"
#include "capt_tsk_api.h"
#include "disp_tsk_api.h"
#include "vdec_tsk_api.h"
#include "venc_tsk_api.h"
#include "audio_tsk_api.h"
#include "recode_tsk_api.h"
#include "jpeg_tsk_api.h"

#include "chip_dual_hal.h"
#include "sva_out.h"
#include "ba_out.h"
#include "ppm_out.h"
#include "face_out.h"

#line __LINE__ "system_common.c"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */


/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */

#ifndef SVA_NO_USE
static INST_CFG_S stVpInstCfg =
{
    .szInstPreName = "DUP_VPROC",
    .u32NodeNum = 5,
    .stNodeCfg =
    {
        {IN_NODE_0, "in_0", NODE_BIND_TYPE_SEND},
        {OUT_NODE_0, "out_0", NODE_BIND_TYPE_GET},
        {OUT_NODE_1, "out_1", NODE_BIND_TYPE_GET},
        {OUT_NODE_2, "out_2", NODE_BIND_TYPE_GET},
        {OUT_NODE_3, "out_3", NODE_BIND_TYPE_GET},
    },
};

static DupHandle VpHandle[MAX_CAPT_CHAN] = {0};
static DupHandle VpPpmHandle[MAX_CAPT_CHAN] = {0};

#endif

DupHandle dupHandle1[MAX_XRAY_CHAN];
DupHandle dupHandle2[MAX_XRAY_CHAN];

extern INST_CFG_S stVpDupInstCfg;

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/**
 * @function   System_GetBoardType
 * @brief      system 根据CPLD信息获取主板型号

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_GetBoardType()
{
    UINT32 u32Board = 0;

    if (0 == (u32Board = HARDWARE_GetBoardType()))
    {
        SYS_LOGE("get board type fail\n");
        return SAL_FAIL;
    }

    SAL_LOGI("log:Board type 0x%x\n", u32Board);
    return u32Board;
}

#ifndef SVA_NO_USE

/**
 * @function:   System_commonInitTransDev
 * @brief:      初始化片间传输接口
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 System_commonInitTransDev(VOID)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 uiType = 0;
    UINT32 u32BoardType = 0;

    if (SAL_TRUE != IA_GetModTransFlag(IA_MOD_SVA)
        && SAL_TRUE != IA_GetModTransFlag(IA_MOD_BA)
        && SAL_TRUE != IA_GetModTransFlag(IA_MOD_FACE)
        && SAL_TRUE != IA_GetModTransFlag(IA_MOD_PPM))
    {
        SAL_LOGI("no need init trans device! \n");
        return SAL_SOK;
    }

#ifdef USE_XTRANS
    s32Ret = xtrans_Init();
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("xtrans init end! \n");
        return SAL_FAIL;
    }

    SYS_LOGI("use xtrans, no need open trans device! return success! \n");
    return SAL_SOK;
#endif

    /* TODO: 与BSP约定通过特定接口确定片间传输使用何种方式，兼容不同底板，当前默认USB。
       当前调试PCIe时可以临时将下述uiType修改为1，后续基线使用读取clpd进行判断硬件能力 */

    u32BoardType = System_GetBoardType();
    if (DB_RS20011_V1_0 == u32BoardType || DB_RS20016_V1_0 == u32BoardType || 0x4005 == u32BoardType
        || DB_RS20016_V1_1 == u32BoardType || DB_RS20016_V1_1_F303 == u32BoardType || DB_TS3637_V1_0 == u32BoardType)
    {
        uiType = 0x1;   /* PCIE Transmit Type */
    }
    else if (DB_RS20001_V1_1 == u32BoardType || DB_DS50019_V1_0 == u32BoardType || DB_DS8255_V1_0 == u32BoardType) /* 安检机分析仪默认用USB通信 */
    {
        uiType = 0x0;   /* USB Transmit Type */
    }
    else
    {
        SAL_ERROR("BoardType %x not match! \n", u32BoardType);
    }

    uiType = 0x1;  /* debug: use pcie type and dbg */

    s32Ret = Trans_HalCmdProc(TRANS_HAL_REGISTER, &uiType);
    if (s32Ret < 0)
    {
        SAL_ERROR("ioctl error! ret: %d \n", s32Ret);
        goto err;
    }

    s32Ret = Trans_HalCmdProc(TRANS_HAL_INIT_DEV, NULL);
    if (s32Ret < 0)
    {
        SAL_ERROR("ioctl error! ret: %d \n", s32Ret);
        /* goto err;            / *从片没有接入pcie初始化失败可以正常调试运行同步R7* / */
    }

    return SAL_SOK;

err:
    s32Ret = Trans_HalCmdProc(TRANS_HAL_DEINIT_DEV, NULL);
    if (s32Ret < 0)
    {
        SAL_ERROR("ioctl error! ret: %d \n", s32Ret);
        goto err;
    }

    s32Ret = Trans_HalCmdProc(TRANS_HAL_UNREGISTER, NULL);
    if (s32Ret < 0)
    {
        SAL_ERROR("ioctl error! ret: %d \n", s32Ret);
        goto err;
    }

    return SAL_FAIL;
}

#endif

/**
 * @function   System_commonAudioInit
 * @brief      system 通用模块 音频的初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonAudioInit()
{
    /* 音频模块初始化 */
    if (SAL_SOK != audio_tsk_init())
    {
        SYS_LOGE("audio_tsk_init err !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   System_commonAudioDeInit
 * @brief      system 通用模块 音频的去初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonAudioDeInit()
{
    return SAL_SOK;
}

/**
 * @function   System_commonVdecInit
 * @brief      system 通用模块解码,转封装的初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonVdecInit()
{
    /* 解码解包模块初始化 */
    if (SAL_SOK != vdec_tsk_ModuleCreate())
    {
        SYS_LOGE("VDEC_moduleCreate.\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != recode_tskModuleCreate())
    {
        SYS_LOGE("Recode_drvModuleCreate.\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   System_commonVdecDeInit
 * @brief      system 通用模块解码,转封装的去初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonVdecDeInit()
{
    return SAL_SOK;
}

/**
 * @function   System_commonIaVprocInit
 * @brief      视频处理模块的初始化，这里主要用在分析仪-
               里做镜像功能，目前主要用在包裹抓图

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonIaVprocInit()
{
#ifndef SVA_NO_USE
    UINT32 i = 0;
    UINT32 j = 0;
    INST_INFO_S *pstInstInfo = NULL;
    VOID *pNodeHandle = NULL;
    VOID *pAutoTestNodeHandle = NULL;

    CHAR acTmpName[NAME_LEN] = {'\0'};
    HAL_VPSS_GRP_PRM stSupportGrpPrm = {0};

    DSPINITPARA *pDspInitPara = SystemPrm_getDspInitPara();
    CAPB_VPROC *capb_vproc = capb_get_vproc();

    for (i = 0; i < pDspInitPara->stViInitInfoSt.uiViChn; i++)
    {
        sal_memset_s(&stSupportGrpPrm, sizeof(HAL_VPSS_GRP_PRM), 0x00, sizeof(HAL_VPSS_GRP_PRM));

        for (j = 0; j < DUP_VPSS_CHN_MAX_NUM; j++)
        {
            stSupportGrpPrm.uiChnEnable[j] = (SAL_TRUE != IA_GetModEnableFlag(IA_MOD_PPM) \
                                              ? capb_vproc->vpssGrp.uiChnEnable[j] \
                                              : ENABLED);
        }

        /* 当前人包和违禁品检测不会在同一个芯片上运行 */
        if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_SVA))
        {
            /* 当前每个采集通道对应存在一个vp group，物理通道0用于sva镜像处理，另外三个物理通道未启用 */
            if (SAL_SOK != dup_task_vpDupCreate(&stVpDupInstCfg, i, &VpHandle[i]))
            {
                SYS_LOGE("vproc dup create failed! i %d \n", i);
                return SAL_FAIL;
            }
        }
        if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_PPM))
        {
            /* 人包三目相机分辨率的特殊处理  */
            if (SAL_SOK != dup_task_PpmvpDupCreate(&stVpInstCfg, i, &VpPpmHandle[i]))
            {
                SYS_LOGE("vproc dup create failed! i %d \n", i);
                return SAL_FAIL;
            }
        }

        if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_SVA))
        {
            memset(acTmpName, 0x00, NAME_LEN);
            snprintf(acTmpName, NAME_LEN, "%s_%d", "DUP_VP", i);
            pstInstInfo = link_drvGetInst(acTmpName);
            if (NULL == pstInstInfo)
            {
                SYS_LOGE("pstInstInfo == null! \n");
                return SAL_FAIL;
            }

            /* sva默认使用out_0实现对从左到右过包图像镜像 */
            pNodeHandle = link_drvGetHandleFromNode(pstInstInfo, "out_jpeg");
            if (NULL == pNodeHandle)
            {
                SYS_LOGE("pNodeHandle == null! \n");
                return SAL_FAIL;
            }

            if (SAL_SOK != Sva_tskSetMirrorChnPrm(i, pNodeHandle))
            {
                SYS_LOGE("error \n");
                return SAL_FAIL;
            }

            memset(acTmpName, 0x00, NAME_LEN);
            snprintf(acTmpName, NAME_LEN, "%s_%d", "DUP_VP", 0);
            pstInstInfo = link_drvGetInst(acTmpName);
            if (NULL == pstInstInfo)
            {
                SYS_LOGE("pstInstInfo == null! \n");
                return SAL_FAIL;
            }

            pAutoTestNodeHandle = link_drvGetHandleFromNode(pstInstInfo, "out_yuv");
            if (NULL == pAutoTestNodeHandle)
            {
                SYS_LOGE("pNodeHandle == null! \n");
                return SAL_FAIL;
            }

			if (SAL_SOK != Sva_tskSetScaleChnPrm(i, pAutoTestNodeHandle))
			{
				SYS_LOGE("error \n");
				return SAL_FAIL;
			}
        }
    }

    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_PPM))
    {
        memset(acTmpName, 0x00, NAME_LEN);
        snprintf(acTmpName, NAME_LEN, "%s_%d", "DUP_VPROC", 0);   /* 当前人包关联仅有一路，故此处仅使用vproc_0 */
        pstInstInfo = link_drvGetInst(acTmpName);
        if (NULL == pstInstInfo)
        {
            SYS_LOGE("pstInstInfo == null! \n");
            return SAL_FAIL;
        }

        /* ppm模块使用vproc实现裁剪和放缩的功能 */
        for (j = 0; j < 4; j++)
        {
            memset(acTmpName, 0x00, NAME_LEN);
            snprintf(acTmpName, NAME_LEN, "out_%d", j);
            pNodeHandle = link_drvGetHandleFromNode(pstInstInfo, acTmpName);
            if (NULL == pNodeHandle)
            {
                SYS_LOGE("pNodeHandle == null! \n");
                return SAL_FAIL;
            }

            if (SAL_SOK != Ppm_TskSetVprocHandle(0, j, pNodeHandle))
            {
                SYS_LOGE("set vproc handle failed! i %d, j %d \n", i, j);
                return SAL_FAIL;
            }
        }
    }

#endif
    return SAL_SOK;
}

/**
 * @function   System_commonVideoInitOneChn
 * @brief      system 通用模块 视频通道的初始化
 * @param[in]  UINT32 uiChn  媒体通道号
 * @param[out] None
 * @return     static INT32 HIK_SOK  : 成功
 *                          HIK_FAIL : 失败
 */
static INT32 System_commonVideoInitOneChn(UINT32 uiChn)
{
    CaptHandle captHandle;
    CAPT_ChanHandle *pCaptOutChnHandle = NULL;
    CAPT_MOD_CREATE_PRM stCaptCreatePrm;
    CAPT_ChanCreate stCaptChnCreatePrm;
    CAPT_NOSIGNAL_INFO_ST stNoSignalInfo;
    DUP_MOD_CREATE_PRM stDupCreatePrm;
    UINT32 i = 0;

    /* 1. 创建采集模块 */
    if (VIDEO_INPUT_OUTSIDE == capb_get_product()->enInputType)
    {
        memset(&stCaptCreatePrm, 0, sizeof(CAPT_MOD_CREATE_PRM));

        if (SAL_SOK == SystemPrm_getSysVideoFormat())
        {
            stCaptCreatePrm.stCaptCreateAttr.stCaptChnPicAttr.uiCaptFps = 25;
        }
        else
        {
            stCaptCreatePrm.stCaptCreateAttr.stCaptChnPicAttr.uiCaptFps = 30;
        }

        stCaptCreatePrm.stCaptCreateAttr.stCaptChnPicAttr.uiCaptHei = 1080;
        stCaptCreatePrm.stCaptCreateAttr.stCaptChnPicAttr.uiCaptWid = 1920;

        memset(&stNoSignalInfo, 0, sizeof(CAPT_NOSIGNAL_INFO_ST));
        if (SAL_SOK == SystemPrm_getNoSignalInfo(&stNoSignalInfo))
        {
            for (i = 0; i < MAX_NO_SIGNAL_PIC_CNT; i++)
            {
                stCaptCreatePrm.stCaptCreateAttr.stCaptChnNoSignalAttr.uiNoSignalPicW[i] = stNoSignalInfo.uiNoSignalImgW[i];
                stCaptCreatePrm.stCaptCreateAttr.stCaptChnNoSignalAttr.uiNoSignalPicH[i] = stNoSignalInfo.uiNoSignalImgH[i];
                stCaptCreatePrm.stCaptCreateAttr.stCaptChnNoSignalAttr.pucNoSignalPicAddr[i] = (PUINT8)stNoSignalInfo.uiNoSignalImgAddr[i];
                stCaptCreatePrm.stCaptCreateAttr.stCaptChnNoSignalAttr.uiNoSignalPicSize[i] = stNoSignalInfo.uiNoSignalImgSize[i];
            }
        }
        else
        {
            for (i = 0; i < MAX_NO_SIGNAL_PIC_CNT; i++)
            {
                stCaptCreatePrm.stCaptCreateAttr.stCaptChnNoSignalAttr.uiNoSignalPicW[i] = stCaptCreatePrm.stCaptCreateAttr.stCaptChnPicAttr.uiCaptHei;
                stCaptCreatePrm.stCaptCreateAttr.stCaptChnNoSignalAttr.uiNoSignalPicH[i] = stCaptCreatePrm.stCaptCreateAttr.stCaptChnPicAttr.uiCaptHei;
                stCaptCreatePrm.stCaptCreateAttr.stCaptChnNoSignalAttr.pucNoSignalPicAddr[i] = NULL;
                stCaptCreatePrm.stCaptCreateAttr.stCaptChnNoSignalAttr.uiNoSignalPicSize[i] = 0;
            }
        }

        if (SAL_SOK != capt_tsk_moduleCreate(&stCaptCreatePrm, &captHandle))
        {
            SYS_LOGE("Capt Module Create Failed !!!\n");
            return SAL_FAIL;
        }

        /* 创建采集模块输出通道 */
        memset(&stCaptChnCreatePrm, 0, sizeof(CAPT_ChanCreate));
        stCaptChnCreatePrm.module = captHandle;

        if (SAL_SOK != capt_tsk_askCreateChan(&stCaptChnCreatePrm, &pCaptOutChnHandle))
        {
            SYS_LOGE("Capt Ask Chn Failed !!!\n");
            return SAL_FAIL;
        }

        pCaptOutChnHandle->pfrontDupHandle = &dupHandle1[uiChn];
        pCaptOutChnHandle->prearDupHandle = &dupHandle2[uiChn];
    }

    /*****************************************************************************
                                创建 dup 模块 0
                                分别给  jpeg0     copy
                                         venc0-0   bind
                                         venc0-1   bind
                                         disp0     copy
    *****************************************************************************/
    /* 2. 创建采集关联dup模块 */
    memset(&stDupCreatePrm, 0, sizeof(DUP_MOD_CREATE_PRM));
    stDupCreatePrm.captHandle = (PhysAddr)pCaptOutChnHandle;
    if (SAL_SOK != dup_task_multDupCreate(&stDupCreatePrm, uiChn, &dupHandle1[uiChn], &dupHandle2[uiChn]))
    {
        SYS_LOGE("Dup Module Create Failed !!!\n");
        return SAL_FAIL;
    }

#ifndef SVA_NO_USE
    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_SVA))
    {
        if (SAL_SOK != Sva_tskDevInit(uiChn))
        {
            SYS_LOGE("Sva_tskDevInit failed! \n");
            return SAL_FAIL;
        }
    }

#endif

    SYS_LOGD("Media Chn %d Created End !!!\n", uiChn);

    return SAL_SOK;
}

/**
 * @function   System_commonSlaveVideoInit
 * @brief      system 通用模块 从片视频的初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonSlaveVideoInit()
{
#ifndef SVA_NO_USE
    DSPINITPARA *pDspInitPara = SystemPrm_getDspInitPara();

    /************ 抓图 ***************/
    if (SAL_SOK != jpeg_tsk_init())
    {
        SYS_LOGE("jpeg_tsk_init err !!!\n");
        return SAL_FAIL;
    }

    /*
        智能配置文件说明:
        为了兼容安检机和分析仪之间算法配置的差异，如模型分辨率和模型个数等，新增智能模块配置文件。
        项目中目前仅有安检智能子模块sva支持配置，其他子模块ba、face暂不支持配置，若有需要可以添加。

        使用说明:
        目前配置文件深度为4级。当前主要使用的是算法相关信息，alg_info子集中包含的是智能子模块具体信息。
        各模块间配置参数可以不相同，具体参考下述树状图描述。

        注意点:
        在初始化算法配置前，需要先将各模块内实现修改算法配置文件的接口注册到通用模块中，否则应用层的算法配置不会生效。

        Root
       |-----alg_info
     |    |----------sva
     |    |           |-------width
     |    |           |-------height
     |    |           |-------model_num
     |    |           |  ...
     |    |           |-------model_path
     |    |
     |    |----------ba
     |    |           |-------...
     |    |
     |    |----------face
     |    |           |-------...
     |    |
     |    |-----misc_info
     |    |----------...
           ...
     |
     */

    /* 智能子模块注册回调用于初始化相关配置，目前仅有安检智能支持配置 */
    IA_RegModCfgCbFunc(IA_MOD_SVA, (CFG_CB_FUNC)Sva_HalSetCfgPrm);
    /* IA_RegModCfgCbFunc(IA_MOD_BA, (CFG_CB_FUNC)Ba_HalSetCfgPrm); */
    /* IA_RegModCfgCbFunc(IA_MOD_FACE, (CFG_CB_FUNC)Face_HalSetCfgPrm); */

    /* 初始化配置文件名称，此处根据输入路数进行区分，写得不好，后续优化 */
    if (SAL_SOK != IA_InitCfgPath())
    {
        SYS_LOGE("IA_InitCfgPath err !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != IA_InitCfgPrm())
    {
        SYS_LOGE("IA_InitCfgPrm err !!!\n");
        goto err;
    }
	
#ifdef ISM_DUAL_CHIP_TRANS
    if (SAL_SOK != System_commonInitTransDev())
    {
        SYS_LOGE("init trans dev failed! \n");
        goto err;
    }
#endif

    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_SVA))
    {
        if (SAL_SOK != Sva_tskInit())
        {
            SYS_LOGE("Sva_tskInit err !!!\n");
            goto err;
        }

        UINT32 i = 0;

        for (i = 0; i < pDspInitPara->stViInitInfoSt.uiViChn; i++)
        {
            if (SAL_SOK != Sva_tskDevInit(i))
            {
                SYS_LOGE("Sva_tskDevInit failed! i %d \n", i);
                goto err;
            }
        }
    }

	/* fixme: 解码转包相关业务的初始化，当前从片仅有人包使用到，后续若需要可以拿到外面 */
	if (SAL_SOK != System_commonVdecInit())
	{
		SYS_LOGE("System_commonVdecInit err !!!\n");
		return SAL_FAIL;
	}

    /* 人包关联从片设备需要启用PPM */
    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_PPM))
    {
        if (SAL_SOK != Ppm_TskInit())
        {
            SYS_LOGE("Ppm_TskInit err !!!\n");
            return SAL_FAIL;
        }

        SYS_LOGI("ppm tsk init end!!!\n");
    }

    /********IPC 行为分析 ************/
    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_BA))
    {
        if (SAL_SOK != Ba_tskInit())
        {
            SYS_LOGE("Ba_tskInit err !!!\n");
            return SAL_FAIL;
        }
    }

    /********智能人脸 ************/
    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_FACE))
    {
        if (SAL_SOK != Face_tskInit())
        {
            SYS_LOGE("Face_tskInit err !!!\n");
            return SAL_FAIL;
        }
    }
#endif

    if (SAL_SOK != System_commonIaVprocInit())
    {
        SYS_LOGE("Video Process Init Failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;

#ifndef SVA_NO_USE
err:
    return SAL_FAIL;
#endif
}

/**
 * @function   System_commonVideoInit
 * @brief      system 通用模块 视频的初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonVideoInit()
{
    UINT32 i = 0;

    DSPINITPARA *pDspInitPara = SystemPrm_getDspInitPara();

    for (i = 0; i < pDspInitPara->stViInitInfoSt.uiViChn; i++)
    {
        if (i > MAX_CAPT_CHAN)
        {
            SYS_LOGE("Invalid init prm! vi_chn(%d) > Max(%d) \n", pDspInitPara->stViInitInfoSt.uiViChn, MAX_CAPT_CHAN);
            return SAL_FAIL;
        }

        if (SAL_SOK != System_commonVideoInitOneChn(i))
        {
            SYS_LOGE("Media Chn %d Init Failed! \n", i);
            return SAL_FAIL;
        }
    }

    if (pDspInitPara->stVoInitInfoSt.uiVoCnt)
    {
        /* fb初始化 */
        if (SAL_SOK != fb_hal_Init())
        {
            SYS_LOGE("fb init failed\n");
        }

        if (SAL_SOK != disp_tsk_moduleInit())
        {
            SYS_LOGE("disp_tsk_moduleInit failed.\n");
            return SAL_FAIL;
        }
    }

    SYS_LOGD("Video Created SUC !!!\n");

    return SAL_SOK;
}

/**
 * @function   System_commonSysDeInit
 * @brief      system 通用模块 系统全局的去初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonSysDeInit()
{
#ifndef SVA_NO_USE
    if (SAL_SOK != Ia_MemDeInit())
    {
        SYS_LOGE("Sva_tskInit err !!!\n");
        return SAL_FAIL;
    }

#endif

    if (SAL_SOK != sys_task_deInit())
    {
        SYS_LOGE("SYS Tsk Create failed !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   System_commonSysInit
 * @brief      system 通用模块 系统全局的初始化

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 System_commonSysInit()
{
#ifndef SVA_NO_USE
    CAPB_AI *pstAiCapPrm = NULL;
    DSPINITPARA *pDspInitPara = SystemPrm_getDspInitPara();

    pstAiCapPrm = capb_get_ai();
    if (NULL == pstAiCapPrm)
    {
        SYS_LOGE("get ai capability failed! \n");
        return SAL_FAIL;
    }

    if (SAL_SOK != sys_task_init(pDspInitPara->stViInitInfoSt.uiViChn))
    {
        SYS_LOGE("SYS Tsk Create failed !!!\n");
        (VOID)sys_task_deInit();
        return SAL_FAIL;
    }
#if 0  //fixme  新版本直接引擎自己申请mem,不需要table map
    if (SAL_SOK != Ia_MemInit())
    {
        SYS_LOGE("Ia_MemInit err !!!\n");
        return SAL_FAIL;
    }
#endif
#endif

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : System_videoInputInit
* 描  述  :输入模块的初始化
* 输  入  :
*         :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 System_videoInputInit()
{
    CAPB_PRODUCT *capb_product = capb_get_product();

#ifndef DOUBLE_CHIP_SLAVE_CHIP
    if (SAL_TRUE == capb_product->bXPackEnable)
    {
        /*********************************/
        if (SAL_SOK != Xpack_DrvInit())
        {
            SYS_LOGE("Xpack_DrvInit failed !!!\n");
            return SAL_FAIL;
        }
    }

#endif

    return SAL_SOK;
}

/**
 * @function   System_commonIaInit
 * @brief      system 智能模块的初始化
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 System_commonIaInit(VOID)
{
#ifndef SVA_NO_USE

    /*
       智能配置文件说明:
       为了兼容安检机和分析仪之间算法配置的差异，如模型分辨率和模型个数等，新增智能模块配置文件。
       项目中目前仅有安检智能子模块sva支持配置，其他子模块ba、face暂不支持配置，若有需要可以添加。

       使用说明:
       目前配置文件深度为4级。当前主要使用的是算法相关信息，alg_info子集中包含的是智能子模块具体信息。
       各模块间配置参数可以不相同，具体参考下述树状图描述。

       注意点:
       在初始化算法配置前，需要先将各模块内实现修改算法配置文件的接口注册到通用模块中，否则应用层的算法配置不会生效。

       Root
       |-----alg_info
     |       |----------sva
     |       |           |-------width
     |       |           |-------height
     |       |           |-------model_num
     |       |           |  ...
     |       |           |-------model_path
     |       |
     |       |----------ba
     |       |           |-------...
     |       |
     |       |----------face
     |       |           |-------...
     |       |
     |       |---------misc_info
     |       |----------...
          ...
     |
     */

    /* 智能子模块注册回调用于初始化相关配置，目前仅有安检智能支持配置 */
    {
        IA_RegModCfgCbFunc(IA_MOD_SVA, (CFG_CB_FUNC)Sva_HalSetCfgPrm);
    }

    /* IA_RegModCfgCbFunc(IA_MOD_BA, (CFG_CB_FUNC)Ba_HalSetCfgPrm); */
    /* IA_RegModCfgCbFunc(IA_MOD_FACE, (CFG_CB_FUNC)Face_HalSetCfgPrm); */

    /* 初始化配置文件名称，用于所有智能模块，当前仅用于sva模块，后续优化 */
    if (SAL_SOK != IA_InitCfgPath())
    {
        SYS_LOGE("IA_InitCfgPath err !!!\n");
        return SAL_FAIL;
    }

    /************ 配置文件 *************/
    if (SAL_SOK != IA_InitCfgPrm())
    {
        SYS_LOGE("IA_InitCfgPrm err !!!\n");
        return SAL_FAIL;
    }

    /************ 安检智能分析 *************/
    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_SVA))
    {
        if (SAL_SOK != Sva_tskInit())
        {
            SYS_LOGE("Sva_tskInit err !!!\n");
            return SAL_FAIL;
        }

        SYS_LOGI("sva tsk Init end !!!\n");
    }


#if 0
    DSPINITPARA *pstInit = SystemPrm_getDspInitPara();

    /************ VDA 移动侦测 ************/
    if ((pstInit->boardType >= SECURITY_ANALYSIS_START) && (pstInit->boardType < SECURITY_ANALYSIS_END))
    {
        if (SAL_SOK != Vda_DrvMdInit())
        {
            SYS_LOGE("Vda_DrvMdInit.\n");
            return SAL_FAIL;
        }
    }

#endif

#if 1
    /********IPC 行为分析 ************/
    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_BA))
    {
        if (SAL_SOK != Ba_tskInit())
        {
            SYS_LOGE("Ba_tskInit err !!!\n");
            return SAL_FAIL;
        }
    }

#endif

    /********智能人脸 ************/
    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_FACE))
    {
        if (SAL_SOK != Face_tskInit())
        {
            SYS_LOGE("Face_tskInit err !!!\n");
            return SAL_FAIL;
        }
    }

    /********人包关联功能 ************/
    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_PPM))
    {
        if (SAL_SOK != Ppm_TskInit())
        {
            SYS_LOGE("Ppm_TskInit err !!!\n");
            return SAL_FAIL;
        }

        SYS_LOGI("ppm tsk init end!!!\n");
    }


#endif
    return SAL_SOK;
}

/**
 * @function   System_commonDeInit
 * @brief      system 通用模块的去初始化

 * @param[out] None
 * @return     INT32
 */
INT32 System_commonDeInit()
{
    return SAL_SOK;
}

/**
 * @function   System_commonInit
 * @brief      system 通用模块的初始化

 * @param[out] None
 * @return     INT32
 */
INT32 System_commonInit()
{
    DSPINITPARA *pstDspInfo = SystemPrm_getDspInitPara();

#ifndef DSP_ISM
    /************ 采集 ***************/
    if (0 != pstDspInfo->stViInitInfoSt.uiViChn && pstDspInfo->dspCapbPar.dev_tpye != PRODUCT_TYPE_ISM)
    {
        if (SAL_SOK != capt_tsk_init())
        {

            SYS_LOGE("capt_tsk_init err !!!\n");
            return SAL_FAIL;
        }
    }

#endif
    /************ 编码 ***************/
    if (0 != pstDspInfo->encChanCnt)
    {
        if (SAL_SOK != venc_tsk_init())
        {
            SYS_LOGE("venc_tsk_init err !!!\n");
            return SAL_FAIL;
        }

        /************ 抓图 ***************/
        if (SAL_SOK != jpeg_tsk_init())
        {
            SYS_LOGE("jpeg_tsk_init err !!!\n");
            return SAL_FAIL;
        }
    }

    /************ jpg解码 ***************/
#if 0  //5030内存优化，该部分内存该为调用该功能时申请
    if (0 != pstDspInfo->decChanCnt)
    {
        if (SAL_SOK != vdec_tsk_JpgDecBuffInit())
        {
            XPACK_LOGE("vdec_tsk_JpgDecBuffInit !!!\n");
            /* return SAL_FAIL; */
        }
    }
#endif
    /************ 智能初始化 ***************/
    if (SAL_SOK != System_commonIaInit())
    {
        SYS_LOGE("System_commonIaInit err !!!\n");
        return SAL_FAIL;
    }

#ifdef ISM_DUAL_CHIP_TRANS
    if (SAL_SOK != System_commonInitTransDev())
    {
        SYS_LOGE("init trans dev failed! \n");
        return SAL_FAIL;
    }
#endif

    return SAL_SOK;
}

/**
 * @function   System_SysInit
 * @brief      system sys prm的初始化

 * @param[out] None
 * @return     INT32
 */
INT32 System_SysInit()
{
    /************ 系统 ***************/
    if (SAL_SOK != SystemPrm_Init())
    {

        SYS_LOGE("SystemPrm_Init err !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   System_XrayInit
 * @brief      system 成像的初始化

 * @param[out] None
 * @return     INT32
 */
INT32 System_XrayInit()
{
#ifdef DSP_ISM
    DSPINITPARA *pstInit = NULL;
    CAPB_PRODUCT *capb_product = NULL;

    pstInit = SystemPrm_getDspInitPara();
    capb_product = capb_get_product();

    /*只有安检机支持*/
    if ((pstInit->boardType >= SECURITY_MACHINE_START) && (pstInit->boardType <= SECURITY_MACHINE_END))
    {
        #ifndef DOUBLE_CHIP_SLAVE_CHIP
        if (SAL_TRUE == capb_product->bXRayEnable)
        {
            /********** X RAY 解析 ***********/
            if (SAL_SOK != xray_tsk_init())
            {
                SAL_ERROR("Xray_tskInit err !!!\n");
                return SAL_FAIL;
            }
        }
        #endif
    }

#endif

    return SAL_SOK;
}

