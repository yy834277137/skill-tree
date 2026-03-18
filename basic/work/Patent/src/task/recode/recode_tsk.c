/*******************************************************************************
* recode_tsk.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wutao <wutao19@hikvision.com.cn>
* Version: V1.0.0  2019年9月28日 Create
*
* Description :
* Modification:
*******************************************************************************/
#include <string.h>
#include <sys/time.h>
#include "sal.h"
#include "recode_tsk_api.h"
#include "libdemux.h"
#include "libmux.h"
#include "system_prm_api.h"
#include <platform_hal.h>
#include "PSMuxLib.h"
#include "stream_bits_info_def.h"

#line __LINE__ "recode_tsk.c"
#include "sva_out.h"
#include "ppm_out.h"
#include "../func/func_baseLib/vca_system/IVS_SYS_lib.h"
#include "vca_pack_api.h"
#include "../plat/himpp_v4p0/include/platform_sdk.h"
#include "ia_common.h"

/*****************************************************************************
                               全局变量
*****************************************************************************/

static UINT8 g_PackAacAdtsHeader[MAX_VIPC_CHAN][7] =
{
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
    {0xff, 0xf1, 0x6c, 0x40, 0x24, 0xbf, 0xfc},
};

static INT32 g_PackAacAdtsHeaderSampleFre[16] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0};


static RECODE_OBJ_COMMON g_stRecodeObjCommon =
{
    .uiObjCnt = 0,
};

/* 人包设备叠加算法pos信息所需参数及接口 */
#define BUFFER_SIZE				(1024 * 1024)                /* 转包处理参数的缓存大小 */
#define TEST_TARGET_NUM			32                           /* 智能帧支持最大的目标个数，同编码私有信息处理一致 */
#define DEBUG_MEM_LEN			(4 * 1024 * 1024)
#define DEBUG_EXTRA_MEM			(512 * 1024)
#define DEBUG_RECODE_CHN_NUM	(12)                   /* 目前预研项目，写死只有两路转包，人脸1 + 三目0 */
#define DEBUG_BUF_CNT			(32)
#define DEBUG_BUF_SIZE			(DEBUG_MEM_LEN)
/* #define DEBUG_DELAY_MS (1300)                         / * 算法处理平均耗时1300ms，此处作为延迟缓存 * / */
#define DEBUG_RECODE_CHAN_MAX   (2)
#define DEBUG_FACE_VDEC_CHAN    (10)
#define DEBUG_TRI_VDEC_CHAN     (11)
#define DEBUG_FACE_RECODE_CHAN    (1)
#define DEBUG_TRI_RECODE_CHAN     (0)

static HIK_MULT_VCA_TARGET_LIST target_list[12] = {0};       /* 智能帧目标链 */
static IVS_SYS_PROC_PARAM proc_param[12] = {0};              /* 处理参数 */
static SVA_PROCESS_OUT stOut[12] = {0};                      /* 中间变量，用于保存智能检测结果，此处未使用 */
static UINT32 uiOutBufSize[12] = {0};                        /* 转封装后单帧码流使用的缓存大小 */
static UINT8 *pOutBuf[12] = {NULL};                          /* 转封装后单帧码流使用的缓存 */
static PPM_POS_INFO_S stPosInfo[2] = {0};                    /* pos全局结构体 */

typedef struct _DEBUG_BUF_INFO_
{
    char *p;
    UINT32 uiLen;
} DEBUG_BUF_INFO_S;

typedef struct _DEBUG_PROC_PRM_
{
    char *mem;
    UINT32 uiWIdx;                               /* 中间缓存写索引，unit: byte */
    UINT32 uiRIdx;                               /* 中间缓存读索引 */
    UINT32 uiMemLen;                             /* 内存大小 */
    UINT32 uiExtraMemLen;                        /* 拓展内存大小 */

    BOOL bEndJudgeFlag;                          /* 停止判断的标志位 */
    BOOL bBufFillEnd;                            /* buf填充满的标志位 */
    UINT32 uiBufCnt;                             /* buf个数，暂定64 */
    UINT32 uiBufWIdx;                            /* buf写索引 */
    UINT32 uiBufRIdx;                            /* buf读索引 */
    DEBUG_BUF_INFO_S stProcBuf[DEBUG_BUF_CNT];
} DEBUG_PROC_PRM_S;

static DEBUG_PROC_PRM_S stDebugProcPrm[DEBUG_RECODE_CHN_NUM] = {0};       /* 帧延时使用的相关参数 */

INT32 recode_tskDemuxWait(UINT32 chan);

BOOL recode_tskJudgeNeedDelay(VOID)
{
#ifdef PPM_NO_POS_DEBUG
    return SAL_FALSE;
#endif

	if (SAL_TRUE != IA_GetModEnableFlag(IA_MOD_PPM))
    {
    	return SAL_FALSE;
    }

    return SAL_TRUE;
}

/**
 * @function    get_buf_full_sts
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
BOOL get_buf_full_sts(UINT32 chan)
{
    return (stDebugProcPrm[chan].bEndJudgeFlag || stDebugProcPrm[chan].bBufFillEnd);
}

/**
 * @function    update_fill_buf_flag
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID update_fill_buf_flag(UINT32 chan)
{
    BOOL flag = SAL_FALSE;

    if ((stDebugProcPrm[chan].uiBufWIdx + 1) % stDebugProcPrm[chan].uiBufCnt == stDebugProcPrm[chan].uiBufRIdx)
    {
        flag = SAL_TRUE;
    }

    stDebugProcPrm[chan].bBufFillEnd = flag;
    return;
}

/**
 * @function    get_data_size
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
UINT32 get_data_size(UINT32 w, UINT32 r, UINT32 len)
{
    return (w >= r ? w - r : w - r + len);
}

/**
 * @function    fill_buf
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 fill_buf(UINT32 chan, char *src, UINT32 w, UINT32 r, UINT32 len, UINT32 *dataLen)
{
    UINT32 mem_data_len = 0;
    UINT32 offset = 0;
    DEBUG_PROC_PRM_S *pstProcPrm = NULL;

    pstProcPrm = &stDebugProcPrm[chan];

    mem_data_len = get_data_size(w, r, len);

    if (mem_data_len == 0)
    {
        SAL_DEBUG("no data! \n");
        *dataLen = offset;
        return SAL_SOK;
    }

    if (mem_data_len > DEBUG_BUF_SIZE )//|| mem_data_len < 100
    {
        SAL_WARN("buf size err! len %d, w %d, r %d, len %d, chan %d \n", mem_data_len, w, r, len, chan);
        return SAL_FAIL;
    }

    offset = 0;
    if (w >= r)
    {
        SAL_DEBUG("ywn:w %d, r %d, widx %d, p_addr %p, src_addr %p \n", w, r, pstProcPrm->uiBufWIdx, pstProcPrm->stProcBuf[pstProcPrm->uiBufWIdx].p, src);
        sal_memcpy_s(pstProcPrm->stProcBuf[pstProcPrm->uiBufWIdx].p + offset, DEBUG_BUF_SIZE, src + r, w - r);
        offset += (w - r);
    }
    else
    {
        sal_memcpy_s(pstProcPrm->stProcBuf[pstProcPrm->uiBufWIdx].p + offset, DEBUG_BUF_SIZE, src + r, len - r);
        offset += (len - r);

        sal_memcpy_s(pstProcPrm->stProcBuf[pstProcPrm->uiBufWIdx].p + offset, DEBUG_BUF_SIZE, src, w);
        offset += w;
    }

    *dataLen = offset;
    pstProcPrm->stProcBuf[pstProcPrm->uiBufWIdx].uiLen = offset;

    pstProcPrm->uiBufWIdx = (pstProcPrm->uiBufWIdx + 1) % DEBUG_BUF_CNT;  /* 索引自增 */
    return SAL_SOK;
}

/**
 * @function    get_buf
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 get_buf(UINT32 chan, char **src, UINT32 *size)
{
    DEBUG_PROC_PRM_S *pstProcPrm = NULL;

    pstProcPrm = &stDebugProcPrm[chan];

    *src = pstProcPrm->stProcBuf[pstProcPrm->uiBufRIdx].p;
    *size = pstProcPrm->stProcBuf[pstProcPrm->uiBufRIdx].uiLen;

   // pstProcPrm->stProcBuf[pstProcPrm->uiBufRIdx].uiLen = 0;      /* 数据量清空，避免旧数据残留 */

    //pstProcPrm->uiBufRIdx = (pstProcPrm->uiBufRIdx + 1) % DEBUG_BUF_CNT;  /* 索引自增 */
    return SAL_SOK;
}

/**
 * @function    update_mem_idx
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID update_mem_idx(UINT32 dataLen, volatile UINT32 len, volatile UINT32 *idx)
{
    *idx = (*idx + dataLen) % len;
    return;
}

/**
 * @function    cpy_to_mem
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 cpy_to_mem(UINT32 chan, char *dst, UINT32 w, UINT32 r, UINT32 len)
{
    UINT32 size = 0;
    char *src = NULL;
    int offset = 0;

    /* 取 debug结构体的读指针对应的buf数据 */
    if (SAL_SOK != get_buf(chan, &src, &size))
    {
        SAL_ERROR("fill buf failed! chan %d \n", chan);
        return SAL_FAIL;
    }

    if (((r > w) && (size >= r-w-1))
       ||((w > r) && (size >= (r + len - w))))
    {
        SAL_DEBUG("not enough mem left! w %d, r %d, len %d, size %d\n", w, r, len, size);
        return SAL_SOK;
    }

    stDebugProcPrm[chan].stProcBuf[stDebugProcPrm[chan].uiBufRIdx].uiLen = 0;      /* 数据量清空，避免旧数据残留 */
    stDebugProcPrm[chan].uiBufRIdx = (stDebugProcPrm[chan].uiBufRIdx + 1) % DEBUG_BUF_CNT;  /* 索引自增 */

    RECODE_LOGD("get buf! [%p, %d], chan %d \n", src, size, chan);
    if (!src)
    {
        SAL_ERROR("get buf ptr null! chan %d \n", chan);
        return SAL_FAIL;
    }
    
    if (0 == size)
    {
        SAL_WARN("size == 0! \n");
        return SAL_SOK;
    }
    
    if (w >= r)
    {
        if (size > len - w)
        {
            memcpy(dst + w, src + offset, len - w);
            offset += len - w;//w - r;

            memcpy(dst, src + offset, size - len + w);
        }
        else
        {
            memcpy(dst + w, src + offset, size);
            offset += size;
        }
    }
    else
    {
        if (size < r - w)
        {
            memcpy(dst + w, src + offset, size);
            offset += size;
        }
        else
        {
            SAL_ERROR("err! no enough memory\n");
            return SAL_FAIL;
        }
    }

    (VOID)update_mem_idx(size, stDebugProcPrm[chan].uiMemLen, &stDebugProcPrm[chan].uiWIdx);
    RECODE_LOGD("update mem idx! wIdx[%d], rIdx[%d], chan %d \n",
                stDebugProcPrm[chan].uiWIdx, stDebugProcPrm[chan].uiRIdx, chan);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskRtpUpdataPts
* 描  述  : 转包组件更新时间戳
* 输  入  : 无
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskRtpUpdataPts(UINT32 chan)
{
    RECODE_OBJ_PRM *pstObjPrm = NULL;
    RECODE_PACK_TIME *pRecodeTime = NULL;
    INT32 ret = 0;
    UINT32 timeOffset = 0;
    BOOL timeErr = SAL_FALSE;
    UINT64 u64CurPts = 0;

    if (chan >= RECODE_MAX_CHN)
    {
        RECODE_LOGE("chan %d is error\n", chan);
        return SAL_FAIL;
    }

    pstObjPrm = &g_stRecodeObjCommon.recodeStObj[chan];
    pRecodeTime = &pstObjPrm->recodeTime;


    /* 计算相对时间时间 */
    if (!pRecodeTime->bFirstVideoPts)
    {
        if (pRecodeTime->timeStamp < pRecodeTime->lastVRtpTime)
        {
            if (pRecodeTime->lastVRtpTime - pRecodeTime->timeStamp > 0xf0000000)
            {
                timeOffset = (0xffffffff - pRecodeTime->lastVRtpTime + pRecodeTime->timeStamp + pRecodeTime->compTime) / 90;
                pRecodeTime->compTime = (0xffffffff - pRecodeTime->lastVRtpTime + pRecodeTime->timeStamp + pRecodeTime->compTime) % 90;
            }
            else
            {
                timeOffset = pRecodeTime->timeDist;
                pRecodeTime->compTime = 0;
                timeErr = SAL_TRUE;
            }
        }
        else
        {
            timeOffset = (pRecodeTime->timeStamp - pRecodeTime->lastVRtpTime + pRecodeTime->compTime) / 90;
            pRecodeTime->compTime = (pRecodeTime->timeStamp - pRecodeTime->lastVRtpTime + pRecodeTime->compTime) % 90;
        }

        if (timeOffset > 16 * 1000 || timeErr)
        {
            ret = sys_hal_GetSysCurPts(&u64CurPts);
            if (SAL_SOK != ret)
            {
                RECODE_LOGE("chan %d is error\n", chan);
                return SAL_FAIL;
            }

            pRecodeTime->u64Vpts = u64CurPts + (UINT64)timeOffset * 1000;
            pRecodeTime->compTime = 0;
        }
        else
        {

            pRecodeTime->u64Vpts += (UINT64)timeOffset * 1000;
        }

        pRecodeTime->u64Pts_v = pRecodeTime->u64Vpts;

        pRecodeTime->timeDist += timeOffset;
        pRecodeTime->lastVRtpTime = pRecodeTime->timeStamp;
    }
    else
    {
        pRecodeTime->bFirstVideoPts = SAL_FALSE;
        pRecodeTime->timeDist = 0;
        timeOffset = 40;
        ret = sys_hal_GetSysCurPts(&u64CurPts);
        if (SAL_SOK != ret)
        {
            RECODE_LOGE("chan %d is error\n", chan);
            return SAL_FAIL;
        }

        pRecodeTime->u64Vpts = u64CurPts;

        pRecodeTime->u64Pts_v = pRecodeTime->u64Vpts;

        pRecodeTime->lastVRtpTime = pRecodeTime->timeStamp;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskRtpUpdataAudPts
* 描  述  : 转包组件更新时间戳
* 输  入  : 无
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskRtpUpdataAudPts(UINT32 chan)
{
    RECODE_OBJ_PRM *pstObjPrm = NULL;
    RECODE_PACK_TIME *pRecodeTime = NULL;
    INT32 ret = 0;
    UINT32 timeOffset = 0;
    BOOL timeErr = SAL_FALSE;
    UINT64 u64CurPts = 0;

    if (chan >= RECODE_MAX_CHN)
    {
        RECODE_LOGE("chan %d is error\n", chan);
        return SAL_FAIL;
    }

    pstObjPrm = &g_stRecodeObjCommon.recodeStObj[chan];
    pRecodeTime = &pstObjPrm->recodeAudTime;


    /* 计算相对时间时间 */
    if (!pRecodeTime->bFirstVideoPts)
    {
        if (pRecodeTime->timeStamp < pRecodeTime->lastVRtpTime)
        {
            if (pRecodeTime->lastVRtpTime - pRecodeTime->timeStamp > 0xf0000000)
            {
                timeOffset = (0xffffffff - pRecodeTime->lastVRtpTime + pRecodeTime->timeStamp + pRecodeTime->compTime) / 90;
                pRecodeTime->compTime = (0xffffffff - pRecodeTime->lastVRtpTime + pRecodeTime->timeStamp + pRecodeTime->compTime) % 90;
            }
            else
            {
                timeOffset = pRecodeTime->timeDist;
                pRecodeTime->compTime = 0;
                timeErr = SAL_TRUE;
            }
        }
        else
        {
            timeOffset = (pRecodeTime->timeStamp - pRecodeTime->lastVRtpTime + pRecodeTime->compTime) / 90;
            pRecodeTime->compTime = (pRecodeTime->timeStamp - pRecodeTime->lastVRtpTime + pRecodeTime->compTime) % 90;
        }

        if (timeOffset > 16 * 1000 || timeErr)
        {
            ret = sys_hal_GetSysCurPts(&u64CurPts);
            if (SAL_SOK != ret)
            {
                RECODE_LOGE("chan %d is error\n", chan);
                return SAL_FAIL;
            }

            pRecodeTime->u64Vpts = u64CurPts + (UINT64)timeOffset * 1000;
            pRecodeTime->compTime = 0;
        }
        else
        {

            pRecodeTime->u64Vpts += (UINT64)timeOffset * 1000;
        }

        pRecodeTime->u64Pts_v = pRecodeTime->u64Vpts;

        pRecodeTime->timeDist += timeOffset;
        pRecodeTime->lastVRtpTime = pRecodeTime->timeStamp;
    }
    else
    {
        pRecodeTime->bFirstVideoPts = SAL_FALSE;
        pRecodeTime->timeDist = 0;
        timeOffset = 40;
        ret = sys_hal_GetSysCurPts(&u64CurPts);
        if (SAL_SOK != ret)
        {
            RECODE_LOGE("chan %d is error\n", chan);
            return SAL_FAIL;
        }

        pRecodeTime->u64Vpts = u64CurPts;

        pRecodeTime->u64Pts_v = pRecodeTime->u64Vpts;

        pRecodeTime->lastVRtpTime = pRecodeTime->timeStamp;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskDemuxInit
* 描  述  : 转包组件初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskDemuxInit(DEM_TYPE demType, UINT8 *addr, UINT32 lenth, INT32 *deMuxHandle)
{
    INT32 ret = SAL_FAIL;
    DEM_PARAM demPrm;
    DEM_ORGBUF_PRM orgBufPrm;
    DEM_PARAM *pDemPrm = &demPrm;
    DEM_CHN_PRM demChnPrm;

    demChnPrm.demType = demType;
    demChnPrm.outType = DEM_VIDEO_OUT | DEM_AUDIO_OUT;

    /******************************************************************************************/
    /*获取空闲的解封装通道*/
    ret = DemuxControl(DEM_GET_HANDLE, &demChnPrm, pDemPrm);
    if (SAL_SOK != ret)
    {
        RECODE_LOGE("get  demux handle err ret %x demhdl %d\n", ret, pDemPrm->demHdl);
        return SAL_FAIL;
    }

    pDemPrm->bufAddr = (unsigned char *)SAL_memMalloc(pDemPrm->bufLen, "recode", "demux");
    if (NULL == pDemPrm->bufAddr)
    {
        RECODE_LOGE("demMem %p memSz %d\n", pDemPrm->bufAddr, pDemPrm->bufLen);
        return SAL_FAIL;
    }

    memset(pDemPrm->bufAddr, 0x0, pDemPrm->bufLen);

    ret = DemuxControl(DEM_CREATE, pDemPrm, NULL);
    if (SAL_SOK != ret)
    {
        RECODE_LOGE("create demux handle err ret %x\n", ret);
        SAL_memfree((void **)&pDemPrm->bufAddr, "recode", "OS");
        return SAL_FAIL;
    }

    orgBufPrm.orgBuf = addr;
    orgBufPrm.orgbuflen = lenth;
    ret = DemuxControl(DEM_SET_ORGBUF, &pDemPrm->demHdl, &orgBufPrm);
    if (SAL_SOK != ret)
    {
        RECODE_LOGE("DEM_SET_ORGBUF ret %x\n", ret);
        SAL_memfree((void **)&pDemPrm->bufAddr, "recode", "OS");
        return SAL_FAIL;
    }

    *deMuxHandle = pDemPrm->demHdl;

    return SAL_SOK;
}

/******************************************************************
   Function:   recode_tskIsKeyFrame
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static UINT32 recode_tskIsKeyFrame(unsigned char *addr)
{
    if ((*(addr + 4) == 0x67) || ((*(addr + 4) == 0x09) && (*(addr + 5) == 0x10)) || (*(addr + 4) == 0x40)
            || ((*(addr + 4) & 0x1f) == 0x07)) /* 大华有的IPC(比如IPC-HFW4833F-ZAS)SPS对应的BYTE是0x27*/
    {
        return SAL_TRUE;
    }

    return SAL_FALSE;
}

/******************************************************************
   Function:   recode_tskMuxCreate
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static INT32 recode_tskMuxCreate(UINT32 PackTpye, UINT32 *pHandleNum)
{
    INT32 status = SAL_FAIL;
    UINT32 MuxType = 0;
    MUX_PARAM MuxPrm;

    /**********动态创建一个新的打包通道***************/
    MuxType = PackTpye;
    memset(&MuxPrm, 0, sizeof(MuxPrm));
    status = MuxControl(MUX_GET_CHAN, &MuxType, &MuxPrm);
    if (status != SAL_SOK)
    {
        RECODE_LOGE("MuxControl idx %d err %x\n", MUX_GET_CHAN, status);
        return SAL_FAIL;
    }

    /**********创建打包模块***************/
    MuxPrm.audPack = 1; /*PS封装里面添加AUD打包 RTP无此项设置，不影响*/
    MuxPrm.bufAddr = (PUINT8)SAL_memMalloc(MuxPrm.bufLen, "recode", "mux");
    if (NULL == MuxPrm.bufAddr)
    {
        RECODE_LOGE("MuxPrm buffer malloc err!\n");
        return SAL_FAIL;
    }

    status = MuxControl(MUX_CREATE, &MuxPrm, NULL);
    if (SAL_isFail(status))
    {
        SAL_memfree(MuxPrm.bufAddr, "recode", "OS");
        RECODE_LOGE("MuxControl idx %d err %x\n", MUX_CREATE, status);
        return SAL_FAIL;
    }

    /* 记录当前打包handle的通道号 */
    *pHandleNum = MuxPrm.muxHdl;
    return SAL_SOK;

}

/******************************************************************
   Function:   recode_tskPackCreate
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static INT32 recode_tskPackCreate(UINT32 chan)
{
    INT32 status = SAL_SOK;
    RECODE_OBJ_PRM *pstObjPrm = NULL;
    MUX_STREAM_INFO_S streamInfo;

    if (chan >= RECODE_MAX_CHN)
    {
        RECODE_LOGE("chan %d error !!!\n", chan);
        return SAL_FAIL;
    }

    pstObjPrm = &g_stRecodeObjCommon.recodeStObj[chan];


    if (pstObjPrm->recodeChn.dstPackType & (0xf & (0x1 << DEM_PS)))
    {
        /* RECODE_LOGI("Create Ps ChnHandle!!!\n"); */
        if (SAL_SOK != recode_tskMuxCreate(MUX_PS, &pstObjPrm->recodePack.PsHandle))
        {
            RECODE_LOGE("Ps Pack Create Failed !!!\n");
            return SAL_FAIL;
        }
    }

    if (pstObjPrm->recodeChn.dstPackType & (0xf & (0x1 << DEM_RTP)))
    {
        /* RECODE_LOGI("Create RTP ChnHandle!!!\n"); */
        if (SAL_SOK != recode_tskMuxCreate(MUX_RTP, &pstObjPrm->recodePack.RtpHandle))
        {
            RECODE_LOGE("RTP Pack Create Failed !!!\n");
            return SAL_FAIL;
        }
    }

    /* 申请存放码流的内存 */
    memset(&streamInfo, 0, sizeof(streamInfo));
    streamInfo.muxType = MUX_VID;
    status = MuxControl(MUX_GET_STREAMSIZE, &streamInfo, &pstObjPrm->recodePack.packBufSize);
    if (status != SAL_SOK)
    {
        RECODE_LOGE("MuxControl idx %d err %#x\n", MUX_GET_STREAMSIZE, status);
        return SAL_FAIL;
    }

    pstObjPrm->recodePack.packOutputBuf = (UINT8 *)SAL_memMalloc(pstObjPrm->recodePack.packBufSize, "recode", "packOutput");
    if (NULL == pstObjPrm->recodePack.packOutputBuf)
    {
        RECODE_LOGE("packOutputBuf malloc err \n");
        return SAL_FAIL;
    }

    pstObjPrm->recodePack.packTmpBufSize = 512 * 1024;
    pstObjPrm->recodePack.packTmpBuf = (UINT8 *)SAL_memMalloc(pstObjPrm->recodePack.packTmpBufSize, "recode", "packTmp");
    if (NULL == pstObjPrm->recodePack.packTmpBuf)
    {
        RECODE_LOGE("packOutputBuf malloc err \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
   Function:   recode_tskVideoFramePack
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 recode_tskVideoFramePack(UINT32 chan, DEM_BUFFER *pDemBuf)
{
    MUX_IN_BITS_INTO_S *pMuxInInfo = NULL;
    MUX_OUT_BITS_INTO_S *pMuxOutInfo = NULL;
    MUX_PROC_INFO_S muxProcInfo;
    UINT32 isKeyFrame = 0;
    RECODE_OBJ_PRM *pstObjPrm = NULL;
    INT32 ret = 0;
    REC_STREAM_INFO_ST stRecStreamInfo;
    NET_STREAM_INFO_ST stNetStreamInfo;
    MUX_INFO muxInfo = {0};
    UINT64 u64PhyAddr = 0;

    if (chan >= RECODE_MAX_CHN)
    {
        RECODE_LOGE("chan %d error\n", chan);
        return SAL_FAIL;
    }

    if (NULL == pDemBuf)
    {
        RECODE_LOGE("chan %d error null\n", chan);
        return SAL_FAIL;
    }

    memset(&muxProcInfo, 0, sizeof(MUX_PROC_INFO_S));
    pMuxInInfo = &muxProcInfo.muxData.stInBuffer;
    pMuxOutInfo = &muxProcInfo.muxData.stOutBuffer;

    pstObjPrm = &g_stRecodeObjCommon.recodeStObj[chan];

    pMuxInInfo->bVideo = SAL_TRUE;

    if (NULL == pDemBuf->addr)
    {
        RECODE_LOGE("chan %d error %p,%p,%p,%p\n", chan, pDemBuf->addr, pDemBuf->spsAddr, pDemBuf->ppsAddr, pDemBuf->contAddr);
        return SAL_FAIL;
    }

    /* TODO:需要考虑到后续安检机功能的使用问题 */
    if (recode_tskJudgeNeedDelay() && (DEBUG_RECODE_CHAN_MAX > chan) )
    {
#if 1   /* Debug: 转码封装码流中增加智能帧信息 */
        UINT32 i = 0;
        UINT32 u32NameLen = 0;
        UINT32 ivsBufLen = 0;
        UINT32 syncBufLen = 0;
        UINT32 alert_num = 0;
        UINT32 length_int = 0;
        UINT32 reliability = 0;
        UINT32 status = 0;
        /* UINT64 u64PTS = 0; */

        SVA_PROCESS_OUT *pstOut = NULL;
        HIK_MULT_VCA_TARGET_LIST *pstTarget_list = NULL;
        IVS_SYS_PROC_PARAM *pstProc_param = NULL;
        IS_PRIVT_INFO_CONTRABAND *pstPrivtInfo = NULL;

        UINT8 au8OsdBuff[100] = {0};
        UINT8 syncCmdBuf[1024] = {0};
        UINT8 ivsPrivtBuf[10000] = {0};

        memset(&stOut[chan], 0x00, sizeof(SVA_PROCESS_OUT));

        pstOut = &stOut[chan];
        pstTarget_list = &target_list[chan];
        pstProc_param = &proc_param[chan];

        if (!pOutBuf[chan])
        {
            MUX_STREAM_INFO_S streamInfo = {0};

            streamInfo.muxType = MUX_VID; /* pInPrm->DataType; */
            status = MuxControl(MUX_GET_STREAMSIZE, &streamInfo, &uiOutBufSize[chan]);
            if (status != SAL_SOK)
            {
                VENC_LOGE("MuxControl idx %d err %#x\n", MUX_GET_STREAMSIZE, status);
                return SAL_FAIL;
            }

            ret = mem_hal_mmzAlloc(uiOutBufSize[chan], "recode", "recode_prvt", NULL, SAL_FALSE, &u64PhyAddr, (VOID **)&pOutBuf[chan]);
            if (ret != SAL_SOK)
            {
                VENC_LOGE("mem_hal_mmzAlloc err %#x\n", ret);
                return SAL_FAIL;
            }

            if (NULL == pOutBuf[chan])
            {
                VENC_LOGE("pOutBuf alloc mmz err \n");
                return SAL_FAIL;
            }
        }

        /* 整合引擎返回的pos信息，填充相关码流智能帧信息 */
        SVA_RECT_F *pstRect = NULL;

        /* 转封装相机通道号: 0--人脸相机，1--三目相机 */
        //sal_memset_s(&stPosInfo[0], sizeof(PPM_POS_INFO_S), 0x00, sizeof(PPM_POS_INFO_S));        
        //sal_memset_s(&stPosInfo[1], sizeof(PPM_POS_INFO_S), 0x00, sizeof(PPM_POS_INFO_S));
        if (DEBUG_FACE_RECODE_CHAN == chan)
        {
            if (SAL_SOK != Ppm_DrvGetPosInfo(0, &stPosInfo[0]))
            {
                SAL_ERROR("get pos info failed! chan %d \n", chan);
                return SAL_FAIL;
            }

            /* u64PTS = stPosInfo[0].u64PTS; */
            SAL_DEBUG("recode get pos: frmNum %d, time %d, chan %d, uiFaceCnt %d \n", stPosInfo[0].uiReserved[0], SAL_getCurMs(), chan, stPosInfo[0].stFaceRectInfo.uiFaceCnt);

            /* 填充人脸相关目标数据 */
            for (i = 0; i < stPosInfo[0].stFaceRectInfo.uiFaceCnt; i++)
            {
                pstRect = &stPosInfo[0].stFaceRectInfo.stFaceRect[i];

                pstOut->target[pstOut->target_num].type = 777;
                pstOut->target[pstOut->target_num].alarm_flg = 1;
                pstOut->target[pstOut->target_num].ID = 777;
                pstOut->target[pstOut->target_num].visual_confidence = 77 + 7 * i;
                /* sprintf((char *)pstOut->target[pstOut->target_num].sva_name, "test_%d", i); */
                memcpy((char *)pstOut->target[pstOut->target_num].sva_name, stPosInfo[0].stFaceRectInfo.str[i], 20);
                pstOut->target[pstOut->target_num].color = 0xFF0000;
                SAL_DEBUG("recode get pos:face frmNum %d, time %d, chan %d, uiFaceCnt %d, name %s \n", 
                    stPosInfo[0].uiReserved[0], SAL_getCurMs(), chan, stPosInfo[0].stFaceRectInfo.uiFaceCnt,
                    pstOut->target[pstOut->target_num].sva_name);

                pstOut->target[pstOut->target_num].rect.x = pstRect->x;
                pstOut->target[pstOut->target_num].rect.y = pstRect->y;
                pstOut->target[pstOut->target_num].rect.width = pstRect->width;
                pstOut->target[pstOut->target_num].rect.height = pstRect->height;

                pstOut->target_num++;
            }

            /* 画人脸规则区域 */
            pstRect = &stPosInfo[0].stRuleRgnInfo.stFaceRgn;

            pstOut->target[pstOut->target_num].type = 777;
            pstOut->target[pstOut->target_num].color = 0x00FF00;
            pstOut->target[pstOut->target_num].rect.x = pstRect->x;
            pstOut->target[pstOut->target_num].rect.y = pstRect->y;
            pstOut->target[pstOut->target_num].rect.width = pstRect->width;
            pstOut->target[pstOut->target_num].rect.height = pstRect->height;
            sprintf((char *)pstOut->target[pstOut->target_num].sva_name, "Face区域%d", chan);

            pstOut->target_num++;
        }
        else if (DEBUG_TRI_RECODE_CHAN == chan)
        {
            if (SAL_SOK != Ppm_DrvGetPosInfo(0, &stPosInfo[1]))
            {
                SAL_ERROR("get pos info failed! chan %d \n", chan);
                return SAL_FAIL;
            }

            /* u64PTS = stPosInfo[1].u64PTS; */
            SAL_DEBUG("recode get pos: frmNum %d, time %d, chan %d, uiJointsCnt %d \n", 
            stPosInfo[1].uiReserved[0], SAL_getCurMs(), chan, stPosInfo[1].stJointsRectInfo.uiJointsCnt);

            /* 填充三目相关关节点目标数据 */
            for (i = 0; i < stPosInfo[1].stJointsRectInfo.uiJointsCnt; i++)
            {
                pstRect = &stPosInfo[1].stJointsRectInfo.stJointsRect[i];

                pstOut->target[pstOut->target_num].type = 777;
                pstOut->target[pstOut->target_num].alarm_flg = 1;
                pstOut->target[pstOut->target_num].ID = 777;
                pstOut->target[pstOut->target_num].visual_confidence = 77 + 7 * i;
                /* sprintf((char *)pstOut->target[pstOut->target_num].sva_name, "test_%d", i); */
                memcpy((char *)pstOut->target[pstOut->target_num].sva_name, stPosInfo[1].stJointsRectInfo.str[i], 20);
                pstOut->target[pstOut->target_num].color = 0xFF0000;
                SAL_DEBUG("recode get pos:pkg frmNum %d, time %d, chan %d, uiJointsCnt %d, name %s \n", 
                    stPosInfo[1].uiReserved[0], SAL_getCurMs(), chan, stPosInfo[1].stJointsRectInfo.uiJointsCnt,
                    pstOut->target[pstOut->target_num].sva_name);

                pstOut->target[pstOut->target_num].rect.x = pstRect->x;
                pstOut->target[pstOut->target_num].rect.y = pstRect->y;
                pstOut->target[pstOut->target_num].rect.width = pstRect->width;
                pstOut->target[pstOut->target_num].rect.height = pstRect->height;

                pstOut->target_num++;
            }

            /* 画Cap A区域 */
            pstRect = &stPosInfo[1].stRuleRgnInfo.stCapARgn;

            pstOut->target[pstOut->target_num].type = 777;
            pstOut->target[pstOut->target_num].alarm_flg = 1;
            pstOut->target[pstOut->target_num].ID = 777;
            pstOut->target[pstOut->target_num].visual_confidence = 77 + 7 * i;
            sprintf((char *)pstOut->target[pstOut->target_num].sva_name, "A区域%d", chan);
            pstOut->target[pstOut->target_num].color = 0xFF0000;

            pstOut->target[pstOut->target_num].rect.x = pstRect->x;
            pstOut->target[pstOut->target_num].rect.y = pstRect->y;
            pstOut->target[pstOut->target_num].rect.width = pstRect->width;
            pstOut->target[pstOut->target_num].rect.height = pstRect->height;

            pstOut->target_num++;

            /* 画Cap B区域 */
            pstRect = &stPosInfo[1].stRuleRgnInfo.stCapBRgn;

            pstOut->target[pstOut->target_num].type = 777;
            pstOut->target[pstOut->target_num].alarm_flg = 1;
            pstOut->target[pstOut->target_num].ID = 777;
            pstOut->target[pstOut->target_num].visual_confidence = 77 + 7 * i;
            sprintf((char *)pstOut->target[pstOut->target_num].sva_name, "B区域%d",chan);
            pstOut->target[pstOut->target_num].color = 0xFF0000;

            pstOut->target[pstOut->target_num].rect.x = pstRect->x;
            pstOut->target[pstOut->target_num].rect.y = pstRect->y;
            pstOut->target[pstOut->target_num].rect.width = pstRect->width;
            pstOut->target[pstOut->target_num].rect.height = pstRect->height;

            pstOut->target_num++;
        }
        else
        {
            /* SAL_WARN("------------------------------- \n"); */
        }

        alert_num = (pstOut->target_num > 32) ? (32) : pstOut->target_num;

        for (i = 0; i < alert_num; i++)
        {
            /*IVS 信息*/
            pstTarget_list->p_target[i].target.type = 0;
            pstTarget_list->p_target[i].target.alarm_flg = pstOut->target[i].alarm_flg;             /* 算法给出结果中一致 */
            pstTarget_list->p_target[i].target.ID = pstOut->target[i].ID;                           /* 算法给出结果中一致 */
            pstTarget_list->p_target[i].target.rect.x = pstOut->target[i].rect.x;                   /* 0.0-1.0之间 */
            pstTarget_list->p_target[i].target.rect.y = pstOut->target[i].rect.y;                   /* 0.0-1.0之间 */
            pstTarget_list->p_target[i].target.rect.width = pstOut->target[i].rect.width;           /* 0.0-1.0之间 */
            pstTarget_list->p_target[i].target.rect.height = pstOut->target[i].rect.height;         /* 0.0-1.0之间 */

            if (sizeof(IS_PRIVT_INFO_CONTRABAND) <= IS_MAX_PRIVT_LEN)
            {
                pstTarget_list->p_target[i].privt_info.privt_type = IS_PRIVT_COLOR_CONTRABAND;
                pstTarget_list->p_target[i].privt_info.privt_len = sizeof(IS_PRIVT_INFO_CONTRABAND);

                pstPrivtInfo = (IS_PRIVT_INFO_CONTRABAND *)pstTarget_list->p_target[i].privt_info.privt_data;
                pstPrivtInfo->type = pstOut->target[i].type;
                pstPrivtInfo->color.alpha = 255;
                pstPrivtInfo->color.red = (pstOut->target[i].color >> 16) & 0xFF;
                pstPrivtInfo->color.green = (pstOut->target[i].color >> 8) & 0xFF;
                pstPrivtInfo->color.blue = (pstOut->target[i].color & 0xFF);

                memset(au8OsdBuff, 0, sizeof(au8OsdBuff));
                if (0) /* (pstIn->stTargetPrm.confidence) */
                {
                    pstPrivtInfo->confidence = pstOut->target[i].visual_confidence;  /* 置信度 */
                    reliability = pstPrivtInfo->confidence;
                    sprintf((char *)au8OsdBuff, "%s(%d%s)", pstOut->target[i].sva_name, reliability, "%");
                }
                else
                {
                    pstPrivtInfo->confidence = 0;  /* 置信度 */
                    sprintf((char *)au8OsdBuff, "%s", pstOut->target[i].sva_name);
                }

                u32NameLen = strlen((char *)au8OsdBuff);
                if (u32NameLen < sizeof(pstPrivtInfo->pos_data))
                {
                    pstPrivtInfo->pos_len = u32NameLen;
                    sal_memcpy_s((char *)pstPrivtInfo->pos_data,sizeof(pstPrivtInfo->pos_data) - 1, (const char *)au8OsdBuff, sizeof(pstPrivtInfo->pos_data) - 1);
                }
                else
                {
                    pstPrivtInfo->pos_len = 0;
                    VCA_LOGW("sva info size[%u] is bigger than buff\n", u32NameLen);
                }
            }
            else
            {
                VCA_LOGW("privt info size is bigger than buff\n");
            }
        }

        /* IVS 信息 */
        pstTarget_list->type = SHOW_TYPE_NORMAL | MATCH_TYPE_ACCURACY;
        pstTarget_list->target_num = alert_num;


        ret = IVS_MULT_META_DATA_to_system(pstTarget_list, pstProc_param);
        if (ret != HIK_IVS_SYS_LIB_S_OK)
        {
            VCA_LOGE("Error to mux meta data, error coed = %x\n", ret);
            return SAL_FAIL;
        }

        /* IVS 私有信息填充 */
        ivsBufLen = 0;
        /* 0 - 1 */
        ivsPrivtBuf[ivsBufLen++] = (unsigned char)(HIK_PRIVT_IVS_INFO >> 8);
        ivsPrivtBuf[ivsBufLen++] = (unsigned char)(HIK_PRIVT_IVS_INFO);
        /* 以4字节为单位，表示私有数据长度 */
        length_int = (pstProc_param->len + sizeof(ivs_hdr_t)) >> 2;
        /* 2 - 3 */
        ivsPrivtBuf[ivsBufLen++] = (length_int >> 8) & 0xff;
        ivsPrivtBuf[ivsBufLen++] = (length_int & 0xff);

        ret = vca_pack_makeAlgIvsInfoHdr(ivsPrivtBuf + ivsBufLen);
        if (ret != SAL_SOK)
        {
            VCA_LOGE("Vca_vencMakeAlgIvsInfoHdr error !\n");
            return SAL_FAIL;
        }

        ivsBufLen += sizeof(ivs_hdr_t);

        memcpy(ivsPrivtBuf + ivsBufLen, pstProc_param->buf, pstProc_param->len);
        ivsBufLen += pstProc_param->len;

        /* 打包相关 */
        memset(&muxProcInfo, 0, sizeof(MUX_PROC_INFO_S));
        pMuxInInfo = &muxProcInfo.muxData.stInBuffer;
        pMuxOutInfo	= &muxProcInfo.muxData.stOutBuffer;
        pMuxInInfo->bVideo = SAL_FALSE;
        pMuxInInfo->naluNum	= 1;
        pMuxInInfo->frame_type = FRAME_TYPE_PRIVT_FRAME;
        /* pMuxInInfo->u64PTS	= pstBitsDataInfo->stStreamFrameInfo[0].stNaluInfo[0].u64PTS; */
        pMuxInInfo->is_key_frame = 0;

        pstObjPrm->recodeTime.timeStamp = pDemBuf->timestamp;
        /* RECODE_LOGI("rtp %u,pts %u\n",pDemBuf->timestamp,pstObjPrm->recodeTime.timeStamp); */
        ret = recode_tskRtpUpdataPts(chan);
        if (ret != SAL_SOK)
        {
            RECODE_LOGE("Recode_drvRtpUpdataPts chan %d err %x\n", chan, ret);
            return SAL_FAIL;
        }

        pMuxInInfo->u64PTS = pstObjPrm->recodeTime.u64Pts_v / 1000;
        RECODE_LOGD("rtp %u,pts %u, u64PTS %llu\n", pDemBuf->timestamp, pstObjPrm->recodeTime.timeStamp, pMuxInInfo->u64PTS);

        syncBufLen = 0;

        syncCmdBuf[syncBufLen++] = (unsigned char)(0x1006 >> 8);
        syncCmdBuf[syncBufLen++] = (unsigned char)(0x1006);
        /* 以4字节为单位，表示私有数据长度 */
        length_int = (sizeof(HIK_COMMAND_INFO) + sizeof(ivs_hdr_t)) >> 2;
        /* 2 - 3 */
        syncCmdBuf[syncBufLen++] = (length_int >> 8) & 0xff;
        syncCmdBuf[syncBufLen++] = (length_int & 0xff);

        ret = vca_pack_makeAlgSyncCmd(syncCmdBuf + syncBufLen);
        if (ret != SAL_SOK)
        {
            VCA_LOGE("Vca_vencMakeAlgSyncCmd error !\n");
            return SAL_FAIL;
        }

        syncBufLen += sizeof(ivs_hdr_t);
        syncBufLen += sizeof(HIK_COMMAND_INFO);

        /*rtp 打包*/
        if (pstObjPrm->recodeChn.dstPackType & (0xf & (0x1 << DEM_RTP)))
        {
            muxProcInfo.muxHdl = pstObjPrm->recodePack.RtpHandle;  /* Rtp 打包创建的通道 */

            /**********************************************************************************************************/
            pMuxInInfo->bufferAddr[0] = syncCmdBuf;
            pMuxInInfo->bufferLen[0] = syncBufLen;

            pMuxOutInfo->streamLen = 0;
            pMuxOutInfo->bufferAddr = pOutBuf[chan];
            pMuxOutInfo->bufferLen = uiOutBufSize[chan];

            ret = MuxControl(MUX_PRCESS, &muxProcInfo, NULL);
            if (ret != SAL_SOK)
            {
                VCA_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, ret);
                return SAL_FAIL;
            }

            /* 写入共享内存 */
            memset(&stNetStreamInfo, 0, sizeof(NET_STREAM_INFO_ST));

            stNetStreamInfo.pucAddr = pMuxOutInfo->bufferAddr;
            stNetStreamInfo.uiSize = pMuxOutInfo->streamLen;
            stNetStreamInfo.uiType = 1;
            SystemPrm_writeToNetPoolByRecode(chan, &stNetStreamInfo);

            /**********************************************************************************************************/

            /**********************************************************************************************************/
            pMuxInInfo->bufferAddr[0] = ivsPrivtBuf;
            pMuxInInfo->bufferLen[0] = ivsBufLen;

            pMuxOutInfo->streamLen = 0;
            pMuxOutInfo->bufferAddr = pOutBuf[chan];
            pMuxOutInfo->bufferLen = uiOutBufSize[chan];

            ret = MuxControl(MUX_PRCESS, &muxProcInfo, NULL);
            if (ret != SAL_SOK)
            {
                VCA_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, ret);

                return SAL_FAIL;
            }

            /* 写入共享内存 */
            memset(&stNetStreamInfo, 0, sizeof(NET_STREAM_INFO_ST));

            stNetStreamInfo.pucAddr = pMuxOutInfo->bufferAddr;
            stNetStreamInfo.uiSize = pMuxOutInfo->streamLen;
            stNetStreamInfo.uiType = 1;
            SystemPrm_writeToNetPoolByRecode(chan, &stNetStreamInfo);

            /**********************************************************************************************************/
        }

        /* Ps 打包 */
        if (pstObjPrm->recodeChn.dstPackType & (0xf & (0x1 << DEM_PS)))
        {
            muxProcInfo.muxHdl = pstObjPrm->recodePack.PsHandle; /* Ps 打包创建的通道 */;

            /**********************************************************************************************************/
            /* SYNC 信息 */
            pMuxInInfo->bufferAddr[0] = syncCmdBuf;
            pMuxInInfo->bufferLen[0] = syncBufLen;

            pMuxOutInfo->streamLen = 0;
            pMuxOutInfo->bufferAddr = pOutBuf[chan];
            pMuxOutInfo->bufferLen = uiOutBufSize[chan];

            ret = MuxControl(MUX_PRCESS, &muxProcInfo, NULL);
            if (ret != SAL_SOK)
            {
                VCA_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, ret);
                return SAL_FAIL;
            }

            ret = MuxControl(MUX_GET_INFO, &pstObjPrm->recodePack.PsHandle, &muxInfo);
            if (ret != SAL_SOK)
            {
                VCA_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, ret);
                return SAL_FAIL;
            }

            /* 写入录像缓存 */
            memset(&stRecStreamInfo, 0, sizeof(REC_STREAM_INFO_ST));
            stRecStreamInfo.pucAddr = pMuxOutInfo->bufferAddr;
            stRecStreamInfo.uiSize = pMuxOutInfo->streamLen;
            stRecStreamInfo.uiType = 1;
            stRecStreamInfo.streamType = FRAME_TYPE_PRIVT_FRAME;
            stRecStreamInfo.IFrameInfo = (1 | (pMuxOutInfo->streamLen << 1));
            SAL_getDateTime(&stRecStreamInfo.absTime);
            stRecStreamInfo.stdTime = muxInfo.time_stamp;
            stRecStreamInfo.pts = pMuxInInfo->u64PTS;

            SystemPrm_writeRecPoolByRecode(chan, &stRecStreamInfo);

            /**********************************************************************************************************/

            /**********************************************************************************************************/
            /* IVS 信息 */
            pMuxInInfo->bufferAddr[0] = ivsPrivtBuf;
            pMuxInInfo->bufferLen[0] = ivsBufLen;

            pMuxOutInfo->streamLen = 0;
            pMuxOutInfo->bufferAddr = pOutBuf[chan];
            pMuxOutInfo->bufferLen = uiOutBufSize[chan];

            ret = MuxControl(MUX_PRCESS, &muxProcInfo, NULL);
            if (ret != SAL_SOK)
            {
                VCA_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, ret);
                return SAL_FAIL;
            }

            ret = MuxControl(MUX_GET_INFO, &pstObjPrm->recodePack.PsHandle, &muxInfo);
            if (ret != SAL_SOK)
            {
                VCA_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, ret);
                return SAL_FAIL;
            }

            /* 写入录像缓存 */
            memset(&stRecStreamInfo, 0, sizeof(REC_STREAM_INFO_ST));
            stRecStreamInfo.pucAddr = pMuxOutInfo->bufferAddr;
            stRecStreamInfo.uiSize = pMuxOutInfo->streamLen;
            stRecStreamInfo.uiType = 1;
            stRecStreamInfo.streamType = FRAME_TYPE_PRIVT_FRAME;
            stRecStreamInfo.IFrameInfo = (1 | (pMuxOutInfo->streamLen << 1));
            SAL_getDateTime(&stRecStreamInfo.absTime);
            stRecStreamInfo.stdTime = muxInfo.time_stamp;
            stRecStreamInfo.pts = pMuxInInfo->u64PTS;

            SystemPrm_writeRecPoolByRecode(chan, &stRecStreamInfo);

            /**********************************************************************************************************/
        }

#else

        pstObjPrm->recodeTime.timeStamp = pDemBuf->timestamp;
        /* RECODE_LOGI("rtp %u,pts %u\n",pDemBuf->timestamp,pstObjPrm->recodeTime.timeStamp); */
        ret = recode_tskRtpUpdataPts(chan);
        if (ret != SAL_SOK)
        {
            RECODE_LOGE("Recode_drvRtpUpdataPts chan %d err %x\n", chan, ret);
            return SAL_FAIL;
        }

        pMuxInInfo->u64PTS = pstObjPrm->recodeTime.u64Pts_v / 1000;
        RECODE_LOGD("rtp %u,pts %u, u64PTS %llu\n", pDemBuf->timestamp, pstObjPrm->recodeTime.timeStamp, pMuxInInfo->u64PTS);

#endif
    }

    pMuxInInfo->bVideo = SAL_TRUE;

    isKeyFrame = recode_tskIsKeyFrame(pDemBuf->addr);

    /* 海思3559a h265有6个nalu 分别是vps 、sps 、pps 、sei 、sei 、islice */
    /* 此处写死不具有通用性，不同平台sei nale数可能不同 */
    /* 应该从平台(platform)层获取。 */
    if (pDemBuf->streamType == H265)
    {
        if (SAL_TRUE == isKeyFrame)
        {
            if (NULL == pDemBuf->spsAddr || NULL == pDemBuf->spsAddr || NULL == pDemBuf->ppsAddr || NULL == pDemBuf->contAddr)
            {
                RECODE_LOGE("chan %d error %p,%p,%p,%p,%p,0x%x,frmTyp %d\n", chan, pDemBuf->addr, pDemBuf->spsAddr, pDemBuf->spsAddr, pDemBuf->ppsAddr, pDemBuf->contAddr, *(pDemBuf->addr + 4), pDemBuf->frmType);
                return SAL_FAIL;
            }

            pMuxInInfo->naluNum = 4;       /* 海思3559a h265有两个sei nalu; */
            pMuxInInfo->frame_type = STREAM_TYPE_H265_IFRAME;
            pMuxInInfo->is_key_frame = SAL_TRUE;

            pMuxInInfo->vpsLen = pDemBuf->vpsLen;
            pMuxInInfo->vpsBufferAddr = pDemBuf->vpsAddr;

            pMuxInInfo->spsLen = pDemBuf->spsLen;
            pMuxInInfo->spsBufferAddr = pDemBuf->spsAddr;


            pMuxInInfo->ppsLen = pDemBuf->ppsLen;
            pMuxInInfo->ppsBufferAddr = pDemBuf->ppsAddr;


            pMuxInInfo->bufferAddr[0] = pDemBuf->contAddr;
            pMuxInInfo->bufferLen[0] = pDemBuf->contLen;

            /* pMuxInInfo->u64PTS = pDemBuf->timestamp; */
            RECODE_LOGD("ps uiChn %d..u64PTS %llu, %d,%d,%d,%d,\n", chan, pMuxInInfo->u64PTS,
                        pMuxInInfo->spsLen, pMuxInInfo->ppsLen, pDemBuf->contLen, pDemBuf->datalen);
        }
        else
        {
            if (NULL == pDemBuf->contAddr)
            {
                RECODE_LOGE("chan %d error %p,%p\n", chan, pDemBuf->addr, pDemBuf->contAddr);
                return SAL_FAIL;
            }

            pMuxInInfo->naluNum = 1;
            pMuxInInfo->frame_type = STREAM_TYPE_H265_PFRAME;
            pMuxInInfo->is_key_frame = SAL_FALSE;
            pMuxInInfo->bufferAddr[0] = pDemBuf->contAddr;
            pMuxInInfo->bufferLen[0] = pDemBuf->contLen;
            /* pMuxInInfo->u64PTS = pDemBuf->timestamp; */
            RECODE_LOGD("ps uiChn %d..u64PTS %llu, %d,%d,%d,%d,\n", chan, pMuxInInfo->u64PTS,
                        pMuxInInfo->spsLen, pMuxInInfo->ppsLen, pDemBuf->contLen, pDemBuf->datalen);
        }
    }
    else if (pDemBuf->streamType == H264)
    {

        if (SAL_TRUE == isKeyFrame)
        {
            if (NULL == pDemBuf->spsAddr || NULL == pDemBuf->ppsAddr || NULL == pDemBuf->contAddr)
            {
                RECODE_LOGE("chan %d error %p,%p,%p,%p,date 0x%x,frmType %d\n", chan, pDemBuf->addr, pDemBuf->spsAddr, pDemBuf->ppsAddr, pDemBuf->contAddr, *(pDemBuf->addr + 4), pDemBuf->frmType);
                return SAL_FAIL;
            }

            pMuxInInfo->naluNum = 3;
            pMuxInInfo->frame_type = STREAM_TYPE_H264_IFRAME;
            pMuxInInfo->is_key_frame = SAL_TRUE;
            /* SPS 00 00 00 01 67 */
            pMuxInInfo->spsLen = pDemBuf->spsLen;
            pMuxInInfo->spsBufferAddr = pDemBuf->spsAddr;

            /* PPS 00 00 00 01 68 */
            pMuxInInfo->ppsLen = pDemBuf->ppsLen;
            pMuxInInfo->ppsBufferAddr = pDemBuf->ppsAddr;

            /* Islice  00 00 00 01 65 */
            pMuxInInfo->bufferAddr[0] = pDemBuf->contAddr;
            pMuxInInfo->bufferLen[0] = pDemBuf->contLen;

            pMuxInInfo->vpsLen = 0;              /* H264无vps */
            pMuxInInfo->vpsBufferAddr = NULL;    /* */
            /* pMuxInInfo->u64PTS = pDemBuf->timestamp; */
            RECODE_LOGD("ps uiChn %d..u64PTS %llu, %d,%d,%d,%d,\n", chan, pMuxInInfo->u64PTS,
                        pMuxInInfo->spsLen, pMuxInInfo->ppsLen, pDemBuf->contLen, pDemBuf->datalen);
        }
        else
        {
            if (NULL == pDemBuf->contAddr)
            {
                RECODE_LOGE("chan %d error %p,%p\n", chan, pDemBuf->addr, pDemBuf->contAddr);
                return SAL_FAIL;
            }

            pMuxInInfo->naluNum = 1;
            pMuxInInfo->frame_type = STREAM_TYPE_H264_PFRAME;
            pMuxInInfo->is_key_frame = SAL_FALSE;
            pMuxInInfo->bufferAddr[0] = pDemBuf->contAddr;
            pMuxInInfo->bufferLen[0] = pDemBuf->contLen;
            /* pMuxInInfo->u64PTS = pDemBuf->timestamp; */
            RECODE_LOGD("ps uiChn %d..u64PTS %llu, %d,%d,%d,%d,\n", chan, pMuxInInfo->u64PTS,
                        pMuxInInfo->spsLen, pMuxInInfo->ppsLen, pDemBuf->contLen, pDemBuf->datalen);
        }
    }
    else
    {
        RECODE_LOGE("recode_tskRtpUpdataPts chan %d err %x\n", chan, pDemBuf->streamType);
        return SAL_FAIL;
    }

    pstObjPrm->recodeTime.timeStamp = pDemBuf->timestamp;
    /* RECODE_LOGI("rtp %u,pts %u\n",pDemBuf->timestamp,pstObjPrm->recodeTime.timeStamp); */
    ret = recode_tskRtpUpdataPts(chan);
    if (ret != SAL_SOK)
    {
        RECODE_LOGE("recode_tskRtpUpdataPts chan %d err %x\n", chan, ret);
        return SAL_FAIL;
    }

    pMuxInInfo->u64PTS = pstObjPrm->recodeTime.u64Pts_v / 1000;
    RECODE_LOGD("rtp %u,pts %u, u64PTS %llu\n", pDemBuf->timestamp, pstObjPrm->recodeTime.timeStamp, pMuxInInfo->u64PTS);


    /* 如果是人包设备则进行全局时间戳赋值 */
    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_PPM))
    {
        RECODE_LOGD("PPM:ps uiChn %d..u64PTS %llu, year %d\n", chan, pMuxInInfo->u64PTS, pDemBuf->glb_time.year);
        pMuxInInfo->stGlobalTime.year = pDemBuf->glb_time.year;
        pMuxInInfo->stGlobalTime.month = pDemBuf->glb_time.month;
        pMuxInInfo->stGlobalTime.date = pDemBuf->glb_time.date;
        pMuxInInfo->stGlobalTime.hour = pDemBuf->glb_time.hour;
        pMuxInInfo->stGlobalTime.minute = pDemBuf->glb_time.minute;
        pMuxInInfo->stGlobalTime.second = pDemBuf->glb_time.second;
        pMuxInInfo->stGlobalTime.msecond = pDemBuf->glb_time.msecond;
    }

    /* Ps 打包 */
    if (pstObjPrm->recodeChn.dstPackType & (0xf & (0x1 << DEM_PS)))
    {
        RECODE_LOGD("ps uiChn %d..u64PTS %llu\n", chan, pMuxInInfo->u64PTS);
        pMuxOutInfo->streamLen = 0;
        pMuxOutInfo->bufferAddr = pstObjPrm->recodePack.packOutputBuf;
        pMuxOutInfo->bufferLen = pstObjPrm->recodePack.packBufSize;
        muxProcInfo.muxHdl = pstObjPrm->recodePack.PsHandle; /* Ps 打包创建的通道 */;
        ret = MuxControl(MUX_PRCESS, &muxProcInfo, NULL);
        if (ret != SAL_SOK)
        {
            RECODE_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, ret);
            return SAL_FAIL;
        }

        ret = MuxControl(MUX_GET_INFO, &pstObjPrm->recodePack.PsHandle, &muxInfo);
        if (ret != SAL_SOK)
        {
            RECODE_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, ret);
            return SAL_FAIL;
        }

        /* 写入录像缓存 */
        memset(&stRecStreamInfo, 0, sizeof(REC_STREAM_INFO_ST));
        stRecStreamInfo.pucAddr = pMuxOutInfo->bufferAddr;
        stRecStreamInfo.uiSize = pMuxOutInfo->streamLen;
        stRecStreamInfo.uiType = 1;
        stRecStreamInfo.streamType = pMuxInInfo->is_key_frame;
        stRecStreamInfo.IFrameInfo = (1 | (pMuxOutInfo->streamLen << 1));
        SAL_getDateTime(&stRecStreamInfo.absTime);
        stRecStreamInfo.stdTime = muxInfo.time_stamp;
        /* RECODE_LOGI("stdTime %u\n",stRecStreamInfo.stdTime); */
        stRecStreamInfo.pts = pMuxInInfo->u64PTS;
        SystemPrm_writeRecPoolByRecode(chan, &stRecStreamInfo);
    }

    /* Rtp 打包 */
    if (pstObjPrm->recodeChn.dstPackType & (0xf & (0x1 << DEM_RTP)))
    {
        RECODE_LOGD("RTP uiChn %d..u64PTS %llu\n", chan, pMuxInInfo->u64PTS);
        pMuxOutInfo->streamLen = 0;
        pMuxOutInfo->bufferAddr = pstObjPrm->recodePack.packOutputBuf;
        pMuxOutInfo->bufferLen = pstObjPrm->recodePack.packBufSize;
        muxProcInfo.muxHdl = pstObjPrm->recodePack.RtpHandle;       /* Rtp 打包创建的通道 */
        ret = MuxControl(MUX_PRCESS, &muxProcInfo, NULL);
        if (ret != SAL_SOK)
        {
            RECODE_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, ret);

            return SAL_FAIL;
        }

        /* 写入共享内存 */
        memset(&stNetStreamInfo, 0, sizeof(NET_STREAM_INFO_ST));
        stNetStreamInfo.pucAddr = pMuxOutInfo->bufferAddr;;
        stNetStreamInfo.uiSize = pMuxOutInfo->streamLen;;
        stNetStreamInfo.uiType = 1;
        SystemPrm_writeToNetPoolByRecode(chan, &stNetStreamInfo);
    }

    return SAL_SOK;
}

/******************************************************************
   Function:   recode_tskAudioFramePack
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 recode_tskAudioFramePack(UINT32 chan, DEM_BUFFER *pDemBuf)
{
    MUX_IN_BITS_INTO_S *pMuxInInfo = NULL;
    MUX_OUT_BITS_INTO_S *pMuxOutInfo = NULL;
    MUX_PROC_INFO_S muxProcInfo;
    RECODE_OBJ_PRM *pstObjPrm = NULL;
    INT32 ret = 0;
    REC_STREAM_INFO_ST stRecStreamInfo;
    NET_STREAM_INFO_ST stNetStreamInfo;

    /* AUD_BITS_INFO_ST   stAudBitsInfo    = {0}; */

    if (chan >= RECODE_MAX_CHN)
    {
        RECODE_LOGE("chan %d error\n", chan);
        return SAL_FAIL;
    }

    if (NULL == pDemBuf)
    {
        RECODE_LOGE("chan %d error null\n", chan);
        return SAL_FAIL;
    }

    pstObjPrm = &g_stRecodeObjCommon.recodeStObj[chan];
    memset(&muxProcInfo, 0, sizeof(MUX_PROC_INFO_S));
    pMuxInInfo = &muxProcInfo.muxData.stInBuffer;
    pMuxOutInfo = &muxProcInfo.muxData.stOutBuffer;

    if ((NULL == pDemBuf->addr) || (0 == pDemBuf->contLen) || (NULL == pDemBuf->contAddr))
    {
        RECODE_LOGE("chan %d error %p,datalen %d,contLen %d contAddr %p\n", chan, pDemBuf->addr, pDemBuf->datalen,\
                    pDemBuf->contLen,pDemBuf->contAddr);
        return SAL_FAIL;
    }

    if (AUDIO_G711_U == pDemBuf->audEncType)
    {
        RECODE_LOGD("G711_MU,len %d\n", pDemBuf->datalen);
    }
    else if (AUDIO_G711_A == pDemBuf->audEncType)
    {
        RECODE_LOGD("G711_A\n");
    }
    else
    {
        RECODE_LOGD("audType %x\n", pDemBuf->audEncType);
    }

    pMuxInInfo->naluNum = 1;
    pMuxInInfo->bVideo = 0;
    pMuxInInfo->audExInfo = 0;
    pMuxInInfo->frame_type = FRAME_TYPE_AUDIO_FRAME;
    pMuxInInfo->audEnctype = pDemBuf->audEncType;
    pMuxInInfo->audChn = pstObjPrm->recodeChn.audioChannels + 1;       /* jdw说海康私有帧里面1表示单声道，2表示双声道 */
    pMuxInInfo->audSampleRate = pstObjPrm->recodeChn.audioSamplesPerSec;
    pMuxInInfo->audBitRate = pstObjPrm->recodeChn.audioBitsPerSample;
    pstObjPrm->recodeAudTime.timeStamp = pDemBuf->timestamp;
    /* RECODE_LOGI("rtp %u,pts %u\n",pDemBuf->timestamp,pstObjPrm->recodeTime.timeStamp); */
    ret = recode_tskRtpUpdataAudPts(chan);
    if (ret != SAL_SOK)
    {
        RECODE_LOGE("recode_tskRtpUpdataPts chan %d err %x\n", chan, ret);
        return SAL_FAIL;
    }

    pMuxInInfo->u64PTS = pstObjPrm->recodeAudTime.u64Pts_v / 1000;
    RECODE_LOGD("rtp %u,pts %u, u64PTS %llu\n", pDemBuf->timestamp, pstObjPrm->recodeAudTime.timeStamp, pMuxInInfo->u64PTS);

    if ((pstObjPrm->recodeChn.dstPackType & (0xf & (0x1 << DEM_PS))) && pstObjPrm->recodeChn.audPackPs)
    {
        if (AUDIO_AAC == pDemBuf->audEncType)
        {
            if (pDemBuf->contLen < 4)
            {
                RECODE_LOGE("error chan %d audEncType:AUDIO_AAC contLen %d too small\n", chan, pDemBuf->contLen);
                return SAL_FAIL;
            }
            if (pDemBuf->contLen + 7 <= pstObjPrm->recodePack.packTmpBufSize)
            {
                /* 组装aac的头部和数据 */
                memmove(pstObjPrm->recodePack.packTmpBuf, g_PackAacAdtsHeader[chan], 7);
                memmove(pstObjPrm->recodePack.packTmpBuf + 7, pDemBuf->contAddr + 4, pDemBuf->contLen - 4); /* 去掉rtp打包额外加的4个字节 */
                pMuxInInfo->bufferAddr[0] = pstObjPrm->recodePack.packTmpBuf;
                pMuxInInfo->bufferLen[0] = pDemBuf->contLen + 7 - 4;
            }
            else
            {
                RECODE_LOGW("uiChn %d..contLen %d, packTmpBufSize %d\n", chan, pDemBuf->contLen, pstObjPrm->recodePack.packTmpBufSize);
            }
        }
        else
        {
            pMuxInInfo->bufferAddr[0] = pDemBuf->contAddr;
            pMuxInInfo->bufferLen[0] = pDemBuf->contLen;
        }

        pMuxOutInfo->streamLen = 0;
        pMuxOutInfo->bufferAddr = pstObjPrm->recodePack.packOutputBuf;
        pMuxOutInfo->bufferLen = pstObjPrm->recodePack.packBufSize;
        muxProcInfo.muxHdl = pstObjPrm->recodePack.PsHandle; /* Ps 打包创建的通道 */;
        ret = MuxControl(MUX_PRCESS, &muxProcInfo, NULL);
        if (ret != SAL_SOK)
        {
            RECODE_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, ret);
            return SAL_FAIL;
        }

        /* 写入录像缓存 */
        memset(&stRecStreamInfo, 0, sizeof(REC_STREAM_INFO_ST));
        stRecStreamInfo.pucAddr = pMuxOutInfo->bufferAddr;
        stRecStreamInfo.uiSize = pMuxOutInfo->streamLen;
        stRecStreamInfo.uiType = 0;
        stRecStreamInfo.streamType = 0;
        stRecStreamInfo.pts = pMuxInInfo->u64PTS;
        /* RECODE_LOGW("ps uiChn %d..uiSize %d,bVideo %d\n",chan,stRecStreamInfo.uiSize,pMuxInInfo->bVideo); */
        SystemPrm_writeRecPoolByRecode(chan, &stRecStreamInfo);
    }

    /* Rtp 打包 */
    if ((pstObjPrm->recodeChn.dstPackType & (0xf & (0x1 << DEM_RTP))) && pstObjPrm->recodeChn.audPackRtp)
    {
        RECODE_LOGD("RTP uiChn %d..u64PTS %llu\n", chan, pMuxInInfo->u64PTS);
        pMuxInInfo->bufferAddr[0] = pDemBuf->contAddr;
        pMuxInInfo->bufferLen[0] = pDemBuf->contLen;
        pMuxOutInfo->streamLen = 0;
        pMuxOutInfo->bufferAddr = pstObjPrm->recodePack.packOutputBuf;
        pMuxOutInfo->bufferLen = pstObjPrm->recodePack.packBufSize;
        muxProcInfo.muxHdl = pstObjPrm->recodePack.RtpHandle;       /* Rtp 打包创建的通道 */
        ret = MuxControl(MUX_PRCESS, &muxProcInfo, NULL);
        if (ret != SAL_SOK)
        {
            RECODE_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, ret);

            return SAL_FAIL;
        }

        /* 写入共享内存 */
        memset(&stNetStreamInfo, 0, sizeof(NET_STREAM_INFO_ST));
        stNetStreamInfo.pucAddr = pMuxOutInfo->bufferAddr;;
        stNetStreamInfo.uiSize = pMuxOutInfo->streamLen;;
        stNetStreamInfo.uiType = 1;
        SystemPrm_writeToNetPoolByRecode(chan, &stNetStreamInfo);
    }

    return SAL_SOK;
}

/**
 * @function   recode_tskSendMetaToPpm
 * @brief      将解封装得到的metadata数据送入人包模块处理
 * @param[in]  DEM_BUFFER *pstDemBuf  
 * @param[out] None
 * @return     static INT32
 */
static INT32 recode_tskSendMetaToPpm(DEM_BUFFER *pstDemBuf)
{
	INT32 s32Ret = SAL_FAIL;
	
	if (NULL == pstDemBuf)
	{
		RECODE_LOGE("pstDemBuf == null! \n");
		return SAL_FAIL;
	}
	
	static UINT32 time_arr[4] = {0};
	
	if (time_arr[0] == 0)
	{
		time_arr[0] = SAL_getCurMs();
		time_arr[2] = 0;
	}
	
	s32Ret = Ppm_DrvGenerateDepthInfo(pstDemBuf->addr, pstDemBuf->datalen);
	if (SAL_SOK != s32Ret)
	{
		RECODE_LOGE("generate depth info failed! \n");
		return SAL_FAIL;
	}

	time_arr[1] = SAL_getCurMs();
	time_arr[2]++;

	RECODE_LOGD("============dataLen %d \n", pstDemBuf->datalen);

	if (time_arr[1] - time_arr[0] >= 2000)
	{
		RECODE_LOGW("get depth [%d] times, cost [%d] ms. \n", time_arr[2], time_arr[1] - time_arr[0]);
		time_arr[0] = 0;
	}
	
	return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskDemuxThread
* 描  述  :
* 输  入  : - prm: 通道号
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
VOID *recode_tskDemuxThread(VOID *prm)
{
    UINT32 variable = 0;
    UINT32 chan = 0;
    INT32 s32Ret = SAL_SOK;
    DSPINITPARA *pDspInit = SystemPrm_getDspInitPara();
    DEC_SHARE_BUF *pDecShare = NULL;
    RECODE_OBJ_PRM *pstObjPrm = NULL;
    /* RECODE_MOD_CTRL    *pDemuxCtrl      = NULL; */
    DEM_CALC_PRM calcPrm;
    DEM_PRCO_OUT procOut;
    DEM_BUFFER demBuf;
    /* SAL_VideoFrameBuf   decFrameIn; */
    INT32 *Handle = NULL;
    UINT32 uiDataLen = 0;
    DEC_SHARE_BUF stDecTmpMem = {0};
    DEC_SHARE_BUF *pstTmpMem = NULL;

    if (NULL == pDspInit)
    {
        RECODE_LOGE("!!!pDspInit is null \n");
        return NULL;
    }

    if (NULL == prm)
    {
        RECODE_LOGE("!!!Input prm error \n");
        return NULL;
    }

    memset(&demBuf, 0, sizeof(demBuf));
    pstObjPrm = (RECODE_OBJ_PRM *)prm;
    chan = pstObjPrm->uiChn;

    if (chan >= RECODE_MAX_CHN)
    {
        RECODE_LOGE("!!!recode Chn(%u) error \n", chan);
        return NULL;
    }

    pDecShare = &pDspInit->ipcShareBuf[chan];
    /* pDemuxCtrl   = &pstObjPrm->recodeModCtrl; */
    pstTmpMem = pDecShare;                 /* 套娃 */

    /* 套娃初始化 */
    if (recode_tskJudgeNeedDelay() && (DEBUG_RECODE_CHAN_MAX > chan) )
    {
        stDecTmpMem.writeIdx = stDebugProcPrm[chan].uiWIdx;
        stDecTmpMem.readIdx = stDebugProcPrm[chan].uiRIdx;
        stDecTmpMem.bufLen = stDebugProcPrm[chan].uiMemLen;
        stDecTmpMem.pVirtAddr = (unsigned char *)stDebugProcPrm[chan].mem;
    }

    if (pstObjPrm->recodeChn.packType == DEM_RTP)
    {
        pstObjPrm->recodeModCtrl.chnHandle = pstObjPrm->recodeModCtrl.rtpHandle;
        /* RECODE_LOGI("chan %d rtpHandle = 0x%x \n",chan,pDemuxCtrl->rtpHandle); */
        RECODE_LOGI("chan %d rtpHandle = 0x%x \n", chan, pstObjPrm->recodeModCtrl.chnHandle);
    }
    else if (pstObjPrm->recodeChn.packType == DEM_PS)
    {
        pstObjPrm->recodeModCtrl.chnHandle = pstObjPrm->recodeModCtrl.psHandle;
        /* RECODE_LOGI("chan %d psHandle = 0x%x \n",chan,pDemuxCtrl->psHandle); */
        RECODE_LOGI("chan %d psHandle = 0x%x \n", chan, pstObjPrm->recodeModCtrl.chnHandle);
    }

    Handle = &pstObjPrm->recodeModCtrl.chnHandle;

    SAL_SET_THR_NAME();

    while (1)
    {
        /*阻塞等待模块开关*/
        recode_tskDemuxWait(chan);

        /* 当前策略:
        ** 1. 根据算法处理平均耗时1300ms计算，将转封装的码流数据延迟32帧送处理。
        ** 2. 将算法结果进行缓存，并在转封装中进行时间戳匹配，寻找最相近的结果进行后续智能帧封包处理。暂定缓存个数为32
        */
#if 1
		/* todo: 后续将转封装码流帧延迟通用化，配置参数作为recode入参传入，与人包解耦 */
        if (recode_tskJudgeNeedDelay() && (DEBUG_RECODE_CHAN_MAX > chan) )
        {
            /* 更新上次处理完的共享缓冲区读写索引 */
            stDebugProcPrm[chan].uiWIdx = stDecTmpMem.writeIdx;
            stDebugProcPrm[chan].uiRIdx = stDecTmpMem.readIdx;
            
            RECODE_LOGD("chan %d need delay! \n", chan);
            if (get_buf_full_sts(chan)) /* 判断debug结构体 buf是否已满 */
            {
                RECODE_LOGD("before get buf! chan %d \n", chan);

                if (SAL_SOK != cpy_to_mem(chan,
                                          stDebugProcPrm[chan].mem,
                                          stDebugProcPrm[chan].uiWIdx,
                                          stDebugProcPrm[chan].uiRIdx,
                                          stDebugProcPrm[chan].uiMemLen))
                {
                    SAL_ERROR("cpy mem failed! chan %d \n", chan);
                    continue;
                }
            }

            /* ipc的共享缓冲区数据拷贝到  全局变量debug结构体 */
            if (SAL_SOK != fill_buf(chan,
                                    (char *)pDecShare->pVirtAddr,
                                    pDecShare->writeIdx,
                                    pDecShare->readIdx,
                                    pDecShare->bufLen,
                                    &uiDataLen))
            {
                SAL_ERROR("fill buf failed! chan %d \n", chan);
                continue;
            }

            /* 更新缓冲buf的满状态，debug结构体的写指针自加，超过32，则判断为buf已满 */
            (VOID)update_fill_buf_flag(chan);
            RECODE_LOGD("fill buf! uiDataLen[%d], bufWIdx[%d], bufRIdx[%d], flag %d, chan %d \n",
                        uiDataLen, stDebugProcPrm[chan].uiBufWIdx, stDebugProcPrm[chan].uiBufRIdx, stDebugProcPrm[chan].bBufFillEnd, chan);

            /* 更新外部填充共享缓存的读指针，我们只维护读指针，外部维护写指针 */
            (VOID)update_mem_idx(uiDataLen, pDecShare->bufLen, &pDecShare->readIdx);

            stDecTmpMem.writeIdx = stDebugProcPrm[chan].uiWIdx;
            stDecTmpMem.readIdx = stDebugProcPrm[chan].uiRIdx;
            pstTmpMem = &stDecTmpMem;             /* 启用套娃 */
        }
#endif

        calcPrm.demHdl = *Handle;
        calcPrm.orgWIdx = pstTmpMem->writeIdx;
        s32Ret = DemuxControl(DEM_CALC_DATA, &calcPrm, &variable);
        if (variable <= 255)
        {
            usleep(10 * 1000);
            continue;
        }

        /* 解封装处理，拿到完整一帧结果后再处理 */
        s32Ret = DemuxControl(DEM_PROC_DATA, Handle, &procOut);
        pstTmpMem->readIdx = procOut.orgRIdx;
        if ((!procOut.newFrm) || (DEMUX_S_OK != s32Ret))
        {
        	//printf("-------------- newFrm %d, s32Ret %d \n", procOut.newFrm, s32Ret);
            usleep(2 * 1000);
            continue;
        }
        else
        {
            if (STREAM_FRAME_TYPE_AUDIO == procOut.frmType)
            {
                demBuf.bufType = STREAM_FRAME_TYPE_AUDIO;
                demBuf.addr = NULL;
                demBuf.length = 0;
                demBuf.datalen = 0;

                s32Ret = DemuxControl(DEM_FULL_BUF, Handle, &demBuf);
                if (s32Ret == 0)
                {
                    if ((demBuf.addr == NULL) || (0 == demBuf.contLen) || (NULL == demBuf.contAddr))
                    {
                        RECODE_LOGE("demBuf.addr = %p %u, %u contLen %u contAddr %p\n", demBuf.addr, demBuf.datalen,\
                            demBuf.audEncType, demBuf.contLen,demBuf.contAddr);
                        continue;
                    }

                    if (SAL_SOK != recode_tskAudioFramePack(chan, &demBuf))
                    {
                        RECODE_LOGE("recode_tskAudioFramePack %d error\n", chan);
                        continue;
                    }
                }
            }
            else if (STREAM_FRAME_TYPE_VIDEO == procOut.frmType)
            {
                demBuf.bufType = STREAM_FRAME_TYPE_VIDEO;
                demBuf.addr = NULL;
                demBuf.spsAddr = NULL;
                demBuf.vpsAddr = NULL;
                demBuf.ppsAddr = NULL;
                demBuf.contAddr = NULL;
                demBuf.length = 0;
                demBuf.datalen = 0;
                s32Ret = DemuxControl(DEM_FULL_BUF, Handle, &demBuf);
                if (s32Ret)
                {
                    RECODE_LOGE("new frame error. ret:%d, %d\n", s32Ret, demBuf.frmType);
                }
                else
                {
                    RECODE_LOGD("chan %d ,stp %d,addr %p,%p,%p,%p,%p\n", chan, demBuf.streamType, demBuf.addr, demBuf.vpsAddr, demBuf.spsAddr, demBuf.ppsAddr, demBuf.contAddr);
                    RECODE_LOGD("vpsLen %d,spsLen:%d,ppsLen:%d,key %d,0x%x,0x%x\n", demBuf.vpsLen, demBuf.spsLen, demBuf.ppsLen, recode_tskIsKeyFrame(demBuf.addr), (UINT32)*demBuf.addr, (UINT32)*(demBuf.addr + 4));
                    if (SAL_SOK != recode_tskVideoFramePack(chan, &demBuf))
                    {
                        RECODE_LOGE("recode_tskVideoFramePack %d error\n", chan);
                        continue;
                    }

                    /* write_es(chan, decFrameOut.virAddr[0], decFrameOut.frameParam.width * decFrameOut.frameParam.height * 3 / 2); */
                }
            }
			/* 新增meta解析帧类型，将解析后的负载payload信息同步到外部模块，如深度信息发送给人包 */
			else if (STREAM_FRAME_TYPE_METADATA == procOut.frmType)
			{
                demBuf.bufType = STREAM_FRAME_TYPE_METADATA;
                demBuf.addr = NULL;
                demBuf.length = 0;
                demBuf.datalen = 0;

                s32Ret = DemuxControl(DEM_FULL_BUF, Handle, &demBuf);
                if (s32Ret)
                {
                    RECODE_LOGE("new frame error. ret:%d \n", s32Ret);
                }
                else
                {
					/* 将获取到的负载数据进行格式化解析，生成结构化结果送入人包模块 */
                    if (SAL_SOK != recode_tskSendMetaToPpm(&demBuf))
                    {
                        RECODE_LOGE("recode_tskVideoFramePack %d error\n", chan);
                        continue;
                    }
                }
			}
            else
            {
                RECODE_LOGE("-----------------------error frmType %d.\n", procOut.frmType);
            }
        }

        //usleep(10 * 1000);
    }

    return NULL;
}

/*******************************************************************************
* 函数名  : recode_tskDemuxCtrl
* 描  述  :
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskDemuxCtrl(UINT32 chan, UINT32 Ctrl)
{
    RECODE_OBJ_PRM *pstObjPrm = NULL;
    RECODE_MOD_CTRL *pRecodeCtrl = NULL;

    if (chan >= RECODE_MAX_CHN)
    {
        RECODE_LOGE("recode obj index(%u) error !!!\n", chan);
        return SAL_FAIL;
    }

    pstObjPrm = &g_stRecodeObjCommon.recodeStObj[chan];
    pRecodeCtrl = &pstObjPrm->recodeModCtrl;

    SAL_mutexLock(pRecodeCtrl->mutexHdl);
    pRecodeCtrl->enable = Ctrl;
    SAL_mutexSignal(pRecodeCtrl->mutexHdl);
    SAL_mutexUnlock(pRecodeCtrl->mutexHdl);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskDemuxWait
* 描  述  :
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskDemuxWait(UINT32 chan)
{
    RECODE_OBJ_PRM *pstObjPrm = NULL;
    RECODE_MOD_CTRL *pRecodeCtrl = NULL;

    if (chan >= RECODE_MAX_CHN)
    {
        RECODE_LOGE("recode obj index(%u) error !!!\n", chan);
        return SAL_FAIL;
    }

    pstObjPrm = &g_stRecodeObjCommon.recodeStObj[chan];
    pRecodeCtrl = &pstObjPrm->recodeModCtrl;

    SAL_mutexLock(pRecodeCtrl->mutexHdl);

    if (SAL_FALSE == pRecodeCtrl->enable)
    {
        SAL_mutexWait(pRecodeCtrl->mutexHdl);
    }

    SAL_mutexUnlock(pRecodeCtrl->mutexHdl);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskDemuxStart
* 描  述  : 开启解包
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskDemuxStart(UINT32 chan, RECODE_PRM *pRecodePrm)
{
    INT32 ret = 0;
    INT32 i = 0;
    INT32 val = 0;
    UINT32 encType = 0;
    UINT32 demuxChan = 0;
    RECODE_OBJ_PRM *pstObjPrm = NULL;
    DSPINITPARA *pDspInit = SystemPrm_getDspInitPara();
    DEC_SHARE_BUF *pDecShare = NULL;
    DEM_CALC_PRM calcPrm;
    UINT32 u32Variable = 0;
    INT32 *Handle = NULL;
    INT32 s32ResEnc = 0;
    
    pDecShare = &pDspInit->ipcShareBuf[chan];

    if (chan >= RECODE_MAX_CHN)
    {
        RECODE_LOGE("recode Demux chan(%u) error \n", chan);
        return SAL_FAIL;
    }

    if (NULL == pRecodePrm)
    {
        RECODE_LOGE("chan(%u) error null\n", chan);
        return SAL_FAIL;
    }

    if (MPEG2MUX_STREAM_TYPE_RTP != pRecodePrm->srcPackType)
    {
        RECODE_LOGE("chan(%u) srcPackType %u error only support rtp\n", chan, pRecodePrm->srcPackType);
        return SAL_FAIL;
    }

    if ((0x0 == (pRecodePrm->dstPackType & (1 << MPEG2MUX_STREAM_TYPE_PS)))
        && (0x0 == (pRecodePrm->dstPackType & (1 << MPEG2MUX_STREAM_TYPE_RTP))))
    {
        RECODE_LOGE("chan(%u) detPackType 0x%x error only support rtp/ps\n", chan, pRecodePrm->dstPackType);
        return SAL_FAIL;
    }

    pstObjPrm = &g_stRecodeObjCommon.recodeStObj[chan];

    if (ENCODER_H264 == pRecodePrm->srcEncType || ENCODER_H265 == pRecodePrm->srcEncType)
    {
        encType = ENCODER_H264 == pRecodePrm->srcEncType ? 1 : 2; /* 1表示h264，2表示h265 */
    }
    else if (ENCODER_UNKOWN == pRecodePrm->srcEncType)
    {
        encType = 0; /* dsp 通过解包解析视频编码类型 */
        
        encType = 1; /* 暂时指定为h264 */
    }
    else
    {
        RECODE_LOGE("srcEncType %x error!\n", pRecodePrm->srcEncType);
        return SAL_FAIL;
    }

    demuxChan = pstObjPrm->recodeModCtrl.rtpHandle;

    Handle = &g_stRecodeObjCommon.recodeStObj[chan].recodeModCtrl.chnHandle;

    RECODE_LOGD("chan %d, chnHandle %d ,rtpHandle %d, psHandle %d \n", chan, pstObjPrm->recodeModCtrl.chnHandle,
              pstObjPrm->recodeModCtrl.rtpHandle, pstObjPrm->recodeModCtrl.psHandle);
    
    ret = DemuxControl(DEM_RESET, Handle, (void *)&s32ResEnc);
    if (ret < 0)
    {
        RECODE_LOGW("chan %d, ret %d DEM_RESET erro\n", chan, ret);
    }
    

    ret = DemuxControl(DEM_SET_ENC_TYPE, &demuxChan, &encType);
    if (SAL_SOK != ret)
    {
        RECODE_LOGE("chan %d error\n", chan);
        return SAL_FAIL;
    }
#if 0
    if (0x0 != (pRecodePrm->dstPackType & (1 << MPEG2MUX_STREAM_TYPE_PS)))
    {
        pstObjPrm->recodeChn.dstPackType |= (0xf & (0x1 << DEM_PS));
    }

    if (0x0 != (pRecodePrm->dstPackType & (1 << MPEG2MUX_STREAM_TYPE_RTP)))
    {
        pstObjPrm->recodeChn.dstPackType |= (0xf & (0x1 << DEM_RTP));
    }
#endif 
    pstObjPrm->recodeChn.audPackPs = pRecodePrm->audioPackPs;
    pstObjPrm->recodeChn.audPackRtp = pRecodePrm->audioPackRtp;
    pstObjPrm->recodeChn.audioChannels = pRecodePrm->audioChannels;
    pstObjPrm->recodeChn.audioSamplesPerSec = pRecodePrm->audioSamplesPerSec;
    pstObjPrm->recodeChn.audioBitsPerSample = pRecodePrm->audioBitsPerSample;
    if (pRecodePrm->audioSamplesPerSec > 0)
    {
        for (i = 0; i < 16; i++)
        {
            if (pRecodePrm->audioSamplesPerSec == g_PackAacAdtsHeaderSampleFre[i])
            {
                val = g_PackAacAdtsHeader[chan][2];
                if (i != ((val & 0x3c) >> 2))
                {
                    RECODE_LOGI("i %d,val %x_%d\n", i, ((val & 0x3c) >> 2), ((val & 0x3c) >> 2));
                    g_PackAacAdtsHeader[chan][2] &= 0xc3; /* ~((1 << 2) | (1 << 3) | (1 << 4) | (1 << 5));//置为0 */
                    g_PackAacAdtsHeader[chan][2] |= (i << 2);
                }

                val = g_PackAacAdtsHeader[chan][2];
                RECODE_LOGI("val %x_%d\n", ((val & 0x3c) >> 2), val);
                break;
            }
        }
    }
    

    calcPrm.demHdl = *Handle;
    calcPrm.orgWIdx = pDecShare->writeIdx;
    pDecShare->readIdx =calcPrm.orgWIdx;
    ret = DemuxControl(DEM_CALC_DATA, &calcPrm, &u32Variable);
    if (ret < 0)
    {
        RECODE_LOGW("chan %d DEM_CALC_DATA erro\n", chan);
    }

    RECODE_LOGI("chan %d,dstPackType 0x%x,srcEncType 0x%x,audPack %d_%d,demuxChan %d\n", chan, pstObjPrm->recodeChn.dstPackType, pRecodePrm->srcEncType, pRecodePrm->audioPackPs, pRecodePrm->audioPackRtp, demuxChan);
    ret = recode_tskDemuxCtrl(chan, SAL_TRUE);
    if (SAL_SOK != ret)
    {
        RECODE_LOGE("recode_tskDemuxCtrl(%u) error !!!\n", chan);
        return SAL_FAIL;
    }
    

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskDemuxAudStart
* 描  述  : 开启音频转封装
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskDemuxAudStart(UINT32 chan, RECODE_PRM *pRecodePrm)
{
    INT32 i = 0;
    INT32 val = 0;
    RECODE_OBJ_PRM *pstObjPrm = NULL;

    if (chan >= RECODE_MAX_CHN)
    {
        RECODE_LOGE("recode Demux chan(%u) error \n", chan);
        return SAL_FAIL;
    }

    if (NULL == pRecodePrm)
    {
        RECODE_LOGE("chan(%u) error null\n", chan);
        return SAL_FAIL;
    }

    pstObjPrm = &g_stRecodeObjCommon.recodeStObj[chan];

    pstObjPrm->recodeChn.audPackPs = pRecodePrm->audioPackPs;
    pstObjPrm->recodeChn.audPackRtp = pRecodePrm->audioPackRtp;
    pstObjPrm->recodeChn.audioChannels = pRecodePrm->audioChannels;
    pstObjPrm->recodeChn.audioSamplesPerSec = pRecodePrm->audioSamplesPerSec;
    pstObjPrm->recodeChn.audioBitsPerSample = pRecodePrm->audioBitsPerSample;
    if (pRecodePrm->audioSamplesPerSec > 0)
    {
        for (i = 0; i < 16; i++)
        {
            if (pRecodePrm->audioSamplesPerSec == g_PackAacAdtsHeaderSampleFre[i])
            {
                val = g_PackAacAdtsHeader[chan][2];
                if (i != ((val & 0x3c) >> 2))
                {
                    RECODE_LOGI("i %d,val %x_%d\n", i, ((val & 0x3c) >> 2), ((val & 0x3c) >> 2));
                    g_PackAacAdtsHeader[chan][2] &= 0xc3; /* ~((1 << 2) | (1 << 3) | (1 << 4) | (1 << 5));//置为0 */
                    g_PackAacAdtsHeader[chan][2] |= (i << 2);
                }

                val = g_PackAacAdtsHeader[chan][2];
                RECODE_LOGI("val %x_%d\n", ((val & 0x3c) >> 2), val);
                break;
            }
        }
    }

    RECODE_LOGI("chan %d,audPack %d_%d\n", chan, pRecodePrm->audioPackPs, pRecodePrm->audioPackRtp);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskDemuxStop
* 描  述  : 关闭解包
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskDemuxStop(UINT32 chan)
{
    INT32 ret = 0;
    INT32 *Handle = NULL;
    INT32 encType = 0;

    ret = recode_tskDemuxCtrl(chan, SAL_FALSE);
    if (SAL_SOK != ret)
    {
        RECODE_LOGE("recode_tskDemuxCtrl(%u) error !!!\n", chan);
        return SAL_FAIL;
    }

    g_stRecodeObjCommon.recodeStObj[chan].recodeTime.timeDist = 0;
    g_stRecodeObjCommon.recodeStObj[chan].recodeTime.bFirstVideoPts = SAL_TRUE;
    g_stRecodeObjCommon.recodeStObj[chan].recodeTime.lastVRtpTime = 0;
    g_stRecodeObjCommon.recodeStObj[chan].recodeTime.compTime = 0;
    g_stRecodeObjCommon.recodeStObj[chan].recodeTime.lastVTime = 0;


    g_stRecodeObjCommon.recodeStObj[chan].recodeAudTime.timeDist = 0;
    g_stRecodeObjCommon.recodeStObj[chan].recodeAudTime.bFirstVideoPts = SAL_TRUE;
    g_stRecodeObjCommon.recodeStObj[chan].recodeAudTime.lastVRtpTime = 0;
    g_stRecodeObjCommon.recodeStObj[chan].recodeAudTime.compTime = 0;
    g_stRecodeObjCommon.recodeStObj[chan].recodeAudTime.lastVTime = 0;

    Handle = &g_stRecodeObjCommon.recodeStObj[chan].recodeModCtrl.chnHandle;
    ret = DemuxControl(DEM_RESET, Handle, (void *)&encType);
    if (ret < 0)
    {
        RECODE_LOGW("chan %d\n", chan);
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskDemuxSet
* 描  述  : 设置解包参数
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskDemuxSet(UINT32 chan, RECODE_PRM *pRecodePrm)
{
    /* INT32           ret       = 0; */
    INT32 i = 0, val = 0;
    RECODE_OBJ_PRM *pstObjPrm = NULL;

    if (chan >= RECODE_MAX_CHN)
    {
        RECODE_LOGE("recode Demux chan(%u) error \n", chan);
        return SAL_FAIL;
    }

    if (NULL == pRecodePrm)
    {
        RECODE_LOGE("chan(%u) error null\n", chan);
        return SAL_FAIL;
    }

    if (MPEG2MUX_STREAM_TYPE_RTP != pRecodePrm->srcPackType)
    {
        RECODE_LOGE("chan(%u) srcPackType %u error only support rtp\n", chan, pRecodePrm->srcPackType);
        return SAL_FAIL;
    }

    if ((0x0 == (pRecodePrm->dstPackType & (1 << MPEG2MUX_STREAM_TYPE_PS)))
        && (0x0 == (pRecodePrm->dstPackType & (1 << MPEG2MUX_STREAM_TYPE_RTP))))
    {
        RECODE_LOGE("chan(%u) detPackType 0x%x error only support rtp/ps\n", chan, pRecodePrm->dstPackType);
        return SAL_FAIL;
    }

    pstObjPrm = &g_stRecodeObjCommon.recodeStObj[chan];

    if ((0x0 != (pRecodePrm->dstPackType & (1 << MPEG2MUX_STREAM_TYPE_PS))) && (0x0 == (pRecodePrm->dstPackType & (1 << MPEG2MUX_STREAM_TYPE_RTP))))
    {
        pstObjPrm->recodeChn.dstPackType = (0xf & (0x1 << DEM_PS));
    }
    else if ((0x0 != (pRecodePrm->dstPackType & (1 << MPEG2MUX_STREAM_TYPE_RTP))) && (0x0 == (pRecodePrm->dstPackType & (1 << MPEG2MUX_STREAM_TYPE_PS))))
    {
        pstObjPrm->recodeChn.dstPackType = (0xf & (0x1 << DEM_RTP));
    }
    else
    {
        pstObjPrm->recodeChn.dstPackType = ((0xf & (0x1 << DEM_RTP)) | (0xf & (0x1 << DEM_PS)));
    }

    /* 音频转包参数设置 目前应用只通过HOST_CMD_RECODE_SET设置音频转包的参数 */
    pstObjPrm->recodeChn.audPackPs = pRecodePrm->audioPackPs;
    pstObjPrm->recodeChn.audPackRtp = pRecodePrm->audioPackRtp;
    pstObjPrm->recodeChn.audioChannels = pRecodePrm->audioChannels;
    pstObjPrm->recodeChn.audioSamplesPerSec = pRecodePrm->audioSamplesPerSec;
    pstObjPrm->recodeChn.audioBitsPerSample = pRecodePrm->audioBitsPerSample;
    if (pRecodePrm->audioSamplesPerSec > 0)
    {
        for (i = 0; i < 16; i++)
        {
            if (pRecodePrm->audioSamplesPerSec == g_PackAacAdtsHeaderSampleFre[i])
            {
                val = g_PackAacAdtsHeader[chan][2];
                if (i != ((val & 0x3c) >> 2))
                {
                    RECODE_LOGI("i %d,val %x_%d\n", i, ((val & 0x3c) >> 2), ((val & 0x3c) >> 2));
                    g_PackAacAdtsHeader[chan][2] &= 0xc3; /* ~((1 << 2) | (1 << 3) | (1 << 4) | (1 << 5));//置为0 */
                    g_PackAacAdtsHeader[chan][2] |= (i << 2);
                }

                val = g_PackAacAdtsHeader[chan][2];
                RECODE_LOGI("val %x_%d\n", ((val & 0x3c) >> 2), val);
                break;
            }
        }
    }

    RECODE_LOGI("chan %d,dstPackType 0x%x audioDstPackType %u_%u audioSamples: cmd_%u now_%u\n", chan, pstObjPrm->recodeChn.dstPackType,
                pRecodePrm->audioPackPs, pRecodePrm->audioPackRtp, pRecodePrm->audioSamplesPerSec, pstObjPrm->recodeChn.audioSamplesPerSec);

    return SAL_SOK;
}

/**
 * @function:   Recode_FrmDelayInit
 * @brief:      帧延时参数初始化
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     INT32
 */
INT32 recode_tskFrmDelayInit(UINT32 chan)
{
    UINT32 i = 0;
    UINT32 ret = 0;
    UINT64 u64PhyAddr = 0;

#if 0
    if (chan > 1)
    {
        SAL_ERROR("invalid chan %d \n", chan);
        return SAL_FAIL;
    }
#endif

    /* 申请缓存需要使用的内存 */
    stDebugProcPrm[chan].uiMemLen = DEBUG_MEM_LEN;           /* 初始化中间内存大小为4M，同转码共享缓存大小一致 */
    stDebugProcPrm[chan].uiExtraMemLen = DEBUG_EXTRA_MEM;    /* 初始化中间额外512KB，具体原因未知 */

    {
        if (stDebugProcPrm[chan].mem)
        {
            goto exit;
        }

        ret = mem_hal_mmzAlloc(DEBUG_MEM_LEN + DEBUG_EXTRA_MEM, "recode", "recode_frmdelay", NULL, SAL_FALSE, &u64PhyAddr, (VOID **)&stDebugProcPrm[chan].mem);
        if (ret != SAL_SOK)
        {
            VENC_LOGE("mem_hal_mmzAlloc err %#x\n", ret);
            return SAL_FAIL;
        }

        /* stDebugProcPrm[chan].mem = SAL_memAlloc(DEBUG_MEM_LEN + DEBUG_EXTRA_MEM, NULL, NULL); */
        if (!stDebugProcPrm[chan].mem)
        {
            SAL_ERROR("malloc failed! chan %d \n", chan);
            return SAL_FAIL;
        }

        SAL_WARN("recode debug: mem[%p], uiMemLen[%d], uiExtraMemLen[%d], chan[%d]\n",
                 stDebugProcPrm[chan].mem, stDebugProcPrm[chan].uiMemLen, stDebugProcPrm[chan].uiExtraMemLen, chan);
    }

    stDebugProcPrm[chan].uiBufRIdx = stDebugProcPrm[chan].uiBufWIdx = 0;
    stDebugProcPrm[chan].bBufFillEnd = SAL_FALSE;     /* 缓存填充结束标志默认FALSE */
    stDebugProcPrm[chan].uiBufCnt = DEBUG_BUF_CNT;    /* 缓存个数，暂定64个 */

    for (i = 0; i < stDebugProcPrm[chan].uiBufCnt; i++)
    {
        if (stDebugProcPrm[chan].stProcBuf[i].p)
        {
            SAL_WARN("i %d is malloc! chan %d \n", i, chan);
            continue;
        }

        ret = mem_hal_mmzAlloc(DEBUG_BUF_SIZE, "recode", "recode_buf", NULL, SAL_FALSE, &u64PhyAddr, (VOID **)&stDebugProcPrm[chan].stProcBuf[i].p);
        /* stDebugProcPrm[chan].stProcBuf[i].p = SAL_memAlloc(DEBUG_BUF_SIZE, NULL, NULL); */
        if (!stDebugProcPrm[chan].stProcBuf[i].p)
        {
            SAL_ERROR("mem alloc failed! i %d, chan %d \n", i, chan);
            return SAL_FAIL;
        }
    }

    SAL_INFO("recode: init frame delay end! chan %d \n", chan);

exit:
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Recode_MakeAlgPackInit
* 描  述  :
* 输  入  : - chn:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 recode_tskMakeAlgPackInit(UINT32 chn)
{
    UINT32 ret = 0;
    UINT64 u64PhyAddr = 0;

    memset(&proc_param[chn], 0x00, sizeof(IVS_SYS_PROC_PARAM));

    proc_param[chn].buf_size = BUFFER_SIZE;
    ret = mem_hal_mmzAlloc(BUFFER_SIZE, "recode", "recode_pack", NULL, SAL_FALSE, &u64PhyAddr, (VOID **)&proc_param[chn].buf);
    if (ret != SAL_SOK)
    {
        VENC_LOGE("mem_hal_mmzAlloc err %#x\n", ret);
        return SAL_FAIL;
    }

    /* proc_param[chn].buf = (unsigned char *)SAL_memAlloc(BUFFER_SIZE, "recode_pack", NULL); */
    if (proc_param[chn].buf == NULL)
    {
        RECODE_LOGE("error\n");
        return SAL_FAIL;
    }

    proc_param[chn].scale_width = 1920;     /* dbg: 临时使用1080P */
    proc_param[chn].scale_height = 1080;

    memset(&target_list[chn], 0x00, sizeof(HIK_MULT_VCA_TARGET_LIST));

    target_list[chn].type = SHOW_TYPE_LINEBG_ONE_HALF | MATCH_TYPE_ACCURACY;
    target_list[chn].target_num = TEST_TARGET_NUM;
    target_list[chn].p_target = (HIK_MULT_VCA_TARGET *)SAL_memMalloc(TEST_TARGET_NUM * sizeof(HIK_MULT_VCA_TARGET), "recode_tsk", "vca_target");
    memset(target_list[chn].p_target, 0x00, TEST_TARGET_NUM * sizeof(HIK_MULT_VCA_TARGET));

    if (target_list[chn].p_target == NULL)
    {
        mem_hal_mmzFree(proc_param[chn].buf, "recode", "recode_pack");
        proc_param[chn].buf = NULL;

        RECODE_LOGE("error\n");
        return SAL_FAIL;
    }

    SAL_INFO("recode pack init end! chan %d \n", chn);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskDemuxCreate
* 描  述  : 创建解包功能模块
* 输  入  : 无
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskDemuxCreate(UINT32 chan)
{
    INT32 ret = SAL_FAIL;
    RECODE_OBJ_PRM *pstObjPrm = NULL;
    RECODE_MOD_CTRL *pRecodeCtrl = NULL;

    if (chan >= RECODE_MAX_CHN)
    {
        RECODE_LOGE("recode Demux chan(%u) error \n", chan);
        return SAL_FAIL;
    }

    pstObjPrm = &g_stRecodeObjCommon.recodeStObj[chan];
    pRecodeCtrl = &pstObjPrm->recodeModCtrl;

    /* 如有需要将ps转包的则打开 */
#if 0
    ret = recode_tskDemuxInit(DEM_PS, pstObjPrm->recodeChn.ipcBufAddr, \
                              pstObjPrm->recodeChn.ipcBufLen, &pstObjPrm->recodeChn.rtpHandle);
    if (SAL_SOK != ret)
    {
        RECODE_LOGE("Demux_Init error.\n");
        return SAL_FAIL;
    }

#endif
    ret = recode_tskDemuxInit(DEM_RTP, pstObjPrm->recodeChn.ipcBufAddr, \
                              pstObjPrm->recodeChn.ipcBufLen, &pstObjPrm->recodeModCtrl.rtpHandle);
    if (SAL_SOK != ret)
    {
        RECODE_LOGE("Demux_Init error.\n");
        return SAL_FAIL;
    }

    SAL_mutexCreate(SAL_MUTEX_NORMAL, &pRecodeCtrl->mutexHdl);
    pRecodeCtrl->enable = SAL_FALSE;

    /* 初始化转包pos智能帧相关内存 */
    if (recode_tskJudgeNeedDelay() && (DEBUG_TRI_RECODE_CHAN == chan || DEBUG_FACE_RECODE_CHAN == chan))
    {
        ret = recode_tskMakeAlgPackInit(chan);
        if (SAL_SOK != ret)
        {
            SAL_ERROR("Recode_MakeAlgPackInit failed! chan %d \n", chan);
            return SAL_FAIL;
        }
    }

    /*创建解包线程*/
    SAL_thrCreate(&pstObjPrm->stThrHandl, recode_tskDemuxThread, SAL_THR_PRI_DEFAULT, 0, pstObjPrm);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskModuleCreate
* 描  述  : 转封装的初始化,根据ipcChanCnt参数进行初始化
* 输  入  : - pCreate:
*         : - pHandle:
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskModuleCreate(void)
{
    DSPINITPARA *pstDspInfo = NULL;
    INT32 i = 0;

    pstDspInfo = SystemPrm_getDspInitPara();
    if (pstDspInfo == NULL)
    {
        RECODE_LOGE("error\n");
        return SAL_FAIL;
    }

    if (pstDspInfo->ipcChanCnt > RECODE_MAX_CHN)
    {
        RECODE_LOGE("Chan %d (Illegal parameters)\n", pstDspInfo->ipcChanCnt);
        return (SAL_FAIL);
    }

    for (i = 0; i < pstDspInfo->ipcChanCnt; i++)
    {
        /* 人包关联:初始化帧延时相关全局参数，当前仅有三目1和人脸0需要处理帧延时 */
        if (recode_tskJudgeNeedDelay() && (DEBUG_RECODE_CHAN_MAX > i) )
        {
            if (SAL_SOK != recode_tskFrmDelayInit(i))
            {
                SAL_ERROR("Recode_FrmDelayInit failed! chan %d \n", i);
                return SAL_FAIL;
            }
        }

        g_stRecodeObjCommon.recodeStObj[i].uiChn = i;
        /*应用送IPC码流内存，如应用未申请由dsp的HostCmd_initDspPrm函数统一申请*/
        g_stRecodeObjCommon.recodeStObj[i].recodeChn.ipcBufAddr = ((recode_tskJudgeNeedDelay() && (DEBUG_RECODE_CHAN_MAX > i) )? (UINT8 *)stDebugProcPrm[i].mem : pstDspInfo->ipcShareBuf[i].pVirtAddr);
        g_stRecodeObjCommon.recodeStObj[i].recodeChn.ipcBufLen = ((recode_tskJudgeNeedDelay() && (DEBUG_RECODE_CHAN_MAX > i) ) ? stDebugProcPrm[i].uiMemLen : pstDspInfo->ipcShareBuf[i].bufLen);

        RECODE_LOGI("recode debug: i %d, now[%p, %d], before[%p, %d] \n", i,
                 g_stRecodeObjCommon.recodeStObj[i].recodeChn.ipcBufAddr,
                 g_stRecodeObjCommon.recodeStObj[i].recodeChn.ipcBufLen,
                 pstDspInfo->ipcShareBuf[i].pVirtAddr,
                 pstDspInfo->ipcShareBuf[i].bufLen);

        g_stRecodeObjCommon.recodeStObj[i].recodeChn.maxWidth = 1280;
        g_stRecodeObjCommon.recodeStObj[i].recodeChn.maxHeight = 720;
        g_stRecodeObjCommon.recodeStObj[i].recodeChn.encType = 96;    /* H264 */
        g_stRecodeObjCommon.recodeStObj[i].recodeChn.packType = DEM_RTP;
        g_stRecodeObjCommon.recodeStObj[i].recodeChn.dstPackType = (1 << DEM_PS) | (1 << DEM_RTP);

        g_stRecodeObjCommon.recodeStObj[i].recodeTime.timeDist = 0;
        g_stRecodeObjCommon.recodeStObj[i].recodeTime.bFirstVideoPts = SAL_TRUE;
        g_stRecodeObjCommon.recodeStObj[i].recodeTime.lastVRtpTime = 0;
        g_stRecodeObjCommon.recodeStObj[i].recodeTime.compTime = 0;
        g_stRecodeObjCommon.recodeStObj[i].recodeTime.lastVTime = 0;


        g_stRecodeObjCommon.recodeStObj[i].recodeAudTime.timeDist = 0;
        g_stRecodeObjCommon.recodeStObj[i].recodeAudTime.bFirstVideoPts = SAL_TRUE;
        g_stRecodeObjCommon.recodeStObj[i].recodeAudTime.lastVRtpTime = 0;
        g_stRecodeObjCommon.recodeStObj[i].recodeAudTime.compTime = 0;
        g_stRecodeObjCommon.recodeStObj[i].recodeAudTime.lastVTime = 0;
        if (SAL_SOK != recode_tskPackCreate(i))
        {
            RECODE_LOGE("recode Init error.\n");
            return SAL_FAIL;
        }

        /*3. 解包层初始化*/
        if (SAL_SOK != recode_tskDemuxCreate(i))
        {
            RECODE_LOGE("recode Init error.\n");
            return SAL_FAIL;
        }

        g_stRecodeObjCommon.uiObjCnt++;
    }

    RECODE_LOGI("Recode Module Create End! \n");
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskStart
* 描  述  :
* 输  入  : - chan  :
*         : - pParam:
*         : - pBuf  :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskStart(UINT32 chan, UINT32 *pParam, UINT32 *pBuf)
{
    INT32 ret = SAL_SOK;
    RECODE_PRM *pRecodePrm = NULL;

    RECODE_LOGI("recode_tskStart chan:%d\n", chan);

    pRecodePrm = (RECODE_PRM *)pBuf;

    ret = recode_tskDemuxStart(chan, pRecodePrm);
    if (SAL_SOK != ret)
    {
        RECODE_LOGE("chan:%d error\n", chan);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskStop
* 描  述  :
* 输  入  : - chan  :
*         : - pParam:
*         : - pBuf  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 recode_tskStop(UINT32 chan, UINT32 *pParam, UINT32 *pBuf)
{
    INT32 ret = SAL_SOK;

    RECODE_LOGI("recode_tskStop chan:%d\n", chan);

    ret = recode_tskDemuxStop(chan);
    if (SAL_SOK != ret)
    {
        RECODE_LOGE("chan:%d error\n", chan);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskSetPrm
* 描  述  :
* 输  入  : - chan  :
*         : - pParam:
*         : - pBuf  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 recode_tskSetPrm(UINT32 chan, UINT32 *pParam, UINT32 *pBuf)
{
    INT32 ret = SAL_SOK;
    RECODE_PRM *pRecodePrm = NULL;

    RECODE_LOGI("recode_tskSetPrm chan:%d\n", chan);

    pRecodePrm = (RECODE_PRM *)pBuf;
    ret = recode_tskDemuxSet(chan, pRecodePrm);
    if (SAL_SOK != ret)
    {
        RECODE_LOGE("chan:%d error\n", chan);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : recode_tskSetAudPack
* 描  述  : 设置音频转包开始、关闭
* 输  入  : - chan  :
*         : - pParam:
*         : - pBuf  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 recode_tskSetAudPack(UINT32 chan, UINT32 *pParam, UINT32 *pBuf)
{
    INT32 ret = SAL_SOK;
    RECODE_PRM *pRecodePrm = NULL;

    RECODE_LOGI("recode_tskSetAudPack chan:%d\n", chan);

    pRecodePrm = (RECODE_PRM *)pBuf;
    ret = recode_tskDemuxAudStart(chan, pRecodePrm);
    if (SAL_SOK != ret)
    {
        RECODE_LOGE("chan:%d error\n", chan);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

