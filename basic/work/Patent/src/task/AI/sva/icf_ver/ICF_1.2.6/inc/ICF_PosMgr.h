/** @file      PosMgr.h
*   @note      HangZhou Hikvision Digital Technology Co., Ltd. All right reserved
*   @brief     Pos Mgr模块函数声明
*   @version   0.9.0
*   @author    曹贺磊
*   @date      2019/10/28
*   @note      初始版本
*/

#ifndef _ICF_POSMGR_H_
#define _ICF_POSMGR_H_

#include <stdio.h>
#include "ICF_base.h"

#ifdef __cplusplus
extern "C" {
#endif

// 宏定义
#define MAX_POS_GRAPH_NUM                     (16)                                    // 最大graph数目
#define MAX_POS_NODE_NUM                      (128)                                   // 最大节点数量
#define POS_FRAME_FLAG_NODEINDEX              (-1)                                    // 框架标志nodeIndex

// 定义返回结构体,供用户解析
typedef struct _ICF_POSMGR_HANDLE_
{
    int         curPos;                                                               // 当前位置
    int         slotSize;                                                             // 每个slot大小（写入结构体大小）
    int         slotNum;                                                              // 总的slot个数
    int         posBufSize;                                                           // buf大小
    int         posBufMemSpace;                                                       // 内存类型
    void       *posBufPtr;                                                            // POS信息指针
}ICF_POSMGR_HANDLE;

// POS PKG信息
typedef struct _POS_PACKAGE_INFO_
{
    int                   GraphType;                                                  // GraphType
    int                   GraphID;                                                    // Graph ID
    int                   NodeNum;                                                    // 写入Node个数
    int                   packageNodeIndex[MAX_POS_NODE_NUM];                         // 内存各个slot对应node号
    ICF_POSMGR_HANDLE     handle;                                                     // POS信息句柄
}POS_PACKAGE_INFO;

// 输出POS信息数据
typedef struct _POS_BUFFER_PTR_
{
    // 框架层pos信息
    ICF_POSMGR_HANDLE      frame_pos_buf_info;                    // POS信息handle

    // 封装层pos信息
    int                    posInfo_Graph_Num;                     // 写入Graph通道个数
    POS_PACKAGE_INFO       package_Pos_Info[MAX_POS_GRAPH_NUM];   // PKG信息
}POS_BUF_OUT_PTR;


/** @fn  int ICF_Pos_AddPosInfo 
 *  @brief  <Pos模块信息写入接口>
 *  @param  pInitHandle       [in]  ICF初始化句柄
 *  @param  package           [in]  封装层使用时传入package指针，用于获取graphID；框架层不使用，传入NULL即可
 *  @param  algtype           [in]  框架层使用时，传入特定标志值
 *  @param  data              [in]  写入数据
 *  @param  dataSize          [in]  写入数据的大小
 *  @return 错误码
*/
int ICF_Pos_AddPosInfo(void *pInitHandle, void *package[], int algtype, void *data, int dataSize);

/** @fn  int ICF_Pos_GetPosBufPtr 
 *  @brief  <Pos模块信息获取接口>
 *  @param  pInitHandle       [in]  ICF初始化句柄
 *  @param  posBufPtr         [in]  Pos模块缓存信息结构体指针
 *  @return 错误码
*/
int ICF_Pos_GetPosBufPtr(void *pInitHandle, POS_BUF_OUT_PTR *posBufPtr);

/** @fn  int ICF_PosDump 
 *  @brief  <Pos模块信息打印接口（仅打印顶层信息，不打印具体写入数据）>
 *  @param  pInitHandle   [in]  ICF初始化句柄
 *  @param  dst           [in]  通过ICF_Pos_GetPosBufPtr接口获取的指针
 *  @param  size          [in]  dst指针对应结构体大小
 *  @return 错误码
*/
int ICF_PosDump(void *pInitHandle, void *dst, int size);

#ifdef __cplusplus 
}
#endif

#endif /* _ICF_POSMGR_H_ */
