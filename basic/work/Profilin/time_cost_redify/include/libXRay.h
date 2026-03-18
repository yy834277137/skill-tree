/**********************************************************************************************************************************
*
* 版权信息：版权所有 (c) 2021, 杭州海康威数字技术股份有限公司, 保留所有权利
*
* 文件名称：libXRay.h
* 摘    要：libXRay算法库接口实现文件
*
***********************************************************************************************************************************/
#pragma once

#ifndef _XRAYLIB_H_
#define _XRAYLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "libXRay_def.h"

/***************************************************************************************************
* 接口函数
**************************************************************************************************/

/****************************************************************************************************
* 函数名：libXRay_GetVersion
* 功  能：获取当前XRayLib库版本信息和编译日期
* 参  数：
*        versionType				- I			 版本类型
* 返回值：返回版本信息结构体
* 备  注：版本信息格式见结构体
* 示  例：
*     // 获取成像算法库版本(4.1.85)
*     XRAYLIB_XRY_VERSION ver1 = libXRay_GetVersion(XRAYLIB_VERSION_IMAGING_ALG);
****************************************************************************************************/
XRAYLIB_XRY_VERSION libXRay_GetVersion(XRAY_LIB_VERSION_TYPE versionType);

/***************************************************************************************************
* 函数名：libXRay_GetMemSize
* 功  能：获取算法库所需内存大小
* 参  数：
*        ability				- I			 算法库能力集
*        memTab                 - I/O        内存申请表
*
* 返回值：状态码
* 备  注：
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_GetMemSize(XRAY_LIB_ABILITY*	   ability,
                                    XRAY_LIB_MEM_TAB       mem_tab[XRAY_LIB_MTAB_NUM]);

/***************************************************************************************************
* 函数名：libXRay_Create
* 功  能: 创建算法库
* 参  数:
*		 ability				- I			 算法库能力集
*        xraylib_AIPara         - I          AI-XSP初始化参数
*        PublicFileFolderName   - I          算法读取参数文件位置，公用的参数文件，不会改变，可以传入相同参数
*        PrivateFileFolderName  - I          算法存储参数文件位置，为各句柄私有文件，会随着算法不同改变，必须为每个句柄创建不同的参数存放位置
*        mem_tab                - I          内存申请表
*        handle                 - O          算法库句柄
*        xraylib_pipeline_mode  - I          流水线模式设置
*        xraylib_devicetype     - I          设备类型
*        xraylib_visualNo       - I          视角类型
*        xraylib_souretype      - I          射线源类型
* 返回值: 状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_Create(XRAY_LIB_ABILITY*	  ability,
	                            XRAYLIB_AI_PARAM      xraylib_AIPara, 
	                            const char*           PublicFileFolderName, 
	                            const char*           PrivateFileFolderName, 
	                            XRAY_LIB_MEM_TAB      mem_tab[XRAY_LIB_MTAB_NUM], 
	                            VOID**                handle, 
	                            XRAYLIB_PIPELINE_MODE xraylib_pipeline_mode, 
	                            XRAY_DEVICEINFO*      xraylib_deviceInfo);

/***************************************************************************************************
* 函数名：libXRay_Create_AbsolutePath
* 功  能: 创建算法库,算法参数位置使用绝对路径
* 参  数:
*		 ability				- I			 算法库能力集
*        xraylib_AIPara         - I          AI-XSP初始化参数
*        PublicFileFolderName   - I          算法读取参数文件位置，公用的参数文件，不会改变，可以传入相同参数
*        PrivateFileFolderName  - I          算法存储参数文件位置，为各句柄私有文件，会随着算法不同改变，必须为每个句柄创建不同的参数存放位置
*        mem_tab                - I/O        内存申请表
*        handle                 - O          算法库句柄
*        xraylib_pipeline_mode  - I          流水线模式设置
*        xraylib_devicetype     - I          设备类型
*        xraylib_visualNo       - I          视角类型
* 返回值: 状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_Create_AbsolutePath(XRAY_LIB_ABILITY*	   ability,
	                                         XRAYLIB_AI_PARAM      xraylib_AIPara, 
	                                         const char*           PublicFileFolderName, 
	                                         const char*           PrivateFileFolderName, 
	                                         XRAY_LIB_MEM_TAB      mem_tab[XRAY_LIB_MTAB_NUM], 
	                                         VOID**                handle, 
	                                         XRAYLIB_PIPELINE_MODE xraylib_pipeline_mode, 
	                                         XRAY_DEVICEINFO*      xraylib_deviceInfo);

/***************************************************************************************************
* 函数名：libXRay_EnablePrint
* 功  能：打开算法打印输出
* 参  数：
*        handle                 -I             句柄(handle由XRayLib_Create返回)
*		 XRAY_LIB_PRINT			-I			   是否打开打印
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_EnablePrint(VOID                   *handle,
									 XRAY_LIB_ONOFF			xarylibprint);

/***************************************************************************************************
* 函数名：libXRay_UpdateZeroAndFull
* 功  能: 更新满载和本底
* 参  数:
*        handle                -I           句柄(handle由XRayLib_Create返回)
*        p_xraylib_org_lh      -I           输入参数结构体地址
*        updatemode            -I           更新方式
* 返回值: 状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_UpdateZeroAndFull(VOID                            *handle,
	                                       XRAYLIB_IMAGE                   *p_xraylib_org_lh,
                                           XRAYLIB_UPDATE_ZEROANDFULL_MODE updatemode);

/***************************************************************************************************
* 函数名：libXRay_UpdateZeroAndFullMem
* 功  能: 更新满载和本底内存
* 参  数:
*        handle                -I           句柄(handle由XRayLib_Create返回)
*        p_xraylib_org_lh      -I/O         输入参数结构体地址
*        updatemode            -I/O         更新方式
* 返回值: 状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_UpdateZeroAndFullMem(VOID                             *handle,
	                                          XRAYLIB_IMAGE                    *p_xraylib_org_lh,
	                                          XRAYLIB_UPDATE_ZEROANDFULL_MODE  updatemode);

/***************************************************************************************************
* 函数名：libXRay_SetTIPImage
* 功  能：设置TIP数据，设置返回成功后具备插入条件即会开始进行TIP插入
* 参  数：
*        handle                       -I         句柄(handle由XRayLib_Create返回)
*        p_xraylib_tip_img            -I         TIP数据结构体
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_SetTIPImage(VOID               *handle,
	                                 XRAYLIB_IMAGE      *p_xraylib_tip_img,
									 int packageHeight,
									 int randomPosition);

/***************************************************************************************************
* 函数名：libXRay_ForceStopTIP
* 功  能：强制停止当前正在的TIP操作
* 参  数：
*        handle                       -I         句柄(handle由XRayLib_Create返回)
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_ForceStopTIP(VOID               *handle);

/***************************************************************************************************
* 函数名：libXRay_GetProcessMergeImage
* 功  能：获取高低能融合图像，该图像为最终显示的灰度图效果。
* 参  数：
*        handle                 -I             句柄(handle由XRayLib_Create返回)
*        p_xraylib_calilh       -I             高低能数据
*        p_xraylib_merge        -O             融合图像数据(不需要输出上下邻域, 算法输出需要截掉)
*		 height_ntop            -I             上邻域高度，可为0，该区域不输出
*        height_nbotm           -I             下邻域高度，可为0，该区域不输出
*		 bDescendOrder          -I			   是否逆序读取数据, 1表示逆序读取, 0则默认正序读取
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_GetProcessMergeImage(VOID                   *handle,
											  XRAYLIB_IMAGE          *p_xraylib_calilh,
											  XRAYLIB_IMAGE          *p_xraylib_merge,
											  int height_ntop,
											  int height_nbotm,
											  int bDescendOrder);

/***************************************************************************************************
* 函数名：libXRay_GetImage
* 功  能：获取彩色化后YUV或RGB图像，需要融合灰度图（或高低能灰度图）和原子序数图输入,输出融合灰度图（输入高低能的情况）和YUV或RGB图像
* 参  数：
*        handle                 -I              句柄(handle由XRayLib_Create返回)
*        p_xraylib_calilhz      -I              输入高能低能原子序数
*        p_xraylib_merge        -I/O            输入融合图像(需要输出上下邻域, DSP自己截掉)
*        p_xraylib_color        -O              输出彩色图像
*        p_xraylib_tip          -I              tip数据
*        height_ntop            -I              上邻域高度，可为0，该区域不输出
*        height_nbotm           -I              下邻域高度，可为0，该区域不输出
*		 bDescendOrder          -I				是否逆序读取数据, 1表示逆序读取, 0则默认正序读取
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_GetImage(VOID                   *handle,
                                  XRAYLIB_IMAGE          *p_xraylib_calilhz,
								  XRAYLIB_IMAGE          *p_xraylib_merge,
								  XRAYLIB_IMAGE          *p_xraylib_color,
	                              XRAYLIB_DUMPPROC_TIP_PARAM *p_xraylib_tip,
							      XRAYLIB_IMG_CHANNEL    img_channel,
                                  int                    height_ntop, 
	                              int                    height_nbotm,
								  int					 bDescendOrder);

/***************************************************************************************************
* 函数名：libXRay_ConcernedArea
* 功  能：根据能量输入高能和低能归一化后图像和原子序数图，分别返回可疑有机物区域坐标及难穿透区域坐标，
		  两者均可以通过单独的开关选择是否计算坐标，分别最大支持返回10个可疑区域
* 参  数：
*        handle                 -I              句柄(handle由XRayLib_Create返回)
*        p_xraylib_img_param    -I/O            输入参数结构体地址
*        ConcerdMaterialM       -0              返回的可疑有机物坐标值
*        LowPenetration			-O              返回的难穿透区域坐标值
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_ConcernedArea(VOID                  *handle,
									   XRAYLIB_IMAGE         *p_xraylib_img_param,
									   XRAYLIB_CONCERED_AREA *ConcerdMaterialM,
									   XRAYLIB_CONCERED_AREA *LowPenetration,
									   XRAYLIB_CONCERED_AREA *ExplosiveArea,
	                                   XRAYLIB_CONCERED_AREA *DrugArea);

/***************************************************************************************************
* 函数名：libXRay_ConcernedAreaPiecewise
* 功  能：根据能量输入高能和低能归一化后图像和原子序数图，分别返回可疑有机物区域坐标及难穿透区域坐标，
		  两者均可以通过单独的开关选择是否计算坐标，分别最大支持返回10个可疑区域
* 参  数：
*        handle                 -I              句柄(handle由XRayLib_Create返回)
*        p_xraylib_img_param    -I/O            输入参数结构体地址
*        ConcerdMaterialM       -0              返回的可疑有机物坐标值
*        LowPenetration			-O              返回的难穿透区域坐标值
*        ExplosiveArea			-O              返回的爆炸物区域坐标值
*        DrugArea			    -O              返回的毒品区域坐标值
*        nDetecFlag			    -O              包裹标志位（非负数）该值改变时表示一个包裹的分段检测结束，开始检测下一个包裹
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_ConcernedAreaPiecewise(VOID                  *handle,
									            XRAYLIB_IMAGE         *p_xraylib_img_param,
									            XRAYLIB_CONCERED_AREA *ConcerdMaterialM,
									            XRAYLIB_CONCERED_AREA *LowPenetration,
									            XRAYLIB_CONCERED_AREA *ExplosiveArea,
	                                            XRAYLIB_CONCERED_AREA *DrugArea,
	                                            int                    nDetecFlag);
/***************************************************************************************************
* 函数名：libXRay_SetConfig
* 功  能：设置对应handle中的参数
* 参  数：
*        handle                 -I         句柄(handle由XRayLib_Create返回)
*        configparam            -I         参数设置结构体
*        configchannel          -I         设置参数类型。（图像处理命令or参数值设置）
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_SetConfig(VOID                       *handle,
	                               VOID                       *configparam,
								   XRAYLIB_SETCONFIG_CHANNEL  configchannel); 

/***************************************************************************************************
* 函数名：libXRay_GetConfig
* 功  能：获取对应handle中的参数
* 参  数：
*        handle                 -I         句柄(handle由XRayLib_Create返回)
*        configparam            -O         参数设置结构体
*        configchannel          -I         设置参数类型。（图像处理命令or参数值设置）
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_GetConfig(VOID                       *handle,
	                               VOID                       *configparam,
	                               XRAYLIB_SETCONFIG_CHANNEL  configchannel);

/***************************************************************************************************
* 函数名：libXRay_GetCaliTable
* 功  能：获取对应handle中的校正表信息
* 参  数：
*        handle                 -I         句柄(handle由XRayLib_Create返回)
*        calitable              -O         校正表信息结构体
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_GetCaliTable(VOID               *handle,
	                                  VOID               *calitable);

/***************************************************************************************************
* 函数名：libXRay_Release
* 功  能：释放dll内申请的handle资源
* 参  数：
*        handle                 -I        句柄(handle由XRayLib_Create返回)
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_Release(VOID *handle);

/***************************************************************************************************
* 函数名：libXRay_AESEncrypt
* 功  能：提供随机数明文的密文
* 参  数：
*        encryptedMessage				提供随机数明文的密文
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_AESEncrypt(unsigned char *encryptedMessage);

/***************************************************************************************************
* 函数名：libXRay_AESCompare
* 功  能：明文比较
* 参  数：
*        Input				经DSP解密后的明文
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_AESCompare(unsigned char *Input);


/***************************************************************************************************
* 函数名：libXRay_PipelineProcess
* 功  能: 流水线实时处理
* 参  数:
*        handle                      -I           句柄(handle由XRayLib_Create返回)
*        p_xraylib_pipeline_mode3a   -I/O         流水线结构体
*        index                       -I           流水线级数设置
* 返回值: 状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_PipelineProcess(VOID    *handle, 
	                                     VOID    *p_xraylib_pipeline, 
	                                     int     index);

/***************************************************************************************************
* 函数名：libXRay_ReloadProfile
* 功  能: 重新载入配置文件（目前用于重载R曲线）
* 参  数:
*        handle                      -I           句柄(handle由XRayLib_Create返回)
*        xraylib_deviceInfo          -I           机型信息
* 返回值: 状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_ReloadProfile(VOID            *handle,
	                                   XRAY_DEVICEINFO *xraylib_deviceInfo);

/***************************************************************************************************
* 函数名：libXRay_GetRCurveName
* 功  能: 外部获取算法资源曲线路径
* 参  数:
*        handle                      -I           句柄(handle由XRayLib_Create返回)
*        xraylib_deviceInfo          -I           机型信息
*        xraylib_rcurve_type         -I           设置需要获取的R曲线类型
*        xraylib_rcurvename          -O           R曲线名
* 返回值: 状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_GetRCurveName(VOID            *handle,
	                                   XRAY_DEVICEINFO *xraylib_deviceInfo,
	                                   XRAY_RCURVE_TYPE xraylib_rcurve_type,
	                                   char* xraylib_rcurvename);

/***************************************************************************************************
* 函数名：libXRay_SetTestModeParams
* 功  能: 设置指标模式参数
* 参  数:
*        handle                      -I           句柄(handle由XRayLib_Create返回)
*        xraylib_testbparams         -I           测试体参数
* 返回值: 状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_SetTestModeParams(VOID                     *handle,
	                                       XRAYLIB_MATERIAL_COLOR_PARAMS  *xraylib_testbparams);


/***************************************************************************************************
* 函数名：libXRay_RCurveAdjust
* 功  能: 设置颜色微调参数
* 参  数:
*        handle                      -I           句柄(handle由XRayLib_Create返回)
*        xraylib_rcurveparams         -I           测试体参数
* 返回值: 状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_RCurveAdjust(VOID                     *handle,
	                                       XRAYLIB_RCURVE_ADJUST_PARAMS  *xraylib_rcurveparams);

/***************************************************************************************************
// * 函数名：libXRay_HLCorrection
// * 功  能：高低能校正接口，用于液体识别，或高低能灰度图输入,输出校正后的高低能灰度图
// * 参  数：
// *        handle                 -I              句柄(handle由XRayLib_Create返回)
// *        p_xraylib_calilhz      -I/O            输入参数结构体地址
// * 返回值：返回状态码
// ***************************************************************************************************/
// XRAY_LIB_HRESULT libXRay_HLCorrection(VOID                   *handle,
// 	                                  XRAYLIB_IMAGE          *p_xraylib_calilhz);


/***************************************************************************************************
* 函数名：libXRay_GetGeoOffsetLut
* 功  能：AI路不做几何校正，输出一个探测器方向检测框的几何校正映射表
* 参  数：
*        handle                 -I              句柄(handle由XRayLib_Create返回)
*		 p_geooffset_lut_width  -I              输入参数结构体宽度
*        p_xraylib_calilhz      -I/O            输入参数结构体地址
* 返回值：返回状态码
***************************************************************************************************/
XRAY_LIB_HRESULT libXRay_GetGeoOffsetLut(VOID                   *handle,
										 int			p_geooffset_lut_width,
	                                     unsigned short         *p_geooffset_lut);

/**
 * @fn      libXRay_ProcessDataDump
 * @brief   XSP库中各流程的数据Dump
 * 
 * @param   [IN] handle 句柄，由XRayLib_Create返回
 * @param   [IN] dataPoint Dump的数据点，内部自定义
 * @param   [IN] dumpCount Dump的次数
 * @param   [IN] pDirName Dump数据到指定路径
 * 
 * @return  XRAY_LIB_HRESULT 状态码
 */
XRAY_LIB_HRESULT libXRay_ProcessDataDump(VOID *handle, int dataPoint, int dumpCount, char* pDirName);


#ifdef __cplusplus
}
#endif 

#endif
