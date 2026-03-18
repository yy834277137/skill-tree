
/**
 * 该文件内实现的功能包括：
 * 1、将Bin配置文件中的参数值转换为Html对应属性，构建Html文件，配置文件中的参数是
 */

 /* ========================================================================== */
 /*                               Include Files                                */
 /* ========================================================================== */
#include "sal_type.h"
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_util.h"
#include "dspttk_panel.h"
#include "dspttk_html.h"

/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */
typedef struct
{
    CHAR sHtml[64];     // Html中的各标签属性值
    CHAR sBinJs[64];    // Bin和Json配置文件中对应的参数
}HTML_BIN_VALUE_MAP;

#define SZ_BUF_SIZE (64)                /* 字符串数组默认大小 */

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/**
 * @fn      dspttk_build_panel_init
 * @brief   根据配置文件中的参数构建初始化区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstInitParam 全局配置参数中的初始化参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_init(VOID *pHtmlHandle, DSPTTK_INIT_PARAM *pstInitParam)
{
    UINT32 i = 0, j = 0;
    CHAR aString[24] = {0};
    CHAR sTableId[32] = {0};
    CHAR *pString = NULL;

    if (NULL == pHtmlHandle || NULL == pstInitParam)
    {
        PRT_INFO("pHtmlHandle(%p) OR pstInitParam(%p) is NULL\n,", pHtmlHandle, pstInitParam);
        return SAL_FAIL;
    }

    /********************* 设备运行状态 *********************/
    if (DSPTTK_RUNSTAT_READY == gstGlobalStatus.enRunStatus)
    {
        dspttk_html_set_text_by_id(pHtmlHandle, "submit-start", "Start");
        dspttk_html_set_attr_by_id(pHtmlHandle, "submit-start", "class", "btn btn-outline-primary");
        dspttk_html_del_attr_by_id(pHtmlHandle, "submit-start", "disabled");
    }
    else if (DSPTTK_RUNSTAT_INITING == gstGlobalStatus.enRunStatus)
    {
        dspttk_html_set_text_by_id(pHtmlHandle, "submit-start", "Start");
        dspttk_html_set_attr_by_id(pHtmlHandle, "submit-start", "class", "btn btn-secondary");
        dspttk_html_set_attr_by_id(pHtmlHandle, "submit-start", "disabled", "disabled");
    }
    else
    {
        dspttk_html_set_text_by_id(pHtmlHandle, "submit-start", "Running");
        dspttk_html_set_attr_by_id(pHtmlHandle, "submit-start", "class", "btn btn-success");
        dspttk_html_set_attr_by_id(pHtmlHandle, "submit-start", "disabled", "disabled");
    }

    /********************* 通用参数，包括设备型号等 *********************/
    // 设备类型
    dspttk_html_set_select_by_name(pHtmlHandle, "dev-board-type", pstInitParam->stCommon.boardType);

    /********************* XRay参数 *********************/
    // XRaw宽，探测板数据 * 64
    dspttk_html_set_attr_by_name(pHtmlHandle, "xray-width-max", "value", dspttk_long_to_string(pstInitParam->stXray.xrayWidthMax, aString, sizeof(aString)));
    // XRaw高
    dspttk_html_set_attr_by_name(pHtmlHandle, "xray-height-max", "value", dspttk_long_to_string(pstInitParam->stXray.xrayHeightMax, aString, sizeof(aString)));
    // XSP算法库类型
    dspttk_html_set_select_by_name(pHtmlHandle, "xray-lib-src", dspttk_long_to_string(pstInitParam->stXray.enXspLibSrc, aString, sizeof(aString)));
    // XSP超分系数
    dspttk_html_set_attr_by_name(pHtmlHandle, "xray-resize-factor", "value", dspttk_float_to_string(pstInitParam->stXray.resizeFactor, aString, sizeof(aString)));
    // Xray射线源数
    dspttk_html_set_attr_by_name(pHtmlHandle, "xray-chan-cnt", "value", dspttk_long_to_string(pstInitParam->stXray.xrayChanCnt, aString, sizeof(aString)));
    // XRay探测板供型号
    dspttk_html_set_select_by_name(pHtmlHandle, "xray-det-vendor", pstInitParam->stXray.xrayDetVendor);
    // 射线源型号
    dspttk_html_set_select_by_name(pHtmlHandle, "xray-source-type-0", pstInitParam->stXray.xraySourceType[0]);

    dspttk_html_set_select_range(pHtmlHandle,  "xray-source-type-0", pstInitParam->stXray.szXraySrcTypeRang[0], SAL_arraySize(pstInitParam->stXray.szXraySrcTypeRang[0]));

    if (pstInitParam->stXray.xrayChanCnt > 1)
    {
        dspttk_html_set_style_by_id(pHtmlHandle, "td-xray-source-type-1", "opacity", "1"); // 设置为不透明
        dspttk_html_del_attr_by_id(pHtmlHandle, "xray-source-type-1", "disabled"); // 取消禁用
        dspttk_html_set_select_by_name(pHtmlHandle, "xray-source-type-1", pstInitParam->stXray.xraySourceType[1]);
        dspttk_html_set_select_range(pHtmlHandle, "xray-source-type-1",  pstInitParam->stXray.szXraySrcTypeRang[1], SAL_arraySize(pstInitParam->stXray.szXraySrcTypeRang[1]));
    }
    else
    {
        dspttk_html_set_style_by_id(pHtmlHandle, "td-xray-source-type-1", "opacity", "0"); // 设置为透明，隐藏
        dspttk_html_set_attr_by_id(pHtmlHandle, "xray-source-type-1", "disabled", "disabled"); // 禁用下拉选择
    }

    // Xraw传送带速度，条带高度，积分时间，显示帧率
    for (i = 0; i < XRAY_FORM_NUM; i++)
    {
        for (j = 0; j < XRAY_SPEED_NUM; j++)
        {
            snprintf(sTableId, sizeof(sTableId), "%s%d%d%s", "xray-speed", i, j, "-init");

            /* 传送带速度 */
            snprintf(aString, sizeof(aString), "%g", pstInitParam->stXray.stSpeed[i][j].beltSpeed);
            pString = aString;
            dspttk_html_set_table_cell_inputs(pHtmlHandle, sTableId, 0, 0, &pString, 1);

            /* 条带高度 */
            snprintf(aString, sizeof(aString), "%d", pstInitParam->stXray.stSpeed[i][j].sliceHeight);
            pString = aString;
            dspttk_html_set_table_cell_inputs(pHtmlHandle, sTableId, 0, 1, &pString, 1);

            /* 积分时间 */
            snprintf(aString, sizeof(aString), "%d", pstInitParam->stXray.stSpeed[i][j].integralTime);
            pString = aString;
            dspttk_html_set_table_cell_inputs(pHtmlHandle, sTableId, 1, 0, &pString, 1);

            /* 显示帧率 */
            snprintf(aString, sizeof(aString), "%d", pstInitParam->stXray.stSpeed[i][j].dispfps);
            pString = aString;
            dspttk_html_set_table_cell_inputs(pHtmlHandle, sTableId, 1, 1, &pString, 1);
        }
    }

    /********************* display参数 *********************/
    // display输出设备数
    dspttk_html_set_attr_by_id(pHtmlHandle, "disp-vodev-cnt-init", "value", dspttk_long_to_string(pstInitParam->stDisplay.voDevCnt, aString, sizeof(aString)));
    // display Padding Top
    dspttk_html_set_attr_by_id(pHtmlHandle, "disp-padding-top-init", "value", dspttk_long_to_string(pstInitParam->stDisplay.paddingTop, aString, sizeof(aString)));
    // display padding Bottom
    dspttk_html_set_attr_by_id(pHtmlHandle, "disp-padding-bottom-init", "value", dspttk_long_to_string(pstInitParam->stDisplay.paddingBottom, aString, sizeof(aString)));
    // display Blanking Top
    dspttk_html_set_attr_by_id(pHtmlHandle, "disp-blanking-top-init", "value", dspttk_long_to_string(pstInitParam->stDisplay.blankingTop, aString, sizeof(aString)));
    // display Blanking Bottom
    dspttk_html_set_attr_by_id(pHtmlHandle, "disp-blanking-bottom-init", "value", dspttk_long_to_string(pstInitParam->stDisplay.blankingBottom, aString, sizeof(aString)));

    /********************* Enc & Dec参数 *********************/
    // 编码通道数
    dspttk_html_set_attr_by_id(pHtmlHandle, "enc-chan-cnt-init", "value", dspttk_long_to_string(pstInitParam->stEncDec.encChanCnt, aString, sizeof(aString)));
    // 解码通道数
    dspttk_html_set_attr_by_id(pHtmlHandle, "dec-chan-cnt-init", "value", dspttk_long_to_string(pstInitParam->stEncDec.decChanCnt, aString, sizeof(aString)));
    // 转封装通道数
    dspttk_html_set_attr_by_id(pHtmlHandle, "ipc-stream-pack-trans-chan-cnt-init", "value", dspttk_long_to_string(pstInitParam->stEncDec.ipcStreamPackTransChanCnt, aString, sizeof(aString)));

    /********************* SVA参数 *********************/
    // SVA设备种类，区分单双芯片 */
    dspttk_html_set_select_by_name(pHtmlHandle, "sva-dev-chip-type", dspttk_long_to_string(pstInitParam->stSva.enDevChipType, aString, sizeof(aString)));

    return SAL_SOK;
}


/**
 * @fn      dspttk_build_panel_setting_env
 * @brief   根据配置文件中的参数构建Setting-Env区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstEnv 全局配置参数中的Env参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_env(VOID *pHtmlHandle, DSPTTK_SETTING_ENV *pstEnv)
{
    SAL_STATUS sRet = SAL_SOK;

    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-runtime-dir", "value", pstEnv->runtimeDir);
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "env-input-raw-path-loop", pstEnv->bInputDirRtLoop);
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "env-output-dir-auto-clear", pstEnv->bOutputDirAutoClear);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-input-rtxraw", "value", pstEnv->stInputDir.rtXraw);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-input-tipnraw", "value", pstEnv->stInputDir.tipNraw);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-input-ipcstream", "value", pstEnv->stInputDir.ipcStream);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-input-aimodel", "value", pstEnv->stInputDir.aiModel);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-input-gui", "value", pstEnv->stInputDir.gui);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-input-alarmaudio", "value", pstEnv->stInputDir.alramAudio);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-output-rawslice", "value", pstEnv->stOutputDir.sliceNraw);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-output-packageimg", "value", pstEnv->stOutputDir.packageImg);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-output-savescreen", "value", pstEnv->stOutputDir.saveScreen);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-output-packageseg", "value", pstEnv->stOutputDir.packageSeg);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-output-encstream", "value", pstEnv->stOutputDir.encStream);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-output-packagetrans", "value", pstEnv->stOutputDir.packageTrans);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-output-streamtrans", "value", pstEnv->stOutputDir.streamTrans);
    // sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "env-output-xrawstore", "value", pstEnv->stOutputDir.xrawStore);

    if (sRet == SAL_FAIL)
    {
        PRT_INFO("error setting env.\n");
    }

    return sRet;
}


/**
 * @fn      dspttk_build_panel_setting_xray
 * @brief   根据配置文件中的参数构建Setting-XRay区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgXray 全局配置参数中的XRay参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_xray(VOID *pHtmlHandle, DSPTTK_SETTING_XRAY *pstCfgXray)
{
    SAL_STATUS sRet = SAL_SOK;
    CHAR aString[24] = {0};

    sRet |= dspttk_html_set_select_by_name(pHtmlHandle, "xray-speed", dspttk_long_to_string(pstCfgXray->stSpeedParam.enSpeedUsr, aString, sizeof(aString)));
    sRet |= dspttk_html_set_radio_by_name(pHtmlHandle, "xray-format", dspttk_long_to_string(pstCfgXray->stSpeedParam.enFormUsr, aString, sizeof(aString)));
    sRet |= dspttk_html_set_radio_by_name(pHtmlHandle, "xray-direction", dspttk_long_to_string(pstCfgXray->enDispDir, aString, sizeof(aString)));
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xray-vertical-mirror-ch0", pstCfgXray->stVMirror[0].bEnable);
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xray-vertical-mirror-ch1", pstCfgXray->stVMirror[1].bEnable);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xray-fillin-blank-left-ch0", "value", dspttk_long_to_string(pstCfgXray->stFillinBlank[0].marginTop, aString, sizeof(aString)));
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xray-fillin-blank-right-ch0", "value", dspttk_long_to_string(pstCfgXray->stFillinBlank[0].marginBottom, aString, sizeof(aString)));
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xray-fillin-blank-left-ch1", "value", dspttk_long_to_string(pstCfgXray->stFillinBlank[1].marginTop, aString, sizeof(aString)));
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xray-fillin-blank-right-ch1", "value", dspttk_long_to_string(pstCfgXray->stFillinBlank[1].marginBottom, aString, sizeof(aString)));
    sRet |= dspttk_html_set_radio_by_name(pHtmlHandle, "xray-color-table", dspttk_long_to_string(pstCfgXray->stColorTable.enConfigueTable, aString, sizeof(aString)));
    sRet |= dspttk_html_set_radio_by_name(pHtmlHandle, "xray-pseudo-color", dspttk_long_to_string(pstCfgXray->stPseudoColor.pseudo_color, aString, sizeof(aString)));
    sRet |= dspttk_html_set_radio_by_name(pHtmlHandle, "xray-default-style", dspttk_long_to_string(pstCfgXray->stXspStyle.xsp_style, aString, sizeof(aString)));
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xray-unpen-enable", pstCfgXray->stXspUnpen.uiEnable);
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xray-susorg-enable", pstCfgXray->stSusAlert.uiEnable);
    sRet |= dspttk_html_set_radio_by_name(pHtmlHandle, "xray-unpen-sensitivity", dspttk_long_to_string(pstCfgXray->stXspUnpen.uiSensitivity, aString, sizeof(aString)));
    sRet |= dspttk_html_set_radio_by_name(pHtmlHandle, "xray-susorg-sensitivity", dspttk_long_to_string(pstCfgXray->stSusAlert.uiSensitivity, aString, sizeof(aString)));
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xray-tip-timeout", "value", dspttk_long_to_string(pstCfgXray->stTipSet.stTipData.uiTime, aString, sizeof(aString)));
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xray-bkg-det-th", "value", dspttk_long_to_string(pstCfgXray->stBkg.uiBkgDetTh, aString, sizeof(aString)));
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xray-bkg-update-ratio", "value", dspttk_long_to_string(pstCfgXray->stBkg.uiFullUpdateRatio, aString, sizeof(aString)));
    sRet |= dspttk_html_set_radio_by_name(pHtmlHandle, "xray-deformity-corr", dspttk_long_to_string(pstCfgXray->stDeformityCorrection.bEnable, aString, sizeof(aString)));
    sRet |= dspttk_html_set_radio_by_name(pHtmlHandle, "xray-package-divide-method", dspttk_long_to_string(pstCfgXray->stPackagePrm.enDivMethod, aString, sizeof(aString)));
    sRet |= dspttk_html_set_radio_by_name(pHtmlHandle, "xray-divide-seg-master", dspttk_long_to_string(pstCfgXray->stPackagePrm.u32segMaster, aString, sizeof(aString)));
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xray-rm-blank-slice-enable", pstCfgXray->stRmBlankSlice.bEnable);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xray-rm-blank-slice-ratio", "value", dspttk_long_to_string(pstCfgXray->stRmBlankSlice.uiLevel, aString, sizeof(aString)));
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xray-trans-type-bmp", pstCfgXray->bTransTypeEn[0]);
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xray-trans-type-jpg", pstCfgXray->bTransTypeEn[1]);
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xray-trans-type-gif", pstCfgXray->bTransTypeEn[2]);
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xray-trans-type-png", pstCfgXray->bTransTypeEn[3]);
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xray-trans-type-tiff",pstCfgXray->bTransTypeEn[4]);
    sRet |= dspttk_html_set_radio_by_name(pHtmlHandle, "xray-color-imaging", dspttk_long_to_string(pstCfgXray->stColorImage.enColorsImaging, aString, sizeof(aString)));
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xray-rt-input-delay-time", "value", dspttk_long_to_string(pstCfgXray->u32InputDelayTime, aString, sizeof(aString)));
    // sRet |= dspttk_html_set_radio_by_name(pHtmlHandle, "xray-xraw-store-enable", dspttk_long_to_string(pstCfgXray->enXrawStore, aString, sizeof(aString)));

    if (sRet == SAL_FAIL)
    {
        PRT_INFO("error setting xray.\n");
    }

    return sRet;
}

/**
 * @fn      dspttk_build_panel_setting_sva
 * @brief   根据配置文件中的参数构建Setting-SVA区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgOsd 全局配置参数中的OSD参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_sva(VOID *pHtmlHandle, DSPTTK_SETTING_SVA *pstCfgSva)
{
    SAL_STATUS sRet = SAL_SOK;
    sRet = dspttk_html_set_checkbox_by_name(pHtmlHandle, "sva-init-switch", pstCfgSva->enSvaInit);
    sRet = dspttk_html_set_checkbox_by_name(pHtmlHandle, "sva-pack-conse", pstCfgSva->enSvaPackConse);

    if (sRet == SAL_FAIL)
    {
        PRT_INFO("error setting osd.\n");
    }

    return sRet;
}


/**
 * @fn      dspttk_build_panel_setting_osd
 * @brief   根据配置文件中的参数构建Setting-OSD区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgOsd 全局配置参数中的OSD参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_osd(VOID *pHtmlHandle, DSPTTK_SETTING_OSD *pstCfgOsd)
{
    SAL_STATUS sRet = SAL_SOK;
    CHAR aString[24] = {0};

    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "osd-all-disp-enable", pstCfgOsd->enOsdAllDisp);
    sRet |= dspttk_html_set_select_by_name(pHtmlHandle, "osd-line-type", \
                dspttk_long_to_string(pstCfgOsd->enBorderType, aString, sizeof(aString)));
    sRet |= dspttk_html_set_select_by_name(pHtmlHandle, "osd-disp-color",pstCfgOsd->cDispColor);
    sRet |= dspttk_html_set_select_by_name(pHtmlHandle, "osd-line-attr", \
                dspttk_long_to_string(pstCfgOsd->stDispLinePrm.frametype, aString, sizeof(aString)));
    sRet |= dspttk_html_set_select_by_name(pHtmlHandle, "osd-scale-level", \
                dspttk_long_to_string(pstCfgOsd->u32ScaleLevel, aString, sizeof(aString)));
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "osd-combine-alert-name", pstCfgOsd->stDispLinePrm.emDispAIDrawType);
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "osd-conf-disp-enable", pstCfgOsd->enOsdConfDisp);

    if (sRet == SAL_FAIL)
    {
        PRT_INFO("error setting osd.\n");
    }

    return sRet;
}

/**
 * @fn      dspttk_build_panel_setting_xpack
 * @brief   根据配置文件中的参数构建Setting-OSD区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgOsd 全局配置参数中的OSD参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_xpack(VOID *pHtmlHandle, DSPTTK_SETTING_XPACK *pstCfgXpack)
{
    SAL_STATUS sRet = SAL_SOK;
    CHAR aString[24] = {0};
    /* 整包jpg */
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xpack-jpg-draw-switch", pstCfgXpack->stXpackJpgSet.bJpgDrawSwitch);
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xpack-jpg-crop-switch", pstCfgXpack->stXpackJpgSet.bJpgCropSwitch);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xpack-jpg-back-width", "value",\
                             dspttk_long_to_string(pstCfgXpack->stXpackJpgSet.u32JpgBackWidth, aString, sizeof(aString)));
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xpack-jpg-back-height", "value",\
                             dspttk_long_to_string(pstCfgXpack->stXpackJpgSet.u32JpgBackHeight, aString, sizeof(aString)));
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xpack-jpg-width-ratio", "value",\
                             dspttk_long_to_string(pstCfgXpack->stXpackJpgSet.f32WidthResizeRatio, aString, sizeof(aString)));
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xpack-jpg-height-ratio", "value",\
                             dspttk_long_to_string(pstCfgXpack->stXpackJpgSet.f32HeightResizeRatio, aString, sizeof(aString)));
    /* yuv偏移 */
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xpack-yuv-reset", pstCfgXpack->stXpackYuvOffset.reset);
    sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, "xpack-yuv-offset", "value",\
                             dspttk_long_to_string(pstCfgXpack->stXpackYuvOffset.offset, aString, sizeof(aString)));
    /* 包裹分片设置 */
    sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, "xpack-segment-enable", pstCfgXpack->stSegmentSet.bSegFlag);
    sRet |= dspttk_html_set_radio_by_name(pHtmlHandle, "xpack-segment-type", \
                            dspttk_long_to_string(pstCfgXpack->stSegmentSet.SegmentDataTpyeTpye, aString, sizeof(aString)));
    /* 主辅视角同步 */
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("error setting xpack.\n");
    }

    return sRet;
}

/**
 * @fn      dspttk_build_panel_setting_disp
 * @brief   根据配置文件中的参数构建Setting-Disp区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgDisp 全局配置参数中的Disp参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_disp(VOID *pHtmlHandle, DSPTTK_SETTING_DISP *pstCfgDisp)
{
    SAL_STATUS sRet = SAL_SOK;
    CHAR aString[24] = {0};
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    /* vodev为1表示当前支持的输出设备数为1
     * winNum为8表示当前每个vodev下最大支持8个可配置窗口
     */
    UINT32 i = 0, j = 0, winNum = 8;

    CHAR szEnFB[MAX_VODEV_CNT][SZ_BUF_SIZE] = {0};
    CHAR szEnMouse[MAX_VODEV_CNT][SZ_BUF_SIZE] = {0};
    CHAR szMouseX[MAX_VODEV_CNT][SZ_BUF_SIZE] = {0};
    CHAR szMouseY[MAX_VODEV_CNT][SZ_BUF_SIZE] = {0};
    CHAR szWinEn[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szWinChan[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szWinDistX[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szWinDistY[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szWinDistW[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szWinDistH[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szInSrcMode[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szInSrcChan[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szBorderEn[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szBorderWidth[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szEnlargeGlobalX[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szEnlargeGlobalY[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szEnlargeGlobalRatio[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szEnlargeLocalX[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szEnlargeLocalY[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szEnlargeLocalRatio[MAX_VODEV_CNT * MAX_DISP_CHAN][SZ_BUF_SIZE] = {0};

    /* 参照全局配置参数中的Disp参数对html页面表单进行配置：
     * 
     * 其中fb和mouse只需要最多配置MAX_VODEV_CNT次。
     * 其他窗口内相关参数，根据表单信息，每个显示设备下有8个窗口，因此win参数最多需要配置MAX_VODEV_CNT*MAX_DISP_WINDOW_NUM次
     * 
     * u32CfgCnt表示当前参数在astFormCfgMap中的索引
     * i表示当前所在显示设备序号
     * j表示当前所在窗口序号
    */
    for(i = 0; i < pstDevCfg->stInitParam.stDisplay.voDevCnt; i++)
    {
        snprintf(szEnFB[i], sizeof(szEnFB[i]), "disp-vodev%d-enfb", i);
        sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, szEnFB[i], pstCfgDisp->stFBParam[i].bFBEn);

        snprintf(szEnMouse[i], sizeof(szEnMouse[i]), "disp-vodev%d-enmouse", i);
        sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, szEnMouse[i], pstCfgDisp->stFBParam[i].bMouseEn);

        snprintf(szMouseX[i], sizeof(szMouseX[i]), "disp-vodev%d-mouse-x", i);
        sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szMouseX[i], "value", dspttk_long_to_string(pstCfgDisp->stFBParam[i].stMousePos.x, aString, sizeof(aString)));

        snprintf(szMouseY[i], sizeof(szMouseY[i]), "disp-vodev%d-mouse-y", i);
        sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szMouseY[i], "value", dspttk_long_to_string(pstCfgDisp->stFBParam[i].stMousePos.y, aString, sizeof(aString)));

        for(j = 0; j < winNum; j++)
        {   
        /*
         * snprintf中j+(i)*winNum表示当前显示设备号和窗口号组合的索引
         * 比如:当前sz_win_en[16]共有16组数据，其中vodev1下的8个窗口对应sz_win_en下的前8个索引[0-7]，vodev2下的8个窗口对应sz_win_en下的后8个索引[85]; 
        */
            snprintf(szWinEn[j+(i)*winNum], sizeof(szWinEn[j+(i)*winNum]), "disp-vodev%d-win%d-en",i,j);
            sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, szWinEn[j+(i)*winNum], pstCfgDisp->stWinParam[i][j].enable);

            snprintf(szWinChan[j+(i)*winNum], sizeof(szWinChan[j+(i)*winNum]), "disp-vodev%d-win%d-chan",i,j);
            sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szWinChan[j+(i)*winNum], "value", dspttk_long_to_string(pstCfgDisp->stWinParam[i][j].stDispRegion.uiChan, aString, sizeof(aString)));

            snprintf(szInSrcMode[j+(i)*winNum], sizeof(szInSrcMode[j+(i)*winNum]), "disp-vodev%d-win%d-input-src",i,j);
            sRet |= dspttk_html_set_select_by_name(pHtmlHandle, szInSrcMode[j+(i)*winNum], dspttk_long_to_string(pstCfgDisp->stInSrcParam[i][j].enInSrcMode, aString, sizeof(aString)));

            snprintf(szInSrcChan[j+(i)*winNum], sizeof(szInSrcChan[j+(i)*winNum]), "disp-vodev%d-win%d-input-chan",i,j);
            sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szInSrcChan[j+(i)*winNum], "value", dspttk_long_to_string(pstCfgDisp->stInSrcParam[i][j].u32SrcChn, aString, sizeof(aString)));

            snprintf(szWinDistX[j+(i)*winNum], sizeof(szWinDistX[j+(i)*winNum]), "disp-vodev%d-win%d-dist-x",i,j);
            sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szWinDistX[j+(i)*winNum], "value", dspttk_long_to_string(pstCfgDisp->stWinParam[i][j].stDispRegion.x, aString, sizeof(aString)));

            snprintf(szWinDistY[j+(i)*winNum], sizeof(szWinDistY[j+(i)*winNum]), "disp-vodev%d-win%d-dist-y",i,j);
            sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szWinDistY[j+(i)*winNum], "value", dspttk_long_to_string(pstCfgDisp->stWinParam[i][j].stDispRegion.y, aString, sizeof(aString)));

            snprintf(szWinDistW[j+(i)*winNum], sizeof(szWinDistW[j+(i)*winNum]), "disp-vodev%d-win%d-dist-w",i,j);
            sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szWinDistW[j+(i)*winNum], "value", dspttk_long_to_string(pstCfgDisp->stWinParam[i][j].stDispRegion.w, aString, sizeof(aString)));

            snprintf(szWinDistH[j+(i)*winNum], sizeof(szWinDistH[j+(i)*winNum]), "disp-vodev%d-win%d-dist-h",i,j);
            sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szWinDistH[j+(i)*winNum], "value", dspttk_long_to_string(pstCfgDisp->stWinParam[i][j].stDispRegion.h, aString, sizeof(aString)));

            snprintf(szBorderEn[j+(i)*winNum], sizeof(szBorderEn[j+(i)*winNum]), "disp-vodev%d-win%d-border-en",i,j);
            sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, szBorderEn[j+(i)*winNum], pstCfgDisp->stWinParam[i][j].stDispRegion.bDispBorder);

            snprintf(szBorderWidth[j+(i)*winNum], sizeof(szBorderWidth[j+(i)*winNum]), "disp-vodev%d-win%d-border-width",i,j);
            sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szBorderWidth[j+(i)*winNum], "value", dspttk_long_to_string(pstCfgDisp->stWinParam[i][j].stDispRegion.BorDerLineW, aString, sizeof(aString)));

            snprintf(szEnlargeGlobalX[j + (i)*winNum], sizeof(szEnlargeGlobalX[j+(i)*winNum]), "disp-vodev%d-win%d-enlarge-global-x",i,j);
            sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szEnlargeGlobalX[j + (i)*winNum], "value", dspttk_long_to_string(pstCfgDisp->stGlobalEnlargePrm[i][j].x, aString, sizeof(aString)));

            snprintf(szEnlargeGlobalY[j + (i)*winNum], sizeof(szEnlargeGlobalY[j+(i)*winNum]), "disp-vodev%d-win%d-enlarge-global-y",i,j);
            sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szEnlargeGlobalY[j + (i)*winNum], "value", dspttk_long_to_string(pstCfgDisp->stGlobalEnlargePrm[i][j].y, aString, sizeof(aString)));

            snprintf(szEnlargeGlobalRatio[j + (i)*winNum], sizeof(szEnlargeGlobalRatio[j+(i)*winNum]), "disp-vodev%d-win%d-enlarge-global-ratio",i,j);
            sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szEnlargeGlobalRatio[j + (i)*winNum], "value", dspttk_long_to_string(pstCfgDisp->stGlobalEnlargePrm[i][j].ratio, aString, sizeof(aString)));

            snprintf(szEnlargeLocalX[j + (i)*winNum], sizeof(szEnlargeLocalX[j+(i)*winNum]), "disp-vodev%d-win%d-enlarge-local-x",i,j);
            sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szEnlargeLocalX[j + (i)*winNum], "value", dspttk_long_to_string(pstCfgDisp->stLocalEnlargePrm[i][j].x, aString, sizeof(aString)));

            snprintf(szEnlargeLocalY[j + (i)*winNum], sizeof(szEnlargeLocalY[j+(i)*winNum]), "disp-vodev%d-win%d-enlarge-local-y",i,j);
            sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szEnlargeLocalY[j + (i)*winNum], "value", dspttk_long_to_string(pstCfgDisp->stLocalEnlargePrm[i][j].y, aString, sizeof(aString)));

            snprintf(szEnlargeLocalRatio[j + (i)*winNum], sizeof(szEnlargeLocalRatio[j+(i)*winNum]), "disp-vodev%d-win%d-enlarge-local-ratio",i,j);
            sRet |= dspttk_html_set_select_by_name(pHtmlHandle, szEnlargeLocalRatio[j + (i)*winNum], dspttk_long_to_string(pstCfgDisp->stLocalEnlargePrm[i][j].ratio, aString, sizeof(aString)));


        }
    }

    if (sRet == SAL_FAIL)
    {
        PRT_INFO("error setting disp.\n");
    }

    return sRet;
}

/**
 * @fn      dspttk_build_panel_setting_enc
 * @brief   根据配置文件中的参数构建Setting-Disp区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgEnc 全局配置参数中的Disp参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_enc(VOID *pHtmlHandle, DSPTTK_SETTING_ENC *pstCfgEnc)
{
    SAL_STATUS sRet = SAL_SOK;
    CHAR aString[24] = {0};
    UINT32 i = 0;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    
    CHAR szEnEncode[DSPTTK_ENC_STREAMTYPE_NUM][SZ_BUF_SIZE] = {0};
	CHAR szEnSave[DSPTTK_ENC_STREAMTYPE_NUM][SZ_BUF_SIZE] = {0};
    CHAR szStreamType[DSPTTK_ENC_STREAMTYPE_NUM][SZ_BUF_SIZE] = {0};
    CHAR szVideoType[DSPTTK_ENC_STREAMTYPE_NUM][SZ_BUF_SIZE] = {0};
    CHAR szAudioType[DSPTTK_ENC_STREAMTYPE_NUM][SZ_BUF_SIZE] = {0};
    CHAR szResolution[DSPTTK_ENC_STREAMTYPE_NUM][SZ_BUF_SIZE] = {0};
    CHAR szBpsType[DSPTTK_ENC_STREAMTYPE_NUM ][SZ_BUF_SIZE] = {0};
    CHAR szBps[DSPTTK_ENC_STREAMTYPE_NUM][SZ_BUF_SIZE] = {0};
    CHAR szQuant[DSPTTK_ENC_STREAMTYPE_NUM][SZ_BUF_SIZE] = {0};
    CHAR szFps[DSPTTK_ENC_STREAMTYPE_NUM][SZ_BUF_SIZE] = {0};
    CHAR szIFrameInterval[DSPTTK_ENC_STREAMTYPE_NUM][SZ_BUF_SIZE] = {0};
    CHAR szMuxType[DSPTTK_ENC_STREAMTYPE_NUM][SZ_BUF_SIZE] = {0};

    /* 参照全局配置参数中的Enc参数对html页面表单进行配置：
     * 
    */
    for(i = 0; i < pstDevCfg->stInitParam.stEncDec.encChanCnt; i++)
    {
        snprintf(szEnEncode[i], sizeof(szEnEncode[i]), "enc-en-%d", i);
        sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, szEnEncode[i], pstCfgEnc->stEncMultiParam[i].bStartEn);

		snprintf(szEnSave[i], sizeof(szEnSave[i]), "enc-save-%d", i);
        sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, szEnSave[i], pstCfgEnc->stEncMultiParam[i].bSaveEn);
		
        snprintf(szStreamType[i], sizeof(szStreamType[i]), "enc-streamtype-%d", i);
        sRet |= dspttk_html_set_select_by_name(pHtmlHandle, szStreamType[i], dspttk_long_to_string(pstCfgEnc->stEncMultiParam[i].stEncParam.streamType, aString, sizeof(aString)));

        snprintf(szVideoType[i], sizeof(szVideoType[i]), "enc-video-type-%d", i);
        sRet |= dspttk_html_set_select_by_name(pHtmlHandle, szVideoType[i], dspttk_long_to_string(pstCfgEnc->stEncMultiParam[i].stEncParam.videoType, aString, sizeof(aString)));

        snprintf(szAudioType[i], sizeof(szAudioType[i]), "enc-audio-type-%d", i);
        sRet |= dspttk_html_set_select_by_name(pHtmlHandle, szAudioType[i], dspttk_long_to_string(pstCfgEnc->stEncMultiParam[i].stEncParam.audioType, aString, sizeof(aString)));

        snprintf(szResolution[i], sizeof(szResolution[i]), "enc-resolution-%d", i);
        sRet |= dspttk_html_set_select_by_name(pHtmlHandle, szResolution[i], dspttk_long_to_string(pstCfgEnc->stEncMultiParam[i].stEncParam.resolution, aString, sizeof(aString)));

        snprintf(szBpsType[i], sizeof(szBpsType[i]), "enc-bps-type-%d", i);
        sRet |= dspttk_html_set_select_by_name(pHtmlHandle, szBpsType[i], dspttk_long_to_string(pstCfgEnc->stEncMultiParam[i].stEncParam.bpsType, aString, sizeof(aString)));

        snprintf(szBps[i], sizeof(szBps[i]), "enc-bps-%d", i);
        sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szBps[i], "value", dspttk_long_to_string(pstCfgEnc->stEncMultiParam[i].stEncParam.bps, aString, sizeof(aString)));

        snprintf(szQuant[i], sizeof(szQuant[i]), "enc-quant-%d", i);
        sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szQuant[i], "value", dspttk_long_to_string(pstCfgEnc->stEncMultiParam[i].stEncParam.quant, aString, sizeof(aString)));

        snprintf(szFps[i], sizeof(szFps[i]), "enc-fps-%d", i);
        sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szFps[i], "value", dspttk_long_to_string(pstCfgEnc->stEncMultiParam[i].stEncParam.fps, aString, sizeof(aString)));

        snprintf(szIFrameInterval[i], sizeof(szIFrameInterval[i]), "enc-Iframeinterval-%d", i);
        sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szIFrameInterval[i], "value", dspttk_long_to_string(pstCfgEnc->stEncMultiParam[i].stEncParam.IFrameInterval, aString, sizeof(aString)));

        snprintf(szMuxType[i], sizeof(szMuxType[i]), "enc-muxtype-%d", i);
        sRet |= dspttk_html_set_select_by_name(pHtmlHandle, szMuxType[i], dspttk_long_to_string(pstCfgEnc->stEncMultiParam[i].stEncParam.muxType, aString, sizeof(aString)));
    }

    if (sRet == SAL_FAIL)
    {
        PRT_INFO("error setting enc.\n");
    }

    return sRet;
}

/**
 * @fn      dspttk_build_panel_setting_dec
 * @brief   根据配置文件中的参数构建Setting-Dec区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgDec 全局配置参数中的Dec参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_dec(VOID *pHtmlHandle, DSPTTK_SETTING_DEC *pstCfgDec)
{
    SAL_STATUS sRet = SAL_SOK;
    CHAR aString[24] = {0};
    UINT32 i = 0;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();

    CHAR szENDecode[MAX_VDEC_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szENRecode[MAX_VDEC_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szStreamtype[MAX_VDEC_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szSpeedtype[MAX_VDEC_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szMode[MAX_VDEC_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szENCover[MAX_VDEC_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szENCoverIdx[MAX_VDEC_CHAN][SZ_BUF_SIZE] = {0};
    CHAR szReplayMode[MAX_VDEC_CHAN][SZ_BUF_SIZE] = {0};

    for(i = 0; i < pstDevCfg->stInitParam.stEncDec.decChanCnt; i++)
    {
        snprintf(szENDecode[i], sizeof(szENDecode[i]), "dec-en-chan%d", i);
        sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, szENDecode[i], pstCfgDec->stDecMultiParam[i].bDecode);

        snprintf(szENRecode[i], sizeof(szENRecode[i]), "dec-en-recode-chan%d", i);
        sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, szENRecode[i], pstCfgDec->stDecMultiParam[i].bRecode);

        snprintf(szStreamtype[i], sizeof(szStreamtype[i]), "dec-streamtype-chan%d", i);
        sRet |= dspttk_html_set_select_by_name(pHtmlHandle, szStreamtype[i], dspttk_long_to_string(pstCfgDec->stDecMultiParam[i].enMixStreamType, aString, sizeof(aString)));

        snprintf(szSpeedtype[i], sizeof(szSpeedtype[i]), "dec-speedtype-chan%d", i);
        sRet |= dspttk_html_set_select_by_name(pHtmlHandle, szSpeedtype[i], dspttk_long_to_string(pstCfgDec->stDecMultiParam[i].stDecAttrPrm.speed, aString, sizeof(aString)));
        
        snprintf(szMode[i], sizeof(szMode), "dec-mode-chan%d", i);
        sRet |= dspttk_html_set_select_by_name(pHtmlHandle, szMode[i], dspttk_long_to_string(pstCfgDec->stDecMultiParam[i].stDecAttrPrm.vdecMode, aString, sizeof(aString)));
        
        snprintf(szENCover[i], sizeof(szENCover[i]), "dec-en-coversign-chan%d", i);
        sRet |= dspttk_html_set_checkbox_by_name(pHtmlHandle, szENCover[i], pstCfgDec->stDecMultiParam[i].stCoverPrm.cover_sign);
        
        snprintf(szENCoverIdx[i], sizeof(szENCoverIdx[i]), "dec-coversign-index-chan%d", i);
        sRet |= dspttk_html_set_attr_by_name(pHtmlHandle, szENCoverIdx[i], "value", dspttk_long_to_string(pstCfgDec->stDecMultiParam[i].stCoverPrm.logo_pic_indx, aString, sizeof(aString)));
        
        snprintf(szReplayMode[i], sizeof(szReplayMode), "dec-replay-type-chan%d", i);
        sRet |= dspttk_html_set_select_by_name(pHtmlHandle, szReplayMode[i], dspttk_long_to_string(pstCfgDec->stDecMultiParam[i].enRepeat, aString, sizeof(aString)));
    }

    if(sRet == SAL_FAIL)
    {
        PRT_INFO("error setting dec.\n");
    }

    return sRet;

}


/**
 * @fn      dspttk_build_panel_setting
 * @brief   根据配置文件中的参数构建Setting区块的所有标签页参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstSettingParam 全局配置参数中的Setting参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting(VOID *pHtmlHandle, DSPTTK_SETTING_PARAM *pstSettingParam)
{
    SAL_STATUS sRet = SAL_SOK;

    sRet |= dspttk_build_panel_setting_env(pHtmlHandle, &pstSettingParam->stEnv);
    sRet |= dspttk_build_panel_setting_xray(pHtmlHandle, &pstSettingParam->stXray);
    sRet |= dspttk_build_panel_setting_xpack(pHtmlHandle, &pstSettingParam->stXpack);
    sRet |= dspttk_build_panel_setting_disp(pHtmlHandle, &pstSettingParam->stDisp);
    sRet |= dspttk_build_panel_setting_osd(pHtmlHandle, &pstSettingParam->stOsd);
    sRet |= dspttk_build_panel_setting_sva(pHtmlHandle, &pstSettingParam->stSva);
    sRet |= dspttk_build_panel_setting_enc(pHtmlHandle, &pstSettingParam->stEnc);
    sRet |= dspttk_build_panel_setting_dec(pHtmlHandle, &pstSettingParam->stDec);

    return sRet;
}


/**
 * @fn      dspttk_build_panels
 * @brief   根据配置文件中的参数构建各块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstDevCfg 全局配置参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panels(VOID *pHtmlHandle, DSPTTK_DEVCFG *pstDevCfg)
{
    SAL_STATUS sRet = SAL_FAIL;

    if (NULL == pHtmlHandle || NULL == pstDevCfg)
    {
        PRT_INFO("pHtmlHandle(%p) OR pstDevCfg(%p) is NULL\n,", pHtmlHandle, pstDevCfg);
        return SAL_FAIL;
    }

    sRet = dspttk_build_panel_init(pHtmlHandle, &pstDevCfg->stInitParam);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_build_panel_init failed\n");
        return SAL_FAIL;
    }

    sRet = dspttk_build_panel_setting(pHtmlHandle, &pstDevCfg->stSettingParam);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_build_panel_setting failed\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}
