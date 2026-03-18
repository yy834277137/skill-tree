/*******************************************************************************
* 
* 版权信息：版权所有 (c) 2010-2018, 杭州海康威视软件有限公司, 保留所有权利
*
* 文件名称：vca_common.h
* 文件标识：HIK_VCA_COMMOM_H_
* 摘    要：海康威视VCA公共数据结构体声明文件
*
* 当前版本：0.0.11
* 作    者：戚红命
* 日    期：2012年11月28日
* 备    注：添加了CPU类型不支持的错误码
*
* 当前版本：0.0.10
* 作    者：张继霞
* 日    期：2010年10月8日
* 备    注：追加交通ITS错误码
*
* 历史版本：0.0.9
* 作    者：王道荣
* 日    期：2010年9月1日
* 备    注：初始版本
********************************************************************************/

#ifndef _HIK_VCA_ERROR_CODE_H_
#define _HIK_VCA_ERROR_CODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HIK_VCA_LIB_S_OK                     1           //成功	  

//系统类
#define HIK_VCA_LIB_RESOLUTION_UNSUPPORT     0x80000000  //分辨率不支持
#define HIK_VCA_LIB_E_MEM_OUT                0x80000001  //内存不够
#define HIK_VCA_LIB_E_PTR_NULL	             0x80000002  //传入指针为空  

//参数类
#define HIK_VCA_LIB_KEY_PARAM_ERR            0x80000003  //高级参数设置错误

//配置信息类
#define HIK_VCA_LIB_ID_ERR                   0x80000004  //配置ID不合理
#define HIK_VCA_LIB_POLYGON_ERR              0x80000005  //多边形不符合要求 
#define HIK_VCA_LIB_SIZE_FILTER_ERR          0x80000006  //过滤框不合理
#define HIK_VCA_LIB_RULE_PARAM_ERR           0x80000007  //规则参数不合理
#define HIK_VCA_LIB_RULE_CFG_CONFLICT        0x80000008  //配置信息冲突 
#define HIK_VCA_LIB_CALIBRATE_NOT_READY      0x80000009  //当前没有标定信息
#define HIK_VCA_LIB_CALIBRATE_DATA_ERR       0x8000000A  //标定样本数目错误，或数据值错误，或样本点超出地平线 
#define HIK_VCA_LIB_CAMERA_PARAM_ERR         0x8000000B  //摄像机参数不合理
#define HIK_VCA_LIB_CALIBRATE_DATA_UNFIT     0x8000000C  //长度线不够倾斜,不利于摄像机标定
#define HIK_VCA_LIB_CALIBRATE_DATA_CONFLICT  0x8000000D  //标定出错,因为标定点所有点共线或位置太集中
#define HIK_VCA_LIB_CALIBRATE_FAIL           0x8000000E  //摄像机标定参数值计算失败
#define HIK_VCA_LIB_CALIBRATE_DISABLE_FAIL   0x8000000F  //所配置规则不允许取消标定
#define HIK_VCA_LIB_CALIBRATE_PNT_OUT_RECT   0x80000010  //输入的样本标定线超出了样本外接矩形框

//IAS错误码
#define HIK_VCA_LIB_ENTER_RULE_NOT_READY     0x80000011  //未设置人进入检测区域

//ITS错误码			     
#define HIK_VCA_LIB_AID_RULE_NO_INCLUDE_LANE 0x80000012    //事件规则没有包括车道(特值拥堵和逆行)
#define HIK_VCA_LIB_AID_RULE_INCLUDE_TWO_WAY 0x80000013    //特指逆行事件规则中包括2中不同车流方向 
#define HIK_VCA_LIB_E_LANE_NOT_READY         0x80000014    //当前没有车道设置

#define HIK_VCA_LIB_DEBUG_UNSUPPORT          0x80000015    //不支持调试信息输出
#define HIK_VCA_LIB_E_LANE_TPS_RULE_CONFLICT 0x80000016 //车道与数据规则冲突
#define HIK_VCA_LIB_E_LANE_NO_WAY            0x80000017 //车道没有设置方向

//FD错误码
#define HIK_VCA_LIB_FFL_NO_FACE		         0x80000018 // 特征点定位时输入的图像没有人脸
#define HIK_VCA_LIB_FFL_IMG_TOO_SMALL        0x80000019 // 特征点定位时输入的图像中人脸占比太大
	
//主从跟踪错误码
#define HIK_VCA_LIB_E_LF_CAL_POINTS_NUM      0x8000001A  //LF标定点的个数不对
#define HIK_VCA_LIB_E_LF_CAL_POINTS_LINEAR   0x8000001B  //LF标定点在同一条线上
#define HIK_VCA_LIB_E_LF_CAL_NOT_READY       0x8000001C  //当前LF还没有标定

//音频错误码
#define HIK_VCA_LIB_E_AUDIO_UNSUPPORT        0x8000001D  //音频输入错误

//FD错误码
#define HIK_VCA_LIB_FD_IMG_NO_FACE           0x8000001E  // 单张图像人脸检测时输入的图像没有人脸

//FR错误码
#define HIK_VCA_LIB_FACE_TOO_SMALL           0x8000001F  // 建模时人脸太小
#define HIK_VCA_LIB_FACE_QUALITY_TOO_BAD     0x80000020  // 建模时人脸图像质量太差

//FD错误码
#define HIK_VCA_LIB_FD_SCALE_OUTRANGE        0x80000021  // 最大过滤框的宽高最小值超过最小过滤框的宽高最大值两倍以上
#define HIK_VCA_LIB_FD_REGION_TOO_LARGE      0x80000022  // 当前检测区域范围过大。检测区最大为图像的2/3。

//FR错误码
#define HIK_VCA_LIB_FR_FPL_FAIL              0x80000023 // 人脸特征点定位失败
#define HIK_VCA_LIB_FR_IQA_FAIL              0x80000024 // 人脸评分失败
#define HIK_VCA_LIB_FR_FEM_FAIL              0x80000025 // 人脸特征提取失败
#define HIK_VCA_LIB_FFD_CONF_TOO_LOW		 0x80000026 // 准正面人脸检测置信度过低
#define HIK_VCA_LIB_FPL_CONF_TOO_LOW         0x80000027 // 特征点定位置信度过低
#define HIK_VCA_LIB_IQA_SCORE_TOO_LOW        0x80000028 // 建模时人脸图像质量太差

//处理器
#define HIK_VCA_LIB_CPU_TYPE_UNSUPPORT       0x80000029 // CPU型号不支持

//FR错误码
#define HIK_VCA_LIB_E_DATA_SIZE              0x8000002A  // 数据长度不匹配
#define HIK_VCA_LIB_FR_MODEL_VERSION_ERR     0x8000002B  // 人脸模型数据中的模型版本错误

// 属性识别库错误码
#define HIK_VCA_LIB_FA_NORMALIZE_ERR         0x8000002C  // 人脸归一化出错

//FR错误码
#define HIK_VCA_LIB_FR_FFD_FAIL              0x8000002D  // 识别库中人脸检测失败
#define HIK_VCA_LIB_E_IN_PARAM               0x8000002E  // 输入参数有误

//SMD错误码
#define HIK_VCA_LIB_ERR_UNPROC               0x8000002F  //返回的错误码没有处理(SMD)


#define HIK_VCA_VIDEO_FORMAT_UNSUPPORT       0x80000030  //视频格式不支持，视频格式详见VCA_YUV_FORMAT

#define HIK_VCA_CNN_MODEL_ERROR              0x80000031  // 模型错误
#ifdef __cplusplus
}
#endif 

#endif /* _HIK_VCA_LIB_ERROR_CODE_H_ */

