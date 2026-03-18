
#ifndef _DSPTTK_CALLBACK_H_
#define _DSPTTK_CALLBACK_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "sal_type.h"
#include "dspttk_cmd.h"
#include "dspttk_sqlite.h"

#define DSPTTK_SEG_CNT_FOR_WEB      300      // 节点存储的300个分片信息
#define SEG_CNT_MAX      64                 // 存储的最大分片数

/* 数据库表slice的元素信息 */
typedef struct
{
    UINT64 u64No;           // 条带序号，>0，即0为非法值
    UINT32 u32Width;        // 条带宽
    UINT32 u32Height;       // 条带高
    UINT32 u32Size;         // 条带NRAW数据大小，单位：字节
    CHAR sPath[128];        // 条带NRAW数据保存位置
    UINT64 u64Time;         // 条带输入时间
    XSP_SLICE_CONTENT enContent; // 包裹条带或空白条带
} DB_TABLE_SLICE;

/* 数据库表package的元素信息 */
typedef struct
{
    UINT64 u64Id;               // 包裹序号，>0，即0为非法值
    UINT32 u32Width;            // 包裹宽
    UINT32 u32Height;           // 包裹高
    UINT64 u64SliceStart;       // 包裹起始条带号
    UINT64 u64SliceEnd;         // 包裹结束条带号
    UINT64 u64TimeEnd;          // 包裹结束时间，系统启动后的相对时间
    UINT64 u64TimeCb;           // 包裹回调时间，系统启动后的相对时间
    UINT32 u32AiDgrNum;         // AI危险品识别结果数
    UINT32 u32XrIdtNum;         // XSP难穿透&可疑有机物识别结果数
    XRAY_PROC_DIRECTION enDir;  // 过包显示方向
    BOOL bVMirror;              // 是否垂直镜像
    CHAR sInfoPath[128];        // 包裹信息的保存路径，包裹信息对应结构体XPACK_RESULT_ST
    CHAR sJpgPath[128];         // 包裹JPG图片保存路径
} DB_TABLE_PACKAGE;

/**
 * 分片回调，存储着当前发送给web端的300个最新的分片
 */
typedef struct
{
    char sSegImgPath[128];
    UINT32 u32SegmentIndx;             /* 分片序号 */
    UINT32 u32SegmentWidth;                 /* 回调图片宽 */
    UINT32 u32SegmentHeight;                /* 回调图片高 */
    XPACK_SAVE_TYPE_E SegmentDataTpye;       /* 分片数据类型 */
} DSPTTK_SEG_IMG_DATA;

/**
 * 分片回调，存储着当前发送给web端的300个最新的分片
 */
typedef struct
{
    char sSegDir[128];
    XRAY_PROC_DIRECTION  Direction;          /* 实时过包方向 */
    UINT32 u32SegPackNo;                // 包裹号，0为非法
    UINT32 u32SegPackNum;               // 包裹总共分多少个分片
    UINT32 u32SegPackW;                // 包裹宽度
    UINT32 u32SegPackH;                // 包裹高度
    UINT64 u64SegPackTime;
    DSPTTK_SEG_IMG_DATA stSegImgData[SEG_CNT_MAX];
} DSPTTK_SEG_DATA_NODE;

/**
 * 分片回调，存储着当前发送给web端的300个最新的分片
 */
typedef struct
{
    DSPTTK_LIST *lstSeg; // 当前屏幕上的条带号链表，以正向回拉为基准，从头节点到尾节点条带号依次递减
    DSPTTK_SEG_DATA_NODE stSegNode[DSPTTK_SEG_CNT_FOR_WEB];

} SEGMENT_CB_FOR_WEB;

typedef struct
{
    DB_HANDLE pHandle; // 数据库句柄
    CHAR sName[32]; // 数据库表名，使用单库单表
    pthread_rwlock_t rwlock; // 数据库读写锁
    UINT64 u64PrimKeyBase; // 数据库中Primary Key基值
} DB_TABLE_ATTR;

/**
 * @function:   dspttk_get_seg_prm_for_web
 * @brief:     分片参数回调
 * @param[in]:  void
 * @param[out]: None
 * @return:     SEGMENT_CB_FOR_WEB *全局变量结构体指针
 */
SEGMENT_CB_FOR_WEB *dspttk_get_seg_prm_for_web(UINT32 u32Chan);

/**
 * @fn      dspttk_query_slice_free
 * @brief   释放dspttk_query_slice_xxx()类查询接口输出的条带记录Buffer
 * 
 * @param   [IN] pstSlice dspttk_query_slice_xxx()类查询接口输出的条带记录Buffer
 */
void dspttk_query_slice_free(DB_TABLE_SLICE *pstSlice);

/**
 * @fn      dspttk_query_slice_all
 * @brief   从条带数据库中查询所有的条带记录 
 * @warning 该接口内部有申请内存（pstSlice）的操作，使用完成后需要调用dspttk_query_slice_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstSlice 查询得到的条带记录，数组形式，内存中以DB_TABLE_SLICE连续存放
 * @param   [OUT] pu32Num 查询得到的条带记录条目数
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_slice_all(UINT32 chan, DB_TABLE_SLICE **pstSlice, UINT32 *pu32Num);

/**
 * @fn      dspttk_query_slice_lastest_n
 * @brief   从条带数据库中查询最新N个条带记录 
 * @warning 该接口内部有申请内存（pstSlice）的操作，使用完成后需要调用dspttk_query_slice_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstSlice 查询得到的条带记录，数组形式，内存中以DB_TABLE_SLICE连续存放
 * @param   [IN/OUT] pu32Num 输入需要查询的记录条目数（需大于0），输出查询得到的条带记录条目数
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_slice_lastest_n(UINT32 chan, DB_TABLE_SLICE **pstSlice, UINT32 *pu32Num);

/**
 * @fn      dspttk_query_slice_assigned_n
 * @brief   从条带数据库中查询指定条带号开始的N个条带记录（不包含该条带本身）
 * @warning 该接口内部有申请内存（pstSlice）的操作，使用完成后需要调用dspttk_query_slice_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstSlice 查询得到的条带记录，数组形式，内存中以DB_TABLE_SLICE连续存放
 * @param   [IN/OUT] pu32Num 输入需要查询的记录条目数（需大于0），输出查询得到的条带记录条目数
 * @param   [IN] u64NoStart 开始条带号，查询结果中不包含该条带本身
 * @param   [IN] bReversed 是否逆序查询，TRUE：逆序，查询条带号小于u64NoStart的记录，FALSE：顺序，查询条带号大于u64NoStart的记录
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_slice_assigned_n(UINT32 chan, DB_TABLE_SLICE **pstSlice, UINT32 *pu32Num, UINT64 u64NoStart, BOOL bReversed);

/**
 * @fn      dspttk_query_slice_assigned_n
 * @brief   从条带数据库中查询条带号范围内的记录
 * @warning 该接口内部有申请内存（pstSlice）的操作，使用完成后需要调用dspttk_query_slice_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstSlice 查询得到的条带记录，数组形式，内存中以DB_TABLE_SLICE连续存放
 * @param   [OUT] pu32Num 查询得到的条带记录条目数
 * @param   [IN] u64NoStart 起始条带号，可大于u64NoEnd，当大于u64NoEnd时，逆序查询
 * @param   [IN] u64NoEnd 结束条带号，可小于u64NoStart，当小于u64NoStart时，逆序查询
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_slice_by_range(UINT32 chan, DB_TABLE_SLICE **pstSlice, UINT32 *pu32Num, UINT64 u64NoStart, UINT64 u64NoEnd);

/**
 * @fn      dspttk_query_package_free
 * @brief   释放dspttk_query_package_xxx()类查询接口输出的包裹记录Buffer
 * 
 * @param   [IN] pstPackage dspttk_query_package_xxx()类查询接口输出的包裹记录Buffer
 */
void dspttk_query_package_free(DB_TABLE_PACKAGE *pstPackage);

/**
 * @fn      dspttk_query_package_all
 * @brief   从包裹数据库中查询所有的包裹记录 
 * @warning 该接口内部有申请内存（pstPackage）的操作，使用完成后需要调用dspttk_query_package_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstPackage 查询得到的包裹记录，数组形式，内存中以DB_TABLE_PACKAGE连续存放
 * @param   [OUT] pu32Num 查询得到的包裹记录条目数
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_package_all(UINT32 chan, DB_TABLE_PACKAGE **pstPackage, UINT32 *pu32Num);

/**
 * @fn      dspttk_query_package_lastest_n
 * @brief   从包裹数据库中查询最新N个包裹记录 
 * @warning 该接口内部有申请内存（pstPackage）的操作，使用完成后需要调用dspttk_query_package_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstPackage 查询得到的包裹记录，数组形式，内存中以DB_TABLE_PACKAGE连续存放
 * @param   [IN/OUT] pu32Num 输入需要查询的记录条目数（需大于0），输出查询得到的包裹记录条目数
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_package_lastest_n(UINT32 chan, DB_TABLE_PACKAGE **pstPackage, UINT32 *pu32Num);

/**
 * @fn      dspttk_query_package_assigned_n
 * @brief   从包裹数据库中查询指定包裹号开始的N个包裹记录（不包含该包裹本身）
 * @warning 该接口内部有申请内存（pstPackage）的操作，使用完成后需要调用dspttk_query_package_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstPackage 查询得到的包裹记录，数组形式，内存中以DB_TABLE_PACKAGE连续存放
 * @param   [IN/OUT] pu32Num 输入需要查询的记录条目数（需大于0），输出查询得到的包裹记录条目数
 * @param   [IN] u64IdStart 开始包裹号，查询结果中不包含该包裹本身
 * @param   [IN] bReversed 是否逆序查询，TRUE：逆序，查询包裹号小于u64IdStart的记录，FALSE：顺序，查询包裹号大于u64IdStart的记录
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_package_assigned_n(UINT32 chan, DB_TABLE_PACKAGE **pstPackage, UINT32 *pu32Num, UINT64 u64IdStart, BOOL bReversed);

/**
 * @fn      dspttk_query_package_by_sliceno
 * @brief   从包裹数据库中查询指定条带号范围内涉及的所有包裹记录 
 *          比如#1包裹条带号范围[10, 20]，#2包裹条带号范围[30, 40]，查找条带号范围[15, 32]，则输出#1和#2包裹 
 * @warning 该接口内部有申请内存（pstPackage）的操作，使用完成后需要调用dspttk_query_package_free()释放 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstPackage 查询得到的包裹记录，数组形式，内存中以DB_TABLE_PACKAGE连续存放
 * @param   [OUT] pu32Num 查询得到的包裹记录条目数
 * @param   [IN] u64SliceNoStart 起始条带号，可大于u64SliceNoEnd，当大于u64SliceNoEnd时，逆序查询
 * @param   [IN] u64SliceNoEnd 结束条带号，可小于u64SliceNoStart，当小于u64SliceNoStart时，逆序查询
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_package_by_sliceno(UINT32 chan, DB_TABLE_PACKAGE **pstPackage, UINT32 *pu32Num, UINT64 u64SliceNoStart, UINT64 u64SliceNoEnd);

/**
 * @fn      dspttk_query_package_by_packageid
 * @brief   从包裹数据库中查询指定包裹ID的记录 
 * 
 * @param   [IN] chan XRay通道号
 * @param   [OUT] pstPackage 查询得到的单个包裹记录
 * @param   [IN] u64PackageId 包裹ID号
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_query_package_by_packageid(UINT32 chan, DB_TABLE_PACKAGE *pstPackage, UINT64 u64PackageId);

#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_CALLBACK_H_ */

