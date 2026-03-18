#ifndef _XTRANS_MGR_H
#define _XTRANS_MGR_H

#include "xtrans_sys.h"
#include "HPR_Types.h"

/*!< MGR名称字符串最大长度 */
#define XTRANS_MGR_NAME_MAX_SIZE    20

typedef HPR_INT32 (*XTRANS_MGR_SERVER_PROC_PF)(HPR_HANDLE);

/*!< MGR服务信息 */
typedef struct xtrans_mgr_server_info
{
    char acModuleName[XTRANS_MGR_NAME_MAX_SIZE];    /*!< 服务所在模块名称 */
    char acServerName[XTRANS_MGR_NAME_MAX_SIZE];    /*!< 服务名称 */
    HPR_INT32 *ps32ExitFlag;  /*!< 服务退出标志位 */
    HPR_INT32 s32SleepMs;     /*!< 服务睡眠时间，默认10ms，要求服务外部不做休眠 */
    XTRANS_MGR_SERVER_PROC_PF pfServerProc;    /*!< 服务运行函数 */
}xtrans_mgr_server_info_s;

/*!< MGR设备结构体 */
typedef struct XTRANS_MGR_S
{
    void *Private;
    
    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 搜索设备
     
     \param [in] hDesc 指向Desc结构体的指针
     \param [out] phDev 指向设备结构体指针的指针
     \param [in] Thiz 指向设备管理结构体的指针
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*FindDev)(const HPR_HANDLE hDesc, HPR_HANDLE *phDev, struct XTRANS_MGR_S *Thiz);
    
    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 设备管理添加新的设备

     \param [in] hDesc 指向Desc结构体的指针
     \param [in] hDev 指向设备结构体的指针
     \param [in] Thiz 指向设备管理结构体的指针
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败

     \note: mgr保存desc和dev指针，desc和dev内存必须为外部创建，如动态创建或静态变量，否则会出错
     \note: 若dev已插入，则mgr内部dev引用计数加1，在del设备时，引用计数减1
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*InsterDev)(const HPR_HANDLE hDesc, const HPR_HANDLE hDev, struct XTRANS_MGR_S *Thiz);
    
    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 设备管理删除设备
     
     \param [in] hDev 指向设备结构体的指针
     \param [in] Thiz 指向设备管理结构体的指针
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败

     \note: del设备时，引用计数减1，直至引用计数为0，表明设备可以被销毁，则从mgr中销毁，返回HPR_OK
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*DelDev)(const HPR_HANDLE hDev, struct XTRANS_MGR_S *Thiz);

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 设备管理获取所有设备
     
     \param [out] ahDev 获取到设备结构体的数组
     \param [out] ps32Cnt 获取到的设备个数
     \param [in] Thiz 指向设备管理结构体的指针
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败

     \note: 内部没有保护机制，只能获取到当前设备信息，之后可能会被改掉
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*GetDev)(HPR_HANDLE ahDev[], HPR_INT32 *ps32Cnt, struct XTRANS_MGR_S *Thiz);

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 注册服务
     
     \param [in] pstInfo 注册的服务信息
     \param [in] Thiz 指向设备管理结构体的指针
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败

     \note: 内部没有保护机制，只能获取到当前设备信息，之后可能会被改掉
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*RegisterServer)(xtrans_mgr_server_info_s *pstInfo, struct XTRANS_MGR_S *Thiz);
}XTRANS_MGR_S;


/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */


/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 创建模块设备公共管理
 
 \param [in] s32DevMaxNum 模块设备最大个数
 \param [in] s32DescSize 模块设备匹配符结构体大小
 \param [out] Thiz 指向管理模块结构体指针
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_MGR_Create(HPR_INT32 s32DevMaxNum, HPR_INT32 s32DescSize, XTRANS_MGR_S **ppstMgr);


/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 销毁模块设备公共管理
 
 \param [in] Thiz 指向管理模块结构体指针
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_MGR_Destroy(XTRANS_MGR_S *pstMgr);


/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif 

