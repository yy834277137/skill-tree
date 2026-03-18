
/** @file      ICF_MemPool.h
*   @note      HangZhou Hikvision Digital Technology Co., Ltd. All right reserved
*   @brief     内存池对外接口头文件
*   @version   0.9.0
*   @author    祁文涛
*   @date      2019/7/9
*   @note      初始版本
*/

#ifndef _ICF_MEMPOOL_H_
#define _ICF_MEMPOOL_H_

#include "ICF_base_v2.h"

#ifdef __cplusplus
extern "C" {
#endif
   
// 内存池模块成功返回码
#define ICF_MEMPOOL_OK              (0)

// 内存申请释放接口
#define MemPoolMemAlloc(pMemPoolHdl, pMemTab)         MemPoolAllocInter_V2(__FILE__, __FUNCTION__, __LINE__, pMemPoolHdl, pMemTab)
#define MemPoolMemFree(pMemPoolHdl, pMemTab, nSpace)  MemPoolFreeInter_V2(__FILE__, __FUNCTION__, __LINE__, pMemPoolHdl, pMemTab, nSpace)
#define MemPoolMemGetStatus  MemPoolMemGetStatus_V2


// 内存池内存对齐属性
typedef enum _ICF_MEM_ALIGNMENT_E_
{
    ICF_MEM_ALIGN_4BYTE    = 4,
    ICF_MEM_ALIGN_8BYTE    = 8,
    ICF_MEM_ALIGN_16BYTE   = 16,
    ICF_MEM_ALIGN_32BYTE   = 32,
    ICF_MEM_ALIGN_64BYTE   = 64,
    ICF_MEM_ALIGN_128BYTE  = 128,
    ICF_MEM_ALIGN_256BYTE  = 256,
    ICF_MEM_ALIGN_1024BYTE = 1024,
    ICF_MEM_ALIGN_4096BYTE = 4096,
    ICF_MEM_ALIGN_AMOUNT   = 0x7FFFFFFF
} ICF_STR_ALIGN_4 ICF_MEM_ALIGNMENT_E;

// 内存池吐出内存表的初始化枚举
typedef enum _ICF_MEM_CLEAN_E_
{
    ICF_MEM_VALUE_NOTCLEAN = 0,                       // 不清零吐出的内存表地址中数据
    ICF_MEM_VALUE_CLEAN    = 1                        // 清零吐出的内存表地址中数据
} ICF_STR_ALIGN_4 ICF_MEM_CLEAN_E;

// 内存用途枚举(目前规定最多100种类型)
typedef enum _ICF_MEM_USED_TYPE_
{
    // 框架预先定义 (0~49)
    ICF_MEM_USED_DEFAULT      = 0,       // 默认值

    // 静态
    ICF_MEM_USED_GLOBUF       = 1,       // 全局缓存
    ICF_MEM_USED_POS          = 2,       // POS内存
    ICF_MEM_USED_MODEL_FILE   = 3,       // 打开模型文件申请
    ICF_MEM_USED_MODEL_HANDLE = 4,       // 模型句柄
    ICF_MEM_USED_ALGO_HANDLE  = 5,       // 算子句柄

    // 动态
    ICF_MEM_USED_MEDIADATA    = 6,       // 输入数据
    ICF_MEM_USED_PACKAGE      = 7,       // 数据包
    ICF_MEM_USED_ALG_STRU     = 8,       // 算子结构体
    
    // 用户自定义用途(50-99)
    ICF_MEM_USED_END          = 99,      // 最大到99

} ICF_MEM_USED_TYPE;

// 内存设置相关接口
typedef struct _ICF_MEM_CONFIG_V2
{  
    int                nNum;                        /// 内存池中内存类型数量
    ICF_MEM_INFO_V2    stMemInfo[ICF_MEM_TYPE_NUM]; /// 不同类型内存信息,最大内存池类型数ICF_MEM_TYPE_NUM
    void              *pReserved[8];
    int                nReserved[8];
}ICF_STR_ALIGN_4 ICF_MEM_CONFIG_V2;
#define ICF_MEM_CONFIG ICF_MEM_CONFIG_V2

// 内存池memtab
typedef struct _ICF_MEM_TAB_
{
    long long                lSize;                   // 以BYTE为单位的内存大小
    ICF_MEM_ALIGNMENT_E      nAlignment;              // 内存对齐属性, 建议为128
    ICF_MEM_USED_TYPE        eUseType;                // 内存用途
    ICF_MEM_TYPE_E           nSpace;                  // 内存分配空间
    ICF_MEM_CLEAN_E          nClean;                  // 申请出的内存是否清空为0, 1为清空，0为不清。该参数不配置，则默认不清0；
    void                    *pBase;                   // 分配出的内存指针
    void                    *pPhyBase;                // 分配出的物理内存指针,HISI平台可用
    void                    *pReserved[8];
    int                      nReserved[8];
} ICF_STR_ALIGN_4 ICF_MEM_TAB_V2;
#define ICF_MEM_TAB ICF_MEM_TAB_V2

/** @fn     int MemPoolMemAlloc(ICF_MEM_TAB   *pMemTab)
*   @brief  内存池内存申请对外接口
*   @param  file         [in]            - 所在文件
*   @param  func         [in]            - 所在函数
*   @param  line         [in]            - 所在行数
*   @param  pMemPool     [in]            - 内存池句柄
*   @param  pMemTab      [in/out]        - 内存表 ICF_MEM_TAB_V2
*   @return int
*/

int ICF_EXPORT MemPoolAllocInter_V2(const char *file, const char *func, int line, void *pMemPool, void *pMemTab);

/** @fn     int MemPoolMemFree(void *pMemAddr, ICF_MEM_TYPE_E nSpace)
*   @brief  内存池内存释放对外接口
*   @param  file         [in]            - 所在文件
*   @param  func         [in]            - 所在函数
*   @param  line         [in]            - 所在行数
*   @param  pMemPool     [in]            - 内存池句柄
*   @param  pMemAddr     [in]            - 内存地址
*   @param  nSpace       [in]            - 内存类型
*   @return int
*/
int ICF_EXPORT MemPoolFreeInter_V2(const char *file, const char *func, int line, void *pMemPool, void *pMemAddr, ICF_MEM_TYPE_E nSpace);

/**@fn     int MemPoolMemGetStatus_V2
 * @brief  内存池状态查看对外接口
 * @param  pInitHandle        [in]     - 引擎句柄
 * @param  pMemPoolStatus     [out]    - 内存池状态
 * @return int
 */
int ICF_EXPORT MemPoolMemGetStatus_V2(void *pMemPool, ICF_MEMPOOL_STATUS_V2 pMemPoolStatus[ICF_MEM_TYPE_NUM]);

#ifdef __cplusplus
}
#endif

#endif /* _ICF_MEMPOOL_H_ */
