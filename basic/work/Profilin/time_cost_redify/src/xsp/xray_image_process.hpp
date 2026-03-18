/**
 *      @file xray_image_process.hpp
 *	   @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
 *
 *      @brief XSP图像处理接口实现类：该类仅负责实现参数获取设置，及基础的
 *             业务逻辑功能(TIP、包裹检测、实时处理、回拉处理)，具体功能
 *             细节（校正、双能伪彩、增强、区域检测等）不在该类中实现
 *
 *      @author wangtianshu
 *      @date 2023/2/7
 *
 *      @note
 */

#ifndef _XRAY_IMAGE_PROCESS_HPP_
#define _XRAY_IMAGE_PROCESS_HPP_

#include <map>
#include <queue>
#include <math.h>
#include "xsp_interface.hpp"
#include "xsp_dump.hpp"
#include "xsp_mod_cali.hpp"
#include "xsp_mod_dual.hpp"
#include "xsp_mod_area.hpp"
#include "xsp_mod_cnn.hpp"
#include "xsp_mod_imgproc.hpp"
#include "xsp_mod_tc_enhance.hpp"
#include "xray_shared_para.hpp"
#include "isl_log.hpp"

class XRayImageProcess : public IXRayImageProcess
{
public:
	XRayImageProcess();

	/**@fn      Init
	 * @brief    算法库初始化
	 * @param1   xraylib_pipeline_mode       [I]     - 流水线模式
	 * @param2   nFilePathNameMode           [I]     - 绝对/相对路径模式
	 * @param3   xraylib_deviceInfo          [I]     - 机型信息
	 * @param4   xraylib_AIPara              [I]     - ai相关参数
	 * @param5   memTab                      [I]     - 内存表数组
	 * @param6   PublicFileFolderName        [I]     - 公用资源文件路径
	 * @param7   PrivateFileFolderName       [I]     - 私有资源文件路径
	 * @param8   nImageWidth                 [I]     - 探测器宽度
	 * @return   错误码
	 * @note
	 */
	virtual XRAY_LIB_HRESULT Init(XRAYLIB_PIPELINE_MODE xraylib_pipeline_mode,
								  XRAYLIB_FILEPATH_MODE enFilePathNameMode,
								  XRAY_DEVICEINFO xraylib_deviceInfo,
								  XRAYLIB_AI_PARAM xraylib_AIPara,
								  XRAY_LIB_MEM_TAB *memTab,
								  const char *PublicFileFolderName,
								  const char *PrivateFileFolderName,
								  XRAY_LIB_ABILITY *ability);

	/**@fn      ReloadProfile
	 * @brief    重新载入配置文件（目前用于重载R曲线）
	 * @param1   xraylib_deviceInfo          [I]     - 机型信息
	 * @return   错误码
	 * @note
	 */
	virtual XRAY_LIB_HRESULT ReloadProfile(XRAY_DEVICEINFO *xraylib_deviceInfo);

	/**@fn      GetRCurveName
	 * @brief    获取R曲线名
	 * @param1   xraylib_deviceInfo          [I]     - 机型信息
	 * @param2   xraylib_rcurve_type         [I]     - R曲线类型设置
	 * @param3   xraylib_rcurvename          [O]     - R曲线名
	 * @return   错误码
	 * @note
	 */
	virtual XRAY_LIB_HRESULT GetRCurveName(XRAY_DEVICEINFO *xraylib_deviceInfo,
										   XRAY_RCURVE_TYPE xraylib_rcurve_type,
										   char *xraylib_rcurvename);

	/**@fn      SetTestModeParams
	 * @brief    指标模式设置参数
	 * @param1   xraylib_testbparams         [I]     - 相关调整参数
	 * @return   错误码
	 * @note
	 */
	virtual XRAY_LIB_HRESULT SetTestModeParams(XRAYLIB_MATERIAL_COLOR_PARAMS *xraylib_testbparams);

	/**@fn      SetRCurveAdjustParams
	 * @brief    微调R曲线的值
	 * @param1   xraylib_testbparams         [I]     - 相关调整参数
	 * @return   错误码
	 * @note
	 */
	virtual XRAY_LIB_HRESULT SetRCurveAdjustParams(XRAYLIB_RCURVE_ADJUST_PARAMS &xraylib_rcurveparams);

	/**@fn      GenerateCaliTable
	 * @brief    更新满载本底并生成校正表
	 * @param1   xData                       [I]     - 输入满载/本地数据
	 * @param2   nUpdateMode                 [I]     - 0,更新本底; 1,更新空气
	 * @return   错误码
	 * @note
	 */
	virtual XRAY_LIB_HRESULT GenerateCaliTable(XRAYLIB_IMAGE &xData,
											   XRAYLIB_UPDATE_ZEROANDFULL_MODE nUpdateMode);

	/**@fn      UpdateCaliMem
	 * @brief    更新满载本底内存
	 * @param1   xData                       [I]     - 输入满载/本地数据
	 * @param2   nUpdateMode                 [I]     - 0,更新本底; 1,更新空气
	 * @return   错误码
	 * @note
	 */
	virtual XRAY_LIB_HRESULT UpdateCaliMem(XRAYLIB_IMAGE &xData,
										   XRAYLIB_UPDATE_ZEROANDFULL_MODE nUpdateMode);

	// @brief    获取满载本底校正表
	virtual XRAY_LIB_HRESULT GetCaliTable(XRAYLIB_CALI_TABLE *ptrCaliTable);

	// @brief    算法库打印开关
	virtual XRAY_LIB_HRESULT SetLibXRayPrint(bool bPrint);

	// @brief    设置内存表内存大小(字节)
	virtual XRAY_LIB_HRESULT SetMemTabSize(XRAY_LIB_MEM_TAB *memTab, XRAY_LIB_ABILITY *ability);

	/**************************************
	 *            实时处理相关
	 ***************************************/
	// @brief    多级流水处理
	virtual XRAY_LIB_HRESULT PipelineProcess(VOID *pipe, int32_t index);

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
	virtual XRAY_LIB_HRESULT CalilhzMergeToColor(XRAYLIB_IMAGE &calilhz,
												 XRAYLIB_IMAGE &merge,
												 XRAYLIB_IMAGE &colorImg,
												 XRAYLIB_DUMPPROC_TIP_PARAM &p_xraylib_tip,
												 XRAYLIB_IMG_CHANNEL channel,
												 int32_t ntop, int32_t nbotm, int32_t bDescendOrder);

	/**@fn      GetProcessMerge
	* @brief    获取融合增强图像
	* @param1   calilh                      [I]      - 归一化高低能数据
	* @param2   merge                       [O]      - 融合增强图像
	* @return   错误码
	* @note
	*/
	virtual XRAY_LIB_HRESULT GetProcessMerge(XRAYLIB_IMAGE& calilh, 
		                                     XRAYLIB_IMAGE& merge, 
		                                     int32_t ntop, int32_t nbotm, int32_t bDescendOrder);

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
	virtual XRAY_LIB_HRESULT GetConcernedArea(XRAYLIB_IMAGE &xraylib_img,
											  XRAYLIB_CONCERED_AREA *ConcerdMaterialM,
											  XRAYLIB_CONCERED_AREA *LowPeneArea,
											  XRAYLIB_CONCERED_AREA *ExplosiveArea,
											  XRAYLIB_CONCERED_AREA *DrugArea);

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
	virtual XRAY_LIB_HRESULT GetConcernedAreaPiecewise(XRAYLIB_IMAGE &xraylib_img,
													   XRAYLIB_CONCERED_AREA *ConcerdMaterialM,
													   XRAYLIB_CONCERED_AREA *LowPeneArea,
													   XRAYLIB_CONCERED_AREA *ExplosiveArea,
													   XRAYLIB_CONCERED_AREA *DrugArea,
													   int32_t nDetecFlag);

	virtual XRAY_LIB_HRESULT GetGeoOffsetLut(uint16_t *p_geooffset_lut, int32_t p_geooffset_lut_width);

	virtual XRAY_LIB_HRESULT SetProcessDataDumpPrm(int32_t dataPoint, int32_t dumpCount, char *pDirName);

	/**************************************
	 *             TIP设置相关
	 ***************************************/
	// @brief    设置TIP图像, 开始插入TIP
	virtual XRAY_LIB_HRESULT SetTipImage(XRAYLIB_IMAGE& xraylib_tip_img, int32_t packageHeight, int32_t randomPosition);

	// @brief    强制停止当前正在进行的TIP插入
	virtual XRAY_LIB_HRESULT ForceStopTIP();

	/**************************************
	 *            参数设置相关
	 ***************************************/
	// @brief    设置图像通道参数
	virtual XRAY_LIB_HRESULT SetImage(XRAYLIB_CONFIG_IMAGE *stConfigImage);

	// @brief    获取图像通道参数
	virtual XRAY_LIB_HRESULT GetImage(XRAYLIB_CONFIG_IMAGE *stConfigImage);

	// @brief    设置参数通道参数
	virtual XRAY_LIB_HRESULT SetPara(XRAYLIB_CONFIG_PARAM *stConfigPara);

	// @brief    获取参数通道参数
	virtual XRAY_LIB_HRESULT GetPara(XRAYLIB_CONFIG_PARAM *stConfigPara);

	// @brief    释放相关内存
	virtual void Release();

protected:
	/**@fn      GetMemSize
	 * @brief    获取内存表所需内存大小(字节单位)
	 * @param1   MemTab             [O]     - 内存表
	 * @param2   nDetectorWidth     [I]     - 探测器宽度
	 * @return   错误码
	 * @note     算法库所需内存由外部申请, 需提前计算所需内存大小
	 */
	virtual XRAY_LIB_HRESULT GetMemSize(XRAY_LIB_MEM_TAB &MemTab, XRAY_LIB_ABILITY &ability);

	/**@fn      RegistParaSetMap
	 * @brief    注册改变参数map
	 * @return   void
	 * @note
	 */
	void RegistParaSetMap();

	/**@fn      RegistParaGetMap
	 * @brief    注册改变参数map
	 * @return   void
	 * @note
	 */
	void RegistParaGetMap();

	/**@fn      RegistImageSetMap
	 * @brief    注册图像处理map
	 * @return   void
	 * @note
	 */
	void RegistImageSetMap();

	/**@fn      RegistImageGetMap
	 * @brief    注册改变参数map
	 * @return   void
	 * @note
	 */
	void RegistImageGetMap();

	/**************************************
	 *             初始化相关
	 ***************************************/
	/**@fn      GetKeyNameByDeviceInfo
	 * @brief    通过设备信息构造查找键值
	 * @param1   xraylib_deviceInfo [I]     - 设备信息
	 * @param2   szCnnKey           [O]     - 模型键值
	 * @param3   szCnnKeyTmp        [O]     - 模型键值,该键值区分机型子类
	 * @param4   szDeviceKey        [O]     - 设备键值
	 * @return   错误码
	 * @note
	 */
	XRAY_LIB_HRESULT GetKeyNameByDeviceInfo(XRAY_DEVICEINFO &xraylib_deviceInfo,
											char *szCnnKey, char *szCnnKeyTmp,
											char *szDeviceKey);

	/**@fn      GetKeyNameByDeviceInfo
	 * @brief    通过设备信息构造查找键值
	 * @param1   xraylib_deviceInfo [I]     - 设备信息
	 * @param2   szDeviceKey        [O]     - 设备键值
	 * @return   错误码
	 * @note
	 */
	XRAY_LIB_HRESULT GetKeyNameByDeviceInfo(XRAY_DEVICEINFO &xraylib_deviceInfo,
											char *szDeviceKey);

	/**@fn      InitDeviceParaByIni
	 * @brief    通过设备ini文件初始化参数
	 * @param1   szDeviceIniPath    [I]     - 设备ini文件路径
	 * @param2   szDeviceKey        [I]     - 设备键值
	 * @return   错误码
	 * @note
	 */
	XRAY_LIB_HRESULT InitDeviceParaByIni(const char *szDeviceIniPath, const char *szDeviceKey);

	/**@fn      InitBSDeviceParaByIni
	* @brief    通过背散设备ini文件初始化参数
	* @param1   szDeviceIniPath    [I]     - 设备ini文件路径
	* @param2   szDeviceKey        [I]     - 设备键值
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT InitBSDeviceParaByIni(const char* szDeviceIniPath, const char* szDeviceKey);

	/**@fn      GetRCurvePathByIni
	 * @brief    通过设备ini文件获取R曲线文件所在路径
	 * @param1   szDeviceIniPath    [I]     - 设备ini文件路径
	 * @param2   szDeviceKey        [I]     - 设备键值
	 * @return   错误码
	 * @note
	 */
	XRAY_LIB_HRESULT GetRCurvePathByIni(const char *szDeviceIniPath, const char *szDeviceKey,
										char *szZTablePath,
										char *szMixZTablePath,
										bool &nGetMixCurveValid);

	/******************* 透射流水 *******************/
	XRAY_LIB_HRESULT PipelineProcess3A(XRAYLIB_PIPELINE_PARAM_MODE3A &pipe3A, int32_t index);

	XRAY_LIB_HRESULT PipeDual3A_PreProcess(XRAYLIB_PIPELINE_PARAM_MODE3A::_pipeline_3a_idx0_ &idx0);

	XRAY_LIB_HRESULT PipeDual3A_CnnProcess(XRAYLIB_PIPELINE_PARAM_MODE3A::_pipeline_3a_idx1_ &idx1);

	XRAY_LIB_HRESULT PipeDual3A_PostProcess(XRAYLIB_PIPELINE_PARAM_MODE3A::_pipeline_3a_idx2_ &idx2);

	/**************************************
	*        背散三级流水线实现
	***************************************/
	XRAY_LIB_HRESULT PipelineProcessBS3B(XRAYLIB_PIPELINE_PARAM_MODE3B& pipe3B, int32_t index);

	XRAY_LIB_HRESULT PipeBS3B_PreProcess(XRAYLIB_PIPELINE_PARAM_MODE3B::_pipeline_3b_idx0_& idx0);

	XRAY_LIB_HRESULT PipeBS3B_CnnProcess(XRAYLIB_PIPELINE_PARAM_MODE3B::_pipeline_3b_idx1_& idx1);

	XRAY_LIB_HRESULT PipeBS3B_PostProcess(XRAYLIB_PIPELINE_PARAM_MODE3B::_pipeline_3b_idx2_& idx2);

	/**************************************
	 *            透射整图处理
	 ***************************************/
    /**
     * @fn      Cali2GrayEnhance
     * @brief   校正后的高低能图像转默认增强后的融合灰度图 
     * @note    如果有TIP，插入TIP的流程应在此之前
     * 
     * @param   [OUT] matGray 默认增强后的融合灰度图，输出到matGray指定的Buffer中
     * @param   [IN] matLow 校正后的低能数据
     * @param   [IN] matHigh 校正后的高能数据，单能时为nullptr
     * @param   [IN] matMask 包裹Mask
     * @param   [IN] enhanceDir 默认增强方向，0-非法，即不做增强，1-从上往下顺序增强，2-从下往上逆序增强
     * 
     * @return  XRAY_LIB_HRESULT 
     */
    XRAY_LIB_HRESULT Cali2GrayEnhance(XMat& matGray, XMat& matLow, XMat* matHigh, XMat& matMask, int32_t enhanceDir);

    /**
     * @fn      GraySpecialEnhance
     * @brief   融合灰度图的特殊增强（边增、超增、局增）
     * 
     * @param   [IN] matLow 校正后的低能数据
     * @param   [IN] matHigh 校正后的高能数据，单能时为nullptr
     * @param   [IN] matGray 融合灰度图
     * 
     * @return  XMat 特殊增强后的融合灰度图，出现异常时会直接输出matGray，其他情况会返回一个全局对象
     */
    XMat GraySpecialEnhance(XMat& matLow, XMat* matHigh, XMat& matGray);

	/***************************************
	 *            背散整图处理
	 ****************************************/
	XRAY_LIB_HRESULT DualDenoiseToEnhance(XRAYLIB_IMAGE& calilhz, XRAYLIB_IMAGE& merge, 
	                                      int32_t ntop, int32_t nbotm, int32_t bDescendOrder);

	// @brief    根据缩放系数和实际条带尺寸计算缩放后尺寸
	XRAY_LIB_HRESULT UpdateResizePara(int32_t nWidth, int32_t nHeight);

	/**************************************
	 *               TIP插入
	 ***************************************/
	/**@fn      RealTimeTipImage
	 * @brief    条带tip插入
	 * @param1   caliH              [I/O]   - 输入高能 输出插入tip高能
	 * @param2   caliL              [I/O]   - 输入低能 输出插入tip低能
	 * @param3   zData              [I/O]   - 输入原子序数 输出插入tip原子序数
	 * @param4   merge              [I/O]   - 用于判断插入位置的数据
	 * @param5   TipStatus          [O]     - tip插入状态
	 * @param6   nTipedPosW         [O]     - tip插入位置
	 * @return   错误码
	 * @note
	 */
	XRAY_LIB_HRESULT RealTimeTipImage(XMat &caliH, XMat &caliL, XMat &zData, XMat &merge,
									  XRAYLIB_TIP_STATUS &TipStatus, int32_t &nTipedPosW);
	
	/**************************************
	*          民航九宫格TIP插入
	***************************************/
	/**@fn      RealTimeTipImage
	* @brief    条带tip插入
	* @param1   caliH              [I/O]   - 输入高能 输出插入tip高能
	* @param2   caliL              [I/O]   - 输入低能 输出插入tip低能
	* @param3   zData              [I/O]   - 输入原子序数 输出插入tip原子序数
	* @param4   merge              [I/O]   - 用于判断插入位置的数据
	* @param5   TipStatus          [O]     - tip插入状态
	* @param6   nTipedPosW         [O]     - tip插入位置
	* @param6   packpos            [O]     - 包裹分割结果
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT RealTimeTipImage_CA(XMat& caliH, XMat& caliL, XMat& zData, XMat& mask, XMat& merge,
			XRAYLIB_TIP_STATUS& TipStatus, int32_t& nTipedPosW, XRAYLIB_RTPROC_PACKAGE* packpos);

    /**@fn      RealTimeTipImage
	* @brief    tip数据条带探测器方向边界检测
	* @param1   caliH              [I]		- 输入tip低能
	* @param2   caliL              [I]		- 输入条带起始
	* @param3   zData              [I]		- 输入条带结束
	* @param4   merge              [I/O]	- 上边界
	* @param5   TipStatus          [O/O]	- 下边界
	* @return   错误码
	* @note
	*/
	XRAY_LIB_HRESULT DetTIPPackageWidth(XMat& TIPData, int32_t nhStart, int32_t nhLength, int32_t& ntopWidth, int32_t& nbottomWidth);

	XRAY_LIB_HRESULT DumpProcTipImage(XMat& caliL, XMat& caliH, XMat& zData, XMat& mask, XRAYLIB_DUMPPROC_TIP_PARAM &tipParams);

	/**************************************
	 *         原子序数16位转8位
	 ***************************************/
	XRAY_LIB_HRESULT ZU16ToU8(XMat &Z16, XMat &Z8);

	/**************************************
	 *     Ai结果与原始结果加权融合处理
	 ***************************************/
	XRAY_LIB_HRESULT MergeSingleAiOri(XMat &matAi, XMat &matOri, float32_t ratio);

	XRAY_LIB_HRESULT MergeDualAiOri(XMat &matAiHigh, XMat &matAiLow,
									XMat &matOriHigh, XMat &matOriLow, float32_t ratio);

	XRAY_LIB_HRESULT MergeDualAiOriFp32ByMask(XMat &matAiHigh, XMat &matAiLow,
											  XMat &matOriHigh, XMat &matOriLow, XMat &matFuseMask);

	XRAY_LIB_HRESULT GetColorImage(XRAYLIB_IMAGE& imgOut, XMat& gray, XMat& z, XMat& wt, 
                                   std::vector<XRAYLIB_RECT>& procWins, int32_t nStrideH, int32_t nStrideW);

	/**@fn      GetAiColorImage
	 * @brief    获取Ai路彩色图像（该路出图始终保持不变）
	 * @param1   merge              [I]     - 融合数据
	 * @param2   zData              [I]     - 原子序数
	 * @param3   low                [I]     - 低能数据
	 * @param4   colorImg           [I/O]   - Ai路彩图
	 * @param5   nColorWidthNeed    [I]     - 彩色图实际条带高度
	 * @param6   nStrideH           [I]     - 彩色图内存高度
	 * @param7   nStrideW           [I]     - 彩色图内存宽度
	 * @return   错误码
	 * @note
	 */
	XRAY_LIB_HRESULT GetAiColorImage(XMat &merge, XMat &zData, XRAYLIB_IMAGE &colorImg,
									 int32_t nColorWidthNeed, int32_t nStrideH, int32_t nStrideW);

	/**@fn      GetColorImageBS
	* @brief    获取背散图像
	* @param1   merge              [I]     - 融合数据
	* @param2   colorImg           [I/O]   - 背散伪彩图
	* @param3   nColorWidthNeed    [I]     - 彩色图实际条带高度
	* @param4   nStrideH           [I]     - 彩色图内存高度
	* @param5   nStrideW           [I]     - 彩色图内存宽度
	* @return   错误码
	* @note     
	*/
	XRAY_LIB_HRESULT GetColorImageBS(XMat& merge, XRAYLIB_IMAGE& colorImg, int32_t nColorWidthNeed, int32_t nStrideH, int32_t nStrideW);

	XRAY_LIB_HRESULT CaliBSToRGBImage(XRAYLIB_IMAGE& calilhz, XRAYLIB_IMAGE& colorImg, int32_t ntop, int32_t nbotm);

protected:
	/*********************************
	 *           功能子模块
	 **********************************/
	CaliModule m_modCali;
	DualModule m_modDual;
	AreaModule m_modArea;
	CnnModule m_modCnn;
	ImgProcModule m_modImgproc;
	TcProcModule m_modTcproc;

	isl::PipeComm m_islPipeComm;
	std::vector<BaseModule *> m_vecModPtr; // 子模块基类指针

	/*********************************
	 *           相关路径
	 **********************************/
	/* 路径 */
	char m_szPath[XRAY_LIB_MAX_PATH];				   // 临时路径存储
	char m_szPublicFileFolderName[XRAY_LIB_MAX_PATH];  // 公有文件路径
	char m_szPrivateFileFolderName[XRAY_LIB_MAX_PATH]; // 私有文件路径

	/* 文件名 */
	char m_szCnnKeyName[XRAY_LIB_MAX_NAME];		   // 模型文件ini键值
	char m_szCnnKeyNameSubType[XRAY_LIB_MAX_NAME]; // 模型文件ini键值 区分机型子类
	char m_szDeviceKeyName[XRAY_LIB_MAX_NAME];	   // 设备配置ini键值
	char m_szZTableName[XRAY_LIB_MAX_NAME];		   // R曲线文件名
	char m_szAutoZTableName[XRAY_LIB_MAX_NAME];	   // 自动颜色标定文件名
	char m_szZ6TableName[XRAY_LIB_MAX_NAME];	   // 6色R曲线文件名
	char m_szMixZTableName[XRAY_LIB_MAX_NAME];	   // 混合R曲线文件名
	char m_szGeoCaliName[XRAY_LIB_MAX_NAME];	   // 几何校正表文件名
	char m_szFlatDetCaliName[XRAY_LIB_MAX_NAME];   // 平铺校正表文件名
	char m_szGeometryName[XRAY_LIB_MAX_NAME];	   // 几何光路文件名

	/* 完整文件路径 */
	char m_szDevicePath[XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME]; // 设备配置ini文件路径

	/*********************************
	 *           共享参数
	 **********************************/
	SharedPara m_sharedPara;

	/*********************************
	 *           业务参数
	 **********************************/
	int32_t m_nDetectorWidth;		 // 探测器实际宽度
	int32_t m_nMaxHeightRealTime; 	 // 最大条带高度
	float32_t m_fMaxResizeFactor; 	 // 最大缩放系数

	float32_t m_fHeightResizeFactor; // 高度缩放因子
	float32_t m_fWidthResizeFactor;	 // 宽度缩放因子
	int32_t m_nResizeHeight;		 // 缩放后条带高度(带扩边)
	int32_t m_nResizeWidth;			 // 缩放后条带宽度
	int32_t m_nResizePadLen;		 // 缩放后扩边长度 流水线模式下使用
	int32_t m_nRTHeight;			 // 安检机实时处理条带高度，用于依据高度切换自研模型
	int32_t m_nPadLen;				 // 实际扩边高度 <= XRAY_LIB_MAX_FILTER_KERNEL_LENGTH

	float32_t m_fDenoiseSigma;	// dsp下发后的降噪加权系数(背散中用来在前级对NLMeans数据做控制, 配合增强系数生成指标模式)
	float32_t m_fSigmaDn;		// 降噪模型中ai或平滑处理数据与原始数据加权系数
	float32_t m_fSigmaSr;		// 超分模型中ai或平滑处理数据与原始数据加权系数
	int32_t m_AiEdgeFactor; 	// AIMask边缘系数，0-100，越小代表检测边缘清晰物体，越大代表检测边缘模糊物体

	float32_t m_fSpeed;			  // 安检机当前条带速度，用于高低能相关偏移计算
	XRAY_SPEEDGEAR m_enSpeedGear; // 安检机实时处理条带高度，用于依据高度切换研究院模型
	bool m_bMixCurveValid;		  // 安检机混合曲线是否存在
	bool m_hasPackage;			  // 包裹分割结果，包裹是否存在，用于写入包裹上下界的判断

	XRAYLIB_PIPELINE_MODE m_enPipelineMode; // 流水线模式
	XRAYLIB_SCAN_MODE m_enScanMode;			// 扫描模式

	XRAYLIB_BSIMAGE_MODE m_enumBSImageMode;				// 背散射模式

	XRAYLIB_DEFAULT_ENHANCE m_enDefaultEnhanceJudge; // 外部的默认增强判断
	XRAY_LIB_VISUAL m_enVisual;						 // 机型视角

	/* 彩图出图旋转状态 */
	XRAYLIB_IMG_ROTATE m_enRotate;		 // 出图旋转状态(不区分实时回拉)
	XRAYLIB_IMG_ROTATE m_enRotateRT;	 // 实时旋转状态记录
	XRAYLIB_IMG_ROTATE m_enRotateEntire; // 回拉旋转状态记录
	XRAYLIB_IMG_MIRROR m_enMirror;		 // 镜像设置

	/* TIP相关 */
	int32_t m_nNeedTip;		   // 0-不需要插入TIP，1-需要插入TIP
	int32_t m_nTipHeight;	   // TIP总高度
	int32_t m_nTipWidth;	   // TIP总宽度
	int32_t m_nTipedHeight;	   // 已插入的TIP高度
	int32_t m_nTipWidthStart;  // 探测器方向上插入起始位置，每次setTip完归零。第一次插入时随机取位置
	int32_t m_nTipStripeStart; // 包裹条带标志
	int32_t					  m_nPackageHeight;			   // 待插入包裹长度
	int32_t					  m_nPackageHeightIn;		   // 待插入包裹长度,固定不变
	int32_t					  m_nCount;					   // TIP条带标志
	int32_t					  m_nRandomPosition;           // 九宫格随机位置
	std::vector<int32_t>      m_vecCoordinates;			   // 九宫格位置对应坐标
	/* TIP双视角共有参数 */
	static int32_t			  m_SnTipHeightRand;		   //Tip过包方向随机数，主辅视角保持一致
	static bool			  m_SbMainReady;			   //主视角包裹可插入标志
	static bool			  m_SbPackReady;			   //Tip执行标志，主辅视角均为可插入包裹
	static int32_t			  m_SbPackHeightRemian;		   //Tip剩余可插入长度
	static XRAYLIB_TIP_STATUS m_SeStatus;              // Tip状态，仅返回是否清除tip信息（主辅视角需同步）

    /* 条带信息 */
    std::deque<XRAYLIB_FLAG> m_queFlag;                     // 条带标志队列，透传应用的条带标志
    std::deque<XRAYLIB_RTPROC_PACKAGE> quePack;             // 条带中包裹区域坐标
    std::deque<std::vector<isl::Rect<int32_t>>> queProcWt;  // 条带中处理权重非0的区域

    // 低穿不同灵敏度下参数
	struct LOWPENESENS
	{
		int32_t lowPeneMin;
		int32_t LowGrayMin;

		int32_t lowPeneMid;
		int32_t LowGrayMid;

		int32_t lowPeneMax;
		int32_t LowGrayMax;
	};
	LOWPENESENS m_nLowPenesensParam;

	std::vector<isl::px_intp_t> intpCnn2DstHor, intpCnn2DstVer; /* CNN输出图像尺寸 缩放至 最终输出（应用设置）的尺寸 */
	std::vector<isl::px_intp_t> intpSrc2DstHor, intpSrc2DstVer; /* 原始输入图像尺寸 缩放至 最终输出（应用设置）的尺寸 */
	std::vector<isl::px_intp_t> intpSrc2CnnHor, intpSrc2CnnVer; /* 原始输入图像尺寸 缩放至 CNN输入图像尺寸 */
	std::vector<isl::px_intp_t> intpZSrc2DstHor, intpZSrc2DstVer; /* 原始输入图像尺寸 缩放至 CNN输入图像尺寸 */
	XMat m_mPreLanczosTemp; 	// 用于缩放的临时内存，仅在前处理流水中使用
	XMat m_mCnnLanczosTemp;		// 用于缩放的临时内存，仅在CNN流水中使用
	XMat m_mPostLanczosTemp;	// 用于缩放的临时内存，仅在后处理流水中使用

	/* 条带内存 */
	XMat m_matHTemp;        // 条带原始尺寸高能
	XMat m_matLTemp;        // 条带原始尺寸低能

	XMat m_matOriGeoHigh; 	// 原始条带高能几何校正
	XMat m_matOriGeomLow;	// 原始条带低能几何校正

	XMat m_matOriHTemp;		// 条带原始尺寸Ori高能
	XMat m_matOriLTemp;		// 条带原始尺寸Ori低能
	XMat m_matNLMeansTmp;	// NLMeans处理中间数据
	XMat m_matAiXspTmp;		// Aixsp处理中间数据
	XMat m_matNLMeansOut;	// NLMeans输出数据
	XMat m_matAiXspOut;		// Aixsp输出数据
	XMat m_matNLMeansResizeOut;	// NLMeans缩放后输出数据
	XMat m_matAiXspResizeOut;	// Aixsp缩放后输出数据
	XMat m_matBSTmp;		// 背散缓存输出数据

	XMat m_matResizeLow;	// 条带缩放后的低能
	XMat m_matResizeHigh;	// 条带缩放后的高能
	XMat m_matResizeZValue; // 条带缩放后的原子序数

	XMat m_matNeighborBs0;  // 背散带扩边临时内存
	XMat m_matNeighborBs1;  // 背散带扩边临时内存

	XMat m_matMerge;			// 条带后处理中双能融合图像或单能图像内存
	XMat m_matProcWt;			// 条带后处理中前景处理权重内存
	XMat m_matDefaultEnhance;	// 条带后处理中默认增强内存
	XMat m_matSpecialEnhance;	// 条带后处理中特殊增强内存

    XRAY_TBPAD_CACHE_PARA procWtCache;

    /* 整图内存 */
	XMat m_matResizeHighPad;    // 条带缩放后的高能
	XMat m_matResizeLowPad;	    // 条带缩放后的低能
	XMat m_matResizeMergePad;   // 条带双能融合图像或单能处理图像临时内存

	XMat m_matEntireZValue;	    // 整图处理原子序数
	XMat m_matEntireMerge;	    // 整图处理融合图像
	XMat m_matEntireEnhance;    // 整图处理增强图像
	XMat m_matEntireProcWt;     // 整图处理的权重，255为包裹区，全处理，0为空白区，不处理，其他为平滑区
	XMat m_matEntireMask;       // 包裹Mask区域

	/* TIP相关 */
	XMat m_matTipHigh;	 // TIP高能数据
	XMat m_matTipLow;	 // TIP低能数据
	XMat m_matTipZValue; // TIP原子序数

	XMat m_matTipedHigh;   // 插入TIP的部分高能数据
	XMat m_matTipedLow;	   // 插入TIP的部分低能数据
	XMat m_matTipedZValue; // 插入TIP的原子序数
	XMat m_matTipedMerge;  // 插入TIP的融合图像

	// NLMeans内存校正使用, 200表示手动校正模板最大高度
	XMat m_matV1;	// <uint16_t> width >= nDetectorWidth + 16 height >= 200 + 16 
	XMat m_matV2;	// <uint16_t> width >= nDetectorWidth + 10 height >= 200 + 10
	XMat m_matV2v;	// <uint16_t> width >= nDetectorWidth  height >= 200
	XMat m_matSd;	// <uint32> width >= nDetectorWidth + 6  height >= 200 + 6
	XMat m_matfAvg;	// <float32_t> width >= nDetectorWidth  height >= 200
	XMat m_matfWeight;	// <float32_t> width >= nDetectorWidth  height >= 200
	XMat m_matfWMax;	// <float32_t> width >= nDetectorWidth  height >= 200

	/* 手动校正或其他异步操作等待数据处理结束信号 */ 
	std::atomic<bool> m_AllowSyncOperation;

	/* 彩图相关 */
	XMat m_matEntireColorTemp; // 整屏与条带处理的argb、rgb、yuv上色的临时共用内存(Disp与Ai路共用)

	typedef XRAY_LIB_HRESULT (XRayImageProcess::*ImageFunc)(XRAYLIB_CONFIG_IMAGE *stConfigImage);
	typedef XRAY_LIB_HRESULT (XRayImageProcess::*ParaFunc)(XRAYLIB_CONFIG_PARAM *stConfigPara);

	std::map<XRAYLIB_CONFIG_IMAGE_KEY, ImageFunc> m_mapImageSet;
	std::map<XRAYLIB_CONFIG_PARAM_KEY, ParaFunc> m_mapParaSet;
	std::map<XRAYLIB_CONFIG_IMAGE_KEY, ImageFunc> m_mapImageGet;
	std::map<XRAYLIB_CONFIG_PARAM_KEY, ParaFunc> m_mapParaGet;

	/******************************************************************************************
	 *                                      图像状态设置
	 *******************************************************************************************/
	XRAY_LIB_HRESULT SetEnergyMode(XRAYLIB_CONFIG_IMAGE *stPara);		   // 能量设置
	XRAY_LIB_HRESULT SetColorMode(XRAYLIB_CONFIG_IMAGE *stPara);		   // 颜色设置
	XRAY_LIB_HRESULT SetPenetrationMode(XRAYLIB_CONFIG_IMAGE *stPara);	   // 高低穿设置
	XRAY_LIB_HRESULT SetEnahnceMode(XRAYLIB_CONFIG_IMAGE *stPara);		   // 增强设置
	XRAY_LIB_HRESULT SetInverseMode(XRAYLIB_CONFIG_IMAGE *stPara);		   // 反色设置
	XRAY_LIB_HRESULT SetGrayLevel(XRAYLIB_CONFIG_IMAGE *stPara);		   // 灰度级数设置
	XRAY_LIB_HRESULT SetBrightness(XRAYLIB_CONFIG_IMAGE *stPara);		   // 亮度设置
	XRAY_LIB_HRESULT SetSingleColorTableSel(XRAYLIB_CONFIG_IMAGE *stPara); // 单能色表设置
	XRAY_LIB_HRESULT SetDualColorTableSel(XRAYLIB_CONFIG_IMAGE *stPara);   // 双能色表设置
	XRAY_LIB_HRESULT SetColorImagingSel(XRAYLIB_CONFIG_IMAGE *stPara);	   // 三色六色颜色设置

	/******************************************************************************************
	 *                                        参数设置
	 *******************************************************************************************/
	/* 图像校正参数 */
	XRAY_LIB_HRESULT SetWhitenUpNum(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetWhitenDownNum(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetBackgroundThreshold(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetBackgroundGray(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetRTUpdateAirRatio(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetEmptyThresholdH(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetEmptyThresholdL(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetRTUpdateLowGrayRatio(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetPackageThreshold(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetLibXRayDefaultImageWidth(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetColdHotCaliThreshold(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetBeltGrainLimit(XRAYLIB_CONFIG_PARAM *stPara);
	/* 图像处理参数	*/
	XRAY_LIB_HRESULT SetDualFilterKernelLength(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetDualFilterRange(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetDualDistinguishGrayUp(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetDualDistinguishGrayDown(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetDualDistinguishNoneInorganic(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetDefaultEnhance(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetDefaultEnhanceIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetEdgeEnhanceIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetSuperEnhanceIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetSpecialEnhanceThresholdUp(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetSpecialEnhanceThresholdDown(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetSpecialEnhanceStretchGrayUp(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetSpecialEnhanceStretchGrayDown(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetCaliLowGrayThreshold(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetDenoisingIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetCompositiveRatio(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetDenoisingMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetAntialiasingMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetAIDeviceMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetResizeHeightFactor(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetResizeWidthFactor(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetFusionMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetBackgoundColor(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetStretchRatio(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetGammaMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetGammaIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetSigmaNoise(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetHighPeneRatio(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetLowPeneRatio(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetMergeBaseLine(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetTestMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetTcStripeNum(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetTcEnhanceIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetLuminance(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetContrast(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetSharpnessIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetLowCompensation(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetHighSensitivity(XRAYLIB_CONFIG_PARAM *stPara);
	/* 功能类参数 */
	XRAY_LIB_HRESULT SetRGBRotate(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetRGBMirror(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetScanMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetMaterialAdjust(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetDoseCali(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetGeomertricCali(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetGeoCaliInverse(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetLowPenetrationPrompt(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetLowPeneSenstivity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetLowPenetrationThreshold(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetLowPenetrationWarnThreshold(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetConcernedMPrompt(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetConcernedMSnestivity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetFlatDetmertricCali(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetEexcutionMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetSpeed(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetRTHeight(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetIntegralTime(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetColdAndHotCali(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetDrugPrompt(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetExplosivePrompt(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetEuTestPrompt(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetLogSaveMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetLogLevel(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetTimeCostShow(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT SetDetectionMode(XRAYLIB_CONFIG_PARAM * stPara);
	XRAY_LIB_HRESULT SetGeometricRatio(XRAYLIB_CONFIG_PARAM * stPara);
	/******************************************************************************************
	 *                                      图像状态获取
	 *******************************************************************************************/
	XRAY_LIB_HRESULT GetEnergyMode(XRAYLIB_CONFIG_IMAGE *stPara);		   // 能量获取
	XRAY_LIB_HRESULT GetColorMode(XRAYLIB_CONFIG_IMAGE *stPara);		   // 颜色获取
	XRAY_LIB_HRESULT GetPenetrationMode(XRAYLIB_CONFIG_IMAGE *stPara);	   // 高低穿获取
	XRAY_LIB_HRESULT GetEnahnceMode(XRAYLIB_CONFIG_IMAGE *stPara);		   // 增强获取
	XRAY_LIB_HRESULT GetInverseMode(XRAYLIB_CONFIG_IMAGE *stPara);		   // 反色获取
	XRAY_LIB_HRESULT GetGrayLevel(XRAYLIB_CONFIG_IMAGE *stPara);		   // 灰度级数获取
	XRAY_LIB_HRESULT GetBrightness(XRAYLIB_CONFIG_IMAGE *stPara);		   // 亮度获取
	XRAY_LIB_HRESULT GetSingleColorTableSel(XRAYLIB_CONFIG_IMAGE *stPara); // 单能色表获取
	XRAY_LIB_HRESULT GetDualColorTableSel(XRAYLIB_CONFIG_IMAGE *stPara);   // 双能色表获取
	XRAY_LIB_HRESULT GetColorImagingSel(XRAYLIB_CONFIG_IMAGE *stPara);	   // 三色六色颜色获取

	/******************************************************************************************
	 *                                        参数获取
	 *******************************************************************************************/
	/* 图像校正参数 */
	XRAY_LIB_HRESULT GetWhitenUpNum(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetWhitenDownNum(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetBackgroundThreshold(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetBackgroundGray(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetRTUpdateAirRatio(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetEmptyThresholdH(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetEmptyThresholdL(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetRTUpdateLowGrayRatio(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetPackageThreshold(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetLibXRayDefaultImageWidth(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetColdHotCaliThreshold(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetBeltGrainLimit(XRAYLIB_CONFIG_PARAM *stPara);
	/* 图像处理参数	*/
	XRAY_LIB_HRESULT GetDualFilterKernelLength(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetDualFilterRange(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetDualDistinguishGrayUp(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetDualDistinguishGrayDown(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetDualDistinguishNoneInorganic(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetDefaultEnhance(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetDefaultEnhanceIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetEdgeEnhanceIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetSuperEnhanceIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetSpecialEnhanceThresholdUp(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetSpecialEnhanceThresholdDown(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetSpecialEnhanceStretchGrayUp(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetSpecialEnhanceStretchGrayDown(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetCaliLowGrayThreshold(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetDenoisingIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetCompositiveRatio(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetDenoisingMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetAntialiasingMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetAIDeviceMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetResizeHeightFactor(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetResizeWidthFactor(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetFusionMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetBackgoundColor(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetStretchRatio(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetGammaMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetGammaIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetSigmaNoise(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetPeneMergeParam(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetMergeBaseLine(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetTestMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetTcStripeNum(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetLumIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetContrastRatio(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetSharpnessRatio(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetLowLumCompensation(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetHighLumSensity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetTcEnhanceIntensity(XRAYLIB_CONFIG_PARAM *stPara);
	/* 功能类参数 */
	XRAY_LIB_HRESULT GetRGBRotate(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetRGBMirror(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetScanMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetMaterialAdjust(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetDoseCali(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetGeomertricCali(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetGeoCaliInverse(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetFlatDetmertricCali(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetLowPenetrationPrompt(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetLowPeneSenstivity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetLowPenetrationThreshold(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetLowPenetrationWarnThreshold(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetConcernedMPrompt(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetConcernedMSnestivity(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetEexcutionMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetSpeed(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetRTHeight(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetIntegralTime(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetSpeedGear(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetColdAndHotCali(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetDrugPrompt(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetExplosivePrompt(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetCurveState(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetEuTestPrompt(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetLogSaveMode(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetLogLevel(XRAYLIB_CONFIG_PARAM *stPara);
	XRAY_LIB_HRESULT GetDetectionMode(XRAYLIB_CONFIG_PARAM * stPara);
	XRAY_LIB_HRESULT GetGeometricRatio(XRAYLIB_CONFIG_PARAM * stPara);
}; // class XRayImageProcess

extern "C" IXRayImageProcess *CreateXspObj(void *ptr);
extern "C" void DestroyXspObj(IXRayImageProcess *pExport);

#endif // _XRAY_IMAGE_PROCESS_HPP_
