/**
*      @file xsp_mod_dual.hpp
*	   @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
*
*      @brief XSP双能伪彩模块，负责R曲线读取、原子序数图生成、
*             原子序数图后处理、双能颜色表读取切换、RGB及YUV图像生成，
*             彩色图像翻转、反色等伪彩相关功能实现
*
*      @author wangtianshu
*      @date 2023/2/9
*
*      @note
*/

#ifndef _XSP_MOD_DUAL_HPP_
#define _XSP_MOD_DUAL_HPP_

#include "xsp_interface.hpp"
#include "xray_shared_para.hpp"
#include "isl_pipe.hpp"
#include <array>

/**************************************************************************************************
*                                       分类曲线结构体定义
***************************************************************************************************/
/**
* @brief 物质分类曲线查找表
*/
struct ZTable
{
	float64_t* pC;      					/* C 原子序数曲线数据指针 */
	float64_t* pH2O;      					/* H2O 原子序数曲线数据指针 */
	float64_t* pAl;     					/* Al原子序数曲线数据指针 */
	float64_t* pFe;     					/* Fe原子序数曲线数据指针 */
	float64_t* pDiffThr;     				/* R值对应的差异度阈值指针 */
	int32_t len;     						/* 查找表长度 */
	float64_t minTAV;						/* TAV-Zeff查找表最小索引所对应的最小衰减值 */	
	float64_t maxTAV;						/* TAV-Zeff查找表最大索引所对应的最大衰减值 */
	float64_t minResolvableTAV;  			/* 可分辨最小衰减值 */
	float64_t maxResolvableTAV;   			/* 可分辨最大衰减值 */

	ZTable()
		: pC(0), pH2O(0), pAl(0), pFe(0), pDiffThr(0), len(0) {}

	// @brief 基于低能灰度获取碳Z值
	inline float64_t CurveC(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pC[grayVal];
	}
	// @brief 基于低能灰度获取水Z值
	inline float64_t CurveH2O(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pH2O[grayVal];
	}

	// @brief 基于低能灰度获取铝Z值
	inline float64_t CurveAl(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pAl[grayVal];
	}

	// @brief 基于低能灰度获取铁Z值
	inline float64_t CurveFe(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pFe[grayVal];
	}

	// @brief 获取当前R值所对应的差异度阈值
	inline float64_t DiffThr(float64_t RVal)
	{
		uint16_t uRVal = (uint16_t)(RVal * len);
		return pDiffThr[uRVal];
	}

	void InitMem()
	{
		for(int i = 0; i < len; i++)
		{
			pC[i] = 0.0;
			pH2O[i] = 0.0;
			pAl[i] = 0.0;
			pFe[i] = 0.0;
			pDiffThr[i] = 0.0;
		}
	}
};

/**************************************************************************************************
*                                      原子序数补偿结构体定义
***************************************************************************************************/
/**
* @brief 原子序数补偿结构体
*/
struct CorrparTable
{
	int32_t     len;     /* 探测器像素数 */
	float32_t* pZcorrpar;      /* C 原子序数曲线数据指针 */
	

	CorrparTable()
		: len(0), pZcorrpar(0){}

	// @brief 基于低能灰度获取碳Z值
	inline double CurveZcorrpar(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pZcorrpar[grayVal];
	}
	
};


/**
* @brief 6色模式物质分类曲线查找表
*/
struct Z6Table
{
	int32_t     len;     /* 查找表长度，表示低能灰度级数 */
	double* pPE;     /* PE原子序数曲线数据指针 */
	double* pPOM;    /* POM原子序数曲线数据指针 */
	double* pPEAL;   /* PEAL原子序数曲线数据指针 */
	double* pFeAl;   /* FeAl原子序数曲线数据指针 */
	double* pAl;     /* Al原子序数曲线数据指针 */
	double* pFe;     /* Fe原子序数曲线数据指针 */

	Z6Table()
		: len(0), pPE(0), pPOM(0), pPEAL(0), 
		pFeAl(0), pAl(0), pFe(0) {}

	// @brief 基于低能灰度获取碳Z值
	inline double CurvePE(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pPE[grayVal];
	}

	// @brief 基于低能灰度获取碳Z值
	inline double CurvePOM(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pPOM[grayVal];
	}

	// @brief 基于低能灰度获取碳Z值
	inline double CurvePEAL(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pPEAL[grayVal];
	}

	// @brief 基于低能灰度获取碳Z值
	inline double CurveFeAl(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pFeAl[grayVal];
	}

	// @brief 基于低能灰度获取铝Z值
	inline double CurveAl(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pAl[grayVal];
	}

	// @brief 基于低能灰度获取铁Z值
	inline double CurveFe(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pFe[grayVal];
	}
};

/**************************************************************************************************
*                                       颜色表结构体定义
***************************************************************************************************/
/**
* @brief 颜色表信息头
*/
#pragma pack(4) 
typedef struct _COLOR_TBE_HEADER_
{
	int32_t hueNum;   /* 色调级数(exp 44) */
	int32_t yLevel;   /* 颜色亮度(exp 1024) */
	int32_t pixelSize;/* 颜色通道数(exp 3) */
}COLOR_TBE_HEADER;
#pragma pack() 

/**
 * @struct  ColorTable
 * @brief   颜色表结构体
 *
 *                     XSP颜色表文件结构
 *  COLOR_TBE_HEADER       yLevel
 *        _  __________________________________
 *       |_||                                  |
 *  hueNum  |            ColorTable            |
 *          |__________________________________|
 */
struct ColorTable
{
	COLOR_TBE_HEADER hdr;
	uint8_t* pColor;

    enum // 3色颜色表各区间定义
    {
        _3c_z0_gray,        // 无物质分类-灰色
        _3c_org_org,        // 有机物-橙色，organic-orange
        _3c_mix_green,      // 混合物-绿色
        _3c_ino_blue,       // 无机物-蓝色
        _3c_z789_magenta,   // Z789-洋红
        _3c_cls_num,        // 类别数，class number
    } _3c_cls; // 3-color classify

    static constexpr std::array<std::pair<int32_t, int32_t>, _3c_cls_num> z44_3c_map = 
        {{{0, 1}, {2, 11}, {12, 22}, {23, 41}, {42, 43}}}; // Z44-3色的原子序数对照表

    static constexpr std::array<std::pair<int32_t, int32_t>, _3c_cls_num> z128_3c_map = 
        {{{0, 1}, {2, 31}, {32, 66}, {67, 125}, {126, 127}}}; // Z128-3色的原子序数对照表

    // @brief 手动设置颜色表内存大小 
	inline void SetMem(int32_t hueNum, int32_t yLevel, int32_t pixelSize)
	{
		hdr.hueNum = hueNum;
		hdr.yLevel = yLevel;
		hdr.pixelSize = pixelSize;
	}

	std::array<int32_t, 44> szZMapOriginal = {0};
	std::array<int32_t, 44> szZMapUsed = {0};

	// @brief 获取颜色表内存大小
	inline size_t Size()
	{
		return hdr.hueNum * hdr.pixelSize * hdr.yLevel * sizeof(uint8_t);
	}

	// @brief 获取颜色表hue/yLevel坐标下指针
	inline uint8_t* Ptr(int32_t hue, int32_t yLevel)
	{
		XSP_ASSERT(hue < hdr.hueNum && yLevel < hdr.yLevel);
		return pColor + hue * hdr.yLevel * hdr.pixelSize + yLevel * hdr.pixelSize;
	}

	// @brief 获取灰度虚拟R值关系曲线表 仅单能色表使用
	inline uint8_t* CurvePtr()
	{
		return pColor + (hdr.hueNum - 1) * hdr.yLevel * hdr.pixelSize;
	}
};


#pragma pack(4) 
typedef struct _SINGLE_Z_TBE_HEADER_
{
	int32_t zLevel;   /* 原子序数最大值(exp 44) */
	int32_t yLevel;   /* 灰度范围(exp 1024) */
	int32_t perZSize;/* 单个原子序数所占字节大小(exp 1) */
}SINGLE_Z_TBE_HEADER;
#pragma pack() 
struct SingleZMapTbe
{
	SINGLE_Z_TBE_HEADER hdr;
	uint8_t* pZValue;

	// @brief 手动设置映射表内存大小 
	inline void SetMem(int32_t zLevel, int32_t yLevel, int32_t perZSize)
	{
		hdr.zLevel = zLevel;
		hdr.yLevel = yLevel;
		hdr.perZSize = perZSize;
	}

	// @brief 获取映射表内存大小
	inline size_t Size()
	{
		return hdr.perZSize * hdr.yLevel * sizeof(uint8_t);	// 1024灰度级映射原子序数, 只要1024 * sizeof(int8_t) 内存大小
	}

	// @brief 获取灰度虚拟R值关系曲线表 仅单能色表使用
	inline uint8_t* CurvePtr()
	{
		return pZValue;
	}
};

/**************************************************************************************************
*                                       其他结构体定义
***************************************************************************************************/
/* 灰度校正查找表 */
struct GrayCaliTable
{
	int32_t     len;           /* 查找表长度，表示高低能灰度级数 */
	uint16_t* pPMMACurveH;
	uint16_t* pPMMACurveL;
	uint16_t* pAlCurveH;
	uint16_t* pAlCurveL;
	uint16_t* pFeCurveH;
	uint16_t* pFeCurveL;

	GrayCaliTable()
		: len(0), pPMMACurveH(0), pPMMACurveL(0), 
		pAlCurveH(0),pAlCurveL(0), pFeCurveH(0), 
		pFeCurveL(0) {}

	inline uint16_t HCurvePMMA(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pPMMACurveH[grayVal];
	}

	inline uint16_t LCurvePMMA(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pPMMACurveL[grayVal];
	}

	inline uint16_t HCurveAl(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pAlCurveH[grayVal];
	}

	inline uint16_t LCurveAl(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pAlCurveL[grayVal];
	}

	inline uint16_t HCurveFe(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pFeCurveH[grayVal];
	}

	inline uint16_t LCurveFe(uint16_t grayVal)
	{
		XSP_ASSERT((int32_t)grayVal < len);
		return pFeCurveL[grayVal];
	}
};

/**************************************************************************************************
*                                      双能伪彩功能模块
***************************************************************************************************/
/**
* @brief 双能处理模块
*/
class DualModule : public BaseModule
{
public:
    DualModule();

    /**@fn      Init
    * @brief    双能模块初始化
    * @param1   szPublicFileFolderName  [I]     - 公共文件夹
    * @param2   szZTableName            [I]     - z曲线文件名
    * @param3   pPara                   [I]     - 公共参数
    * @param4   visual                  [I]     - 视角
    * @param5   detectorName            [I]     - 探测器名称
    * @return   错误码
    * @note     
    */
    XRAY_LIB_HRESULT Init(const char* szPublicFileFolderName,
                          const char* szZTableName,
                          const char* szZ6TableName,
                          SharedPara* pPara,
                          XRAY_LIB_VISUAL visual,
                          XRAY_DETECTORNAME detectorName,
                          XRAY_DEVICETYPE devicetype);

    /**@fn      Init
    * @brief    双能模块重新初始化（R曲线重载）
    * @param1   szPublicFileFolderName  [I]     - 公共文件夹
    * @param2   szZTableName            [I]     - z曲线文件名
    * @param3   visual                  [I]     - 视角
    * @return   错误码
    * @note     
    */
    XRAY_LIB_HRESULT ReInit(const char* szPublicFileFolderName,
                            const char* szZTableName,
                            const char* szZ6TableName,
                            XRAY_LIB_VISUAL visual);

    /**@fn      ZIdentify
    * @brief    对外统一接口原子序数计算
    * @param1   highData           [I]     - 高能数据
    * @param2   lowData            [I]     - 低能数据
    * @param3   zData              [O]     - 原子序数
    * @return   错误码
    * @note     与原始处理的Z值计算区分，使用不同临时内存
    *           避免线程冲突
    */
    XRAY_LIB_HRESULT ZIdentify(XMat& highData, XMat& lowData, XMat& zData);

    /**@fn      SingleZMapSimulate
    * @brief    单能图像模拟原子序数生成
    * @param1   xData              [I]     - 单能数据
    * @param2   zData              [O]     - 原子序数
    * @param3   bAi				   [I]	   - 是否为AI路, 1->是 0->否
    * @return   错误码
    * @note
    */
    XRAY_LIB_HRESULT SingleZMapSimulate(XMat& xData, XMat& zData, uint16_t bAi);

    /**
     * @fn      GetColorImageRgb
     * @brief   根据灰度图、原子序数图和当前选择的颜色模式，伪彩化为RGB颜色空间的图像
     * 
     * @param   [OUT] rgbOut RGB图像，格式为RGB888
     * @param   [IN] gray 融合灰度图
     * @param   [IN] z 原子序数图，尺寸需与gray相同
     * @param   [IN] wt 处理权重图，255为包裹区，0为背景区，其他为过渡区
     * @param   [IN] procWins 需处理的矩形区域集合
     * 
     * @return  XRAY_LIB_HRESULT 
     */
    XRAY_LIB_HRESULT GetColorImageRgb(XMat& rgbOut, XMat& gray, XMat& z, XMat& wt, std::vector<XRAYLIB_RECT>& procWins);

    /**@fn      GetColorImageAiYUV
    * @brief    获取Ai路彩色图像
    * @param1   merge              [I]     - 融合数据
    * @param2   zData              [I]     - 原子序数
    * @param3   low                [I]     - 低能数据
    * @param4   yTemp              [I/O]   - yuv临时内存
    * @param5   uvTemp             [I/O]   - yuv临时内存
    * @return   错误码
    * @note     ai路图像不随外部设置参数变化，单独实现
    */
    XRAY_LIB_HRESULT GetColorImageAiYuv(XMat& merge, XMat& zData, XMat& yTemp, XMat& uvTemp);

    /**@fn      ColorTbeSelected
    * @brief    颜色表选取
    * @return   错误码
    * @note
    */
    XRAY_LIB_HRESULT ColorTbeSelected();

	    /**
     * @fn      ColorModeSelected
     * @brief   通过重映射Z值，实现各种上色模式：灰彩、有机无机剔除、Z789、冷暖色调 
     * @note    颜色表的首行为灰度的映射专用，末行为Z789的映射专用
     * 
     * @return  XRAY_LIB_HRESULT 
     */
    XRAY_LIB_HRESULT ColorModeSelected();


    /**@fn      GrayRValueTune
    * @brief    指标模式下调整灰度值及原子序数值
    * @param1   merge              [I]     - 融合数据
    * @param2   zData              [I]     - 原子序数
    * @return   错误码
    * @note     argbTemp的高度大于等于原始条带不扩边高度
    */
    XRAY_LIB_HRESULT GrayRValueTune(XMat& merge, XMat& zData, XMat& low);

    // @brief    R曲线微调
	XRAY_LIB_HRESULT AdjustZTable(XRAYLIB_RCURVE_ADJUST_PARAMS xraylib_rcurveparams);

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

private:
	/**************************************************************************
	*                               初始化函数
	***************************************************************************/
	// @brief    内部路径清空为0
	XRAY_LIB_HRESULT ClearFilePath();

	// @brief    初始化ZTable路径
	XRAY_LIB_HRESULT InitZTableFilePath(const char* szPublicFileFolderName,
		                                const char* szZTableName,
		                                const char* szZ6TableName,
		                                XRAY_LIB_VISUAL visual);

	// @brief    初始化ColorTable路径
	XRAY_LIB_HRESULT InitColorTableFilePath(const char* szPublicFileFolderName);

	// @brief    初始化民航ColorTable路径
	XRAY_LIB_HRESULT InitColorTableCAFilePath(const char* szPublicFileFolderName);

	// @brief    R曲线读取初始化
	XRAY_LIB_HRESULT InitZTable();

	// @brief    颜色表读取初始化
	XRAY_LIB_HRESULT InitColorTable();

	// @brief    民航颜色表读取初始化
	XRAY_LIB_HRESULT InitColorTableCA();

	/**************************************************************************
	*                               配置文件读取
	***************************************************************************/
	/**@fn      ReadAutoZTable
	* @brief    从文件读取ZTable
	* @param1   szFileOriPath      [I]     	- Z表文件路径
	* @param2   szFileAutoPath       [I]     - 自动标定AI路Z表文件路径
	* @param3   szFileDispPath       [I]     - 自动标定显示路Z表文件路径
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT ReadZTable(const char* szFileOriName, const char* szFileAutoPath);

	XRAY_LIB_HRESULT ReadZTableElement(FILE* file, void* buffer, size_t elementSize, size_t elementCount, const char* elementName);

	/**@fn      ReadAutoZTable
	* @brief    从文件读取CorrparTable
	* @param1   szCorrparName    [I]     - 通道补偿表文件名
	* @param2   stZcorrparTbe             [O]     - 通道补偿表结构体
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT ReadZcorrparTable(const char* szCorrparName, CorrparTable& stZcorrparTbe);

	/**@fn      ReadZ6Table
	* @brief    从文件读取Z6Table
	* @param1   szFileName         [I]     - Z表文件名
	* @param2   stZTbe             [O]     - 6色Z表结构体
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT ReadZ6Table(const char* szFileName, Z6Table& stZTbe);

	/**@fn      ReadColorTable
	* @brief    从文件读取ColorTable
	* @param1   szFileName         [I]     - 颜色表文件名
	* @param2   stCTbe             [O]     - 颜色表结构体
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT ReadColorTable(const char* szFileName, ColorTable& stCTbe);


	/**@fn      ReadSingleZMapTbe
	* @brief    从文件读取单能Z值映射表
	* @param1   szFileName         [I]     - 单能Z值映射表文件名
	* @param2   stZMapTbe          [O]     - 单能Z值映射表结构体
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT ReadSingleZMapTbe(const char* szFileName, SingleZMapTbe& stZMapTbe);

	/**************************************************************************
	*                               三色原子序数计算
	***************************************************************************/
	/**@fn      ZIdentifyByLowValue
	* @brief    利用低能和R值表进行原子序数计算
	* @param1   highData           [I]     - 高能数据
	* @param2   lowData            [I]     - 低能数据
	* @param3   zData              [O]     - 原子序数
	* @param4   nPadLen            [I]     - 额外计算的扩边高度
	* @return   错误码
	* @note    
	*/
	XRAY_LIB_HRESULT Z8IdentifyByLow(XMat& highData, XMat& lowData, 
		XMat& zData, int32_t nPadLen);
	
	/**@fn      ZIdentifyByTAV
	* @brief    利用总衰减值进行原子序数计算
	* @param1   highData           [I]     - 高能数据
	* @param2   lowData            [I]     - 低能数据
	* @param3   zData              [O]     - 原子序数
	* @param4   nPadLen            [I]     - 额外计算的扩边高度
	* @return   错误码
	* @note    
	*/
	XRAY_LIB_HRESULT Z8IdentifyByTAV(XMat& highData, XMat& lowData, 
		                       XMat& zData, int32_t nPadLen);


	/**************************************************************************
	*                             6色原子序数计算
	***************************************************************************/
	/**@fn      ZIdentify
	* @brief    高低能Z值求解
	* @param1   highData           [I]     - 高能数据
	* @param2   lowData            [I]     - 低能数据
	* @param3   zData              [O]     - 原子序数
	* @param4   nPadLen            [I]     - 额外计算的扩边高度
	* @return   错误码
	* @note    
	*/
	XRAY_LIB_HRESULT Z8Identify6S(XMat& highData, XMat& lowData, 
		                         XMat& zData, int32_t nPadLen);

	/**************************************************************************
	*                             6色原子序数计算
	***************************************************************************/
	/**@fn      ZIdentify
	* @brief    高低能Z值求解
	* @param1   highData           [I]     - 高能数据
	* @param2   lowData            [I]     - 低能数据
	* @param3   zData              [O]     - 原子序数
	* @param4   nPadLen            [I]     - 额外计算的扩边高度
	* @return   错误码
	* @note    
	*/
	XRAY_LIB_HRESULT Z8IdentifyCA(XMat& highData, XMat& lowData, 
		                         XMat& zData, int32_t nPadLen);

	/**************************************************************************
	*                           原子序数16位计算
	***************************************************************************/
	/**@fn      Z16IdentifyByLow
	* @brief    高低能Z值求解
	* @param1   highData           [I]     - 高能数据
	* @param2   lowData            [I]     - 低能数据
	* @param3   zData              [O]     - 原子序数
	* @param4   nPadLen            [I]     - 额外计算的扩边高度
	* @return   错误码
	* @note    
	*/
	XRAY_LIB_HRESULT Z16IdentifyByLow(XMat& highData, XMat& lowData,
									XMat& zData, int32_t nPadLen);
	/**@fn      Z16IdentifyByTAV
	* @brief    利用总衰减值进行原子序数计算
	* @param1   highData           [I]     - 高能数据
	* @param2   lowData            [I]     - 低能数据
	* @param3   zData              [O]     - 原子序数
	* @param4   nPadLen            [I]     - 额外计算的扩边高度
	* @return   错误码
	* @note    
	*/
	XRAY_LIB_HRESULT Z16IdentifyByTAV(XMat& highData, XMat& lowData,
									XMat& zData, int32_t nPadLen);	

	/**@fn      ColorTableCopy
	* @brief    颜色表拷贝
	* @param1   colorTbeOut        [I]     - 输出颜色表
	* @param2   colorTbeIn         [I]     - 输入颜色表
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT ColorTableCopy(ColorTable& colorTbeOut, ColorTable& colorTbeIn);

	/**@fn      InitColorTableZMap
	* @brief    初始化原子序数映射表
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT InitColorTableZMap();

public:
	/**************************************************************************
	*                               外部控制参数
	***************************************************************************/
	XRAYLIB_COLOR_MODE           m_enColorMode;             // 颜色模式
	XRAYLIB_PENETRATION_MODE     m_enPenetrationMode;       // 高低穿模式
	XRAYLIB_INVERSE_MODE         m_enInverseMode;           // 反色模式
	XRAYLIB_SINGLECOLORTABLE_SEL m_enSingleColorTableSel;   // 单能表设置
	XRAYLIB_DUALCOLORTABLE_SEL   m_enDualColorTableSel;     // 双能表设置
	XRAYLIB_COLORSIMAGING_SEL    m_enColorImagingSel;       // 彩色成像选择

	XRAYLIB_MATERIAL_COLOR_PARAMS m_stTestbParams;                // 指标模型下调整参数

	/* 双能分辨参数 */
	int32_t                          m_nDualFilterKernelLength;       // 双能分辨滤波核大小,小于XRAY_LIB_MAX_FILTER_KERNEL_LENGTH
	float32_t                        m_fDualFilterRange;              // 双能分辨滤波值判断范围
	float32_t                        m_fMaterialAdjust;			      // 材料值调整系数
    int32_t                          m_nMixtureRangeDownAdj;          // 混合物原子序数下限值调整，影响有机物和混合物的过渡，正值偏有机物，负值偏混合物
    int32_t                          m_nMixtureRangeUpAdj;            // 混合物原子序数上限值调整，影响混合物和无机物的过渡，正值偏混合物，负值偏无机物
    int32_t                          m_nDualDistinguishGrayUp;        // 双能分辨上限
	int32_t                          m_nDualDistinguishGrayDown;      // 双能分辨下限
	int32_t                          m_nDualDistinguishNoneInorganic; // 双能分辨无机物上限
	int32_t                          m_nDualDistinguishNoneMixture;   // 双能分辨混合物下限
	/* 窗位调节 */
	int32_t                          m_nGrayMax;
	int32_t                          m_nGrayMin;
	int32_t                          m_nGrayLevel;

	int32_t                          m_nBrightnessAdjust;

	uint32_t                         m_nBKColorARGB;    // 默认背景颜色 | A 24~32bit | R 16~23bit | G 8~15bit | B 0~7bit |
	uint32_t						 m_nBKLuminance;    // 默认背景亮度[0, 255]

	bool                         m_bZ6CanUse;       // 六色模式是否可用

	float32_t						 m_fpeneMergeParm;  // 高穿正常融合比例
	float32_t						 m_fHpeneRatio;		// 高穿正常融合比例
	float32_t						 m_fLpeneRatio;		// 低穿正常融合比例

	XRAY_LIB_ONOFF               m_enCurveState;    	// 液体识别曲线状态
	XRAY_LIB_ONOFF               m_enZcorrparState;    	//通道补偿曲线状态
	XRAY_LIB_ONOFF               m_enTestAutoLE;  	 	// 欧标美标测试体自动局增开关
	isl::PipeChroma              m_islPipeChroma;

	bool                         m_bUseTAVZTable;    	// 是否启用总衰减值对应原子序数查找表
	int32_t m_nWhiteUpBs;					// 背散射上置白
	int32_t m_nWhiteDownBs;					// 背散射下置白

private:

    SharedPara* m_pSharedPara;
    XRAY_DEVICETYPE m_enDevicetype;                //机型用于区分原子序数位置修正
    XRAY_DETECTORNAME m_enDetectorName;                //探测板类型用于区分原子序数位置修正

    /**************************************************************************
    *                                 文件路径
    ***************************************************************************/
    char m_szDualColorPath[5][XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];      // 4类双能颜色表路径 + 伪彩颜色表路径
    char m_szDualColorAiPath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];       // ai路双能颜色表路径
    char m_szSingleZMapTbePath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];	   // 单能颜色表Z值映射曲线路径
    char m_szPresetZTablePath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];      // 预置的物质分类曲线路径
    char m_szPresetZ6TablePath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];     // 6色分类曲线路径
    char m_szAutoZTablePath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];        // 自动标定分类曲线路径
	char m_szCorrparZTablePath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];     // 自动补偿曲线路径
    char m_szDualZ6ColorPath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];       // 4类六色双能颜色表路径

    /**************************************************************************
    *                              分类曲线及颜色表
    ***************************************************************************/
    ZTable     		m_stZTable;                                 // 分类曲线
	ZTable     		m_stZTable_bak;                             // 分类曲线备份，防止曲线微调出现异常
	CorrparTable 	m_stCorrparTable;                         	// 原子序数补偿曲线
    ColorTable 		m_stColorTbeRgb[5];                         // RGB双能颜色表 
    ColorTable 		m_stColorTbe;							   	// 实际使用的RGB双能颜色表
    SingleZMapTbe 	m_stSingleZMapTbe;					   		// 单能Z值虚拟映射曲线查找表
    ColorTable 		m_stColorTbeAiYuv;                          // YUVAI路颜色表 
    Z6Table    		m_stZ6Table;                                // 六色分类曲线
    ColorTable 		m_stColorTbeZ6Rgb;                          // RGB六色双能颜色表 

    /**************************************************************************
    *                               临时内存
    ***************************************************************************/
    XMat m_matEntireZDataSelected;                         // 整屏颜色模式选取后的原子序数 条带整屏共用
    XMat m_matIslYuvOut;
    XMat m_matIslYuvTmp;

    /* 高低能灰度校正查找表，用于液体识别校正灰度 */
    // GrayCaliTable m_stGrayCaliTbe;
}; // DualModule

#endif //_XSP_MOD_DUAL_HPP_
