/***********************************************************************************************************************
* 版权信息：版权所有 (c) 2010-2027, 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：hcnn_scheduler.h
* 摘    要：海思3559通用CNN库调度器模块
*
* 当前版本：0.9.5.1
* 作    者：孙所瑞
* 日    期：2019/05/22
* 备    注：修复线程频繁创建、销毁时，调度器线程挂起，导致hcnn不继续执行也不退出的bug
* 
* 历史版本：0.9.5
* 作    者：孟泽民
* 日    期：2018/09/03
* 备    注：升级调度内部校验，配合hcnn lib进行版本和平台校验
*
* 历史版本：0.8.5
* 作    者：刘锦胜
* 日    期：2018/04/25
* 备    注：加入HCNN_GetDevTaskQueueHandle接口
*
* 历史版本：0.8.0
* 作    者：刘锦胜
* 日    期：2017/11/13
* 备    注：调度器初步版本
***********************************************************************************************************************/
#ifndef _HCNN_SCHEDULER_H_
#define _HCNN_SCHEDULER_H_

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
* 宏定义
***********************************************************************************************************************/
//当前版本号
#define HCNN_SCHEDULER_MAJOR_VERSION        0     //主版本号
#define HCNN_SCHEDULER_SUB_VERSION          9     //副版本号
#define HCNN_SCHEDULER_REVISION_VERSION     5     //修订版本号
#define HCNN_SCHEDULER_ADDITION_VERSION     1     //补充版本号

//版本日期
#define HCNN_SCHEDULER_VERSION_YEAR         2018  //年份
#define HCNN_SCHEDULER_VERSION_MONTH        09    //月份
#define HCNN_SCHEDULER_VERSION_DAY          03    //日期

//HCNN库最大管理设备数量
#define HCNN_SCHEDULER_MAX_DEV_NUM		    10 	  //3559平台最大设备数量

/***********************************************************************************************************************
* 枚举及结构体
***********************************************************************************************************************/
//HCNN设备ID
typedef enum _HCNN_DEV_ID_E
{
	HCNN_DEV_ID_NNIE0 = 0,
	HCNN_DEV_ID_NNIE1,
	HCNN_DEV_ID_DSP0,
	HCNN_DEV_ID_DSP1,
	HCNN_DEV_ID_DSP2,
	HCNN_DEV_ID_DSP3,
	HCNN_DEV_ID_A730,
	HCNN_DEV_ID_A731,
	HCNN_DEV_ID_A530,
	HCNN_DEV_ID_A531
} HCNN_DEV_ID_E;

typedef enum _HCNN_DEV_TYPE_E
{
    HCNN_DEV_NNIE    = 0,
    HCNN_DEV_DSP_VP6,
    HCNN_DEV_A73,
} HCNN_DEV_TYPE_E;

//错误码
typedef enum _HCNN_SCHEDULER_ERR_CODE_E
{
    HCNN_SCHEDULER_ERR_RET_OK   = 0,
    HCNN_SCHEDULER_ERR_LACK_MEM = 0X86100000,
} HCNN_SCHEDULER_ERR_CODE_E;

//HCNN 调度器管理设备结构体
typedef struct _HCNN_SCHEDULER_MGR_PARAM_T
{
    HCNN_DEV_ID_E 	dev_id[HCNN_SCHEDULER_MAX_DEV_NUM];				//设备ID数组
	int        	    dev_num;										//设备数量
} HCNN_SCHEDULER_MGR_PARAM_T;

/***********************************************************************************************************************
* 接口函数
***********************************************************************************************************************/
/***********************************************************************************************************************
* 功  能：创建调度器
* 参  数：*
*            scheduler_param        -I         算法库配置信息
*            mem_tab                -I         内存结构
*            scheduler_handle       -O         调度器handle
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_CreateScheduler(HCNN_SCHEDULER_MGR_PARAM_T   *scheduler_param,
						 void              		     **scheduler_handle);


/***********************************************************************************************************************
* 功  能：获取指定设备任务队列句柄
* 参  数：*
*            scheduler_handle       -I         调度器handle
*            dev_type               -I         设备类型
*            task_queue_handle      -O         任务队列handle
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_GetDevTaskQueueHandle(void              		 *scheduler_handle,
                               HCNN_DEV_TYPE_E           dev_type,
                               void                      **task_queue_handle);


/***********************************************************************************************************************
* 功  能：销毁调度器
* 参  数：*
*            scheduler_handle       -I         调度器handle
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_ReleaseScheduler(void              		 *scheduler_handle);


#ifdef __cplusplus 
}
#endif

#endif /* _HCNN_SCHEDULER_H_ */
