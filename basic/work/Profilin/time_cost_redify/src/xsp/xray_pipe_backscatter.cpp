#include "xray_image_process.hpp"

XRAY_LIB_HRESULT XRayImageProcess::PipelineProcessBS3B(XRAYLIB_PIPELINE_PARAM_MODE3B& pipe3B, int index)
{
	XRAY_LIB_HRESULT hr;
	if (0 == index)
	{
        /*********************************************************************************
         * 一级流水：
         * 1. 缓存dsp输入原始数据
         * 2. NLMeans数据: 归一化(时间轴方向缩小)-->NLMeans降噪-->缩放(时间轴方向放大到原尺寸)
         *   -->几何配准(探测器方向放大到数据宽度)-->拷贝输出
         * 3. AiXsp预处理数据: 归一化-->时间轴放大-->几何配准-->拷贝输出
         ********************************************************************************/
		hr = PipeBS3B_PreProcess(pipe3B.idx0);
	}
	else if (1 == index)
	{
        /*********************************************************************************
         * 二级流水：
         * 1. 透传一级流水中NLMeans数据从输入到输出
         * 2. 处理AiXsp预处理数据过模型, 并拷贝到输出
         ********************************************************************************/
		hr = PipeBS3B_CnnProcess(pipe3B.idx1);
	}
    else if (2 == index)
    {
        /*********************************************************************************
         * 三级流水：
         * 1. NLmeans和AiXsp数据分别做背景滤除, 输出
         * 2. 做完背景滤除的NLMeans和Aixsp数据做加权增强,输出
         * 3. 增强数据上色, 输出
         * 
         * note:
         *    当前未增加指标模式, 指标模式流程为:
         *    原始数据-->归一化-->背景滤除-->几何配准-->横向放大-->上色显示
         *    通过下发"降噪加权系数"的方式, 控制原始数据和NLMeans后数据的加权关系, 
         *    "降噪加权系数"同时控制是否做增强(?待定, 这样写有点奇怪), 输出上色
         ********************************************************************************/
        hr = PipeBS3B_PostProcess(pipe3B.idx2);
    }
	else
	{
		hr = XRAY_LIB_INVALID_PARAM;
	}
	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::PipeBS3B_PreProcess(XRAYLIB_PIPELINE_PARAM_MODE3B::_pipeline_3b_idx0_& idx0)
{
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER == m_sharedPara.m_enEnergyMode, XRAY_LIB_ENERGYMODE_ERROR);

	XRAY_LIB_HRESULT hr;
	/*******************************************
	*           out_calilhz_ori 获取
	*******************************************/
    XMat matXrawIn, matProc0;

    int nHeight = idx0.in_xraw.height;
    int nWidth = idx0.in_xraw.width;

    int32_t nPadCacheNum = ((XRAYLIB_TESTMODE_CN_CHECK == m_modImgproc.m_enTestMode) ? (m_modTcproc.m_nStripeBufferNum - 1) : 1);

	// 缓存参数
	std::pair<int32_t, int32_t> nBSPad(nHeight, nPadCacheNum * nHeight);

	hr = matXrawIn.Init(nHeight, nWidth, XSP_16UC1, (uint8_t*)idx0.in_xraw.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "PipeBS3B_PreProcess idx0: Get xraw_in bs data failed.");

    /* DSP传入条带标志入列 */
	m_queFlag.push_back(idx0.in_flag);

    // 原始数据是否需要镜像
    if (m_modCali.m_bMirrorBS)
    {
        MirrorUpDownBS_u16(matXrawIn, m_modCali.m_nImageWidthBS);
    }

    hr = m_modCali.SetTBpad(nWidth, nHeight, XSP_16UC1, nBSPad, m_modCali.m_stBSCache, 0);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "PipeBS3B_PreProcess idx0: SetTBpad failed");

    bool bCacheSuc = false;
    m_matBSTmp = m_modCali.CacheTBpad(matXrawIn, m_modCali.m_stBSCache, &bCacheSuc);
	XSP_CHECK((m_matBSTmp.tpad == nBSPad.first) && (m_matBSTmp.bpad == nBSPad.second), XRAY_LIB_XMAT_SIZE_ERR, "Pipe3A idx0: ModStripeTBpadCache err.");

    if (bCacheSuc != true)
	{
		idx0.out_calilhz.height = 0;
		idx0.out_calilhz.width = 0;
		return XRAY_LIB_OK;
	}

    int nProcessHeight = m_matBSTmp.hei;

    hr = matProc0.Init(nProcessHeight, nWidth, XSP_16UC1, 
                        (uint8_t*)m_matNeighborBs0.data.ptr);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe2A idx1: Init matProc0 data failed.");

    /* NLMeans降噪 */
    // NLMeans归一化
    hr = matProc0.Reshape(nProcessHeight / m_modCali.m_fPulsBS, m_modCali.m_nImageWidthBS, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "NLMeans Reshape idx1: Reshape matProc0 data failed.");

    hr = m_modCali.CaliBSImageCPU(m_matBSTmp, matProc0, m_modCali.m_fAntiBS1_NLM);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "CaliBSImageCPU function execute failed");

    // 使用ImageProcess申请内存
    std::vector<XMat> matsProc = {
					m_matV1,
					m_matV2,
					m_matV2v,
					m_matSd,
					m_matfAvg,
					m_matfWeight,
					m_matfWMax
				};
    
    
    hr = m_matNLMeansTmp.Reshape(nProcessHeight / m_modCali.m_fPulsBS, m_modCali.m_nImageWidthBS, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx0: m_matNLMeansTmp reshape failed");

    if (0 != m_fDenoiseSigma)
    {
        hr = NLMeans(matProc0, m_matNLMeansTmp, matsProc);
	    XSP_CHECK(XRAY_LIB_OK == hr, hr, "NLMeans is err.");
    }
    else
    {
        memcpy(m_matNLMeansTmp.Ptr(), matProc0.Ptr(), nProcessHeight / m_modCali.m_fPulsBS * m_modCali.m_nImageWidthBS * sizeof(uint16_t));
    }
    
    // 时间轴方向放大到原尺寸
    hr = matProc0.Reshape(nProcessHeight, m_modCali.m_nImageWidthBS, XSP_16UC1);
    memset(matProc0.Ptr(), 0, nProcessHeight * m_modCali.m_nImageWidthBS * sizeof(uint16_t));
    
    hr = ResizeBilinear(m_matNLMeansTmp, matProc0);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe2A idx1:ResizeBilinear function execute failed");

    // 探测器方向放大到matXRawIn的宽度
    hr = m_matNLMeansTmp.Reshape(nProcessHeight, nWidth, XSP_16UC1);
    memset(m_matNLMeansTmp.Ptr(), 0, nProcessHeight * nWidth * sizeof(uint16_t));

    hr = m_modCali.GeoCali_BS_InterLinear(matProc0, m_matNLMeansTmp, m_modCali.m_bGeoCaliBSInverse);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe2A idx1: GeoCali_BS_InterLinear function execute failed");

    XMat matNLMeansOut;
    hr = matNLMeansOut.Init(m_matNLMeansTmp.hei, m_matNLMeansTmp.wid, XSP_16UC1, (uint8_t*)idx0.out_calilhz.pData[0]);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx0: matNLMeansOut init failed");
    if (XRAYLIB_ON == m_modCali.m_enGeoMertric && m_modCali.m_bGeoCaliCanUse)
    {
        hr = m_modCali.ExecuteGeoCali(m_matNLMeansTmp, matNLMeansOut);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "GeoCali is err.");
    }
    else
    {
        memcpy(matNLMeansOut.Ptr(), m_matNLMeansTmp.Ptr(), sizeof(uint16_t) * nWidth * nProcessHeight);
    }
    // 设置NLMeans输出参数
    idx0.out_calilhz.width = nWidth;
    idx0.out_calilhz.height = nProcessHeight;
    idx0.out_calilhz.stride[0] = nWidth;

    /* AIXSP预处理 */
    // 归一化模块会对时间轴方向做m_fPulsBS倍下采样, 提前Reshape, 方便后续Resize回原尺寸
    hr = m_matAiXspTmp.Reshape(nProcessHeight / m_modCali.m_fPulsBS, m_modCali.m_nImageWidthBS, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx0: m_matAiXspTmp reshape failed");

    hr = m_modCali.CaliBSImageCPU(m_matBSTmp, m_matAiXspTmp, m_modCali.m_fAntiBS1_AI);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "CaliBSImageCPU function execute failed");

    hr = matProc0.Reshape(nProcessHeight, m_modCali.m_nImageWidthBS, XSP_16UC1);
    memset(matProc0.Ptr(), 0, nProcessHeight * m_modCali.m_nImageWidthBS * sizeof(uint16_t));

    // 时间轴方向放大到原尺寸
    hr = ResizeBilinear(m_matAiXspTmp, matProc0);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe2A idx1:ResizeBilinear function execute failed");

    hr = m_matAiXspTmp.Reshape(nProcessHeight, nWidth, XSP_16UC1);
    memset(m_matAiXspTmp.Ptr(), 0, nProcessHeight * nWidth * sizeof(uint16_t));

    // 探测器方向放大到matXRawIn的宽度
    hr = m_modCali.GeoCali_BS_InterLinear(matProc0, m_matAiXspTmp, m_modCali.m_bGeoCaliBSInverse);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe2A idx1: GeoCali_BS_InterLinear function execute failed");

    XMat matAiXspOut;
    hr = matAiXspOut.Init(m_matAiXspTmp.hei, m_matAiXspTmp.wid, XSP_16UC1, (uint8_t*)idx0.out_calilhz.pData[1]);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx0: matAiXspOut init failed");
    if (XRAYLIB_ON == m_modCali.m_enGeoMertric && m_modCali.m_bGeoCaliCanUse)
    {
        hr = m_modCali.ExecuteGeoCali(m_matAiXspTmp, matAiXspOut);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "GeoCali is err.");
    }
    else
    {
        memcpy(matAiXspOut.Ptr(), m_matAiXspTmp.Ptr(), sizeof(uint16_t) * nWidth * nProcessHeight);
    }
    // 设置AiXsp输出参数
	idx0.out_calilhz.width = nWidth;
    idx0.out_calilhz.height = nProcessHeight;    
	idx0.out_calilhz.stride[1] = nWidth;
    
    XSP_CHECK(!m_queFlag.empty(), XRAY_LIB_EMPTY_DATA, "stripe flag queue empty");
	idx0.out_flag = m_queFlag.front();
	m_queFlag.pop_front();

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::PipeBS3B_CnnProcess(XRAYLIB_PIPELINE_PARAM_MODE3B::_pipeline_3b_idx1_& idx1)
{
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER == m_sharedPara.m_enEnergyMode, XRAY_LIB_ENERGYMODE_ERROR);

	XRAY_LIB_HRESULT hr;
    // 背散没有超分, 不涉及Resize, 直接略过resize相关流程

    int nWidth = idx1.in_calilhz.width;
    int nHeight = idx1.in_calilhz.height;
    int nAiProcHeight = m_modCnn.AiXsp_GetHeiIn();
    int nHeightOut = std::min(nHeight, nAiProcHeight);  // 找AI输出和扩边最小的高度, 保证二级流水输出的NLMeans数据和Aixsp数据宽高一致

    XMat matAiXspIn, matAiXspOut;
    
    hr = matAiXspIn.Init(nHeight, nWidth, XSP_16UC1, (uint8_t*)idx1.in_calilhz.pData[1]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "PipeBS3B_PreProcess idx0: Get xraw_in bs data failed.");

    hr = matAiXspOut.Init(nHeightOut, nWidth, XSP_16UC1, (uint8_t*)idx1.out_calilhz.pData[1]);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "PipeBS3B_PreProcess idx0: Get xraw_in bs data failed.");

    // 模型降噪
    Calilhz caliAiXspIn, caliAiXspOut;
    hr = caliAiXspIn.low.Init(matAiXspIn.hei, matAiXspIn.wid, XSP_16UC1, (uint8_t*)matAiXspIn.data.ptr);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx1: Get in_calilhz low data failed.");

    hr = caliAiXspOut.low.Init(matAiXspOut.hei, matAiXspOut.wid, XSP_16UC1, (uint8_t*)matAiXspOut.data.ptr);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3A idx1: Get in_calilhz low data failed.");

    hr = m_modCnn.AiXsp_SyncProcess(caliAiXspIn, caliAiXspOut);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "AiXsp_SyncProcess BS function execute failed");

    XMat matNLMeansIn, matNLMeansOut;

    hr = matNLMeansIn.Init(nHeight, nWidth, XSP_16UC1, (uint8_t*)idx1.in_calilhz.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "PipeBS3B_PreProcess idx0: Get xraw_in bs data failed.");

    hr = matNLMeansOut.Init(nHeightOut, nWidth, XSP_16UC1, (uint8_t*)idx1.out_calilhz.pData[0]);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "PipeBS3B_PreProcess idx0: Get xraw_in bs data failed.");

    int nNLMeansHeiProc = (nHeight - nHeightOut) / 2;
    // NLMeansHeiProc >= 0

    // 透传NLMeans数据
    memcpy(matNLMeansOut.Ptr(0), matNLMeansIn.Ptr(nNLMeansHeiProc), nHeightOut * nWidth * sizeof(uint16_t));

    // 输出参数
    idx1.out_calilhz.height = nHeightOut;
    idx1.out_calilhz.width = nWidth;
    idx1.out_calilhz.stride[0] = nWidth;
    idx1.out_calilhz.stride[1] = nWidth;

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::PipeBS3B_PostProcess(XRAYLIB_PIPELINE_PARAM_MODE3B::_pipeline_3b_idx2_& idx2)
{
	XSP_CHECK(XRAYLIB_ENERGY_SCATTER == m_sharedPara.m_enEnergyMode, XRAY_LIB_ENERGYMODE_ERROR);

	XRAY_LIB_HRESULT hr;
    // 背散没有超分, 不涉及Resize, 直接略过resize相关流程

    int nPad = (idx2.in_calilhz.height - m_nRTHeight) / 2;
    int nWidthIn = idx2.in_calilhz.width;
    int nHeightIn = idx2.in_calilhz.height;   // 此处的nHeight为2级流水透传过来, 含邻边
    int nHeightNoNe = nHeightIn - 2 * nPad;  // 此处的nHeightNoNe为不含邻域的实际输出高度
    int nWidthResizeOut = nWidthIn * m_fWidthResizeFactor;
    int nHeightResizeOut = nHeightIn * m_fHeightResizeFactor;
    int nHeightOut = nHeightNoNe * m_fHeightResizeFactor;
    int nPadResize = nPad * m_fHeightResizeFactor;
    
    XMat matAiXspIn, matAiXspOut, matNLMeansIn, matNLMeansOut, matEnhance, matProc1;

    /* NLMeans数据背景滤除 */
    hr = matNLMeansIn.Init(nHeightIn, nWidthIn, nPad, nPad, XSP_16UC1, (uint8_t*)idx2.in_calilhz.pData[0]);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx2: Get matNLMeansIn data failed.");
    hr = m_matNLMeansOut.Reshape(nHeightIn, nWidthIn, nPad, nPad, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx2: Reshape m_matNLMeansOut data failed.");
    hr = matProc1.Init(nHeightIn, nWidthIn, nPad, nPad, XSP_16UC1, (uint8_t*)m_matNeighborBs1.data.ptr);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx2: Init matProc1 data failed.");
    // NLMeans背景滤除模板数据为0
    memset(matProc1.Ptr(), 0, nHeightIn * nWidthIn * sizeof(uint16_t));
    
    hr = m_modCali.BackgroundCali_BS(matNLMeansIn, m_matNLMeansOut, matProc1, m_modCali.m_fAntiBS2_NLM);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx2: Init matProc1 data failed.");

    hr = m_matNLMeansResizeOut.Reshape(nHeightResizeOut, nWidthResizeOut, nPadResize, nPadResize, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx2: Reshape m_matNLMeansResizeOut failed.");

    hr = ResizeBilinear(m_matNLMeansOut, m_matNLMeansResizeOut);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx2: ResizeBilinear m_matNLMeansResizeOut failed.");

    // 背景滤除后的NLMeans数据输出
    memcpy((uint8_t*)idx2.out_calilhz.pData[0], m_matNLMeansResizeOut.PadPtr(), 
            nHeightOut * nWidthResizeOut * sizeof(uint16_t));

    /* AIXSP数据背景滤除 */
    hr = matAiXspIn.Init(nHeightIn, nWidthIn, nPad, nPad, XSP_16UC1, (uint8_t*)idx2.in_calilhz.pData[1]);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx2: Init matAiXspIn data failed.");
    hr = m_matAiXspOut.Reshape(nHeightIn, nWidthIn, nPad, nPad, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx2: Reshape m_matAiXspOut data failed.");
    hr = m_modCali.BackgroundCali_BS(matAiXspIn, m_matAiXspOut, m_modCali.m_matCaliTableAirBSSecCali_AI, m_modCali.m_fAntiBS2_AI);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe2A idx1: BackgroundCaili_BS function execute failed");

    hr = m_matAiXspResizeOut.Reshape(nHeightResizeOut, nWidthResizeOut, nPadResize, nPadResize, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx2: ResizeBilinear m_matAiXspResizeOut failed.");

    hr = ResizeBilinear(m_matAiXspOut, m_matAiXspResizeOut);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx2: ResizeBilinear m_matAiXspResizeOut failed.");

    // 背景滤除后的AiXsp数据输出
    memcpy((uint8_t*)idx2.out_calilhz.pData[1], m_matAiXspResizeOut.PadPtr(), nHeightOut * nWidthResizeOut * sizeof(uint16_t));

    /* Sobel算子增强 */
    hr = m_matDefaultEnhance.Reshape(nHeightResizeOut, nWidthResizeOut, nPadResize, nPadResize, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx2: Reshape m_matDefaultEnhance data failed.");

    hr = m_modImgproc.EdgeEnhance_Sobel(m_matAiXspResizeOut, m_matNLMeansResizeOut, m_matDefaultEnhance, 0);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx2:EdgeEnhance_Sobel function execute failed");

    // 增强后的merge数据输出
    memcpy((uint8_t*)idx2.out_merge.pData[0], m_matDefaultEnhance.PadPtr(), 
            nHeightOut * nWidthResizeOut * sizeof(uint16_t));

    // 上色数据直接输出
    hr = GetColorImageBS(m_matDefaultEnhance, idx2.out_disp, nHeightOut,
						idx2.out_disp.stride[0], nWidthResizeOut);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Pipe3B idx1: GetColorImage err.");

    idx2.out_disp.width = nWidthResizeOut;
    idx2.out_disp.height = nHeightOut;
    idx2.out_disp.stride[0] = nWidthResizeOut;
    idx2.out_disp.stride[1] = nWidthResizeOut;
    idx2.out_disp.stride[2] = nWidthResizeOut;

	return XRAY_LIB_OK;
}