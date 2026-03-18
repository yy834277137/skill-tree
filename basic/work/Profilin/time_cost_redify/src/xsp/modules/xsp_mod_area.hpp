/**
*      @file xsp_mod_area.hpp
*	   @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
*
*      @brief XSP区域检测模块，负责难穿透、可疑有机物、爆炸物、毒品区域检测
*
*      @author wangtianshu
*      @date 2023/2/9
*
*      @note
*/

#ifndef _XSP_MOD_AREA_HPP_
#define _XSP_MOD_AREA_HPP_

#include "xsp_interface.hpp"
#include "xray_shared_para.hpp"

/**************************************************************************************************
*                                       可疑物参数结构体定义
***************************************************************************************************/
typedef struct _CONCERNEDM_
{
	int32_t nGrayDown;        ///可疑物灰度下限
	int32_t nGrayUp;          ///可疑物灰度上限
	int32_t nZDown;           ///可疑物Z值下限
	int32_t nZUp;             ///可疑物Z值上限
	int32_t nAreaNumberMin;    ///可疑物连通区域像素点数阈值
	int32_t nRectWidthMin;     ///可疑物报警框宽度最小值
	int32_t nRectHeightMin;    ///可疑物报警框高度最小值
}CONCERNEDM;

/**************************************************************************************************
*                                       爆炸物检测结构体定义
***************************************************************************************************/

struct RegionInfo
{
	int32_t pixel_count = 0;      // 区域像素数量
	float32_t total_gray = 0.0f;  // 区域灰度总和（用于计算平均值）
	int32_t original_label = 0;   // 原始标签值
	int32_t min_width = 65535;
	int32_t max_width = 0;
	int32_t min_height = 65535;
	int32_t max_height = 0;

	// 计算平均灰度
	float32_t mean_gray() const
	{
		return (pixel_count > 0) ? total_gray / pixel_count : 0.0f;
	}
};

struct SeedPoint
{
	int32_t n_width;          // 坐标X
	int32_t n_height;         // 坐标Y
	int32_t n_index;          // 索引
	SeedPoint(int32_t width, int32_t height, int32_t index) : n_width(width), n_height(height), n_index(index) {}
};

struct RegionAttr
{
	int32_t min_width;
	int32_t max_width;
	int32_t min_height;
	int32_t max_height;
	int32_t area;//像素个数
	float32_t rate;//占空比
	float32_t thick;//平均厚度

	// 检查当前矩形是否完全包含另一个矩形
	bool fullyContains(const RegionAttr& other) const
	{
		return (other.min_height - 20 <= min_height &&
			other.min_width - 20 <= min_width &&
			other.max_height + 20 >= max_height &&
			other.max_width + 20 >= max_width);
	}

	// 检查两个矩形是否存在包含关系（双向检查）
	bool hasContainment(const RegionAttr& other) const
	{
		return fullyContains(other) || other.fullyContains(*this);
	}
	// 检查两个矩形是否重叠或包含
	bool overlapsOrContains(const RegionAttr& other) const
	{
		// 不重叠条件
		if (max_height < other.min_height || min_height> other.max_height ||
			max_width < other.min_width || min_width> other.max_width)
		{
			return false;
		}
		return true;
	}

	// 合并矩形
	void merge(const RegionAttr& other)
	{
		min_width = std::min(min_width, other.min_width);
		min_height = std::min(min_height, other.min_height);
		max_height = std::max(max_height, other.max_height);
		max_width = std::max(max_width, other.max_width);
	}
};

struct limit
{
	//1.原始高低能引导滤波
	int32_t guide_radius = 1;
	float32_t image_epsilon = 0.005 * 65535 * 65535;

	//2.厚度信息引导滤波
	float32_t thick_epsilon = 0.001 * 65535 * 65535;

	//3.最大值滤波
	int32_t max_radius = 2;

	//4.二值化阈值
	float32_t gradient_thresh = 0.1;

	//5.连通域使用
	int32_t nGrayDown = 2000;        ///可疑物灰度下限
	int32_t nGrayUp = 45000;          ///可疑物灰度上限
	int32_t nZDown = 6;           ///可疑物Z值下限
	int32_t nZUp = 13;             ///可疑物Z值上限
	int32_t nAreaNumberMin = 800;    ///可疑物连通区域像素点数阈值
	int32_t nAreaNumberMax = 15000;    ///可疑物连通区域像素点数阈值
	int32_t max_width = 300;   //用于过滤行李箱瘦长区域
	int32_t min_width = 10;
	int32_t min_height = 25;   
	//6.区域生长筛选使用
	int32_t nRegionNumberMin = 800;
	int32_t nRegionNumberMax = 25000;
	int32_t nMaxLen = 240;
	int32_t nMinLen = 10;
	int32_t nLenMaxRate = 5;
	float32_t fMinRate = 0.4;
	int32_t nGrowthValue = 7;
	int32_t nMinThick = 20; //默认中间模式，厚度： 20 30 40

};

/**************************************************************************************************
*                                      基材料线衰减系数定义
***************************************************************************************************/
/**
* @brief 物质分类曲线查找表
*/
struct MuTable
{
	int32_t     len;     /* 查找表长度，表示低能灰度级数 */
	double* pLowMu;      /*  低能线衰减系数指针 */
	double* pHighMu;     /* 高能线衰减系数指针  */

	MuTable()
		: len(0), pLowMu(0), pHighMu(0) {}

	// @brief 基于低能灰度获取低能线衰减值
	inline double CurveLowMu(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pLowMu[grayVal];
	}

	// @brief 基于低能灰度获取高能线衰减值
	inline double CurveHighMu(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pHighMu[grayVal];
	}
};

typedef struct _REGIONINFO_ /// 区域属性
{
	int32_t nNumber = 0; /// 区域编号
	int32_t nWidthMin = 0; /// 区域宽度最小坐标
	int32_t nHeightMin = 0;  /// 区域高度最小坐标
	int32_t nWidthMax = 0; /// 区域宽度最大坐标
	int32_t nHeightMax = 0; /// 区域高度最大坐标
	int32_t nArea = 0; /// 区域面积

	float32_t fThick = 0; /// 区域平均厚度
	float32_t fThickAlDiff = 0; /// 区域去除背景后的平均厚度
	float32_t fThickAlDiff1 = 0; /// 区域去除邻域后的平均厚度
	float32_t fThickAlDiffMin = 0; /// 区域去除背景后的最小厚度
	float32_t fWeight = 0; /// 区域重量
	float32_t fRatioAir = 0; /// 区域周围的空气占比
	float32_t fRatioPMMA = 0; /// 区域C基占比
	float32_t fRatioAl = 0; /// 区域Al基占比
	float32_t fRatioFe = 0; /// 区域Fe基占比
	float32_t fScore = 0.0f; /// 区域评分
	float32_t fRatio = 0.0f; /// 占空比
}REGIONINFO;

typedef struct _PARAM_
{
	int32_t nNeighborhood = 3; /// 引导滤波邻域参数
	float32_t fDegreeOfSmoothGray = 1e-3 * 65536 * 65536; /// 灰度图引导滤波平滑因子
	float32_t fDegreeOfSmoothMu = 0.1; /// 厚度图引导滤波参数

	float32_t fRegionMeanDiff = 1.5; /// 区域生长因数，区域平均差异因子
	float32_t fRegionPixelDiff = 0.5; /// 区域生长因子，区域逐像素差异因子
	float32_t fRegionCoef = 3.5; /// 区域生长因子

	uint16_t unMaxGray = 61000; /// 灰度上限，厚度直接置为0

	int32_t nMinFilterSize = 4; /// 最小值滤波参数
	int32_t nMinFilterSizeFe = 2; /// Fe基厚度最小值滤波参数
	int32_t nFindSeedRadius = 5; /// 种子点搜索半径

	int32_t nBoxInnerSize = 5; /// 平均滤波邻域像素数1
	int32_t nBoxOuterSize = 50; /// 平均滤波邻域像素数2
	int32_t nConnect = 8; /// 连通域参数
	int32_t nExpandSize = 30; /// 外部区域扩充参数

	float32_t fThickAlDiffThres = 5.0f; /// 主视角Al差异图阈值1
	float32_t fThickAlDiffThres2 = 10.0f; /// 主视角Al差异图阈值2
	float32_t fFeThs = -8; /// Fe基图像阈值

	int32_t nHeight; /// 图像高度
	int32_t nWidth; /// 图像宽度
	int32_t nSize; /// 图像尺寸
	int32_t nView; /// 图像视角
	int32_t nScale; /// 图像缩放因子

	int32_t nSegNumber = 4; /// 区域生长起始索引
	float32_t fScoreWeight = 10.0; /// 评分函数重量阈值
	float32_t fScoreDiff = 8.0; /// 评分函数厚度阈值

	int32_t BWSeg = 0; /// 是否为连通域模式
	float32_t fThickThsBW = 5.0;
	float32_t fAreaThsBW = 50000;

	float32_t fScoreThsMain = 0.8;
	float32_t fScoreThsAux = 0.9;
}PARAM;

typedef struct _SEEDPOINT_
{
	int32_t nWidth;  /// 坐标x
	int32_t nHeight; /// 坐标y
	int32_t fThickAlDiff; /// 特征点
}SEEDPOINT;

class AreaModule : public BaseModule
{
public:
	AreaModule();


	/**@fn      Init
	* @brief    区域检测模块初始化
	* @param1   pPara                   [I]     - 公共参数
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT Init(const char* szPublicFileFolderName,SharedPara* pPara);

	/**@fn      SetSensitivity
	* @brief    设置可疑区域检测等级
	* @param1   sensitivity              [I]     - 可疑等级
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT SetSensitivity(XRAYLIB_CONCERNEDM_SENSITIVITY sensitivity);
	
	
	/**@fn      GetConcernedArea
	* @brief    可疑区域识别
	* @param1   xraylib_img                 [I]     - 归一化数据输入
	* @param2   ConcerdMaterialM            [O]     - 可疑有机物区域
	* @param3   LowPeneArea                 [O]     - 低穿透区域
	* @param4   ExplosiveArea               [O]     - 爆炸物区域
	* @param5   DrugArea                    [O]     - 毒品区域
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT GetConcernedArea(XRAYLIB_IMAGE&         xraylib_img,
									  XRAYLIB_CONCERED_AREA	*ConcerdMaterialM,
									  XRAYLIB_CONCERED_AREA	*LowPeneArea,
									  XRAYLIB_CONCERED_AREA	*ExplosiveArea,
									  XRAYLIB_CONCERED_AREA	*DrugArea);

	/**@fn      MapAreaCoordinates
	* @brief    可疑区域畸变校正映射
	* @param1   m_matGeoDetectionPixelOffsetLut		[I]     - 畸变校正映射关系
	* @param2   pArea								[O]     - 可疑区域
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT MapAreaCoordinates(XMat& m_matGeoDetectionPixelOffsetLut,
										XRAYLIB_CONCERED_AREA* pArea);

	/**@fn      GeoConcernedArea
	* @brief    可疑区域畸变校正映射
	* @param1   m_matGeoDetectionPixelOffsetLut				[I]     - 畸变校正映射关系
	* @param2   ConcerdMaterialM							[O]     - 可疑有机物区域
	* @param3   LowPeneArea									[O]     - 低穿透区域
	* @param4   ExplosiveArea								[O]     - 爆炸物区域
	* @param5   DrugArea									[O]     - 毒品区域
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT GeoConcernedArea(XMat&         m_matGeoDetectionPixelOffsetLut,
									  XRAYLIB_CONCERED_AREA	*ConcerdMaterialM,
									  XRAYLIB_CONCERED_AREA	*LowPeneArea,
									  XRAYLIB_CONCERED_AREA	*ExplosiveArea,
									  XRAYLIB_CONCERED_AREA	*DrugArea);


	/**@fn      GetConcernedAreaPiecewise
	* @brief    可疑区域识别分段检测
	* @param1   xraylib_img                 [I]     - 归一化数据输入
	* @param2   ConcerdMaterialM            [O]     - 可疑有机物区域
	* @param3   LowPeneArea                 [O]     - 低穿透区域
	* @param4   ExplosiveArea               [O]     - 爆炸物区域
	* @param5   DrugArea                    [O]     - 毒品区域
	* @param6   nCurNum                     [I]     - 分段索引
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT GetConcernedAreaPiecewise(XRAYLIB_IMAGE&            xraylib_img,
									           XRAYLIB_CONCERED_AREA	*ConcerdMaterialM,
									           XRAYLIB_CONCERED_AREA	*LowPeneArea,
									           XRAYLIB_CONCERED_AREA	*ExplosiveArea,
									           XRAYLIB_CONCERED_AREA	*DrugArea,
		                                       int32_t                       nDetecFlag);

	/**@fn      Release
	* @brief    释放接口
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT Release();

	/**@fn      GetMemSize
	* @brief    获取内存表所需内存大小(字节单位)
	* @param1   MemTab             [O]     - 内存表
	* @param2   nDetectorWidth     [I]     - 探测器宽度
	* @return   错误码
	* @note     算法库所需内存由外部申请, 需提前计算所需内存大小
	*/
	virtual XRAY_LIB_HRESULT GetMemSize(XRAY_LIB_MEM_TAB &MemTab, XRAY_LIB_ABILITY &ability);



protected:
	typedef bool(AreaModule::*Func)(uint8_t, uint16_t, int32_t, int32_t, int32_t, int32_t);
	
	/**@fn      RegionSeg
	* @brief    可疑物区域分割
	* @param1   ptrImageIn          [I]     - 归一化数据输入
	* @param2   ptrMaterialIn       [I]     - 原子序数
	* @param3   pConceredM          [I/O]   - 可疑物区域
	* @param4   stConcerned         [I]     - 可疑物参数的阈值
	* @param5   nRegionSegNum       [O]     - 可疑物区域的数量
	* @param6   nRegionSegStatus    [O]     - 可疑物区域的状态
	* @param7   bFitpoint           [I]     - 函数指针
	* @return   
	* @note     
	*/
	void RegionSeg(XMat& ptrImageIn,
					XMat& ptrMaterialIn,
					XRAYLIB_CONCERED_AREA* pConceredM,
					CONCERNEDM stConcerned,
					XMat& nRegionSegNum,
					XMat& nRegionSegStatus,
					Func bFitpoint);

	// @brief    判断是否为可疑有机物
	bool bFitPoint2(uint8_t Material,
					uint16_t GrayValue,
					int32_t nConcernedGrayDown,
					int32_t nConcernedGrayUp,
					int32_t nConcernedZDown,
					int32_t nConcernedZUp);
	
	// @brief    判断是否为难穿透
	bool bFitPoint3(uint8_t Material,
					uint16_t GrayValue,
					int32_t nConcernedGrayDown,
					int32_t nConcernedGrayUp,
					int32_t nConcernedZDown,
					int32_t nConcernedZUp);

	// @brief    去除重复候选框
	void RemoveBox(std::vector<XRAYLIB_BOX> &vecRect);

	/**@fn      SaveBox
	* @brief    存储area中rect到vector容器
	* @param1   rect                           [I]     - 原始存储rect
	* @param2   vecRect					       [O]     - 临时rect的vector向量
	* @return   错误码
	* @note
	*/
	void SaveBox(XRAYLIB_CONCERED_AREA* rect, std::vector<XRAYLIB_BOX>& vecRect);

	/**@fn      ResetBox
	* @brief    存储area中rect到vector容器
	* @param1   rect                           [I/O]     - 原始存储rect
	* @param2   LowPeneArea					   [I]       - 临时rect的vector向量
	* @return   错误码
	* @note
	*/
	void ResetBox(XRAYLIB_CONCERED_AREA* rect, std::vector<XRAYLIB_BOX>& vecRect, int32_t nCurNum);


    //**************************************************new add************************************
	/**@fn       InitMuTable
	* @brief     初始化Mu列表
	*/
	XRAY_LIB_HRESULT InitMuTable(const char*szPublicFileFolderName);
	
	
	
	/**@fn      ReadMuTable
	* @brief    读取线衰减系数表
	* @param1   szFileName                     [I]       - 文件名
	* @param2   stMuTbe					       [O]       - 线衰减系数表
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT ReadMuTable(const char* szFileName, MuTable& stMuTbe);
/************************************************************************/
	/*								dw代码									*/
	/************************************************************************/

	void ConnectedComponents(const int32_t* image_data,
		int32_t width,
		int32_t height,
		int32_t* labels_data,
		std::vector<int32_t>& areas);

	void BoxFilterESD(const float32_t* src,
		float32_t* dst,
		int32_t nWidth,
		int32_t nHeight,
		int32_t r);
	void GuidedFilter(const float32_t* I,
		float32_t* fImgOut,
		int32_t nWidth,
		int32_t nHeight,
		int32_t nRadius,
		float32_t eps);
	void CalcThick(const float32_t* pfImgL,
		const float32_t* pfImgH,
		float32_t* pfThickPMMA,
		float32_t* pfThickAl,
		float32_t* pfThickFe,
		float32_t* pfThickAl1,
		PARAM param);

	void MinFilter(const float32_t* fImgIn,
		float32_t* fImgOut,
		float32_t* fThreshold,
		PARAM param);

	void MinFilter(const float32_t* fImgIn,
		float32_t* fImgOut,
		PARAM param);

	void FindSeed(float32_t* fThick,
		std::vector <SEEDPOINT>& SeedPointInfo,
		int32_t seedradius,
		int32_t nWidth,
		int32_t nHeight);

	void Process_Rescale(uint16_t* psImgL,	/// 输入低能图像 (0-65535)
		uint16_t* psImgH, /// 输入高能图像 (0-65535)
		uint8_t* pcImgZ,	/// 输入原子序数图 (0-43)
		float32_t* pfImgL,			/// 输出低能图像 (0-65535)
		float32_t* pfImgH,			/// 输出高能图像 (0-65535)
		float32_t* pfImgZ,			/// 输出原子序数图
		PARAM& param);			/// 图像参数

	void Process_Filter(float32_t* pfImgL,			/// 输入低能图像
		float32_t* pfImgH,			/// 输入高能图像
		float32_t* pfImgLGuided,	/// 输出低能引导滤波图像
		float32_t* pfImgHGuided,	/// 输出高能引导滤波图像
		PARAM param);			/// 图像参数

	void Process_Stripe(float32_t* pfImgLGuided,	/// 输入低能引导滤波图像
		float32_t* pfImgHGuided,	/// 输入高能引导滤波图像
		float32_t* fThickAl,		/// 输入Al-Fe分解 Al系数图
		PARAM param);			/// 图像参数

	void Process_FindSeed(float32_t* fThickAlGuidedMinf,				/// 输入最小值滤波后的Al厚度值
		float32_t* fThickAlCorr,					/// 经过sigmoid函数后的Al厚度值
		float32_t* fThickFe,						/// C-Fe分解 Fe系数值
		float32_t* fThickInputInner,				/// 小范围均值滤波结果
		float32_t* fThickInputOuter,				///	大范围均值滤波结果
		float32_t* fThickAlDiff,					/// Al厚度差
		std::vector<SEEDPOINT>& SeedPointInfo,	/// 种子点位置
		PARAM param);							/// 图像参数

	void Score(int32_t* nSegIndex,
		const float32_t* pfThick,
		const float32_t* pfThickAlDiff,
		float32_t* fThickPMMA,
		float32_t* fThickAl,
		float32_t* fThickFe,
		float32_t* fAirConv,
		int32_t* nSegSingle,
		REGIONINFO& attr,
		std::vector<int32_t>& connect,
		int32_t nHeightMin,
		int32_t nHeightMax,
		int32_t nWidthMin,
		int32_t nWidthMax,
		int32_t nArea,
		int32_t nSegNumber,
		float32_t fThickSum,
		PARAM param);

	void RegionSegment(int32_t* nSegIndex,
		const float32_t* pfThickCorr,
		const float32_t* pfThick,
		const float32_t* pfThickAlDiff,
		float32_t* fThickPMMA,
		float32_t* fThickAl,
		float32_t* fThickFe,
		float32_t* fAirConv,
		int32_t* nVisited,
		int32_t* nSegSingle,
		int32_t* nSegConv,
		std::vector<SEEDPOINT> SeedPointInfo,
		std::vector<REGIONINFO>& RegionInfo,
		std::vector<std::vector<int32_t>>& vConnectTotal,
		PARAM param);

	void Process_Segment(int32_t* nSegIndex,
		float32_t* pfImgL,
		float32_t* fThickPMMA,
		float32_t* fThickAl1,
		float32_t* fThickFe,
		const float32_t* fThickAlGuidedMinf,
		float32_t* fThickAlCorr,
		const float32_t* fThickAlDiff,
		float32_t* fAirMap,
		float32_t* fAirConv,
		int32_t* nVisited,
		int32_t* nSegSingle,
		int32_t* nSegConv,
		std::vector<SEEDPOINT> SeedPointInfo,
		std::vector<REGIONINFO>& RegionInfo,
		std::vector<std::vector<int32_t>>& vConnectTotal,
		PARAM param);

	void Process_Merge(int32_t* nSegIndex,
		int32_t* nSegSingle,
		std::vector<REGIONINFO>& RegionInfo,
		std::vector<std::vector<int32_t>>& vConnectTotal,
		float32_t* pfThick,
		float32_t* pfThickAlDiff,
		float32_t* fThickPMMA,
		float32_t* fThickAl,
		float32_t* fThickFe,
		float32_t* fAirConv,
		PARAM param);

	void RegionExp(uint16_t* psImgL,             //输入低能
		uint16_t* psImgH,             //输入高能
		uint8_t* pcImgZ,
		XRAYLIB_CONCERED_AREA* ExplosiveArea,  //输出位置信息结构体
		int32_t nWidth,
		int32_t nHeight);

public:
	/**************************************************************************
	*                               外部控制参数
	***************************************************************************/
	/*报警灵敏度*/
	XRAYLIB_LOWPENE_SENSITIVITY    m_enLowPeneSenstivity;		
	XRAYLIB_CONCERNEDM_SENSITIVITY m_enConcernedMSenstivity;
	XRAYLIB_CONCERNEDM_SENSITIVITY m_enDrugSenstivity;		
	XRAYLIB_CONCERNEDM_SENSITIVITY m_enExplosiveSenstivity;
	
	int32_t m_nLowPenetrationThreshold; //低穿透报警阈值  

	XRAY_LIB_ONOFF m_enDrugPrompt;          	// 毒品识别开关
	XRAY_LIB_ONOFF m_enLowPenetrationPrompt;	// 低穿透提示开关
	XRAY_LIB_ONOFF m_enConcernedMaterialPrompt;	// 可疑有机物识别开关
	XRAY_LIB_ONOFF m_enExplosivePrompt;	        // 爆炸物识别开关

	XRAYLIB_EXPLOSIVEDETECTION_MODE m_enDetectionMode; // 爆炸物检测方法

private:
	/**************************************************************************
	*                              new add
	***************************************************************************/
	/*曲线路径*/
	char PmmaMuTablePath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];
	char AlMuTablePath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];
	char FeMuTablePath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];

	MuTable PmmaMu;
	MuTable AlMu;
	MuTable FeMu;

	limit limitdata;
	SharedPara* m_pSharedPara;
	
	XMat m_matRegionSegStatusTemp; //存储状态和每个像素所属编号
	XMat m_matRegionSegNumTemp;    //存储分割后每个区域的数量
	XMat m_matZDataU8;             //16位原子序数转换到8位的临时内存
	XMat m_total_matZDataU8;

	//相似点邻域寻找范围
	int32_t m_nRegionXYPos_Concerned[8]; 
	
	//矩形框合并时扩边大小
	int32_t m_nConcernedRectExpendBorder; 
	
	CONCERNEDM m_stLowPene;   // 低穿灰度范围、联通区域点数阈值、宽度和高度方向阈值
	CONCERNEDM m_stConcernedM;// 可疑有机物识别Z值、灰度范围、联通区域点数阈值、宽度和高度方向阈值
	CONCERNEDM m_stDrug;      // 可疑毒品识别Z值，灰度范围，联通区域点数阈值，宽度及高度方向阈值
	CONCERNEDM m_stExplosive; // 可疑爆炸物识别Z值，灰度范围，联通区域点数阈值，宽度及高度方向阈值


	std::vector<int32_t> m_vecRegionlist;
	std::vector<int32_t>::iterator m_it;
	std::vector<XRAYLIB_BOX> m_RectRegonList;
	std::vector<XRAYLIB_BOX>::iterator m_rectit;

	// 分段检测相关变量
	std::vector<XRAYLIB_BOX> m_vecRectRegonSave[4];  // 内部存储的检测结果
	int32_t m_nLoopHeight;                                // 内部循环的检测图像高度
	int32_t m_nDetecFlag;                                 // 检测结束标志位

}; // AreaModule

#endif // _XSP_MOD_AREA_HPP_
