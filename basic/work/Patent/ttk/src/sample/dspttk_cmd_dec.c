/**
 * @file    dspttk_cmd_dec.c
 * @brief
 * @note
 */

/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */

#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_devcfg.h"
#include "dspttk_init.h"
#include "dspttk_util.h"
#include "dspttk_cmd.h"
#include "dspcommon.h"
#include "dspttk_fop.h"
#include "sal_type.h"
#include "dspttk_cmd_dec.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */

/* 解码状态struct, 每个状态对应三种操作:开始、暂停和停止 */
typedef struct DSPTTK_DEC_STATE
{
    void (*start)();
    void (*pause)();
    void (*stop)();
} DSPTTK_DEC_STATE;

/* 转封装状态struct，每个状态对应两种操作：开始、停止 */
typedef struct DSPTTK_RECODE_STATE
{
    void (*start)();
    void (*stop)();
}DSPTTK_RECODE_STATE;

typedef struct DSPTTK_RECODE_CUR_FILE_STATE
{
    BOOL bRecodeWriteToPsFile[MAX_VIPC_CHAN];
    BOOL bRecodeWriteToRtpFile[MAX_VIPC_CHAN];
    BOOL bReadToIPCBuffer[MAX_VIPC_CHAN];
    pthread_mutex_t writeToPsLock;
    pthread_mutex_t writeToRtpLock;
    pthread_mutex_t readToBufferLock;
    DSPTTK_COND_T stCondRecodeCurFile;
}DSPTTK_RECODE_CUR_FILE_STATE;

#define MAX_FILENAME_SIZE (256)
#define MAX_BUFFER_SIZE (9000)

/* 从文件中写数据到DEC_SHARE_BUF的方向(读取文件的方向：文件头|文件尾) */
typedef enum dspttk_dec_share_buf_direction
{
    DSPTTK_DEC_READ_FROM_HEAD = 0,  //从文件头开始读数据
    DSPTTK_DEC_READ_FROM_TAIL = 1   //从文件尾开始读数据
}DSPTTK_DEC_SHARE_BUF_DIRECTION;

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/* 解码功能函数 */
void dspttk_dec_start();  // 状态对应功能函数, 开始播放
void dspttk_dec_stop();   // 停止播放
void dspttk_dec_pause();  // 暂停播放
void dspttk_dec_null();   // 空操作
void dspttk_dec_resume(); // 恢复播放
void dspttk_decode_progress_update(UINT32 u32Chan, UINT32 u32TotalTime);

/* 转封装功能函数 */
void dspttk_recode_start();     //转封装对应功能函数， 开始转封装
void dspttk_recode_stop();      //停止转封装
void dspttk_recode_null();
/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */

/* 解码状态 */
/* 空闲状态时,暂停与停止功能无效,start会开始解码 */
DSPTTK_DEC_STATE stIdle = {
    dspttk_dec_start,
    dspttk_dec_null,
    dspttk_dec_null};

/* 正在解码状态时, 开启功能无效, pause对应暂停解码, stop对应停止解码 */
DSPTTK_DEC_STATE stDecoding = {
    dspttk_dec_null,
    dspttk_dec_pause,
    dspttk_dec_stop};

/* 暂停状态时, 暂停功能无效, start对应恢复解码, stop对应停止解码 */
DSPTTK_DEC_STATE stPausing = {
    dspttk_dec_resume,
    dspttk_dec_null,
    dspttk_dec_stop};

DSPTTK_DEC_STATE *gpCurrent[MAX_VDEC_CHAN] = {NULL}; // 定义解码对应通道状态指针, 保存对应通道当前状态
pthread_mutex_t gMutexCurrentState = PTHREAD_MUTEX_INITIALIZER;
CHAR gszCurFileName[MAX_FILENAME_SIZE] = {0};
DSPTTK_DEC_MULTI_PTHREAD gstDecMultiPthreadCtl = {0};
DSPTTK_DEC_CHAN_CTL gstDecChanCtl = {0};
DSPTTK_PROGRESS_CTL gstDecProgressCtl = {0};
BOOL bJump[MAX_VDEC_CHAN] = {SAL_FALSE};




/* 转封装状态 */
/* 空闲状态时，停止功能无效 */
DSPTTK_RECODE_STATE stRecodeStop = {
    dspttk_recode_start,
    dspttk_recode_null};

DSPTTK_RECODE_STATE stRecodeStart = {
    dspttk_recode_null,
    dspttk_recode_stop};
DSPTTK_RECODE_MULTI_PTHREAD gstRecodeMultiPthreadCtl = {0};
DSPTTK_RECODE_CUR_FILE_STATE gstRecodeCurFileState = {0};
DSPTTK_RECODE_CHAN_CTL gstRecodeChanCtl = {0};
DSPTTK_RECODE_STATE *gpCurrentRecode[MAX_VIPC_CHAN] = {NULL};  //定义转封装对应通道状态指针，保存对应通道当前状态
pthread_mutex_t gMutexCurrentRecodeState = PTHREAD_MUTEX_INITIALIZER;



/**
 * @fn      dspttk_dec_update_progress
 * @brief   更新Web Dec页面中解码的进度
 *
 * @param   [IN] u32Chan 解码通道号，取值范围[0, MAX_VDEC_CHAN-1]
 * @param   [IN] u32TotalTime 当前录像码流总时长，单位：秒，0表示当前通道解码关闭
 * @param   [IN] u32Progress 当前录像解码进度，范围[0, 100]
 */
VOID dspttk_dec_update_progress(UINT32 u32Chan, UINT32 u32TotalTime, UINT32 u32Progress)
{
    BOOL bSignal = SAL_FALSE;
    DSPTTK_COND_T condZero = {0};

    if (0 == memcmp(&condZero, &gstDecProgressCtl.stCondDecProgress, sizeof(DSPTTK_COND_T)))
    {
        return;
    }
    if (u32Chan >= MAX_VDEC_CHAN)
    {
        return;
    }

    dspttk_mutex_lock(&gstDecProgressCtl.stCondDecProgress.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    if (u32TotalTime)
    {
        if (gstDecProgressCtl.stDecProgress.au32Progress[u32Chan] != SAL_MIN(u32Progress, 100))
        {
            gstDecProgressCtl.stDecProgress.au32UpdatedFlag[u32Chan]++;
            gstDecProgressCtl.stDecProgress.u32TotalTime[u32Chan] = u32TotalTime;
            gstDecProgressCtl.stDecProgress.au32Progress[u32Chan] = SAL_MIN(u32Progress, 100);
            bSignal = SAL_TRUE;
        }
    }
    else
    {
        if (gstDecProgressCtl.stDecProgress.u32TotalTime[u32Chan]) // 从开启到关闭，复位进度条
        {
            gstDecProgressCtl.stDecProgress.au32UpdatedFlag[u32Chan]++;
            gstDecProgressCtl.stDecProgress.u32TotalTime[u32Chan] = 0;
            gstDecProgressCtl.stDecProgress.au32Progress[u32Chan] = 0;
            bSignal = SAL_TRUE;
        }
    }
    if (bSignal)
    {
        dspttk_cond_signal(&gstDecProgressCtl.stCondDecProgress, SAL_TRUE, __FUNCTION__, __LINE__);
    }
    dspttk_mutex_unlock(&gstDecProgressCtl.stCondDecProgress.mid, __FUNCTION__, __LINE__);

    return;
}

/* 解码相关回调函数 */
/**
 * @fn      dspttk_dec_sharebuffer_operate
 * @brief   sharebuffer控制函数(从sharebuffer中正向或逆向解析数据)
 *
 * @param   [IN] pFileName 文件名
 * @param   [IN] enDirection 读取方向
 * @param   [IN|OUT] u64Param 正向读取时做输入最大时间戳,逆向读取时为输出最大时间戳
 *
 * @return  成功:SAL_SOK 失败:SAL_FALSE
 */
SAL_STATUS dspttk_dec_sharebuffer_operate(CHAR *pFileName, DSPTTK_DEC_SHARE_BUF_DIRECTION enDirection, UINT64 *u64Param)
{
    SAL_STATUS sRet = SAL_SOK;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DEC_SHARE_BUF *pstDecShare = NULL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;
    CHAR *pData = NULL;
    UINT32 u32ReadSize = 8192, u32ReadCnt = 0, i = 0;
    UINT64 u64rIdx = 0, u64wIdx = 0, u64TotalLen = 0, u64Len1 = 0, u64Len2 = 0, u64UseableSize = 0, u64Offset = 0, u64OffsetTraceBack = 0, u64wrGap = 0; // u64rIdx为读索引, u64wIdx = 0为写索引, u64TotalLen为DecShareBuf大小, u64Len1|u64Len2为缓存池处理过程中变量, u64UseableSize为当前缓存池中可用大小, u64Offset为文件中读索引
    long fileSize = 0;                                                                                                                                   // 表示当前文件大小
    UINT8 *pArmAddr = NULL;
    UINT64 u64MaxTimeStampInBuf = 0;
    FLOAT64 f64Percent = 0;

    if (NULL == pFileName || NULL == u64Param)
    {
        PRT_INFO("pFileName is %s, u64Param is %lld\n", pFileName, *u64Param);
        return SAL_FAIL;
    }

    fileSize = dspttk_get_file_size(pFileName);
    if (SAL_FAIL == fileSize)
    {
        PRT_INFO("get file size error. filename is %s, filesize is %ld\n", pFileName, fileSize);
        return SAL_FAIL;
    }

    switch (enDirection)
    {
    case DSPTTK_DEC_READ_FROM_HEAD:
    {
        pstDecShare = &pstShareParam->decShareBuf[gstDecChanCtl.s32CurChan];
        memcpy(gszCurFileName, pFileName, sizeof(gszCurFileName));
        pstDecShare->writeIdx = 0;                              // 写索引清0
        pstDecShare->readIdx = 0;                               // 读索引清0
        u64Offset = 0;                                          // 文件内索引清0
        memset(pstDecShare->pVirtAddr, 0, pstDecShare->bufLen); // 对DecShareBuffer清0
        pstDecShare->decodeStandardTime = 0;
        pData = (CHAR *)malloc(sizeof(CHAR) * 8192);
        if (pData == NULL)
        {
            PRT_INFO("pData is NULL.\n");
            break;
        }
        while (pstDecCfg->stDecMultiParam[gstDecChanCtl.s32CurChan].bDecode) // while中对一个文件读取直到文件尾
        {
            memset(pData, 0, u32ReadSize == 8192 ? u32ReadSize : 8192);

            if (bJump[gstDecChanCtl.s32CurChan])
            {
                u64Offset = gstDecProgressCtl.stDecProgressCurFile.u64Offset;
                bJump[gstDecChanCtl.s32CurChan] = SAL_FALSE;
                dspttk_mutex_lock(&gstDecProgressCtl.stCondDecProgress.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                gstDecProgressCtl.stDecProgress.au32Progress[gstDecChanCtl.s32CurChan] = gstDecProgressCtl.stDecProgressCurFile.u32Percent;
                dspttk_mutex_unlock(&gstDecProgressCtl.stCondDecProgress.mid, __FUNCTION__, __LINE__);
                pstDecShare->writeIdx = 0;
                pstDecShare->readIdx = 0;
            }

            if (gpCurrent[gstDecChanCtl.s32CurChan] == &stIdle)
            {
                pstDecShare->writeIdx = 0;
                pstDecShare->readIdx = 0;
                memset(pstDecShare->pVirtAddr, 0, pstDecShare->bufLen);
                break;
            }

            f64Percent = (FLOAT64)pstDecShare->decodeStandardTime * 1.0 / (*u64Param);

            dspttk_mutex_lock(&gstDecProgressCtl.stCondDecProgressTrans.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            gstDecProgressCtl.stDecProgressTrans.au32Progress[gstDecChanCtl.s32CurChan] = (UINT32)(f64Percent * 100);
            dspttk_mutex_unlock(&gstDecProgressCtl.stCondDecProgressTrans.mid, __FUNCTION__, __LINE__);
            u64OffsetTraceBack = u64Offset - u64wrGap;

            if (f64Percent * 100 >= i * 10 && !gstDecProgressCtl.stDecProgressKeyPoint[i].bUpdate && i < 10)
            {
                dspttk_mutex_lock(&gstDecProgressCtl.stCondDecProgressKeyPoint.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                gstDecProgressCtl.stDecProgressKeyPoint[i].u64Offset = u64OffsetTraceBack;
                gstDecProgressCtl.stDecProgressKeyPoint[i].bUpdate = SAL_TRUE;
                gstDecProgressCtl.stDecProgressKeyPoint[i].u32Percent = i * 10;
                dspttk_mutex_unlock(&gstDecProgressCtl.stCondDecProgressKeyPoint.mid, __FUNCTION__, __LINE__);
                i++;
            }

            if (u32ReadSize)
            {
                sRet = dspttk_read_file(pFileName, u64Offset, pData, &u32ReadSize);
            }

            if ((u64Offset + u32ReadSize > fileSize) || (u64Offset == fileSize)) // 当读取文件失败或已读到文件尾时跳出while循环, 按条件再次进入while
            {
                u32ReadSize = 0; // 默认读写文件大小初始化
                if (u64wIdx == u64rIdx)
                {
                    PRT_INFO("%s filename is %s, DecShareBuf writex equal to readIdx.\n", (u64Offset + u32ReadSize >= fileSize) ? "readfile to the end." : "readfile error.", pFileName);
                    if (DSPTTK_DEC_REPEAT_FILE == pstDecCfg->stDecMultiParam[gstDecChanCtl.s32CurChan].enRepeat)
                    {
                        u64Offset = 0;
                        u32ReadSize = 8192;
                    }
                    else
                    {
                        dspttk_mutex_lock(&gMutexCurrentState, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                        gpCurrent[gstDecChanCtl.s32CurChan] = &stIdle;
                        dspttk_mutex_unlock(&gMutexCurrentState, __FUNCTION__, __LINE__);
                        PRT_INFO("start readfile to the end. current status is idle.\n");
                        break;
                    }
                }
            }

            u64Offset += u32ReadSize;

        ReCalculateHead:

            u64wIdx = pstDecShare->writeIdx;
            u64rIdx = pstDecShare->readIdx;

            if (u64wIdx >= u64rIdx)
            {
                u64UseableSize = pstDecShare->bufLen - (u64wIdx - u64rIdx);
                u64wrGap = u64wIdx - u64rIdx;
            }
            else if (u64wIdx < u64rIdx)
            {
                u64UseableSize = (u64rIdx - u64wIdx);
                u64wrGap = pstDecShare->bufLen - u64rIdx + u64wIdx;
            }

            if ((u64UseableSize > 0) && (u64UseableSize < pstDecShare->bufLen / 2))
            {
                dspttk_msleep(5);
                goto ReCalculateHead;
            }

            pArmAddr = pstDecShare->pVirtAddr;

            u64TotalLen = pstDecShare->bufLen;

            u64Len1 = u64TotalLen - u64wIdx;

            if (u32ReadSize > u64Len1)
            {
                memcpy((CHAR *)(pArmAddr + u64wIdx), pData, u64Len1);
                u64Len2 = u32ReadSize - u64Len1;
                memcpy((CHAR *)pArmAddr, pData + u64Len1, u64Len2);
            }
            else
            {
                memcpy((CHAR *)(pArmAddr + u64wIdx), pData, u32ReadSize);
            }

            pstDecShare->writeIdx = (pstDecShare->writeIdx + u32ReadSize) % pstDecShare->bufLen;
        }
    }
    break;

    case DSPTTK_DEC_READ_FROM_TAIL:
    {
        pstDecShare = &pstShareParam->decShareBuf[gstDecChanCtl.s32BackChan];
        while (pstDecCfg->stDecMultiParam[gstDecChanCtl.s32CurChan].bDecode)
        {
            ++u32ReadCnt;
            if ((0 == u32ReadSize) && (1 == u32ReadCnt))
            {
                u32ReadSize = 20480;
                u32ReadCnt = 0;
                continue;
            }

            u64Offset = fileSize - u32ReadSize;

            if (u64Offset < 0)
            {
                u64Offset = 0;
                u32ReadSize = fileSize;
            }

            pData = (CHAR *)malloc(u32ReadSize);
            if (pData == NULL)
            {
                PRT_INFO("pData is NULL!\n");
                return SAL_FAIL;
            }
            memset(pData, 0, u32ReadSize);

            sRet = dspttk_read_file(pFileName, u64Offset, pData, &u32ReadSize);

            if (sRet != SAL_SOK || (0 == u64Offset)) // 当读取文件失败或已读到文件尾时跳出while循环, 按条件再次进入while
            {
                PRT_INFO("%s filename is %s\n", (u64Offset + u32ReadSize >= fileSize) ? "readfile to the end." : "readfile error.", pFileName);
                pstDecShare->writeIdx = 0;                // 写索引清0
                pstDecShare->readIdx = 0;                 // 读索引清0
                u64Offset = 0;                            // 文件内索引清0
                u32ReadSize = 8192;                       // 默认读写文件大小初始化
                memset(pArmAddr, 0, pstDecShare->bufLen); // 对DecShareBuffer清0
                free(pData);
                pData = NULL;
                break; // 跳出当前while循环
            }

        ReCalculateTail:

            u64wIdx = pstDecShare->writeIdx;
            u64rIdx = pstDecShare->readIdx;

            if (u64wIdx > u64rIdx)
            {
                u64UseableSize = pstDecShare->bufLen - (u64wIdx - u64rIdx);
            }
            else if (u64wIdx < u64rIdx)
            {
                u64UseableSize = (u64rIdx - u64wIdx);
            }

            if ((u64UseableSize > 0) && (u64UseableSize < pstDecShare->bufLen / 2))
            {
                dspttk_msleep(5);
                goto ReCalculateTail;
            }

            pArmAddr = pstDecShare->pVirtAddr;

            u64TotalLen = pstDecShare->bufLen;

            u64Len1 = u64TotalLen - u64wIdx;

            if (u32ReadSize > u64Len1)
            {
                memcpy((CHAR *)(pArmAddr + u64wIdx), pData, u64Len1);
                u64Len2 = u32ReadSize - u64Len1;
                memcpy((CHAR *)pArmAddr, pData + u64Len1, u64Len2);
            }
            else
            {
                memcpy((CHAR *)(pArmAddr + u64wIdx), pData, u32ReadSize);
            }

            pstDecShare->writeIdx = (pstDecShare->writeIdx + u32ReadSize) % pstDecShare->bufLen;
            if (pstDecShare->decodeStandardTime > u64MaxTimeStampInBuf)
            {
                u64MaxTimeStampInBuf = pstDecShare->decodeStandardTime;
                *u64Param = u64MaxTimeStampInBuf;
                dspttk_mutex_lock(&gstDecProgressCtl.curFileMaxTimeStampLock, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                gstDecProgressCtl.u64CurFileMaxTimeStamp = *u64Param;
                dspttk_mutex_unlock(&gstDecProgressCtl.curFileMaxTimeStampLock, __FUNCTION__, __LINE__);
            }
            else if (pstDecShare->decodeStandardTime < u64MaxTimeStampInBuf)
            {
                break;
            }
            else
            {
                u32ReadSize = (++u32ReadCnt) * 20480;
                if (u32ReadSize > fileSize)
                {
                    break;
                }
            }

            free(pData);
            pData = NULL;
        }
        pstDecShare->writeIdx = 0;                // 写索引清0
        pstDecShare->readIdx = 0;                 // 读索引清0
        u64Offset = 0;                            // 文件内索引清0
        u32ReadSize = 8192;                       // 默认读写文件大小初始化
        memset(pArmAddr, 0, pstDecShare->bufLen); // 对DecShareBuffer清0
    }
    break;
    default:
        break;
    }

    return SAL_SOK;
}

/**
 * @fn      dspttk_dec_downstream
 * @brief   解码回调函数(从编码文件中读取数据放到DecShareBuffer中)
 *
 * @param   无
 *
 * @return  无
 */
void dspttk_dec_downstream()
{
    SAL_STATUS sRet = SAL_FAIL;
    INT32 i = 0, s32FileNum = 0; // s32FileNum保存目录下普通文件的数目
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_ENV_INPUT *pstEnvInputCfg = &pstDevCfg->stSettingParam.stEnv.stInputDir;
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;
    UINT64 u64ParamIn = 0;
    CHAR **sFileList = NULL;

    if (gstDecChanCtl.s32CurChan > MAX_VDEC_CHAN - 1)
    {
        PRT_INFO("Vdec Channel Num Error. Channel Num is %d.\n", gstDecChanCtl.s32CurChan);
        return;
    }

    s32FileNum = dspttk_get_file_name_list(pstEnvInputCfg->ipcStream, &sFileList); // 获取当前目录下普通文件的文件名并依次写入szFileName
    if (s32FileNum <= 0)
    {
        PRT_INFO("dspttk_get_file_name_list error.dirName is %s\n", pstEnvInputCfg->ipcStream);
        return;
    }

    for (i = 0; i < s32FileNum; i++) // 按文件数量循环,让线程读写文件到DecSharePool中. while中对一个文件进行读取直到文件尾, for中对当前目录下所有普通文件进行读取
    {
        if (gpCurrent[gstDecChanCtl.s32CurChan] == &stIdle)
        {
            goto EXIT;
        }

        // sRet = dspttk_dec_sharebuffer_operate((CHAR *)(*(pu64FileBuf + i)), DSPTTK_DEC_READ_FROM_TAIL, &u64ParamIn); // 用来获取末尾帧的时间戳
        // if (SAL_SOK != sRet)
        // {
        //     PRT_INFO("dspttk_dec_sharebuffer_operate failed. channel number is %d, filename is %s, direction is %d, paramIn is %lld\n",
        //              gstDecChanCtl.s32BackChan, ((CHAR *)(*(pu64FileBuf + i))), DSPTTK_DEC_READ_FROM_TAIL, u64ParamIn);
        //     continue;
        // }
        // sRet = dspttk_pthread_spawn(&gstDecMultiPthreadCtl.decProgressUpdateThread, DSPTTK_PTHREAD_SCHED_OTHER, 50, 0x100000, (void *)dspttk_decode_progress_update, 2, gstDecChanCtl.s32CurChan, pstDecCfg->stDecMultiParam[gstDecChanCtl.s32CurChan].enMixStreamType == DSPTTK_DEC_PACKET_PS ? (u64ParamIn / 45000) : (u64ParamIn / 90000));
        // if (SAL_SOK != sRet)
        // {
        //     PRT_INFO("create thread for update decode progress failed. pthread_id is %ld, channel number is %d, TotalTime is %lld\n", gstDecMultiPthreadCtl.decProgressUpdateThread, gstDecChanCtl.s32CurChan, u64ParamIn);
        //     goto EXIT;
        // }
        sRet = dspttk_dec_sharebuffer_operate(sFileList[i], DSPTTK_DEC_READ_FROM_HEAD, &u64ParamIn); // 用来正向遍历
        if (SAL_SOK != sRet)
        {
            PRT_INFO("dspttk_dec_sharebuffer_operate failed. channel number is %d, filename is %s, direction is %d, paramIn is %lld\n",
                     gstDecChanCtl.s32CurChan, sFileList[i], DSPTTK_DEC_READ_FROM_TAIL, u64ParamIn);
            continue;
        }

        if ((DSPTTK_DEC_REPEAT_DIR == pstDecCfg->stDecMultiParam[gstDecChanCtl.s32CurChan].enRepeat) && (i == (s32FileNum - 1)))
        {
            if (i == 0)
            {
                i = SAL_FAIL; // 下一轮循环开始, i = 0
            }
            else
            {
                i = 0;
            }
        }
    }

    sRet = SendCmdToDsp(HOST_CMD_DEC_STOP, gstDecChanCtl.s32BackChan, NULL, NULL); // 文件遍历完后, 关闭获取最大解码文件时间戳的通道.
    if (SAL_SOK != sRet)
    {
        PRT_INFO("HOST_CMD_DEC_STOP failed. channel number is %d Ret is %d\n", gstDecChanCtl.s32BackChan, sRet);
        goto EXIT;
    }

EXIT:
    if (NULL != sFileList)
    {
        free(sFileList);
    }

    return;
}

/**
 * @fn      dspttk_decode_find_idx
 * @brief   解码跳转中寻找对应时间戳所在文件索引的回调函数
 *
 * @param   [IN] s32Chan  解码通道号
 * @param   [IN] pFileName  当前待解码文件
 * @param   [IN&OUT] u64Param 输入为当前文件开始索引地址, 输出为定位到相关进度的文件索引
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
void dspttk_decode_find_idx(INT32 s32Chan)
{
    SAL_STATUS sRet = SAL_SOK;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DEC_SHARE_BUF *pstDecBuf = NULL;
    CHAR *pData = NULL;
    UINT32 u32ReadSize = 8192;
    UINT64 u64rIdx = 0, u64wIdx = 0, u64TotalLen = 0, u64Len1 = 0, u64Len2 = 0, u64UseableSize = 0, u64wrGap = 0, u64OffsetTraceBack = 0;
    FLOAT32 f32Percent = 0;
    long fileSize = 0;
    UINT8 *pArmAddr = NULL;

    if (s32Chan > MAX_VDEC_CHAN - 1 || s32Chan < 0)
    {
        PRT_INFO("Vdec Channel Num Error. Channel Num is %d\n", s32Chan);
        return;
    }

    if (0 == gstDecProgressCtl.u64CurFileMaxTimeStamp)
    {
        PRT_INFO("Param In Error. FileName is %s, FileOffset is %lld, PercentIn is %d, CurFileMaxTimeStamp is %lld\n", gszCurFileName, gstDecProgressCtl.stDecProgressCurFile.u64Offset, gstDecProgressCtl.stDecProgressCurFile.u32Percent, gstDecProgressCtl.u64CurFileMaxTimeStamp);
        return;
    }

    pstDecBuf = &pstShareParam->decShareBuf[s32Chan];
    pData = (CHAR *)malloc(sizeof(CHAR) * 8192);
    fileSize = dspttk_get_file_size(gszCurFileName);
    if (gstDecProgressCtl.stDecProgressCurFile.u64Offset > fileSize)
    {
        PRT_INFO("FileOffset greater than FileSize. FileOffset is %lld, fileSize is %ld\n", gstDecProgressCtl.stDecProgressCurFile.u64Offset, fileSize);
        return;
    }

    pstDecBuf->readIdx = 0;
    pstDecBuf->writeIdx = 0;
    pstDecBuf->decodeStandardTime = 0;

    while (!gstDecProgressCtl.stDecProgressCurFile.bUpdate) // 若当前文件所需的索引还未更新
    {
        memset(pData, 0, sizeof(CHAR) * 8192);
        f32Percent = pstDecBuf->decodeStandardTime * 1.0 / gstDecProgressCtl.u64CurFileMaxTimeStamp * 100;
        if (f32Percent >= gstDecProgressCtl.stDecProgressCurFile.u32Percent)
        {
            dspttk_mutex_lock(&gstDecProgressCtl.stCondDecProgressCurFile.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            gstDecProgressCtl.stDecProgressCurFile.bUpdate = SAL_TRUE;
            gstDecProgressCtl.stDecProgressCurFile.u64Offset = u64OffsetTraceBack;
            dspttk_mutex_unlock(&gstDecProgressCtl.stCondDecProgressCurFile.mid, __FUNCTION__, __LINE__);
            dspttk_cond_signal(&gstDecProgressCtl.stCondDecProgressCurFile, SAL_TRUE, __FUNCTION__, __LINE__);
            return;
        }

        sRet = dspttk_read_file(gszCurFileName, gstDecProgressCtl.stDecProgressCurFile.u64Offset, pData, &u32ReadSize);

        if (sRet != SAL_SOK || (gstDecProgressCtl.stDecProgressCurFile.u64Offset + u32ReadSize >= fileSize))
        {

            if (u64wIdx == u64rIdx)
            {
                PRT_INFO("%s filename is %s. wIdx equal to rIdx\n", (gstDecProgressCtl.stDecProgressCurFile.u64Offset + u32ReadSize >= fileSize) ? "readfile to the end." : "readfile error.", gszCurFileName);
                return;
            }
        }

        dspttk_mutex_lock(&gstDecProgressCtl.stCondDecProgressCurFile.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        gstDecProgressCtl.stDecProgressCurFile.u64Offset += u32ReadSize;
        dspttk_mutex_unlock(&gstDecProgressCtl.stCondDecProgressCurFile.mid, __FUNCTION__, __LINE__);

    DecJumpRecalculate:

        u64wIdx = pstDecBuf->writeIdx;
        u64rIdx = pstDecBuf->readIdx;
        u64OffsetTraceBack = gstDecProgressCtl.stDecProgressCurFile.u64Offset - u64wrGap;
        if (u64wIdx >= u64rIdx)
        {
            u64UseableSize = pstDecBuf->bufLen - (u64wIdx - u64rIdx);
            u64wrGap = u64wIdx - u64rIdx;
        }
        else if (u64wIdx < u64rIdx)
        {
            u64UseableSize = (u64rIdx - u64wIdx);
            u64wrGap = pstDecBuf->bufLen - u64rIdx + u64wIdx;
        }

        if ((u64UseableSize > 0) && (u64UseableSize < pstDecBuf->bufLen / 2))
        {
            dspttk_msleep(5);
            goto DecJumpRecalculate;
        }

        pArmAddr = pstDecBuf->pVirtAddr;
        u64TotalLen = pstDecBuf->bufLen;
        u64Len1 = u64TotalLen - u64wIdx;

        if (u32ReadSize > u64Len1)
        {
            memcpy((CHAR *)(pArmAddr + u64wIdx), pData, u64Len1);
            u64Len2 = u32ReadSize - u64Len1;
            memcpy((CHAR *)pArmAddr, pData + u64Len1, u64Len2);
        }
        else
        {
            memcpy((CHAR *)(pArmAddr + u64wIdx), pData, u32ReadSize);
        }

        pstDecBuf->writeIdx = (pstDecBuf->writeIdx + u32ReadSize) % pstDecBuf->bufLen;
    }
}

/* 解码功能函数(不依赖于状态) */
/**
 * @fn      dspttk_decode_progress_update
 * @brief
 * @brief   解码进度更新回调函数(从全局变量gstDecProgress中读取当前解码进度, 按时更新解码进度到前端GUI)
 *
 * @param   [IN] u32Chan  解码通道号
 * @param   [IN] u32TotalTime
 *
 * @return  无
 */
void dspttk_decode_progress_update(UINT32 u32Chan, UINT32 u32TotalTime)
{
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;
    while (pstDecCfg->stDecMultiParam[u32Chan].bDecode && !pstDecCfg->stDecMultiParam[u32Chan].bRecode)
    {
        if (gstDecProgressCtl.stDecProgressTrans.au32Progress[u32Chan] < 100)
        {
            dspttk_dec_update_progress(u32Chan, u32TotalTime, gstDecProgressCtl.stDecProgressTrans.au32Progress[u32Chan]);
        }
        else
        {
            dspttk_dec_update_progress(u32Chan, 0, 100);
            break;
        }
        dspttk_msleep(1000);
    }
}

/**
 * @fn      dspttk_dec_sync
 * @brief   解码同步功能
 *
 * @param   [IN] u32Chan  解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_sync(UINT32 u32Chan)
{
    DECODER_SYNC_PRM stVdecSync = {0};
    DECODER_ATTR stVdecCtl = {0};
    SAL_STATUS sRet = SAL_FAIL;
    UINT32 i = 0;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;

    if (u32Chan > MAX_VDEC_CHAN - 1)
    {
        PRT_INFO("Vdec Channel Num Error. Channel Num is %d.\n", u32Chan);
        return CMD_RET_MIX(HOST_CMD_MODULE_DEC, SAL_FAIL);
    }

    for (i = 0; i < MAX_VDEC_CHAN; i++)
    {
        /* 只对当前已开启解码功能的通道做同步 */
        if (pstDecCfg->stDecMultiParam[i].bDecode)
        {
            /* 设置解码器属性 */
            stVdecCtl.speed = pstDecCfg->stDecMultiParam[u32Chan].stDecAttrPrm.speed;
            stVdecCtl.vdecMode = pstDecCfg->stDecMultiParam[u32Chan].stDecAttrPrm.vdecMode;
            sRet = SendCmdToDsp(HOST_CMD_DEC_ATTR, i, NULL, &stVdecCtl);
            if (SAL_SOK != sRet)
            {
                PRT_INFO("HOST_CMD_DEC_ATTR error. channel number is %d\n", i);
                return CMD_RET_MIX(HOST_CMD_DEC_ATTR, sRet);
            }

            /* 设置解码器同步属性 */
            if (i == u32Chan)
            {
                stVdecSync.mode = SYNC_MODE_MASTER;
            }
            else
            {
                stVdecSync.mode = SYNC_MODE_SLAVE;
            }
            stVdecSync.syncMasterChanId = u32Chan;
            sRet = SendCmdToDsp(HOST_CMD_DEC_SYNC, i, NULL, &stVdecSync);
            if (SAL_SOK != sRet)
            {
                PRT_INFO("HOST_CMD_DEC_SYNC error. channel number is %d\n", i);
                return CMD_RET_MIX(HOST_CMD_DEC_SYNC, sRet);
            }
        }
    }

    return CMD_RET_MIX(HOST_CMD_DEC_SYNC, sRet);
}

/**
 * @fn      dspttk_dec_set_attr
 * @brief   设置dec的speed和mode
 *
 * @param   [IN] u32Chan  解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_set_attr(UINT32 u32Chan)
{
    SAL_STATUS sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;

    if (u32Chan > MAX_VDEC_CHAN - 1)
    {
        PRT_INFO("Vdec Channel Num Error. Channel Num is %d.\n", u32Chan);
        return CMD_RET_MIX(HOST_CMD_MODULE_DEC, SAL_FAIL);
    }

    DECODER_ATTR stDecCtl = {0};

    stDecCtl.speed = pstDecCfg->stDecMultiParam[u32Chan].stDecAttrPrm.speed;
    stDecCtl.vdecMode = pstDecCfg->stDecMultiParam[u32Chan].stDecAttrPrm.vdecMode;
    sRet = SendCmdToDsp(HOST_CMD_DEC_ATTR, u32Chan, NULL, &stDecCtl);

    return CMD_RET_MIX(HOST_CMD_DEC_ATTR, sRet);
}

/**
 * @fn      dspttk_dec_set_muxtype
 * @brief   设置dec的muxtype
 *
 * @param   [IN] u32Chan  解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_set_muxtype(UINT32 u32Chan)
{
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;

    if (u32Chan > MAX_VDEC_CHAN - 1)
    {
        PRT_INFO("Vdec Channel Num Error. Channel Num is %d.\n", u32Chan);
        return CMD_RET_MIX(HOST_CMD_MODULE_DEC, SAL_FAIL);
    }

    pstDecCfg->stDecMultiParam[u32Chan].stDecPrm.muxType = (pstDecCfg->stDecMultiParam[u32Chan].enMixStreamType == DSPTTK_DEC_PACKET_PS) ? MPEG2MUX_STREAM_TYPE_PS : MPEG2MUX_STREAM_TYPE_RTP;
    return CMD_RET_MIX(HOST_CMD_DEC_ATTR, SAL_SOK);
}

/**
 * @fn      dspttk_dec_jump
 * @brief   解码跳转功能
 *
 * @param   [IN] u32Chan  解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_jump(UINT32 u32Chan)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT32 u32PercentIdx = 0; // u32PercentIdx用来查找最接近的10的倍数的索引[0, 100].
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;
    INT32 i = 0;

    if (u32Chan > MAX_VDEC_CHAN - 1)
    {
        PRT_INFO("Vdec Channel Num Error. Channel Num is %d.\n", u32Chan);
        return CMD_RET_MIX(HOST_CMD_MODULE_DEC, SAL_FAIL);
    }

    dspttk_mutex_lock(&gstDecProgressCtl.stCondDecProgressCurFile.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    gstDecProgressCtl.stDecProgressCurFile.bUpdate = SAL_FALSE;
    gstDecProgressCtl.stDecProgressCurFile.u64Offset = 0;
    gstDecProgressCtl.stDecProgressCurFile.u32Percent = pstDecCfg->stDecMultiParam[u32Chan].u32JumpPercent; // 传进去的希望找到的进度
    dspttk_mutex_unlock(&gstDecProgressCtl.stCondDecProgressCurFile.mid, __FUNCTION__, __LINE__);

    u32PercentIdx = (pstDecCfg->stDecMultiParam[u32Chan].u32JumpPercent + 5) / 10; //+5为了找到最近的10的倍数, 如JumpPercent=6, 则应该找10而不是0

    for (i = u32PercentIdx; i > SAL_FAIL; i--)
    {
        if (gstDecProgressCtl.stDecProgressKeyPoint[i].bUpdate)
        {
            dspttk_mutex_lock(&gstDecProgressCtl.stCondDecProgressKeyPoint.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            gstDecProgressCtl.stDecProgressCurFile.u64Offset = gstDecProgressCtl.stDecProgressKeyPoint[i].u64Offset;
            dspttk_mutex_unlock(&gstDecProgressCtl.stCondDecProgressKeyPoint.mid, __FUNCTION__, __LINE__);
            break;
        }
    }

    if (NULL == gszCurFileName)
    {
        PRT_INFO("CurFileName is NULL.\n");
        return CMD_RET_MIX(HOST_CMD_MODULE_DEC, SAL_FAIL);
    }

    sRet = dspttk_pthread_spawn(&gstDecMultiPthreadCtl.decProgressFindIdx, DSPTTK_PTHREAD_SCHED_OTHER, 50, 0x100000, (void *)dspttk_decode_find_idx, 1, gstDecChanCtl.s32BackChan);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("create thread for find %d percent key frame index error.\n", gstDecProgressCtl.stDecProgressCurFile.u32Percent);
        return CMD_RET_MIX(HOST_CMD_MODULE_DEC, SAL_FAIL);
    }

    while (!gstDecProgressCtl.stDecProgressCurFile.bUpdate) // wait for find key frame in file index.
    {
        dspttk_cond_wait(&gstDecProgressCtl.stCondDecProgressCurFile, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    }
    bJump[u32Chan] = SAL_TRUE;

    return CMD_RET_MIX(HOST_CMD_DEC_START, sRet);
}

/* 解码状态对应功能函数 */
/**
 * @fn      dspttk_dec_start
 * @brief   开启解码功能
 *
 * @param   无
 *
 * @return  无
 */
void dspttk_dec_start()
{
    SAL_STATUS sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;
    DECODER_PARAM stDecPrm = {0};
    // UINT32 i = 0;
    UINT64 u64Ret = 0;

    if (gstDecChanCtl.s32CurChan > MAX_VDEC_CHAN - 1)
    {
        PRT_INFO("Vdec Channel Num Error. Channel Num is %d.\n", gstDecChanCtl.s32CurChan);
        return;
    }

    // for (i = pstDevCfg->stInitParam.stEncDec.decChanCnt; i > -1; i--) // 逆序遍历查找一个未被启用的解码通道, 用来获取当前解码文件最大时间戳
    // {
    //     if (!pstDecCfg->stDecMultiParam[i].bDecode)
    //     {
    //         dspttk_mutex_lock(&gstDecChanCtl.backChanLock, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    //         gstDecChanCtl.s32BackChan = i;
    //         dspttk_mutex_unlock(&gstDecChanCtl.backChanLock, __FUNCTION__, __LINE__);
    //         break;
    //     }
    // }

    // if (SAL_FAIL == gstDecChanCtl.s32BackChan)
    // {
    //     dspttk_mutex_lock(&gstDecChanCtl.backChanLock, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    //     gstDecChanCtl.s32BackChan = pstDevCfg->stInitParam.stEncDec.decChanCnt - 1;
    //     dspttk_mutex_unlock(&gstDecChanCtl.backChanLock, __FUNCTION__, __LINE__);
    // }

    // /* 设置查询时间戳所用解码通道的相关属性 */
    // PRT_INFO("s32BackChan is %d pstDevCfg->stInitParam.stEncDec.decChanCnt is %d\n", gstDecChanCtl.s32BackChan, pstDevCfg->stInitParam.stEncDec.decChanCnt);
    // u64Ret = dspttk_dec_set_attr(gstDecChanCtl.s32BackChan);
    // sRet = CMD_RET_OF(u64Ret);
    // if (SAL_SOK != sRet)
    // {
    //     PRT_INFO("dspttk_dec_set_attr error BackChanNum is %d.\n", gstDecChanCtl.s32BackChan);
    //     return;
    // }

    // 初次开启解码器需要设置相关参数
    u64Ret = dspttk_dec_set_attr(gstDecChanCtl.s32CurChan);
    sRet = CMD_RET_OF(u64Ret);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_dec_set_attr error. chanNum is %d\n", gstDecChanCtl.s32CurChan);
        return;
    }

    pstDecCfg->stDecMultiParam[gstDecChanCtl.s32CurChan].stDecPrm.muxType = (pstDecCfg->stDecMultiParam[gstDecChanCtl.s32CurChan].enMixStreamType == DSPTTK_DEC_PACKET_PS) ? MPEG2MUX_STREAM_TYPE_PS : MPEG2MUX_STREAM_TYPE_RTP;
    stDecPrm.muxType = pstDecCfg->stDecMultiParam[gstDecChanCtl.s32CurChan].stDecPrm.muxType;

    // sRet = SendCmdToDsp(HOST_CMD_DEC_START, gstDecChanCtl.s32BackChan, NULL, &stDecPrm); // 开启解码通道(用来寻找当前解码文件中最大时间戳开启的通道)
    // if (SAL_SOK != sRet)
    // {
    //     PRT_INFO("HOST_CMD_DEC_START error. channel number is %d\n", gstDecChanCtl.s32BackChan);
    //     return;
    // }

    sRet = SendCmdToDsp(HOST_CMD_DEC_START, gstDecChanCtl.s32CurChan, NULL, &stDecPrm); // 开启解码通道(页面设置的通道号)
    if (SAL_SOK != sRet)
    {
        PRT_INFO("HOST_CMD_DEC_START error. channel number is %d\n", gstDecChanCtl.s32CurChan);
        return;
    }

    sRet = dspttk_pthread_spawn(&gstDecMultiPthreadCtl.StartDecThread[gstDecChanCtl.s32CurChan], DSPTTK_PTHREAD_SCHED_OTHER, 50, 0x100000, (void *)dspttk_dec_downstream, 0, NULL);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_pthread_spawn failed. hook function name is dspttk_dec_downstream. channel number is %d\n", gstDecChanCtl.s32CurChan);
        return;
    }

    dspttk_mutex_lock(&gMutexCurrentState, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    gpCurrent[gstDecChanCtl.s32CurChan] = &stDecoding;
    dspttk_mutex_unlock(&gMutexCurrentState, __FUNCTION__, __LINE__);
    PRT_INFO("current state is decoding\n");
    return;
}

/**
 * @fn      dspttk_dec_stop
 * @brief   关闭解码功能
 *
 * @param   无
 *
 * @return  无
 */
void dspttk_dec_stop()
{
    SAL_STATUS sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;
    DECODEC_COVER_PICPRM stVdecCoverPrm = {0};

    if (gstDecChanCtl.s32CurChan > MAX_VDEC_CHAN - 1)
    {
        PRT_INFO("Vdec Channel Num Error. Channel Num is %d.\n", gstDecChanCtl.s32CurChan);
        return;
    }

    stVdecCoverPrm.cover_sign = pstDecCfg->stDecMultiParam[gstDecChanCtl.s32CurChan].stCoverPrm.cover_sign;
    stVdecCoverPrm.logo_pic_indx = pstDecCfg->stDecMultiParam[gstDecChanCtl.s32CurChan].stCoverPrm.logo_pic_indx;

    sRet = SendCmdToDsp(HOST_CMD_DEC_STOP, gstDecChanCtl.s32CurChan, NULL, &stVdecCoverPrm);

    if (SAL_SOK != sRet)
    {
        PRT_INFO("HOST_CMD_DEC_STOP failed. channel number is %d\n", gstDecChanCtl.s32CurChan);
        return;
    }

    dspttk_mutex_lock(&gMutexCurrentState, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    gpCurrent[gstDecChanCtl.s32CurChan] = &stIdle;
    dspttk_mutex_unlock(&gMutexCurrentState, __FUNCTION__, __LINE__);
    PRT_INFO("current state is idle\n");
    return;
}

/**
 * @fn      dspttk_dec_pause
 * @brief   暂停解码功能
 *
 * @param   [IN] u32Chan  解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
void dspttk_dec_pause()
{
    SAL_STATUS sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;
    DECODEC_COVER_PICPRM stVdecCoverPrm = {0};

    if (gstDecChanCtl.s32CurChan > MAX_VDEC_CHAN - 1)
    {
        PRT_INFO("Vdec Channel Num Error. Channel Num is %d.\n", gstDecChanCtl.s32CurChan);
        return;
    }

    stVdecCoverPrm.cover_sign = pstDecCfg->stDecMultiParam[gstDecChanCtl.s32CurChan].stCoverPrm.cover_sign;
    stVdecCoverPrm.logo_pic_indx = pstDecCfg->stDecMultiParam[gstDecChanCtl.s32CurChan].stCoverPrm.logo_pic_indx;

    sRet = SendCmdToDsp(HOST_CMD_DEC_PAUSE, gstDecChanCtl.s32CurChan, NULL, &stVdecCoverPrm);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("HOST_CMD_DEC_PAUSE error. channel number is %d, sRet is %d\n", gstDecChanCtl.s32CurChan, sRet);
        return;
    }

    dspttk_mutex_lock(&gMutexCurrentState, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    gpCurrent[gstDecChanCtl.s32CurChan] = &stPausing;
    dspttk_mutex_unlock(&gMutexCurrentState, __FUNCTION__, __LINE__);
    PRT_INFO("current state is pause\n");
    return;
}

/**
 * @fn      dspttk_dec_null
 * @brief   空操作
 *
 * @param   无
 *
 * @return  无
 */
void dspttk_dec_null()
{
    PRT_INFO("nothing need to do. current state is %s\n", gpCurrent[gstDecChanCtl.s32CurChan] == &stDecoding ? "Decoding..." : gpCurrent[gstDecChanCtl.s32CurChan] == &stPausing ? "paused."
                                                                                                                                                                                 : "idle.");
    return;
}

/**
 * @fn      dspttk_dec_resume
 * @brief   恢复解码功能
 *
 * @param   无
 *
 * @return  无
 */
void dspttk_dec_resume()
{
    SAL_STATUS sRet = SAL_SOK;
    DECODER_PARAM stDecPrm = {0};
    sRet = SendCmdToDsp(HOST_CMD_DEC_START, gstDecChanCtl.s32CurChan, NULL, &stDecPrm);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("HOST_CMD_DEC_START in dec resume failed. channel number is %d, ret is 0x%x\n", gstDecChanCtl.s32CurChan, sRet);
        return;
    }

    dspttk_mutex_lock(&gMutexCurrentState, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    gpCurrent[gstDecChanCtl.s32CurChan] = &stDecoding;
    dspttk_mutex_unlock(&gMutexCurrentState, __FUNCTION__, __LINE__);
    PRT_INFO("current state is decoding\n");
    return;
}

/**
 * @fn      dspttk_dec_status_init
 * @brief   状态初始化
 *
 * @param   无
 *
 * @return  无
 */
void dspttk_dec_status_init()
{
    UINT32 i = 0;
    for (i = 0; i < MAX_VDEC_CHAN; i++)
    {
        gpCurrent[i] = &stIdle;
    }
    return;
}

UINT64 dspttk_dec_chan_enable(UINT32 u32Chan)
{
    gpCurrent[u32Chan] = &stIdle;
    return CMD_RET_MIX(HOST_CMD_MODULE_DEC, SAL_SOK);
}

/**
 * @fn      dspttk_dec_cond_init
 * @brief   状态初始化
 *
 * @param   无
 *
 * @return  无
 */
void dspttk_dec_cond_init()
{
    SAL_STATUS sRet = SAL_SOK;
    sRet |= dspttk_mutex_init(&gstDecChanCtl.backChanLock);
    sRet |= dspttk_mutex_init(&gstDecChanCtl.curChanLock);
    
    sRet |= dspttk_cond_init(&gstDecProgressCtl.stCondDecProgress);
    sRet |= dspttk_cond_init(&gstDecProgressCtl.stCondDecProgressCurFile);
    sRet |= dspttk_cond_init(&gstDecProgressCtl.stCondDecProgressKeyPoint);
    sRet |= dspttk_cond_init(&gstDecProgressCtl.stCondDecProgressTrans);
    sRet |= dspttk_mutex_init(&gstDecProgressCtl.curFileMaxTimeStampLock);

    sRet |= dspttk_mutex_init(&gMutexCurrentState);

    if (SAL_SOK != sRet)
    {
        PRT_INFO("decode mutex and cond init failed.\n");
        return;
    }

    return;
}

/**
 * @fn      dspttk_recode_status_init
 * @brief   状态初始化
 *
 * @param   无
 *
 * @return  无
 */
void dspttk_recode_status_init()
{
    UINT32 i = 0;
    for (i = 0; i < MAX_VIPC_CHAN; i++)
    {
        gpCurrentRecode[i] = &stRecodeStop;
    }
    return;
}

/**
 * @fn      dspttk_recode_cond_init
 * @brief   状态初始化
 *
 * @param   无
 *
 * @return  无
 */
void dspttk_recode_cond_init()
{
    SAL_STATUS sRet = SAL_SOK;
    sRet |= dspttk_mutex_init(&gstRecodeCurFileState.writeToPsLock);
    sRet |= dspttk_mutex_init(&gstRecodeCurFileState.writeToRtpLock);
    sRet |= dspttk_mutex_init(&gstRecodeCurFileState.readToBufferLock);
    sRet |= dspttk_mutex_init(&gstRecodeChanCtl.curChanLock);
    sRet |= dspttk_mutex_init(&gMutexCurrentRecodeState);
    sRet |= dspttk_cond_init(&gstRecodeCurFileState.stCondRecodeCurFile);

    if (SAL_SOK != sRet)
    {
        PRT_INFO("recode mutex and cond init failed.\n");
        return;
    }

    return;
}

/**
 * @fn      dspttk_dec_init
 * @brief   解码初始化
 *
 * @param   无
 *
 * @return  无
 */
void dspttk_dec_init()
{
    /* 解码初始化 */
    dspttk_dec_cond_init();         //解码：信号量、锁初始化
    dspttk_dec_status_init();       //解码：状态初始化

    /* 转封装初始化 */
    dspttk_recode_cond_init();      //转封装：信号量、锁初始化
    dspttk_recode_status_init();    //转封装：状态初始化

    return;
}

/**
 * @fn      dspttk_dec_btn_start
 * @brief   解码栏start按钮绑定函数
 *
 * @param   [IN] u32Chan 解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_btn_start(UINT32 u32Chan)
{
    SAL_STATUS sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;

    if (u32Chan > MAX_VDEC_CHAN)
    {
        PRT_INFO("channel number is %d error. supposed in [0, %d).\n", u32Chan, MAX_VDEC_CHAN);
        return CMD_RET_MIX(HOST_CMD_DEC_START, sRet);
    }

    dspttk_mutex_lock(&gstDecChanCtl.curChanLock, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    gstDecChanCtl.s32CurChan = u32Chan;
    dspttk_mutex_unlock(&gstDecChanCtl.curChanLock, __FUNCTION__, __LINE__);

    if (!pstDecCfg->stDecMultiParam[u32Chan].bDecode)
    {
        PRT_INFO("channel %d hasn't been enable. please make sure checkbox enabled.\n", u32Chan);
        return CMD_RET_MIX(HOST_CMD_DEC_START, sRet);
    }

    if (NULL == gpCurrent[u32Chan])
    {
        PRT_INFO("Current global pointer is NULL. chan %d\n", u32Chan);
        return CMD_RET_MIX(HOST_CMD_DEC_START, SAL_FAIL);
    }

    gpCurrent[u32Chan]->start();

    return CMD_RET_MIX(HOST_CMD_DEC_START, SAL_SOK);
}

/**
 * @fn      dspttk_dec_btn_pause
 * @brief   解码栏pause按钮绑定函数
 *
 * @param   [IN] u32Chan 解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_btn_pause(UINT32 u32Chan)
{
    SAL_STATUS sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;

    if (u32Chan > MAX_VDEC_CHAN)
    {
        PRT_INFO("channel number is %d error. supposed in [0, %d).\n", u32Chan, MAX_VDEC_CHAN);
        return CMD_RET_MIX(HOST_CMD_DEC_PAUSE, sRet);
    }

    if (!pstDecCfg->stDecMultiParam[u32Chan].bDecode)
    {
        PRT_INFO("channel %d hasn't been enable. please make sure checkbox enabled.\n", u32Chan);
        return CMD_RET_MIX(HOST_CMD_DEC_PAUSE, sRet);
    }

    if (NULL == gpCurrent[u32Chan])
    {
        PRT_INFO("Current global pointer is NULL. chan %d\n", u32Chan);
        return CMD_RET_MIX(HOST_CMD_DEC_START, SAL_FAIL);
    }

    gpCurrent[u32Chan]->pause();

    return CMD_RET_MIX(HOST_CMD_DEC_PAUSE, SAL_SOK);
}

/**
 * @fn      dspttk_dec_btn_stop
 * @brief   解码栏stop按钮绑定函数
 *
 * @param   [IN] u32Chan 解码通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_btn_stop(UINT32 u32Chan)
{
    SAL_STATUS sRet = SAL_FAIL;
    DSPINITPARA *pstDevShare = dspttk_get_share_param();
    DEC_SHARE_BUF *pstDecBuf = &pstDevShare->decShareBuf[gstDecChanCtl.s32CurChan];
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;

    if (u32Chan > MAX_VDEC_CHAN)
    {
        PRT_INFO("channel number is %d error. supposed in [0, %d).\n", u32Chan, MAX_VDEC_CHAN);
        return CMD_RET_MIX(HOST_CMD_DEC_STOP, sRet);
    }

    if (!pstDecCfg->stDecMultiParam[u32Chan].bDecode)
    {
        PRT_INFO("channel %d hasn't been enable. please make sure checkbox enabled.\n", u32Chan);
        return CMD_RET_MIX(HOST_CMD_DEC_STOP, sRet);
    }

    if (NULL == gpCurrent[u32Chan])
    {
        PRT_INFO("Current global pointer is NULL. chan %d\n", u32Chan);
        return CMD_RET_MIX(HOST_CMD_DEC_START, SAL_FAIL);
    }

    gpCurrent[u32Chan]->stop();

    memset(pstDecBuf->pVirtAddr, 0, pstDecBuf->bufLen);
    pstDecBuf->readIdx = 0;
    pstDecBuf->writeIdx = 0;
    pstDecBuf->decodeStandardTime = 0;

    return CMD_RET_MIX(HOST_CMD_DEC_STOP, SAL_SOK);
}

/* 转封装功能函数 */

/**
 * @fn      dspttk_dec_recode_set
 * @brief   转包功能参数设置
 *
 * @param   [IN] u32Chan  转包通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_recode_set(UINT32 u32Chan)
{
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;
    RECODE_PRM stRecodePrm = {0};
    SAL_STATUS sRet = SAL_FAIL;

    if (u32Chan > MAX_VIPC_CHAN - 1)
    {
        PRT_INFO("Vdec Channel Num Error. Channel Num is %d.\n", u32Chan);
        return CMD_RET_MIX(HOST_CMD_MODULE_DEC, SAL_FAIL);
    }

    if (pstDecCfg->stDecMultiParam[u32Chan].enMixStreamType == DSPTTK_DEC_PACKET_RTP_VIDEOTYPE_H264 || pstDecCfg->stDecMultiParam[u32Chan].enMixStreamType == DSPTTK_DEC_PACKET_RTP_VIDEOTYPE_H265 || pstDecCfg->stDecMultiParam[u32Chan].enMixStreamType == DSPTTK_DEC_PACKET_PS)
    {
        stRecodePrm.dstPackType = (1 << MPEG2MUX_STREAM_TYPE_RTP) | (1 << MPEG2MUX_STREAM_TYPE_PS);
    }
    else
    {
        PRT_INFO("dstPackType Error. Chan is %d, dstPackType is %d\n", u32Chan, pstDecCfg->stDecMultiParam[u32Chan].enMixStreamType);
        return CMD_RET_MIX(HOST_CMD_RECODE_START, SAL_FAIL);
    }

    sRet = SendCmdToDsp(HOST_CMD_RECODE_SET, u32Chan, NULL, &stRecodePrm);

    return CMD_RET_MIX(HOST_CMD_RECODE_SET, sRet);
}

/**
 * @fn      dspttk_recode_netpool
 * @brief   转封装回调函数(rtp)
 *
 * @param   [IN] pFileName  转包对应源文件名
 * @param   [IN] u32TimeOutTimes 超时时间计数器 每一次休眠5ms
 * @return  无
 */
void dspttk_recode_netpool(CHAR *pFileName, UINT32 u32TimeOutTimes)
{
    SAL_STATUS sRet = SAL_SOK;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    NET_POOL_INFO *pNetPool = &pstShareParam->NetPoolRecode[gstRecodeChanCtl.s32CurChan];
    UINT64 u64wIdx = 0, u64rIdx = 0, u64Len1 = 0, u64Len2 = 0, u64TotalLen = 0;
    UINT8 *pVirAddr = NULL;
    UINT32 i = 0;

    if (gstRecodeChanCtl.s32CurChan > MAX_VIPC_CHAN - 1)
    {
        PRT_INFO("Recode Channel Num Error. Channel Num is %d.\n", gstRecodeChanCtl.s32CurChan);
        return;
    }

    if (NULL == pFileName)
    {
        PRT_INFO("pFileName is NULL. Channel Num is %d.\n", gstRecodeChanCtl.s32CurChan);
        return;
    }

    u64wIdx = pNetPool->wIdx;
    u64rIdx = u64wIdx;
    while (gstRecodeCurFileState.bRecodeWriteToRtpFile[gstRecodeChanCtl.s32CurChan])
    {
        u64wIdx = pNetPool->wIdx;

        if (u64wIdx == u64rIdx)
        {
            for (i = 0; i < u32TimeOutTimes; i++)
            {
                dspttk_msleep(5);
                if (pNetPool->wIdx != u64wIdx) // 如果当前写索引和之前相比有变化, 则表明此时数据已更新
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
            if (u32TimeOutTimes == i) // 当达到超时时间,直接退出
            {
                goto dspttk_recode_netpool_EXIT;
            }
        }

        if (u64wIdx >= u64rIdx)
        {
            u64Len1 = u64wIdx - u64rIdx;
            u64Len2 = 0;
        }
        else
        {
            u64Len1 = pNetPool->totalLen - u64rIdx;
            u64Len2 = u64wIdx;
        }

        u64TotalLen = u64Len1 + u64Len2;
        pVirAddr = pNetPool->vAddr + u64rIdx;

        if (u64Len1)
        {
            sRet = dspttk_write_file(pFileName, 0, SEEK_END, pVirAddr, u64Len1);
            if (SAL_SOK != sRet)
            {
                PRT_INFO("dspttk_write_file error. filename is %s, write data size is %lld\n", pFileName, u64Len1);
                return;
            }
        }

        if (u64Len2)
        {
            sRet = dspttk_write_file(pFileName, 0, SEEK_END, pNetPool->vAddr, u64Len2);
            if (SAL_SOK != sRet)
            {
                PRT_INFO("dspttk_write_file error. filename is %s, write data size is %lld\n", pFileName, u64Len1);
                return;
            }
        }

        u64rIdx = (u64rIdx + u64TotalLen) % pNetPool->totalLen;
    }
dspttk_recode_netpool_EXIT:
    dspttk_mutex_lock(&gstRecodeCurFileState.writeToRtpLock, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    gstRecodeCurFileState.bRecodeWriteToRtpFile[gstRecodeChanCtl.s32CurChan] = SAL_FALSE;
    dspttk_mutex_unlock(&gstRecodeCurFileState.writeToRtpLock, __FUNCTION__, __LINE__);
    dspttk_cond_signal(&gstRecodeCurFileState.stCondRecodeCurFile, SAL_TRUE, __FUNCTION__, __LINE__);
    pNetPool = NULL;
    pstShareParam = NULL;
    pVirAddr = NULL;
    return;
}

/**
 * @fn      dspttk_recode_recpool
 * @brief   转封装回调函数(ps)
 *
 * @param   [IN] u32Chan  IPC通道号
 * @param   [IN] pFileName  转包对应源文件名
 * @param   [IN] u32TimeOutTimes 超时时间计数器 每一次休眠5ms
 * @return  无
 */
void dspttk_recode_recpool(CHAR *pFileName, UINT32 u32TimeOutTimes)
{
    SAL_STATUS sRet = SAL_SOK;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    REC_POOL_INFO *pRecPool = &pstShareParam->RecPoolRecode[gstRecodeChanCtl.s32CurChan];
    UINT64 u64wIdx = 0, u64rIdx = 0, u64Len1 = 0, u64Len2 = 0, u64TotalLen = 0;
    UINT8 *pVirAddr = NULL;
    UINT32 i = 0;

    if (gstRecodeChanCtl.s32CurChan > MAX_VIPC_CHAN - 1)
    {
        PRT_INFO("Recode Channel Num Error. Channel Num is %d.\n", gstRecodeChanCtl.s32CurChan);
        return;
    }

    if (NULL == pFileName)
    {
        PRT_INFO("pFileName is NULL. Channel Num is %d.\n", gstRecodeChanCtl.s32CurChan);
        return;
    }

    while (gstRecodeCurFileState.bRecodeWriteToPsFile[gstRecodeChanCtl.s32CurChan])
    {
        u64wIdx = pRecPool->wOffset;
        u64rIdx = pRecPool->rOffset;
        if (u64wIdx == u64rIdx)
        {
            for (i = 0; i < u32TimeOutTimes; i++)
            {
                dspttk_msleep(5);
                if (pRecPool->wOffset != u64wIdx) // 如果当前写索引和之前相比有变化, 则表明此时数据已更新
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
            if (u32TimeOutTimes == i) // 当达到超时时间,直接退出
            {
                goto dspttk_recode_recpool_EXIT;
            }
        }

        if (u64wIdx >= u64rIdx)
        {
            u64Len1 = u64wIdx - u64rIdx;
            u64Len2 = 0;
        }
        else
        {
            u64Len1 = pRecPool->totalLen - u64rIdx;
            u64Len2 = u64wIdx;
        }

        u64TotalLen = u64Len1 + u64Len2;
        pVirAddr = pRecPool->vAddr + u64rIdx;

        if (u64Len1)
        {
            sRet = dspttk_write_file(pFileName, 0, SEEK_END, pVirAddr, u64Len1);

            if (SAL_SOK != sRet)
            {
                PRT_INFO("dspttk_write_file error. filename is %s, write data size is %lld\n", pFileName, u64Len1);
                return;
            }
        }

        if (u64Len2)
        {
            sRet = dspttk_write_file(pFileName, 0, SEEK_END, pRecPool->vAddr, u64Len2);

            if (SAL_SOK != sRet)
            {
                PRT_INFO("dspttk_write_file error. filename is %s, write data size is %lld\n", pFileName, u64Len1);
                return;
            }
        }

        u64rIdx = (u64rIdx + u64TotalLen) % pRecPool->totalLen;
        pRecPool->rOffset = u64rIdx;
    }
dspttk_recode_recpool_EXIT:
    dspttk_mutex_lock(&gstRecodeCurFileState.writeToPsLock, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    gstRecodeCurFileState.bRecodeWriteToPsFile[gstRecodeChanCtl.s32CurChan] = SAL_FALSE;
    dspttk_mutex_unlock(&gstRecodeCurFileState.writeToPsLock, __FUNCTION__, __LINE__);
    dspttk_cond_signal(&gstRecodeCurFileState.stCondRecodeCurFile, SAL_TRUE, __FUNCTION__, __LINE__);
    pstShareParam = NULL;
    pRecPool = NULL;
    pVirAddr = NULL;
    return;
}

/**
 * @fn      dspttk_recode_downstream
 * @brief   转封装回调函数(将文件加载到ipcsharebuf里)
 *
 * @param   [IN] pFileName  源文件名
 *
 * @return  无
 */
void dspttk_recode_downstream(CHAR *pFileName)
{
    SAL_STATUS sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DEC_SHARE_BUF *pstRecodeShare = &pstShareParam->ipcShareBuf[gstRecodeChanCtl.s32CurChan];
    UINT32 u32ReadSize = 8192;
    UINT64 u64rIdx = 0, u64wIdx = 0, u64TotalLen = 0, u64Len1 = 0, u64Len2 = 0, u64UseableSize = 0, u64Offset = 0;
    long fileSize = 0;
    CHAR szData[MAX_BUFFER_SIZE] = {0};
    UINT8 *pVirAddr = NULL;

    if (gstRecodeChanCtl.s32CurChan > MAX_VIPC_CHAN - 1)
    {
        PRT_INFO("Recode Channel Num Error. Channel Num is %d.\n", gstRecodeChanCtl.s32CurChan);
        return;
    }

    if (NULL == pFileName)
    {
        PRT_INFO("filename is null.\n");
        return;
    }

    fileSize = dspttk_get_file_size(pFileName);

    while (gstRecodeCurFileState.bReadToIPCBuffer[gstRecodeChanCtl.s32CurChan])
    {
        memset(szData, 0, sizeof(szData));

        if (u64Offset >= fileSize)
        {
            PRT_INFO("readfile to the end. offset is %lld, filesize is %ld\n", u64Offset, fileSize);
            dspttk_mutex_lock(&gstRecodeCurFileState.readToBufferLock, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            gstRecodeCurFileState.bReadToIPCBuffer[gstRecodeChanCtl.s32CurChan] = SAL_FALSE;
            dspttk_mutex_unlock(&gstRecodeCurFileState.readToBufferLock, __FUNCTION__, __LINE__);
            dspttk_cond_signal(&gstRecodeCurFileState.stCondRecodeCurFile, SAL_TRUE, __FUNCTION__, __LINE__);
            break;
        }

        sRet = dspttk_read_file(pFileName, u64Offset, szData, &u32ReadSize);
        if (sRet != SAL_SOK) // 当读取文件失败或已读到文件尾时跳出while循环, 按条件再次进入while
        {
            PRT_INFO("readfile error. filename is %s\n", pFileName);
            break;
        }

    RecodeDownCalc:

        u64wIdx = pstRecodeShare->writeIdx;
        u64rIdx = pstRecodeShare->readIdx;

        if (u64wIdx > u64rIdx)
        {
            u64UseableSize = pstRecodeShare->bufLen - (u64wIdx - u64rIdx);
        }
        else if (u64wIdx < u64rIdx)
        {
            u64UseableSize = (u64rIdx - u64wIdx);
        }

        if ((u64UseableSize > 0) && (u64UseableSize < pstRecodeShare->bufLen / 2))
        {
            dspttk_msleep(5);
            goto RecodeDownCalc;
        }

        pVirAddr = pstRecodeShare->pVirtAddr;
        u64TotalLen = pstRecodeShare->bufLen;
        u64Len1 = u64TotalLen - u64wIdx;

        if (u32ReadSize > u64Len1)
        {
            memcpy((CHAR *)(pVirAddr + u64wIdx), szData, u64Len1);
            u64Len2 = u32ReadSize - u64Len1;
            memcpy((CHAR *)pVirAddr, szData + u64Len1, u64Len2);
        }
        else
        {
            memcpy((CHAR *)(pVirAddr + u64wIdx), szData, u32ReadSize);
        }

        pstRecodeShare->writeIdx = (pstRecodeShare->writeIdx + u32ReadSize) % pstRecodeShare->bufLen;

        if (u64Offset + u32ReadSize > fileSize)
        {
            u32ReadSize = fileSize - u64Offset;
        }
        else
        {
            u64Offset += u32ReadSize;
        }
    }

    pstShareParam = NULL;
    pstRecodeShare = NULL;
    pVirAddr = NULL;
    return;
}

/**
 * @fn      dspttk_recode_tsk_process
 * @brief   转包回调函数
 *
 * @param   无
 *
 * @return  无
 */
void dspttk_recode_tsk_process()
{
    SAL_STATUS sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;
    DSPTTK_SETTING_ENV_INPUT *pstEnvInputCfg = &pstDevCfg->stSettingParam.stEnv.stInputDir;
    DSPTTK_SETTING_ENV_OUTPUT *pstEnvOutCfg = &pstDevCfg->stSettingParam.stEnv.stOutputDir;
    RECODE_PRM stRecodePrm = {0};
    UINT32 i = 0, u32StreamType = 0; // s32FileNum保存从目录中获取的文件个数, u32StreamType保存从文件名中截取的视频流数据格式
    INT32 s32FileNum = 0;
    CHAR **sFileList = NULL;
    CHAR szSuffixes[8] = {0};                        // 保存后缀名, 8为默认值, 假定后缀名最大占8个字节
    CHAR *pSubString = NULL;                         // SubString用来截取文件名获取相关参数
    CHAR szSubString[MAX_FILENAME_SIZE] = {0};
    pSubString = szSubString;
    CHAR szBaseName[MAX_FILENAME_SIZE] = {0}; // 获取文件名除目录外的部分
    CHAR *pBaseName = NULL;
    CHAR szRecodeNameRtp[MAX_FILENAME_SIZE] = {0}; // 保存下发给saveStreamFromRecodeNetThread的文件名
    CHAR szRecodeNamePS[MAX_FILENAME_SIZE] = {0};  // 保存下发给saveStreamFromRecodeNetThread的文件名
    CHAR szFileHead[MAX_FILENAME_SIZE] = {0};      // 截取源文件后缀前的字符串
    CHAR *pFileHead = szFileHead;
    CHAR szNull[16] = {0}; // 用来新建文件

    if (gstRecodeChanCtl.s32CurChan > MAX_VIPC_CHAN - 1)
    {
        PRT_INFO("Vdec Channel Num Error. Channel Num is %d.\n", gstRecodeChanCtl.s32CurChan);
        return;
    }

    s32FileNum = dspttk_get_file_name_list(pstEnvInputCfg->ipcStream, &sFileList);
    if (s32FileNum <= 0)
    {
        PRT_INFO("dspttk_get_file_name_list failed, dir %s, fileNum %d, chan %d\n", pstEnvInputCfg->ipcStream, s32FileNum, gstRecodeChanCtl.s32CurChan);
        return;
    }

    for (i = 0; i < s32FileNum; i++) // 按目录将文件依次遍历
    {
        sRet = dspttk_get_file_suffixes(sFileList[i], szSuffixes); // 获取文件后缀
        /* 1.先判断源文件是否为rtp格式(目前仅支持源文件为rtp格式) */
        if (0 == strcmp(szSuffixes, "rtp"))
        {
            stRecodePrm.srcPackType = MPEG2MUX_STREAM_TYPE_RTP;
            pSubString = strstr(sFileList[i], "_h");
            if (NULL == pSubString)
            {
                PRT_INFO("pSubString is NULL.\n");
                continue;
            }

            /* 2.文件名按照"ipc_h264_aac_3.rtp"的形式保存,因此截取h后的数字可知视频流格式 */
            sRet = sscanf(pSubString, "%*[^h]h%u[^_]", &u32StreamType);
            if (sRet < 0)
            {
                PRT_INFO("sscanf height from %s failed.\n", pSubString);
                goto RECODE_EXIT;
            }

            stRecodePrm.srcEncType = u32StreamType == 264 ? ENCODER_H264 : (u32StreamType == 265 ? ENCODER_H265 : ENCODER_UNKOWN);
            /* 3. 默认将源文件分别转包成RTP和PS两种封装格式 */
            stRecodePrm.dstPackType = (1 << MPEG2MUX_STREAM_TYPE_PS) | (1 << MPEG2MUX_STREAM_TYPE_RTP);

            /* 从setting中获取其他相关参数 */
            stRecodePrm.audioPackPs = pstDecCfg->stDecMultiParam[gstRecodeChanCtl.s32CurChan].stRecodePrm.audioPackPs;
            stRecodePrm.audioPackRtp = pstDecCfg->stDecMultiParam[gstRecodeChanCtl.s32CurChan].stRecodePrm.audioPackRtp;
            stRecodePrm.audioChannels = pstDecCfg->stDecMultiParam[gstRecodeChanCtl.s32CurChan].stRecodePrm.audioChannels;
            stRecodePrm.audioSamplesPerSec = pstDecCfg->stDecMultiParam[gstRecodeChanCtl.s32CurChan].stRecodePrm.audioSamplesPerSec;
            stRecodePrm.audioBitsPerSample = pstDecCfg->stDecMultiParam[gstRecodeChanCtl.s32CurChan].stRecodePrm.audioBitsPerSample;

            /* 4.按当前从文件和setting设置中获取的参数下发转包开始指令 */
            sRet = SendCmdToDsp(HOST_CMD_RECODE_START, gstRecodeChanCtl.s32CurChan, NULL, &stRecodePrm);
            if (SAL_SOK != sRet)
            {
                PRT_INFO("HOST_CMD_RECODE_START failed. filename is %s, channel number is %d.\n", sFileList[i], gstRecodeChanCtl.s32CurChan);
                continue;
            }

            /* 5.将源文件数据搬运到ipc_sharebuffer中 */
            dspttk_mutex_lock(&gstRecodeCurFileState.readToBufferLock, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            gstRecodeCurFileState.bReadToIPCBuffer[gstRecodeChanCtl.s32CurChan] = SAL_TRUE;
            dspttk_mutex_unlock(&gstRecodeCurFileState.readToBufferLock, __FUNCTION__, __LINE__);
            sRet = dspttk_pthread_spawn(&gstRecodeMultiPthreadCtl.startRecodeSaveThread[gstRecodeChanCtl.s32CurChan], DSPTTK_PTHREAD_SCHED_OTHER, 
                                        50, 0x100000, (void *)dspttk_recode_downstream, 1, sFileList[i]);
            if (SAL_SOK != sRet)
            {
                PRT_INFO("create carry data from %s to ipc share buffer pthread failed.\n", sFileList[i]);
                continue;
            }

            memcpy(szBaseName, sFileList[i], sizeof(szBaseName));
            pBaseName = basename(szBaseName); // 只获取除目录外部分的文件名

            pFileHead = strsep(&pBaseName, "."); // 截取源文件后缀前的字符串
            if (NULL == pFileHead)
            {
                PRT_INFO("pFileHead is NULL. pBaseName is %s\n", pBaseName);
                continue;
            }

            /* 转包后的文件命名为源文件名_recode.rtp */
            snprintf(szRecodeNameRtp, sizeof(szRecodeNameRtp), "%s/%d/%s_recode.%s", pstEnvOutCfg->streamTrans, gstRecodeChanCtl.s32CurChan, pFileHead, "rtp");
            sRet = dspttk_write_file(szRecodeNameRtp, 0, SEEK_SET, szNull, 0); // 创建文件
            if (SAL_SOK != sRet)
            {
                PRT_INFO("create file %s failed.\n", szRecodeNameRtp);
                return;
            }

            /* 6.将转rtp包成功的数据从netpool中保存到新的xxx_recode.rtp文件中 */
            dspttk_mutex_lock(&gstRecodeCurFileState.writeToRtpLock, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            gstRecodeCurFileState.bRecodeWriteToRtpFile[gstRecodeChanCtl.s32CurChan] = SAL_TRUE;
            dspttk_mutex_unlock(&gstRecodeCurFileState.writeToRtpLock, __FUNCTION__, __LINE__);
            sRet = dspttk_pthread_spawn(&gstRecodeMultiPthreadCtl.saveStreamFromRecodeNetThread[gstRecodeChanCtl.s32CurChan], DSPTTK_PTHREAD_SCHED_OTHER, 50, 0x100000, (void *)dspttk_recode_netpool, 2, szRecodeNameRtp, 10);
            if (SAL_SOK != sRet)
            {
                PRT_INFO("create carry data from netpool to %s pthread failed.\n", szRecodeNameRtp);
                continue;
            }

            /* 转包后的文件命名为源文件名_recode.ps */
            snprintf(szRecodeNamePS, sizeof(szRecodeNamePS), "%s/%d/%s_recode.%s", pstEnvOutCfg->streamTrans, gstRecodeChanCtl.s32CurChan, pFileHead, "ps");
            sRet = dspttk_write_file(szRecodeNamePS, 0, SEEK_SET, szNull, 0); // 创建文件
            if (SAL_SOK != sRet)
            {
                PRT_INFO("create file %s failed.\n", szRecodeNamePS);
                return;
            }

            /* 7.将转ps包成功的数据从recpool中保存到新的xxx_recode.ps文件中 */
            dspttk_mutex_lock(&gstRecodeCurFileState.writeToPsLock, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            gstRecodeCurFileState.bRecodeWriteToPsFile[gstRecodeChanCtl.s32CurChan] = SAL_TRUE;
            dspttk_mutex_unlock(&gstRecodeCurFileState.writeToPsLock, __FUNCTION__, __LINE__);
            sRet = dspttk_pthread_spawn(&gstRecodeMultiPthreadCtl.saveStreamFromRecodeRecThread[gstRecodeChanCtl.s32CurChan], DSPTTK_PTHREAD_SCHED_OTHER, 50, 0x100000, (void *)dspttk_recode_recpool, 2, szRecodeNamePS, 10);
            if (SAL_SOK != sRet)
            {
                PRT_INFO("create carry data from recpool to %s pthread failed.\n", szRecodeNamePS);
                continue;
            }

            /*
             * 当文件中数据已全部搬运到ipcsharebuffer,
             * netpool中数据已搬运到rtp文件中
             * 且recpool中数据已搬运到ps文件中开始下次循环.
             * 因为线程异步执行, 防止数据乱解乱丢的现象.
             *
             */
            while (gstRecodeCurFileState.bRecodeWriteToPsFile[gstRecodeChanCtl.s32CurChan] || gstRecodeCurFileState.bRecodeWriteToRtpFile[gstRecodeChanCtl.s32CurChan] || gstRecodeCurFileState.bReadToIPCBuffer[gstRecodeChanCtl.s32CurChan])
            {
                dspttk_cond_wait(&gstRecodeCurFileState.stCondRecodeCurFile, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            }
        }
        else
        {
            PRT_INFO("source file %s packet type doesn't matched. type is %s, supposed to be rtp.\n", sFileList[i], szSuffixes);
            continue;
        }
    }

RECODE_EXIT:
    if (NULL != sFileList)
    {
        free(sFileList);
    }

    return;
}

/* 转封装状态对应功能函数 */
/**
 * @fn      dspttk_recode_start()
 * @brief   开启转封装功能
 *
 * @param   无
 *
 * @return  无
 */

void dspttk_recode_start()
{
    SAL_STATUS sRet = SAL_SOK;

    sRet = dspttk_pthread_spawn(&gstRecodeMultiPthreadCtl.startRecodeSaveThread[gstRecodeChanCtl.s32CurChan], DSPTTK_PTHREAD_SCHED_OTHER, 50, 0x100000, (void *)dspttk_recode_tsk_process, 0, NULL);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_pthread_spawn failed. hook function name is dspttk_recode_tsk_process. channel number is %d\n", gstRecodeChanCtl.s32CurChan);
        return;
    }

    dspttk_mutex_lock(&gMutexCurrentRecodeState, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    gpCurrentRecode[gstRecodeChanCtl.s32CurChan] = &stRecodeStart;
    dspttk_mutex_unlock(&gMutexCurrentRecodeState, __FUNCTION__, __LINE__);

    return;
}

/**
 * @fn      dspttk_recode_stop()
 * @brief   关闭转封装功能
 *
 * @param   无
 *
 * @return  无
 */

void dspttk_recode_stop()
{
    SAL_STATUS sRet = SAL_SOK;
    
    sRet = SendCmdToDsp(HOST_CMD_RECODE_STOP, gstRecodeChanCtl.s32CurChan, NULL, NULL);

    if (SAL_SOK != sRet)
    {
        PRT_INFO("HOST_CMD_RECODE_STOP failed. current recode channel number is %d\n", gstRecodeChanCtl.s32CurChan);
        return;
    }

    dspttk_mutex_lock(&gMutexCurrentRecodeState, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    gpCurrentRecode[gstRecodeChanCtl.s32CurChan] = &stRecodeStop;
    dspttk_mutex_unlock(&gMutexCurrentRecodeState, __FUNCTION__, __LINE__);
    return;
}

/**
 * @fn      dspttk_recode_null()
 * @brief   转封装空操作
 *
 * @param   无
 *
 * @return  无
 */

void dspttk_recode_null()
{
    PRT_INFO("current recode state is %s, there is nothing need to do.\n", gpCurrentRecode[gstRecodeChanCtl.s32CurChan] == &stRecodeStart ? "Recoding" : "Idle");
    return;
}


/**
 * @fn      dspttk_dec_recode_start
 * @brief   开启转包功能
 *
 * @param   [IN] u32Chan  转包通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_recode_start(UINT32 u32Chan)
{
    dspttk_mutex_lock(&gstRecodeChanCtl.curChanLock, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    gstRecodeChanCtl.s32CurChan = u32Chan;
    dspttk_mutex_unlock(&gstRecodeChanCtl.curChanLock, __FUNCTION__, __LINE__);
    gpCurrentRecode[u32Chan]->start();

    return CMD_RET_MIX(HOST_CMD_RECODE_START, SAL_SOK);
}

/**
 * @fn      dspttk_dec_recode_stop
 * @brief   关闭转包功能
 *
 * @param   [IN] u32Chan  转包通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_recode_stop(UINT32 u32Chan)
{
    dspttk_mutex_lock(&gstRecodeChanCtl.curChanLock, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    gstRecodeChanCtl.s32CurChan = u32Chan;
    dspttk_mutex_unlock(&gstRecodeChanCtl.curChanLock, __FUNCTION__, __LINE__);
    gpCurrentRecode[u32Chan]->stop();

    return CMD_RET_MIX(HOST_CMD_RECODE_STOP, SAL_SOK);
}

/**
 * @fn      dspttk_dec_recode_enable_set
 * @brief   转包使能设置
 *
 * @param   [IN] u32Chan  转包通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_dec_recode_enable_set(UINT32 u32Chan)
{
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC *pstDecCfg = &pstDevCfg->stSettingParam.stDec;
    INT64L sRet = SAL_FAIL;

    if (u32Chan > MAX_VIPC_CHAN - 1)
    {
        PRT_INFO("Vdec Channel Num Error. Channel Num is %d.\n", u32Chan);
        return CMD_RET_MIX(HOST_CMD_MODULE_DEC, SAL_FAIL);
    }

    if (pstDecCfg->stDecMultiParam[u32Chan].bRecode)
    {
        sRet = dspttk_dec_recode_start(u32Chan);
    }
    else
    {
        sRet = dspttk_dec_recode_stop(u32Chan);
    }

    return sRet;
}

