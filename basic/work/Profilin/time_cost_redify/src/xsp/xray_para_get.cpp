#include "xray_image_process.hpp"

void XRayImageProcess::RegistParaGetMap()
{
	/******************************************************************************************
	*                                        参数获取
	*******************************************************************************************/
	/* 图像校正参数 */
	m_mapParaGet.insert({ XRAYLIB_PARAM_WHITENUP, &XRayImageProcess::GetWhitenUpNum });
	m_mapParaGet.insert({ XRAYLIB_PARAM_WHITENDOWN, &XRayImageProcess::GetWhitenDownNum });
	m_mapParaGet.insert({ XRAYLIB_PARAM_BACKGROUNDTHRESHOLED, &XRayImageProcess::GetBackgroundThreshold });
	m_mapParaGet.insert({ XRAYLIB_PARAM_BACKGROUNDGRAY, &XRayImageProcess::GetBackgroundGray });
	m_mapParaGet.insert({ XRAYLIB_PARAM_RTUPDATEAIRRATIO, &XRayImageProcess::GetRTUpdateAirRatio });
	m_mapParaGet.insert({ XRAYLIB_PARAM_AIRTHRESHOLDHIGH, &XRayImageProcess::GetEmptyThresholdH });
	m_mapParaGet.insert({ XRAYLIB_PARAM_AIRTHRESHOLDLOW, &XRayImageProcess::GetEmptyThresholdL });
	m_mapParaGet.insert({ XRAYLIB_PARAM_RTUPDATE_LOWGRAY_RATIO, &XRayImageProcess::GetRTUpdateLowGrayRatio });
	m_mapParaGet.insert({ XRAYLIB_PARAM_PACKAGEGRAY, &XRayImageProcess::GetPackageThreshold });
	m_mapParaGet.insert({ XRAYLIB_PARAM_IMAGEWIDTH, &XRayImageProcess::GetLibXRayDefaultImageWidth });
	m_mapParaGet.insert({ XRAYLIB_PARAM_COLDHOTTHRESHOLED, &XRayImageProcess::GetColdHotCaliThreshold });
	m_mapParaGet.insert({ XRAYLIB_PARAM_BELTGRAINLIMIT, &XRayImageProcess::GetBeltGrainLimit });

	/* 图像处理参数	*/
	m_mapParaGet.insert({ XRAYLIB_PARAM_DUAL_FILTERKERNEL_LENGTH, &XRayImageProcess::GetDualFilterKernelLength });
	m_mapParaGet.insert({ XRAYLIB_PARAM_DUALFILTER_RANGE, &XRayImageProcess::GetDualFilterRange });
	m_mapParaGet.insert({ XRAYLIB_PARAM_DUALDISTINGUISH_GRAYUP, &XRayImageProcess::GetDualDistinguishGrayUp });
	m_mapParaGet.insert({ XRAYLIB_PARAM_DUALDISTINGUISH_GRAYDOWN, &XRayImageProcess::GetDualDistinguishGrayDown });
	m_mapParaGet.insert({ XRAYLIB_PARAM_DUALDISTINGUISH_NONEINORGANIC, &XRayImageProcess::GetDualDistinguishNoneInorganic });
	m_mapParaGet.insert({ XRAYLIB_PARAM_DEFAUL_ENHANCE, &XRayImageProcess::GetDefaultEnhance });
	m_mapParaGet.insert({ XRAYLIB_PARAM_DEFAULTENHANCE_INTENSITY, &XRayImageProcess::GetDefaultEnhanceIntensity });
	m_mapParaGet.insert({ XRAYLIB_PARAM_EDGE1ENHANCE_INTENSITY, &XRayImageProcess::GetEdgeEnhanceIntensity });
	m_mapParaGet.insert({ XRAYLIB_PARAM_SUPERENHANCE_INTENSITY, &XRayImageProcess::GetSuperEnhanceIntensity });
	m_mapParaGet.insert({ XRAYLIB_PARAM_SPECIALENHANCE_THRESHOLDUP, &XRayImageProcess::GetSpecialEnhanceThresholdUp });
	m_mapParaGet.insert({ XRAYLIB_PARAM_SPECIALENHANCE_THRESHOLDDOWN, &XRayImageProcess::GetSpecialEnhanceThresholdDown });
	m_mapParaGet.insert({ XRAYLIB_PARAM_SPECIALENHANCE_STRETCHGRAYUP, &XRayImageProcess::GetSpecialEnhanceStretchGrayUp });
	m_mapParaGet.insert({ XRAYLIB_PARAM_SPECIALENHANCE_STRETCHGRAYDOWN, &XRayImageProcess::GetSpecialEnhanceStretchGrayDown });
	m_mapParaGet.insert({ XRAYLIB_PARAM_CALILOWGRAY_THRESHOLD, &XRayImageProcess::GetCaliLowGrayThreshold });
	m_mapParaGet.insert({ XRAYLIB_PARAM_DENOISING_INTENSITY, &XRayImageProcess::GetDenoisingIntensity });
	m_mapParaGet.insert({ XRAYLIB_PARAM_COMPOSITIVE_RATIO, &XRayImageProcess::GetCompositiveRatio });
	m_mapParaGet.insert({ XRAYLIB_PARAM_DENOISING_MODE, &XRayImageProcess::GetDenoisingMode });
	m_mapParaGet.insert({ XRAYLIB_PARAM_ANTIALIASING_MODE, &XRayImageProcess::GetAntialiasingMode });
	m_mapParaGet.insert({ XRAYLIB_PARAM_AIDEVICE_MODE, &XRayImageProcess::GetAIDeviceMode });
	m_mapParaGet.insert({ XRAYLIB_PARAM_HEIGHT_RESIZE_FACTOR, &XRayImageProcess::GetResizeHeightFactor });
	m_mapParaGet.insert({ XRAYLIB_PARAM_WIDTH_RESIZE_FACTOR, &XRayImageProcess::GetResizeWidthFactor });
	m_mapParaGet.insert({ XRAYLIB_PARAM_FUSION_MODE, &XRayImageProcess::GetFusionMode });
	m_mapParaGet.insert({ XRAYLIB_PARAM_BACKGROUNDCOLOR, &XRayImageProcess::GetBackgoundColor });
	m_mapParaGet.insert({ XRAYLIB_PARAM_STRETCHRATIO, &XRayImageProcess::GetStretchRatio });
	m_mapParaGet.insert({ XRAYLIB_PARAM_GAMMA_MODE, &XRayImageProcess::GetGammaMode });
	m_mapParaGet.insert({ XRAYLIB_PARAM_GAMMA_INTENSITY, &XRayImageProcess::GetGammaIntensity });
	m_mapParaGet.insert({ XRAYLIB_PARAM_SIGMA_NOISES, &XRayImageProcess::GetSigmaNoise });
	// m_mapParaGet.insert({ XRAYLIB_PARAM_PENE_INTENSITY, &XRayImageProcess::GetPeneMergeParam });
	m_mapParaGet.insert({ XRAYLIB_PARAM_MERGE_BASELINE, &XRayImageProcess::GetMergeBaseLine });
	m_mapParaGet.insert({ XRAYLIB_PARAM_TEST_MODE, &XRayImageProcess::GetTestMode });
	m_mapParaGet.insert({ XRAYLIB_PARAM_TCSTRIPENUM, &XRayImageProcess::GetTcStripeNum });
	m_mapParaGet.insert({ XRAYLIB_PARAM_TCENHANCE_INTENSITY, &XRayImageProcess::GetTcEnhanceIntensity });
	m_mapParaGet.insert({ XRAYLIB_PARAM_LUMINANCE_INTENSITY, &XRayImageProcess::GetLumIntensity });
	m_mapParaGet.insert({ XRAYLIB_PARAM_CONTRANST_INTENSITY, &XRayImageProcess::GetContrastRatio });
	m_mapParaGet.insert({ XRAYLIB_PARAM_SHARPNESS_INTENSITY, &XRayImageProcess::GetSharpnessRatio });
	m_mapParaGet.insert({ XRAYLIB_PARAM_LOW_COMPENSATION, &XRayImageProcess::GetLowLumCompensation });
	m_mapParaGet.insert({ XRAYLIB_PARAM_HIGH_SENSITIVITY, &XRayImageProcess::GetHighLumSensity });
	
	/* 功能类参数 */
	m_mapParaGet.insert({ XRAYLIB_PARAM_ROTATE, &XRayImageProcess::GetRGBRotate });
	m_mapParaGet.insert({ XRAYLIB_PARAM_MIRROR, &XRayImageProcess::GetRGBMirror });
	m_mapParaGet.insert({ XRAYLIB_PARAM_SCAN_MODE, &XRayImageProcess::GetScanMode });
	m_mapParaGet.insert({ XRAYLIB_PARAM_MATERIAL_ADJUST, &XRayImageProcess::GetMaterialAdjust });
	m_mapParaGet.insert({ XRAYLIB_PARAM_GEOMETRIC_CALI, &XRayImageProcess::GetGeomertricCali });
	m_mapParaGet.insert({ XRAYLIB_PARAM_GEOMETRIC_INVERSE ,&XRayImageProcess::GetGeoCaliInverse });
	m_mapParaGet.insert({ XRAYLIB_PARAM_LOWPENETRATION_PROMPT, &XRayImageProcess::GetLowPenetrationPrompt });
	m_mapParaGet.insert({ XRAYLIB_PARAM_LOWPENETRATION_SENSTIVTY, &XRayImageProcess::GetLowPeneSenstivity });
	m_mapParaGet.insert({ XRAYLIB_PARAM_LOWPENETHRESHOLD, &XRayImageProcess::GetLowPenetrationThreshold });
	m_mapParaGet.insert({ XRAYLIB_PARAM_LOWPENEWARNTHRESHOLD, &XRayImageProcess::GetLowPenetrationWarnThreshold });
	m_mapParaGet.insert({ XRAYLIB_PARAM_CONCERDMATERIAL_PROMPT, &XRayImageProcess::GetConcernedMPrompt });
	m_mapParaGet.insert({ XRAYLIB_PARAM_CONCERDMATERIAL_SENSTIVTY, &XRayImageProcess::GetConcernedMSnestivity });
	m_mapParaGet.insert({ XRAYLIB_PARAM_FLATDETMETRIC_CALI, &XRayImageProcess::GetFlatDetmertricCali });
	m_mapParaGet.insert({ XRAYLIB_PARAM_SPEED, &XRayImageProcess::GetSpeed });
	m_mapParaGet.insert({ XRAYLIB_PARAM_RT_HEIGHT, &XRayImageProcess::GetRTHeight });
	m_mapParaGet.insert({ XRAYLIB_PARAM_SPEEDGEAR, &XRayImageProcess::GetSpeedGear });
	m_mapParaGet.insert({ XRAYLIB_PARAM_COLDANDHOT_CALI, &XRayImageProcess::GetColdAndHotCali });
	m_mapParaGet.insert({ XRAYLIB_PARAM_DOSE_CALI, &XRayImageProcess::GetDoseCali });
	m_mapParaGet.insert({ XRAYLIB_PARAM_DRUGMATERIAL_PROMPT, &XRayImageProcess::GetDrugPrompt });
	m_mapParaGet.insert({ XRAYLIB_PARAM_EXPLOSIVEMATERIAL_PROMPT, &XRayImageProcess::GetExplosivePrompt });
	m_mapParaGet.insert({ XRAYLIB_PARAM_CURVE_STATE, &XRayImageProcess::GetCurveState });
	m_mapParaGet.insert({ XRAYLIB_PARAM_EUTEST_PROMPT, &XRayImageProcess::GetEuTestPrompt });
	m_mapParaGet.insert({ XRAYLIB_PARAM_LOGSAVEMODE, &XRayImageProcess::GetLogSaveMode });
	m_mapParaGet.insert({ XRAYLIB_PARAM_LOGLEVEL, &XRayImageProcess::GetLogLevel });
	m_mapParaGet.insert({ XRAYLIB_PARAM_EXPLOSIVEDETECTION_MODE, &XRayImageProcess::GetDetectionMode });
	m_mapParaGet.insert({ XRAYLIB_PARAM_GEOMETRIC_CALI_RATIO, &XRayImageProcess::GetGeometricRatio });
}

void XRayImageProcess::RegistImageGetMap()
{
	/******************************************************************************************
	*                                      图像状态获取
	*******************************************************************************************/
	m_mapImageGet.insert({ XRAYLIB_ENERGY, &XRayImageProcess::GetEnergyMode });
	m_mapImageGet.insert({ XRAYLIB_COLOR, &XRayImageProcess::GetColorMode });
	m_mapImageGet.insert({ XRAYLIB_PENETRATION, &XRayImageProcess::GetPenetrationMode });
	m_mapImageGet.insert({ XRAYLIB_ENHANCE, &XRayImageProcess::GetEnahnceMode });
	m_mapImageGet.insert({ XRAYLIB_INVERSE, &XRayImageProcess::GetInverseMode });
	m_mapImageGet.insert({ XRAYLIB_GRAYLEVEL, &XRayImageProcess::GetGrayLevel });
	m_mapImageGet.insert({ XRAYLIB_BRIGHTNESS, &XRayImageProcess::GetBrightness });
	m_mapImageGet.insert({ XRAYLIB_SINGLECOLORTABLE, &XRayImageProcess::GetSingleColorTableSel });
	m_mapImageGet.insert({ XRAYLIB_DUALCOLORTABLE, &XRayImageProcess::GetDualColorTableSel });
	m_mapImageGet.insert({ XRAYLIB_COLORSIMAGING, &XRayImageProcess::GetColorImagingSel });
}

/******************************************************************************************
*                                      图像状态获取
*******************************************************************************************/

XRAY_LIB_HRESULT XRayImageProcess::GetEnergyMode(XRAYLIB_CONFIG_IMAGE* stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetEnergyMode para is null.");

	*(XRAYLIB_ENERGY_MODE*)stPara->value = m_sharedPara.m_enEnergyMode;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetColorMode(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetColorMode para is null.");

	*(XRAYLIB_COLOR_MODE*)stPara->value = m_modDual.m_enColorMode;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetPenetrationMode(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetPenetrationMode para is null.");

	*(XRAYLIB_PENETRATION_MODE*)stPara->value = m_modDual.m_enPenetrationMode;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetEnahnceMode(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetEnahnceMode para is null.");

	*(XRAYLIB_ENHANCE_MODE*)stPara->value = m_modImgproc.m_enEnhanceMode;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetInverseMode(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetInverseMode para is null.");

	*(XRAYLIB_INVERSE_MODE*)stPara->value = m_modDual.m_enInverseMode;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetGrayLevel(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetGrayLevel para is null.");

	*(int*)stPara->value = m_modDual.m_nGrayLevel;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetBrightness(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetBrightness para is null.");

	int nBaseLevel = XRAY_LIB_BRIGHTNESSLEVEL / 2;
	int nBrightNessStep = 65535 / nBaseLevel;

	int nBrightnessLevel = m_modDual.m_nBrightnessAdjust / nBrightNessStep + nBaseLevel;
	*(int*)stPara->value = nBrightnessLevel;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetSingleColorTableSel(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetSingleColorTableSel para is null.");

	*(XRAYLIB_SINGLECOLORTABLE_SEL*)stPara->value = m_modDual.m_enSingleColorTableSel;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetDualColorTableSel(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetDualColorTableSel para is null.");

	*(XRAYLIB_DUALCOLORTABLE_SEL*)stPara->value = m_modDual.m_enDualColorTableSel;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetColorImagingSel(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetColorImagingSel para is null.");

	*(XRAYLIB_COLORSIMAGING_SEL*)stPara->value = m_modDual.m_enColorImagingSel;
	return XRAY_LIB_OK;
}

/******************************************************************************************
*                                        参数获取
*******************************************************************************************/

XRAY_LIB_HRESULT XRayImageProcess::GetWhitenUpNum(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetWhitenUpNum para is null.");

	stPara->numeratorValue = m_modCali.m_nWhitenUp;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetWhitenDownNum(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetWhitenDownNum para is null.");

	stPara->numeratorValue = m_modCali.m_nWhitenDown;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetBackgroundThreshold(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetBackgroundThreshold para is null.");

	stPara->numeratorValue = m_sharedPara.m_nBackGroundThreshold;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetBackgroundGray(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetBackgroundGray para is null.");

	stPara->numeratorValue = m_sharedPara.m_nBackGroundGray;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetRTUpdateAirRatio(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetRTUpdateAirRatio para is null.");

	int num = 10000;
	stPara->numeratorValue = int(m_modCali.m_fRTUpdateAirRatio * num);
	stPara->denominatorValue = num;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetEmptyThresholdH(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetEmptyThresholdH para is null.");

	stPara->numeratorValue = 0;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetEmptyThresholdL(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetEmptyThresholdL para is null.");

	stPara->numeratorValue = 0;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetRTUpdateLowGrayRatio(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetRTUpdateLowGrayRatio para is null.");

	stPara->numeratorValue = 0;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetPackageThreshold(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetPackageThreshold para is null.");

	stPara->numeratorValue = 0;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetLibXRayDefaultImageWidth(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetLibXRayDefaultImageWidth para is null.");

	stPara->numeratorValue = m_nDetectorWidth;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetColdHotCaliThreshold(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetColdHotCaliThreshold para is null.");

	stPara->numeratorValue = m_modCali.m_nCountThreshold;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetBeltGrainLimit(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetBeltGrainLimit para is null.");

	stPara->numeratorValue = m_modCali.m_nBeltGrainLimit;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}


XRAY_LIB_HRESULT XRayImageProcess::GetDualFilterKernelLength(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetDualFilterKernelLength para is null.");

	stPara->numeratorValue = m_modDual.m_nDualFilterKernelLength;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetDualFilterRange(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetDualFilterRange para is null.");

	int num = 10000;
	stPara->numeratorValue = int(m_modDual.m_fDualFilterRange * num);
	stPara->denominatorValue = num;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetDualDistinguishGrayUp(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetDualDistinguishGrayUp para is null.");

	stPara->numeratorValue = m_modDual.m_nDualDistinguishGrayUp;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetDualDistinguishGrayDown(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetDualDistinguishGrayDown para is null.");

	stPara->numeratorValue = m_modDual.m_nDualDistinguishGrayDown;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetDualDistinguishNoneInorganic(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetDualDistinguishNoneInorganic para is null.");

	stPara->numeratorValue = m_modDual.m_nDualDistinguishNoneInorganic;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetDefaultEnhance(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetDefaultEnhance para is null.");

	stPara->numeratorValue = (int)m_enDefaultEnhanceJudge;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetDefaultEnhanceIntensity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetDefaultEnhanceIntensity para is null.");

	int num = 10000;
	stPara->numeratorValue = int(m_modImgproc.m_fDefaultEnhanceIntensity * num);
	stPara->denominatorValue = num;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetEdgeEnhanceIntensity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetEdgeEnhanceIntensity para is null.");

	int num = 10000;
	stPara->numeratorValue = int(m_modImgproc.m_fEdgeEnhanceIntensity * num);
	stPara->denominatorValue = num;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetSuperEnhanceIntensity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetSuperEnhanceIntensity para is null.");

	int num = 10000;
	stPara->numeratorValue = int(m_modImgproc.m_fEdgeSuperIntensity * num);
	stPara->denominatorValue = num;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetSpecialEnhanceThresholdUp(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetSpecialEnhanceThresholdUp para is null.");

	stPara->numeratorValue = m_modImgproc.m_nLocalEnhanceThresholdUP;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetSpecialEnhanceThresholdDown(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetSpecialEnhanceThresholdDown para is null.");

	stPara->numeratorValue = m_modImgproc.m_nWindowCenter;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetSpecialEnhanceStretchGrayUp(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetSpecialEnhanceStretchGrayUp para is null.");

	stPara->numeratorValue = 0;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetSpecialEnhanceStretchGrayDown(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetSpecialEnhanceStretchGrayDown para is null.");

	stPara->numeratorValue = 0;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetCaliLowGrayThreshold(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetCaliLowGrayThreshold para is null.");

	stPara->numeratorValue = m_modImgproc.m_nCaliLowGrayThreshold;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetDenoisingIntensity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetDenoisingIntensity para is null.");

	stPara->numeratorValue = (int)m_modImgproc.m_enDenoisingIntensity;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetCompositiveRatio([[maybe_unused]]XRAYLIB_CONFIG_PARAM *stPara)
{
    return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetDenoisingMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetDenoisingMode para is null.");

	stPara->numeratorValue = (int)m_modImgproc.m_enDenoisingMode;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetAntialiasingMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetAntialiasingMode para is null.");

	stPara->numeratorValue = 0;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetAIDeviceMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetAIDeviceMode para is null.");

	stPara->numeratorValue = 0;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetResizeHeightFactor(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetResizeHeightFactor para is null.");

	int num = 10000;
	stPara->numeratorValue = int(m_fHeightResizeFactor * num);
	stPara->denominatorValue = num;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetResizeWidthFactor(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetResizeWidthFactor para is null.");

	int num = 10000;
	stPara->numeratorValue = int(m_fWidthResizeFactor * num);
	stPara->denominatorValue = num;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetFusionMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetFusionMode para is null.");

	stPara->numeratorValue = (int)m_modImgproc.m_enFusionMode;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetBackgoundColor(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetBackgoundColor para is null.");
	if (XRAYLIB_ENERGY_SCATTER == m_sharedPara.m_enEnergyMode)
	{
		stPara->numeratorValue = m_modDual.m_enInverseMode ? 0xFFFFFF : 0;
	}
	else
	{
		stPara->numeratorValue = (int)m_modDual.m_islPipeChroma.GetBgColor(m_modDual.m_enInverseMode);
	}
	
	stPara->denominatorValue = 1;

    return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetStretchRatio(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetStretchRatio para is null.");

	int num = 10000;
	stPara->numeratorValue = int(m_modCali.m_fStretchRatio * num);
	stPara->denominatorValue = num;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetGammaMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetGammaMode para is null.");

	stPara->numeratorValue = (int)m_modImgproc.m_enGammaMode;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetGammaIntensity([[maybe_unused]] XRAYLIB_CONFIG_PARAM *stPara)
{
	return XRAY_LIB_INVALID_PARAM;
}

XRAY_LIB_HRESULT XRayImageProcess::GetSigmaNoise(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetSigmaNoise para is null.");

	int num = 10000;
	stPara->numeratorValue = int(m_fSigmaDn * num);
	stPara->denominatorValue = num;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetPeneMergeParam(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetPeneParam para is null.");

	int num = 10000;
	stPara->numeratorValue = int(m_modDual.m_fpeneMergeParm * num);
	stPara->denominatorValue = num;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetMergeBaseLine(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetSigmaNoise para is null.");


	stPara->numeratorValue = m_modImgproc.m_nMergeBaseLine;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetTestMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetTestMode para is null.");

	stPara->numeratorValue = (int)m_modImgproc.m_enTestMode;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetRGBRotate(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetRGBRotate para is null.");

	stPara->numeratorValue = (int)m_enRotate;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetRGBMirror(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetRGBMirror para is null.");

	stPara->numeratorValue = (int)m_enMirror;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetScanMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetScanMode para is null.");

	stPara->numeratorValue = (int)m_enScanMode;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetMaterialAdjust(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetMaterialAdjust para is null.");

	int num = 10000;
	stPara->numeratorValue = int(m_modDual.m_fMaterialAdjust * num);
	stPara->denominatorValue = num;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetDoseCali(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetDoseCali para is null.");

	stPara->numeratorValue = (int)(m_modCali.m_enDoseCaliMertric);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetGeomertricCali(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetGeomertricCali para is null.");

	stPara->numeratorValue = (int)(m_modCali.m_enGeoMertric);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetFlatDetmertricCali(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetFlatDetmertricCali para is null.");

	stPara->numeratorValue = (int)(m_modCali.m_enFlatDetMertric);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetLowPenetrationPrompt(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetLowPenetrationPrompt para is null.");

	stPara->numeratorValue = (int)(m_modArea.m_enLowPenetrationPrompt);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetLowPeneSenstivity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetLowPeneSenstivity para is null.");

	stPara->numeratorValue = (int)(m_modArea.m_enLowPeneSenstivity);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetLowPenetrationThreshold(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetLowPenetrationThreshold para is null.");

	stPara->numeratorValue = (int)(m_modArea.m_nLowPenetrationThreshold);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetLowPenetrationWarnThreshold(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetLowPenetrationWarnThreshold para is null.");

	// TODO : 该参数是否保留
	stPara->numeratorValue = 0;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetConcernedMPrompt(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetConcernedMPrompt para is null.");

	stPara->numeratorValue = (int)(m_modArea.m_enConcernedMaterialPrompt);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetConcernedMSnestivity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetConcernedMSnestivity para is null.");

	stPara->numeratorValue = (int)(m_modArea.m_enConcernedMSenstivity);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetEexcutionMode([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	return XRAY_LIB_HRESULT();
}

XRAY_LIB_HRESULT XRayImageProcess::GetSpeed(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetSpeed para is null.");

	int num = 10000;
	stPara->numeratorValue = (int)(m_fSpeed * num);
	stPara->denominatorValue = num;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetRTHeight(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetRTHeight para is null.");

	stPara->numeratorValue = m_nRTHeight;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetTcStripeNum(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetTcStripeNum para is null.");

	stPara->numeratorValue = m_modTcproc.m_nStripeBufferNum;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetTcEnhanceIntensity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetTcEnhanceIntensity para is null.");

	stPara->numeratorValue = m_modTcproc.m_sTcProcParam.testAEnhanceGrade;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetLumIntensity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetLumIntensity para is null.");

	stPara->numeratorValue = m_modImgproc.m_nLumIntensity;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetContrastRatio(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetContrastRatio para is null.");

	stPara->numeratorValue = m_modImgproc.m_nContrastRatio;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetSharpnessRatio(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetSharpnessRatio para is null.");

	stPara->numeratorValue = m_modImgproc.m_nSharpnessRatio;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetLowLumCompensation(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetLowLumCompensation para is null.");

	stPara->numeratorValue = m_modImgproc.m_nLowLumCompensation;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetHighLumSensity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetHighLumSensity para is null.");

	stPara->numeratorValue = m_modImgproc.m_nHighLumSensity;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetIntegralTime([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	return XRAY_LIB_HRESULT();
}

XRAY_LIB_HRESULT XRayImageProcess::GetSpeedGear(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetSpeedGear para is null.");

	stPara->numeratorValue = (int)m_enSpeedGear;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetColdAndHotCali(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetColdAndHotCali para is null.");

	stPara->numeratorValue = (int)(m_modCali.m_enColdAndHot);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetGeoCaliInverse(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetGeoCaliInverse para is null.");

	stPara->numeratorValue = (int)(m_modCali.m_bGeoCaliInverse);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetDrugPrompt(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetDrugPrompt para is null.");

	stPara->numeratorValue = (int)(m_modArea.m_enDrugPrompt);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetExplosivePrompt(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetExplosivePrompt para is null.");

	stPara->numeratorValue = (int)(m_modArea.m_enExplosivePrompt);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetCurveState(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetCurveState para is null.");

	stPara->numeratorValue = (int)(m_modDual.m_enCurveState);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetEuTestPrompt(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetEuTestPrompt para is null.");

	stPara->numeratorValue = (int)(m_modImgproc.m_enTestAutoLE);
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetLogSaveMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetExplosivePrompt para is null.");

	stPara->numeratorValue = g_LogSaveMode;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetLogLevel(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetExplosivePrompt para is null.");

	stPara->numeratorValue = g_LogLevel;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetDetectionMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetDetectionMode para is null.");

	stPara->numeratorValue = (int)m_modArea.m_enDetectionMode;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetGeometricRatio(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "GetGeometricRatio para is null.");

	stPara->numeratorValue = (int)(1 - m_modCali.m_fGeomFactor) * 100;
	stPara->denominatorValue = 1;
	return XRAY_LIB_OK;
}