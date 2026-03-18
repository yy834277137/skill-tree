#include "xray_image_process.hpp"

XRAY_LIB_HRESULT XRayImageProcess::SetTipImage(XRAYLIB_IMAGE& xraylib_tip_img, int packageHeight, int randomPosition)
{
	XRAY_LIB_HRESULT hr;
	/*应用要求不做长度检查，避免大图插不进去*/
	XSP_CHECK(packageHeight > static_cast<int>(xraylib_tip_img.height),
			  XRAY_LIB_TIPSIZE_ERROR, "PackageHeight : %d, TipHeight: %d", packageHeight, xraylib_tip_img.height);
	
	XSP_CHECK(m_nTipedHeight >= m_nTipHeight, XRAY_LIB_TIP_WORKING);

	XSP_CHECK(randomPosition >= 0 && randomPosition <= 8, XRAY_LIB_INVALID_PARAM, "randomPosition: %d", randomPosition);

	int nTipHei = xraylib_tip_img.height;
	int nTipWid = xraylib_tip_img.width;

	/* 低能TIP */
	if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
		XRAYLIB_ENERGY_LOW == m_sharedPara.m_enEnergyMode)
	{
		XSP_CHECK(xraylib_tip_img.pData[0], XRAY_LIB_NULLPTR);
		hr = m_matTipLow.Reshape(nTipHei, nTipWid, XSP_16UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
		memcpy(m_matTipLow.Ptr(), xraylib_tip_img.pData[0], nTipHei * nTipWid * XSP_ELEM_SIZE(XSP_16UC1));
	}

	/* 高能TIP */
	if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
		XRAYLIB_ENERGY_HIGH == m_sharedPara.m_enEnergyMode)
	{
		XSP_CHECK(xraylib_tip_img.pData[1], XRAY_LIB_NULLPTR);
		hr = m_matTipHigh.Reshape(nTipHei, nTipWid, XSP_16UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
		memcpy(m_matTipHigh.Ptr(), xraylib_tip_img.pData[1], nTipHei * nTipWid * XSP_ELEM_SIZE(XSP_16UC1));
	}

	/* 原子序数TIP */
	if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode)
	{
		XSP_CHECK(xraylib_tip_img.pData[2], XRAY_LIB_NULLPTR);
		hr = m_matTipZValue.Reshape(nTipHei, nTipWid, XSP_8UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
		memcpy(m_matTipZValue.Ptr(), xraylib_tip_img.pData[2], nTipHei * nTipWid * XSP_ELEM_SIZE(XSP_8UC1));
	}
	
	m_nNeedTip = 1;
	m_nTipHeight = nTipHei;
	m_nTipWidth = nTipWid;
	m_nTipedHeight = 0;
	m_nTipWidthStart = -1;
	m_nTipStripeStart = -1;
	m_nPackageHeight = packageHeight;	
	m_nPackageHeightIn = packageHeight;
	m_nCount = 0;
	m_nRandomPosition = randomPosition;

	/*双视角共有变量*/
	m_SnTipHeightRand = -1;
	m_SbMainReady = false;
	m_SbPackReady = false;
	m_SbPackHeightRemian = packageHeight;
	m_SeStatus = XRAYLIB_TIP_NONE;
	log_info("nTipHei: %d, nTipWid: %d, packageHeight: %d, randomPosition", 
		nTipHei, nTipWid, packageHeight, m_nRandomPosition);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::ForceStopTIP()
{
	m_nNeedTip = 0;
	m_nTipHeight = 0;
	m_nTipWidth = 0;
	m_nTipedHeight = 0;
	m_nTipWidthStart = -1;
	m_nTipStripeStart = -1;
	m_nPackageHeight = 0;
	m_nPackageHeightIn = 0;

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::DetTIPPackageWidth(XMat& TIPData, int nhStart, int nhLength, int& ntopWidth, int& nbottomWidth)
{
	int nhEnd = nhStart + nhLength;
	/* 求包裹位置上下界 */
	for (int nw = 0; nw < m_nTipWidth; nw++)
	{
		int nPackageNum = 0;
		for (int nh = nhStart; nh < nhEnd; nh++)
		{
			if (TIPData.Ptr<ushort>(nh, nw)[0] < (ushort)m_sharedPara.m_nBackGroundThreshold)
			{
				nPackageNum++;
			}
		}
		if (nPackageNum > 5)
		{
			if (ntopWidth == m_nTipWidth)
			{
				ntopWidth = nw;
			}
			else
			{
				nbottomWidth = nw;
			}
		}
	}
	log_info("TIP nhStart: %d, nhEnd : %d", nhStart, nhEnd);
	/*如果当前条带数据不含包裹，边界设置位整图边界*/
	if (m_nTipWidth == ntopWidth || 0 == nbottomWidth)
	{
		ntopWidth = 0;
		nbottomWidth = m_nTipWidth;
	}

	if (ntopWidth >= nbottomWidth)
	{
		log_error("TIP Width Wrong! topWidth: %d, bottomWidth: %d", ntopWidth, nbottomWidth);
		return XRAY_LIB_TIPSIZE_ERROR;
	}
	log_info("TIP detect topWidth: %d, bottomWidth: %d", ntopWidth, nbottomWidth);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::RealTimeTipImage_CA(XMat& caliH, XMat& caliL, XMat& zData, XMat& mask, XMat& merge,
	XRAYLIB_TIP_STATUS& TipStatus, int32_t& nTipedPosW, XRAYLIB_RTPROC_PACKAGE* packpos)
{
	/* check para */
	if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode)
	{
		XSP_CHECK(caliH.IsValid() && caliL.IsValid() &&
			zData.IsValid() && merge.IsValid(),
			XRAY_LIB_XMAT_INVALID);

		XSP_CHECK(MatSizeEq(caliH, caliL) && MatSizeEq(caliH, zData) &&
			MatSizeEq(caliH, merge), XRAY_LIB_XMAT_SIZE_ERR);

		XSP_CHECK(XSP_16UC1 == caliH.type && XSP_16UC1 == caliL.type &&
			XSP_8UC1 == zData.type && XSP_16UC1 == merge.type,
			XRAY_LIB_XMAT_SIZE_ERR);
	}
	else if (XRAYLIB_ENERGY_LOW == m_sharedPara.m_enEnergyMode)
	{
		XSP_CHECK(caliL.IsValid() && merge.IsValid(), XRAY_LIB_XMAT_INVALID);

		XSP_CHECK(MatSizeEq(caliL, merge), XRAY_LIB_XMAT_SIZE_ERR);

		XSP_CHECK(XSP_16UC1 == caliL.type && XSP_16UC1 == merge.type, XRAY_LIB_XMAT_SIZE_ERR);
	}
	else if (XRAYLIB_ENERGY_HIGH == m_sharedPara.m_enEnergyMode)
	{
		XSP_CHECK(caliH.IsValid() && merge.IsValid(), XRAY_LIB_XMAT_INVALID);

		XSP_CHECK(MatSizeEq(caliH, merge), XRAY_LIB_XMAT_SIZE_ERR);

		XSP_CHECK(XSP_16UC1 == caliH.type && XSP_16UC1 == merge.type, XRAY_LIB_XMAT_SIZE_ERR);
	}
	else
	{
		return XRAY_LIB_ENERGYMODE_ERROR;
	}

	XRAY_LIB_VISUAL enVisual = m_sharedPara.m_enDeviceInfo.xraylib_visualNo;
	int nCount = m_nCount;
	m_nCount++;
	/* 插入完成返回 */
	if (m_nTipedHeight >= m_nTipHeight)
	{
		TipStatus = XRAYLIB_TIP_END;
		m_nNeedTip = 0;
		m_nTipHeight = 0;
		m_nTipWidth = 0;
		m_nTipedHeight = 0;
		m_nTipWidthStart = -1;
		m_nTipStripeStart = -1;
		m_nPackageHeight = 0;

		log_info("Finished: enVisual: %d, nCount: %d, TipStatus: %d", enVisual, nCount, TipStatus);
		return XRAY_LIB_OK;
	}
	int nWidIn, nHeiIn;
	nWidIn = merge.wid;
	nHeiIn = merge.hei;
	int nTPad = merge.tpad;
	int nBPad = merge.bpad;
	int nSkip = 2;
	/**********************************************
	*            获取插入位置信息
	***********************************************/
	if (XRAYLIB_TIP_FAILED == m_SeStatus || XRAYLIB_TIP_INTERRUPT == m_SeStatus)
	{
		if (0 < m_nTipedHeight)
		{
			TipStatus = XRAYLIB_TIP_INTERRUPT;
		}
		else
		{
			TipStatus = XRAYLIB_TIP_FAILED;
		}
		log_info("FAILED: enVisual: %d, nCount: %d,m_nTipedHeight: %d, TipStatus: %d", 
			enVisual, nCount, m_nTipedHeight, TipStatus);
		return XRAY_LIB_TIPSIZE_ERROR;
	}

	/*根据包裹分割结果计算，保证分包与包裹分割结果统一*/
	int nPackNum = packpos->nPackNum;
    int nWidthPackMin = packpos->stPackPos->x + packpos->stPackPos->width - 1, nWidthPackMax = packpos->stPackPos->x;
	int nPackageWidthRange = 0;

	if (1 == nPackNum)
	{
		if (nWidIn <= nSkip * 2)
		{
			TipStatus = XRAYLIB_TIP_FAILED;
			m_SeStatus = XRAYLIB_TIP_FAILED;
			return XRAY_LIB_TIPSIZE_ERROR;
		}

		nWidthPackMin = nWidIn, nWidthPackMax = 0;
		int nPackageNum;
		/* 求包裹位置上下界 */
		for (int nw = nSkip; nw < nWidIn - nSkip; nw++)
		{
			nPackageNum = 0;
			for (int nh = nTPad; nh < nHeiIn - nBPad; nh++)
			{
				if (merge.Ptr<ushort>(nh, nw)[0] < (ushort)m_sharedPara.m_nBackGroundThreshold)
				{
					nPackageNum++;
				}
			}
			if (nPackageNum == nHeiIn - nTPad - nBPad)
			{
				if (nWidthPackMin == nWidIn)
				{
					nWidthPackMin = nw;
				}
				else
				{	
					nWidthPackMax = nw;
				}
			}
		}

		if (false == m_SbPackReady)
		{
			nPackageWidthRange = nWidthPackMax - nWidthPackMin;
			if (nPackageWidthRange <= m_nTipWidth || nPackageWidthRange <= 0)
			{
				/*剩余可插入长度减去一个条带 */ 
				m_nPackageHeight -= (nHeiIn - nTPad - nBPad);
				if (m_nTipHeight >= m_nPackageHeight)
				{
					log_info("enVisual: %d, nCount: %d, Remianed Height not enough for tip", enVisual, nCount);
					TipStatus = XRAYLIB_TIP_FAILED;
					m_SeStatus = XRAYLIB_TIP_FAILED;
					return XRAY_LIB_TIPSIZE_ERROR;
				}
				log_info("enVisual: %d, nCount: %d, package Width not enough for tip", enVisual, nCount);
				return XRAY_LIB_OK;
			}
			/*两个视角均满足插入条件时，再等待一个条带才可以插入*/
			if (XRAYLIB_VISUAL_MAIN == enVisual || XRAYLIB_VISUAL_SINGLE == enVisual)
			{
				m_SbMainReady = true;
			}
			else
			{
				return XRAY_LIB_VISUALVIEW_ERROR;
			}
			m_SbPackReady = m_SbMainReady; //主辅视角是否均可插入
			m_nPackageHeight -= (nHeiIn - nTPad - nBPad);
			if (m_nTipHeight >= m_nPackageHeight)
			{
				TipStatus = XRAYLIB_TIP_FAILED;
				m_SeStatus = XRAYLIB_TIP_FAILED;
				log_info("enVisual: %d, nCount: %d, m_nPackageHeight: %d, TipStatus: %d", 
					enVisual, nCount, m_nPackageHeight, TipStatus);
				return XRAY_LIB_TIPSIZE_ERROR;
			}
			m_SbPackHeightRemian = m_nPackageHeight;
			return XRAY_LIB_OK;
		}
	}
	else if (0 == nPackNum)
	{
		if (m_nPackageHeight < m_nPackageHeightIn)
		{
			if (0 < m_nTipedHeight)
			{
				TipStatus = XRAYLIB_TIP_INTERRUPT;
				m_SeStatus = XRAYLIB_TIP_INTERRUPT;
			}
			else
			{
				TipStatus = XRAYLIB_TIP_FAILED;
				m_SeStatus = XRAYLIB_TIP_FAILED;
			}
			log_info("Not Package, Tip stop. enVisual: %d, nCount: %d, m_nTipedHeight: %d, TipStatus: %d", 
				enVisual, nCount, m_nTipedHeight, TipStatus);
			return XRAY_LIB_TIPSIZE_ERROR;
		}
		else
		{
			log_info("Not Package, Tip continue. enVisual: %d, nCount: %d", enVisual, nCount);
			return XRAY_LIB_OK;
		}
	}
	//当主辅视角均可插入时计算过包方向的插入位置
	if (true == m_SbPackReady && -1 == m_nTipStripeStart)
	{
		nPackageWidthRange = nWidthPackMax - nWidthPackMin;	// 可插入宽度
		int nRandRangeHeight = m_SbPackHeightRemian - m_nTipHeight;	// 仅在余量范围内才可插入
		int nRandRangeWidth = nPackageWidthRange - m_nTipWidth; // 仅在余量范围内才可插入
		
		if (0 >= nRandRangeHeight || 0 >= nRandRangeWidth)
		{
			TipStatus = XRAYLIB_TIP_FAILED;
			m_SeStatus = XRAYLIB_TIP_FAILED;
			log_error("enVisual: %d, nCount: %d,nRandRangeHeight: %d,m_SbPackHeightRemian: %d,\
				m_nTipHeight: %d, nRandRangeWidth: %d, nPackageWidthRange: %d,m_nTipWidth: %d",
				enVisual, nCount, nRandRangeHeight, m_SbPackHeightRemian, m_nTipHeight, nRandRangeWidth, 
				nPackageWidthRange, m_nTipWidth);
			return XRAY_LIB_TIPSIZE_ERROR;
		}
		/*图像均分九个区域,line328会导致插入位置偏上，待优化*/
		int nRegionWidth = int(nRandRangeWidth / 3);
		int nRegionHeight = int(nRandRangeHeight / 3);
		int nCoorWid = m_vecCoordinates[m_nRandomPosition * 2];
		int nCoorHei = m_vecCoordinates[m_nRandomPosition * 2 + 1];
		/*计算注入起始坐标点*/
		m_nTipWidthStart = nRegionWidth * nCoorWid + nWidthPackMin;
		m_nTipStripeStart = nRegionHeight * nCoorHei / (nHeiIn - nTPad - nBPad);
		/*如果区域大于TIP尺寸，中心点对齐插入*/
		if (nRegionWidth > m_nTipWidth)
		{
			m_nTipWidthStart = nRegionWidth * nCoorWid + nRegionWidth / 2 - m_nTipWidth / 2 + nWidthPackMin;
		}
		if (nRegionHeight > m_nTipHeight)
		{
			m_nTipStripeStart = (nRegionHeight * nCoorHei + nRegionHeight / 2 - m_nTipHeight / 2) / (nHeiIn - nTPad - nBPad);
		}
		/*包裹上方三个位置，插入起始位置向下偏移10个像素*/
		if (0 == m_nRandomPosition || 1 == m_nRandomPosition || 2 == m_nRandomPosition)
		{
			log_info("enVisual: %d, nCount: %d, topPosition : %d, TipWidthStart before offset: %d, nRandRangeWidth: %d",
				enVisual, nCount, m_nRandomPosition, m_nTipWidthStart, nRandRangeWidth);
			if (10 < nRandRangeWidth)
			{
				m_nTipWidthStart += 10;
			}
			else 
			{
				m_nTipWidthStart = m_nTipWidthStart + nRandRangeWidth / 2;
			}
			log_info("enVisual: %d, nCount: %d, TipWidthStart after offset: %d",
				enVisual, nCount, m_nTipWidthStart);
		}
		log_info("enVisual: %d, nCount: %d, m_SbPackReady: nRegionWidth: %d, nRegionHeight: %d, \
				CoorWid: %d, nCoorHei: %d, m_nTipWidthStart:%d, m_nTipStripeStart: %d",
			enVisual, nCount, nRegionWidth, nRegionHeight, nCoorWid, nCoorHei, m_nTipWidthStart, m_nTipStripeStart);
		/* 插入位置错误判断 */
		if (m_nTipWidthStart < 0 || m_nTipWidthStart >= (nWidIn - m_nTipWidth))
		{
			TipStatus = XRAYLIB_TIP_FAILED;
			m_SeStatus = XRAYLIB_TIP_FAILED;
			log_error("tip width coor out of range: m_nTipWidthStart: %d, nWidIn: %d, m_nTipWidth: %d", 
				m_nTipWidthStart, nWidIn, m_nTipWidth);
			return XRAY_LIB_TIPWIDTH_ERROR;
		}

		if (m_nTipStripeStart * (nHeiIn - nTPad - nBPad) > m_nPackageHeight - m_nTipHeight)
		{
			TipStatus = XRAYLIB_TIP_FAILED;
			m_SeStatus = XRAYLIB_TIP_FAILED;
			log_error("tip height coor out of range: m_nTipStripeStart: %d, m_nPackageHeight: %d, m_nTipHeight: %d", 
				m_nTipStripeStart, m_nPackageHeight, m_nTipHeight);
			return XRAY_LIB_TIPWIDTH_ERROR;
		}
	}
	nTipedPosW = m_nTipWidthStart;


	/**********************************************
	*            TIP插入
	***********************************************/
	if (0 == m_nTipStripeStart)
	{
		/**********************************************
		*            TIP插入相关参数计算
		***********************************************/
		/*
		nTipTempStart  : 需插入TIP的条带起始高度坐标（带扩边）
		nTipRealStart  : 本次插入TIP数据的起始高度坐标
		nTipLength     : 本次需拷贝的TIP高度
		m_nTipedHeight : 已拷贝累加的TIP高度
		*/
		int nTipTempStart, nTipRealStart, nTipLength;
		if (m_nTipedHeight == 0)
		{
			/* 第一次插入 */
			nTipTempStart = nTPad;
			nTipRealStart = 0;
		}
		else if (m_nTipedHeight - nTPad <= 0)
		{
			/* 非第一次插入，已拷贝的tip高度小于扩边高度,
			* tip内存不会拷贝到算法库内部维护的扩边缓存
			* 这里需要重新拷贝*/
			nTipTempStart = nTPad - m_nTipedHeight;
			nTipRealStart = 0;
		}
		else
		{
			/* 非第一次插入，已拷贝的tip高度大于扩边高度 */
			nTipTempStart = 0;
			nTipRealStart = m_nTipedHeight - nTPad;
		}
		/* 拷贝高度计算 */
		if (nHeiIn - nTipTempStart > m_nTipHeight - nTipRealStart)
		{
			nTipLength = m_nTipHeight - nTipRealStart;
		}
		else
		{
			nTipLength = nHeiIn - nTipTempStart;
		}

		XRAY_LIB_HRESULT hr;
		int nTipPackTop = m_nTipWidth;
		int nTipPackBottom = 0;
		hr = DetTIPPackageWidth(m_matTipLow, nTipRealStart, nTipLength, nTipPackTop, nTipPackBottom);
		/*判断当前包裹边界是否满足已插入边界*/
		if(0 < nTipPackTop || m_nTipWidth > nTipPackBottom)//反之为空白，直接插入
		{	
			if (nWidthPackMin > (m_nTipWidthStart + nTipPackTop )|| nWidthPackMax < (m_nTipWidthStart + nTipPackBottom))
			{
				if (0 == m_nTipedHeight)
				{
					//未插入直接返回错位
					TipStatus = XRAYLIB_TIP_FAILED;
					m_SeStatus = XRAYLIB_TIP_FAILED;
					log_info("enVisual: %d, nCount: %d, Tip Interrupt: m_nTipedHeight: %d, m_nTipWidthStart: %d, \
							nTipPackTop: %d, nTipPackBottom: %d, nWidthPackMin: %d, nWidthPackMax: %d, TipStatus: %d",
						enVisual, nCount, m_nTipedHeight, m_nTipWidthStart, nTipPackTop, nTipPackBottom,
						nWidthPackMin, nWidthPackMax, TipStatus);
				}
				else
				{
					//插入一部分返回，dsp清除
					TipStatus = XRAYLIB_TIP_INTERRUPT;
					m_SeStatus = XRAYLIB_TIP_INTERRUPT;
					log_info("enVisual: %d, nCount: %d, Tip Interrupt: m_nTipedHeight: %d, m_nTipWidthStart: %d, \
							nTipPackTop: %d, nTipPackBottom: %d, nWidthPackMin: %d, nWidthPackMax: %d, TipStatus: %d",
						enVisual, nCount, m_nTipedHeight, m_nTipWidthStart,nTipPackTop, nTipPackBottom, 
						nWidthPackMin, nWidthPackMax,  TipStatus);
				}
				return XRAY_LIB_TIPWIDTH_ERROR;
			}
		}
		
		/*先插入实际条带数据*/
		hr = m_matTipedHigh.Reshape(nHeiIn, m_nTipWidth, XSP_16UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
		hr = m_matTipedLow.Reshape(nHeiIn, m_nTipWidth, XSP_16UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
		hr = m_matTipedZValue.Reshape(nHeiIn, m_nTipWidth, XSP_8UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);

		/* 内存清空 */
		memset(m_matTipedHigh.Ptr(), 0, m_matTipedHigh.Size());
		memset(m_matTipedLow.Ptr(), 0, m_matTipedLow.Size());
		memset(m_matTipedZValue.Ptr(), 0, m_matTipedZValue.Size());

		/* 临时内存先复制原图像 */
		for (int nh = 0; nh < nHeiIn; nh++)
		{
			for (int nw = 0; nw < m_nTipWidth; nw++)
			{
				if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
					XRAYLIB_ENERGY_HIGH == m_sharedPara.m_enEnergyMode)
				{
					m_matTipedHigh.Ptr<ushort>(nh, nw)[0] = caliH.Ptr<ushort>(nh, nw + m_nTipWidthStart)[0];
				}
				if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
					XRAYLIB_ENERGY_LOW == m_sharedPara.m_enEnergyMode)
				{
					m_matTipedLow.Ptr<ushort>(nh, nw)[0] = caliL.Ptr<ushort>(nh, nw + m_nTipWidthStart)[0];
				}
			}
		}

		/* 已拷贝tip高度累加 */
		m_nTipedHeight += (nHeiIn - nTPad - nBPad);
		TipStatus = XRAYLIB_TIP_WORKING;

		/**********************************************
		*                插入TIP数据
		***********************************************/
		float fTipRatio;
		float fResult;
		for (int nh = 0; nh < nTipLength; nh++)
		{
			for (int nw = 0; nw < m_nTipWidth; nw++)
			{
				if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
					XRAYLIB_ENERGY_HIGH == m_sharedPara.m_enEnergyMode)
				{
					fTipRatio = m_matTipHigh.Ptr<ushort>(nTipRealStart, nw)[0] / (0.97f * 65535.0f);
					fTipRatio = Clamp(fTipRatio, 0.0f, 1.0f);

					fResult = m_matTipedHigh.Ptr<ushort>(nTipTempStart, nw)[0] * fTipRatio;
					fResult = Clamp(fResult, 0.0f, 65535.0f);
					m_matTipedHigh.Ptr<ushort>(nTipTempStart, nw)[0] = (ushort)fResult;
				}
				if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
					XRAYLIB_ENERGY_LOW == m_sharedPara.m_enEnergyMode)
				{
					fTipRatio = m_matTipLow.Ptr<ushort>(nTipRealStart, nw)[0] / (0.97f * 65535.0f);
					fTipRatio = Clamp(fTipRatio, 0.0f, 1.0f);

					fResult = m_matTipedLow.Ptr<ushort>(nTipTempStart, nw)[0] * fTipRatio;
					fResult = Clamp(fResult, 0.0f, 65535.0f);
					m_matTipedLow.Ptr<ushort>(nTipTempStart, nw)[0] = (ushort)fResult;
				}
			}
			nTipRealStart++;
			nTipTempStart++;
		}

		/**********************************************
		*                双能或单能处理
		***********************************************/
		if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode)
		{
			hr = m_modDual.ZIdentify(m_matTipedHigh, m_matTipedLow, m_matTipedZValue);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);
		}
		else if (XRAYLIB_ENERGY_LOW == m_sharedPara.m_enEnergyMode)
		{
			hr = m_modDual.SingleZMapSimulate(m_matTipedLow, m_matTipedZValue, 0);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);
		}
		else if (XRAYLIB_ENERGY_HIGH == m_sharedPara.m_enEnergyMode)
		{
			hr = m_modDual.SingleZMapSimulate(m_matTipedHigh, m_matTipedZValue, 0);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);
		}
		else
		{
			return XRAY_LIB_ENERGYMODE_ERROR;
		}

		/**********************************************
		*        插入TIP后数据拷贝回原始条带
		***********************************************/
		for (int nh = 0; nh < nHeiIn; nh++)
		{
			for (int nw = 0; nw < m_nTipWidth; nw++)
			{
				if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
					XRAYLIB_ENERGY_HIGH == m_sharedPara.m_enEnergyMode)
				{
					caliH.Ptr<ushort>(nh, nw + m_nTipWidthStart)[0] = m_matTipedHigh.Ptr<ushort>(nh, nw)[0];
				}
				if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
					XRAYLIB_ENERGY_LOW == m_sharedPara.m_enEnergyMode)
				{
					caliL.Ptr<ushort>(nh, nw + m_nTipWidthStart)[0] = m_matTipedLow.Ptr<ushort>(nh, nw)[0];
				}
				zData.Ptr<uint8_t>(nh, nw + m_nTipWidthStart)[0] = m_matTipedZValue.Ptr<uint8_t>(nh, nw)[0];
				mask.Ptr<uint8_t>(nh, nw + m_nTipWidthStart)[0] = 0;
			}
		}
		log_info("enVisual: %d, nCount: %d, Processing: TipStatus: %d, m_nTipedHeight: %d", enVisual, nCount, TipStatus, m_nTipedHeight);
	}
	else
	{
		m_nTipStripeStart--;
		log_info("enVisual: %d, nCount: %d, Waiting for Tip:  m_nTipStripeStart: %d", enVisual, nCount, m_nTipStripeStart);
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::RealTimeTipImage(XMat& caliH, XMat& caliL, XMat& zData, XMat& merge,
	                                                XRAYLIB_TIP_STATUS& TipStatus, int& nTipedPosW)
{
	/* check para */
	if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode)
	{
		XSP_CHECK(caliH.IsValid() && caliL.IsValid() && 
			      zData.IsValid() && merge.IsValid(),
		          XRAY_LIB_XMAT_INVALID);

	    XSP_CHECK(MatSizeEq(caliH, caliL) && MatSizeEq(caliH, zData) && 
			      MatSizeEq(caliH, merge), XRAY_LIB_XMAT_SIZE_ERR);

	    XSP_CHECK(XSP_16UC1 == caliH.type && XSP_16UC1 == caliL.type && 
			      XSP_8UC1 == zData.type && XSP_16UC1 == merge.type, 
			      XRAY_LIB_XMAT_SIZE_ERR);
	}
	else if (XRAYLIB_ENERGY_LOW == m_sharedPara.m_enEnergyMode)
	{
		XSP_CHECK(caliL.IsValid() && merge.IsValid(), XRAY_LIB_XMAT_INVALID);

		XSP_CHECK(MatSizeEq(caliL, merge), XRAY_LIB_XMAT_SIZE_ERR);

		XSP_CHECK(XSP_16UC1 == caliL.type && XSP_16UC1 == merge.type, XRAY_LIB_XMAT_SIZE_ERR);
	}
	else if (XRAYLIB_ENERGY_HIGH == m_sharedPara.m_enEnergyMode)
	{
		XSP_CHECK(caliH.IsValid() && merge.IsValid(), XRAY_LIB_XMAT_INVALID);

		XSP_CHECK(MatSizeEq(caliH, merge), XRAY_LIB_XMAT_SIZE_ERR);

		XSP_CHECK(XSP_16UC1 == caliH.type && XSP_16UC1 == merge.type, XRAY_LIB_XMAT_SIZE_ERR);
	}
	else
	{
		return XRAY_LIB_ENERGYMODE_ERROR;
	}

	/* 插入完成返回 */
	if (m_nTipedHeight >= m_nTipHeight)
	{
		TipStatus = XRAYLIB_TIP_END;

		m_nNeedTip = 0;
		m_nTipHeight = 0;
		m_nTipWidth = 0;
		m_nTipedHeight = 0;
		m_nTipWidthStart = -1;
		m_nTipStripeStart = 0;

		return XRAY_LIB_OK;
	}

	int nWidIn, nHeiIn, nPad;
	nWidIn = merge.wid;
	nHeiIn = merge.hei;
	nPad = merge.tpad;

	/************************************** 
	*        第一次插入位置计算 
	***************************************/
	if (m_nTipWidthStart == -1)
	{
		int nSkip = 32; //图像上方和下方跳过的列数

		if (nWidIn <= nSkip * 2)
		{
			TipStatus = XRAYLIB_TIP_FAILED;
			return XRAY_LIB_TIPSIZE_ERROR;
		}

		///寻找一个探测器方向上随机的位置插入。
		int nWidthPackMin = nWidIn, nWidthPackMax = 0;
		int nPackageNum;

		/* 求包裹位置下界 */
		for (int nw = nSkip; nw < nWidIn; nw++)
		{
			nPackageNum = 0;
			for (int nh = nPad; nh < nHeiIn - nPad; nh++)
			{
				if (merge.Ptr<ushort>(nh, nw)[0] < (ushort)m_sharedPara.m_nBackGroundThreshold)
				{
					nPackageNum++;
				}
			}
			if (nPackageNum == nHeiIn - 2 * nPad)
			{
				nWidthPackMin = nw;
				break;
			}
		}
		/* 求包裹位置上界 */
		for (int nw = nWidIn - nSkip; nw > nSkip; nw--)
		{
			nPackageNum = 0;
			for (int nh = nPad; nh < nHeiIn - nPad; nh++)
			{
				if (merge.Ptr<ushort>(nh, nw)[0] < (ushort)m_sharedPara.m_nBackGroundThreshold)
				{
					nPackageNum++;
				}
			}
			if (nPackageNum == nHeiIn - 2 * nPad)
			{
				nWidthPackMax = nw;
				break;
			}
		}

		//求可插入范围
		if (nWidthPackMax <= nWidthPackMin)
		{
			TipStatus = XRAYLIB_TIP_FAILED;
			return XRAY_LIB_TIPSIZE_ERROR;
		}
		int nPackageRange = nWidthPackMax - nWidthPackMin;

		if (nPackageRange <= m_nTipWidth)
		{
			//当第一个包裹条带宽度小于tip宽度时，多判断一个条带
			if (0 == m_nTipStripeStart)
			{
				nTipedPosW = 0;
				log_error("The first package stripe");
				m_nTipStripeStart++;
				return XRAY_LIB_OK;
			}
			TipStatus = XRAYLIB_TIP_FAILED;
			return XRAY_LIB_TIPSIZE_ERROR;
		}

		log_info("PackRange: %d; PackWidMax: %d; PackWidMin: %d; TipWid: %d;",
                  nPackageRange, nWidthPackMax, nWidthPackMin, m_nTipWidth);

		int nRandRange = nPackageRange - m_nTipWidth;

		/* 产生随机插入位置 */
		srand((int)time(0));
		m_nTipWidthStart = rand() % nRandRange + nWidthPackMin;
	}

	nTipedPosW = m_nTipWidthStart;

	/* 插入位置错误判断 */
	if (m_nTipWidthStart < 0 || m_nTipWidthStart >= (nWidIn - m_nTipWidth))
	{
		log_warn("nTipedPosW: %d", nTipedPosW);

		TipStatus = XRAYLIB_TIP_FAILED;
		return XRAY_LIB_TIPWIDTH_ERROR;
	}

	/**********************************************
	*            先插入实际条带数据 
	***********************************************/
	XRAY_LIB_HRESULT hr;
	hr = m_matTipedHigh.Reshape(nHeiIn, m_nTipWidth, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	hr = m_matTipedLow.Reshape(nHeiIn, m_nTipWidth, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	hr = m_matTipedZValue.Reshape(nHeiIn, m_nTipWidth, XSP_8UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	/* 内存清空 */
	memset(m_matTipedHigh.Ptr(), 0, m_matTipedHigh.Size());
	memset(m_matTipedLow.Ptr(), 0, m_matTipedLow.Size());
	memset(m_matTipedZValue.Ptr(), 0, m_matTipedZValue.Size());

	/* 临时内存先复制原图像 */
	for (int nh = 0; nh < nHeiIn; nh++)
	{
		for (int nw = 0; nw < m_nTipWidth; nw++)
		{
			if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
				XRAYLIB_ENERGY_HIGH == m_sharedPara.m_enEnergyMode)
			{
				m_matTipedHigh.Ptr<ushort>(nh, nw)[0] = caliH.Ptr<ushort>(nh, nw + m_nTipWidthStart)[0];
			}
			if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
				XRAYLIB_ENERGY_LOW == m_sharedPara.m_enEnergyMode)
			{
				m_matTipedLow.Ptr<ushort>(nh, nw)[0] = caliL.Ptr<ushort>(nh, nw + m_nTipWidthStart)[0];
			}
		}
	}

	/**********************************************
	*            TIP插入相关参数计算
	***********************************************/
	/*
	nTipTempStart  : 需插入TIP的条带起始高度坐标（带扩边）
	nTipRealStart  : 本次插入TIP数据的起始高度坐标
	nTipLength     : 本次需拷贝的TIP高度
	m_nTipedHeight : 已拷贝累加的TIP高度
	*/
	int nTipTempStart, nTipRealStart, nTipLength;
	if (m_nTipedHeight == 0)
	{
		/* 第一次插入 */
		nTipTempStart = nPad;
		nTipRealStart = 0;
	}
	else if (m_nTipedHeight - nPad <= 0)
	{
		/* 非第一次插入，已拷贝的tip高度小于扩边高度,
		* tip内存不会拷贝到算法库内部维护的扩边缓存
		* 这里需要重新拷贝*/
		nTipTempStart = nPad - m_nTipedHeight;
		nTipRealStart = 0;
	}
	else
	{
		/* 非第一次插入，已拷贝的tip高度大于扩边高度 */
		nTipTempStart = 0;
		nTipRealStart = m_nTipedHeight - nPad;
	}
	/* 拷贝高度计算 */
	if (nHeiIn - nTipTempStart > m_nTipHeight - nTipRealStart)
	{
		nTipLength = m_nTipHeight - nTipRealStart;
	}
	else
	{
		nTipLength = nHeiIn - nTipTempStart;
	}
	/* 已拷贝tip高度累加 */
	m_nTipedHeight += (nHeiIn - nPad * 2);
	TipStatus = XRAYLIB_TIP_WORKING;

	/**********************************************
	*                插入TIP数据
	***********************************************/
	float fTipRatio;
	float fResult;
	for (int nh = 0; nh < nTipLength; nh++)
	{
		for (int nw = 0; nw < m_nTipWidth; nw++)
		{
			if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
				XRAYLIB_ENERGY_HIGH == m_sharedPara.m_enEnergyMode)
			{
				fTipRatio = m_matTipHigh.Ptr<ushort>(nTipRealStart, nw)[0] / (0.97f * 65535.0f);
				fTipRatio = Clamp(fTipRatio, 0.0f, 1.0f);

				fResult = m_matTipedHigh.Ptr<ushort>(nTipTempStart, nw)[0] * fTipRatio;
				fResult = Clamp(fResult, 0.0f, 65535.0f);
				m_matTipedHigh.Ptr<ushort>(nTipTempStart, nw)[0] = (ushort)fResult;
			}
			if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
				XRAYLIB_ENERGY_LOW == m_sharedPara.m_enEnergyMode)
			{
				fTipRatio = m_matTipLow.Ptr<ushort>(nTipRealStart, nw)[0] / (0.97f * 65535.0f);
				fTipRatio = Clamp(fTipRatio, 0.0f, 1.0f);

				fResult = m_matTipedLow.Ptr<ushort>(nTipTempStart, nw)[0] * fTipRatio;
				fResult = Clamp(fResult, 0.0f, 65535.0f);
				m_matTipedLow.Ptr<ushort>(nTipTempStart, nw)[0] = (ushort)fResult;
			}
		}
		nTipRealStart++;
		nTipTempStart++;
	}

	/**********************************************
	*                双能或单能处理
	***********************************************/
	if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode)
	{
		hr = m_modDual.ZIdentify(m_matTipedHigh, m_matTipedLow, m_matTipedZValue);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
	}
	else if (XRAYLIB_ENERGY_LOW == m_sharedPara.m_enEnergyMode)
	{
		hr = m_modDual.SingleZMapSimulate(m_matTipedLow, m_matTipedZValue, 0);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
	}
	else if (XRAYLIB_ENERGY_HIGH == m_sharedPara.m_enEnergyMode)
	{
		hr = m_modDual.SingleZMapSimulate(m_matTipedHigh, m_matTipedZValue, 0);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
	}
	else
	{
		return XRAY_LIB_ENERGYMODE_ERROR;
	}

	/**********************************************
	*        插入TIP后数据拷贝回原始条带
	***********************************************/
	for (int nh = 0; nh < nHeiIn; nh++)
	{
		for (int nw = 0; nw < m_nTipWidth; nw++)
		{
			if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
				XRAYLIB_ENERGY_HIGH == m_sharedPara.m_enEnergyMode)
			{
				caliH.Ptr<ushort>(nh, nw + m_nTipWidthStart)[0] = m_matTipedHigh.Ptr<ushort>(nh, nw)[0];
			}
			if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
				XRAYLIB_ENERGY_LOW == m_sharedPara.m_enEnergyMode)
			{
				caliL.Ptr<ushort>(nh, nw + m_nTipWidthStart)[0] = m_matTipedLow.Ptr<ushort>(nh, nw)[0];
			}
			zData.Ptr<uint8_t>(nh, nw + m_nTipWidthStart)[0] = m_matTipedZValue.Ptr<uint8_t>(nh, nw)[0];
		}
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::DumpProcTipImage(XMat& caliL, XMat& caliH, XMat& zData, XMat& mask, XRAYLIB_DUMPPROC_TIP_PARAM &tipParams)
{
	/* 如果当前不需要插入tip,直接返回*/
	if (!tipParams.enTipflag)
	{
		return XRAY_LIB_OK;
	}

	XSP_CHECK(tipParams.nHeight < caliL.hei && tipParams.nWidth < caliL.wid, XRAY_LIB_TIPSIZE_ERROR);
	XSP_CHECK(tipParams.nHeight + tipParams.nTipedPosX < caliL.hei && tipParams.nWidth + tipParams.nTipedPosY < caliL.wid, XRAY_LIB_TIPSIZE_ERROR);

    XSP_CHECK(caliL.IsValid(), XRAY_LIB_XMAT_INVALID);
    XSP_CHECK(XSP_16UC1 == caliL.type, XRAY_LIB_XMAT_TYPE_ERR);
    XSP_CHECK(zData.IsValid(), XRAY_LIB_XMAT_INVALID);
    XSP_CHECK(XSP_8UC1 == zData.type, XRAY_LIB_XMAT_TYPE_ERR);
    XSP_CHECK(MatSizeEq(caliL, zData), XRAY_LIB_XMAT_SIZE_ERR);
	if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode)
	{
		XSP_CHECK(caliH.IsValid(), XRAY_LIB_XMAT_INVALID);
		XSP_CHECK(MatSizeEq(caliL, caliH), XRAY_LIB_XMAT_SIZE_ERR);
		XSP_CHECK(XSP_16UC1 == caliH.type, XRAY_LIB_XMAT_TYPE_ERR);
	}
	else
	{
		return XRAY_LIB_ENERGYMODE_ERROR;
	}

	int nTipWid, nTipHei, nTipStartW, nTipStartH;
	nTipWid = tipParams.nWidth;
	nTipHei = tipParams.nHeight;

	/*注意！要求的X方向就是过包方向,Y是探测器方向*/
	nTipStartW = tipParams.nTipedPosY;
	nTipStartH = tipParams.nTipedPosX;

	/**********************************************
	*            先插入实际条带数据
	***********************************************/
	XRAY_LIB_HRESULT hr;
	hr = m_matTipedHigh.Reshape(nTipHei, nTipWid, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	hr = m_matTipedLow.Reshape(nTipHei, nTipWid, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	hr = m_matTipedZValue.Reshape(nTipHei, nTipWid, XSP_8UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	hr = m_matTipHigh.Reshape(nTipHei, nTipWid, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	hr = m_matTipLow.Reshape(nTipHei, nTipWid, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	hr = m_matTipZValue.Reshape(nTipHei, nTipWid, XSP_8UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	/* tiped内存清空 */
	/* tip数据拷贝 */
	if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
		XRAYLIB_ENERGY_HIGH == m_sharedPara.m_enEnergyMode)
	{
		//memset(m_matTipedHigh.Ptr<ushort>(), 0, nTipHei*nTipWid);
		memcpy(m_matTipHigh.Ptr<ushort>(), tipParams.pTipedCaliHighData, nTipHei*nTipWid * sizeof(ushort));
	}
	if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode ||
		XRAYLIB_ENERGY_LOW == m_sharedPara.m_enEnergyMode)
	{
		//memset(m_matTipedLow.Ptr<ushort>(), 0, nTipHei*nTipWid);
		memcpy(m_matTipLow.Ptr<ushort>(), tipParams.pTipedCaliLowData, nTipHei*nTipWid * sizeof(ushort));
	}
	if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode)
	{
		//memset(m_matTipedZValue.Ptr<uint8_t*>(), 0, nTipHei*nTipWid);
		memcpy(m_matTipZValue.Ptr<uint8_t>(), tipParams.pTipedZData, nTipHei*nTipWid * sizeof(uint8_t));
	}
	
	/* 临时内存先复制原图像 */
	float fTipRatio;
	float fResult;
	for (int nh = 0; nh < nTipHei; nh++)
	{
		for (int nw = 0; nw < nTipWid; nw++)
		{
            /******************** 低能 ********************/
            //先拷贝原始数据
            m_matTipedLow.Ptr<ushort>(nh, nw)[0] = caliL.Ptr<ushort>(nh+ nTipStartH, nw + nTipStartW)[0];

            //灰度叠加
            fTipRatio = m_matTipLow.Ptr<ushort>(nh, nw)[0] / (0.97f * 65535.0f);
            fTipRatio = Clamp(fTipRatio, 0.0f, 1.0f);

            fResult = m_matTipedLow.Ptr<ushort>(nh, nw)[0] * fTipRatio;
            fResult = Clamp(fResult, 0.0f, 65535.0f);
            m_matTipedLow.Ptr<ushort>(nh, nw)[0] = (ushort)fResult;

            /******************** 高能 ********************/
			if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode)
			{
				//先拷贝原始数据
				m_matTipedHigh.Ptr<ushort>(nh, nw)[0] = caliH.Ptr<ushort>(nh + nTipStartH, nw + nTipStartW)[0];

				//灰度叠加
				fTipRatio = m_matTipHigh.Ptr<ushort>(nh, nw)[0] / (0.97f * 65535.0f);
				fTipRatio = Clamp(fTipRatio, 0.0f, 1.0f);

				fResult = m_matTipedHigh.Ptr<ushort>(nh, nw)[0] * fTipRatio;
				fResult = Clamp(fResult, 0.0f, 65535.0f);
				m_matTipedHigh.Ptr<ushort>(nh, nw)[0] = (ushort)fResult;
			}
		}
	}

	/**********************************************
	*                双能或单能处理
	***********************************************/
	if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode)
	{
		hr = m_modDual.ZIdentify(m_matTipedHigh, m_matTipedLow, m_matTipedZValue);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
	}

	/**********************************************
	*        插入TIP后数据拷贝回原始条带
	***********************************************/
	for (int nh = 0; nh < nTipHei; nh++)
	{
		for (int nw = 0; nw < nTipWid; nw++)
		{
            caliL.Ptr<ushort>(nh + nTipStartH, nw + nTipStartW)[0] = m_matTipedLow.Ptr<ushort>(nh, nw)[0];
            zData.Ptr<uint8_t>(nh + nTipStartH, nw + nTipStartW)[0] = m_matTipedZValue.Ptr<uint8_t>(nh, nw)[0];
			mask.Ptr<uint8_t>(nh + nTipStartH, nw + nTipStartW)[0] = 0;
			if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode)
			{
				caliH.Ptr<ushort>(nh + nTipStartH, nw + nTipStartW)[0] = m_matTipedHigh.Ptr<ushort>(nh, nw)[0];
			}
		}
	}
	return XRAY_LIB_OK;
}