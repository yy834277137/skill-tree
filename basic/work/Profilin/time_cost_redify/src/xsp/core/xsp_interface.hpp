/**    
*      @file xsp_interface.hpp
*	   @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
*      @brief XSP图像处理纯虚基类
*
*      @author wangtianshu
*      @date 2023/2/7
*
*      @note
*/

#ifndef _XSP_INTERFACE_HPP_
#define _XSP_INTERFACE_HPP_

#include "libXRay_def.h"
#include "xsp_types.hpp"
#include "xmat.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>

/**
* @brief XSP调用基类接口
*/
class IXRayImageProcess
{
public:
	/**@fn      Init
	* @brief    算法库初始化
	* @param1   xraylib_pipeline_mode       [I]     - 流水线模式
	* @param2   nFilePathNameMode           [I]     - 绝对/相对路径模式
	* @param3   xraylib_deviceInfo          [I]     - 机型信息
	* @param4   xraylib_AIPara              [I]     - ai相关参数
	* @param5   memTab                      [I]     - 内存表数组
	* @param6   PublicFileFolderName        [I]     - 公用资源文件路径
	* @param7   PrivateFileFolderName       [I]     - 私有资源文件路径
	* @param8   ability                     [I]     - 算法库能力集
	* @return   错误码
	* @note     
	*/
	virtual XRAY_LIB_HRESULT Init(XRAYLIB_PIPELINE_MODE xraylib_pipeline_mode, 
		                          XRAYLIB_FILEPATH_MODE nFilePathNameMode, 
		                          XRAY_DEVICEINFO xraylib_deviceInfo, 
		                          XRAYLIB_AI_PARAM xraylib_AIPara, 
		                          XRAY_LIB_MEM_TAB* memTab, 
		                          const char* PublicFileFolderName, 
		                          const char* PrivateFileFolderName, 
		                          XRAY_LIB_ABILITY* ability) = 0;

	/**@fn      ReloadProfile
	* @brief    重新载入配置文件（目前用于重载R曲线）
	* @param1   xraylib_deviceInfo          [I]     - 机型信息
	* @return   错误码
	* @note     
	*/
	virtual XRAY_LIB_HRESULT ReloadProfile(XRAY_DEVICEINFO* xraylib_deviceInfo) = 0;

	/**@fn      GetRCurveName
	* @brief    获取R曲线名
	* @param1   xraylib_deviceInfo          [I]     - 机型信息
	* @param2   xraylib_rcurve_type         [I]     - R曲线类型设置
	* @param3   xraylib_rcurvename          [O]     - R曲线名
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT GetRCurveName(XRAY_DEVICEINFO* xraylib_deviceInfo, 
		                                   XRAY_RCURVE_TYPE xraylib_rcurve_type, 
		                                   char* xraylib_rcurvename) = 0;


	/**@fn      SetTestModeParams
	* @brief    指标模式设置参数
	* @param1   xraylib_testbparams         [I]     - 相关调整参数
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT SetTestModeParams(XRAYLIB_MATERIAL_COLOR_PARAMS  *xraylib_testbparams) = 0;

	/**@fn      SetRCurveAdjustParams
	* @brief    R曲线微调，颜色微调
	* @param1   xraylib_rcurveparams         [I]     - 相关调整参数
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT SetRCurveAdjustParams(XRAYLIB_RCURVE_ADJUST_PARAMS &xraylib_rcurveparams) = 0;

	/**@fn      SetMemTabSize
	* @brief    设置内存表内存大小(字节)
	* @param1   memTab                      [O]     - 内存表数组
	* @param2   nDetectorWidth              [I]     - 探测器宽度
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT SetMemTabSize(XRAY_LIB_MEM_TAB* memTab, 
		                                   XRAY_LIB_ABILITY* ability) = 0;

	/**@fn      SetLibXRayPrint
	* @brief    算法库打印开关
	* @param1   bPrint                      [I]     - 打印开关
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT SetLibXRayPrint(bool bPrint) = 0;

	/**@fn      GenerateCaliTable
	* @brief    更新满载本底并生成校正表
	* @param1   xData                       [I]     - 输入满载/本地数据
	* @param2   nUpdateMode                 [I]     - 0,更新本底; 1,更新空气
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT GenerateCaliTable(XRAYLIB_IMAGE& xData, 
		                                       XRAYLIB_UPDATE_ZEROANDFULL_MODE nUpdateMode) = 0;

	/**@fn      UpdateCaliMem
	* @brief    更新满载本底内存
	* @param1   xData                       [I]     - 输入满载/本地数据
	* @param2   nUpdateMode                 [I]     - 0,更新本底; 1,更新空气
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT UpdateCaliMem(XRAYLIB_IMAGE& xData,
		                                   XRAYLIB_UPDATE_ZEROANDFULL_MODE nUpdateMode) = 0;

	/**@fn      GetCaliTable
	* @brief    获取满载本底校正表
	* @param1   ptrCaliTable                [I/O]    - 校正表结构体
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT GetCaliTable(XRAYLIB_CALI_TABLE* ptrCaliTable) = 0;

	/**************************************
	*            实时处理相关
	***************************************/
	/**@fn      PipelineProcess
	* @brief    流水处理
	* @param1   pipe                        [I/O]    - 流水线结构体
	* @param2   index                       [I]      - 流水线级数
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT PipelineProcess(VOID* pipe, int32_t index) = 0;


	/**************************************
	*          回拉整图处理相关
	***************************************/

	/**@fn      CaliHLToMergeImage
	* @brief    高低能归一化及融合图像获取彩图
	* @param1   caliData                    [I]      - 归一化高低能数据(包括扩边)
	* @param2   merge                       [O]      - 融合图像输出(包括扩边)
	* @param3   ntop                        [O]      - 高度上界扩边
	* @param4   nbotm                       [O]      - 高度下界扩边
	* @return   错误码
	* @note     扩边内存用于默认增强处理使用
	*/
	virtual XRAY_LIB_HRESULT CalilhzMergeToColor(XRAYLIB_IMAGE& calilhz,
	                                             XRAYLIB_IMAGE& merge,
	                                             XRAYLIB_IMAGE& colorImg,
		                                         XRAYLIB_DUMPPROC_TIP_PARAM& p_xraylib_tip,
	                                             XRAYLIB_IMG_CHANNEL channel,
	                                             int32_t ntop, int32_t nbotm, int32_t bDescendOrder) = 0;

	virtual XRAY_LIB_HRESULT CaliBSToRGBImage(XRAYLIB_IMAGE& calilhz,
											  XRAYLIB_IMAGE& colorImg,
											  int32_t ntop, int32_t nbotm) = 0;


	/**@fn      GetProcessMerge
	* @brief    获取融合增强图像
	* @param1   calilhz                     [I]      - 归一化高低能数据
	* @param2   merge                       [O]      - 融合增强图像
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT GetProcessMerge(XRAYLIB_IMAGE& calilhz, 
		                                     XRAYLIB_IMAGE& merge, 
		                                     int ntop, int nbotm, int bDescendOrder) = 0;

	/**@fn      GetConcernedArea
	* @brief    可疑区域识别
	* @param1   caliData                    [I]     - 归一化数据输入
	* @param2   ConcerdMaterialM            [O]     - 可疑有机物区域
	* @param3   LowPeneArea                 [O]     - 低穿透区域
	* @param4   ExplosiveArea               [O]     - 爆炸物区域
	* @param5   DrugArea                    [O]     - 毒品区域
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT GetConcernedArea(XRAYLIB_IMAGE&        xraylib_img,
											  XRAYLIB_CONCERED_AREA *ConcerdMaterialM, 
											  XRAYLIB_CONCERED_AREA *LowPeneArea,
											  XRAYLIB_CONCERED_AREA *ExplosiveArea,
											  XRAYLIB_CONCERED_AREA *DrugArea) = 0;

	/**@fn      GetConcernedAreaPiecewise
	* @brief    可疑区域识别
	* @param1   caliData                    [I]     - 归一化数据输入
	* @param2   ConcerdMaterialM            [O]     - 可疑有机物区域
	* @param3   LowPeneArea                 [O]     - 低穿透区域
	* @param4   ExplosiveArea               [O]     - 爆炸物区域
	* @param5   DrugArea                    [O]     - 毒品区域
	* @param6   nDetecFlag                  [I]     - 检测标志位
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT GetConcernedAreaPiecewise(XRAYLIB_IMAGE&         xraylib_img,
											           XRAYLIB_CONCERED_AREA *ConcerdMaterialM, 
											           XRAYLIB_CONCERED_AREA *LowPeneArea,
											           XRAYLIB_CONCERED_AREA *ExplosiveArea,
											           XRAYLIB_CONCERED_AREA *DrugArea,
		                                               int32_t                    nDetecFlag) = 0;

	/**************************************
	*             TIP设置相关
	***************************************/

	/**@fn      SetTipImage
	* @brief    设置TIP图像
	* @param1   tipData                     [I]      - TIP归一化数据输入
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT SetTipImage(XRAYLIB_IMAGE& xraylib_tip_img, int32_t packageHeight, int32_t randomPosition) = 0;

	/**@fn      ForceStopTIP
	* @brief    强制停止当前正在进行的TIP插入
	* @param1   tipData                     [I]      - TIP归一化数据输入
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT ForceStopTIP() = 0;

	virtual XRAY_LIB_HRESULT GetGeoOffsetLut(uint16_t* p_geooffset_lut, int32_t p_geooffset_lut_width) = 0;

	virtual XRAY_LIB_HRESULT SetProcessDataDumpPrm(int32_t dataPoint, int32_t dumpCount, char* pDirName) = 0;

	/**************************************
	*            参数设置相关
	***************************************/

	/**@fn      SetImage
	* @brief    设置图像通道参数
	* @param1   stConfigImage               [I]     - 图像设置结构体
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT SetImage(XRAYLIB_CONFIG_IMAGE* stConfigImage) = 0;

	/**@fn      GetImage
	* @brief    获取图像通道参数
	* @param1   caliData                    [O]     - 图像设置结构体
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT GetImage(XRAYLIB_CONFIG_IMAGE* stConfigImage) = 0;

	/**@fn      SetPara
	* @brief    设置参数通道参数
	* @param1   stConfigImage               [I]     - 图像设置结构体
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT SetPara(XRAYLIB_CONFIG_PARAM* stConfigPara) = 0;

	/**@fn      GetPara
	* @brief    获取参数通道参数
	* @param1   stConfigImage               [I]     - 图像设置结构体
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT GetPara(XRAYLIB_CONFIG_PARAM* stConfigPara) = 0;

	/**@fn      Release
	* @brief    释放相关内存
	* @return   错误码
	* @note
	*/
	virtual void Release() = 0;

protected:
	/**************************************
	*            内存申请相关
	***************************************/
	/**@fn      GetMemSize
	* @brief    获取内存表所需内存大小(字节单位)
	* @param1   MemTab             [O]     - 内存表
	* @param2   nDetectorWidth     [I]     - 探测器宽度
	* @param3   nMaxHeightRealTime [I]     - 实时出图及回拉等分段状态下最大允许的列数
	* @param4   fResizeScale       [I]     - 最大支持的缩放比例
	* @return   错误码
	* @note     算法库所需内存由外部申请, 需提前计算所需内存大小
	*/
	virtual XRAY_LIB_HRESULT GetMemSize(XRAY_LIB_MEM_TAB &MemTab, 
		                                XRAY_LIB_ABILITY &ability) = 0;

	/**@fn      InitMemTab
	* @brief    内存表内存分配
	* @param1   MemTab             [I]     - 内存表
	* @return   错误码
	* @note     外部申请内存后, 调用该接口做算法库内部内存分配
	*/
	XRAY_LIB_HRESULT InitMemTab(XRAY_LIB_MEM_TAB &MemTab)
	{
		size_t nSkip = 0;
		for (int32_t i = 0; i < (int32_t)m_vecDat.size(); i++)
		{
			if (nSkip > MemTab.size)
			{
				return XRAY_LIB_MEM_ERROR;
			}
			*(m_vecDat[i].addr) = (uint8_t*)MemTab.base + nSkip;
			nSkip += m_vecDat[i].size;
		}
		return XRAY_LIB_OK;
	}

	/**@fn      XspMalloc
	* @brief    算法库自定义malloc接口
	* @param1   ppData             [I]     - 需要申请内存的指针地址
	* @param2   size               [I]     - 申请内存大小
	* @param3   MemTab             [O]     - 内存表
	* @return   void
	* @note     算法库内部内存申请调用该接口, 记录所需分配内存的所有指针
	*           地址及对应内存大小(字节)，内存表记录所需内存大小总和
	*/
	void XspMalloc(void** ppData, size_t size, XRAY_LIB_MEM_TAB &MemTab)
	{
		m_vecDat.push_back(Dat{ ppData, size });
		MemTab.size += (unsigned long)size;
	}

protected:
	/**
	* @brief 内存信息结构体
	*/
	struct Dat
	{
		void** addr; /* 指针地址 */
		size_t size; /* 内存字节大小 */
	};
	std::vector<Dat> m_vecDat; /* 内存信息数组 */
};

/**
* @brief 基础模块处理单元
* @note  算法库单元模块纯虚类接口，具体处理模块需继承该接口进行实现
*/
class BaseModule
{
public:
	BaseModule()
		: m_vecDat() {};

	/**@fn      Release
	* @brief    释放接口
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT Release() = 0;

	/**@fn      GetMemSize
	* @brief    获取内存表所需内存大小(字节单位)
	* @param1   MemTab             [O]     - 内存表
	* @param2   nDetectorWidth     [I]     - 探测器宽度
	* @param3   nMaxHeightRealTime [I]     - 实时出图及回拉等分段状态下最大允许的列数
	* @param4   fResizeScale       [I]     - 最大支持的缩放比例
	* @return   错误码
	* @note     算法库所需内存由外部申请, 需提前计算所需内存大小
	*/
	virtual XRAY_LIB_HRESULT GetMemSize(XRAY_LIB_MEM_TAB &MemTab, 
		                                XRAY_LIB_ABILITY &ability) = 0;

	/**@fn      InitMemTab
	* @brief    内存表内存分配
	* @param1   MemTab             [I]     - 内存表
	* @return   错误码
	* @note     外部申请内存后, 调用该接口做算法库内部内存分配
	*/
	XRAY_LIB_HRESULT InitMemTab(XRAY_LIB_MEM_TAB &MemTab)
	{
		size_t nSkip = 0;
		for (int32_t i = 0; i < (int32_t)m_vecDat.size(); i++)
		{
			if (nSkip > MemTab.size)
			{
				return XRAY_LIB_MEM_ERROR;
			}
			*(m_vecDat[i].addr) = (uint8_t*)MemTab.base + nSkip;
			nSkip += m_vecDat[i].size;
		}
		return XRAY_LIB_OK;
	}
protected:
	/**@fn      XspMalloc
	* @brief    算法库自定义malloc接口
	* @param1   ppData             [I]     - 需要申请内存的指针地址
	* @param2   size               [I]     - 申请内存大小
	* @param3   MemTab             [O]     - 内存表
	* @return   void
	* @note     算法库内部内存申请调用该接口, 记录所需分配内存的所有指针
	*           地址及对应内存大小(字节)，内存表记录所需内存大小总和
	*/
	void XspMalloc(void** ppData, size_t size, XRAY_LIB_MEM_TAB &MemTab)
	{
		m_vecDat.push_back(Dat{ ppData, size });
		MemTab.size += (unsigned long)size;
	}

	/**
	* @brief 内存信息结构体
	*/
	struct Dat
	{
		void** addr; /* 指针地址 */
		size_t size; /* 内存字节大小 */
	};
	std::vector<Dat> m_vecDat; /* 内存信息数组 */
};


#endif // _XSP_INTERFACE_HPP_