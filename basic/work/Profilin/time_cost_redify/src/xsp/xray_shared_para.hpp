#ifndef _XRAY_SHARED_PARA_
#define _XRAY_SHARED_PARA_

#include "libXRay_def.h"

/**
* @brief 算法库全局共享参数，不同算法库句柄维护不同全局参数
* @note  用于共享部分参数到每个功能子模块
*/
class SharedPara
{
public:
	XRAYLIB_ENERGY_MODE m_enEnergyMode;           // 能量模式
	XRAY_DEVICEINFO     m_enDeviceInfo;           // 机型信息
	XRAY_LIB_VISUAL		m_enVisual;				  // 视角信息
	XRAY_DETECTOR_PIXEL m_nPerDetectorWidth;	  // 单个探测器所占宽度, 默认为64
	int                 m_nBackGroundThreshold;   // 背景阈值
	int                 m_nBackGroundGray;        // 背景灰度
	int                 m_nPackageThresholdR;     // 包裹检测阈值
	int					m_nDetectorWidth;	      // 探测器尺寸
	float               m_fResizeScale;           // 最大缩放系数
	int					m_nMaxHeightEntire;		  // 最大整图高度
}; // SharedPara

#endif // _XRAY_SHARED_PARA_
