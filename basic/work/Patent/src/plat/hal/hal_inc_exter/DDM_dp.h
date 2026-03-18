/**
 * @File: DDM_dp.h
 * @Module: DDM (Device Driver Model 设备驱动模型)
 * @Author: fushichen@hikvision.com.cn
 * @Created: 2017年2月6日
 *
 * @Description: 平台RESET模块(component)模型(内核层kernel) 与 应用层(user-land)接口
 *               功能包括 通过 DDM_IOC_RESET_TOTAL 对外设进行复位
 *
 * @Note : PCM 提供给内部模块、外部模块（如DDM）使用的公共头文件。修改需要评审
 * 
 * @Usage:操作步骤
 *
 *                  open /dev/display 打开驱动
 *                             |
 *               ioctl DDM_IOC_DP_GET_PARAM 获取对应通道下芯片模块参数，个数及名称
 *                             |
 *               ioctl DDM_IOC_DP_SET_RES 设置分辨率
 *                             |
 *               ioctl DDM_IOC_DP_RX_RESET 进行rx复位
 *                             |
 *               ioctl DDM_IOC_DP_DISPLAY_ENABLE 进行外设复位
 *                             |
 *                           close
 */

#ifndef INCLUDE_DDM_DDM_DP_H_
#define INCLUDE_DDM_DDM_DP_H_

/*********************************************************************
 ******************************* 头文件  *******************************
 *********************************************************************/
/**
 * 底层公共接口
 */
//#include "OSAL_api.h"
/*********************************************************************
 ******************************* 宏枚举定义  ***************************
 *********************************************************************/ 
#define DDM_DEV_DP_NAME        "/dev/DDM/display"     					//display显示类设备名称

/**
 * dp 字符驱动魔数 (magic number)
 */
#define DDM_IOC_DP_MNUM        'D'

/**
 *  dp 设置分辨率
 */
#define DDM_IOC_DP_SET_RES          _IOW(DDM_IOC_DP_MNUM, 0, DDM_dpSet)

/**
 *  dp rx复位，以通道为单位
 */
#define DDM_IOC_DP_RX_RESET         _IOW(DDM_IOC_DP_MNUM, 1, UINT8)

/**
 *   获取对应通道下芯片模块参数，个数及名称
 */
#define DDM_IOC_DP_GET_PARAM        _IOW(DDM_IOC_DP_MNUM, 2, DDM_dpGet*)


/**
* 使能模块输出，以通道为单位
 */
#define DDM_IOC_DP_DISPLAY_ENABLE        _IOW(DDM_IOC_DP_MNUM, 3, UINT8)

/**
* 获取热插拔状态,以通道为单位获取状态
 */
#define DDM_IOC_DP_PLUG_STATE       _IOW(DDM_IOC_DP_MNUM, 4, DDM_dpGet*)


/*********************************************************************
 ******************************* 结构体  *****************************
 *********************************************************************/


typedef enum tagDDM_dpVoDev
{
    VO_DEV_DHD0 = 0,			/* VO's device HD0 */
    VO_DEV_DHD1,				/* VO's device HD1 */
    VO_DEV_DHD2,				/* VO's device HD2 */
    VO_DEV_DHD3,				/* VO's device HD3 */
    VO_DEV_MAX,
}DDM_dpVoDev;  


/**
 *  显示转接模块分辨率支持类型
 */
typedef enum tagDDM_dpDisplayMode
{
    MODE_SVGA_60HZ,
    MODE_SVGA_75HZ,
    MODE_XGA_60HZ,
    MODE_XGA_75HZ,
    MODE_SXGA_60HZ,
    MODE_SXGA2_60HZ,
    MODE_720P_60HZ,
    MODE_720P_50HZ,
    MODE_1080I_60HZ,
    MODE_1080I_50HZ,
    MODE_1080P_60HZ,
    MODE_1080P_50HZ,
    MODE_1080P_30HZ,
    MODE_1080P_25HZ,
    MODE_1080P_24HZ,
    MODE_UXGA_60HZ,
    MODE_UXGA_30HZ,
    MODE_WSXGA_60HZ,
    MODE_WUXGA_60HZ,
    MODE_WUXGA_30HZ,
    MODE_720x480_50HZ,
    MODE_720x576_50HZ,
    MODE_720P_60HZ_TEST,
    MODE_WXGA_60HZ, /*1360 x 768P  60HZ			support*/
    MODE_WXGA2_60HZ, /*1366 x 768P  60HZ		support*/
    MODE_XGA2_60HZ, /*1440 x 900P  60HZ			support*/
    MODE_SXGA_75HZ,
    MODE_1280x768_75HZ,
    MODE_SXGA_70HZ,
    MODE_XGA_70HZ,
    MODE_XGA_72HZ,
    MODE_XGA_85HZ,
    MODE_SXGA_72HZ,
    MODE_SXGA_85HZ,
    MODE_1080P_72HZ,
    MODE_720P_72HZ,
    MODE_1280x800_60HZ,
    MODE_1280x800_72HZ,
    MODE_1280x800_75HZ,
    MODE_1280x800_85HZ,
    MODE_1280x768_60HZ,
    MODE_1280x768_72HZ,
    MODE_1280x768_85HZ,
    MODE_1680x1050_60HZ,
} DDM_dpDisplayMode;   


typedef enum tagDDM_dpPlugInState
{
	DP_PLUGIN_NEVER = 0,       /*未曾接入*/
	DP_PLUGIN_ALREADY,		  /*已接入*/
	DP_PLUGIN_MAX,
}DDM_dpPlugInState;


/**
 *  显示转接模块应用设置参数集合
 */
typedef struct tagDDM_dpSet
{
	DDM_dpVoDev channel_num;		/*通道号，代表第几路输出@DDM_dpVoDev*/
	unsigned char ref[4];			/*预留，32字节对齐*/
	DDM_dpDisplayMode vo_mode;  /*分辨率类型*/
}DDM_dpSet;

/**
 *  显示转接模块应用获取能力集合
 */
typedef struct tagDDM_dpGet
{
	DDM_dpVoDev channel;         	/*通道号，代表第几路输出@DDM_dpVoDev*/
	unsigned char chip_count;     			/*芯片个数*/
	unsigned char refs[3];					/*预留，32字节对齐*/
	char chip_name[4][32]; 			/*芯片名称*/	
	DDM_dpPlugInState plugin_state;  /*芯片热插拔状态*/
}DDM_dpGet;
/*********************************************************************
 ******************************* 函数声明  ***************************
 *********************************************************************/


#endif /* INCLUDE_DDM_DDM_DP_H_ */
