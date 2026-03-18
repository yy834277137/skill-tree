/***********************************************************************************************************************
* 版权信息：版权所有 (c) 2010-2027, 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：hcnn_scheduler.h
* 摘    要：通用CNN库调度器模块
*
* 当前版本：1.2.4
* 作    者：曾煜
* 日    期：2022/02/11
* 备    注：统一所有平台调度器，使之通用
*
* 当前版本：1.2.0
* 作    者：管煜祥
* 日    期：2020/04/14
* 备    注：在调度器中增加nt_cnn支持
*
* 当前版本：1.2.0
* 作    者：冷明鑫
* 日    期：2020/02/22
* 备    注：支持多进程调度和算力限制，与前端统一
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
#ifndef _HKANN_SCHEDULER_H_
#define _HKANN_SCHEDULER_H_

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <semaphore.h>

/***********************************************************************************************************************
* 跨平台调度器库公共错误码范围[0x8641 27FF ~ 0x8641 29FF]，共256 * 2个
***********************************************************************************************************************/
typedef enum _HKANN_SCHEDULE_ERROR_CODE_
{
    HKANN_SCHEDULER_ERR_RET_OK                  = 1,
    HKANN_SCHEDULE_ERROR_Enumerate_NPU          = 0x864127FF,               // 枚举NPU时错误，未在指定NPU范围内
    HKANN_SCHEDULE_ERROR_GetDevTaskQueueHandle  = 0x86412800,               // 根据设备类型获取对应handle错误
    HKANN_SCHEDULE_ERROR_ENUMERATE_NPU          = 0x86412230,               // 枚举NPU时错误，未在指定NPU范围内
    HKANN_SCHEDULE_ERROR_DEV_ID                 = 0x86412231,               // 核ID超过最大核数
    HKANN_SCHEDULE_ERROR_DEV_TYPE               = 0x86412232,               // 根据设备类型获取对应handle错误
    HKANN_SCHEDULER_ERR_LACK_MEM                = 0x86412233,
} HKANN_SCHEDULE_ERROR_CODE;

/***********************************************************************************************************************
* 宏定义
***********************************************************************************************************************/
//当前版本号
#define HKANN_SCHEDULER_MAJOR_VERSION        1     //主版本号
#define HKANN_SCHEDULER_SUB_VERSION          2     //副版本号
#define HKANN_SCHEDULER_REVISION_VERSION     4     //修订版本号
#define HKANN_SCHEDULER_ADDITION_VERSION     0     //补充版本号 

//版本日期
#define HKANN_SCHEDULER_VERSION_YEAR         2022  //年份
#define HKANN_SCHEDULER_VERSION_MONTH        02    //月份
#define HKANN_SCHEDULER_VERSION_DAY          11    //日期

/***********************************************************************************************************************
* 枚举及结构体
***********************************************************************************************************************/

typedef struct _HKANN_ARG_CTRL_T
{
   sem_t                    sem; 
   // 可以添加其他
}HKANN_ARG_CTRL_T;

// 设备类型
typedef enum _HKANN_DEV_TYPE_E
{
    HKANN_DEV_NPU0 = 0,
    HKANN_DEV_NPU1,
    HKANN_DEV_NPU2,
    HKANN_DEV_NPU3,
    HKANN_DEV_NPU4,
    HKANN_DEV_NPU5,
    HKANN_DEV_NUM
} HKANN_DEV_TYPE_E;

// 设备类型
typedef enum _HKANN_DEV_ID_E
{
    HKANN_DEV_ID0 = 0,
    HKANN_DEV_ID1,
    HKANN_DEV_ID2,
    HKANN_DEV_ID3,
    HKANN_DEV_ID4,
    HKANN_DEV_ID5,
    HKANN_DEV_ID6,
    HKANN_DEV_ID7,
    HKANN_DEV_ID8,
    HKANN_DEV_ID9,
    HKANN_DEV_ID_COUNT
} HKANN_DEV_ID_E;

typedef struct _HKANN_SCHEDULER_SPEC_T
{
    void           *npu_handle[HKANN_DEV_NUM];
    signed char     dev_id[HKANN_DEV_NUM][HKANN_DEV_ID_COUNT];
    pthread_t       tid;                     // 状态统计线程
    int             reserve[3];
} HKANN_SCHEDULER_SPEC_T;

// 设备参数
typedef struct _HKANN_DEV_DATA_T
{
    int                     dev_type;              // 处理核心类型
    int                     dev_id;                // 处理核心编号
    float                   per;                   // 算力占用比例
}HKANN_DEV_DATA_T;

//HKANN 调度器管理设备结构体
typedef struct _HKANN_SCHEDULER_MGR_PARAM_T
{
    HKANN_DEV_DATA_T  dev_data[HKANN_DEV_NUM * HKANN_DEV_ID_COUNT];     // 设备数组
    int               dev_num;                                          // 设备数量
    int               priority;                                         // 进程优先级
    int               state;                                            // 是否输出状态统计信息
} HKANN_SCHEDULER_MGR_PARAM_T;

// 进程优先级
typedef enum _HKANN_PRI_LEVEL_E
{
    HKANN_PRI_LOW = 0,
    HKANN_PRI_DEF = 8,
    HKANN_PRI_HIGH = 16
} HKANN_PRI_LEVEL_E;
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
int HKANN_CreateScheduler(HKANN_SCHEDULER_MGR_PARAM_T   *scheduler_param,
                          void                           **scheduler_handle);

/***********************************************************************************************************************
* 功  能：获取指定设备任务队列句柄
* 参  数：*
*            scheduler_handle       -I         调度器handle
*            dev_type               -I         设备类型
*            task_queue_handle      -O         任务队列handle
*            dev_id_ability         -I         当前core的最大可配置数
*            num                    -I         当前设备ID的序列号
* 返回值：状态码
***********************************************************************************************************************/
int HKANN_GetDevTaskQueueHandle(void                        *scheduler_handle,
                                HKANN_DEV_TYPE_E             dev_type,
                                void                       **task_queue_handle,
                                signed char                  dev_id_ability,
                                int                          num[]);

/***********************************************************************************************************************
* 功  能：销毁调度器
* 参  数：*
*            scheduler_handle       -I         调度器handle
* 返回值：状态码
***********************************************************************************************************************/
int HKANN_ReleaseScheduler(void                       *scheduler_handle);

void HKANN_Scheduler_Done(void *arg);

void HKANN_Scheduler_WaitDone(void *arg);


#ifdef __cplusplus 
}
#endif

#endif /* _HKANN_SCHEDULER_H_ */
