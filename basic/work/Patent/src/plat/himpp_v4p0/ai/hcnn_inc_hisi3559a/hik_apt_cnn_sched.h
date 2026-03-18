/******************************************************************************
* 
* 版权信息：版权所有 (c) 201X, 杭州海康威视数字技术有限公司, 保留所有权利
* 
* 文件名称：hik_apt_cnn_sched.h
* 文件标示：HIK_APT_CNN_SCHEDED_H__
* 摘    要：将cuda, hisi, arm等平台的调度器封装为统一的调度器
* 
* 当前版本：1.0
* 作    者：秦川6
* 日    期：2020-08-07
* 备    注：
******************************************************************************/
#ifndef __HIK_APT_CNN_SCHED_H__
#define __HIK_APT_CNN_SCHED_H__


#ifdef __cplusplus
extern "C" {
#endif


#include "hkann_def.h"
#ifdef _WIN32
#include "hik_cuda_lib.h"
#elif defined HISI_PLT
#include "hcnn_scheduler.h"
#endif
/***********************************************************************************************************************
* 宏定义
***********************************************************************************************************************/
#define MAX_ARM_CUSTOM_LAYER_NUM 8



#define HIK_APT_CNN_SCHED_MAX_NUM 16 //scheduler最多数量

// X86仿真时,统一使用CUDA


typedef enum _HIK_APT_CNN_DEVICE_
{
    HIK_APT_CNN_DEV_HISI_NNIE_0, // 海思平台NNIE
    HIK_APT_CNN_DEV_HISI_DSP_0, // 海思平台DSP
    HIK_APT_CNN_DEV_HISI_DSP_1, // 海思平台DSP
    HIK_APT_CNN_DEV_HISI_DSP_2, // 海思平台DSP
    HIK_APT_CNN_DEV_HISI_DSP_3, // 海思平台DSP
    HIK_APT_CNN_DEV_ARM, // ARM平台HKANN
    HIK_APT_CNN_DEV_TENSORRT, // TensorRT
    HIK_APT_CNN_DEV_NT, //NT平台
    HIK_APT_CNN_DEV_END = VCA_ENUM_END
} HIK_APT_CNN_DEVICE;


// CNN_TYPE 由 CNN_DEVICE自动推断而来
typedef enum _HIK_APT_CNN_TYPE_
{
    HIK_APT_CNN_HISI, // 海思平台HKANN
    HIK_APT_CNN_ARM, // ARM平台HKANN
    HIK_APT_CNN_TENSORRT, // TensorRT
    HIK_APT_CNN_NT, //NT平台
    HIK_APT_CNN_TYPE_END = VCA_ENUM_END
} HIK_APT_CNN_TYPE;
/***********************************************************************************************************************
* 结构体定义
***********************************************************************************************************************/

typedef struct _HIK_APT_CNN_SCHED_PARAM_
{
    HIK_APT_CNN_DEVICE cnn_dev[HIK_APT_CNN_SCHED_MAX_NUM];
    int32_t cnn_dev_num;
    
    int32_t arm_thread_num; // 设定ACNN 使用的线程数量

}HIK_APT_CNN_SCHED_PARAM;

/***********************************************************************************************************************
* 接口函数
***********************************************************************************************************************/
/***********************************************************************************************************************
* 功  能：创建调度器
* 参  数：*
*            param                  -I         调度器配置信息
*            mem_tab                -I         内存结构
*            handle                 -O         封装的调度器handle
* 返回值：状态码
* 备注  : plat_type仅决定scheduler的具体设置, 而scheduler的种类(hisi还是cuda等)则由宏决定(HISI_PLT, _WIN32等)
***********************************************************************************************************************/
int HIK_APT_CNN_SCHED_Create(HIK_APT_CNN_SCHED_PARAM *param,
                         VCA_MEM_TAB_V3 mem_tab[HKANN_MEM_TAB_NUM],
                         void  **handle);


/***********************************************************************************************************************
* 功  能：获取调度器内存大小
* 参  数：*
*            param                  -I         调度器配置信息
*            mem_tab                -I         内存结构
* 返回值：状态码
***********************************************************************************************************************/
int HIK_APT_CNN_SCHED_GetMemSize(HIK_APT_CNN_SCHED_PARAM *param, 
                            VCA_MEM_TAB_V3 mem_tab[HKANN_MEM_TAB_NUM]);


/***********************************************************************************************************************
* 功  能：获取实际调度器
* 参  数：*
*            handle                 -I         封装的调度器handle
*            cnn_type               -I         带获取的调度器对应的CNN种类
*            origin_sched_handle    -O         真实的调度器handle
* 返回值：状态码
***********************************************************************************************************************/
int HIK_APT_CNN_SCHED_GetOriginScheduler(void* handle, HIK_APT_CNN_TYPE cnn_type, void** origin_sched_handle);



/***********************************************************************************************************************
* 功  能：销毁调度器
* 参  数：*
*            handle       -I         封装的调度器handle
* 返回值：状态码
***********************************************************************************************************************/
int HIK_APT_CNN_SCHED_Release(void *scheduler_handle);



#ifdef __cplusplus
}
#endif
#endif //__HIK_APT_CNN_SCHED_H__