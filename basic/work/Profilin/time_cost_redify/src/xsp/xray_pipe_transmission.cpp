#include "xray_image_process.hpp"

XRAY_LIB_HRESULT XRayImageProcess::PipelineProcess3A(XRAYLIB_PIPELINE_PARAM_MODE3A& pipe3A, int32_t index)
{
	XRAY_LIB_HRESULT hr;
	if (0 == index)
	{
		hr = PipeDual3A_PreProcess(pipe3A.idx0);
	}
	else if (1 == index)
	{
		hr = PipeDual3A_CnnProcess(pipe3A.idx1);
	}
	else if (2 == index)
	{
		hr = PipeDual3A_PostProcess(pipe3A.idx2);
	}
	else
	{
		hr = XRAY_LIB_INVALID_PARAM;
	}
	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::PipeDual3A_PreProcess(XRAYLIB_PIPELINE_PARAM_MODE3A::_pipeline_3a_idx0_& idx0)
{
	XSP_CHECK(XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode, XRAY_LIB_ENERGYMODE_ERROR);
    XRAY_LIB_HRESULT hr;

	m_AllowSyncOperation.store(false);

	/************************************************************************
	*                        设置XRAW数据处理相关参数
	************************************************************************/
	/* 初始化输入变量 */ 
	int32_t nIdx0WidIn = idx0.in_xraw.width;
	int32_t nIdx0HeiIn = idx0.in_xraw.height;

	/* 下扩边的条带数量 */ 
	int32_t nPadCacheNum = ((XRAYLIB_TESTMODE_CN_CHECK == m_modImgproc.m_enTestMode) ? (m_modTcproc.m_nStripeBufferNum - 1) : 1);

	/* OriRaw的扩边设置需要和后续处理的扩边设置一致 */
	std::pair<int32_t, int32_t> nOriRawTBPad(nIdx0HeiIn, nPadCacheNum * nIdx0HeiIn);

	/* 计算流水需要输出的缩放尺寸，用于后段流水中缩放 */
	UpdateResizePara(nIdx0WidIn, nIdx0HeiIn);

	/************************************************************************
	*         输入XRAW数据校正处理：Xraw数据归一化、几何校正、脏图去除等
	************************************************************************/
	XMat matXrawL, matXrawH;
	hr = matXrawL.Init(nIdx0HeiIn, nIdx0WidIn, XSP_16UC1, (uint8_t*)idx0.in_xraw.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: Get xraw_in low data failed.");

	hr = matXrawH.Init(nIdx0HeiIn, nIdx0WidIn, XSP_16UC1, (uint8_t*)idx0.in_xraw.pData[1]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: Get xraw_in high data failed");

	/* 条带状态判断，用于自动更新校正数据 */ 
	hr = m_modCali.StripeTypeJudge(matXrawL);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: StripeTypeJudge failed");

	/* 高低能数据归一化、校正相关处理 */
	hr = m_matLTemp.Reshape(nIdx0HeiIn, nIdx0WidIn, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: m_matLow reshape failed");

	hr = m_matOriLTemp.Reshape(nIdx0HeiIn, nIdx0WidIn, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: m_matOriLTemp reshape failed");

	hr = m_modCali.CaliLImage(matXrawL, m_matLTemp, m_matOriLTemp);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: Cali low data failed");

	hr = m_matHTemp.Reshape(nIdx0HeiIn, nIdx0WidIn, XSP_16UC1); 
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: m_matHigh reshape failed");

	hr = m_matOriHTemp.Reshape(nIdx0HeiIn, nIdx0WidIn, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: m_matOriHTemp reshape failed");

	hr = m_modCali.CaliHImage(matXrawH, m_matHTemp, m_matOriHTemp);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: Cali high data failed");

    /* 若重置模板，先清除队列中已有的条带信息 */
    if (m_modCali.m_stOriLPadCache.bClearTemp)
    {
        m_queFlag.clear();
        quePack.clear();
    }

	/* 设置OriRaw的上下扩边高度 */
	hr = m_modCali.SetTBpad(nIdx0WidIn, nIdx0HeiIn, XSP_16UC1, nOriRawTBPad, m_modCali.m_stOriLPadCache);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: SetStripeTBpad failed");

	hr = m_modCali.SetTBpad(nIdx0WidIn, nIdx0HeiIn, XSP_8UC1, nOriRawTBPad, m_modCali.m_stMaskCache);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: SetStripeTBpad failed");

	hr = m_modCali.SetTBpad(nIdx0WidIn, nIdx0HeiIn, XSP_16UC1, nOriRawTBPad, m_modCali.m_stOriHPadCache);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: SetStripeTBpad failed");

	/* 得到输出的OriRaw数据 */
	XMat matOriL = m_modCali.CacheTBpad(m_matOriLTemp, m_modCali.m_stOriLPadCache);
	XSP_CHECK((matOriL.tpad == nOriRawTBPad.first) && (matOriL.bpad == nOriRawTBPad.second), 
              XRAY_LIB_XMAT_SIZE_ERR, "Pipe3A idx0: CacheTBpad m_stOriLPadCache err.");

	XMat matOriH = m_modCali.CacheTBpad(m_matOriHTemp, m_modCali.m_stOriHPadCache);
	XSP_CHECK((matOriH.tpad == nOriRawTBPad.first) && (matOriH.bpad == nOriRawTBPad.second), 
              XRAY_LIB_XMAT_SIZE_ERR, "Pipe3A idx0: CacheTBpad m_stOriHPadCache err.");

	/* 裁剪OriRaw的上下扩边都为nIdx0HeiIn */
	matOriL.Reshape(3 * nIdx0HeiIn, nIdx0WidIn, nIdx0HeiIn, XSP_16UC1);
	matOriH.Reshape(3 * nIdx0HeiIn, nIdx0WidIn, nIdx0HeiIn, XSP_16UC1);

	/************************************************************************
	*                    	out_calilhz_ori数据整理
	************************************************************************/
	XMat matZValueOri;
	hr = matZValueOri.Init(matOriL.hei, matOriL.wid, matOriL.tpad, matOriL.bpad, XSP_16UC1, (uint8_t*)idx0.out_calilhz_ori.pData[2]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: Get out_calilhz_ori zvalue data failed");

	hr = m_modDual.ZIdentify(matOriH, matOriL, matZValueOri);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: ZIdentify err");

	memcpy((uint8_t*)idx0.out_calilhz_ori.pData[0], matOriL.Ptr(), matOriL.hei * matOriL.wid * XSP_ELEM_SIZE(matOriL.type));
	memcpy((uint8_t*)idx0.out_calilhz_ori.pData[1], matOriH.Ptr(), matOriH.hei * matOriH.wid * XSP_ELEM_SIZE(matOriH.type));
	memcpy((uint8_t*)idx0.out_calilhz_ori.pData[2], matZValueOri.Ptr(), matZValueOri.hei * matZValueOri.wid * XSP_ELEM_SIZE(matZValueOri.type));

	idx0.out_calilhz_ori.stride[0] = matOriL.wid;
	idx0.out_calilhz_ori.stride[1] = matOriH.wid;
	idx0.out_calilhz_ori.stride[2] = matZValueOri.wid;
	idx0.out_calilhz_ori.height = matOriL.hei;
	idx0.out_calilhz_ori.width = matOriL.wid;


	/************************************************************************
	*                    	cali数据缓存处理
	************************************************************************/
	hr = m_modCali.SetTBpad(nIdx0WidIn, nIdx0HeiIn, XSP_16UC1, nOriRawTBPad, m_modCali.m_stCaliLPadCache);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: SetStripeTBpad failed");

	hr = m_modCali.SetTBpad(nIdx0WidIn, nIdx0HeiIn, XSP_16UC1, nOriRawTBPad, m_modCali.m_stCaliHPadCache);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: SetStripeTBpad failed");

	XMat matCaliL = m_modCali.CacheTBpad(m_matLTemp, m_modCali.m_stCaliLPadCache);
	XSP_CHECK((matCaliL.tpad == nOriRawTBPad.first) && (matCaliL.bpad == nOriRawTBPad.second), 
              XRAY_LIB_XMAT_SIZE_ERR, "Pipe3A idx0: CacheTBpad m_stCaliLPadCache err.");

	XMat matCaliH = m_modCali.CacheTBpad(m_matHTemp, m_modCali.m_stCaliHPadCache);
	XSP_CHECK((matCaliH.tpad == nOriRawTBPad.first) && (matCaliH.bpad == nOriRawTBPad.second), 
              XRAY_LIB_XMAT_SIZE_ERR, "Pipe3A idx0: CacheTBpad m_stCaliLPadCache err.");

    /* 条带标志入列，给应用透传 */
	m_queFlag.push_back(idx0.in_flag);

    /// 对最新条带（即in_flag对应的条带）计算包裹区域，imgLe.pwin()和fgMask.pwin()对应的是最新条带区域
    isl::Imat<uint16_t> imgLe(nIdx0HeiIn * 2, nIdx0WidIn, 1, matCaliL.Ptr<uint16_t>(matCaliL.hei - nIdx0HeiIn * 2), 
                              nIdx0WidIn * nIdx0HeiIn * 2 * XSP_ELEM_SIZE(matCaliL.type), isl::pads_t{nIdx0HeiIn, 0});
    isl::Imat<uint8_t> fgMask(imgLe, m_matLTemp.data.ptr, m_matLTemp.size); // fgMask.pwin()的上邻域条带区也会有输出mask，m_matLTemp复用为FbgCull()的结果内存
    isl::Imat<uint16_t> imgTmpFbg(imgLe, m_matHTemp.data.us, m_matHTemp.size); // m_matHTemp作为FbgCull()的临时内存
    std::vector<isl::Fbg::Fg4Slice> fgResults = m_modImgproc.m_islFbg.FbgCull(fgMask, imgLe, imgTmpFbg, quePack.empty());
    if (!fgResults.empty())
    {
        auto _win_merge = [](auto ia, auto ib) -> XRAYLIB_RECT
        {
            isl::Rect<int32_t> win(*ia);
            auto it = ia;
            while (it != ib)
            {
                win.unite(*(++it));
            }
            XRAYLIB_RECT rect{static_cast<uint32_t>(win.x), static_cast<uint32_t>(win.y), 
                static_cast<uint32_t>(win.width), static_cast<uint32_t>(win.height)};
            return rect;
        };
        for (auto it = fgResults.rbegin(); it != fgResults.rend(); ++it) // TODO: 需输出(缓存条带数+1)个，这里等缓存逻辑更新后需补充
        {
            XRAYLIB_RTPROC_PACKAGE& packInfo = (it == fgResults.rbegin()) ? quePack.emplace_back() : 
                quePack.at(quePack.size() - 1 - std::distance(fgResults.rbegin(), it));
            auto iLast = std::prev(it->bboxes.end());
            if (isl::Fbg::morph_none == it->status)
            {
                packInfo.nPackNum = 0;
            }
            else if (isl::Fbg::morph_stend == it->status)
            {
                packInfo.nPackNum = 2;
                packInfo.stPackPos[1] = _win_merge(iLast, iLast); // 取最后一个区域为新包裹的start
                packInfo.stPackPos[0] = _win_merge(it->bboxes.begin(), std::prev(iLast)); // 其他所有区域合并为旧包裹的end
            }
            else
            {
                packInfo.nPackNum = 1;
                packInfo.stPackPos[0] = _win_merge(it->bboxes.begin(), iLast); // 合并所有区域
            }
        }
        /// 更新上一条带的包裹区域
        if (fgResults.size() > 1)
        {
            m_matLTemp.Reshape(fgMask.get_vpads().first, fgMask.width(), XSP_8UC1);
            hr = m_modCali.ModTpadBuf(m_matLTemp, m_modCali.m_stMaskCache);
            XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: ModTpadBuf m_stMaskCache failed");
        }
    }
    else
    {
        log_warn("fbg cull failed, in_flag %llu", idx0.in_flag);
        XRAYLIB_RTPROC_PACKAGE packInfo = {1, {0, 0, static_cast<uint32_t>(nIdx0WidIn), static_cast<uint32_t>(nIdx0HeiIn)}};
        quePack.push_back(packInfo); // 放一个空的对象进去，保持in和out的数量同步
        fgMask.fill(isl::Fbg::fgVal, fgMask.pwin()); // 异常时，该条带视为全包裹区域
    }

    /* 将最新条带的包裹区域写入缓存 */
    m_matLTemp.Reshape(fgMask.height(), fgMask.width(), fgMask.get_vpads().first, 0, XSP_8UC1);
	XMat matMaskOri = m_modCali.CacheTBpad(m_matLTemp, m_modCali.m_stMaskCache);
	XSP_CHECK((matMaskOri.tpad == nOriRawTBPad.first) && (matMaskOri.bpad == nOriRawTBPad.second), 
              XRAY_LIB_XMAT_SIZE_ERR, "Pipe3A idx0: CacheTBpad m_stMaskCache err.");

	/************************************************************************
	*            对归一化、校正数据进行AIXSP（若有）、扩边处理
	************************************************************************/
	Calilhz calilhzAiIn, calilhzAiOut;
	int32_t nAiXspNB = m_modCnn.AiXsp_GetNbLen();
	auto fAixspScale = m_modCnn.AiXsp_GetScale();
	bool bUseSr = (fAixspScale.first > 1.0f) || (fAixspScale.second > 1.0f) ? true : false;

	// 注意:这里送入AIXSP的数据需要的是当前采传输入的最新条带数据，并不是当前需要被处理的条带, 并且需要上部有2领边
	XMat matAixspLIn, matAixspHIn;
	uint8_t *pAixspLIn = m_modCali.m_stCaliLPadCache.mRingBuf.Ptr(m_modCali.m_stCaliLPadCache.nWriteIndex - nIdx0HeiIn - 2 * nAiXspNB);
	uint8_t *pAixspHIn = m_modCali.m_stCaliHPadCache.mRingBuf.Ptr(m_modCali.m_stCaliHPadCache.nWriteIndex - nIdx0HeiIn - 2 * nAiXspNB);

	matAixspLIn.Init(nIdx0HeiIn + 2 * nAiXspNB, nIdx0WidIn, matCaliL.type, pAixspLIn);
	matAixspHIn.Init(nIdx0HeiIn + 2 * nAiXspNB, nIdx0WidIn, matCaliH.type, pAixspHIn);
	
	/* 送入AIXSP预处理，主要是数据对齐、扩边等 */ 
	hr = m_modCnn.AiXsp_PreProcess(matAixspLIn, matAixspHIn, calilhzAiIn, calilhzAiOut);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: AI_XSP_PreProcess err.");

	// TODO：100100的高速模式下两倍缩放使用AIXSP耗时过长，所以直接使用lanzcos缩放，不使用超分模型，后面需要删除
	if((nIdx0WidIn >= 960) && (nIdx0HeiIn >= 32) && bUseSr)
	{
		// lanzos缩放
		hr = m_mPreLanczosTemp.Reshape(MAX(matAixspLIn.hei, calilhzAiOut.low.hei), MAX(matAixspLIn.wid, calilhzAiOut.low.wid), XSP_16UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx1: Get m_matLanzosResizeTempPre data failed");

		hr = m_matResizeLow.Reshape(calilhzAiOut.low.hei, calilhzAiOut.low.wid, XSP_16UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "m_matResizeLow init failed.");
		hr = m_matResizeHigh.Reshape(calilhzAiOut.low.hei, calilhzAiOut.low.wid, XSP_16UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "m_matResizeHigh init failed.");

		isl::Imat<uint16_t> ImatCaliLIn(matAixspLIn);
		isl::Imat<uint16_t> ImatCaliHIn(matAixspHIn);
		isl::Imat<uint16_t> ImatTmp(m_mPreLanczosTemp);

		isl::Imat<uint16_t> ImatCaliLOut(m_matResizeLow);
		isl::Imat<uint16_t> ImatCaliHOut(m_matResizeHigh);

		XSP_CHECK(0 == isl::imgAnyScale(ImatCaliLOut, ImatCaliLIn, &ImatTmp, &intpSrc2CnnHor, &intpSrc2CnnVer), XRAY_LIB_INVALID_PARAM);
		XSP_CHECK(0 == isl::imgAnyScale(ImatCaliHOut, ImatCaliHIn, &ImatTmp, &intpSrc2CnnHor, &intpSrc2CnnVer), XRAY_LIB_INVALID_PARAM);
	}
	else
	{
		hr = m_modCnn.AiXsp_SyncProcess(calilhzAiIn, calilhzAiOut);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: AI_XSP_AcceProcess err.");

		// lanczos缩放
		hr = m_mPreLanczosTemp.Reshape(MAX(matAixspLIn.hei, calilhzAiOut.low.hei), MAX(matAixspLIn.wid, calilhzAiOut.low.wid), XSP_16UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx1: Get m_matLanzosResizeTempPre data failed");

		hr = m_matResizeLow.Reshape(calilhzAiOut.low.hei, calilhzAiOut.low.wid, XSP_32FC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "m_matResizeLow init failed.");
		hr = m_matResizeHigh.Reshape(calilhzAiOut.low.hei, calilhzAiOut.low.wid, XSP_32FC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "m_matResizeHigh init failed.");

		isl::Imat<uint16_t> ImatCaliLIn(matAixspLIn);
		isl::Imat<uint16_t> ImatCaliHIn(matAixspHIn);
		isl::Imat<uint16_t> ImatTmp(m_mPreLanczosTemp);

		isl::Imat<float32_t> ImatCaliLOut(m_matResizeLow);
		isl::Imat<float32_t> ImatCaliHOut(m_matResizeHigh);

		std::function<float32_t(const uint16_t)> _u16_2_f32 = [](const uint16_t src) -> float32_t {return static_cast<float32_t>(src) / 65535.0f;};
		XSP_CHECK(0 == isl::imgAnyScale(ImatCaliLOut, ImatCaliLIn, &ImatTmp, &intpSrc2CnnHor, &intpSrc2CnnVer, _u16_2_f32), XRAY_LIB_INVALID_PARAM);
		XSP_CHECK(0 == isl::imgAnyScale(ImatCaliHOut, ImatCaliHIn, &ImatTmp, &intpSrc2CnnHor, &intpSrc2CnnVer, _u16_2_f32), XRAY_LIB_INVALID_PARAM);

		hr = MergeDualAiOriFp32ByMask(calilhzAiOut.high, calilhzAiOut.low, m_matResizeHigh, m_matResizeLow, calilhzAiOut.zData);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: MergeDualAiOriFp32ByMask err.");

		/* 32FC1转为16UC1 */
		hr = m_matResizeLow.Reshape(calilhzAiOut.low.hei, calilhzAiOut.low.wid, XSP_16UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "m_matResizeLow init failed.");

		hr = m_matResizeHigh.Reshape(calilhzAiOut.low.hei, calilhzAiOut.low.wid, XSP_16UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "m_matResizeHigh init failed.");

		ImgConvertType(calilhzAiOut.low.Ptr<float32_t>(), m_matResizeLow.Ptr<uint16_t>(), calilhzAiOut.low.hei * calilhzAiOut.low.wid, 65535.0f, 0.0f, 65535.0f);
		ImgConvertType(calilhzAiOut.high.Ptr<float32_t>(), m_matResizeHigh.Ptr<uint16_t>(), calilhzAiOut.low.hei * calilhzAiOut.low.wid, 65535.0f, 0.0f, 65535.0f);
	}

	/************************************************************************
	*    将条带扩边缓存数据中未被AIXSP正常处理的数据进行替换,并缓存当前条带数据
	************************************************************************/
	XMat mCaliL2Cache, mCaliH2Cache, mReplaceL, mReplaceH, mCaliL, mCaliH;

	// 扩边条带缓存中需要被替换的条带尺寸
	int32_t nProcHei = fAixspScale.first * nAiXspNB;
	int32_t nProcWid = fAixspScale.second * nIdx0WidIn;

	mReplaceL.Init(nProcHei, nProcWid, m_matResizeLow.type, m_matResizeLow.Ptr(nProcHei));
	mReplaceH.Init(nProcHei, nProcWid, m_matResizeHigh.type, m_matResizeHigh.Ptr(nProcHei));

	/* 修改扩边缓冲池内存 */ 
	hr = m_modCali.ModTpadBuf(mReplaceL, m_modCali.m_stLPadCache);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: ModTpadBuf failed");

	hr = m_modCali.ModTpadBuf(mReplaceH, m_modCali.m_stHPadCache);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: ModTpadBuf failed");

	/* 裁剪掉当前条带的多余数据 */
	mCaliL2Cache.Init(m_matResizeLow.hei - 2 * nProcHei, m_matResizeLow.wid, m_matResizeLow.type, m_matResizeLow.Ptr(2 * nProcHei));
	mCaliH2Cache.Init(m_matResizeHigh.hei - 2 * nProcHei, m_matResizeHigh.wid, m_matResizeHigh.type, m_matResizeHigh.Ptr(2 * nProcHei));

	/* 设置经过校正、归一化、AIXSP处理后的条带扩边高度 */
	std::pair<int32_t, int32_t> nAixspTBPad(mCaliL2Cache.hei, nPadCacheNum * mCaliL2Cache.hei);

    hr = m_modCali.SetTBpad(mCaliL2Cache.wid, mCaliL2Cache.hei, mCaliL2Cache.type, nAixspTBPad, m_modCali.m_stLPadCache);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: SetTBpad failed");

	hr = m_modCali.SetTBpad(mCaliH2Cache.wid, mCaliH2Cache.hei, mCaliH2Cache.type, nAixspTBPad, m_modCali.m_stHPadCache);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: SetTBpad failed");

	bool bCachingDone = false;
	mCaliL = m_modCali.CacheTBpad(mCaliL2Cache, m_modCali.m_stLPadCache, &bCachingDone);
	XSP_CHECK((mCaliL.tpad == nAixspTBPad.first) && (mCaliL.bpad == nAixspTBPad.second), 
              XRAY_LIB_XMAT_SIZE_ERR, "Pipe3A idx0: ModStripeTBpadCache err.");

	mCaliH = m_modCali.CacheTBpad(mCaliH2Cache, m_modCali.m_stHPadCache);
	XSP_CHECK((mCaliH.tpad == nAixspTBPad.first) && (mCaliH.bpad == nAixspTBPad.second), 
              XRAY_LIB_XMAT_SIZE_ERR, "Pipe3A idx0: ModStripeTBpadCache err.");

    // 包裹分割信息
    isl::Imat<uint8_t> imgMaskOri(matMaskOri.hei, matMaskOri.wid, 1, matMaskOri.data.ptr, matMaskOri.size);
    isl::Imat<uint8_t> imgMaskRsz(m_nResizeHeight, m_nResizeWidth, 1, (uint8_t *)idx0.out_mask.pData[0], idx0.out_mask.height * idx0.out_mask.stride[0]);
    isl::Imat<uint8_t> imgTmpRsz(imgMaskRsz, m_mPreLanczosTemp.data.ptr, m_mPreLanczosTemp.size);
    XSP_CHECK(0 == isl::imgAnyScale(imgMaskRsz, imgMaskOri, &imgTmpRsz, isl::intp_method_t::nearest), 
              XRAY_LIB_INVALID_PARAM, "scale mask failed");

    int32_t out_num = std::min(quePack.size(), sizeof(idx0.out_pack)/sizeof(idx0.out_pack[0]));
    const float64_t maskScaleHor = static_cast<float64_t>(m_nResizeWidth) / matMaskOri.wid;
    const float64_t maskScaleVer = static_cast<float64_t>(m_nResizeHeight) / matMaskOri.hei;
    for (int32_t i = 0; i < out_num; ++i)
    {
        XRAYLIB_RTPROC_PACKAGE& packInfo = quePack.at(i);
        idx0.out_pack[i].nPackNum = packInfo.nPackNum;
        for (uint32_t j = 0; j < packInfo.nPackNum; ++j)
        {
            idx0.out_pack[i].stPackPos[j].x = std::floor(maskScaleHor * packInfo.stPackPos[j].x);
            idx0.out_pack[i].stPackPos[j].y = std::floor(maskScaleVer * packInfo.stPackPos[j].y);
            idx0.out_pack[i].stPackPos[j].width = std::ceil(maskScaleHor * packInfo.stPackPos[j].width);
            idx0.out_pack[i].stPackPos[j].height = std::ceil(maskScaleVer * packInfo.stPackPos[j].height);
        }
    }

    /// 正在缓存时，直接返回XRAY_LIB_OK，不返回XRAY_LIB_RTCACHING是为了兼容之前版本
    if (!bCachingDone)
	{
		idx0.out_calilhz.height = 0;
		idx0.out_calilhz.width = 0;
		idx0.out_calilhz_ori.height = 0;
		idx0.out_calilhz_ori.width = 0;
		return XRAY_LIB_OK;
	}

	/************************************************************************
	*                    	out_calilhz数据整理
	************************************************************************/
	memcpy((uint8_t*)idx0.out_calilhz.pData[0], mCaliL.Ptr(), mCaliL.hei * mCaliL.wid * XSP_ELEM_SIZE(mCaliL.type));
	memcpy((uint8_t*)idx0.out_calilhz.pData[1], mCaliH.Ptr(), mCaliH.hei * mCaliH.wid * XSP_ELEM_SIZE(mCaliH.type));

	idx0.out_calilhz.stride[0] = mCaliL.wid;
	idx0.out_calilhz.stride[1] = mCaliH.wid;
	idx0.out_calilhz.height = mCaliL.hei;
	idx0.out_calilhz.width = mCaliL.wid;


    /* 取队列中头条带标志给应用透传，然后出列 */
	idx0.out_flag = m_queFlag.front();
	m_queFlag.pop_front();
    quePack.pop_front();
	m_AllowSyncOperation.store(true);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::PipeDual3A_CnnProcess(XRAYLIB_PIPELINE_PARAM_MODE3A::_pipeline_3a_idx1_& idx1)
{
	XSP_CHECK(XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode, XRAY_LIB_ENERGYMODE_ERROR);
	XRAY_LIB_HRESULT hr;

	/*******************************************
	*             输入数据整理
	*******************************************/
	XMat matCaliL, matCaliH;
	int32_t nIdx1InWid = idx1.in_calilhz.width;
	int32_t nIdx1InHei = idx1.in_calilhz.height;
	std::pair<int32_t, int32_t>  nIdx0TBPad(m_modCali.m_stLPadCache.nTpadHei, m_modCali.m_stLPadCache.nBpadHei);

	hr = matCaliL.Init(nIdx1InHei, nIdx1InWid, nIdx0TBPad.first, nIdx0TBPad.second, XSP_16UC1, (uint8_t*)idx1.in_calilhz.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx1: Get in_calilhz matCaliL data failed");

	hr = matCaliH.Init(nIdx1InHei, nIdx1InWid, nIdx0TBPad.first, nIdx0TBPad.second, XSP_16UC1, (uint8_t*)idx1.in_calilhz.pData[1]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx1: Get in_calilhz matCaliH data failed");

	/****************************************************************
		灰度图像特殊处理：
			1、测试体增强处理，这里需要适配不同缩放比例的模型
	***************************************************************/
	XMat matSpProcL = matCaliL;
	XMat matSpProcH = matCaliH;

	int32_t nTcStripeHei = nIdx1InHei - nIdx0TBPad.first - nIdx0TBPad.second; // 去扩边的条带高度

	if (XRAYLIB_TESTMODE_CN_CHECK == m_modImgproc.m_enTestMode)
	{
		/* 增强参数传入 */
		m_modTcproc.IslTcEnhance(matCaliL, matCaliH, nIdx1InWid, nTcStripeHei + matCaliL.bpad, &m_modImgproc.m_nWindowCenter);

		/* 如果检测到TESTB，则记录当前条带序列号+缓存条带号作为R曲线微调的开始，在颜色微调6个TEST4宽度的区域之后结束微调 */
		if (m_modTcproc.m_vecSliceSeq.front() == (uint)m_modTcproc.m_nTestBStartIdx)  
		{
			m_modDual.AdjustZTable(m_modDual.m_stTestbParams.stRCurveAdjust);
		}

		if(m_modTcproc.m_vecSliceSeq.front() == m_modTcproc.m_nTestBStartIdx + 6 * m_modTcproc.m_vecSliceSeq.size())
		{
			m_modDual.AdjustZTable({50,50,50,50,50,50});
		}

		hr = matSpProcL.Init(nTcStripeHei + 2 * nTcStripeHei, nIdx1InWid, XSP_16UC1, matCaliL.Ptr(nIdx0TBPad.first - nTcStripeHei));
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: Get low roi data failed");

		hr = matSpProcH.Init(nTcStripeHei + 2 * nTcStripeHei, nIdx1InWid, XSP_16UC1, matCaliH.Ptr(nIdx0TBPad.first - nTcStripeHei));
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx0: Get High roi data failed");

	}

	/**********************************************
	*        out_calilhz数据lanczos缩放处理
	**********************************************/
	XMat matCaliLOut, matCaliHOut;
	hr = matCaliLOut.Init(m_nResizeHeight, m_nResizeWidth, XSP_16UC1, (uint8_t*)idx1.out_calilhz.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx1: Get matCaliLOut data failed");

	hr = matCaliHOut.Init(m_nResizeHeight, m_nResizeWidth, XSP_16UC1, (uint8_t*)idx1.out_calilhz.pData[1]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx1: Get matCaliHOut data failed");

	hr = m_mCnnLanczosTemp.Reshape(MAX(m_nResizeHeight,matSpProcL.hei), MAX(m_nResizeWidth,nIdx1InWid),XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx1: Get m_mCnnLanczosTemp data failed");

	matSpProcL.Reshape(matSpProcL.hei, matSpProcL.wid, matSpProcL.type);
	matSpProcH.Reshape(matSpProcH.hei, matSpProcH.wid, matSpProcH.type);

	isl::Imat<uint16_t> ImatCaliLIn(matSpProcL);
	isl::Imat<uint16_t> ImatCaliHIn(matSpProcH);
	isl::Imat<uint16_t> ImatTmp(m_mCnnLanczosTemp);

	isl::Imat<uint16_t> ImatCaliLOut(matCaliLOut);
	isl::Imat<uint16_t> ImatCaliHOut(matCaliHOut);

	XSP_CHECK(0 == isl::imgAnyScale(ImatCaliLOut, ImatCaliLIn, &ImatTmp, &intpCnn2DstHor, &intpCnn2DstVer), XRAY_LIB_INVALID_PARAM);
	XSP_CHECK(0 == isl::imgAnyScale(ImatCaliHOut, ImatCaliHIn, &ImatTmp, &intpCnn2DstHor, &intpCnn2DstVer), XRAY_LIB_INVALID_PARAM);

	idx1.out_calilhz.height = matCaliLOut.hei;
    idx1.out_calilhz.width = matCaliLOut.wid;
    idx1.out_calilhz.stride[0] = matCaliLOut.wid;
    idx1.out_calilhz.stride[1] = matCaliLOut.wid;
    idx1.out_calilhz.stride[2] = matCaliLOut.wid;

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::PipeDual3A_PostProcess(XRAYLIB_PIPELINE_PARAM_MODE3A::_pipeline_3a_idx2_& idx2)
{
	XSP_CHECK(XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode, XRAY_LIB_ENERGYMODE_ERROR);
	XRAY_LIB_HRESULT hr;

	/************************************************************************
	*                               Ai路处理
	************************************************************************/
	XMat matCaliHOri, matCaliLOri, matZValueOri;
	int32_t nIdx2InOriWid = idx2.in_calilhz_ori.width;
	int32_t nIdx2InOriHei = idx2.in_calilhz_ori.height;
	int32_t nIdx2InOriPad = nIdx2InOriHei / 3;

	/* 数据转换 */
	hr = matCaliLOri.Init(nIdx2InOriHei, nIdx2InOriWid, nIdx2InOriPad, XSP_16UC1, (uint8_t*)idx2.in_calilhz_ori.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: Get in_calilhz low data failed.");

	hr = matCaliHOri.Init(nIdx2InOriHei, nIdx2InOriWid, nIdx2InOriPad, XSP_16UC1, (uint8_t*)idx2.in_calilhz_ori.pData[1]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: Get in_calilhz high data failed.");

	hr = matZValueOri.Init(nIdx2InOriHei, nIdx2InOriWid, nIdx2InOriPad, XSP_16UC1, (uint8_t*)idx2.in_calilhz_ori.pData[2]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: Get in_calilhz zdata failed.");

	/* 原子序数16转8位，用于复用Ai路上色 */
	hr = m_matResizeZValue.Reshape(nIdx2InOriHei, nIdx2InOriWid, nIdx2InOriPad, XSP_8UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: zdata temp data reshape err.");

	ZU16ToU8(matZValueOri, m_matResizeZValue);

	/* 图像融合 */
	hr = m_matMerge.Reshape(nIdx2InOriHei, nIdx2InOriWid, nIdx2InOriPad, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: merge temp data reshape err.");

	hr = m_modImgproc.ImgMergingAI(matCaliHOri, matCaliLOri, m_matMerge);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: ImgMergingAI err.");

	/* Ai路伪彩上色 */
	hr = GetAiColorImage(m_matMerge, m_matResizeZValue, idx2.out_ai, nIdx2InOriHei - 2 * nIdx2InOriPad, idx2.out_ai.stride[0], nIdx2InOriWid);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: GetAiColorImage err.");

	/* out_calilhz_ori 返回 */
	int32_t nCalilhzOriHei = nIdx2InOriHei - 2 * nIdx2InOriPad;
	idx2.out_calilhz_ori.height = nCalilhzOriHei;
	idx2.out_calilhz_ori.width = nIdx2InOriWid;
	idx2.out_calilhz_ori.stride[0] = nIdx2InOriWid;
	idx2.out_calilhz_ori.stride[1] = nIdx2InOriWid;
	idx2.out_calilhz_ori.stride[2] = nIdx2InOriWid;

	if (idx2.out_calilhz_ori.pData[0] && idx2.out_calilhz_ori.pData[1] && idx2.out_calilhz_ori.pData[2])
	{
		memcpy(idx2.out_calilhz_ori.pData[0], matCaliLOri.PadPtr(), nIdx2InOriWid * nCalilhzOriHei * XSP_ELEM_SIZE(XSP_16UC1));
		memcpy(idx2.out_calilhz_ori.pData[1], matCaliHOri.PadPtr(), nIdx2InOriWid * nCalilhzOriHei * XSP_ELEM_SIZE(XSP_16UC1));
		memcpy(idx2.out_calilhz_ori.pData[2], matZValueOri.PadPtr(), nIdx2InOriWid * nCalilhzOriHei * XSP_ELEM_SIZE(XSP_16UC1));
	}

	/************************************************************************
	*                               Disp路处理
	************************************************************************/
	XSP_CHECK(idx2.in_calilhz.width == static_cast<uint32_t>(m_nResizeWidth), 
              XRAY_LIB_PARAM_WIDTH_ERROR, "Pipe3A idx2: idx2.in_calilhz.width error");
	XSP_CHECK(idx2.in_calilhz.height == static_cast<uint32_t>(m_nResizeHeight), 
              XRAY_LIB_PARAM_WIDTH_ERROR, "Pipe3A idx2: idx2.in_calilhz.height error");

	XMat matHigh, matLow, matZ, matMask;

	hr = matLow.Init(m_nResizeHeight, m_nResizeWidth, m_nResizePadLen, XSP_16UC1, (uint8_t*)idx2.in_calilhz.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: Get in_calilhz low data failed.");

	hr = matHigh.Init(m_nResizeHeight, m_nResizeWidth, m_nResizePadLen, XSP_16UC1, (uint8_t*)idx2.in_calilhz.pData[1]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: Get in_calilhz high data failed.");

	hr = matMask.Init(m_nResizeHeight, m_nResizeWidth, m_nResizePadLen, XSP_8UC1, (uint8_t*)idx2.in_mask.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: Get in_mask data failed.");

	hr = matZ.Init(m_nResizeHeight, m_nResizeWidth, m_nResizePadLen, XSP_8UC1, (uint8_t*)idx2.in_calilhz.pData[2]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: Get in_calilhz zdata failed.");

	hr = m_matProcWt.Reshape(m_nResizeHeight, m_nResizeWidth, m_nResizePadLen, XSP_8UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: m_matProcWt reshape err.");

	/* 原子序数计算,使用OriRaw数据计算 */
	matZ.Reshape(nIdx2InOriHei, nIdx2InOriWid, nIdx2InOriPad, XSP_8UC1);

	if (XRAYLIB_ON == m_modCali.m_enGeoMertric && m_modCali.m_bGeoCaliCanUse)
	{
		hr = m_matOriGeomLow.Reshape(nIdx2InOriHei, nIdx2InOriWid, nIdx2InOriPad, XSP_16UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: low geom temp data reshape err.");

		hr = m_matOriGeoHigh.Reshape(nIdx2InOriHei, nIdx2InOriWid, nIdx2InOriPad, XSP_16UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: high geom temp data reshape err.");

		hr = m_modCali.ExecuteGeoCali(matCaliLOri, m_matOriGeomLow);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "GeoCali is err.");

		hr = m_modCali.ExecuteGeoCali(matCaliHOri, m_matOriGeoHigh);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "GeoCali is err.");

		hr = m_modDual.ZIdentify(m_matOriGeoHigh, m_matOriGeomLow, matZ);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: ZIdentifySR err.");
	}
	else
	{
		hr = m_modDual.ZIdentify(matCaliHOri, matCaliLOri, matZ);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: ZIdentifySR err.");
	}

	/* 对Z值进行缩放处理 */
	int32_t nZValueMinNb = 2; // 原子序数图所需要的最小邻边为2
	int32_t nResizeZValueMinNb = static_cast<int32_t>(2 * m_fHeightResizeFactor);

	m_matResizeZValue.Reshape(m_nResizeHeight, m_nResizeWidth, m_nResizePadLen - nResizeZValueMinNb, XSP_8UC1);
	m_mPostLanczosTemp.Reshape(MAX(matZ.hei, m_matResizeZValue.hei), MAX(matZ.wid, m_matResizeZValue.wid), m_nResizePadLen - nResizeZValueMinNb, XSP_8UC1);
	matZ.Reshape(matZ.hei, matZ.wid, matZ.tpad - nZValueMinNb, XSP_8UC1);

	isl::Imat<uint8_t> ImatZValueIn(matZ);
	isl::Imat<uint8_t> ImatZValueOut(m_matResizeZValue);
	isl::Imat<uint8_t> ImatTmp(m_mPostLanczosTemp);
	std::function<uint8_t(const uint8_t)> Zclamp = [](const uint8_t src) -> uint8_t {return std::clamp(static_cast<int32_t>(src), 0, 41);};
	XSP_CHECK(0 == isl::imgAnyScale(ImatZValueOut, ImatZValueIn, &ImatTmp, &intpZSrc2DstHor, &intpZSrc2DstVer, Zclamp), XRAY_LIB_INVALID_PARAM);

	matZ.Reshape(m_nResizeHeight, m_nResizeWidth, m_nResizePadLen, XSP_8UC1);
	memcpy(matZ.Ptr(m_nResizePadLen - nResizeZValueMinNb), m_matResizeZValue.PadPtr(), m_nResizeWidth * (m_nResizeHeight + 2 * nResizeZValueMinNb) * XSP_ELEM_SIZE(XSP_8UC1));

	/* 输出out_calilhz */
	int32_t nCalilhzWid = m_nResizeWidth;
	int32_t nCalilhzHei = m_nResizeHeight - m_nResizePadLen * 2;
	idx2.out_calilhz.height = nCalilhzHei;
	idx2.out_calilhz.width = nCalilhzWid;
	idx2.out_calilhz.stride[0] = nCalilhzWid;
	idx2.out_calilhz.stride[1] = nCalilhzWid;
	idx2.out_calilhz.stride[2] = nCalilhzWid;
	memcpy(idx2.out_calilhz.pData[0], matLow.PadPtr(), nCalilhzWid * nCalilhzHei * XSP_ELEM_SIZE(XSP_16UC1));
	memcpy(idx2.out_calilhz.pData[1], matHigh.PadPtr(), nCalilhzWid * nCalilhzHei * XSP_ELEM_SIZE(XSP_16UC1));
    for (int32_t i = 0; i < nCalilhzHei; ++i)
    {
        uint8_t* pSrcZ = matZ.Ptr(matZ.tpad + i); // 取第0~6Bit
        uint8_t* pSrcM = matMask.Ptr(matMask.tpad + i); // 取第7Bit
        uint8_t* pDst = (uint8_t*)idx2.out_calilhz.pData[2] + i * nCalilhzWid;
        for (int32_t j = 0; j < nCalilhzWid; ++j, ++pSrcZ, ++pSrcM, ++pDst)
        {
            *pDst = (*pSrcZ & 0x7F) | (*pSrcM & 0x80);
        }
    }

    idx2.out_merge.height = nCalilhzHei;
    idx2.out_merge.width = nCalilhzWid;
    idx2.out_merge.stride[0] = nCalilhzWid;
    memcpy(&idx2.packpos, &idx2.in_pack[0], sizeof(XRAYLIB_RTPROC_PACKAGE)); // 输出包裹分割结果

    if (procWtCache.bClearTemp)
    {
        queProcWt.clear();
    }
    std::pair<int32_t, int32_t> tbPads{nCalilhzHei, nCalilhzHei};
	hr = CaliModule::SetTBpad(nCalilhzWid, nCalilhzHei, XSP_8UC1, tbPads, procWtCache);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: SetTBpad for procWtCache failed");

	/********************************************
	*                 TIP插入
	********************************************/
	if (1 == m_nNeedTip)
	{
		hr = RealTimeTipImage_CA(matHigh, matLow, matZ, matMask, matLow, idx2.TipStatus, idx2.TipParam.nTipedPosW, &idx2.packpos);
		if (hr != XRAY_LIB_OK)	/// 判断是否插入错误，如果错误后续不再进行插入
		{
			m_nNeedTip = 0;
			m_nTipHeight = 0;
			m_nTipWidth = 0;
			m_nTipedHeight = 0;
			m_nTipWidthStart = -1;
			m_nTipStripeStart = -1;
			m_nPackageHeight = 0;
			m_nPackageHeightIn = 0;
		}
	}

	isl::Imat<uint8_t> imgMask(matMask);
    isl::Imat<uint8_t> imgWt(m_matProcWt);
    const int32_t smoothRadius = 3;
    std::vector<isl::Rect<int32_t>> packArea; // 带平滑的包裹区域（实际包裹区域往外扩展smoothRadius）
    for (uint32_t i = 0; i < idx2.in_pack[0].nPackNum; ++i)
    {
        packArea.push_back(isl::Rect<int32_t>(idx2.in_pack[0].stPackPos[i].x, idx2.in_pack[0].stPackPos[i].y + m_nResizePadLen, 
                                              idx2.in_pack[0].stPackPos[i].width, idx2.in_pack[0].stPackPos[i].height));
    }
    packArea = isl::Fbg::SmoothWeight(imgWt, imgMask, smoothRadius, packArea);
    hr = CaliModule::ModTpadBuf(m_matProcWt, procWtCache);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: ModTpadBuf procWtCache failed");
    if (!queProcWt.empty()) // 更新列队中最后一个元素（即上一次处理的下邻域条带）
    {
        queProcWt.pop_back();
    }
    queProcWt.push_back(packArea);

    imgMask.move_vpads({nCalilhzHei, -nCalilhzHei});
    imgWt.move_vpads({nCalilhzHei, -nCalilhzHei});
    std::vector<isl::Rect<int32_t>> packNext;
    for (uint32_t i = 0; i < idx2.in_pack[1].nPackNum; ++i)
    {
        packNext.push_back(isl::Rect<int32_t>(idx2.in_pack[1].stPackPos[i].x, idx2.in_pack[1].stPackPos[i].y+m_nResizePadLen+nCalilhzHei, 
                                              idx2.in_pack[1].stPackPos[i].width, idx2.in_pack[1].stPackPos[i].height));
    }
    packNext = isl::Fbg::SmoothWeight(imgWt, imgMask, smoothRadius, packNext);
    hr = m_matProcWt.Reshape(m_nResizeHeight, m_nResizeWidth, m_nResizePadLen+nCalilhzHei, 0, XSP_8UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: m_matProcWt reshape err.");
	XMat&& matProcWt = CaliModule::CacheTBpad(m_matProcWt, procWtCache);
	XSP_CHECK((matProcWt.tpad == m_nResizePadLen) && (matProcWt.bpad == m_nResizePadLen), 
              XRAY_LIB_XMAT_SIZE_ERR, "Pipe3A idx2: CacheTBpad procWtCache err.");
    queProcWt.push_back(packNext);

	// DSP需求，无论是否TIP都输出数据
	memcpy(idx2.TipParam.pTipedCaliLowData, matLow.PadPtr(), nCalilhzWid * nCalilhzHei * XSP_ELEM_SIZE(XSP_16UC1));
	memcpy(idx2.TipParam.pTipedCaliHighData, matHigh.PadPtr(), nCalilhzWid * nCalilhzHei * XSP_ELEM_SIZE(XSP_16UC1));
    for (int32_t i = 0; i < nCalilhzHei; ++i) // 复合Z与包裹Mask
    {
        uint8_t* pSrcZ = matZ.Ptr(matZ.tpad + i); // 取第0~6Bit
        uint8_t* pSrcM = matMask.Ptr(matMask.tpad + i); // 取第7Bit
        uint8_t* pDst = (uint8_t*)idx2.TipParam.pTipedZData + i * nCalilhzWid;
        for (int32_t j = 0; j < nCalilhzWid; ++j, ++pSrcZ, ++pSrcM, ++pDst)
        {
            *pDst = (*pSrcZ & 0x7F) | (*pSrcM & 0x80);
        }
    }

    /// 构造m_matMerge、m_matDefaultEnhance、m_matSpecialEnhance
    hr = m_matMerge.Reshape(m_nResizeHeight, m_nResizeWidth, m_nResizePadLen, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: merge temp data reshape err.");
    hr = m_matDefaultEnhance.Reshape(m_nResizeHeight, m_nResizeWidth, m_nResizePadLen, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: enhance temp data reshape err.");
    hr = m_matSpecialEnhance.Reshape(m_nResizeHeight, m_nResizeWidth, m_nResizePadLen, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: special enhance temp data reshape err.");

    XMat& matMergeEnhIn = (XRAYLIB_DEFAULTENHANCE_CLOSE == m_enDefaultEnhanceJudge) ? m_matMerge : m_matDefaultEnhance; // 原始模式下直接使用未做增强的融合灰度图
    if ((XRAYLIB_TESTMODE_EUR == m_modImgproc.m_enTestMode || XRAYLIB_TESTMODE_USA == m_modImgproc.m_enTestMode) && 
        (XRAYLIB_ON == m_modImgproc.m_enTestAutoLE) && (m_modImgproc.m_enEnhanceMode == XRAYLIB_ENHANCE_NORAML))
    {
        m_modImgproc.m_enEnhanceMode = XRAYLIB_SPECIAL_LOCALENHANCE;
    }
    bool bNormalEnhance = (XRAYLIB_ENHANCE_NORAML == m_modImgproc.m_enEnhanceMode); // 是否只有默认增强，无特殊增强

    std::vector<XRAYLIB_RECT> procWins;
    if (packArea.size() > 0) // 有包裹图像才做处理
    {
        std::transform(packArea.begin(), packArea.end(), std::back_inserter(procWins),
                       [](const isl::Rect<int32_t>& r) -> XRAYLIB_RECT {
                          return {static_cast<unsigned int>(r.x),
                                  static_cast<unsigned int>(r.y),
                                  static_cast<unsigned int>(r.width),
                                  static_cast<unsigned int>(r.height)};
                       });

        /******************* 高低能融合 *******************/
        hr = m_modImgproc.ImgMerging(m_matMerge, matLow, matHigh, matProcWt);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: ImgMerging err.");

        /******************* 默认增强 *******************/
        hr = m_modImgproc.ImgEnhancing(m_matDefaultEnhance, m_matMerge, 0);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: ImgEnhancing err.");
        memcpy(idx2.out_merge.pData[0], m_matDefaultEnhance.PadPtr(), nCalilhzWid * nCalilhzHei * XSP_ELEM_SIZE(XSP_16UC1));

        /******************* 特殊增强 *******************/
        if (!bNormalEnhance)
        {
            hr = m_modImgproc.ImgSpecialEnhance(matMergeEnhIn, matHigh, matLow, m_matSpecialEnhance, 0, m_nResizeHeight);
            XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: ImgSpecialEnhance err.");
        }
    }
    else
    {
        memset(idx2.out_merge.pData[0], 0xFF, nCalilhzWid * nCalilhzHei * XSP_ELEM_SIZE(XSP_16UC1));
    }

    queProcWt.pop_front();

    /******************* 显示图像上伪彩 *******************/
	XMat& matGray = (!bNormalEnhance) ? m_matSpecialEnhance : matMergeEnhIn; // 根据是否使用特殊增强选择上色的灰度XMat
	
    hr = GetColorImage(idx2.out_disp, matGray, matZ, matProcWt, procWins, idx2.out_disp.stride[0], nCalilhzWid);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx2: GetColorImage err.");

	return XRAY_LIB_OK;
}