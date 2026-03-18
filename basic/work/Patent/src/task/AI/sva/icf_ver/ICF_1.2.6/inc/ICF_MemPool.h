
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

#include "ICF_base.h"

#ifdef __cplusplus
extern "C" {
#endif
   
// 内存池模块成功返回码
#define ICF_MEMPOOL_OK              (0)

// 内存池结构体4字节对齐宏
#if (defined(_WIN32) || defined(_WIN64))
#define MEMPOOL_STR_ALIGN_4
#else
#define MEMPOOL_STR_ALIGN_4         __attribute__ ((aligned (4)))  
#endif

// 内存申请释放接口
#define MemPoolMemAlloc(pInitHdl, pMemTab)         MemPoolAllocInter(__FILE__, __FUNCTION__, __LINE__, pInitHdl, pMemTab)
#define MemPoolMemFree(pInitHdl, pMemTab, nSpace)  MemPoolFreeInter(__FILE__, __FUNCTION__, __LINE__, pInitHdl, pMemTab, nSpace)


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
} MEMPOOL_STR_ALIGN_4 ICF_MEM_ALIGNMENT_E;

// 内存池吐出内存表的初始化枚举
typedef enum _ICF_MEM_CLEAN_E_
{
    ICF_MEM_VALUE_NOTCLEAN = 0,                       // 不清零吐出的内存表地址中数据
    ICF_MEM_VALUE_CLEAN    = 1                        // 清零吐出的内存表地址中数据
} MEMPOOL_STR_ALIGN_4 ICF_MEM_CLEAN_E;

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


// 内存池memtab
typedef struct _ICF_MEM_TAB_
{
    long long                lSize;                   // 以BYTE为单位的内存大小
    ICF_MEM_ALIGNMENT_E      nAlignment;              // 内存对齐属性, 建议为128
    ICF_MEM_USED_TYPE        eUseType;                // 内存用途
    ICF_MEM_TYPE_E           nSpace;                  // 内存分配空间
    ICF_MEM_CLEAN_E          nClean;                  // 申请出的内存是否清空为0, 1为清空，0为不清。该参数不配置，则默认不清0；
    void                     *pBase;                  // 分配出的内存指针
    void                     *pPhyBase;               // 分配出的物理内存指针,HISI平台可用
} MEMPOOL_STR_ALIGN_4 ICF_MEM_TAB;

/** @fn     int MemPoolInit(ICF_MEM_CONFIG *pMempoolConfig)
*   @brief  内存池初始化对外接口
*   @param  pMemPoolConfig      [in]      - 内存池初始化参数
*   @return int
*/
int MemPoolInit(ICF_MEM_CONFIG *pMemPoolConfig, int nStage, void *pInitHandle);

/** @fn     int MemPoolFInit()
*   @brief  内存池销毁对外接口
*   @param  pInitHandle        [in]     - 引擎句柄
*   @return int
*/
int MemPoolFinit(void *pInitHandle);

/** @fn     int MemPoolMemAlloc(ICF_MEM_TAB   *pMemTab)
*   @brief  内存池内存申请对外接口
*   @param  file         [in]            - 所在文件
*   @param  func         [in]            - 所在函数
*   @param  line         [in]            - 所在行数
*   @param  pMemTab      [in/out]        - 内存表
*   @return int
*/

int MemPoolAllocInter(const char *file, const char *func, int line, void *pInitHandle, ICF_MEM_TAB *pMemTab);

/** @fn     int MemPoolMemFree(void *pMemAddr, ICF_MEM_TYPE_E nSpace)
*   @brief  内存池内存释放对外接口
*   @param  file         [in]            - 所在文件
*   @param  func         [in]            - 所在函数
*   @param  line         [in]            - 所在行数
*   @param  pMemAddr     [in]            - 内存地址
*   @param  nSpace       [in]            - 内存类型
*   @return int
*/
int MemPoolFreeInter(const char *file, const char *func, int line, void *pInitHandle, void *pMemAddr, ICF_MEM_TYPE_E nSpace);

/**@fn     int MemPoolMemGetStatus(ICF_MEMPOOL_STATUS  pMemPoolStatus[ICF_MEM_TYPE_NUM])
 * @brief  内存池状态查看对外接口
 * @param  pInitHandle        [in]     - 引擎句柄
 * @param  pMemPoolStatus     [out]    - 内存池状态
 * @return int
 */
int MemPoolMemGetStatus(void *pInitHandle, ICF_MEMPOOL_STATUS  pMemPoolStatus[ICF_MEM_TYPE_NUM]);

/**@fn       ICF_MEM_MemInfoAAddB
 * @brief    用于两个ICF_MEM_INFO相加，比如多个GraphType下内存池设置
 * @param    stMemInfoA     [in]    - 内存信息A
 * @param    stMemInfoB     [in]    - 内存信息B
 * @param    stMemInfoOut   [out]   - A+B对应类型相加,支持A=A+B
 * @return   错误码
 * @note
 */
int ICF_MEM_MemInfoAAddB(const ICF_MEM_INFO stMemInfoA[ICF_MEM_TYPE_NUM],
                         const ICF_MEM_INFO stMemInfoB[ICF_MEM_TYPE_NUM],
                         ICF_MEM_INFO stMemInfoOut[ICF_MEM_TYPE_NUM]);
#ifdef __cplusplus
}
#endif

#endif /* _ICF_MEMPOOL_H_ */