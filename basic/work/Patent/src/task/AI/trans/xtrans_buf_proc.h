/*
* @file   xtrans_buf_pro.h
* @note   HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
* @brief 码流缓存的处理，用于统一发送流程,对外头文件
* @author baoqingti
* @date 2021/7/16
* @note 历史记录:
* @note V4.0         新建   2021/7/16
* @warning 
*/

#ifndef __XTRANS_BUF_PROC_H__
#define __XTRANS_BUF_PROC_H__

#include "xtrans_port.h"
#include "xtrans_msg.h"


typedef enum
{
    XTRANS_INVALIDATE_TYPE  = 0,
    XTRANS_IPC_MAIN         = 1,   /*IPC叠框主码流*/
    XTRANS_IPC_SUB          = 2,   /*IPC叠框主码流*/
    XTRANS_ANOL_MAIN        = 3,   /*采集叠框主码流*/
    XTRANS_ANOL_SUB         = 4,   /*采集叠框子码流*/
    
    XTRANS_XRAY_TRANSFER    = 5,  /*xray 转存*/
    XTRANS_XRAY_PLAYBACK    = 6,  /*xray 回拉,和xray的采集互斥*/
    
}XTRANS_CHAN_TYPE_E;




/*DMA和CPU拷贝数据公共消息*/
typedef struct xtrans_msg_buf_stm_pack
{
    HPR_UINT32 u32BufSize;    /* 码流缓存长度 */
	HPR_UINT32 RES;
    uintptr_t uiptrPhyAddr; /* 主片物理地址 */
    void *pvVirAddr;        /* 主片虚拟地址 */
    HPR_INT8 reserve[8];
}xtrans_msg_buf_stm_pack_s;
#define XTRANS_MSG_BUF_STM_PACK_TOTEL_SIZE         (sizeof(xtrans_msg_buf_stm_pack_s))



typedef struct xtrans_data_info
{
	HPR_INT8 datainfo[3*1024];//32k数据，*数据内容的描述信息*

}xtrans_data_info_s;



#define XTRANS_BUF_HEAD_FALG  0xAFAFAFAF
typedef struct XTRANS_BUF_HEAD
{
    HPR_UINT32 u32Flag;               /* flag */
    HPR_UINT64 u64MsPts;              /* ms时间戳 */ 
    HPR_UINT32 u32TvSec;              /* 系统Second时间 */
    HPR_UINT32 u32TransTotalSize;     /* 传输总长度 */
    HPR_UINT32 u32BufRealSize;        /* 码流真实长度 */
    HPR_UINT32 u32Count;              /* 计数 */
	xtrans_stm_info_s stmInfo;/*告知码流地址信息；一般情况下不使用，PCIE会用下*/
	
	xtrans_data_info_s stBufDataInfo;/*数据内容的描述信息*/

	

    HPR_UINT8 reserve[16];
} XTRANS_BUF_HEAD_S;
 
#define XTRANS_BUF_HEAD_TOTEL_SIZE (sizeof(XTRANS_BUF_HEAD_S))



typedef struct xtrans_buf_info
{
    HPR_INT32 s32BufSize;
    uintptr_t uiptrPhyAddr;//不为0，需要使用物理地址
    void *pvVirAddr;//虚拟地址，优先使用物理地址
    HPR_UINT8 reserve[8];
}xtrans_buf_info_s;


#define XTRANS_MAX_STM_NUM            5

typedef struct xtrans_stm_attr//属性可以不填，也可以不用的
{
    HPR_INT32 s32BufNum;              /* 拷贝的buf数目 */
    HPR_UINT32 u32Count;              /* 码流计数，用于debug */
    HPR_UINT64 u64MsPts;              /* ms时间戳 */
    HPR_UINT32 u32TvSec;              /* 系统Second时间 */
	
	xtrans_data_info_s stBufDataInfo;//数据内容的描述信息
	xtrans_buf_info_s astBufInfo[XTRANS_MAX_STM_NUM];/* buf信息 */

    HPR_UINT8 reserve[16];
}xtrans_stm_attr_s;


typedef struct xtrans_buf_param
{
    HPR_UINT8 u8ModuelId;         /* 模块ID，xtrans_msg_module_id_e */
    xtrans_buf_info_s stFromBuf;
    xtrans_buf_info_s stToBuf;
    xtrans_port_s *pstSendStmPort;
    HPR_INT32 s32Align;           /* 传输对其字节数 */
    HPR_UINT8 reserve[16];
}xtrans_buf_param_s;

#ifdef __cplusplus
extern "c" {
#endif

/*
创建传输缓存处理模块
@pstParam [in] 传输缓存创建参数
@phHnd [out] 传输模块句柄
@phBufHnd [out] 传输的缓存句柄
*/
/* 支持多个模块共用一个port口 */
HPR_INT32 xtrans_bufPorcCreate(xtrans_buf_param_s *pstParam, HPR_HANDLE *phHnd, HPR_HANDLE *phBufHnd);

/*
销毁传输缓存处理模块
@hHnd [in] 传输模块句柄
@hBufHnd [in] 缓存句柄
*/
HPR_INT32 xtrans_bufProcDestroy(HPR_HANDLE hHnd, HPR_HANDLE hBufHnd);

/*
传输缓存，先拷贝到本地码流buffer中，然后由另一个线程统一传输到对端
@hHnd [in] 传输模块句柄
@hBufHnd [in] 模块缓存句柄
@pstBufInfo [in] 码流缓存信息
*/
HPR_INT32 xtrans_bufProcCopy(HPR_HANDLE hHnd, HPR_HANDLE hBufHnd, xtrans_stm_attr_s *pstBufInfo);


#ifdef __cplusplus
}
#endif


#endif

