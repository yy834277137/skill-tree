#ifndef _IAS_ABNFACEABNFACE_H_
#define _IAS_ABNFACEABNFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

//#include "vca_error_code.h"
#include "vca_common.h"

/**************************************************************************************************
* 宏定义
**************************************************************************************************/
#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef int    HRESULT;
#endif //!_HRESULT_DEFINED

#ifndef _HIK_DSP_APP_
#define HIK_FD_MTAB_NUM                     1
#else
#define HIK_FD_MTAB_NUM                     3
#endif

#define HIK_IAS_ABNFACE_MAJOR_VERSION       0  //主版本号
#define HIK_IAS_ABNFACE_SUB_VERSION         8  //子版本号
#define HIK_IAS_ABNFACE_REVISION_VERSION    0  //修正版本号
#define HIK_IAS_ABNFACE_VER_YEAR            10 /* 年*/
#define HIK_IAS_ABNFACE_VER_MONTH           10 /* 月*/
#define HIK_IAS_ABNFACE_VER_DAY             19 /* 日*/

/**************************************************************************************************
* 结构体定义
**************************************************************************************************/

// fd智能库参数
typedef struct _FD_PARAM_
{
	unsigned short img_width;            //输入图像宽度
	unsigned short img_height;           //输入图像高度
} FD_PARAM;

typedef struct _FACE_DETECT_RULE_
{
	unsigned char  enable;                   //是否激活
	unsigned char  reserved[3];              //保留字节
	VCA_RULE_PARAM rule_param;
	VCA_POLYGON_F  polygon;
} FACE_DETECT_RULE;

//参数索引枚举
typedef enum _ABNFACE_PARAM_KEY_
{
	ABNFACE_DET_FRAME_DIFF_ENABLE = 0x00000001,                 // 是否利用帧差形成的mask进行加速
	ABNFACE_DET_FRAME_DIFF_THRESH = 0x00000002,                 // 帧差阈值 (范围1 ~ 8)
	ABNFACE_TRACK_ENABLE          = 0x00000003,                 // 是否进行跟踪
	ABNFACE_TCK_MIN_ENTER         = 0x00000004,                 // 进入检测中形成目标的最小轨迹长度,(范围0~100)
	ABNFACE_INTERVAL_TIME         = 0x00000005,                 // 报警间隔参数：范围：1500 - 15000 （1分钟到10分钟）
	ABNFACE_FACE_THRESH           = 0x00000006,                 // 人脸个数报警阈值：范围：1 - 10

	ABNFACE_DEBUG_PIN             = 0x80000000                  // debug_pin
} ABNFACE_PARAM_KEY;

#ifdef __cplusplus
}
#endif

#endif
