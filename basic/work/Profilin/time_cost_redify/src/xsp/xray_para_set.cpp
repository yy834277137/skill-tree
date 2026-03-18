#include "xray_image_process.hpp"

void XRayImageProcess::RegistParaSetMap()
{
	/******************************************************************************************
	*                                        参数设置
	*******************************************************************************************/
	/* 图像校正参数 */
	m_mapParaSet.insert({ XRAYLIB_PARAM_WHITENUP, &XRayImageProcess::SetWhitenUpNum });
	m_mapParaSet.insert({ XRAYLIB_PARAM_WHITENDOWN, &XRayImageProcess::SetWhitenDownNum });
	m_mapParaSet.insert({ XRAYLIB_PARAM_BACKGROUNDTHRESHOLED, &XRayImageProcess::SetBackgroundThreshold });
	m_mapParaSet.insert({ XRAYLIB_PARAM_BACKGROUNDGRAY, &XRayImageProcess::SetBackgroundGray });
	m_mapParaSet.insert({ XRAYLIB_PARAM_RTUPDATEAIRRATIO, &XRayImageProcess::SetRTUpdateAirRatio });
	m_mapParaSet.insert({ XRAYLIB_PARAM_AIRTHRESHOLDHIGH, &XRayImageProcess::SetEmptyThresholdH });
	m_mapParaSet.insert({ XRAYLIB_PARAM_AIRTHRESHOLDLOW, &XRayImageProcess::SetEmptyThresholdL });
	m_mapParaSet.insert({ XRAYLIB_PARAM_RTUPDATE_LOWGRAY_RATIO, &XRayImageProcess::SetRTUpdateLowGrayRatio });
	m_mapParaSet.insert({ XRAYLIB_PARAM_PACKAGEGRAY, &XRayImageProcess::SetPackageThreshold });
	m_mapParaSet.insert({ XRAYLIB_PARAM_IMAGEWIDTH, &XRayImageProcess::SetLibXRayDefaultImageWidth });
	m_mapParaSet.insert({ XRAYLIB_PARAM_COLDHOTTHRESHOLED, &XRayImageProcess::SetColdHotCaliThreshold });
	m_mapParaSet.insert({ XRAYLIB_PARAM_BELTGRAINLIMIT, &XRayImageProcess::SetBeltGrainLimit });

	/* 图像处理参数	*/
	m_mapParaSet.insert({ XRAYLIB_PARAM_DUAL_FILTERKERNEL_LENGTH, &XRayImageProcess::SetDualFilterKernelLength });
	m_mapParaSet.insert({ XRAYLIB_PARAM_DUALFILTER_RANGE, &XRayImageProcess::SetDualFilterRange });
	m_mapParaSet.insert({ XRAYLIB_PARAM_DUALDISTINGUISH_GRAYUP, &XRayImageProcess::SetDualDistinguishGrayUp });
	m_mapParaSet.insert({ XRAYLIB_PARAM_DUALDISTINGUISH_GRAYDOWN, &XRayImageProcess::SetDualDistinguishGrayDown });
	m_mapParaSet.insert({ XRAYLIB_PARAM_DUALDISTINGUISH_NONEINORGANIC, &XRayImageProcess::SetDualDistinguishNoneInorganic });
	m_mapParaSet.insert({ XRAYLIB_PARAM_DEFAUL_ENHANCE, &XRayImageProcess::SetDefaultEnhance });
	m_mapParaSet.insert({ XRAYLIB_PARAM_DEFAULTENHANCE_INTENSITY, &XRayImageProcess::SetDefaultEnhanceIntensity });
	m_mapParaSet.insert({ XRAYLIB_PARAM_EDGE1ENHANCE_INTENSITY, &XRayImageProcess::SetEdgeEnhanceIntensity });
	m_mapParaSet.insert({ XRAYLIB_PARAM_SUPERENHANCE_INTENSITY, &XRayImageProcess::SetSuperEnhanceIntensity });
	m_mapParaSet.insert({ XRAYLIB_PARAM_SPECIALENHANCE_THRESHOLDUP, &XRayImageProcess::SetSpecialEnhanceThresholdUp });
	m_mapParaSet.insert({ XRAYLIB_PARAM_SPECIALENHANCE_THRESHOLDDOWN, &XRayImageProcess::SetSpecialEnhanceThresholdDown });
	m_mapParaSet.insert({ XRAYLIB_PARAM_SPECIALENHANCE_STRETCHGRAYUP, &XRayImageProcess::SetSpecialEnhanceStretchGrayUp });
	m_mapParaSet.insert({ XRAYLIB_PARAM_SPECIALENHANCE_STRETCHGRAYDOWN, &XRayImageProcess::SetSpecialEnhanceStretchGrayDown });
	m_mapParaSet.insert({ XRAYLIB_PARAM_CALILOWGRAY_THRESHOLD, &XRayImageProcess::SetCaliLowGrayThreshold });
	m_mapParaSet.insert({ XRAYLIB_PARAM_DENOISING_INTENSITY, &XRayImageProcess::SetDenoisingIntensity });
	m_mapParaSet.insert({ XRAYLIB_PARAM_COMPOSITIVE_RATIO, &XRayImageProcess::SetCompositiveRatio });
	m_mapParaSet.insert({ XRAYLIB_PARAM_DENOISING_MODE, &XRayImageProcess::SetDenoisingMode });
	m_mapParaSet.insert({ XRAYLIB_PARAM_ANTIALIASING_MODE, &XRayImageProcess::SetAntialiasingMode });
	m_mapParaSet.insert({ XRAYLIB_PARAM_AIDEVICE_MODE, &XRayImageProcess::SetAIDeviceMode });
	m_mapParaSet.insert({ XRAYLIB_PARAM_HEIGHT_RESIZE_FACTOR, &XRayImageProcess::SetResizeHeightFactor });
	m_mapParaSet.insert({ XRAYLIB_PARAM_WIDTH_RESIZE_FACTOR, &XRayImageProcess::SetResizeWidthFactor });
	m_mapParaSet.insert({ XRAYLIB_PARAM_FUSION_MODE, &XRayImageProcess::SetFusionMode });
	m_mapParaSet.insert({ XRAYLIB_PARAM_BACKGROUNDCOLOR, &XRayImageProcess::SetBackgoundColor });
	m_mapParaSet.insert({ XRAYLIB_PARAM_STRETCHRATIO, &XRayImageProcess::SetStretchRatio });
	m_mapParaSet.insert({ XRAYLIB_PARAM_GAMMA_MODE, &XRayImageProcess::SetGammaMode });
	m_mapParaSet.insert({ XRAYLIB_PARAM_GAMMA_INTENSITY, &XRayImageProcess::SetGammaIntensity });
	m_mapParaSet.insert({ XRAYLIB_PARAM_SIGMA_NOISES, &XRayImageProcess::SetSigmaNoise });
	m_mapParaSet.insert({ XRAYLIB_PARAM_HIGH_PENE_INTENSITY, &XRayImageProcess::SetHighPeneRatio });
	m_mapParaSet.insert({ XRAYLIB_PARAM_LOW_PENE_INTENSITY, &XRayImageProcess::SetLowPeneRatio });
	m_mapParaSet.insert({ XRAYLIB_PARAM_MERGE_BASELINE, &XRayImageProcess::SetMergeBaseLine });
	m_mapParaSet.insert({ XRAYLIB_PARAM_TEST_MODE, &XRayImageProcess::SetTestMode });
	m_mapParaSet.insert({ XRAYLIB_PARAM_TCSTRIPENUM ,&XRayImageProcess::SetTcStripeNum });
	m_mapParaSet.insert({ XRAYLIB_PARAM_TCENHANCE_INTENSITY ,&XRayImageProcess::SetTcEnhanceIntensity });
	m_mapParaSet.insert({ XRAYLIB_PARAM_LUMINANCE_INTENSITY ,&XRayImageProcess::SetLuminance});
	m_mapParaSet.insert({ XRAYLIB_PARAM_CONTRANST_INTENSITY ,&XRayImageProcess::SetContrast});
	m_mapParaSet.insert({ XRAYLIB_PARAM_SHARPNESS_INTENSITY ,&XRayImageProcess::SetSharpnessIntensity});
	m_mapParaSet.insert({ XRAYLIB_PARAM_LOW_COMPENSATION ,&XRayImageProcess::SetLowCompensation});
	m_mapParaSet.insert({ XRAYLIB_PARAM_HIGH_SENSITIVITY ,&XRayImageProcess::SetHighSensitivity});
	
	/* 功能类参数 */
	m_mapParaSet.insert({ XRAYLIB_PARAM_ROTATE, &XRayImageProcess::SetRGBRotate });
	m_mapParaSet.insert({ XRAYLIB_PARAM_MIRROR, &XRayImageProcess::SetRGBMirror });
	m_mapParaSet.insert({ XRAYLIB_PARAM_SCAN_MODE, &XRayImageProcess::SetScanMode });
	m_mapParaSet.insert({ XRAYLIB_PARAM_MATERIAL_ADJUST, &XRayImageProcess::SetMaterialAdjust });
	m_mapParaSet.insert({ XRAYLIB_PARAM_GEOMETRIC_CALI, &XRayImageProcess::SetGeomertricCali });
	m_mapParaSet.insert({ XRAYLIB_PARAM_GEOMETRIC_INVERSE ,&XRayImageProcess::SetGeoCaliInverse });
	m_mapParaSet.insert({ XRAYLIB_PARAM_LOWPENETRATION_PROMPT, &XRayImageProcess::SetLowPenetrationPrompt });
	m_mapParaSet.insert({ XRAYLIB_PARAM_LOWPENETRATION_SENSTIVTY, &XRayImageProcess::SetLowPeneSenstivity });
	m_mapParaSet.insert({ XRAYLIB_PARAM_LOWPENETHRESHOLD, &XRayImageProcess::SetLowPenetrationThreshold });
	m_mapParaSet.insert({ XRAYLIB_PARAM_LOWPENEWARNTHRESHOLD, &XRayImageProcess::SetLowPenetrationWarnThreshold });
	m_mapParaSet.insert({ XRAYLIB_PARAM_CONCERDMATERIAL_PROMPT, &XRayImageProcess::SetConcernedMPrompt });
	m_mapParaSet.insert({ XRAYLIB_PARAM_CONCERDMATERIAL_SENSTIVTY, &XRayImageProcess::SetConcernedMSnestivity });
	m_mapParaSet.insert({ XRAYLIB_PARAM_FLATDETMETRIC_CALI, &XRayImageProcess::SetFlatDetmertricCali });
	m_mapParaSet.insert({ XRAYLIB_PARAM_SPEED, &XRayImageProcess::SetSpeed });
	m_mapParaSet.insert({ XRAYLIB_PARAM_RT_HEIGHT, &XRayImageProcess::SetRTHeight });
	m_mapParaSet.insert({ XRAYLIB_PARAM_COLDANDHOT_CALI, &XRayImageProcess::SetColdAndHotCali });
	m_mapParaSet.insert({ XRAYLIB_PARAM_DOSE_CALI, &XRayImageProcess::SetDoseCali});
	m_mapParaSet.insert({ XRAYLIB_PARAM_DRUGMATERIAL_PROMPT, &XRayImageProcess::SetDrugPrompt });
	m_mapParaSet.insert({ XRAYLIB_PARAM_EXPLOSIVEMATERIAL_PROMPT, &XRayImageProcess::SetExplosivePrompt });
	m_mapParaSet.insert({ XRAYLIB_PARAM_EUTEST_PROMPT, &XRayImageProcess::SetEuTestPrompt });
	m_mapParaSet.insert({ XRAYLIB_PARAM_LOGSAVEMODE, &XRayImageProcess::SetLogSaveMode });
	m_mapParaSet.insert({ XRAYLIB_PARAM_LOGLEVEL, &XRayImageProcess::SetLogLevel });
	m_mapParaSet.insert({ XRAYLIB_PARAM_TIMECOST_SHOW, &XRayImageProcess::SetTimeCostShow });
	m_mapParaSet.insert({ XRAYLIB_PARAM_EXPLOSIVEDETECTION_MODE, &XRayImageProcess::SetDetectionMode });
	m_mapParaSet.insert({ XRAYLIB_PARAM_GEOMETRIC_CALI_RATIO, &XRayImageProcess::SetGeometricRatio });
}

void XRayImageProcess::RegistImageSetMap()
{
	/******************************************************************************************
	*                                      图像状态设置
	*******************************************************************************************/
	m_mapImageSet.insert({ XRAYLIB_ENERGY, &XRayImageProcess::SetEnergyMode });
	m_mapImageSet.insert({ XRAYLIB_COLOR, &XRayImageProcess::SetColorMode });
	m_mapImageSet.insert({ XRAYLIB_PENETRATION, &XRayImageProcess::SetPenetrationMode });
	m_mapImageSet.insert({ XRAYLIB_ENHANCE, &XRayImageProcess::SetEnahnceMode });
	m_mapImageSet.insert({ XRAYLIB_INVERSE, &XRayImageProcess::SetInverseMode });
	m_mapImageSet.insert({ XRAYLIB_GRAYLEVEL, &XRayImageProcess::SetGrayLevel });
	m_mapImageSet.insert({ XRAYLIB_BRIGHTNESS, &XRayImageProcess::SetBrightness });
	m_mapImageSet.insert({ XRAYLIB_SINGLECOLORTABLE, &XRayImageProcess::SetSingleColorTableSel });
	m_mapImageSet.insert({ XRAYLIB_DUALCOLORTABLE, &XRayImageProcess::SetDualColorTableSel });
	m_mapImageSet.insert({ XRAYLIB_COLORSIMAGING, &XRayImageProcess::SetColorImagingSel });
}

/******************************************************************************************
*                                      图像状态设置
*******************************************************************************************/

XRAY_LIB_HRESULT XRayImageProcess::SetEnergyMode(XRAYLIB_CONFIG_IMAGE* stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetEnergyMode para is null.");

	XRAYLIB_ENERGY_MODE energymode = *(XRAYLIB_ENERGY_MODE*)stPara->value;
	XSP_CHECK(energymode >= XRAYLIB_ENERGY_MODE_MIN_VALUE && 
		      energymode <= XRAYLIB_ENERGY_MODE_MAX_VALUE, 
		      XRAY_LIB_INVALID_PARAM, "SetEnergyMode. Illegal param: %d\n", energymode);

	m_sharedPara.m_enEnergyMode = energymode;
	log_info("SetEnergyMode: %d", energymode);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetColorMode(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetColorMode para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "SetPenetrationMode. Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	XRAYLIB_COLOR_MODE colormode = *(XRAYLIB_COLOR_MODE*)stPara->value;
	XSP_CHECK(colormode >= XRAYLIB_COLOR_MIN_VALUE && 
		      colormode <= XRAYLIB_COLOR_MAX_VALUE, 
		      XRAY_LIB_INVALID_PARAM, "SetColorMode. Illegal param: %d", colormode);

	m_modDual.m_enColorMode = *(XRAYLIB_COLOR_MODE*)stPara->value;
	m_modDual.ColorModeSelected();
	log_info("SetColorMode: %d", colormode);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetPenetrationMode(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetPenetrationMode para is null.");

	XRAYLIB_PENETRATION_MODE penetrationmode = *(XRAYLIB_PENETRATION_MODE*)stPara->value;
	XSP_CHECK(penetrationmode >= XRAYLIB_PENETRATION_MODE_MIN_VALUE && 
		      penetrationmode <= XRAYLIB_PENETRATION_MODE_MAX_VALUE, 
		      XRAY_LIB_INVALID_PARAM, "SetPenetrationMode. Illegal param: %d", penetrationmode);
	
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "SetPenetrationMode. Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	m_modDual.m_enPenetrationMode = penetrationmode;
	if (XRAYLIB_PENETRATION_NORMAL == penetrationmode)
	{
		m_modDual.m_fpeneMergeParm = 0.5f;
	}
	else if (XRAYLIB_PENETRATION_HIGHPENE == penetrationmode)
	{
		m_modDual.m_fpeneMergeParm = m_modDual.m_fHpeneRatio;
	}
	else
	{
		m_modDual.m_fpeneMergeParm = m_modDual.m_fLpeneRatio;
	}
	log_info("SetPenetrationMode: %d", penetrationmode);
	m_modDual.m_islPipeChroma.SetHlpenYout(m_modDual.m_fpeneMergeParm, m_modDual.m_nBrightnessAdjust, m_modDual.m_nBKLuminance);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetEnahnceMode(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetEnahnceMode para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "SetEnahnceMode. Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	XRAYLIB_ENHANCE_MODE enhancemode = *(XRAYLIB_ENHANCE_MODE*)stPara->value;
	XSP_CHECK(enhancemode >= XRAYLIB_ENHANCE_MODE_MIN_VALUE && 
		      enhancemode <= XRAYLIB_ENHANCE_MODE_MAX_VALUE, 
		      XRAY_LIB_INVALID_PARAM, "SetEnahnceMode. Illegal param: %d", enhancemode);

	m_modImgproc.m_enEnhanceMode = enhancemode;

	log_info("SetEnahnceMode: %d", enhancemode);
	return XRAY_LIB_OK;
}


XRAY_LIB_HRESULT XRayImageProcess::SetInverseMode(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetInverseMode para is null.");

	XRAYLIB_INVERSE_MODE inversemode = *(XRAYLIB_INVERSE_MODE*)stPara->value;
	XSP_CHECK(inversemode >= XRAYLIB_INVERSE_MIN_VALUE &&
		      inversemode <= XRAYLIB_INVERSE_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "SetInverseMode. Illegal param: %d", inversemode);

	m_modDual.m_enInverseMode = inversemode;
	log_info("SetInverseMode: %d", inversemode);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetGrayLevel(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetGrayLevel para is null.");

	int nGrayLevel = *(int*)stPara->value;
	XSP_CHECK(nGrayLevel >= -1 && nGrayLevel < XRAY_LIB_GRAYLEVEL, 
		      XRAY_LIB_INVALID_PARAM, "SetGrayLevel.Illegal param : %d", nGrayLevel);

	m_modDual.m_nGrayLevel = nGrayLevel;

	if (m_modDual.m_nGrayLevel == -1)
	{
		m_modDual.m_nGrayMax = 65535;
		m_modDual.m_nGrayMin = 0;
	}
	else
	{
		///////////////按64个档位设置，在低灰度区域和高灰度区域窗位尽量小
		///////////////最低档位128窗宽，按等比数列设置，依次增加倍数 0.2258
		float fRatio;
		if (nGrayLevel<32) //低灰度区窗宽小
		{
			fRatio = 0.2158f;
			m_modDual.m_nGrayMin = 100;
			m_modDual.m_nGrayMax = (int)(256 * (nGrayLevel + 1)*(1 + nGrayLevel*fRatio) - 1 + 100);
		}
		else
		{
			int nNewLevel = 63 - nGrayLevel;
			fRatio = 0.20875f;
			m_modDual.m_nGrayMin = (int)(65535 - 256 * (nNewLevel + 1)*(1 + nNewLevel*fRatio) + 1 - 2000);
			m_modDual.m_nGrayMax = 63535;
		}
	}
	m_modDual.m_nGrayMin = Clamp(m_modDual.m_nGrayMin, 0, 65535);
	m_modDual.m_nGrayMax = Clamp(m_modDual.m_nGrayMax, 0, 65535);

	log_info("SetGrayLevel: %d; GrayMin: %d; GrayMax: %d.", nGrayLevel, m_modDual.m_nGrayMin, m_modDual.m_nGrayMax);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetBrightness(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetBrightness para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nBrightnessLevel = *(int*)stPara->value;

	XSP_CHECK(nBrightnessLevel >= 0 && nBrightnessLevel <= XRAY_LIB_BRIGHTNESSLEVEL, 
		      XRAY_LIB_INVALID_PARAM, "SetBrightness. Illegal param: %d\n", nBrightnessLevel);
	
	// 根据效果调整映射关系[0, 50] -> [-300, 0], [50, 100] -> [0, 300]
	m_modDual.m_nBrightnessAdjust = (nBrightnessLevel > 50) ? (6 * (nBrightnessLevel - 50)) : (nBrightnessLevel * 6 - 300);

	m_modDual.m_islPipeChroma.SetHlpenYout(m_modDual.m_fpeneMergeParm, m_modDual.m_nBrightnessAdjust, m_modDual.m_nBKLuminance);

   	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetSingleColorTableSel([[maybe_unused]] XRAYLIB_CONFIG_IMAGE * stPara)
{
    /// 不再支持单能颜色表
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetDualColorTableSel(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetDualColorTableSel para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	XRAYLIB_DUALCOLORTABLE_SEL dualColorSel;
	dualColorSel = *(XRAYLIB_DUALCOLORTABLE_SEL*)stPara->value;
	m_modDual.m_enDualColorTableSel = std::clamp(dualColorSel, XRAYLIB_DUALCOLORTABLE_MIN_VALUE, XRAYLIB_DUALCOLORTABLE_MAX_VALUE);
	log_info("SetDualColorTableSel: %d", m_modDual.m_enDualColorTableSel);

    XRAY_LIB_HRESULT result = m_modDual.ColorTbeSelected();
	XSP_CHECK(XRAY_LIB_OK == result, result, "ColorTbeSelected failed.");

	return result;
}

XRAY_LIB_HRESULT XRayImageProcess::SetColorImagingSel(XRAYLIB_CONFIG_IMAGE * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetColorImagingSel para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	XRAYLIB_COLORSIMAGING_SEL colorModelSel;
	colorModelSel = *(XRAYLIB_COLORSIMAGING_SEL*)stPara->value;

	XSP_CHECK(colorModelSel >= XRAYLIB_COLORSIMAGING_MIN_VALUE &&
		      colorModelSel <= XRAYLIB_COLORSIMAGING_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "SetColorImagingSel. Illegal param: %d\n", colorModelSel);

	if (XRAYLIB_COLORSIMAGING_6S == colorModelSel)
	{
		if (m_modDual.m_bZ6CanUse)
		{
			m_modDual.m_enColorImagingSel = colorModelSel;
		}
	}
	else
	{
		m_modDual.m_enColorImagingSel = colorModelSel;
	}

	XRAY_LIB_HRESULT result = XRAY_LIB_OK;
	result = m_modDual.ColorTbeSelected();
	XSP_CHECK(XRAY_LIB_OK == result, result, "SetPenetrationMode failed.");
	return XRAY_LIB_OK;
}

/******************************************************************************************
*                                        参数设置
*******************************************************************************************/

XRAY_LIB_HRESULT XRayImageProcess::SetWhitenUpNum(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetWhitenUpNum para is null.");

	int nWhitenUp = stPara->numeratorValue;
	XSP_CHECK(nWhitenUp >= 0 && nWhitenUp <= m_nDetectorWidth, XRAY_LIB_INVALID_PARAM,
		     "SetWhitenUpNum.Illegal param : %d", nWhitenUp);

	if (XRAYLIB_ENERGY_SCATTER == m_sharedPara.m_enEnergyMode)
	{
		m_modDual.m_nWhiteUpBs = nWhitenUp * m_fWidthResizeFactor;	// 因为背散在上色时置背景色, 所以保留超分后的高度
	}
	else
	{
		m_modCali.m_nWhitenUp = nWhitenUp;
	}

	log_info("SetWhitenUpNum: %d", nWhitenUp);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetWhitenDownNum(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetWhitenDownNum para is null.");

	int nWhitenDown = stPara->numeratorValue;
	XSP_CHECK(nWhitenDown >= 0 && nWhitenDown <= m_nDetectorWidth, XRAY_LIB_INVALID_PARAM,
		      "SetWhitenDownNum.Illegal param : %d", nWhitenDown);

	if (XRAYLIB_ENERGY_SCATTER == m_sharedPara.m_enEnergyMode)
	{
		m_modDual.m_nWhiteDownBs = nWhitenDown * m_fWidthResizeFactor;	// 因为背散在上色时置背景色, 所以保留超分后的高度
	}
	else
	{
		m_modCali.m_nWhitenDown = nWhitenDown;
	}

	log_info("SetWhitenDownNum: %d", nWhitenDown);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetBackgroundThreshold(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetBackgroundThreshold para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	m_sharedPara.m_nBackGroundThreshold = std::clamp(stPara->numeratorValue, 0, 65535);
	log_info("SetBackgroundThreshold: %d", m_sharedPara.m_nBackGroundThreshold);

    m_modImgproc.m_islFbg.SetBgLumTh(m_sharedPara.m_nBackGroundThreshold);
    std::pair<uint16_t, uint16_t> highlight = std::make_pair(
        std::max(m_sharedPara.m_nBackGroundThreshold - (100 - m_modImgproc.m_nHighLumSensity) * 75, 0), // 高亮起始
        m_sharedPara.m_nBackGroundThreshold);
	m_islPipeComm.SetBrightnessParams(m_modImgproc.m_nLumIntensity, highlight);

    return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetBackgroundGray(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetBackgroundGray para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nBackgroundGray = stPara->numeratorValue;
	XSP_CHECK(nBackgroundGray >= 0 && nBackgroundGray <= 65535, XRAY_LIB_INVALID_PARAM,
		      "SetBackgroundGray.Illegal param : %d", nBackgroundGray);

	m_sharedPara.m_nBackGroundGray = nBackgroundGray;
	log_info("SetBackgroundGray: %d", nBackgroundGray);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetRTUpdateAirRatio(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetRTUpdateAirRatio para is null.");

	float fRatio = (float)stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(fRatio >= 0.0f && fRatio <= 0.051f, 
		      XRAY_LIB_INVALID_PARAM,
		      "SetRTUpdateAirRatio.Illegal param : %f", fRatio);

	m_modCali.m_fRTUpdateAirRatio = fRatio;
	log_info("SetRTUpdateAirRatio: %f", fRatio);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetEmptyThresholdH([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	/* 暂无作用 */
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetEmptyThresholdL([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	/* TODO 暂无作用 */
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetRTUpdateLowGrayRatio(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetRTUpdateLowGrayRatio para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	float nRTUpdateLowGrayRatio = (float)stPara->numeratorValue / stPara->denominatorValue;

	m_modImgproc.m_fRTUpdateLowGrayRatio = nRTUpdateLowGrayRatio;

	log_info("SetRTUpdateLowGrayRatio: %f", nRTUpdateLowGrayRatio);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetPackageThreshold([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	/* TODO 暂无作用 */
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetLibXRayDefaultImageWidth([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	/* TODO 功能未开放 */
	return XRAY_LIB_NOACCESS;
}

XRAY_LIB_HRESULT XRayImageProcess::SetColdHotCaliThreshold(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetColdHotCaliThreshold para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nColdHotCaliThreshold = stPara->numeratorValue;
	XSP_CHECK(nColdHotCaliThreshold >= 0 && nColdHotCaliThreshold <= 2000, XRAY_LIB_INVALID_PARAM,
		"SetColdHotCaliThreshold.Illegal param : %d", nColdHotCaliThreshold);

	m_modCali.m_nCountThreshold = nColdHotCaliThreshold;

	log_info("SetColdHotCaliThreshold: %d", nColdHotCaliThreshold);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetBeltGrainLimit(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetColdHotCaliThreshold para is null.");

	int nBeltGrainLimit = stPara->numeratorValue;
	XSP_CHECK(nBeltGrainLimit >= 0 && nBeltGrainLimit <= 20, XRAY_LIB_INVALID_PARAM,
		"SetBeltGrainLimit.Illegal param : %d", nBeltGrainLimit);

	m_modCali.m_nBeltGrainLimit = nBeltGrainLimit;


	log_info("SetBeltGrainLimit: %d", nBeltGrainLimit);
	return XRAY_LIB_OK;
}


XRAY_LIB_HRESULT XRayImageProcess::SetDualFilterKernelLength(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetDualFilterKernelLength para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nDualFilterKernelLength = stPara->numeratorValue;
	XSP_CHECK(nDualFilterKernelLength >= 0 && 
		      nDualFilterKernelLength <= XRAY_LIB_MAX_FILTER_KERNEL_LENGTH, 
		      XRAY_LIB_INVALID_PARAM, 
		      "SetDualFilterKernelLength.Illegal param : %d", nDualFilterKernelLength);

	m_modDual.m_nDualFilterKernelLength = nDualFilterKernelLength;
	log_info("SetDualFilterKernelLength: %d", nDualFilterKernelLength);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetDualFilterRange(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetDualFilterRange para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	float fDualFilterRange = (float)stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(fDualFilterRange >= 0.01f && 
		      fDualFilterRange <= 0.5f, 
		      XRAY_LIB_INVALID_PARAM,
		      "SetDualFilterRange.Illegal param : %f", fDualFilterRange);

	m_modDual.m_fDualFilterRange = fDualFilterRange;
	log_info("SetDualFilterRange: %f", fDualFilterRange);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetDualDistinguishGrayUp(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetDualDistinguishGrayUp para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nDualDistinguishGrayUp = stPara->numeratorValue;
	XSP_CHECK(nDualDistinguishGrayUp >= 0 && nDualDistinguishGrayUp <= 65535,
		      XRAY_LIB_INVALID_PARAM, "SetDualDistinguishGrayUp.Illegal param : %d", 
		      nDualDistinguishGrayUp);

	m_modDual.m_nDualDistinguishGrayUp = nDualDistinguishGrayUp;
    log_info("SetDualDistinguishGrayUp: %d", nDualDistinguishGrayUp);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetDualDistinguishGrayDown(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetDualDistinguishGrayDown para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nDualDistinguishGrayDown = stPara->numeratorValue;
	XSP_CHECK(nDualDistinguishGrayDown >= 0 && nDualDistinguishGrayDown <= 65535,
		      XRAY_LIB_INVALID_PARAM, "SetDualDistinguishGrayDown.Illegal param : %d", 
		      nDualDistinguishGrayDown);

	m_modDual.m_nDualDistinguishGrayDown = nDualDistinguishGrayDown;
	log_info("SetDualDistinguishGrayDown: %d", nDualDistinguishGrayDown);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetDualDistinguishNoneInorganic(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetDualDistinguishNoneInorganic para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nDualDistinguishNoneInorganic = stPara->numeratorValue;
	XSP_CHECK(nDualDistinguishNoneInorganic >= 0 && nDualDistinguishNoneInorganic <= 65535,
		      XRAY_LIB_INVALID_PARAM, "SetDualDistinguishNoneInorganic.Illegal param : %d",
		      nDualDistinguishNoneInorganic);

	m_modDual.m_nDualDistinguishNoneInorganic = nDualDistinguishNoneInorganic;
	log_info("SetDualDistinguishNoneInorganic: %d", nDualDistinguishNoneInorganic);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetDefaultEnhance(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetDefaultEnhance para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	XRAYLIB_DEFAULT_ENHANCE nDefaultEnhance = (XRAYLIB_DEFAULT_ENHANCE)stPara->numeratorValue;
	XSP_CHECK(nDefaultEnhance >= XRAYLIB_DEFAULTENHANCE_MIN_VALUE && 
		      nDefaultEnhance <= XRAYLIB_DEFAULTENHANCE_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "SetDefaultEnhance.Illegal param : %d",
		      nDefaultEnhance);

	/* 图像处理模块默认增强始终不关闭 */
	m_enDefaultEnhanceJudge = nDefaultEnhance;
	if (XRAYLIB_DEFAULTENHANCE_CLOSE != nDefaultEnhance)
	{
		m_modImgproc.m_enDefaultEnhance = nDefaultEnhance;
	}
	log_info("SetDefaultEnhance: %d", nDefaultEnhance);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetDefaultEnhanceIntensity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetDefaultEnhanceIntensity para is null.");

	float nDefaultEnhanceIntensity = (float)stPara->numeratorValue / stPara->denominatorValue;
	
	m_modImgproc.m_fDefaultEnhanceIntensity = nDefaultEnhanceIntensity;
	
	log_info("SetDefaultEnhanceIntensity: %f", nDefaultEnhanceIntensity);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetEdgeEnhanceIntensity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetEdgeEnhanceIntensity para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	float nEdgeEnhanceIntensity = (float)stPara->numeratorValue / stPara->denominatorValue;

	m_modImgproc.m_fEdgeEnhanceIntensity = nEdgeEnhanceIntensity;

	log_info("SetEdgeEnhanceIntensity: %f", nEdgeEnhanceIntensity);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetSuperEnhanceIntensity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetSuperEnhanceIntensity para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	float nEdgeSuperIntensity = (float)stPara->numeratorValue / stPara->denominatorValue;

	m_modImgproc.m_fEdgeSuperIntensity = nEdgeSuperIntensity;

	log_info("SetSuperEnhanceIntensity: %f", nEdgeSuperIntensity);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetSpecialEnhanceThresholdUp(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetSpecialEnhanceThresholdUp para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nSpecialEnhanceThresholdUp = stPara->numeratorValue;

	m_modImgproc.m_nLocalEnhanceThresholdUP = nSpecialEnhanceThresholdUp;

	log_info("SetSpecialEnhanceThresholdUp: %d", nSpecialEnhanceThresholdUp);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetSpecialEnhanceThresholdDown(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetSpecialEnhanceThresholdDown para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nSpecialEnhanceThresholdDown = stPara->numeratorValue;

	m_modImgproc.m_nWindowCenter = nSpecialEnhanceThresholdDown;

	log_info("SetSpecialEnhanceThresholdDown: %d", nSpecialEnhanceThresholdDown);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetSpecialEnhanceStretchGrayUp([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetSpecialEnhanceStretchGrayUp para is null.");

	// int nSpecialEnhanceStretchGrayUp = stPara->numeratorValue;

	// m_modImgproc.m_nSpecialEnhanceStretchGrayUP = nSpecialEnhanceStretchGrayUp;

	// log_info("SetSpecialEnhanceStretchGrayUp: %d", nSpecialEnhanceStretchGrayUp);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetSpecialEnhanceStretchGrayDown([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetSpecialEnhanceStretchGrayDown para is null.");

	// int nSpecialEnhanceStretchGrayDown = stPara->numeratorValue;

	// m_modImgproc.m_nSpecialEnhanceStretchGrayDown = nSpecialEnhanceStretchGrayDown;

	// log_info("SetSpecialEnhanceStretchGrayDown: %d", nSpecialEnhanceStretchGrayDown);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetCaliLowGrayThreshold(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetCaliLowGrayThreshold para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nCaliLowGrayThreshold = stPara->numeratorValue;

	m_modImgproc.m_nCaliLowGrayThreshold = nCaliLowGrayThreshold;

	log_info("SetCaliLowGrayThreshold: %d", nCaliLowGrayThreshold);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetDenoisingIntensity([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	/* TODO 暂无作用 */
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetCompositiveRatio([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetDenoisingMode([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	/* TODO 暂无作用 */
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetAntialiasingMode([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	/* TODO 暂无作用 */
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetAIDeviceMode([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	/* TODO 暂无作用 */
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetResizeHeightFactor(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetResizeHeightFactor para is null.");

	float fTemp = (float)stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(fTemp >= 1.0 && fTemp <= 2.0, XRAY_LIB_INVALID_PARAM, 
		      "SetResizeHeightFactor illegal para %f.3", fTemp);

	m_fHeightResizeFactor = fTemp;

	log_info("SetResizeHeightFactor: %f", m_fHeightResizeFactor);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetResizeWidthFactor(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetResizeWidthFactor para is null.");

	float fTemp = (float)stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(fTemp >= 1.0 && fTemp <= 2.0, XRAY_LIB_INVALID_PARAM,
		      "SetResizeWidthFactor illegal para %f.3", fTemp);

	m_fWidthResizeFactor = fTemp;
	log_info("SetResizeWidthFactor: %f", m_fWidthResizeFactor);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetFusionMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetFusionMode para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	XRAYLIB_FUSION_MODE mode = (XRAYLIB_FUSION_MODE)stPara->numeratorValue;
	XSP_CHECK(mode >= XRAYLIB_FUSION_MIN_VALUE &&
		mode <= XRAYLIB_FUSION_MAX_VALUE,
		XRAY_LIB_INVALID_PARAM, "SetFusionMode.Illegal param : %d",
		mode);

	m_modImgproc.m_enFusionMode = mode;

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetBackgoundColor(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetBackgoundColor para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	uint32_t lrgb = (uint32_t)(stPara->numeratorValue); // 高8位表示背景亮度，低24位表示背景颜色（RGB格式）
	m_modDual.m_nBKLuminance = (std::clamp(static_cast<int32_t>(lrgb >> 24), 0, 100) * 26 / 100 + 229);
	m_modDual.m_nBKColorARGB = lrgb & 0xFFFFFF;
	m_modDual.m_islPipeChroma.SetBgColor(m_modDual.m_nBKColorARGB, m_modDual.m_nBKLuminance);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetStretchRatio(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetStretchRatio para is null.");

	float fTemp = (float)stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(fTemp >= 0.75 && fTemp <= 1.01, XRAY_LIB_INVALID_PARAM,
		     "SetStretchRatio illegal para %f.3", fTemp);

	m_modCali.m_fStretchRatio = fTemp;

	log_info("SetStretchRatio: %f", fTemp);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetGammaMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetGammaMode para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	XRAYLIB_GAMMA_MODE mode = (XRAYLIB_GAMMA_MODE)stPara->numeratorValue;
	XSP_CHECK(mode >= XRAYLIB_GAMMA_MIN_VALUE &&
		      mode <= XRAYLIB_GAMMA_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "SetGammaMode.Illegal param : %d",
		      mode);

	m_modImgproc.m_enGammaMode = mode;

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetGammaIntensity([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	return XRAY_LIB_INVALID_PARAM;
}

XRAY_LIB_HRESULT XRayImageProcess::SetSigmaNoise(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetSigmaNoise para is null.");

	float fTemp = (float)stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(fTemp >= 0.0f && fTemp <= 1.01f, XRAY_LIB_INVALID_PARAM,
		     "SetSigmaNoise illegal para %f.3", fTemp);

	m_fSigmaDn = fTemp;
	m_fSigmaSr = fTemp;
	m_fDenoiseSigma = fTemp;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetHighPeneRatio(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetPeneParam para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	float fTemp = (float)stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(fTemp >= 0.0f && fTemp <= 1.0f, XRAY_LIB_INVALID_PARAM,
			  "SetPeneParam illegal para %f.3", fTemp);

	/**************************************************************************************
	 *  因为SetHlpenYoffs接口接收参数范围[0,1], 
	 *  其中[0, 0.5]和低穿加权(越接近0, 低穿曲线占比越大), [0.5,1]和高穿加权(越接近1, 高穿曲线占比越大)
	 *  所以该接口接收[0,1]范围内的参数后需在此处先缩小范围, 在SetHlpenYoffs中加权时再扩大到[0,1]
	 * 	SetLowPeneRatio接口同理
	 **************************************************************************************/
	m_modDual.m_fHpeneRatio = std::clamp(fTemp * 0.5f + 0.5f, static_cast<float>(0.5f), static_cast<float>(1.0f));
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetLowPeneRatio(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetPeneParam para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	float fTemp = (float)stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(fTemp >= 0.0f && fTemp <= 1.0f, XRAY_LIB_INVALID_PARAM,
			  "SetPeneParam illegal para %f.3", fTemp);

	m_modDual.m_fLpeneRatio = std::clamp(std::abs(fTemp * 0.5f - 0.5f), static_cast<float>(0.0f), static_cast<float>(0.5f));
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetMergeBaseLine(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetMergeBaseLine para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nMergeBaseLine = stPara->numeratorValue;

	m_modImgproc.m_nMergeBaseLine = nMergeBaseLine;

	log_info("MergeBaseLine: %d", nMergeBaseLine);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetGeoCaliInverse(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetGeoCaliInverse para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nGeoCaliInverse = stPara->numeratorValue;
	m_modCali.m_bGeoCaliInverse = (bool)nGeoCaliInverse;
	m_modCali.GetGeoOffsetLut();
	log_info("nGeoCaliInverse: %d", nGeoCaliInverse);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetTestMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetTestMode para is null.");

	XRAY_TESTMODE testMode = (XRAY_TESTMODE)stPara->numeratorValue;
	XSP_CHECK(testMode >= XRAYLIB_TESTMODE_MIN_VALUE &&
		testMode <= XRAYLIB_TESTMODE_MAX_VALUE,
		XRAY_LIB_INVALID_PARAM, "SetTestMode.Illegal Param : 0x%x",
		testMode);

	m_modImgproc.m_enTestMode = testMode;

	if ((testMode == XRAYLIB_TESTMODE_CN) || (testMode == XRAYLIB_TESTMODE_CN_CHECK))
	{
		// 切换测试体模式时，需要清除设备的
		m_modCali.m_stLPadCache.bClearTemp = true;
		m_modCali.m_stHPadCache.bClearTemp = true;
		m_modCali.m_stOriLPadCache.bClearTemp = true;
		m_modCali.m_stOriHPadCache.bClearTemp = true;
		m_modCali.m_stCaliLPadCache.bClearTemp = true;
		m_modCali.m_stCaliHPadCache.bClearTemp = true;
        m_modCali.m_stMaskCache.bClearTemp = true;
        this->procWtCache.bClearTemp = true;
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetRGBRotate(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetRGBRotate para is null.");

	XRAYLIB_IMG_ROTATE rgbrotate = (XRAYLIB_IMG_ROTATE)stPara->numeratorValue;
	XSP_CHECK(rgbrotate >= XRAYLIB_ROTATE_MIN_VALUE &&
		      rgbrotate <= XRAYLIB_ROTATE_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "SetRGBRotate.Illegal param : %d",
		      rgbrotate);

	if (rgbrotate >= XRAYLIB_ENTIRE_MOVE_RIGHT)
	{
		/* 回拉转存方向设置 */
		rgbrotate = XRAYLIB_IMG_ROTATE(rgbrotate - XRAYLIB_ENTIRE_MOVE_RIGHT);
		m_enRotateEntire = rgbrotate;
		m_enRotate = rgbrotate;
		log_debug("SetRotate Entire: %d\n", rgbrotate);
	}
	else
	{
		m_enRotateRT = rgbrotate;
		m_enRotate = rgbrotate;
		log_debug("SetRotate RT: %d\n", rgbrotate);
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetRGBMirror(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetRGBMirror para is null.");

	XRAYLIB_IMG_MIRROR rgbmirror = (XRAYLIB_IMG_MIRROR)stPara->numeratorValue;
	XSP_CHECK(rgbmirror >= XRAYLIB_MIRROR_MIN_VALUE &&
		      rgbmirror <= XRAYLIB_MIRROR_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "SetRGBMirror.Illegal param : %d",
		      rgbmirror);

	m_enMirror = rgbmirror;
	log_debug("SetRGBMirror: %d", rgbmirror);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetScanMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetScanMode para is null.");

	XRAYLIB_SCAN_MODE xRayLibScanMode = (XRAYLIB_SCAN_MODE)stPara->numeratorValue;
	XSP_CHECK(xRayLibScanMode >= XRAYLIB_SCAN_MODE_MIN_VALUE &&
		      xRayLibScanMode <= XRAYLIB_SCAN_MODE_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "SetScanMode.Illegal param : %d",
		      xRayLibScanMode);

	m_enScanMode = xRayLibScanMode;
	log_info("SetScanMode: %d", xRayLibScanMode);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetMaterialAdjust(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetMaterialAdjust para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	float fRCurveAdjust = (float)stPara->numeratorValue / stPara->denominatorValue;
	m_modDual.m_fMaterialAdjust = fRCurveAdjust;
	log_info("SetMaterialAdjust: %f", fRCurveAdjust);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetDoseCali(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetDoseCali para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);
	XRAY_LIB_ONOFF DoseAdjust = (XRAY_LIB_ONOFF)stPara->numeratorValue;
	XSP_CHECK(DoseAdjust >= XRAYLIB_ONOFF_MIN_VALUE &&
		DoseAdjust <= XRAYLIB_ONOFF_MAX_VALUE,
		XRAY_LIB_INVALID_PARAM, "SetDoseCali.Illegal param : %d",
		DoseAdjust);

	if (m_modCali.m_enDoseCaliMertric != DoseAdjust)
	{
		m_modCali.m_enDoseCaliMertric = DoseAdjust;
		log_info("SetDoseCali: %d", (int)DoseAdjust);
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetDrugPrompt(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetDrugPrompt para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);
	XRAY_LIB_ONOFF DrugPrompt = (XRAY_LIB_ONOFF)stPara->numeratorValue;
	XSP_CHECK(DrugPrompt >= XRAYLIB_ONOFF_MIN_VALUE &&
		DrugPrompt <= XRAYLIB_ONOFF_MAX_VALUE,
		XRAY_LIB_INVALID_PARAM, "DrugPrompt.Illegal param : %d", DrugPrompt);

	if (m_modArea.m_enDrugPrompt != DrugPrompt)
	{
		m_modArea.m_enDrugPrompt = DrugPrompt;
		log_info("DrugPrompt: %d", (int)DrugPrompt);
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetExplosivePrompt(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetExplosivePrompt para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);
	XRAY_LIB_ONOFF ExplosivePrompt = (XRAY_LIB_ONOFF)stPara->numeratorValue;
	XSP_CHECK(ExplosivePrompt >= XRAYLIB_ONOFF_MIN_VALUE &&
		ExplosivePrompt <= XRAYLIB_ONOFF_MAX_VALUE,
		XRAY_LIB_INVALID_PARAM, "ExplosivePrompt.Illegal param : %d", ExplosivePrompt);

	if (m_modArea.m_enExplosivePrompt != ExplosivePrompt)
	{
		m_modArea.m_enExplosivePrompt = ExplosivePrompt;
		log_info("ExplosivePrompt: %d", (int)ExplosivePrompt);
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetGeomertricCali(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetGeomertricCali para is null.");
	XRAY_LIB_ONOFF GeometricCali = (XRAY_LIB_ONOFF)stPara->numeratorValue;
	XSP_CHECK(GeometricCali >= XRAYLIB_ONOFF_MIN_VALUE &&
		      GeometricCali <= XRAYLIB_ONOFF_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "SetGeomertricCali.Illegal param : %d",
		      GeometricCali);

	if (!m_modCali.m_bGeoCaliCanUse && GeometricCali == XRAYLIB_ON)
		return XRAY_LIB_GEOCALITABLE_ERROR;

	if (m_modCali.m_enGeoMertric != GeometricCali)
	{
		m_modCali.m_stLPadCache.bClearTemp = true; 
		m_modCali.m_stHPadCache.bClearTemp = true;
		m_modCali.m_stOriLPadCache.bClearTemp = true;
		m_modCali.m_stOriHPadCache.bClearTemp = true;
		m_modCali.m_stCaliLPadCache.bClearTemp = true;
		m_modCali.m_stCaliHPadCache.bClearTemp = true;
		m_modCali.m_stMaskCache.bClearTemp = true;
		this->procWtCache.bClearTemp = true;
		m_modCali.m_enGeoMertric = GeometricCali;

		log_info("SetGeomertricCali: %d", GeometricCali);
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetLowPenetrationPrompt(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetLowPenetrationPrompt para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);
	XRAY_LIB_ONOFF nLowPenetrationPrompt = (XRAY_LIB_ONOFF)stPara->numeratorValue;
	XSP_CHECK(nLowPenetrationPrompt >= XRAYLIB_ONOFF_MIN_VALUE &&
		      nLowPenetrationPrompt <= XRAYLIB_ONOFF_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "SetLowPenetrationPrompt.Illegal param : %d",
		      nLowPenetrationPrompt);
	m_modArea.m_enLowPenetrationPrompt = nLowPenetrationPrompt;
	log_info("SetLowPenetrationPrompt: %d", nLowPenetrationPrompt);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetLowPeneSenstivity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetLowPeneSenstivity para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	XRAYLIB_LOWPENE_SENSITIVITY LowPeneSenstivity = (XRAYLIB_LOWPENE_SENSITIVITY)stPara->numeratorValue;
	XSP_CHECK(LowPeneSenstivity >= XRAYLIB_LOWPENESENS_MIN_VALUE &&
		      LowPeneSenstivity <= XRAYLIB_LOWPENESENS_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "SetLowPeneSenstivity.Illegal param : %d",
		      LowPeneSenstivity);

	m_modArea.m_enLowPeneSenstivity = LowPeneSenstivity;

	if (LowPeneSenstivity == XRAYLIB_LOWPENESENS_MEDIUM)
	{
		m_modArea.m_nLowPenetrationThreshold = m_nLowPenesensParam.lowPeneMid;
		m_modImgproc.m_nCaliLowGrayThreshold = m_nLowPenesensParam.LowGrayMid;
	}
	else if (LowPeneSenstivity == XRAYLIB_LOWPENESENS_HIGH)
	{
		m_modArea.m_nLowPenetrationThreshold = m_nLowPenesensParam.lowPeneMax;
		m_modImgproc.m_nCaliLowGrayThreshold = m_nLowPenesensParam.LowGrayMax;
	}
	else
	{
		m_modArea.m_nLowPenetrationThreshold = m_nLowPenesensParam.lowPeneMin;
		m_modImgproc.m_nCaliLowGrayThreshold = m_nLowPenesensParam.LowGrayMin;
	}
	log_info("SetLowPeneSenstivity: %f", LowPeneSenstivity);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetLowPenetrationThreshold(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetLowPenetrationThreshold para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);
	int nLowPeneThreshold = stPara->numeratorValue;
	XSP_CHECK(nLowPeneThreshold >= 0 && nLowPeneThreshold <= 65535,
		      XRAY_LIB_INVALID_PARAM, "SetLowPenetrationThreshold.Illegal param : %d",
		      nLowPeneThreshold);

	m_modArea.m_nLowPenetrationThreshold = nLowPeneThreshold;
	log_info("SetLowPenetrationThreshold: %f", nLowPeneThreshold);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetLowPenetrationWarnThreshold(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetLowPenetrationWarnThreshold para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);
	int nLowPeneThreshold = stPara->numeratorValue;
	XSP_CHECK(nLowPeneThreshold >= 0 && nLowPeneThreshold <= 65535,
		      XRAY_LIB_INVALID_PARAM, "SetLowPenetrationWarnThreshold.Illegal param : %d",
		      nLowPeneThreshold);

	/* TODO 暂无作用 */
	log_info("SetLowPenetrationWarnThreshold: %f", nLowPeneThreshold);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetConcernedMPrompt(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetConcernedMPrompt para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);
	XRAY_LIB_ONOFF ConcernedMPrompt = (XRAY_LIB_ONOFF)stPara->numeratorValue;
	XSP_CHECK(ConcernedMPrompt >= XRAYLIB_ONOFF_MIN_VALUE &&
		      ConcernedMPrompt <= XRAYLIB_ONOFF_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "SetConcernedMPrompt.Illegal param : %d",
		      ConcernedMPrompt);

	m_modArea.m_enConcernedMaterialPrompt = ConcernedMPrompt;
	log_info("SetConcernedMPrompt: %d", ConcernedMPrompt);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetConcernedMSnestivity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetConcernedMSnestivity para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	XRAYLIB_CONCERNEDM_SENSITIVITY ConcernedMSens = (XRAYLIB_CONCERNEDM_SENSITIVITY)stPara->numeratorValue;
	XSP_CHECK(ConcernedMSens >= XRAYLIB_CONCERNEDMSENS_MIN_VALUE &&
		      ConcernedMSens <= XRAYLIB_CONCERNEDMSENS_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "SetConcernedMSnestivity.Illegal param : %d",
		      ConcernedMSens);
	m_modArea.SetSensitivity(ConcernedMSens);
	log_info("SetConcernedMSnestivity: 0x%x", ConcernedMSens);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetFlatDetmertricCali(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetFlatDetmertricCali para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);
	XRAY_LIB_ONOFF FlatDetmetricCali = (XRAY_LIB_ONOFF)stPara->numeratorValue;
	XSP_CHECK(FlatDetmetricCali >= XRAYLIB_ONOFF_MIN_VALUE &&
		      FlatDetmetricCali <= XRAYLIB_ONOFF_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "SetFlatDetmertricCali.Illegal param : %d",
		      FlatDetmetricCali);

	if (!m_modCali.m_bFlatDetCaliCanUse && FlatDetmetricCali == XRAYLIB_ON)
		return XRAY_LIB_GEOCALITABLE_ERROR;

	if (m_modCali.m_enFlatDetMertric != FlatDetmetricCali)
	{
		m_modCali.m_stHPadCache.bClearTemp = true;
		m_modCali.m_stLPadCache.bClearTemp = true;
        m_modCali.m_stMaskCache.bClearTemp = true;
        this->procWtCache.bClearTemp = true;
		m_modCali.m_enFlatDetMertric = FlatDetmetricCali;

		log_info("SetFlatDetmertricCali: %f", FlatDetmetricCali);
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetEexcutionMode([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	/* TODO 暂无作用 */
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetSpeed(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetSpeed para is null.");

	float fTemp = (float)stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(0.0f < fTemp && 4.0f >= fTemp, XRAY_LIB_INVALID_PARAM,
		     "SetSpeed illegal para %f", fTemp);
	m_fSpeed = fTemp;
	log_info("SetSpeed: %f", fTemp);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetRTHeight(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "LibXRay: SetRTHeight para is null.");

	int nTemp = stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(0 < nTemp && m_nMaxHeightRealTime >= nTemp, XRAY_LIB_INVALID_PARAM,
		"LibXRay: SetRTHeight illegal para %d", nTemp);

	// 重新计算Dsp需要的宽高
	UpdateResizePara(m_nDetectorWidth, nTemp);
	XRAY_LIB_HRESULT hr = m_modCnn.AiXsp_SwitchModel( m_nResizeHeight - 2 * m_nResizePadLen, m_nResizeWidth, nTemp, m_nDetectorWidth);
	if (XRAY_LIB_OK == hr)
	{
		m_nRTHeight = nTemp;
	}

	if (XRAYLIB_ENERGY_SCATTER ==  m_sharedPara.m_enEnergyMode)
	{
		// NLMeans算法参数
		switch (m_nRTHeight)
		{
			case 16:
				m_modCnn.m_fGrayAiRatio = 1.5f;
				break;
			
			case 24:
				m_modCnn.m_fGrayAiRatio = 2.0f;
				break;
			case 32:
				m_modCnn.m_fGrayAiRatio = 3.0f;
				break;
			default:
				XSP_CHECK(16 == nTemp || 24 == nTemp || 32 == nTemp, XRAY_LIB_INVALID_PARAM,
				"LibXRay: SetRTHeight illegal para %d", nTemp);
				hr = XRAY_LIB_INVALID_PARAM;
		}
	}
	else
	{
		std::vector<float64_t> pxPos;
		auto scale = m_modCnn.AiXsp_GetScale();
		int nAiXspNB = m_modCnn.AiXsp_GetNbLen();

		/* CNN输出图像尺寸 缩放至 最终输出（应用设置）的尺寸 */
		XSP_CHECK(0 == isl::buildLinearScalePos(pxPos, scale.first * 3 * m_nRTHeight, m_nResizeHeight), XRAY_LIB_PARAM_WIDTH_ERROR);
		XSP_CHECK(0 == isl::transPos2IntpLut(intpCnn2DstVer, 5, pxPos,scale.first * 3 * m_nRTHeight), XRAY_LIB_PARAM_WIDTH_ERROR);
		XSP_CHECK(0 == isl::buildLinearScalePos(pxPos, m_nDetectorWidth * scale.second, m_nResizeWidth), XRAY_LIB_PARAM_WIDTH_ERROR);
		XSP_CHECK(0 == isl::transPos2IntpLut(intpCnn2DstHor, 5, pxPos, m_nDetectorWidth * scale.second), XRAY_LIB_PARAM_WIDTH_ERROR);

		/* 原始输入图像尺寸 缩放至 CNN输入图像尺寸 */
		XSP_CHECK(0 == isl::buildLinearScalePos(pxPos, m_nRTHeight + 2 * nAiXspNB, scale.first * (m_nRTHeight + 2 * nAiXspNB)), XRAY_LIB_PARAM_WIDTH_ERROR);
		XSP_CHECK(0 == isl::transPos2IntpLut(intpSrc2CnnVer, 7, pxPos, m_nRTHeight + 2 * nAiXspNB), XRAY_LIB_PARAM_WIDTH_ERROR);
		XSP_CHECK(0 == isl::buildLinearScalePos(pxPos, m_nDetectorWidth, scale.second * m_nDetectorWidth), XRAY_LIB_PARAM_WIDTH_ERROR);
		XSP_CHECK(0 == isl::transPos2IntpLut(intpSrc2CnnHor, 7, pxPos, m_nDetectorWidth), XRAY_LIB_PARAM_WIDTH_ERROR);

        /* 原始输入图像尺寸 缩放至 最终输出（应用设置）的尺寸 */
		XSP_CHECK(0 == isl::buildLinearScalePos(pxPos, m_nRTHeight * 3, m_nResizeHeight), XRAY_LIB_PARAM_WIDTH_ERROR);
		XSP_CHECK(0 == isl::transPos2IntpLut(intpSrc2DstVer, 7, pxPos, m_nRTHeight * 3), XRAY_LIB_PARAM_WIDTH_ERROR);
		XSP_CHECK(0 == isl::buildLinearScalePos(pxPos, m_nDetectorWidth, m_nResizeWidth), XRAY_LIB_PARAM_WIDTH_ERROR);
		XSP_CHECK(0 == isl::transPos2IntpLut(intpSrc2DstHor, 7, pxPos, m_nDetectorWidth), XRAY_LIB_PARAM_WIDTH_ERROR);

		/* 原始原子序数 缩放至 最终输出的原子序数尺寸（上下两邻边） */ 
		int32_t nZValueMinNb = 2; // 原子序数图所需要的最小邻边为2
		int32_t nZResizeZValueMinNb = static_cast<int>(nZValueMinNb * m_fHeightResizeFactor);
	
		int32_t nZResizeHeightIn = m_nRTHeight + 2 * nZValueMinNb;
		int32_t nZResizeHeightOut = m_nResizeHeight - 2 * (m_nResizePadLen - nZResizeZValueMinNb);
	
		XSP_CHECK(0 == isl::buildLinearScalePos(pxPos, nZResizeHeightIn, nZResizeHeightOut), XRAY_LIB_PARAM_WIDTH_ERROR);
		XSP_CHECK(0 == isl::transPos2IntpLut(intpZSrc2DstVer, 7, pxPos,  nZResizeHeightIn), XRAY_LIB_PARAM_WIDTH_ERROR);
	
		XSP_CHECK(0 == isl::buildLinearScalePos(pxPos, m_nDetectorWidth, m_nResizeWidth), XRAY_LIB_PARAM_WIDTH_ERROR);
		XSP_CHECK(0 == isl::transPos2IntpLut(intpZSrc2DstHor, 7, pxPos, m_nDetectorWidth), XRAY_LIB_PARAM_WIDTH_ERROR);
	}

	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::SetTcStripeNum(XRAYLIB_CONFIG_PARAM * stPara)
{
    XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "LibXRay: SetTcStripeNum para is null.");

    int nTemp = stPara->numeratorValue / stPara->denominatorValue;
    XSP_CHECK(0 <= nTemp && 30 >= nTemp, XRAY_LIB_INVALID_PARAM,
        "LibXRay: SetTcStripeNum illegal para %d", nTemp);

    /*注意，设置缓存条带数量的时候，需要对序列清空重新初始化,以及记录的上次缓存条带序号变0*/
	m_modTcproc.m_sTcProcParam.SliceIndex = 0;
    m_modTcproc.m_vecSliceSeq.clear();
    for (int i = 0; i < nTemp; i++)
    {
        m_modTcproc.m_vecSliceSeq.push_back(i);
    }

	if (m_modTcproc.m_nStripeBufferNum != nTemp)
	{
		m_modCali.m_stLPadCache.bClearTemp = true;
		m_modCali.m_stHPadCache.bClearTemp = true;
        m_modCali.m_stMaskCache.bClearTemp = true;
        this->procWtCache.bClearTemp = true;
	}

    m_modTcproc.m_nStripeBufferNum = nTemp;

    return XRAY_LIB_OK;
}


XRAY_LIB_HRESULT XRayImageProcess::SetTcEnhanceIntensity(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "LibXRay: SetTcEnhanceIntensity para is null.");

	int nTemp = stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(0 <= nTemp && 200 >= nTemp, XRAY_LIB_INVALID_PARAM,
		"LibXRay: SetTcEnhanceIntensity illegal para %d", nTemp);

	if (nTemp > 100)
	{
		m_modTcproc.m_sTcProcParam.bShowEnhanceInfo = true;
		m_modTcproc.m_sTcProcParam.testAEnhanceGrade = nTemp - 100;
	}
	else
	{
		m_modTcproc.m_sTcProcParam.bShowEnhanceInfo = false;
		m_modTcproc.m_sTcProcParam.testAEnhanceGrade = nTemp;
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetLuminance(XRAYLIB_CONFIG_PARAM* stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "LibXRay: SetLuminance para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nTemp = stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(0 <= nTemp && 100 >= nTemp, XRAY_LIB_INVALID_PARAM, "LibXRay: SetLuminance illegal para %d", nTemp);
	m_modImgproc.m_nLumIntensity = nTemp;

    std::pair<uint16_t, uint16_t> highlight = std::make_pair(
        std::max(m_sharedPara.m_nBackGroundThreshold - (100 - m_modImgproc.m_nHighLumSensity) * 75, 0), // 高亮起始
        m_sharedPara.m_nBackGroundThreshold);
	m_islPipeComm.SetBrightnessParams(m_modImgproc.m_nLumIntensity, highlight);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetContrast(XRAYLIB_CONFIG_PARAM* stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "LibXRay: SetContrast para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nTemp = stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(0 <= nTemp && 100 >= nTemp, XRAY_LIB_INVALID_PARAM, "LibXRay: SetContrast illegal para %d", nTemp);

	m_modImgproc.m_nContrastRatio = nTemp;
    float64_t f64Contrast = 0.01 * m_modImgproc.m_nContrastRatio; // 归一化到[0, 1]
    float64_t f64LowLumComp = 0.01 * m_modImgproc.m_nLowLumCompensation;
	m_modImgproc.m_islPipeGray.SetLaceParams(0.5 * f64Contrast, f64LowLumComp);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetSharpnessIntensity(XRAYLIB_CONFIG_PARAM* stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "LibXRay: SetSharpnessIntensity para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nTemp = stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(0 <= nTemp && 100 >= nTemp, XRAY_LIB_INVALID_PARAM, "LibXRay: SetSharpnessIntensity illegal para %d", nTemp);

    m_modImgproc.m_nSharpnessRatio = nTemp;
    m_modImgproc.m_islPipeGray.SetNreeParams(0.12 * m_modImgproc.m_nSharpnessRatio);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetLowCompensation(XRAYLIB_CONFIG_PARAM* stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "LibXRay: SetLowCompensation para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nTemp = stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(0 <= nTemp && 100 >= nTemp, XRAY_LIB_INVALID_PARAM, "LibXRay: SetLowCompensation illegal para %d", nTemp);

	m_modImgproc.m_nLowLumCompensation = nTemp;
    float64_t f64Contrast = 0.01 * m_modImgproc.m_nContrastRatio;
    float64_t f64LowLumComp = 0.01 * m_modImgproc.m_nLowLumCompensation;
	m_modImgproc.m_islPipeGray.SetLaceParams(0.5 * f64Contrast, f64LowLumComp);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetHighSensitivity(XRAYLIB_CONFIG_PARAM* stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "LibXRay: SetTcEnhanceIntensity para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	int nTemp = stPara->numeratorValue / stPara->denominatorValue;
	XSP_CHECK(0 <= nTemp && 100 >= nTemp, XRAY_LIB_INVALID_PARAM, "LibXRay: SetTcEnhanceIntensity illegal para %d", nTemp);
	m_modImgproc.m_nHighLumSensity = nTemp;

    std::pair<uint16_t, uint16_t> highlight = std::make_pair(
        std::max(m_sharedPara.m_nBackGroundThreshold - (100 - m_modImgproc.m_nHighLumSensity) * 75, 0), // 高亮起始
        m_sharedPara.m_nBackGroundThreshold);
	m_islPipeComm.SetBrightnessParams(m_modImgproc.m_nLumIntensity, highlight);
	m_modImgproc.m_islPipeGray.SetNreeParams(0.12 * m_modImgproc.m_nSharpnessRatio);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetIntegralTime([[maybe_unused]] XRAYLIB_CONFIG_PARAM * stPara)
{
	/* TODO 暂无作用 */
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetColdAndHotCali(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetColdAndHotCali para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);
	XRAY_LIB_ONOFF ColdAndHotCali = (XRAY_LIB_ONOFF)stPara->numeratorValue;
	XSP_CHECK(ColdAndHotCali >= XRAYLIB_ONOFF_MIN_VALUE &&
		ColdAndHotCali <= XRAYLIB_ONOFF_MAX_VALUE,
		XRAY_LIB_INVALID_PARAM, "SetColdAndHotCali.Illegal param : %d",
		ColdAndHotCali);

	if (XRAYLIB_ON == ColdAndHotCali)
	{
		m_modCali.m_TimeColdHotParaH.nColdHotTik = 0;
		m_modCali.m_TimeColdHotParaL.nColdHotTik = 0;
		m_modCali.m_enColdAndHot = XRAYLIB_ON;
	}
	else
	{
		m_modCali.m_enColdAndHot = XRAYLIB_OFF;
	}
	log_info("SetColdAndHotCali: %f", ColdAndHotCali);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetEuTestPrompt(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetEuTestPrompt para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);
	XRAY_LIB_ONOFF EuTestPrompt = (XRAY_LIB_ONOFF)stPara->numeratorValue;
	XSP_CHECK(EuTestPrompt >= XRAYLIB_ONOFF_MIN_VALUE &&
		EuTestPrompt <= XRAYLIB_ONOFF_MAX_VALUE,
		XRAY_LIB_INVALID_PARAM, "EuTestPrompt.Illegal param : %d", EuTestPrompt);

	if (m_modImgproc.m_enTestAutoLE != EuTestPrompt)
	{
		m_modImgproc.m_enTestAutoLE = EuTestPrompt;
		m_modDual.m_enTestAutoLE = EuTestPrompt;


		log_info("EuTestPrompt: %d", (int)EuTestPrompt);
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetLogSaveMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetLogSaveMode para is null.");
	tagLogLevel LogSaveMode = (tagLogLevel)stPara->numeratorValue;
	XSP_CHECK(LogSaveMode >= LOG_LEVEL_PRINT_ONLY && LogSaveMode <= LOG_LEVEL_MAX,
		XRAY_LIB_INVALID_PARAM, "LogSaveMode.Illegal param : %d", LogSaveMode);

	g_LogSaveMode = LogSaveMode;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetLogLevel(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetLogLevel para is null.");
	LOG_LEVEL Level = (LOG_LEVEL)stPara->numeratorValue;
	XSP_CHECK(Level >= LOG_TRACE && Level <= LOG_FATAL,
		XRAY_LIB_INVALID_PARAM, "LOG_LEVEL.Illegal param : %d", Level);

	g_LogLevel = Level;

	// isl模块日志
	std::string strIslLogLevel = "warning";
	switch (Level)
	{
		case 0:
			strIslLogLevel = "trace";
			break;
		case 1:
			strIslLogLevel = "debug";
			break;
		case 2:
			strIslLogLevel = "info";
			break;
		case 3:
			strIslLogLevel = "warning";
			break;
		case 4:
			strIslLogLevel = "error";
			break;
		case 5:
			strIslLogLevel = "critical";
			break;
		default:
			log_info("SetLogLevel: %d failed.", (int)Level);
			break;
	}
	isl::SetLogConsoleLevel(strIslLogLevel);
	isl::SetLogFileLevel(strIslLogLevel);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetTimeCostShow([[maybe_unused]] XRAYLIB_CONFIG_PARAM* stPara)
{
	m_modImgproc.timeCostShow();
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetDetectionMode(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetDetectionMode para is null.");
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER != m_sharedPara.m_enEnergyMode, 
		      XRAY_LIB_OK, "Illegal energyMode: %d", m_sharedPara.m_enEnergyMode);

	XRAYLIB_EXPLOSIVEDETECTION_MODE mode = (XRAYLIB_EXPLOSIVEDETECTION_MODE)stPara->numeratorValue;
	XSP_CHECK(mode >= XRAYLIB_EXPLOSIVEDETECTION_MIN_VALUE &&
		mode <= XRAYLIB_EXPLOSIVEDETECTION_MAX_VALUE,
		XRAY_LIB_INVALID_PARAM, "SetDetectionMode.Illegal param : %d",
		mode);

	m_modArea.m_enDetectionMode = mode;

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetGeometricRatio(XRAYLIB_CONFIG_PARAM * stPara)
{
	XSP_CHECK(stPara, XRAY_LIB_NULLPTR, "SetGeometricRatio para is null.");

	XRAY_LIB_HRESULT hr = XRAY_LIB_OK;

	float denominatorValue = 100.0f;

	float fGeometricRatio = (float)(1 - stPara->numeratorValue / denominatorValue );

	XSP_CHECK(fGeometricRatio >= 0.0f && fGeometricRatio <= 1.0f, XRAY_LIB_INVALID_PARAM,
				"SetGeometricRatio Failed. Illegal Param: %d", stPara->numeratorValue);

	m_modCali.m_fGeomFactor = fGeometricRatio;
	hr = m_modCali.SetGeoCaliTableFacotr();
	return hr;
}