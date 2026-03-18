#ifndef _DSPTTK_CMD_DEC_H_
#define _DSPTTK_CMD_DEC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "dspcommon.h"
#include "sal_type.h"

/* DEC解码进度信息 */
typedef struct
{
    DSPTTK_COND_T cond; // 通知解码进度值更新
    UINT32 au32UpdatedFlag[MAX_VDEC_CHAN]; // 各通道的解码进度是否有更新，每更新一次该值+1
    UINT32 u32TotalTime[MAX_VDEC_CHAN]; // 各通道录像码流总时长，单位：秒，0表示当前解码通道关闭
    UINT32 au32Progress[MAX_VDEC_CHAN]; // 当前录像解码进度，范围[0, 100]
} DSPTTK_DEC_PROGRESS;

/* DEC解码关键进度节点信息 */
typedef struct
{
    UINT32 u32Percent;  //当前结构体中记录的时间戳进度([0%, 90%]每10个百分比记录一次)
    UINT64 u64Offset;   //在文件中的偏移量
    BOOL bUpdate;       //当前进度条是否已更新
} DSPTTK_DEC_PROGRESS_KEY_POINT;

typedef struct DSPTTK_DEC_MULTI_PTHREAD
{
    pthread_t StartDecThread[MAX_VDEC_CHAN];
    pthread_t decProgressFindIdx;
    pthread_t decProgressUpdateThread;
} DSPTTK_DEC_MULTI_PTHREAD;

typedef struct DSPTTK_RECODE_MULTI_PTHREAD
{
    pthread_t startRecodeSaveThread[MAX_VIPC_CHAN];
    pthread_t saveStreamFromRecodeNetThread[MAX_VIPC_CHAN];
    pthread_t saveStreamFromRecodeRecThread[MAX_VIPC_CHAN];
} DSPTTK_RECODE_MULTI_PTHREAD;

typedef struct DSPTTK_DEC_CHAN_CTL
{
    INT32 s32BackChan;
    INT32 s32CurChan;
    INT32 szEnChan[MAX_VDEC_CHAN];
    pthread_mutex_t backChanLock;
    pthread_mutex_t curChanLock;
} DSPTTK_DEC_CHAN_CTL;

typedef struct DSPTTK_RECODE_CHAN_CTL
{
    INT32 s32CurChan;
    pthread_mutex_t curChanLock;
}DSPTTK_RECODE_CHAN_CTL;

typedef struct DSPTTK_PROGRESS_CTL
{
    DSPTTK_DEC_PROGRESS stDecProgress;
    DSPTTK_DEC_PROGRESS_KEY_POINT stDecProgressCurFile;
    DSPTTK_DEC_PROGRESS_KEY_POINT stDecProgressKeyPoint[10];
    DSPTTK_DEC_PROGRESS stDecProgressTrans;
    UINT64 u64CurFileMaxTimeStamp;
    DSPTTK_COND_T stCondDecProgress;
    DSPTTK_COND_T stCondDecProgressCurFile;
    DSPTTK_COND_T stCondDecProgressKeyPoint;
    DSPTTK_COND_T stCondDecProgressTrans;
    pthread_mutex_t curFileMaxTimeStampLock;
}DSPTTK_PROGRESS_CTL;

/**
 * @fn      dspttk_dec_jump
 * @brief   解码跳转功能
 *
 * @param   [IN] u32Chan  解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_jump(UINT32 u32Chan);

/**
 * @fn      dspttk_dec_sync
 * @brief   解码同步功能
 *
 * @param   [IN] u32Chan  解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_sync(UINT32 u32Chan);

/**
 * @fn      dspttk_dec_set_muxtype
 * @brief   设置dec的muxtype
 *
 * @param   [IN] u32Chan  解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_set_muxtype(UINT32 u32Chan);

/**
 * @fn      dspttk_dec_set_attr
 * @brief   设置dec的speed和mode
 *
 * @param   [IN] u32Chan  解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_set_attr(UINT32 u32Chan);

/**
 * @fn      dspttk_dec_recode_start
 * @brief   开启转包功能
 *
 * @param   [IN] u32Chan  转包通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_recode_start(UINT32 u32Chan);

/**
 * @fn      dspttk_dec_update_progress
 * @brief   更新Web Dec页面中解码的进度
 * 
 * @param   [IN] u32Chan 解码通道号，取值范围[0, MAX_VDEC_CHAN-1]
 * @param   [IN] u32TotalTime 当前录像码流总时长，单位：秒，0表示当前通道解码关闭
 * @param   [IN] u32Progress 当前录像解码进度，范围[0, 100]
 */
VOID dspttk_dec_update_progress(UINT32 u32Chan, UINT32 u32TotalTime, UINT32 u32Progress);

/**
 * @fn      dspttk_enable_dec_channel
 * @brief   使能解码通道状态
 *
 * @param   [IN] u32Chan  转包通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_enable_dec_channel(UINT32 u32Chan);

/**
 * @fn      dspttk_dec_status_init
 * @brief   状态初始化
 *
 * @param   无
 *
 * @return  无
 */
void dspttk_dec_status_init();

/**
 * @fn      dspttk_dec_btn_start
 * @brief   解码栏start按钮绑定函数
 *
 * @param   [IN] u32Chan 解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_btn_start(UINT32 u32Chan);

/**
 * @fn      dspttk_dec_btn_pause
 * @brief   解码栏pause按钮绑定函数
 *
 * @param   [IN] u32Chan 解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_btn_pause(UINT32 u32Chan);

/**
 * @fn      dspttk_dec_btn_stop
 * @brief   解码栏stop按钮绑定函数
 *
 * @param   [IN] u32Chan 解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_btn_stop(UINT32 u32Chan);

UINT64 dspttk_dec_chan_enable(UINT32 u32Chan);
#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_CMD_DEC_H_ */
