/***********************************************************************************************************************
* 版权信息：版权所有 (c), 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：hcnn_define.h
* 摘    要：hcnn统一接口定义
*
* 当前版本：2.3.0
* 作    者：唐政5
* 日    期：2018-12-11
* 备    注：初始版本
***********************************************************************************************************************/
#ifndef _HCNN_DEFINE_H__
#define _HCNN_DEFINE_H__
    
#ifdef __cplusplus
    extern "C" {
#endif

/***********************************************************************************************************************
* 宏定义
***********************************************************************************************************************/
//当前版本号
#define HKANN_HCNN_MAJOR_VERSION             2             // 主版本号
#define HKANN_HCNN_SUB_VERSION               3             // 副版本号
#define HKANN_HCNN_REVISION_VERSION          0             // 修订版本号

//版本日期
#define HKANN_HCNN_VERSION_YEAR              2019          // 年份
#define HKANN_HCNN_VERSION_MONTH             1             // 月份
#define HKANN_HCNN_VERSION_DAY               5             // 日期

/***********************************************************************************************************************
* 枚举
***********************************************************************************************************************/
//网络优先级枚举
typedef enum _HKANN_HCNN_NET_PRIORITY_E
{
    HKANN_HCNN_NET_HIGH,                               // 网络优先级-高
    HKANN_HCNN_NET_MID,                                // 网络优先级-中
    HKANN_HCNN_NET_LOW,                                // 网络优先级-低
    HKANN_HCNN_NET_AMOUNT                              // 网络优先级总数
} HKANN_HCNN_NET_PRIORITY_E;

// hcnn私有参数
typedef struct _HKANN_HCNN_PARAM_T
{

    HKANN_HCNN_NET_PRIORITY_E     net_priority;        // 网络优先级

}HKANN_HCNN_PARAM_T;

#ifdef __cplusplus
}
#endif

#endif // _HCNN_DEFINE_H__
