/**
 * @file    dspttk_init.c
 * @brief   初始化并运行DSP
 * @note
 */

/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_fop.h"
#include "dspttk_devcfg.h"
#include "dspcommon.h"
#include "common_boardtype.h"
#include "dspttk_cmd.h"
#include "dspttk_cmd_xray.h"
#include "dspttk_cmd_sva.h"
#include "dspttk_cmd_xpack.h"

/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */
#define HD1080p_FORMAT			0x0000101b /*注意和1920*1080i一样!! 以后修正, 1920*1080p*/

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */
extern void dspttk_data_cb(STREAM_ELEMENT *pstElem, void *pBuf, UINT32 u32BufLen);


/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */
static DSPINITPARA *pstDspShareParam = NULL;

typedef struct
{
    MEMORY_BUFF_S *stFontType;         /* 字库结构体类型 */
    CHAR sFontPath[64];     /* OSD功能所使用字库的文件路径信息 */
}FONT_BUFF_MAP;

UINT32 dspttkSvaColorTab[SVA_XSI_MAX_ALARM_TYPE] =
{
    0xFF3030,
    0xFF00FF,
    0x9400D3,
    0x00FF00,
    0x050505,
    0xEEEE00,
    0xFF3030,
    0xFF00FF,
    0x9400D3,
    0x00FF00,
    0x050505,
    0x050505,
    0x050505,
    0x050505,
    0x050505,
    0x050505,
    0x050505,
    0x050505,
    0x050505,
    0x050505,
    0x9400D3,
};

UINT8 dspttkSvaNameTab[SVA_XSI_MAX_ALARM_TYPE][SVA_ALERT_NAME_LEN] =
{
    "MayBe塑料瓶water-0", "MayBe水果knife-1", "MayBe手枪-2", "疑似雨伞-3", "疑似块状电池-4",
    "疑似剪刀-5", "疑似金属喷灌-6", "疑似手机-7", "疑似笔记本电脑-8", "疑似枪管-9",
    "疑似弹匣-10", "疑似枪套筒-11", "疑似枪握把-12", "疑似子弹-13", "疑似塑料打火机-14",
    "疑似手铐-15", "疑似烟花爆竹-16", "疑似警棍-17", "疑似电击器-18", "疑似指虎-19",
    "羊角锤-20", "塑料枪-21", "玻璃瓶-22", "易拉罐-23", "金属瓶-24",
    "塑料喷灌-25", "玻璃喷灌-26", "金属打火机-27", "长枪-28", "节状电池-29",
    "圆头锤-30", "斧子-31", "凿子-32", "钳子-33", "扳手-34",
    "公章-35", "雷管-36", "整条香烟-37", "强光手电-38", "充电宝-39",
    "纽扣电池-40", "蓄电池-41", "其他电子设备-42", "锥子-43", "扎钩-44",
    "射钉枪-45", "弹弓-46", "利器-47", "指角剪-48", "铁棍-49",
    "焊枪-50", "游标卡尺-51", "钻头-52", "螺丝刀-53", "锯子-54",
    "折叠剪刀-55", "U型刺绣剪刀-56", "折叠刀-57", "铝制喷灌-58", "无-59",
    "无", "无", "无", "无", "无-64",
    "无", "无", "无", "无", "无-69",
    "无", "无", "无", "无", "无-74",
    "无", "无", "无", "无", "无-79",
    "无", "无", "无", "无", "无-84",
    "无", "无", "无", "无", "无-89",
    "无", "无", "无", "无", "无-94",
    "无", "无", "无", "无", "无-99",
    "无-100", "无-101", "无-102", "无-103", "无104",
    "无-105", "无-106", "无-107", "无-108", "无-109",
    "无", "无", "无", "无", "无-114",
    "无", "无", "无", "无", "无-119",
    "无", "无", "无", "无", "无-124",
    "无", "无", "其他违禁品-127",
};

UINT8 dspttkSvaAiNameTab[SVA_AI_MAX_ALARM_TYPE][SVA_ALERT_NAME_LEN] =
{
    "电击器",
    "手铐",
    "甩棍",
    "指虎",
    "爆竹",
    "打火机",
};


/**
 * @fn      dspttk_get_share_param
 * @brief   获取与DSP共享的全局结构体参数
 *
 * @return  DSPINITPARA* 与DSP共享的全局结构体参数
 */
DSPINITPARA *dspttk_get_share_param(void)
{
    return pstDspShareParam;
}


/**
 * @fn      dspttk_get_boardtype
 * @brief   通过Bin配置文件中的boardType获取DSP共享全局结构体中的对应值
 *
 * @param   [IN] boardType Bin配置文件中的boardType
 *
 * @return  BOARD_TYPE DSP共享全局结构体中的boardType
 */
static BOARD_TYPE dspttk_get_boardtype(CHAR boardType[])
{
    UINT32 i = 0;
    BOOL bMatched = SAL_FALSE;
    struct _board_map_s
    {
        BOARD_TYPE enBoardType;
        CHAR sBoardDesc[64];
    } board_map[] = {
        {ISD_SC6550D_4CV, "SC6550D"},
        {ISD_SC6550S_H4CV_C, "SC6550S"},
        {ISD_SG6550S_2CV, "SG6550S"},
        {ISD_SG6550SA_2CV, "SG6550SA"},
        {ISD_SC5030S_1CVS1, "SC5030S"},
        {ISD_SG5030S_1CV, "SG5030S"},
        {ISD_SG5030SA_1C, "SG5030SA"},
        {ISD_SC140100S_VH, "SC140100S"},
        {ISD_SC140100D_VH, "SC140100H"},
        {ISD_SC100100D_V, "SC100100D"},
    };

    if (NULL != boardType)
    {
        for (i = 0; i < SAL_arraySize(board_map); i++)
        {
            if (0 == strcmp(boardType, board_map[i].sBoardDesc))
            {
                bMatched = SAL_TRUE;
                break;
            }
        }
    }

    if (bMatched)
    {
        return board_map[i].enBoardType;
    }
    else
    {
        PRT_INFO("search matched boardType [%s] failed, use '%s' as default\n", boardType,  board_map[0].sBoardDesc);
        return board_map[0].enBoardType;
    }
}


/**
 * @fn      dspttk_get_ismdevtype
 * @brief   通过Bin配置文件中的ismDevType获取DSP共享全局结构体中的对应值
 *
 * @param   [IN] boardType Bin配置文件中的boardType
 *
 * @return  ISM_DEVTYPE_E APP下发能力级结构体参数ism_dev_type
 */
static ISM_DEVTYPE_E dspttk_get_ismdevtype(CHAR boardType[])
{
    UINT32 i = 0;
    BOOL bMatched = SAL_FALSE;
    struct _ism_dev_map_s
    {
        ISM_DEVTYPE_E enIsmDevType;
        CHAR sBoardDesc[64];
    } ism_dev_map[] = {
        {ISM_SC6550XX, "SC6550D"},
        {ISM_SC6550XX, "SC6550S"},
        {ISM_SG6550XX, "SG6550S"},
        {ISM_SG6550XX, "SG6550SA"},
        {ISM_SC5030XX, "SC5030S"},
        {ISM_SG5030XX, "SG5030S"},
        {ISM_SG5030XX, "SG5030SA"},
        {ISM_SC100100XX, "SC100100D"},
        {ISM_SC140100XX, "SC140100S"},
        {ISM_SC140100XX, "SC140100H"},
    };

    if (NULL != boardType)
    {
        for (i = 0; i < SAL_arraySize(ism_dev_map); i++)
        {
            if (0 == strcmp(boardType, ism_dev_map[i].sBoardDesc))
            {
                bMatched = SAL_TRUE;
                break;
            }
        }
    }

    if (bMatched)
    {
        return ism_dev_map[i].enIsmDevType;
    }
    else
    {
        PRT_INFO("search matched boardType(%s) failed, use '%s' as default\n", boardType, ism_dev_map[0].sBoardDesc);
        return ism_dev_map[0].enIsmDevType;
    }
}


/**
 * @fn      dspttk_get_xray_det_vendor
 * @brief   通过Bin配置文件中的xrayDetVendor获取DSP共享全局结构体中的对应值
 *
 * @param   [IN] xrayDetVendor Bin配置文件中的xrayDetVendor
 *
 * @return  UINT32 APP 下发能力级结构体参数xray_det_vendor
 */
static UINT32 dspttk_get_xray_det_vendor(CHAR xrayDetVendor[])
{
    UINT32 i = 0;
    BOOL bMatched = SAL_FALSE;
    struct _det_vendor_map_s
    {
        XRAY_DET_VENDOR  enXrayDetVendor;
        CHAR sDetVendorDesc[64];
    } det_vendor_map[] = {
        {XRAYDV_DT, "DT"},
        {XRAYDV_SUNFY, "Sunfy"},
        {XRAYDV_IRAY, "IRay"},
        {XRAYDV_RAYINZIYAN, "Rayin"},
        {XRAYDV_DTCA, "DT-CA"},
        {XRAYDV_RAYINZIYAN_QIPAN, "Rayin-QP"},
    };

    if (NULL != xrayDetVendor)
    {
        for (i = 0; i < SAL_arraySize(det_vendor_map); i++)
        {
            if (0 == strcmp(xrayDetVendor, det_vendor_map[i].sDetVendorDesc))
            {
                bMatched = SAL_TRUE;
                break;
            }
        }
    }

    if (bMatched)
    {
        return det_vendor_map[i].enXrayDetVendor;
    }
    else
    {
        PRT_INFO("search matched xrayDetVendor [%s] failed, use '%s' as default\n", xrayDetVendor, det_vendor_map[0].sDetVendorDesc);
        return det_vendor_map[0].enXrayDetVendor;
    }
}


/**
 * @fn      dspttk_get_xray_source_type
 * @brief   通过Bin配置文件中的xraySourceType获取DSP共享全局结构体中的对应值
 *
 * @param   [IN] xraySourceType Bin配置文件中的xraySourceType
 *
 * @return  UINT32 APP 下发能力级结构体参数xray_det_vendor
 */
static UINT32 dspttk_get_xray_source_type(CHAR xraySourceType[])
{
    UINT32 i = 0;
    BOOL bMatched = SAL_FALSE;
    struct _det_vendor_map_s
    {
        XRAY_SOURCE_TYPE_E  enxraySourceType;
        CHAR sDetVendorDesc[64];
    } det_source_map[] = {
        {XRAY_SOURCE_JDY_T160       , "JDY_T160"},
        {XRAY_SOURCE_JDY_T140       , "JDY_T140"},
        {XRAY_SOURCE_JDY_T80        , "JDY_T80"},
        {XRAY_SOURCE_JDY_TD120      , "JDY_TD120"},
        {XRAY_SOURCE_JDY_T120       , "JDY_T120"},
        {XRAY_SOURCE_JDY_T160YT     , "JDY_T160YT"},
        {XRAY_SOURCE_JDY_T2050      , "JDY_T2050"},
        {XRAY_SOURCE_JDY_T140RT     , "JDY_T140RT"},
        {XRAY_SOURCE_JDY_TM80       , "JDY_TM80"},
        {XRAY_SOURCE_KWXD_X160      , "KWXD_X160"},
        {XRAY_SOURCE_CQ_T200        , "CQ_T200"},
        {XRAY_SOURCE_BSD_T160       , "BSD_T160"},
        {XRAY_SOURCE_SPM_S180       , "SPM_S180"},
        {XRAY_SOURCE_LIOENERGY_LXB80, "LIOENERGY_LXB80"}
    };

    if (NULL != xraySourceType)
    {
        for (i = 0; i < SAL_arraySize(det_source_map); i++)
        {
            if (0 == strcmp(xraySourceType, det_source_map[i].sDetVendorDesc))
            {
                bMatched = SAL_TRUE;
                break;
            }
        }
    }

    if (bMatched)
    {
        return det_source_map[i].enxraySourceType;
    }
    else
    {
        PRT_INFO("search matched xraySourceType %s failed, use '%s' as default\n",\
                                 det_source_map[0].sDetVendorDesc, xraySourceType);
        return det_source_map[0].enxraySourceType;
    }
}


/**
 * @fn      dspttk_get_xray_energy
 * @brief   通过Bin配置文件中的xrayEnergy获取DSP共享全局结构体中的对应值
 *
 * @param   [IN] xrayEnergy Bin配置文件中的xrayEnergy
 *
 * @return  UINT32 APP 下发能力级结构体参数xray_energy_num
 */
static UINT32 dspttk_get_xray_energy(CHAR xrayEnergy[])
{
    UINT32 i = 0;
    BOOL bMatched = SAL_FALSE;
    struct _energy_map_s
    {
        XRAY_ENERGY_NUM  enXrayEgy;
        CHAR sEnergyDesc[64];
    } energy_map[] = {
        {XRAY_ENERGY_SINGLE, "single"},
        {XRAY_ENERGY_DUAL, "dual"},
    };

    if (NULL != xrayEnergy)
    {
        for (i = 0; i < SAL_arraySize(energy_map); i++)
        {
            if (0 == strcmp(xrayEnergy, energy_map[i].sEnergyDesc))
            {
                bMatched = SAL_TRUE;
                break;
            }
        }
    }

    if (bMatched)
    {
        return energy_map[i].enXrayEgy;
    }
    else
    {
        PRT_INFO("search matched xrayEnergy(%s) failed, use '%s' as default\n", xrayEnergy, energy_map[0].sEnergyDesc);
        return energy_map[0].enXrayEgy;
    }
}


/**
 * @fn      dspttk_get_vointerface
 * @brief   通过Bin配置文件中的voInterface获取DSP共享全局结构体中的对应值
 *
 * @param   [IN] voInterface Bin配置文件中的voInterface
 *
 * @return  UINT32 设置显示设备属性结构体中的eVoDev
 */
static UINT32 dspttk_get_vointerface(CHAR voInterface[])
{
    UINT32 i = 0;
    BOOL bMatched = SAL_FALSE;
    struct _vo_inter_face_map_s
    {
        VIDEO_OUTPUT_DEV_E enVoInterface;
        CHAR sVoInterfaceDesc[64];
    } vo_inter_face_map[] = {
        {VO_DEV_HDMI, "hdmi"},
        {VO_DEV_MIPI, "mipi"},
        {VO_DEV_HDMI_1, "hdmi_1"},
        {VO_DEV_MIPI_1, "mipi_1"},
    };

    if (NULL != voInterface)
    {
        for (i = 0; i < SAL_arraySize(vo_inter_face_map); i++)
        {
            if (0 == strcmp(voInterface, vo_inter_face_map[i].sVoInterfaceDesc))
            {
                bMatched = SAL_TRUE;
                break;
            }
        }
    }

    if (bMatched)
    {
        return vo_inter_face_map[i].enVoInterface;
    }
    else
    {
        PRT_INFO("search matched voInterface %s failed, use '%s' as default\n", voInterface, vo_inter_face_map[0].sVoInterfaceDesc);
        return vo_inter_face_map[0].enVoInterface;
    }
}


/**
 * @fn      dspttk_xpack_init
 * @brief   根据Bin配置文件构建共享全局结构体中的参数，并初始化DSP
 *
 * @param   [IN] pstDevCfg Bin配置文件参数
 *
 * @return  SAL_STATUS SAL_SOK：初始化DSP成功，SAL_FAIL：初始化DSP失败
 */
SAL_STATUS dspttk_xpack_init(DSPTTK_DEVCFG *pstDevCfg)
{
    UINT64 u64CmdRet = 0;

    u64CmdRet = dspttk_xpack_set_jpg(0);
    u64CmdRet |= dspttk_xpack_set_yuv_offset(0);
    u64CmdRet |= dspttk_xpack_set_segment_attr(0);

    return CMD_RET_OF(u64CmdRet);
}

/**
 * @fn      dspttk_osd_init
 * @brief   根据Bin配置文件构建共享全局结构体中的参数，并初始化DSP
 *
 * @param   [IN] pstDevCfg Bin配置文件参数
 *
 * @return  SAL_STATUS SAL_SOK：初始化DSP成功，SAL_FAIL：初始化DSP失败
 */
SAL_STATUS dspttk_osd_init(DSPTTK_DEVCFG *pstDevCfg)
{
    UINT64 u64CmdRet = 0;
    
    u64CmdRet = dspttk_sva_dect_switch(0);
    u64CmdRet |= dspttk_sva_set_confidence_switch(0);
    u64CmdRet |= dspttk_sva_set_scale_level(0);
    u64CmdRet |= dspttk_sva_set_osd_border_type(0);
    u64CmdRet |= dspttk_sva_set_disp_line_type(0);
    u64CmdRet |= dspttk_sva_set_alert_color(0);

    return CMD_RET_OF(u64CmdRet);
}

/**
 * @fn      dspttk_xray_init
 * @brief   根据Bin配置文件构建共享全局结构体中的参数，并初始化DSP
 *
 * @param   [IN] pstDevCfg Bin配置文件参数
 *
 * @return  SAL_STATUS SAL_SOK：初始化DSP成功，SAL_FAIL：初始化DSP失败
 */
SAL_STATUS dspttk_xray_init(DSPTTK_DEVCFG *pstDevCfg)
{
    UINT64 u64CmdRet = 0;
    UINT32 u32Chan = 0;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    PLAYBACK_SLICE_RANGE *pgstPbSliceRange[MAX_XRAY_CHAN] = {NULL, NULL};

    /********************* 初始化xsp参数 *********************/

    u64CmdRet = dspttk_xray_rtpreview_change_speed(0);
    u64CmdRet |= dspttk_rtpreview_enhanced_scan(0);
    for (u32Chan = 0; u32Chan < pstShareParam->xrayChanCnt; u32Chan++)
    {
        u64CmdRet |= dspttk_xsp_vertical_mirror(u32Chan);
        u64CmdRet |= dspttk_xray_rtpreview_fillin_blank(u32Chan);
    }
    if(XRAY_ENERGY_SINGLE == dspttk_get_xray_energy(pstDevCfg->stInitParam.stXray.xrayEnergy))
    {
        u64CmdRet |= dspttk_xsp_pseudo_color(0);
    }
    else
    {
        u64CmdRet |= dspttk_xsp_color_table(0);
    }
    u64CmdRet |= dspttk_xsp_default_style(0);
    u64CmdRet |= dspttk_xsp_set_alert_unpen(0);
    u64CmdRet |= dspttk_xsp_set_alert_sus(0);
    u64CmdRet |= dspttk_xsp_set_bkg(0);
    u64CmdRet |= dspttk_xsp_set_deformity_correction(0);
    u64CmdRet |= dspttk_xsp_set_rm_blank_slice(0);
    u64CmdRet |= dspttk_xsp_set_package_divide_method(0);
    XSP_COLDHOT_PARAM stColdHot = {0};
    stColdHot.uiLevel = 100;
    SendCmdToDsp(HOST_CMD_XSP_SET_COLDHOT_THRESHOLD, 0, NULL, &stColdHot);

    /* 初始化状态锁 */
    if (SAL_SOK != dspttk_mutex_init(&gstGlobalStatus.mutexStatus))
    {
        PRT_INFO("dspttk_mutex_init 'mutexStatus' failed\n");
        return SAL_FAIL;
    }

    for (u32Chan = 0; u32Chan < pstShareParam->xrayChanCnt; u32Chan++)
    {
        pgstPbSliceRange[u32Chan] = dspttk_get_gpbslice_prm(u32Chan);
        if (SAL_SOK != dspttk_mutex_init(&pgstPbSliceRange[u32Chan]->mutexStatus))
        {
            PRT_INFO("dspttk_mutex_init 'mutexStatus' failed\n");
            return SAL_FAIL;
        }
    }
    

    return CMD_RET_OF(u64CmdRet);
}

/**
 * @fn      dspttk_sys_init
 * @brief   根据Bin配置文件构建共享全局结构体中的参数，并初始化DSP
 *
 * @param   [IN] pstDevCfg Bin配置文件参数
 *
 * @return  SAL_STATUS SAL_SOK：初始化DSP成功，SAL_FAIL：初始化DSP失败
 */
SAL_STATUS dspttk_sys_init(DSPTTK_DEVCFG *pstDevCfg)
{
    INT32 s32Ret = 0;
    INT32 s32Idx = 0;
    UINT32 s32FileNum = 0;
    UINT32 i = 0, j = 0;
    UINT32 u32Width = 0, u32Height = 0;
    UINT32 u32FileSize = 0;
    CHAR suffixes[16] = {0};
    CHAR *pFileName = NULL;
    CHAR *pImageFileBuf = NULL;
    CHAR **ppImageFilePath = NULL;
    CHAR **ppFontFilePath = NULL;

    s32Ret = InitDspSys(&pstDspShareParam, (DATACALLBACK)dspttk_data_cb);
    if (0 != s32Ret)
    {
        PRT_INFO("oops, InitDspSys failed, errno: %d\n", s32Ret);
        return SAL_FAIL;
    }

    // 这里要获取pstDspShareParam的地址，因此定义放在了InitDspSys()后
    FONT_BUFF_MAP astInitFontFilePath[] = {
        {&pstDspShareParam->stFontLibInfo.stHzLib, "alipuhui.ttf"},
        {&pstDspShareParam->stFontLibInfo.stAscLib, "arial.ttf"},
        {&pstDspShareParam->stFontLibInfo.stEncHzLib, "HZK16"},
        {&pstDspShareParam->stFontLibInfo.stEncAscLib, "ASC16"}
    };

    /********************* 通用参数，包括设备型号等 *********************/
    pstDspShareParam->boardType = dspttk_get_boardtype(pstDevCfg->stInitParam.stCommon.boardType); /* 设备类型 */
    pstDspShareParam->dspCapbPar.ism_dev_type = dspttk_get_ismdevtype(pstDevCfg->stInitParam.stCommon.boardType); /* 安检机机型大类 */
    pstDspShareParam->dspCapbPar.dev_tpye = PRODUCT_TYPE_ISM; /* 产品类型，当前只支持安检机 */

    /********************* dspttk结构体暂时无法获取的参数 *********************/
    s32FileNum = dspttk_get_file_name_list("./datain/yuv", &ppImageFilePath);
    if (s32FileNum <= 0)
    {
        PRT_INFO("dspttk_get_file_name_list failed, errno: %d\n", s32FileNum);
        s32Ret = SAL_FAIL;
        goto EXIT;
    }

    s32Idx = 0;
    for (i = 0; i < s32FileNum; i++)
    {
        s32Ret = dspttk_get_file_suffixes(ppImageFilePath[i], suffixes);
        if (SAL_FAIL == s32Ret)
        {
            PRT_INFO("dspttk_get_file_suffixes failed, errno: %d\n", s32Ret);
            continue;
        }
        if ((strncmp("yuv", suffixes, strlen(suffixes)) == 0) || (strncmp("rgb", suffixes, strlen(suffixes)) == 0))
        {

            u32FileSize = dspttk_get_file_size(ppImageFilePath[i]);
            if (u32FileSize <= 0)
            {
                PRT_INFO("get %s file size failed! \n", ppImageFilePath[i]);
                continue;
            }

            pImageFileBuf = malloc(u32FileSize);
            if (pImageFileBuf == NULL)
            {
                PRT_INFO("malloc image file buf failed ");
                continue;
            }

            s32Ret = dspttk_read_file(ppImageFilePath[i], 0, pImageFileBuf, (UINT32 *)&u32FileSize);
            if (s32Ret == SAL_FAIL)
            {
                PRT_INFO("Idx %u, reading file[%s] failed, read %d bytes.\n", i, ppImageFilePath[i], u32FileSize);
                continue;
            }

            pFileName = basename(ppImageFilePath[i]); /* 获取文件名 */
            if ((strncmp("novideosignal_w", pFileName, strlen("novideosignal_w")) == 0) || (strncmp("hikvision_w", pFileName, strlen("hikvision_w")) == 0))
            {
                /* 自动获取宽高 */
                pFileName = strstr(pFileName, "_w");  /* 将字符串截取到“_w”出现的位置 */
                s32Ret = sscanf(pFileName, "%*[^w]w%u[^_]", &u32Width);
                if (s32Ret != 1)
                {
                    PRT_INFO("Auto get string width  failed, Loop through the next file. errno: %d\n", s32Ret);
                    continue;
                }

                s32Ret = sscanf(pFileName, "%*[^h]h%u[^.]", &u32Height);
                if (s32Ret != 1)
                {
                    PRT_INFO("Auto get string height failed, Loop through the next file. errno: %d\n", s32Ret);
                    continue;
                }

                if (s32Idx == SAL_arraySize(pstDspShareParam->stCaptNoSignalInfo.uiNoSignalImgAddr))
                {
                    PRT_INFO("The novideosignal image and hikvision image file exceeds the number of array elements, errno: %d\n", s32Idx);
                    break;
                }

                pstDspShareParam->stCaptNoSignalInfo.uiNoSignalImgAddr[s32Idx] = (void *)pImageFileBuf;
                pstDspShareParam->stCaptNoSignalInfo.uiNoSignalImgW[s32Idx] = (u32FileSize == 0) ? 0 : u32Width;
                pstDspShareParam->stCaptNoSignalInfo.uiNoSignalImgH[s32Idx] = (u32FileSize == 0) ? 0 : u32Height;
                pstDspShareParam->stCaptNoSignalInfo.uiNoSignalImgSize[s32Idx] = u32FileSize;

                s32Idx++;
            }
        }
    }

    if (i == s32FileNum && s32Idx != 2)
    {   /*yuv文件命名为novideosignal_w1920_h1080.argb(yuv) 和 hikvision_w1920_h1080.rgb(yuv)*/
        PRT_INFO("The novideosignal image and hikvision image file exceeds the number. errno: %d\n", s32Idx);
        s32Ret = SAL_FAIL;
        goto EXIT;
    }

    for (i = 0; i < SAL_arraySize(pstDspShareParam->stViInitInfoSt.stViInitPrm); i++)
    {
        pstDspShareParam->stViInitInfoSt.stViInitPrm[i].eViStandard = VS_STD_PAL;
        pstDspShareParam->stViInitInfoSt.stViInitPrm[i].eViResolution = HD1080p_FORMAT;
    }

    /* OSD功能所使用字库的信息 */
    s32FileNum = dspttk_get_file_name_list("./datain/font", &ppFontFilePath);
    if (s32FileNum <= 0)
    {
        PRT_INFO("dspttk_get_file_name_list failed, errno: %d\n", s32FileNum);
        s32Ret = SAL_FAIL;
        goto EXIT;
    }

    s32Idx = 0;
    for (i = 0; i < s32FileNum; i++)
    {
        for (j = 0; j < SAL_arraySize(astInitFontFilePath); j++)
        {
            pFileName = basename(ppFontFilePath[i]);
            if (0 == strcmp(pFileName, astInitFontFilePath[j].sFontPath))
            {
                u32FileSize = dspttk_get_file_size(ppFontFilePath[i]);
                if (u32FileSize <= 0)
                {
                    PRT_INFO("get %s file size failed! \n", ppFontFilePath[i]);
                    continue;
                }

                if (s32Idx == SAL_arraySize(astInitFontFilePath))
                {
                    break;
                }

                astInitFontFilePath[j].stFontType->pVirAddr = malloc(u32FileSize);
                if (NULL == astInitFontFilePath[j].stFontType->pVirAddr)
                {
                    PRT_INFO("malloc font pVirAddr failed\n");
                    continue;
                }

                astInitFontFilePath[j].stFontType->uiSize = u32FileSize;
                s32Ret = dspttk_read_file(ppFontFilePath[i], 0, astInitFontFilePath[j].stFontType->pVirAddr, (UINT32 *)&u32FileSize);

                if (s32Ret == SAL_FAIL)
                {
                    PRT_INFO("dspttk_read_file '%s' failed\n", ppFontFilePath[i]);
                    continue;
                }

                s32Idx++;
            }
        }
    }

    /********************* XRay参数 *********************/
    pstDevCfg->stInitParam.stXray.xrayResW = (UINT32)(pstDevCfg->stInitParam.stXray.xrayWidthMax * pstDevCfg->stInitParam.stXray.resizeFactor);
    pstDevCfg->stInitParam.stXray.xrayResH = (UINT32)(pstDevCfg->stInitParam.stXray.xrayHeightMax * pstDevCfg->stInitParam.stXray.resizeFactor);
    /* X ray通道个数(预览+回拉) */
    pstDspShareParam->xrayChanCnt = pstDevCfg->stInitParam.stXray.xrayChanCnt;
    /* X-Ray探测板供应商 */
    pstDspShareParam->dspCapbPar.xray_det_vendor = dspttk_get_xray_det_vendor(pstDevCfg->stInitParam.stXray.xrayDetVendor);
    /* X-Ray射线源厂商 */
    for (i = 0; i < pstDspShareParam->xrayChanCnt; i++)
    {
        pstDspShareParam->dspCapbPar.aenXraySrcType[i] = dspttk_get_xray_source_type(pstDevCfg->stInitParam.stXray.xraySourceType[i]);
    }
    pstDspShareParam->dspCapbPar.xray_energy_num = dspttk_get_xray_energy(pstDevCfg->stInitParam.stXray.xrayEnergy); /* X光机能量类型数 */
    pstDspShareParam->dspCapbPar.xray_width_max = pstDevCfg->stInitParam.stXray.xrayWidthMax; /* X-RAY源输入数据宽最大值，即探测板像素数×探测板数量 */
    pstDspShareParam->dspCapbPar.xray_height_max = pstDevCfg->stInitParam.stXray.xrayHeightMax; /* X-RAY源输入数据高最大值，即全屏显示横向对应的RAW数据长度 */
    pstDspShareParam->xspLibSrc = pstDevCfg->stInitParam.stXray.enXspLibSrc;                    /* XSP成像算法库，枚举XSP_LIB_SRC*/
    pstDspShareParam->dspCapbPar.xsp_resize_factor.resize_height_factor = pstDevCfg->stInitParam.stXray.resizeFactor; /* XSP超分系数，宽高相同 */
    pstDspShareParam->dspCapbPar.xsp_resize_factor.resize_width_factor = pstDevCfg->stInitParam.stXray.resizeFactor; /* XSP超分系数，宽高相同 */
    pstDspShareParam->dspCapbPar.package_len_max = pstDevCfg->stInitParam.stXray.packageLenMax; /* XSP包裹最大分割长度*/
    /* 传送带速度，条带高度，积分时间，显示帧率 */
    for (i = 0; i < XRAY_FORM_NUM; i++)
    {
        for (j = 0; j < XRAY_SPEED_NUM; j++)
        {
            /* 传送带速度 */
            pstDspShareParam->dspCapbPar.xray_speed[i][j].belt_speed = pstDevCfg->stInitParam.stXray.stSpeed[i][j].beltSpeed;
            /* 条带高度 */
            pstDspShareParam->dspCapbPar.xray_speed[i][j].slice_height = pstDevCfg->stInitParam.stXray.stSpeed[i][j].sliceHeight;
            /* 积分时间 */
            pstDspShareParam->dspCapbPar.xray_speed[i][j].integral_time = pstDevCfg->stInitParam.stXray.stSpeed[i][j].integralTime;
            /* 显示帧率 */
            pstDspShareParam->DispStatus->fps = pstDevCfg->stInitParam.stXray.stSpeed[i][j].dispfps;
        }
    }

    /********************* display参数 *********************/
    /* display输出设备数 */
    pstDevCfg->stInitParam.stDisplay.u32DispWithMax = (UINT32)(pstDevCfg->stInitParam.stXray.xrayHeightMax * pstDevCfg->stInitParam.stXray.resizeFactor);
    pstDevCfg->stInitParam.stDisplay.u32DispHeightMax = (UINT32)((pstDevCfg->stInitParam.stXray.xrayWidthMax + \
                                                                  pstDevCfg->stInitParam.stDisplay.paddingTop + \
                                                                  pstDevCfg->stInitParam.stDisplay.paddingBottom) * \
                                                                  pstDevCfg->stInitParam.stXray.resizeFactor);
    pstDspShareParam->xrayDispChanCnt = pstDevCfg->stInitParam.stDisplay.voDevCnt;
    /* display Padding Top */
    pstDspShareParam->dspCapbPar.padding_top = pstDevCfg->stInitParam.stDisplay.paddingTop;
    /* display padding Bottom */
    pstDspShareParam->dspCapbPar.padding_bottom = pstDevCfg->stInitParam.stDisplay.paddingBottom;
    /* display Blanking Top */
    pstDspShareParam->dspCapbPar.blanking_top = pstDevCfg->stInitParam.stDisplay.blankingTop;
    /* display Blanking Bottom */
    pstDspShareParam->dspCapbPar.blanking_bottom = pstDevCfg->stInitParam.stDisplay.blankingBottom;

    /* 视频采集参数配置 MAX_CAPT_CHAN */
    pstDspShareParam->stViInitInfoSt.uiViChn = pstDevCfg->stInitParam.stDisplay.voDevCnt;
    /* 视频输出参数配置 MAX_DISP_CHAN */
    pstDspShareParam->stVoInitInfoSt.uiVoCnt = pstDevCfg->stInitParam.stXray.xrayChanCnt;

    /* 显示输出模块初始化属性 */
    for (i = 0; i < SAL_arraySize(pstDevCfg->stInitParam.stDisplay.stVoDevAttr); i++)
    {
        /* 是否有视频输出   */
        pstDspShareParam->stVoInitInfoSt.stVoInfoPrm[i].bHaveVo = pstDevCfg->stInitParam.stDisplay.stVoDevAttr[i].enable;
        /* 应用设置的最大通道数，只接收一次 */
        pstDspShareParam->stVoInitInfoSt.stVoInfoPrm[i].voChnCnt = pstDevCfg->stInitParam.stDisplay.stVoDevAttr[i].voChnCnt;
        /* 显示输出设备类型 */
        pstDspShareParam->stVoInitInfoSt.stVoInfoPrm[i].stDispDevAttr.eVoDev = dspttk_get_vointerface(pstDevCfg->stInitParam.stDisplay.stVoDevAttr[i].voInterface);
        /* 视频输出设备宽度 */
        pstDspShareParam->stVoInitInfoSt.stVoInfoPrm[i].stDispDevAttr.videoOutputWidth = pstDevCfg->stInitParam.stDisplay.stVoDevAttr[i].width;
        /* 视频输出设备高度 */
        pstDspShareParam->stVoInitInfoSt.stVoInfoPrm[i].stDispDevAttr.videoOutputHeight = pstDevCfg->stInitParam.stDisplay.stVoDevAttr[i].height;
        /* 视频输出设备刷新帧率 */
        pstDspShareParam->stVoInitInfoSt.stVoInfoPrm[i].stDispDevAttr.videoOutputFps = pstDevCfg->stInitParam.stDisplay.stVoDevAttr[i].fpsDefault;
    }

    /********************* Enc & Dec参数 *********************/
    /* 同时支持编码通道个数 */
    pstDspShareParam->encChanCnt = pstDevCfg->stInitParam.stEncDec.encChanCnt;

    for (i = 0; i < SAL_arraySize(pstDevCfg->stInitParam.stEncDec.encRecPsBufLen); i++)
    {
        /* 编码录像主码流buf缓冲区总大小 */
        pstDspShareParam->RecPoolMain[i].totalLen = pstDevCfg->stInitParam.stEncDec.encRecPsBufLen[i][0];
        /* 编码录像子码流buf缓冲区总大小 */
        pstDspShareParam->RecPoolSub[i].totalLen = pstDevCfg->stInitParam.stEncDec.encRecPsBufLen[i][1];
        /* 编码录像third码流buf缓冲区总大小 */
        pstDspShareParam->RecPoolThird[i].totalLen = pstDevCfg->stInitParam.stEncDec.encRecPsBufLen[i][2];
        /* 编码网络主码流buf内存总长 */
        pstDspShareParam->NetPoolMain[i].totalLen = pstDevCfg->stInitParam.stEncDec.encNetRtpBufLen[i][0];
        /* 编码网络子码流buf内存总长 */
        pstDspShareParam->NetPoolSub[i].totalLen = pstDevCfg->stInitParam.stEncDec.encNetRtpBufLen[i][1];
    }
    /* 同时支持解码通道个数 */
    pstDspShareParam->decChanCnt = pstDevCfg->stInitParam.stEncDec.decChanCnt;

    for (i = 0; i < SAL_arraySize(pstDevCfg->stInitParam.stEncDec.decBufLen); i++)
    {
        /* 解码码流共享缓冲区(PS/RTP)长度 */
        pstDspShareParam->decShareBuf[i].bufLen = pstDevCfg->stInitParam.stEncDec.decBufLen[i];
    }

    /* 同时支持转包通道个数 */
    pstDspShareParam->ipcChanCnt = pstDevCfg->stInitParam.stEncDec.ipcStreamPackTransChanCnt;

    for (i = 0; i < SAL_arraySize(pstDevCfg->stInitParam.stEncDec.ipcStreamPackTransBufLen); i++)
    {
        /* 转包码流共享缓冲区长度 */
        pstDspShareParam->ipcShareBuf[i].bufLen = pstDevCfg->stInitParam.stEncDec.ipcStreamPackTransBufLen[i][0];
        /* 转包后ps  码流buf长度 */
        pstDspShareParam->RecPoolRecode[i].totalLen = pstDevCfg->stInitParam.stEncDec.ipcStreamPackTransBufLen[i][1];
        /* 转包后rtp 码流buf长度 */
        pstDspShareParam->NetPoolRecode[i].totalLen = pstDevCfg->stInitParam.stEncDec.ipcStreamPackTransBufLen[i][2];
    }

    /********************* SVA参数 *********************/
    /* 设备种类，用于区分单双芯片 */
    pstDspShareParam->deviceType = pstDevCfg->stInitParam.stSva.enDevChipType;
    if(0 != pstDspShareParam->deviceType)
    {
        pstDspShareParam->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_SVA].stIaModTransPrm.uiDualChipTransFlag = 1;
        pstDspShareParam->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_SVA].stIaModTransPrm.uiRunChipId = 2 - pstDspShareParam->deviceType;
    }
    pstDspShareParam->modelType = DOUBLE_MODEL_TYPE;
    pstDspShareParam->stIaInitMapPrm.stIaModCapbPrm[IA_MOD_SVA].uiEnableFlag = 1;

    for (j = 0; j < pstDspShareParam->stViInitInfoSt.uiViChn; j++)
    {
        pstDspShareParam->stSvaInitInfoSt[j].uiCnt = 0;
        pstDspShareParam->stFontLibInfo.enEncFormat = ENC_FMT_GB2312; /* 设置危险品名称编码类型为GB2312 */

        s32Idx = 0;
        for (i = 0; i < SVA_XSI_MAX_ALARM_TYPE; i++)
        {
            pstDspShareParam->stSvaInitInfoSt[j].det_info[s32Idx].sva_type = i;
            pstDspShareParam->stSvaInitInfoSt[j].det_info[s32Idx].sva_key = 1;
            pstDspShareParam->stSvaInitInfoSt[j].det_info[s32Idx].sva_color =  0x006020; //暂不支持使用dspttkSvaColorTab[i]配置多种颜色OSD文本;
            sprintf((CHAR *)pstDspShareParam->stSvaInitInfoSt[j].det_info[s32Idx].sva_name, "%s", dspttkSvaNameTab[i]);

            s32Idx++;
        }

        pstDspShareParam->stSvaInitInfoSt[j].uiCnt = s32Idx;
    }
    /**********     申请share-buf len save图片的大小超过了默认的share-buf len大小  *******************/
    for (i = 0; i < pstDspShareParam->xrayChanCnt; i++)
    {
        pstDspShareParam->xrayPbBuf[i].bufLen = NRAW_BYTE_SIZE(pstDevCfg->stInitParam.stDisplay.u32DispWithMax, 
                                                               pstDevCfg->stInitParam.stDisplay.u32DispHeightMax, 
                                                               pstDspShareParam->dspCapbPar.xray_energy_num);
        pstDspShareParam->xrayPbBuf[i].pVirtAddr = (unsigned char *)malloc(pstDspShareParam->xrayPbBuf[i].bufLen);
    }
    s32Ret = SendCmdToDsp(HOST_CMD_SVA_DEINIT, 0, (unsigned int *)0, (void *)0);
    if (0 != s32Ret)
    {
        PRT_INFO("oops, SendCmdToDsp 'HOST_CMD_SVA_DEINIT' failed, errno: %d\n", s32Ret);
    }
    /********************* audio参数 *********************/
    /* 音频输出通道个数 */
    pstDspShareParam->dspCapbPar.audio_dev_cnt = pstDevCfg->stInitParam.stAudio.aoDevCnt;

    s32Ret = SendCmdToDsp(HOST_CMD_DSP_INIT, 0, NULL, NULL);
    if (0 != s32Ret)
    {
        PRT_INFO("oops, SendCmdToDsp 'HOST_CMD_DSP_INIT' failed, errno: %d\n", s32Ret);
        goto EXIT;
    }

EXIT:
    if (NULL != (CHAR **)ppImageFilePath)
    {
        free((CHAR **)ppImageFilePath);
    }
    if (NULL != (CHAR **)ppFontFilePath)
    {
        free((CHAR **)ppFontFilePath);
    }
    return s32Ret;
}


