/**
*      @file xsp_mod_cali.hpp
*	   @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
*
*      @brief XSP图像校正模块，负责原始数据归一化校正、畸变校正、平铺校正、
*             高低能偏移校正、邻域缓存拼接等校正功能实现
*
*      @author wangtianshu
*      @date 2023/2/9
*
*      @note
*/


#ifndef _XSP_MOD_CALI_HPP_
#define _XSP_MOD_CALI_HPP_

#include "xsp_interface.hpp"
#include "xray_shared_para.hpp"
#include "utils/xray_time_keeper.h"
#include "isl_transform.hpp"
#include <chrono>

#define XSP_MODCALI_CORRECT_HEIGHT 200

/**************************************************************************************************
*                                      畸变校正结构体定义
***************************************************************************************************/
/**
* @brief 畸变校正表结构体
*
* @note XSP畸变校正表内存分布
*                          usPixelNumber(探测器像素数) 
*                             ___________
*                            |___________|-> idx 0 存储tbe中有效的像素个数         
*  usIndexPerPixel(exp 20)   |           |          
*  (连续内存方向)            |           |-> idx 1~10 存储该像素相关的原始图像像素坐标
*                            |___________| 
*                            |           |
*                            |           |-> idx 11~19 存储该像素相关的原始图像像素加权系数
*                            |___________|
*/
struct GeoCaliTable
{
	XRAYLIB_GEOCALI_HEADER hdr;
	double* table;
	int32_t nIdxRange; 						// 像素坐标索引范围
	const int32_t nValidPixelNum = 1;		// tbe中有效的像素个数
	const int32_t nRelatedIdxNum = 10; 		// 相关的原始图像像素坐标索引个数
	const int32_t nRelatedRatioNum = 9; 	// 相关的原始图像像素加权系数个数

	GeoCaliTable()
		: table(LIBXRAY_NULL)
	{
		memset(&hdr, 0, sizeof(XRAYLIB_GEOCALI_HEADER));
	}

	// @brief 传入探测器宽度，设置内存
	inline void SetMem(int32_t wid)
	{
		hdr.usPixelNumber = wid;
		hdr.usIndexPerPixel = 20; //当前校正表为20
		nIdxRange = wid;
	}

	// @brief 获取内存大小
	inline size_t Size()
	{
		return hdr.usPixelNumber * 
			hdr.usIndexPerPixel * sizeof(double);
	}

	// @brief 获取idx指针
	inline double* IdxPtr(int32_t wid) 
	{ 
		XSP_ASSERT(wid < hdr.usPixelNumber);
		return table + wid * hdr.usIndexPerPixel + nValidPixelNum;
	}

	// @brief 获取ratio指针
	inline double* RatioPtr(int32_t wid)
	{
		XSP_ASSERT(wid < hdr.usPixelNumber);
		return table + wid * hdr.usIndexPerPixel + nValidPixelNum + nRelatedIdxNum;
	}

	// @brief 获取有效坐标数
	inline int32_t IdxNum(int32_t wid)
	{
		XSP_ASSERT(wid < hdr.usPixelNumber);
		return (int32_t)table[wid * hdr.usIndexPerPixel];
	}

	// @brief 设置有效坐标数
	inline void SetIdxNum(int32_t wid, int32_t num)
	{
		XSP_ASSERT(wid < hdr.usPixelNumber);
		table[wid * hdr.usIndexPerPixel] = double(num);
		return;
	}
}; // struct GeoCaliTable

/***************************************************************************************************
*                                      平铺校正结构体定义
***************************************************************************************************/
/**
* @brief 平铺校正表结构体

* @note XSP平铺校正表内存分布
*                        usPixelNumber(探测器像素数)
*                               ___________
*                              |           |
*  usIndexPerPixel(exp 21)     |           |
*   (连续内存方向)             |           |-> 平铺校正存储21个校正加权系数
*                              |           |   坐标对应关系为当前idx的前后10个像素
*                              |           |   
*                              |           |
*                              |___________|
*/
struct FlatCaliTable
{
	XRAYLIB_GEOCALI_HEADER hdr;
	double* table;

	// @brief 传入探测器宽度，设置内存
	inline void SetMem(int32_t wid)
	{
		hdr.usPixelNumber = wid;
		hdr.usIndexPerPixel = 21; //当前校正表为21
	}

	// @brief 获取内存大小
	inline size_t Size()
	{
		return hdr.usPixelNumber *
			hdr.usIndexPerPixel * sizeof(double);
	}

	// @brief 获取idx指针
	inline double* Ptr(int32_t wid)
	{
		XSP_ASSERT(wid < hdr.usPixelNumber);
		return table + wid * hdr.usIndexPerPixel;
	}

}; // struct FlatCaliTable

/***************************************************************************************************
*                                      不一致性校正结构体定义
***************************************************************************************************/
// 几何校正表文件头
struct InconsistCaliHeader
{
	uint16_t uPixelNumber;    	 // 探测器像素数
	uint16_t uRefLevelNum;    	 // 表中每个像素维度使用的数据量
	uint16_t uCaliStartPixel;    // 不一致性校正表起始像素
	uint16_t uCaliEndPixel;  	 // 不一致性校正表结束像素（真正校正时不包含该像素）
};

// 不一致性校正表 存储四个经斜穿校正之后的低灰度校正值
struct InconsistCaliTable
{
	InconsistCaliHeader hdr;
	uint16_t* pTable;
	const int32_t nMaxRefLevelNum = 10; // 表中每个像素使用的最大参考灰度级别

	// @brief 传入探测器宽度，设置内存
	inline void SetMem(int32_t wid)
	{
		hdr.uPixelNumber = wid;
	}

	// @brief 获取内存大小
	inline size_t Size()
	{
		return nMaxRefLevelNum * hdr.uPixelNumber * sizeof(uint16_t);
	}

	// @brief 获取idx指针，级别越低，对应的灰度值越高
	inline uint16_t* Ptr(int32_t refLevel)
	{
		XSP_ASSERT(refLevel >= 0 && refLevel < nMaxRefLevelNum);
		return pTable + refLevel * hdr.uPixelNumber;
	}

}; // struct FlatCaliTable

/***************************************************************************************************
*                                    安检机成像光路几何定义
***************************************************************************************************/
#define DET_MAX_NUM 20

struct Vec2
{
	float32_t x;
	float32_t y;

	Vec2() : x(0.0f), y(0.0f) { }

	Vec2(float32_t x_, float32_t y_) : x(x_), y(y_) { }

	Vec2(const Vec2& v) : x(v.x), y(v.y) { }

	// 自定义赋值操作符
	Vec2& operator=(const Vec2& other) 
	{
		if (this != &other) 
		{
			x = other.x;
			y = other.y;
		}
		return *this;
	}
};

// @brief 探测器板卡位置
struct DetCoor
{
	Vec2 start;             // 探测器起始坐标
	Vec2 end;               // 探测器终止坐标
};

// @brief 安检机成像几何（射线源为坐标原点）
struct Geometry
{
	Geometry()
		: nDetPixels(64), SOD(1.0f), isMain(true),
		  Dets(), nDetNum(0) {}

	// @brief 板卡数目设置
	inline void SetDetNum(uint32_t n)
	{
		XSP_ASSERT(n <= DET_MAX_NUM);
		nDetNum = n;
		return;
	}

	// @brief 板卡坐标设置
	inline void SetDetCoor(Vec2 start, Vec2 end, uint32_t n)
	{
		XSP_ASSERT(n < nDetNum);
		Dets[n].start = start;
		Dets[n].end = end;
		return;
	}

	// @brief 板卡数目
	inline uint32_t GetDetNum()
	{
		return nDetNum;
	}

	// @brief 板卡坐标
	inline DetCoor GetDetCoor(uint32_t n)
	{
		XSP_ASSERT(n < nDetNum);
		return Dets[n];
	}

	uint32_t nDetPixels;            // 单个板卡探测器像素数
	float32_t SOD;                  // 射线源到皮带距离
	bool isMain;                // 主辅视角区分
private:
	DetCoor Dets[DET_MAX_NUM];  // 板卡探测器几何(最大20块板卡)
	uint32_t nDetNum;               // 板卡探测器数目
}; // struct Geometry

/***************************************************************************************************
*                                        相关结构体定义
***************************************************************************************************/
/* 条带上下扩边缓存相关结构体 */
typedef struct _XRAY_TBPAD_CACHE_PARA_
{
	int32_t    	nTpadHei;			// 上扩边像素高度
	int32_t    	nBpadHei;			// 下扩边像素高度
	int32_t    	nTBpadStripeHei;    // 包含上下扩边的条带像素高度

	int32_t     nWriteIndex;   		// 环形缓冲池的写入index
	int32_t     nReadIndex;    		// 环形缓冲池的读取index
	XMat 		mRingBuf;        	// 条带上下扩边缓存内存
	bool 		bClearTemp;			// 缓存清除标志位

	// 初始化函数
	void Init(uint8_t uDefalutValue = 0xFF)
	{
		nTpadHei = 0;
		nBpadHei = 0;
		nWriteIndex = 0;
		nReadIndex = 0;
		bClearTemp = false;
		mRingBuf.SetValue(uDefalutValue);
	}
}XRAY_TBPAD_CACHE_PARA;

/* 实时处理临时内存 */
struct RTCaliTemp
{
	XMat caliTemp0;            // 实时临时内0 
	XMat caliTemp1;            // 实时临时内1 
	XMat orginMean;	           // 冷热源临时内存 
};

/* 冷热源处理参数 */
typedef struct _TIME_COLDHOT_PARA_
{
	bool		bTimeNodeStatus;
	int32_t			nColdHotTik;
	TIME_STRUCT	TimeBegin;
	TIME_STRUCT	TimeEnd;
	long		ldOneTime;	
} TIME_COLDHOT_PARA;

/* 皮带边缘区域 */
typedef struct _BELT_EDGE_
{
	int32_t		nStart1;
	int32_t		nEnd1;
	int32_t     nStart2;
	int32_t		nEnd2;
} BELT_EDGE;

/***************************************************************************************************
*                                          校正功能模块
***************************************************************************************************/
/**
* @brief 校正功能模块
*/
class CaliModule : public BaseModule
{
public:
	CaliModule();

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

	/**@fn             Init
	*  @breif          初始化
	*  @param1         szPublicFileFolderName     [I]         公共文件夹名
	*  @param2         szPrivateFileFolderName    [I]         私有文件夹名
	*  @param3         szGeoCaliTableName         [I]         几何校正表名
	*  @param4         szFlatDetCaliTableName     [I]         平铺校正表名
	*  @param5         szGeometryName             [I]         几何光路文件名
	*  @param6         pPara                      [I]         全局通用参数
	*  @param7         nDetectorLen               [I]         探测器宽度
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT Init(const char* szPublicFileFolderName, 
		                  const char* szPrivateFileFolderName,
		                  const char* szGeoCaliTableName, 
		                  const char* szFlatDetCaliTableName,
		                  const char* szGeometryName,
		                  SharedPara* pPara, int32_t nDetectorLen);

	/**@fn             ManualUpdateCorrect
	*  @breif          手动更新满载本底（校正数据），清除条带缓存
	*  @param1         matHigh          [I]       高能满载本底
	*  @param2         matLow           [I]       低能满载本底
	*  @param3         enUpdateMode     [I]       更新模式
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT ManualUpdateCorrect(XMat& matHigh, XMat& matLow, 
		                               XRAYLIB_UPDATE_ZEROANDFULL_MODE enUpdateMode);


	/**@fn             AutoUpdateCorrect
	*  @breif          自动更新满载本底，不清除条带缓存
	*  @param1         matHigh          [I]       高能满载本底
	*  @param2         matLow           [I]       低能满载本底
	*  @param3         enUpdateMode     [I]       更新模式
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT AutoUpdateCorrect(XMat& matHigh, XMat& matLow,
		                                  XRAYLIB_UPDATE_ZEROANDFULL_MODE enUpdateMode);

	/**@fn             StripeTypeJudge
	*  @breif          脏图判断逻辑
	*  @param1         matLow           [I]       低能原始数据
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT StripeTypeJudge(XMat& matLow);

	/**@fn             CaliHImage
	*  @breif          高能数据归一化校正
	*  @param1         highOrgData        [I]         高能原始数据
	*  @param2         highCaliData       [O]         高能归一化校正数据
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT CaliHImage(XMat& highOrgData, XMat& highCaliData, XMat& highOriCaliData);

	/**@fn             CaliLImage
	*  @breif          高能数据归一化校正
	*  @param1         lowOrgData        [I]         低能原始数据
	*  @param2         lowCaliData       [O]         低能归一化校正数据
	*  @return         错误码
	*  @note   
	*/
	XRAY_LIB_HRESULT CaliLImage(XMat& lowOrgData, XMat& lowCaliData, XMat& lowOriCaliData);

	/**
	 * @brief 			缓存当前条带上下扩边
	 * @param 			matIn 			[I]		输入的矩阵对象，注意!!!!!!这里只处理matIn中的不含扩边部分!!!!!!
	 * @param 			tbPadCache 		[I/O]	条带扩边缓存参数对象
	 * @param 			bReachedLimit 	[O]		缓存条数数是否达到最低限制了，是否可以正常使用了
	 * @return 			返回完成扩边缓存变量类
	 */
	static XMat CacheTBpad(XMat& matIn, XRAY_TBPAD_CACHE_PARA& tbPadCache, bool* bReachedLimit = nullptr);

	/**
	 * @brief 			从当前的writeIndex位置往上更新矩阵
	 * @param 			matIn 			[I]		输入的矩阵对象，注意!!!!!!这里只处理matIn中的不含扩边部分!!!!!!
	 * @param 			tbPadCache 		[I/O]	条带扩边缓存参数对象
	 * @return 			错误码
	 */
	static XRAY_LIB_HRESULT ModTpadBuf(XMat& matIn, XRAY_TBPAD_CACHE_PARA& tbPadCache);

	/**
	 * @brief 设置条带扩边参数的方法
	 * 
	 * 该函数用于根据输入的矩阵和条带扩边参数设置缓存参数。
	 * 如果当前扩边参数与缓存中的参数一致且不需要清除临时数据，则直接返回。
	 * 否则，清空缓存内部所有数据并重新调整环形缓冲区的大小以适应新的参数。
	 * 
	 * @param stripeWid 送入缓存的条带高度
	 * @param stripeHei 送入缓存的条带宽度
	 * @param stripeType 条带类型
	 * @param tbPadHei 存储条带顶部和底部扩边高度的pair对象
	 * @param tbPadCache 条带扩边缓存参数对象
	 * @param PadDefaltValue 条带扩边默认值
	 * @return XRAY_LIB_HRESULT 成功时返回XRAY_LIB_OK，失败时返回相应的错误码
	 */
	static XRAY_LIB_HRESULT SetTBpad(int32_t stripeWid, int32_t stripeHei, int32_t stripeType, 
                                     std::pair<int32_t, int32_t>& tbPadHei, XRAY_TBPAD_CACHE_PARA& tbPadCache, uint8_t uPadDefaltValue = 0XFF);	

	/**@fn             GetCaliTable
	*  @breif          获取校正表数据
	*  @param1         pCaliTable        [I]         校正表地址
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT GetCaliTable(XRAYLIB_CALI_TABLE* pCaliTable);

	/**@fn             SetGeoCaliTableFacotr()
	*  @breif          设置畸变校正比例因子
	*  @return         错误码
	*  @note		   重新生成畸变校正表
	*/
	XRAY_LIB_HRESULT SetGeoCaliTableFacotr();

	XRAY_LIB_HRESULT GetGeoOffsetLut();

	/**@fn             GetCorrrctMatNoise
	*  @breif          获取满载本底校正数据噪声
	*  @return         错误码
	*  @note           对外部要求 严格按照先本底后满载的顺序调用
	*/
	XRAY_LIB_HRESULT GetCorrrctMatNoise(XMat& matIn, XRAYLIB_UPDATE_ZEROANDFULL_MODE nUpdateMode, XRAYLIB_ENERGY_MODE nEnergyType, float32_t* pCorrctNoise);

	XRAY_LIB_HRESULT BackgroundCali_BS(XMat& matIn, XMat& matOut, XMat& matCaliTbe, float32_t fAntiBS);

	/**@fn             CaliBSImageCPU
	*  @breif          背散射数据归一化校正CPU版本
	*  @param1         orgImg            [I]         原始数据
	*  @param2         caliImg           [O]         归一化校正数据
	*  @param3         fAntiBs			 [I]         背散比例因子1
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT CaliBSImageCPU(XMat& orgImg, XMat& caliImg, float32_t fAntiBS);

	/**@fn             GeoCali_BS_InterLinear
	*  @breif          背散射几何配准
	*  @param1         imageIn               [I]         校正前数据   
	*  @param2         imageOut              [O]         校正后数据
	*  @param4         bInverse              [I]         几何校正是否置反
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT GeoCali_BS_InterLinear(XMat& imageIn, XMat& imageOut, bool bInverse);


	/**@fn             PerformGeoCali
	*  @breif          进行几何校正
	*  @param1         imageIn               [I]         校正前数据   
	*  @param2         imageOut              [O]         校正后数据
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT ExecuteGeoCali(XMat& imageIn, XMat& imageOut);


private:
	/**************************************************************************
	*                             初始化相关
	***************************************************************************/
	/**@fn      InitFilePath
	* @brief    获取文件路径
	* @param1   szPublicFileFolderName           [I]     公共文件夹名
	* @param2   szPrivateFileFolderName          [I]     私有文件夹名
	* @param3   szGeoCaliTableName               [I]     几何校正表名
	* @param4   szFlatDetCaliTableName           [I]     平铺校正表名
	* @param5   szGeometryName                   [I]     几何光路文件名
	* @return   错误码
	* @note     
	*/
	XRAY_LIB_HRESULT InitFilePath(const char* szPublicFileFolderName, 
		                          const char* szPrivateFileFolderName,
		                          const char* szGeoCaliTableName, 
		                          const char* szFlatDetCaliTableName,
		                          const char* szGeometryName);

	/**@fn             InitTable
	*  @breif          读取校正表
	*  @param1         nDetectorLen          [I]         探测器宽度
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT InitTable(int32_t nDetectorLen);

	/**************************************************************************
	*                               配置文件读取
	***************************************************************************/
	/**@fn             ReadGeoCaliTable
	*  @breif          读取几何校正表
	*  @param1         szFileName					[I]          文件路径
	*  @param2         stTable                      [I/O]        几何校正表
	*  @param3         bGeoCaliCanUse               [I/O]        几何校正是否可用
	*  @param4         nDetectorWidth				[I]          探测器宽度
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT ReadGeoCaliTable(const char* szFileName, GeoCaliTable& stTable,int32_t nDetectorWidth);

	XRAY_LIB_HRESULT ReadGeoCaliTableBS(const char* szFileName, GeoCaliTable &stTable, int32_t nDetectorWidth);

	XRAY_LIB_HRESULT InitGeoMetricTbe(int32_t nDetectorLen);

		/**@fn         ReadInconsistencyCaliTable
	*  @breif          读取不一致校正表
	*  @param1         szFileName					[I]          文件路径
	*  @param2         stTable                      [I/O]        平铺校正表
	*  @param3         bFlatDetCaliCanUse           [I/O]        平铺校正是否可用
	*  @param4         nDetectorWidth				[I]          探测器宽度
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT ReadInconsistencyCaliTable(const char* szFileName, InconsistCaliTable& stTable, int32_t nDetectorWidth);

	/**@fn             ReadFlatDetCaliTable
	*  @breif          读取平铺校正表
	*  @param1         szFileName					[I]          文件路径
	*  @param2         stTable                      [I/O]        平铺校正表
	*  @param3         bFlatDetCaliCanUse           [I/O]        平铺校正是否可用
	*  @param4         nDetectorWidth				[I]          探测器宽度
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT ReadFlatDetCaliTable(const char* szFileName, FlatCaliTable& stTable, int32_t nDetectorWidth);
	
	/**@fn             ReadGeometryFromJson
	*  @breif          从json文件读取安检机光路几何
	*  @param1         szFileName					[I]          json文件路径
	*  @param2         stGeometry                   [I/O]        成像几何
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT ReadGeometryFromJson(const char* szFileName, Geometry& stGeom);

	// /**@fn             SaveCaliTable
	// *  @breif          保存归一化校正表
	// *  @param1         nDetectorWidth				[I]          探测器宽度
	// *  @return         错误码
	// *  @note
	// */
	// XRAY_LIB_HRESULT SaveCaliTable(int32_t nDetectorWidth);

	/**************************************************************************
	*                          校正功能组合函数
	***************************************************************************/
	/**@fn             CaliImage
	*  @breif          组合各校正功能的透射图像校正接口
	*  @param1         orgData           [I]          原始数据
	*  @param2		   caliOriData		 [O]		  OrgRaw校正数据
	*  @param3         caliData          [O]          校正数据
	*  @param4         tbeZero           [I]          本底数据
	*  @param5         tbeFull           [I/O]        满载数据
	*  @param6         judge             [I/O]        多条带缓存判断参数
	*  @param7         cache             [I/O]        多条带缓存结构体
	*  @param8         caliTemp          [I]          校正处理所需临时内存
	*  @param9         bInconsistCali    [I]          是否进行不一致校正
	*  @return         错误码
	*  @note           为解耦高低能校正，传入不同参数及内存
	*/
	XRAY_LIB_HRESULT CaliImage(XMat& orgData, XMat& caliData, XMat& caliOriData,
	                                   XMat& tbeZero, XMat& tbeFull,  RTCaliTemp& caliTemp, bool bInconsistCali = false);

	/**************************************************************************
	*                             校正相关函数
	***************************************************************************/
	/**@fn             CaliImageCPU
	*  @breif          高能数据归一化校正CPU版本
	*  @param1         orgImg            [I]         原始数据
	*  @param2         caliImg           [O]         归一化校正数据
	*  @param3         tbeZero           [I/O]       本底数据
	*  @param4         tbeFull           [I/O]       满载数据
	*  @param4         Mode				 [I]         能量模式
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT CaliImageCPU(XMat& orgImg, XMat& caliImg,
		                          XMat& tbeZero, XMat& tbeFull);

	XRAY_LIB_HRESULT InconsistCaliImage(XMat& orgImg, XMat& caliImg,
		                          XMat& tbeZero, XMat& tbeFull);
	/**@fn             DoseCaliImageCPU
	*  @breif          剂量波动校正
	*  @param1         orgImg					    [I]          原始数据
	*  @param2         caliImg					    [I/O]        校正数据
	*  @param3         tbeZero						[I]          本地数据
	*  @param4         tbeFull					    [I/O]        满载数据
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT DoseCaliImageCPU(XMat& orgImg, XMat& caliImg,
		                              XMat& tbeZero, XMat& tbeFull);

	/**@fn             CaliDoseRef
	*  @breif          计算剂量修正因子
	*  @param1         orgImg                       [I]           原始数据
	*  @param2         tbeZero                      [I/O]         本底数据
	*  @param3         tbeFull						[I]           满载数据
	*  @param4         fRefTop					    [I/O]         首端修正因子
	*  @param3         fRefDown						[I]           尾端修正因子
	*  @param4         nDetectorWidth			    [I/O]         探测器宽度
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT CaliDoseRef(uint16_t* orgImg, XMat &tbeZero, 
		                         XMat &tbeFull, float32_t& fRefTop, 
		                         float32_t& fRefDown, int32_t nDetectorWidth);

	/**@fn             RemovalBeltGrain
	*  @breif          去除皮带纹
	*  @param1         orgImg                   [I]         归一化后的数据
	*  @param2         beltTemp                 [O]         去除皮带纹后的数据
	*  @param3         nBackGroundThreshold     [I]         背景阈值
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT RemovalBeltGrain(XMat& orgImg, XMat &beltTemp, 
		                              int32_t nBackGroundThreshold);

	/**@fn             GeoCali
	*  @breif          几何校正
	*  @param1         imageIn               [I]         校正前数据   
	*  @param2         imageOut              [O]         校正后数据
	*  @param3         bInverse              [I]         几何校正是否置反
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT GeoCali(XMat& imageIn, XMat& imageOut, GeoCaliTable GeomTbe, bool bInverse);

	/**@fn             ResetGeoCaliTable
	*  @breif          几何校正表重新设置
	*  @param1         stGeom					    [I]          几何参数
	*  @param2         stTable                      [I/O]        几何校正表        
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT ResetGeoCaliTable(Geometry& stGeom, GeoCaliTable& stTable, int32_t nWidth);

	/**@fn             FlatDetCali
	*  @breif          平铺校正
	*  @param1         imageIn               [I]         校正前数据
	*  @param2         imageOut              [O]         校正后数据
	*  @param3         bInverse              [I]         平铺校正是否置反
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT FlatDetCali(XMat& imageIn, XMat& imageOut, bool bInverse);
	
	/**@fn             PixelWriggleCali
	*  @breif          错位校正
	*  @param1         ptrImageIn					[I]          归一化数据
	*  @param2         ptrImageOut					[I/O]        错位校正数据
	*  @param3         lambda						[I]          错位校正表
	*  @param4         direction					[I/O]        错位校正表方向
	*  @param5         mode							[I]          能量模式
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT PixelWriggleCali(XMat& ptrImageIn, XMat& ptrImageOut,  XMat& lambda, 
		                              XMat& direction, XRAYLIB_ENERGY_MODE mode);

	/**@fn             StretchImg
	*  @breif          拉伸校正
	*  @param1         imageIn           [I]          校正输入
	*  @param2         imageOut          [I/O]        校正输出
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT StretchImg(XMat& imageIn, XMat& imageOut);

	/**************************************************************************
	*                          模板更新相关函数
	***************************************************************************/
	/**@fn             AirDetec
	*  @breif          空气条带判断
	*  @param1         caliImg				 [I]          归一化数据
	*  @return         错误码
	*  @note
	*/
	bool AirDetec(XMat& caliImg);

	/**@fn             UpdateCaliTable
	*  @breif          更新空气模板
	*  @param1         orgImg           [I]          原始数据
	*  @param2         tbeFull          [I/O]        满载数据
	*  @param3         bNeedRTCali      [I]          是否需要更新满载
	*  @param4         fRTCaliRatio     [I]          满载更新比例
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT UpdateCaliTable(XMat& orgImg, XMat& tbeFull,
		                             bool bNeedRTCali, float32_t fRTCaliRatio);

	/**@fn             UpdateCaliTableSwitch
	*  @breif          冷热源校正空气模板
	*  @param1         orgImg           [I]          原始数据
	*  @param2         tbeFull          [I/O]        满载数据
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT UpdateCaliTableSwitch(XMat& orgImg, XMat& tbeFull, 
		                                   XMat& orgImgMean);

	/**@fn             UpdateCaliTableSwitch
	*  @breif          依据时间校正冷热源校正空气模板
	*  @param1         orgImg           [I]          原始数据
	*  @param2         tbeFull          [I/O]        满载数据
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT UpdateCaliTableSwitch(XMat& orgImg, XMat& tbeFull, 
		                                   XMat& orgImgMean, 
		                                   TIME_COLDHOT_PARA& timePara);

	/**@fn             pacNumDec
	*  @breif          计算传入三列数据小于归一化后小于nLimit的数量nNum
	*  @param1         ptrImg1          [I]          原始数据
	*  @param2         ptrImg2          [I]          原始数据
	*  @param3         ptrImg2          [I]          原始数据
	*  @param4         nWidth           [I]          探测器高度
	*  @param5         nLimit           [I]          阈值
	*  @param6         nNum             [I/O]        小于nLimit的数量
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT pacNumDec(uint16_t* ptrImg1, uint16_t* ptrImg2, uint16_t* ptrImg3, 
							   int32_t nWidth, int32_t nLimit, int32_t& nNum);

	/**@fn             pacNumDec
	*  @breif          计算传入三列数据小于归一化后小于nLimit的数量nNum
	*  @param1         ptrImg1          [I]          原始数据
	*  @param2         ptrImg2          [I]          原始数据
	*  @param3         ptrImg2          [I]          原始数据
	*  @param4         nWidth           [I]          探测器高度
	*  @param5         nLimit           [I]          阈值
	*  @param6         nLimit1          [I]          阈值1
	*  @param7         nNum             [I/O]        小于nLimit的数量
	*  @param8         nNum1            [I/O]        小于nLimit1的数量
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT pacNumDec(uint16_t* ptrImg1, uint16_t* ptrImg2, uint16_t* ptrImg3,
							   int32_t nWidth, int32_t nLimit, int32_t nLimit1, int32_t& nNum, int32_t& nNum1);

	/**@fn             UpdateCaliTable
	*  @breif          依据脏图判断结果校正空气模板
	*  @param1         orgImgH           [I]          原始高能数据
	*  @param2         tbeFullH          [I/O]        满载高能数据
	*  @param3         orgImgL           [I]          原始低能数据
	*  @param4         tbeFullL          [I/O]        满载低能数据
	*  @return         错误码
	*  @note
	*/
	XRAY_LIB_HRESULT UpdateCaliTable(XMat& orgImg, XMat& tbeFull);

public:
	/*外部可控制参数*/
	/*高低能原始数据邻域缓存处理结构体*/
	XRAY_TBPAD_CACHE_PARA m_stLPadCache;
	XRAY_TBPAD_CACHE_PARA m_stHPadCache;
	XRAY_TBPAD_CACHE_PARA m_stOriHPadCache;
	XRAY_TBPAD_CACHE_PARA m_stOriLPadCache;
	XRAY_TBPAD_CACHE_PARA m_stCaliLPadCache;
	XRAY_TBPAD_CACHE_PARA m_stCaliHPadCache;
    XRAY_TBPAD_CACHE_PARA m_stMaskCache;

	/* 背散射缓存结构体 */
	XRAY_TBPAD_CACHE_PARA m_stBSCache;	// 缓存一级流水输入的BackScatter数据

	/*几何校正是否置反*/
	bool m_bGeoCaliInverse;									// 畸变校正时，是否反转处理
	XMat m_matGeoDetectionPixelOffsetLut;                  	// AI路不做几何校正，输出一个探测器方向检测框的几何校正映射表

	bool m_bFlatDetCaliInverse;								// 平铺校正时，是否反转处理

	float32_t m_fRTUpdateAirRatio;							// 模板更新比例
	float32_t m_fStretchRatio;
	float32_t m_fSpeed;										// 皮带转速用于错位校正

	/*校正开关*/
	XRAY_LIB_ONOFF m_enGeoMertric;						    // 几何校正开关
	XRAY_LIB_ONOFF m_enFlatDetMertric;					    // 探测器平铺校正开关
	XRAY_LIB_ONOFF m_enColdAndHot;						    // 冷热源校正开关
	XRAY_LIB_ONOFF m_enDoseCaliMertric;					    // 剂量波动矫正开关
	bool m_bGeoCaliTableExist;								// .bin畸变校正文件是否存在
	bool m_bGeometryFigExist;								// .json几何光路图是否存在
	bool m_bGeoCaliCanUse;									// 表明畸变校正功能是否可以启用
	bool m_bFlatDetCaliCanUse;								// 表明平铺校正是否可以启用（无对应机型校正表无法启用）
	bool m_bInconsistencyCaliCanUse;						// 表明不一致性校正是否可以启用

	/*冷热源空气判读参数*/
	int32_t m_nCountThreshold;
	TIME_COLDHOT_PARA m_TimeColdHotParaH;
	TIME_COLDHOT_PARA m_TimeColdHotParaL;
	std::chrono::time_point<std::chrono::system_clock> m_TimeBegin;
	std::chrono::time_point<std::chrono::system_clock> m_TimeEnd;
	long long m_lBeginTime;
	long long m_lEndTime;

	/*上下置白参数*/
	int32_t m_nWhitenUp;
	int32_t m_nWhitenDown;

	/*皮带纹去除参数*/
	int32_t m_nBeltGrainLimit;

	/*k=0完全畸变校正，k=1无畸变校正，k=0.7介于中间结果*/
	float32_t m_fGeomFactor;  
	
	/*畸变校正系数调整探测板分界位置*/
	int32_t m_nDetecLocatiion;

	// 背散射模板校正使用
	int32_t m_AI_height_in;
	int32_t m_nUseAi;
	int32_t m_AI_nChannel;

	float32_t m_fAntiBS1_AI;
	float32_t m_fAntiBS2_AI;
	float32_t m_fAntiBS1_NLM;
	float32_t m_fAntiBS2_NLM;

	bool m_bGeoCaliBSInverse;

	// 背散射探测器方向实际有效数据宽度
	int32_t m_nImageWidthBS;

	bool m_bMirrorBS;

	// 背散射下采样比例
	float32_t m_fPulsBS;
	int32_t m_nwSkipUpBS;
	int32_t m_nwSkipDownBS;

	/* 背散校正模板 */
	XMat m_matCaliTableZeroBS;
	XMat m_matCaliTableAirBS;
	XMat m_matCaliTableAirBSSecCali_AI;

	// 背散射横条纹校正的纵向缩放系数
	float32_t m_fCompress;
	
private:
	/*全局共享参数*/
	SharedPara* m_pSharedPara;
	/**************************************************************************
	*                                 文件路径
	***************************************************************************/
	char m_szInconsistCaliTablePath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];
	char m_szGeoCaliTablePath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];
	char m_szGeometryPath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];
	char m_szFlatDetCaliTablePath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME];

	/*本底满载数据*/
	XMat m_matHighZero;            
	XMat m_matHighFull;
	XMat m_matLowZero;
	XMat m_matLowFull;

	/* 高能或低能的校正数据缓存 */
	XMat m_matCorrectTemp;

	/* 背散射几何校正表 */
	GeoCaliTable   m_caliBSGeoTbe;
						
	/*临时内存*/
	RTCaliTemp m_stCaliHTemp;
	RTCaliTemp m_stCaliLTemp;
	
	/*校正表*/
	GeoCaliTable   m_caliGeoTbe;						    //常规几何校正表

	FlatCaliTable  m_stCaliFlatTbe;							//平铺校正表	

	InconsistCaliTable m_stInconsistCaliTbe;				//不一致性校正表

	/* 几何光路定义 */
	Geometry m_stGeometry;                                  // 安检机成像光路几何定义
	XMat m_matGeomPixPhyLen;                                // 探测板像素实际物理距离
	XMat m_matGeomPixPhyPos;                                // 探测板像素实际物理坐标
	XMat m_matGeomPixCorrPos;                               // 探测板像素校正物理坐标
	//float32_t m_fGeomFactor;                                    // k=0完全畸变校正，k=1无畸变校正，k=0.7介于中间结果

	/*皮带边缘*/
	BELT_EDGE m_stBeltdge;                                  // 安检机成像光路皮带边缘

	//拉伸校正
	bool m_bNeedRTCali;										//条带是否判断未空气并进行模板更新
	bool m_bDefusionCanUse;								    //表明错位校正是否可以启用 

	//冷热源校正参数
	int32_t m_nTimeInterval;									//冷热源校正条带时间间隔阈值(ms)

	int m_nCacheNeighborLen;                                //用来记录第一次扩边的

	int32_t m_nPreType;											//记录前一个条带类型，空气为0，包裹为1，过渡（上升沿、下降沿）条带为2。
	int32_t m_nTopHeight;										//由于探测器安装位置，导致上方出现无效数据行数
	int32_t m_nDownHeight;										//由于探测器安装位置，导致下方出现无效数据行数
	float32_t m_fMaxAirViranceL;								//当前低能空气模板方差最大值

	// NLMeans内存校正使用, 200表示手动校正模板最大高度
	XMat m_matCaliV1;	// <uint16_t> width >= nDetectorWidth + 16 height >= 200 + 16 
	XMat m_matCaliV2;	// <uint16_t> width >= nDetectorWidth + 10 height >= 200 + 10
	XMat m_matCaliV2v;	// <uint16_t> width >= nDetectorWidth  height >= 200
	XMat m_matCaliSd;	// <uint32> width >= nDetectorWidth + 6  height >= 200 + 6
	XMat m_matCalifAvg;	// <float32_t> width >= nDetectorWidth  height >= 200
	XMat m_matCalifWeight;	// <float32_t> width >= nDetectorWidth  height >= 200
	XMat m_matCalifWMax;	// <float32_t> width >= nDetectorWidth  height >= 200



}; // class CaliModule

#endif // _XSP_MOD_CALI_HPP_
