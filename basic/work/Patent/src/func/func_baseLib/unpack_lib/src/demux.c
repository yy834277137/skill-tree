#include "type_dscrpt_common.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "RtpUnpackDefine.h"
#include "PSDemuxLib.h"
#include <pthread.h>
#include "libdemux.h"
#include "get_es_info.h"
#include "DSP_hevc_sps_parse.h"
#include "sal.h"
#include "dspcommon.h"

#define MAX_RTPUNPACK_SIZE          (1024 * 10)
#define MIN_DATA_LEN                256 
#define MIN_FIRST_RTP_PACKET_LEN    5
#define MAX_SAMPLE_FREQ_INDEX       0x0d 
#define VIDEO_INFO_NO_EXIT          0x80000001
#define MAX_DEMUX_NUM               (MAX_VDEC_CHAN + MAX_VIPC_CHAN) //16路解码+9路解包 

#define NAL_FLG_SPS                 0x1
#define NAL_FLG_PPS                 0x2
#define NAL_FLG_VIDEO               0x4
#define NAL_FLG_VOL                 0x8
#define NAL_FLG_VPS                 0x10
#define NAL_FLG_SEI                 0x20
#define NAL_PARAM_INFO              (NAL_FLG_SPS|NAL_FLG_PPS)
#define SMART_RESULT_LENGHT         (21*1024)

#define MAX_THIRD_IPC_INFO_LEN			1024

typedef struct
{
    unsigned char* pBuf;
    unsigned int   bufLen;
    unsigned int   mUsed;
    unsigned int   mLeft;
}RTPUNPACK_MEM;

typedef struct
{
    RTPUNPACK_PARAM      topParam;
    RTPUNPACK_PRC_PARAM  prcParam;
    int                  idx;
}RTPUNPACK_INTERNAL;

typedef struct
{
    PSDEMUX_PARAM          staPrm;
    PSDEMUX_PROCESS_PARAM  dynPrm;
}PSUNPACK_INTERNAL;

typedef struct 
{
    unsigned int                 demuxIdx;
    unsigned int                 demuxType;
    unsigned int                 streamSz;
    unsigned char*               pOutBuf;
    unsigned char*               pOrgBuf;
    unsigned char*               pSmartIndexBuf;
    unsigned char*               pPrivtDataBuf;
    unsigned int                 hsBuf;
    unsigned char*               buffer;
    unsigned int                 memUsed;
    unsigned int                 memLen;
    unsigned int                 outBufLen;
    unsigned int                 outWIdx;
    unsigned int                 orgWriteIdx;
    unsigned int                 orgReadIdx;
    unsigned int                 orgDataLen;
    unsigned int                 orgBufLen;
    unsigned int                 minDataLen;
    unsigned int                 bUsed;
    unsigned int                 bCreated;
    unsigned char                bSearchStartCode;
    unsigned int                 videoStreamType;     /*rtp包的视音频负载id及视音频类型*/
    unsigned int                 videoPayloadType;
    unsigned int                 audioStreamType;
    unsigned int                 audioPayloadType;

    unsigned int                 audEncType;
    unsigned int                 sampleRate;
    unsigned int                 bitRate;
    unsigned int                 aFrmLen;
    unsigned int                 privtDataLen;
    
    void*                        rtpDemuxHdl;        /*RTP分离器句柄*/
    RTPUNPACK_INTERNAL           rtpInternal;        /*RTP解析的内部结构体*/

    void*                        psDemuxHdl;
    PSUNPACK_INTERNAL            psInternal;
    
    unsigned char                frmInBuf;      /*告知外部有一帧数据*/
    unsigned int                 frmType;
    
    unsigned int                 timestamp;
    unsigned int                 aOut;
    unsigned int                 vOut;

    pthread_mutex_t              demlock;
    pthread_cond_t               vidcond; 
    pthread_cond_t               audcond; 
} DEMUX_CHN_INFO_ST;

#define H264StartCode(buf)    (((buf[0]) == 0) && ((buf[1]) == 0) && ((buf[2]) == 0) && ((buf[3]) == 1))
#define MEMCHECK(_A_,_B_,_C_) do{                   \
        if(_A_ <=  _B_){                \
          printf("err _A_ %d _B_ %d\n",_A_,_B_);\
          return -1;\
        }\
        if(_A_ <= (_B_ + _C_)){\
          printf("err _A_ %d _B_ %d _C_ %d\n",_A_,_B_,_C_);\
          return -1;\
        }\
    }while(0)
    
#define MEMCALC(_A_,_B_,_C_)  ((_A_ >= _B_) ? (_A_ - _B_) :( _C_ - _B_ + _A_))  

#define DEC_ORG_BUF_2MBPS_LEN (0x100000)
#define DEC_ORG_BUF_4MBPS_LEN (0x200000)
#define DEC_ONLY_AUDIO_LEN    (0xc00)
/* 组类型定义*/
#define I_FRAME_MODE		0x00001001
#define III_FRAME_MODE		0x00001002
#define P_FRAME_MODE		0x00001003
#define BP_FRAME_MODE		0x00001004
#define BBP_FRAME_MODE		0x00001005
#define AUDIO_I_FRAME_MODE	0x00001006
#define AUDIO_P_FRAME_MODE	0x00001007
#define PRIVT_FRAME_MODE	0x00001008

typedef struct 
{
    unsigned int        demuxType;
    DEMUX_CHN_INFO_ST   demChnInfo[DEM_NONE];
}DEMUX_INFO_ST;

#define CHECK_NALU_TYPE(marker)    ((marker & 0x0000007e) >> 1)
#define MIN(_1_, _2_)                   ((_1_) > (_2_) ? (_2_) : (_1_))
#define MAX(_1_, _2_)                   ((_1_) > (_2_) ? (_1_) : (_2_))


static DEMUX_INFO_ST   demChnInfo[MAX_DEMUX_NUM] = {0};
static void RtpDemPrivtFrame(int chan);

static int demDbLevel = DEBUG_LEVEL0;

/***************************************************************
   Function:     RtpDemNalPos
   Description:  查找有效nal
   Input:        chan通道号
   Output:
   Returns:      TRUE or FALSE
 ****************************************************************/
static int RtpDemNalPos(int chan)
{
	unsigned int        demuxType  = demChnInfo[chan].demuxType;
	if (DEM_RTP != demuxType)
	{
	    printf("RtpDemNalPos chan:%d demuxType %d error\n", chan, demuxType);
        return -1;
	}
	
    DEMUX_CHN_INFO_ST   *pDemChn  = &demChnInfo[chan].demChnInfo[demuxType];
    RTPUNPACK_INTERNAL *pInternal = &pDemChn->rtpInternal;
    RTPUNPACK_PRC_PARAM   *param  = &pInternal->prcParam;
    unsigned char* p = param->outbuf;
    unsigned int   r = 0;

    /* 讯美码流含多个aud sei sps pps sei sei sei nal */
    for (r = 0; r + 5 < param->outbuf_len; r++)
    {
        if (p[r] == 0 && p[r + 1] == 0 && p[r + 2] == 0 && p[r + 3] == 1 && ((p[r + 4] & 0x1f) == 7 || (p[r + 4] & 0x1f) == 5 || (p[r + 4] & 0x1f) == 0x1))
        {
            param->outbuf_len -= r;
            param->outbuf += r;
            return 0;
        }
    }

    return -1;
}

/***************************************************************
   Function:     RtpDemCopyData
   Description:  拷贝解包后的数据到备份缓冲区
   Input:        chan:             通道号
                 pSrc:             源数据
                 frameLen:         源数据长度
   Output:
   Returns:      错误码
 ****************************************************************/
static int RtpDemCopyData(int chan, unsigned char* pSrc, unsigned int frameLen)
{
	unsigned int  demuxType  = demChnInfo[chan].demuxType;
	if (DEM_RTP != demuxType)
	{
	    printf("RtpDemCopyData chan:%d demuxType %d error\n", chan, demuxType);
        return -1;
	}

	
    DEMUX_CHN_INFO_ST   *pRtpDem  = &demChnInfo[chan].demChnInfo[demuxType];
    unsigned int       spareLen = 0;

    if(!pRtpDem->vOut)
    {
        printf("chan:%d vOut %d error\n", chan, pRtpDem->vOut);
        return 0;
    }

    if (frameLen >= pRtpDem->outBufLen)
    {
        printf("frameLen error!chan:%d frameLen: %d\n", chan, frameLen);
        return -1;
    }

    // 判断缓冲区是否有足够的空间存放解包出来的数据
    spareLen = pRtpDem->outBufLen - pRtpDem->outWIdx - sizeof(HIK_STREAM_PARSE);
    if (frameLen > spareLen)
    {
        printf("CopyDemuxDataToBuf,DecBuf overflow!chan:%d frameLen=%d spareLen:%d\n", chan, frameLen, spareLen);
        return -1;
    }

    // 拷贝数据到备份缓冲区
    memcpy(pRtpDem->pOutBuf + pRtpDem->outWIdx + sizeof(HIK_STREAM_PARSE), pSrc, frameLen);
	//printf("frameLen error!chan:%d 0x%x,addr %p,frameLen: %d\n", chan, *(pRtpDem->pOutBuf + pRtpDem->outWIdx + sizeof(HIK_STREAM_PARSE) + 4),pRtpDem->pOutBuf + pRtpDem->outWIdx + sizeof(HIK_STREAM_PARSE),frameLen);
    pRtpDem->outWIdx += frameLen;

    return 0;
}

/***************************************************************
   Function:     RtpDemAudioInfo
   Description:  获取音频码流信息
   Input:        chan - 通道号
   Output:       none
   Returns:      void
 ****************************************************************/
static int RtpDemAudioInfo(int chan)
{
    int                    s32Ret  = 0;     
	unsigned int        demuxType  = demChnInfo[chan].demuxType;
	if (DEM_RTP != demuxType)
	{
	    printf("RtpDemAudioInfo chan:%d demuxType %d error\n", chan, demuxType);
        return -1;
	}

    DEMUX_CHN_INFO_ST   *pRtpDem  = &demChnInfo[chan].demChnInfo[demuxType];
    RTPUNPACK_INTERNAL *pInternal = &pRtpDem->rtpInternal;
    RTPUNPACK_PRC_PARAM   *param  = &pInternal->prcParam;

    /*应用给的音频类型可能不对,做简单校验*/
    if ((((pRtpDem->audioStreamType == AUDIO_G722_16)
        || (pRtpDem->audioStreamType == AUDIO_G722_1))
        && (param->outbuf_len != 80))
        || (((pRtpDem->audioStreamType == AUDIO_G711_U)
        ||   (pRtpDem->audioStreamType == AUDIO_G711_A))
        &&   (param->outbuf_len != 320)))
    {
        pRtpDem->audioStreamType = AUDIO_UNKOWN;
    }

    if(AUDIO_UNKOWN != pRtpDem->audioStreamType)
    {
        switch (pRtpDem->audioStreamType)
        {
            case AUDIO_G711_U:
                pRtpDem->audEncType = AUDIO_G711_U;
                pRtpDem->sampleRate = 8000;
                pRtpDem->bitRate    = 25 * 320 * 8;
                break;
            case AUDIO_G711_A:
                pRtpDem->audEncType = AUDIO_G711_A;
                pRtpDem->sampleRate = 8000;
                pRtpDem->bitRate    = 25 * 320 * 8;
                break;
            case AUDIO_G722_1:
            case AUDIO_G722_16:
                pRtpDem->audEncType = AUDIO_G722_1;
                pRtpDem->sampleRate = 16000;
                pRtpDem->bitRate    = 25 * 80 * 8;
                break;
            case AUDIO_G726_16:
                pRtpDem->audEncType = AUDIO_G726_16;
                pRtpDem->sampleRate = 8000;
                pRtpDem->bitRate    = 16000;
                break;
            case AUDIO_G726_A:
                pRtpDem->audEncType = AUDIO_G726_A;
                pRtpDem->sampleRate = 8000;
                pRtpDem->bitRate    = 32000;
                break;
            case AUDIO_AAC:
                pRtpDem->audEncType = AUDIO_AAC;
                pRtpDem->sampleRate = 16000;
                pRtpDem->bitRate    = 32000;
                break;
            default:
                pRtpDem->audEncType = AUDIO_UNKOWN;
                s32Ret = -1;
                break;
        }
        pRtpDem->aFrmLen = pRtpDem->bitRate / 25 /8;
    }
    else
    {
        switch (param->payload_type)
        {
            case 0x0:
                pRtpDem->audEncType = AUDIO_G711_U;
                pRtpDem->sampleRate = 8000;
                pRtpDem->bitRate    = 25 * 320 * 8;
                break;
            case 0x08:
                pRtpDem->audEncType = AUDIO_G711_A;
                pRtpDem->sampleRate = 8000;
                pRtpDem->bitRate    = 25 * 320 * 8;
                break;
            case 0x09:
            case 0x62:
                pRtpDem->audEncType = AUDIO_G722_1;
                pRtpDem->sampleRate = 16000;
                pRtpDem->bitRate    = 25 * 80 * 8;
                break;
            case 0x66:
                pRtpDem->audEncType = AUDIO_G726_16;
                pRtpDem->sampleRate = 8000;
                pRtpDem->bitRate    = 16000;
                break;
            case 0x67:
                pRtpDem->audEncType = AUDIO_G726_A;
                pRtpDem->sampleRate = 8000;
                pRtpDem->bitRate    = 32000;
                break;
            case 0x61:
            case 0x68:
                pRtpDem->audEncType = AUDIO_AAC;
                pRtpDem->sampleRate = 8000;
                pRtpDem->bitRate    = 128000;
                break;
            default:
                pRtpDem->audEncType = AUDIO_UNKOWN;
                s32Ret = -1;
                break;
        }
    }

    return s32Ret;
}

/*******************************************************************************
   Function:   RtpDemH264SPS
   Description:解析H264的SPS
   Input:      chan 通道号
   Output:
   Return:
    0:         Successful
    ohters:    Failed
*******************************************************************************/
static int RtpDemH264SPS(int chan)
{
    int                      len;
    int                      s32Ret   = 0;
	unsigned int           demuxType  = demChnInfo[chan].demuxType;
	if (DEM_RTP != demuxType)
	{
	    printf("RtpDemH264SPS chan:%d demuxType %d error\n", chan, demuxType);
        return -1;
	}

    DEMUX_CHN_INFO_ST   *pDemPrm  = &demChnInfo[chan].demChnInfo[demuxType];
    RTPUNPACK_INTERNAL     *pInternal = &pDemPrm->rtpInternal;
    RTPUNPACK_PRC_PARAM      *param   = &pInternal->prcParam;
    VIDEO_ES_INFO             esInfo; 

    param->lastVRtpTime = param->time_stamp;
    param->flgNal = 0;
    param->is_new_frame = 0;

    pDemPrm->frmType = FRAME_TYPE_VIDEO_IFRAME;
    pDemPrm->outWIdx = 0;/*加清零，防止有以前的数据*/
    seekVideoInfoAvc(param->outbuf,param->outbuf_len,&esInfo);
    param->width  = esInfo.width;
    param->height = esInfo.height;
    param->spsLen = param->outbuf_len;

    /*有些是一帧封装为一个RTP包*/
    for (len = 0; len + 4 < param->outbuf_len; len++)
    {
        if (param->outbuf[len] == 0 && param->outbuf[len + 1] == 0 && param->outbuf[len + 2] == 0 && param->outbuf[len + 3] == 1)
        {
            if ((param->outbuf[len + 4] & 0x1f) == 0x7)
            {
                param->flgNal |= NAL_FLG_SPS;
            }
            else if ((param->outbuf[len + 4] & 0x1f) == 0x8)
            {
                param->flgNal |= NAL_FLG_PPS;
            }
            else if (((param->outbuf[len + 4] & 0x1f) == 0x5) || ((param->outbuf[len + 4] & 0x1f) == 0x1))
            {
                param->flgNal |= NAL_FLG_VIDEO;
                break;
            }
        }
    }

    if (param->flgNal & NAL_FLG_VIDEO)
    {
        printf("debug info----chan=%u stream sps have vedio flgNal=0x%x\n", chan, param->flgNal);
        if (param->marker_bit)
        {
            param->is_new_frame = 1;
            param->flgNal = 0;
        }
    }
    else if (param->marker_bit == 1)
    {
        param->marker_bit = 0;
    }
    
    return s32Ret;
}

/*******************************************************************************
   Function:    RtpDemH264IFrame
   Description: H264 I帧信息解析
   Input:       chan 通道号
   Output:
   Return:
    0:          Successful
    ohters:     Failed
*******************************************************************************/
static int RtpDemH264IFrame(int chan)
{
	unsigned int  demuxType  = demChnInfo[chan].demuxType;
	if (DEM_RTP != demuxType)
	{
	    printf("RtpDemH264IFrame chan:%d demuxType %d error\n", chan, demuxType);
        return -1;
	}

    DEMUX_CHN_INFO_ST   *pRtpDem  = &demChnInfo[chan].demChnInfo[demuxType];
    RTPUNPACK_INTERNAL *pInternal = &pRtpDem->rtpInternal;
    RTPUNPACK_PRC_PARAM  *param = &pInternal->prcParam;
    int                    ret  = 0;
    
    if (param->is_new_frame)
    {
        param->flgNal = 0;
        param->is_new_frame = 0;
        return ret;
    }
    
    if (param->marker_bit)
    {
        param->is_new_frame = 1;
        param->flgNal = 0;
    }

    return ret;
}


/*******************************************************************************
   Function:   RtpDemH264SPS
   Description:解析H264的SPS
   Input:      chan 通道号
   Output:
   Return:
    0:         Successful
    ohters:    Failed
*******************************************************************************/
static int RtpDemH265SPS(int chan)
{
    int                      len;
    int                      s32Ret   = 0;
	unsigned int           demuxType  = demChnInfo[chan].demuxType;
	if (DEM_RTP != demuxType)
	{
	    printf("RtpDemH265SPS chan:%d demuxType %d error\n", chan, demuxType);
        return -1;
	}

    DEMUX_CHN_INFO_ST      *pDemPrm   = &demChnInfo[chan].demChnInfo[demuxType];
    RTPUNPACK_INTERNAL     *pInternal = &pDemPrm->rtpInternal;
    RTPUNPACK_PRC_PARAM    *param     = &pInternal->prcParam;
    VIDEO_INFO               esInfo; 
	unsigned int             naluType = 0;
	unsigned int             marker   = 0;
	VIDEO_HEVC_INFO          hevc_info;

    param->lastVRtpTime = param->time_stamp;
    param->flgNal = 0;
    param->is_new_frame = 0;
    pDemPrm->frmType = FRAME_TYPE_VIDEO_IFRAME;
    esInfo.codec_specific.hevc_info = &hevc_info;
    if (OPENHEVC_GetPicSizeFromSPS_dsp(param->outbuf + 4,param->outbuf_len, &esInfo) != 0)
	{
		printf("HEVCDEC_InterpretSPS fail ! restLen %d\n", param->outbuf_len);
		return -1;
	}
    param->width  = esInfo.img_width;
    param->height = esInfo.img_height;
    param->spsLen = param->outbuf_len;
    //printf("%s|%d w:%d,h:%d,spsLen %d\n",__func__,__LINE__,param->width,param->height,param->spsLen);	
    /*有些是一帧封装为一个RTP包*/
    for (len = 0; len + 4 < param->outbuf_len; len++)
    {
        if (param->outbuf[len] == 0 && param->outbuf[len + 1] == 0 && param->outbuf[len + 2] == 0 && param->outbuf[len + 3] == 1)
        {
            marker = param->outbuf[len + 4] | (param->outbuf[len + 5] << 8);
			naluType = CHECK_NALU_TYPE(marker);	
            if (naluType == NAL_UNIT_VPS)
            {
                param->flgNal |= NAL_FLG_VPS;
            }
			else if (naluType == NAL_UNIT_SPS)
            {
                param->flgNal |= NAL_FLG_SPS;
            }
            else if (naluType == NAL_UNIT_PPS)
            {
                param->flgNal |= NAL_FLG_PPS;
            }
            else if (NAL_UNIT_PREFIX_SEI == naluType)
            {
                param->flgNal |= NAL_FLG_SEI;
            }
			else if (NAL_UNIT_CODED_SLICE_IDR_W_RADL == naluType
			|| NAL_UNIT_CODED_SLICE_IDR_N_LP == naluType)
            {
                param->flgNal |= NAL_FLG_VIDEO;
                break;
            }
        }
    }

    if (param->flgNal & NAL_FLG_VIDEO)
    {
        printf("debug info----chan=%u stream sps have vedio flgNal=0x%x\n", chan, param->flgNal);
        if (param->marker_bit)
        {
            param->is_new_frame = 1;
            param->flgNal = 0;
        }
    }
    else if (param->marker_bit == 1)
    {
        param->marker_bit = 0;
    }
    
    return s32Ret;
}

/*******************************************************************************
   Function:    RtpDemH264IFrame
   Description: H264 I帧信息解析
   Input:       chan 通道号
   Output:
   Return:
    0:          Successful
    ohters:     Failed
*******************************************************************************/
static int RtpDemH265IFrame(int chan)
{
	unsigned int  demuxType  = demChnInfo[chan].demuxType;
	if (DEM_RTP != demuxType)
	{
	    printf("RtpDemH265IFrame chan:%d demuxType %d error\n", chan, demuxType);
        return -1;
	}

    DEMUX_CHN_INFO_ST   *pRtpDem  = &demChnInfo[chan].demChnInfo[demuxType];
    RTPUNPACK_INTERNAL *pInternal = &pRtpDem->rtpInternal;
    RTPUNPACK_PRC_PARAM  *param = &pInternal->prcParam;
    int                    ret  = 0;

	pRtpDem->frmType = FRAME_TYPE_VIDEO_IFRAME;
    if (param->is_new_frame)
    {
        param->flgNal = 0;
        param->is_new_frame = 0;
        return ret;
    }
    
    if (param->marker_bit)
    {
        param->is_new_frame = 1;
        param->flgNal = 0;
    }

    return ret;
}


/***************************************************************
   Function:     RtpDemAudioFrame
   Description:  音频帧处理函数
   Input:        chan - 通道号
   Output:       none
   Returns:      void
 ****************************************************************/
static void RtpDemAudioFrame(int chan)
{
    int              s32Ret  = 0;
	unsigned int  demuxType  = demChnInfo[chan].demuxType;
	if (DEM_RTP != demuxType)
	{
	    printf("RtpDemAudioFrame chan:%d demuxType %d error\n", chan, demuxType);
        return;
	}

    DEMUX_CHN_INFO_ST   *pRtpDem  = &demChnInfo[chan].demChnInfo[demuxType];
    RTPUNPACK_INTERNAL *pInternal = &pRtpDem->rtpInternal;
    RTPUNPACK_PRC_PARAM   *param  = &pInternal->prcParam;
    //unsigned int        audEncType, sampleRate, bitRate;
    unsigned int       strParseSize = sizeof(HIK_STREAM_PARSE);
    HIK_STREAM_PARSE  *pStreamParse = NULL;

    //audEncType = pRtpDem->audEncType;
    //sampleRate = pRtpDem->sampleRate;
    //bitRate    = pRtpDem->bitRate;

    // 获取音频流信息
    s32Ret = RtpDemAudioInfo(chan);
    if(0 != s32Ret)
    {
        return;
    }
    
    if ((pRtpDem->audEncType != AUDIO_G711_U)
     && (pRtpDem->audEncType != AUDIO_G711_A)
     && (pRtpDem->audEncType != AUDIO_G722_1)
     && (pRtpDem->audEncType != AUDIO_G726_16)
     && (pRtpDem->audEncType != AUDIO_AAC)
     && (pRtpDem->audEncType != AUDIO_G726_A))
    {
        return;
    }
    
    /****************解析后的音频码流输出********************/
    pRtpDem->frmType  = FRAME_TYPE_AUDIO_FRAME;
    pRtpDem->outWIdx  = param->outbuf_len;
    pRtpDem->frmInBuf = 1;

    pStreamParse = (HIK_STREAM_PARSE  *)pRtpDem->pOutBuf;
    if((!pStreamParse->bufUsed) && pRtpDem->aOut)
    {
        pStreamParse->bufUsed   = 1;
        pStreamParse->frameLen  = param->outbuf_len;
        pStreamParse->stampMs   = param->time_stamp / 8;
        pStreamParse->frameType = pRtpDem->frmType;
		pStreamParse->audEncType = pRtpDem->audEncType;
        memcpy(pRtpDem->pOutBuf + strParseSize,param->outbuf,param->outbuf_len);
    }
    pRtpDem->timestamp = (param->time_stamp / 8);
    return;
}

/**************************************************************
   Function:     RtpDemVideoFrame
   Description:  视频帧处理函数
   Input:        chan - 通道号
   Output:       none
   Returns:      void
 ****************************************************************/
static void RtpDemVideoFrame(int chan)
{
	unsigned int  demuxType  = demChnInfo[chan].demuxType;
	if (DEM_RTP != demuxType)
	{
	    printf("RtpDemVideoFrame chan:%d demuxType %d error\n", chan, demuxType);
        return;
	}

    DEMUX_CHN_INFO_ST   *pDemPrm  = &demChnInfo[chan].demChnInfo[demuxType];
    RTPUNPACK_INTERNAL   *pInternal = &pDemPrm->rtpInternal;
    RTPUNPACK_PRC_PARAM  *param     = &pInternal->prcParam;
    unsigned int         ret;
    //unsigned int       strParseSize = sizeof(HIK_STREAM_PARSE);
    HIK_STREAM_PARSE  *pStreamParse = NULL;
	unsigned int naluType = 0;
	unsigned int marker   = 0;

	    
    /*如果有RTP丢失的情况发生，直接丢掉这一帧 */
    if (param->status == HIK_RTP_UNPACK_LIB_E_SN_ERR)
    {
        printf("chan %d %s HIK_RTP_UNPACK_LIB_E_SN_ERR\n", chan,__FUNCTION__);
        return;
    }

    /* 时标变化开启新的一帧 */
    if (param->lastVRtpTime != param->time_stamp)
    {
        param->is_new_frame = 1;
    }
    
    if (param->stream_type == STREAM_TYPE_VIDEO_H264)
    {
        if (param->is_new_frame)
        {
            if((param->outbuf_len <= MIN_FIRST_RTP_PACKET_LEN) || !H264StartCode(param->outbuf))
            {
                return;
            }
        }

        /*删除无效nal*/
        if (param->outbuf_len > MIN_FIRST_RTP_PACKET_LEN)
        {
            if (H264StartCode(param->outbuf) &&((param->outbuf[4] == 0x6) 
               || (param->outbuf[4] == 0x9) || (param->outbuf[4] == 0x0)))
            {
               ret = RtpDemNalPos(chan);
               if(ret)
               {
                   return;
               }
            }
        }

        if ((param->outbuf_len > MIN_FIRST_RTP_PACKET_LEN) && H264StartCode(param->outbuf))
        {
            switch (param->outbuf[4] & 0x1f)
            {
                // sps
                case 0x7:
                    ret = RtpDemH264SPS(chan);
                    if(ret)
                    {
                        return;
                    }                    
                    break;
                // pps
                case 0x8:
                    if ((param->flgNal & NAL_FLG_SPS) != NAL_FLG_SPS)
                    {
                        return;
                    }
                    param->flgNal |= NAL_FLG_PPS;
                    if (param->marker_bit)
                    {
                        param->marker_bit = 0;
                    }
                    param->ppsLen = param->outbuf_len;
                    break;
                // I帧
                case 0x5:
                    ret = RtpDemH264IFrame(chan);
                    if(ret)
                    {
                        return;
                    }
                    break;
                case 0x1:
                    if (param->is_new_frame)
                    {
                        param->lastVRtpTime = param->time_stamp;
                        param->flgNal = 0;
                        param->is_new_frame = 0;
                    }

                    if (param->marker_bit)
                    {
                        param->is_new_frame = 1;
                        param->flgNal = 0;
                    }

                    pDemPrm->frmType = FRAME_TYPE_VIDEO_PFRAME;
                    break;
                case 0x6:
                    if (param->is_new_frame)
                    {
                        printf("[chan:%d]err!!!start new frame sei not del\n", chan);
                        return;
                    }

                    if (param->marker_bit)
                    {
                        param->is_new_frame = 1;
                        param->flgNal = 0;
                        printf("[chan:%d]H264 frame finish with sei!\n", chan);
                    }

                    break;
                default:

					#if 0
                    printf("H264 nal header err!data %x %x %x %x %x\n",
                           param->outbuf[0],
                           param->outbuf[1],
                           param->outbuf[2],
                           param->outbuf[3],
                           param->outbuf[4]);
					#endif
                    return;
            }
        }
        else if (param->marker_bit)
        {
            param->is_new_frame = 1;
            param->flgNal = 0;
        }
    }
    else if (param->stream_type == STREAM_TYPE_VIDEO_MPEG4)
    {
        printf("%s|%d:MPWG4 NOT SUPPORT\n",__func__, __LINE__);
        return;
    }
    else if (param->stream_type == STREAM_TYPE_VIDEO_MJPG)
    {
        printf("%s|%d:MJPEG NOT SUPPORT\n",__func__, __LINE__);
        return;
    }
	else if (param->stream_type == STREAM_TYPE_VIDEO_H265)
	{
	    //printf("%s|%d: Rtp demux\n",__func__, __LINE__);
	    //RtpVideoFrameMakeHEVC(chan);
		//ParseNaluH265(chan);
		//printf("%s|%d: Rtp demux\n",__func__, __LINE__);
		if (param->is_new_frame)
		{
			if ((param->outbuf_len <= MIN_FIRST_RTP_PACKET_LEN)
			    || !(param->outbuf[0] == 0 && param->outbuf[1] == 0 && param->outbuf[2] == 0 && param->outbuf[3] == 1)
			    )
			{

				return;
			}
		}
		
		if ((param->outbuf_len > MIN_FIRST_RTP_PACKET_LEN)
		    && (param->outbuf[0] == 0 && param->outbuf[1] == 0 && param->outbuf[2] == 0 && param->outbuf[3] == 1))
		{
		    marker = param->outbuf[4] | (param->outbuf[5] << 8);
			naluType = CHECK_NALU_TYPE(marker);
			//pDemPrm->frmType = FRAME_TYPE_VIDEO_PFRAME;
			#if 0
			printf("%s|%d naluType is %d,[%x,%x]\n",__func__,__LINE__,naluType,param->outbuf[4],param->flgNal);	
			printf("H265 nal header data %x %x %x %x %x\n",
                           param->outbuf[4],
                           param->outbuf[5],
                           param->outbuf[6],
                           param->outbuf[7],
                           param->outbuf[8]);
			#endif
		    switch (naluType)
            {
                // vps
                case NAL_UNIT_VPS:
                    param->flgNal |= NAL_FLG_VPS;
					pDemPrm->outWIdx = 0;/*加清零，防止有以前的数据*/
                    if (param->marker_bit)
                    {
                        param->marker_bit = 0;
                    }
					pDemPrm->frmType = FRAME_TYPE_VIDEO_IFRAME;
                    param->vpsLen = param->outbuf_len;
                    break;
                // sps
                case NAL_UNIT_SPS:
                    ret = RtpDemH265SPS(chan);
                    if(ret)
                    {
                        return;
                    }                    
                    break;
                // pps
                case NAL_UNIT_PPS:
                    if ((param->flgNal & NAL_FLG_SPS) != NAL_FLG_SPS)
                    {
                        return;
                    }
                    param->flgNal |= NAL_FLG_PPS;
                    if (param->marker_bit)
                    {
                        param->marker_bit = 0;
                    }
					pDemPrm->frmType = FRAME_TYPE_VIDEO_IFRAME;
                    param->ppsLen = param->outbuf_len;
                    break;
				//P帧
                case NAL_UNIT_CODED_SLICE_RASL_R:
                case NAL_UNIT_CODED_SLICE_RASL_N:
                case NAL_UNIT_CODED_SLICE_RADL_R:
                case NAL_UNIT_CODED_SLICE_RADL_N:
                case NAL_UNIT_CODED_SLICE_STSA_R:
                case NAL_UNIT_CODED_SLICE_STSA_N:
                case NAL_UNIT_CODED_SLICE_TLA_R:
                case NAL_UNIT_CODED_SLICE_TSA_N:
                case NAL_UNIT_CODED_SLICE_TRAIL_R:
                case NAL_UNIT_CODED_SLICE_TRAIL_N:
 
					 if (param->is_new_frame)
                    {
                        param->lastVRtpTime = param->time_stamp;
                        param->flgNal = 0;
                        param->is_new_frame = 0;
                    }

                    if (param->marker_bit)
                    {
                        param->is_new_frame = 1;
                        param->flgNal = 0;
                    }

                    pDemPrm->frmType = FRAME_TYPE_VIDEO_PFRAME;
					break;
				         // I帧
				case NAL_UNIT_CODED_SLICE_BLA_W_LP:
                case NAL_UNIT_CODED_SLICE_BLA_W_RADL:
                case NAL_UNIT_CODED_SLICE_BLA_N_LP:
			    case NAL_UNIT_CODED_SLICE_CRA:
                case NAL_UNIT_CODED_SLICE_IDR_W_RADL:
                case NAL_UNIT_CODED_SLICE_IDR_N_LP:
               
                    ret = RtpDemH265IFrame(chan);
                    if(ret)
                    {
                        return;
                    }
                    break;
                default:
                    if (param->is_new_frame)
                    {
                        param->lastVRtpTime = param->time_stamp;
                        param->flgNal = 0;
                        param->is_new_frame = 0;
                    }

                    if (param->marker_bit)
                    {
                        param->is_new_frame = 1;
                        param->flgNal = 0;
                    }

                    pDemPrm->frmType = FRAME_TYPE_VIDEO_PFRAME;
                    #if 0
                    printf("H265 nal header err!data %x %x %x %x %x\n",
                           param->outbuf[0],
                           param->outbuf[1],
                           param->outbuf[2],
                           param->outbuf[3],
                           param->outbuf[4]);
					#endif
                    return;
            }
		}
		else
		{
		   	if (param->marker_bit)
			{
				param->is_new_frame = 1;
				param->flgNal = 0;
			}
		}
	}
	else
	{
	   
	}

	//if ( param->outbuf[4] == 0x40 ||  param->outbuf[4] == 0x42 ||  param->outbuf[4] == 0x44 ||  param->outbuf[4]== 0x26)
    /*将数据拷贝到码流备份缓冲区*/
	#if 0
	 printf("H265 nal header data %x %x %x %x %x %x,outbuf_len %d\n",
                           param->outbuf[0],
                           param->outbuf[1],
                           param->outbuf[2],
                           param->outbuf[3],
                           param->outbuf[4],
                           param->outbuf[5],
                           param->outbuf_len);
	#endif
    ret = RtpDemCopyData(chan, param->outbuf, param->outbuf_len);
    if(0 != ret)
    {
        return;
    }

    /*保存每个nalu的地址与长度信息，用于转封装处理*/
    if(param->marker_bit)
    {    
        pDemPrm->frmInBuf = 1;
    }

    if(pDemPrm->frmInBuf)
    {   
        pStreamParse = (HIK_STREAM_PARSE  *)pDemPrm->pOutBuf;
        if((!pStreamParse->bufUsed) && (pDemPrm->vOut))
        {
            pStreamParse->bufUsed   = 1;
            pStreamParse->frameLen  = pDemPrm->outWIdx;
            pStreamParse->stampMs   = param->time_stamp;
            pStreamParse->frameType = ((FRAME_TYPE_VIDEO_IFRAME == pDemPrm->frmType) ? VID_I_FRAME : VID_P_FRAME);
            pStreamParse->width     = param->width;
            pStreamParse->height    = param->height;
			pStreamParse->vpsLen    = param->vpsLen;
            pStreamParse->spsLen    = param->spsLen;
            pStreamParse->ppsLen    = param->ppsLen;
			if (NULL == param->stream_info)
			{
			    pStreamParse->time_info = 0;
			}
			else
			{
			    pStreamParse->time_info = param->stream_info->video_info.time_info;
			}
			
            param->ppsLen = 0;
            param->spsLen = 0;
			param->vpsLen = 0;
        }
        pDemPrm->timestamp = (param->time_stamp);
		pStreamParse->streamType = param->stream_type;
    }

    return;
}

/**************************************************************
   Function:     RtpPrivtFrameProcess
   Description:  私有帧帧处理函数，处理水印信息
   Input:        chan - 通道号
   Output:       none
   Returns:      void
 ****************************************************************/
static void RtpDemPrivtFrame(int chan)
{
    #if 0
	unsigned int  demuxType  = demChnInfo[chan].demuxType;
	if (DEM_RTP != demuxType)
	{
	    printf("chan:%d demuxType %d error\n", chan, demuxType);
        return;
	}

    DEMUX_CHN_INFO_ST   *pDemPrm  = &demChnInfo[chan].demChnInfo[demuxType];
    RTPUNPACK_INTERNAL *pInternal = &pDemPrm->rtpInternal;
    RTPUNPACK_PRC_PARAM  *param   = &pInternal->prcParam;

    /*有些私有包marker_bit不置1*/
    if (param->lastPRtpTime != param->time_stamp)
    {
        if (pDemPrm->privtDataLen > 0)
        {
            pDemPrm->privtDataLen = 0;
        }
        param->lastPRtpTime = param->time_stamp;
    }

    if (pDemPrm->privtDataLen + param->outbuf_len < 1024)
    {
        memcpy(pDemPrm->pPrivtDataBuf + pDemPrm->privtDataLen, param->outbuf, param->outbuf_len);
        pDemPrm->privtDataLen += param->outbuf_len;
    }

    if (param->marker_bit == 1)
    {
        pDemPrm->privtDataLen= 0;
    }
	#endif
    return;
}

/**
 * @function   RtpDemMetadataFrame
 * @brief      解析metadata帧信息
 * @param[in]  int chan  
 * @param[out] None
 * @return     static void
 */
static void RtpDemMetadataFrame(int chan)
{
	int ret = 0;

	unsigned int demuxType  = demChnInfo[chan].demuxType;

	DEMUX_CHN_INFO_ST *pstDemChn = NULL;
    RTPUNPACK_PRC_PARAM *param = NULL;
	HIK_STREAM_PARSE *pStreamParse = NULL;

	if (DEM_RTP != demuxType)
	{
	    printf("RtpUnpack chan:%d demuxType %d error\n", chan, demuxType);
        goto exit;
	}

	/* 解完封装后的结果参数 */
	pstDemChn = &demChnInfo[chan].demChnInfo[demuxType];
    param = &pstDemChn->rtpInternal.prcParam;
    pStreamParse = (HIK_STREAM_PARSE *)pstDemChn->pOutBuf;

	/* 拷贝解封装后的负载数据到外部通道缓存中 */
    ret = RtpDemCopyData(chan, param->outbuf, param->outbuf_len);
	if (0 != ret)
	{
		printf("copy demux data failed! \n");
		goto exit;
	}

	//printf("RtpDemMetadataFrame! metadata get markbit %d, outWIdx %d \n", param->marker_bit, pstDemChn->outWIdx);
	
	/* 更新解封装的全局标识 */
	pstDemChn->frmType = RTP_PAYLOAD_TYPE_META_DATA;
	pstDemChn->frmInBuf = param->marker_bit;
	pStreamParse->bufUsed = param->marker_bit;
	pStreamParse->frameLen = pstDemChn->outWIdx;

exit:
	return;
}

/******************************************************************************
   Function:    RtpDemSearchStartCode
   Description: 搜索RTP起始码
   Input:       chan 通道号
   Output:      none
   Returns:     正确返回RTP包长度，错误返回负值
******************************************************************************/
static int RtpDemSearchStartCode(int chan)
{
    int              len, ret;
    unsigned int     nextPos;
	unsigned int  demuxType  = demChnInfo[chan].demuxType;
	if (DEM_RTP != demuxType)
	{
	    printf("RtpDemSearchStartCode chan:%d demuxType %d error\n", chan, demuxType);
        return -1;
	}

    DEMUX_CHN_INFO_ST   *pRtpDem  = &demChnInfo[chan].demChnInfo[demuxType];
    unsigned int      pos = pRtpDem->orgReadIdx;
    unsigned int     dataLen = pRtpDem->orgDataLen;
    unsigned char    *buf = pRtpDem->pOrgBuf;

    if (pRtpDem->orgDataLen < pRtpDem->minDataLen)
    {
        return -1;
    }
    
    /* 防止回头 */
    if (pos + dataLen > pRtpDem->orgBufLen)
    {
        memcpy(pRtpDem->pOrgBuf + pRtpDem->orgBufLen, pRtpDem->pOrgBuf, 8);
    }

	if (pRtpDem->orgReadIdx == 0)
	{
	    printf("%s|%d:chan %d,data[0~7]is:%x %x %x %x,%x %x %x %x\n", __func__, __LINE__, chan,
                               buf[0], buf[1], buf[2], buf[3],buf[4], buf[5], buf[6], buf[7]);
	}

	SAL_dbPrintf(demDbLevel,DEBUG_LEVEL2,"%s|%d:chan %d,orgReadIdx %d,data[0~7]is:%x %x %x %x,%x %x %x %x\n",__func__, __LINE__,chan,pRtpDem->orgReadIdx,
		 buf[0], buf[1], buf[2], buf[3],buf[4], buf[5], buf[6], buf[7]);
    for (len = 4; len < dataLen; len++)                 /* 小于无符号数可能导致问题 */
    {
        if (buf[pos] == 0x88 && buf[pos + 1] == 0x77 && buf[pos + 2] == 0x66 && buf[pos + 3] == 0x55)
        {
            if(len >= dataLen-4)
            {
                continue;
            }
            if (buf[pos + 4] == 0x03 && buf[pos + 5] == 0x00)
            {
                ret = buf[pos + 6] << 8;
                ret += buf[pos + 7];
                if (ret > MAX_RTPUNPACK_SIZE)
                {
                    printf("chan %d,pos %d,search 88 77 66 55 03 00,Rtp demux invalid packet len %d\n", chan,pos,ret);
                }
                else if (dataLen - len - 8 > ret)
                {
                    /* 剩余数据长度大于包长时校验是否为下一个包的起始，否则重新搜索下一个包 */
                    nextPos = pos + 8 + ret;
                    if (nextPos >= pRtpDem->orgBufLen)
                    {
                        nextPos -= pRtpDem->orgBufLen;
                    }

                    if (!((buf[nextPos] == 0x88 && buf[nextPos + 1] == 0x77 && buf[nextPos + 2] == 0x66 && buf[nextPos + 3] == 0x55)
                          || (buf[nextPos] == 0x03 && buf[nextPos + 1] == 0x00)
                          || (buf[nextPos] == 0x00 && (nextPos & 3))))
                    {
                        printf("%s|%d: chan %d,pos %d,search 88 77 66 55 03 00,rtp demux sync lost!%x %x %x %x, ret 0x%x, nextpos %d\n", 
                            __func__, __LINE__, chan, pos, buf[nextPos], buf[nextPos + 1], buf[nextPos + 2], buf[nextPos + 3], ret, nextPos);
                    }
                    else
                    {
                        pRtpDem->orgReadIdx = pos + 8;
                        if (pRtpDem->orgReadIdx >= pRtpDem->orgBufLen)
                        {
                            pRtpDem->orgReadIdx -= pRtpDem->orgBufLen;
                        }

                        return ret;
                    }
                }
                else
                {
                    pRtpDem->orgReadIdx = pos + 8;
                    if (pRtpDem->orgReadIdx >= pRtpDem->orgBufLen)
                    {
                        pRtpDem->orgReadIdx -= pRtpDem->orgBufLen;
                    }

                    return ret;
                }
            }
            else
            {
                printf("chan %d,pos %d,search rtp packet failed %d %d\n",chan, pos,buf[pos + 4], buf[pos + 5]);
            }
        }
        else if (buf[pos] == 0x03 && buf[pos + 1] == 0x00) /* tpkt头 */
        {
            if(len >= dataLen-4)
            {
                continue;
            }
            //ret  = buf[pos + 2] << 8;
            //ret += buf[pos + 3];

            ret    = buf[pos + 2] + (buf[pos + 3] << 8) - 4;
            if (ret > MAX_RTPUNPACK_SIZE)
            {
                printf("%s|%d: chan %d,Rtp demux invalid packet len %d %d \n",__func__, __LINE__,chan,ret,pos);
            }
            else if (dataLen - len - 4 > ret)
            {
                nextPos = pos + 4 + ret;
                if (nextPos >= pRtpDem->orgBufLen)
                {
                    nextPos -= pRtpDem->orgBufLen;
                }

                if (!((buf[nextPos] == 0x88 && buf[nextPos + 1] == 0x77 && buf[nextPos + 2] == 0x66 && buf[nextPos + 3] == 0x55)
                      || (buf[nextPos] == 0x03 && buf[nextPos + 1] == 0x00)
                      || (buf[nextPos] == 0x00 && (nextPos & 3))))
                {
                    printf("%s|%d:chan %d,pos %d,rtp demux sync lost!%x %x %x %x pos %d nextPos %d ret %x pos %x %x\n",__func__, __LINE__,chan,pos,
                           buf[nextPos], buf[nextPos + 1], buf[nextPos + 2], buf[nextPos + 3],pos,nextPos,ret,buf[pos + 2],buf[pos + 3]);
                }
                else
                {
                    pRtpDem->orgReadIdx = pos + 4;
                    if (pRtpDem->orgReadIdx >= pRtpDem->orgBufLen)
                    {
                        pRtpDem->orgReadIdx -= pRtpDem->orgBufLen;
                    }
                    return ret;
                }
            }
            else
            {
                pRtpDem->orgReadIdx = pos + 4;
                if (pRtpDem->orgReadIdx >= pRtpDem->orgBufLen)
                {
                    pRtpDem->orgReadIdx -= pRtpDem->orgBufLen;
                }

                return ret;
            }
        }
        else if (((buf[pos] == 0x24) && (buf[pos + 1] == 0x00))
               ||((buf[pos] == 0x24)  &&  (buf[pos + 1] == 0x02)))
        {
            ret = (buf[pos + 2] << 8) + buf[pos + 3];
            if (ret > MAX_RTPUNPACK_SIZE)
            {
                printf("%s|%d: chan%d,pos %d,Rtp demux invalid packet len %d\n",__func__, __LINE__,chan,pos,ret);
            }
            else if (dataLen - len - 4 > ret)
            {
                nextPos = pos + 4 + ret;
                if (nextPos >= pRtpDem->orgBufLen)
                {
                    nextPos -= pRtpDem->orgBufLen;
                }

                if (!((buf[nextPos] == 0x24 && buf[nextPos + 1] == 0x00) 
                    ||(buf[nextPos] == 0x24 && buf[nextPos + 1] == 0x04)
                    ||(buf[nextPos] == 0x24 && buf[nextPos + 1] == 0x02)))
                {
                    printf("%s|%d:chan %d,pos %d,rtp demux sync lost!%x %x %x %x pos %d nextPos %d\n",__func__, __LINE__,chan,pos,
                           buf[nextPos], buf[nextPos + 1], buf[nextPos + 2], buf[nextPos + 3],pos,nextPos);
                }
                else
                {
                    pRtpDem->orgReadIdx = pos + 4;
                    if (pRtpDem->orgReadIdx >= pRtpDem->orgBufLen)
                    {
                        pRtpDem->orgReadIdx -= pRtpDem->orgBufLen;
                    }
                    return ret;
                }
            }
            else
            {
                pRtpDem->orgReadIdx = pos + 4;
                if (pRtpDem->orgReadIdx >= pRtpDem->orgBufLen)
                {
                    pRtpDem->orgReadIdx -= pRtpDem->orgBufLen;
                }
                return ret;
            }
        }

        pos++;
        if (pos >= pRtpDem->orgBufLen)
        {
            pos -= pRtpDem->orgBufLen;
        }
    }

    if (len > 4)
    {
        printf("chan = %d rtpUnpack search start code skip %d bytes\n", chan, len - 4);
    }

    pRtpDem->orgReadIdx = pos;
    return -1;
}

/*************************************************
   Function:        RtpUnpack
   Description:     RTP解包函数
   Input:           chan - 通道号
   Output:          none
   Returns:         错误码
*************************************************/
static int RtpUnpack(int chan)
{
	unsigned int  demuxType  = demChnInfo[chan].demuxType;
	if (DEM_RTP != demuxType)
	{
	    printf("RtpUnpack chan:%d demuxType %d error\n", chan, demuxType);
        return -1;
	}

    DEMUX_CHN_INFO_ST   *pDemChn  = &demChnInfo[chan].demChnInfo[demuxType];
    RTPUNPACK_INTERNAL *pInternal = &pDemChn->rtpInternal;
    RTPUNPACK_PRC_PARAM  *param   = &pInternal->prcParam;
    HIK_STREAM_PARSE *pStreamParse = NULL;
    unsigned int         rest_len;
	static int cnt[MAX_DEMUX_NUM] = {0};
	
    int ret = 0;
    pStreamParse    = (HIK_STREAM_PARSE *)pDemChn->pOutBuf;
    while (1)
    {

        if(pStreamParse->bufUsed)
        {
        	cnt[chan]++;
			if (cnt[chan] > 120)
			{
				pStreamParse->bufUsed = 0;
			}
        	SAL_dbPrintf(demDbLevel,DEBUG_LEVEL2,"%s|%d:chan %d ridx %d\n",__func__,__LINE__,chan,pDemChn->orgReadIdx);
            return -1;
        }
        cnt[chan] = 0;
        if (pDemChn->bSearchStartCode)
        {
            ret = RtpDemSearchStartCode(chan);
            /*搜索过以后都要重新计算有效数据 */
            pDemChn->orgDataLen = pDemChn->orgWriteIdx >= pDemChn->orgReadIdx ? \
                                  pDemChn->orgWriteIdx  - pDemChn->orgReadIdx : \
                                  pDemChn->orgBufLen    - pDemChn->orgReadIdx + pDemChn->orgWriteIdx;
            if (ret < 0)
            {
                return 0;
            }
            else if (ret > MAX_RTPUNPACK_SIZE || ret == 0)
            {
                printf("(%d)RTPUnPack_search_StartCode Error,ret=%d,orgR=%d\n", chan, ret, pDemChn->orgReadIdx);
                //usleep(5 * 1000);
                continue;
            }
            param->inbuf_len = ret;
            param->inbuf = pDemChn->pOrgBuf + pDemChn->orgReadIdx;
            pDemChn->bSearchStartCode = 0;
        }
	
        /*等待码流 */
        if (pDemChn->orgDataLen < param->inbuf_len)
        {
            //printf("!!!waiting stream %d %d\n", pDemChn->orgDataLen, param->inbuf_len);
            return 0;
        }

        rest_len = pDemChn->orgBufLen - pDemChn->orgReadIdx;
        if (rest_len < param->inbuf_len)
        {
            memcpy(pDemChn->pOrgBuf + pDemChn->orgBufLen, pDemChn->pOrgBuf, (param->inbuf_len - rest_len));
        }
        param->outbuf = param->outBufBak;
		param->streamType = pDemChn->videoStreamType;
        ret = RtpUnpack_Process(param,pDemChn->rtpDemuxHdl);
        pDemChn->bSearchStartCode = 1;


        pDemChn->orgReadIdx   += param->inbuf_len;
        if (pDemChn->orgReadIdx >= pDemChn->orgBufLen)
        {
            pDemChn->orgReadIdx -= pDemChn->orgBufLen;
        }
        
        /*有效长度计算 */
        pDemChn->orgDataLen = pDemChn->orgWriteIdx >= pDemChn->orgReadIdx ? \
                              pDemChn->orgWriteIdx  - pDemChn->orgReadIdx : \
                              pDemChn->orgBufLen    - pDemChn->orgReadIdx + pDemChn->orgWriteIdx;

        if(HIK_RTP_UNPACK_LIB_S_OK != ret)
        {    
            //printf("%s|%d: Rtp demux, ret %x, payload type %d \n",__func__, __LINE__, ret, param->payload_type);
            continue;
        }

        /*RTP解包音频的mark_bit不置1，而且在一帧视频中有可能混杂音频rtp包 */
        if (param->payload_type == pDemChn->audioPayloadType
			|| (param->payload_type == HIK_PAYLOAD_TYPE_G711U_AUDIO
            || param->payload_type == HIK_PAYLOAD_TYPE_G711A_AUDIO
            || param->payload_type == HIK_PAYLOAD_TYPE_AAC_AUDIO))
        {
            RtpDemAudioFrame(chan);
        }
        else if (param->payload_type == pDemChn->videoPayloadType)
        {
            RtpDemVideoFrame(chan);
        }
        else if(param->is_privt_info)
        {
            RtpDemPrivtFrame(chan);
        }
		/* 增加metadata解析 */
		else if (RTP_PAYLOAD_TYPE_META_DATA == param->payload_type)
		{
			RtpDemMetadataFrame(chan);
			//SAL_LOGE("====================== get depth info payload! \n");
		}

        if(1 == pDemChn->frmInBuf)
        {
			//SAL_LOGW("====================== get full frame! \n");
            break;
        }
    }
    
    return 0;
}

/***************************************************************
   Function:    InitRtpUnpackHandle
   Description: 解包模块初始化
   Input:       chan - 通道号
                handle - 句柄指针
   Output:      初始化的句柄
   Returns:     错误码
 ****************************************************************/
static int RtpDemCreate(int chan)
{
    int                nRet  = -1;
	unsigned int  demuxType  = demChnInfo[chan].demuxType;
	if (DEM_RTP != demuxType)
	{
	    printf("RtpDemCreate chan:%d demuxType %d error\n", chan, demuxType);
        return -1;
	}

    DEMUX_CHN_INFO_ST   *pRtpDem  = &demChnInfo[chan].demChnInfo[demuxType];
    RTPUNPACK_INTERNAL  *pInternal      = 0;
    RTPUNPACK_PARAM      *pTopPrm       = 0;
    RTPUNPACK_PRC_PARAM  *pPrcPrm       = 0;
    HIK_STREAM_PARSE    *pStreamParse   = NULL;

    if(MAX_DEMUX_NUM <= chan)
    {
        printf("chan %d MAX_RTPDEM_NUM %d",chan,MAX_DEMUX_NUM);
        return nRet;
    }

    pRtpDem->demuxIdx      = chan;
    pRtpDem->memUsed       = 0;

    MEMCHECK(pRtpDem->memLen,pRtpDem->memUsed,pRtpDem->outBufLen);
    pRtpDem->pOutBuf       = pRtpDem->buffer + pRtpDem->memUsed;
    pRtpDem->memUsed      += pRtpDem->outBufLen;
    pRtpDem->frmInBuf      = 0;
    pRtpDem->minDataLen    = MIN_DATA_LEN;
    pStreamParse           = (HIK_STREAM_PARSE *)pRtpDem->pOutBuf;
    pStreamParse->bufUsed  = 0;

    pInternal = &pRtpDem->rtpInternal;
    pTopPrm   = &pInternal->topParam;
    pPrcPrm   = &pInternal->prcParam;

    memset(pTopPrm,  0, sizeof(RTPUNPACK_PARAM));
    memset(pPrcPrm,  0, sizeof(RTPUNPACK_PRC_PARAM));

    pTopPrm->stream_type = STREAM_TYPE_UNDEF;
    pPrcPrm->stream_type = STREAM_TYPE_UNDEF;
    pPrcPrm->outbuf_size = MAX_RTPUNPACK_SIZE + 1024;

    nRet = RtpUnpack_GetMemSize(pTopPrm);
    if(HIK_RTP_UNPACK_LIB_S_OK != nRet)
    {
        printf("RtpUnpack_GetMemSize s32Ret %x\n",nRet);
    }

    /* 申请分离器所需的内存 */
    MEMCHECK(pRtpDem->memLen,pRtpDem->memUsed,pTopPrm->buf_size);
    pTopPrm->buffer   = pRtpDem->buffer + pRtpDem->memUsed;
    pRtpDem->memUsed += pTopPrm->buf_size;

    MEMCHECK(pRtpDem->memLen,pRtpDem->memUsed,pPrcPrm->outbuf_size);
    pPrcPrm->outbuf   = pRtpDem->buffer + pRtpDem->memUsed;
    pRtpDem->memUsed += pPrcPrm->outbuf_size;

    MEMCHECK(pRtpDem->memLen,pRtpDem->memUsed, SMART_RESULT_LENGHT);
    pRtpDem->pSmartIndexBuf = pRtpDem->buffer + pRtpDem->memUsed;
    pRtpDem->memUsed += SMART_RESULT_LENGHT;
    nRet = RtpUnpack_Create(pTopPrm, &pRtpDem->rtpDemuxHdl);
    if(HIK_RTP_UNPACK_LIB_S_OK!= nRet)
    {
        printf("RtpUnpack_Create %x\n",nRet);
        return nRet;
    }

    nRet = DEMUX_S_OK;
    pPrcPrm->videoPayloadType = DEFAULT_VIDEO_PT;
    pRtpDem->videoPayloadType = DEFAULT_VIDEO_PT;
    pRtpDem->audioPayloadType = DEFAULT_AUDIO_PT;
    pRtpDem->bCreated     = 1;
    pPrcPrm->outBufBak    = pPrcPrm->outbuf;

    return nRet;
}

/*************************************************
   Function:        PsUnpack
   Description:     PS解包函数
   Input:           chan - 通道号
   Output:          none
   Returns:         错误码
*************************************************/
static int PsUnpack(int chan)
{
	unsigned int  demuxType  = demChnInfo[chan].demuxType;
	if (DEM_PS != demuxType)
	{
	    printf("PsUnpack chan:%d demuxType %d error\n", chan, demuxType);
        return -1;
	}

    DEMUX_CHN_INFO_ST   *pDemChn  = &demChnInfo[chan].demChnInfo[demuxType];
    PSUNPACK_INTERNAL * pInternal = &pDemChn->psInternal;
    PSDEMUX_PROCESS_PARAM *prcprm = &pInternal->dynPrm;
    int                     ret   = 0;

    prcprm->orgWIdx     = pDemChn->orgWriteIdx;
    ret = PSDEMUX_Process(prcprm,pDemChn->psDemuxHdl);
	if (ret <= 0)
	{
	    printf("PsUnpack chan:%d demuxType %d ret 0x%x,error\n", chan, demuxType,ret);
	    return -1;
	}
    pDemChn->orgReadIdx = prcprm->orgRIdx;
    pDemChn->frmInBuf   = prcprm->frmInBuf;
    pDemChn->timestamp  = prcprm->stamp/45;
    pDemChn->frmType    = prcprm->frmType;
    prcprm->frmInBuf    = 0;
    return DEMUX_S_OK;
}

static int PsDemCreate(int chan)
{
    int                    nRet;
	unsigned int  demuxType  = demChnInfo[chan].demuxType;
	if (DEM_PS != demuxType)
	{
	    printf("PsDemCreate chan:%d demuxType %d error\n", chan, demuxType);
        return -1;
	}

    DEMUX_CHN_INFO_ST   *pPsDem  = &demChnInfo[chan].demChnInfo[demuxType];

    PSDEMUX_PARAM       *pstatPrm   = &pPsDem->psInternal.staPrm;
    HIK_STREAM_PARSE    *pStreamParse = NULL;
    PSDEMUX_OUT_PARAM    psDemOut;

    if(MAX_DEMUX_NUM <= chan)
    {
        printf("chan %d MAX_RTPDEM_NUM %d",chan,MAX_DEMUX_NUM);
        return DEMUX_S_CHAN_ERR;
    }

    pPsDem->demuxIdx       = chan;
    pPsDem->memUsed        = 0;

    MEMCHECK(pPsDem->memLen,pPsDem->memUsed,pPsDem->outBufLen);
    pPsDem->pOutBuf        = pPsDem->buffer + pPsDem->memUsed;
    pPsDem->memUsed       += pPsDem->outBufLen;
    pPsDem->frmInBuf       = 0;
    pPsDem->minDataLen     = MIN_DATA_LEN;
    pStreamParse           = (HIK_STREAM_PARSE *)pPsDem->pOutBuf;
    pStreamParse->bufUsed  = 0;
    
    MEMCHECK(pPsDem->memLen,pPsDem->memUsed,pstatPrm->buf_size);
    pstatPrm->buffer    = pPsDem->buffer + pPsDem->memUsed;
    pPsDem->memUsed    += pstatPrm->buf_size;

    pstatPrm->output_callback_func = NULL;
    nRet = PSDEMUX_Create(pstatPrm,&pPsDem->psDemuxHdl);
    if(HIK_PSDEMUX_LIB_S_OK != nRet)
    {
        printf("chan %d PSDEMUX_Create ret %d",chan,nRet);
        return DEMUX_S_FAIL;
    }

    psDemOut.bufAddr   = pPsDem->pOutBuf;
    psDemOut.lenPerFrm = pPsDem->outBufLen;
    psDemOut.frmCnt    = 1;
    psDemOut.bufLen    = pPsDem->outBufLen;
    psDemOut.aOut      = pPsDem->aOut;
    psDemOut.vOut      = pPsDem->vOut;
    PSDEMUX_SetOutBuf(&psDemOut,pPsDem->psDemuxHdl);

    return DEMUX_S_OK;
}

static int Dem_MemSize(int chan,int level,unsigned int* memSz)
{
    int                 mem  = 0;
	unsigned int  demuxType  = demChnInfo[chan].demuxType;
	if (demuxType >= DEM_NONE)
	{
	    printf("Dem_MemSize chan:%d demuxType %d error\n", chan, demuxType);
        return -1;
	}

    DEMUX_CHN_INFO_ST   *pDemChn  = &demChnInfo[chan].demChnInfo[demuxType];

    if(SIZE_STREAM_MAX <= level)
    {
        printf("level %d SIZE_MAX %d",level,SIZE_STREAM_MAX);
        return DEMUX_S_FAIL;
    }

    if(MAX_DEMUX_NUM <= chan)
    {
        printf("chan %d MAX_RTPDEM_NUM %d",chan,MAX_DEMUX_NUM);
        return DEMUX_S_CHAN_ERR;
    }

    pDemChn->outBufLen  = sizeof(HIK_STREAM_PARSE);
    pDemChn->outBufLen += (pDemChn->vOut ? DEC_ORG_BUF_4MBPS_LEN : DEC_ONLY_AUDIO_LEN);
    mem += 2*1024*1024;
    mem += pDemChn->outBufLen;

    if(DEM_RTP == pDemChn->demuxType)
    {
        RtpUnpack_GetMemSize(&pDemChn->rtpInternal.topParam);
        mem += pDemChn->rtpInternal.topParam.buf_size;
    }
    else if(DEM_PS == pDemChn->demuxType)
    {
        PSDEMUX_GetMemSize(&pDemChn->psInternal.staPrm);
        mem += pDemChn->psInternal.staPrm.buf_size;
    }
    *memSz = mem;
    return DEMUX_S_OK;
}

static int Dem_FullBuf(void *inBuf,void *outBuf)
{
    int                ret  = DEMUX_S_FAIL;
    int                demIdx;
    DEMUX_CHN_INFO_ST *pDemChn = NULL;
    unsigned char*     pData = 0;
    DEM_BUFFER       *demBuf = NULL;
    unsigned int     strParseSize   = sizeof(HIK_STREAM_PARSE);
    HIK_STREAM_PARSE  *pStreamParse = NULL;
	DEMUX_INFO_ST       *pdemuxChn = NULL;

    if((NULL == inBuf) || (NULL == outBuf))
    {
        printf("NULL point Dem_SetOrgBuf inBuf %p outBuf %p\n",inBuf,outBuf);
        return DEMUX_S_NULL_POINTER_ERR;
    }

    demIdx  = *((int *)inBuf);
    if(MAX_DEMUX_NUM <= demIdx)
    {
        printf("Dem_SetOrgBuf demux err %d \n",demIdx);
        return DEMUX_S_CHAN_ERR;
    }
    demBuf  = (DEM_BUFFER *)outBuf;

	pdemuxChn = demChnInfo + demIdx;
    if (pdemuxChn->demuxType >= DEM_NONE)
	{
	    printf("Dem_FullBuf chan:%d demuxType %d error\n", demIdx, pdemuxChn->demuxType);
        return -1;
	}
	
    pDemChn  = &demChnInfo[demIdx].demChnInfo[pdemuxChn->demuxType];
    
    pthread_mutex_lock(&pDemChn->demlock);
    if(DEM_RTP == pDemChn->demuxType)
    {
        if(STREAM_FRAME_TYPE_AUDIO == demBuf->bufType)
        {
            pStreamParse        = (HIK_STREAM_PARSE *)pDemChn->pOutBuf;
            if(pStreamParse->bufUsed && (FRAME_TYPE_AUDIO_FRAME == pStreamParse->frameType))
            {
                pStreamParse->bufUsed = 0;
                demBuf->datalen   = pStreamParse->frameLen;
                demBuf->contLen   = pStreamParse->frameLen;
                demBuf->timestamp = pStreamParse->stampMs;
                demBuf->frmType   = pStreamParse->frameType;
				demBuf->frame_num = pStreamParse->frameNum;
                demBuf->audEncType = pStreamParse->audEncType;
                pData  = pDemChn->pOutBuf + strParseSize;
                if((demBuf->length >= demBuf->datalen) && demBuf->addr)
                {
                    memcpy(demBuf->addr,pData,demBuf->datalen);
                }
                else if(NULL == demBuf->addr)
                {
                    demBuf->addr = pData;
                }
                demBuf->contAddr  = demBuf->addr;
                ret = DEMUX_S_OK;
            }
			else
			{
				pStreamParse->bufUsed = 0;
			}
        }
        else if(STREAM_FRAME_TYPE_VIDEO == demBuf->bufType)
        {
            RTPUNPACK_PRC_PARAM *pstPrcPrm = NULL;
            pstPrcPrm = &pDemChn->rtpInternal.prcParam;

            pStreamParse          = (HIK_STREAM_PARSE *)pDemChn->pOutBuf;
            if(pStreamParse->bufUsed && (STREAM_FRAME_TYPE_AUDIO != pStreamParse->frameType))
            {
                pStreamParse->bufUsed = 0;
                demBuf->contLen   = pStreamParse->frameLen;
                demBuf->datalen   = pStreamParse->frameLen;
                demBuf->timestamp = pStreamParse->stampMs;
                demBuf->frmType   = pStreamParse->frameType;
                demBuf->width     = pStreamParse->width;
                demBuf->height    = pStreamParse->height;
				demBuf->vpsLen    = pStreamParse->vpsLen;
                demBuf->spsLen    = pStreamParse->spsLen;
                demBuf->ppsLen    = pStreamParse->ppsLen;
				demBuf->frame_num = pStreamParse->frameNum;
				demBuf->time_info = pStreamParse->time_info;

                if (pstPrcPrm->stream_info)
                {
                    demBuf->glb_time.year = pstPrcPrm->stream_info->glb_time.year;
                    demBuf->glb_time.month = pstPrcPrm->stream_info->glb_time.month;
                    demBuf->glb_time.date = pstPrcPrm->stream_info->glb_time.date;
                    demBuf->glb_time.hour = pstPrcPrm->stream_info->glb_time.hour;
                    demBuf->glb_time.minute = pstPrcPrm->stream_info->glb_time.minute;
                    demBuf->glb_time.second = pstPrcPrm->stream_info->glb_time.second;
                    demBuf->glb_time.msecond = pstPrcPrm->stream_info->glb_time.msecond;

                    SAL_DEBUG("szl_dbg: rtp get glb time[%d-%d-%d] [%d:%d:%d.%d], chan %d \n", 
                              demBuf->glb_time.year, demBuf->glb_time.month, demBuf->glb_time.date, 
                              demBuf->glb_time.hour, demBuf->glb_time.minute, demBuf->glb_time.second, demBuf->glb_time.msecond, demIdx);
                }

				demBuf->streamType= H264;
				if(STREAM_TYPE_VIDEO_H265 == pStreamParse->streamType)
				{
				    demBuf->streamType= H265;
				}
                pData    = pDemChn->pOutBuf + strParseSize;
                if((demBuf->length >= demBuf->datalen) && demBuf->addr)
                {
                    memcpy(demBuf->addr,pData,demBuf->datalen);
                }
                else if(NULL == demBuf->addr)
                {
                    demBuf->addr = pData;
                }
                demBuf->contAddr  = demBuf->addr;
				
                if(VID_I_FRAME == demBuf->frmType)
                {
                    if (STREAM_TYPE_VIDEO_H265 == pStreamParse->streamType)
                    {
                        demBuf->vpsAddr  = demBuf->addr;
						demBuf->spsAddr  = demBuf->addr + demBuf->vpsLen;
                        demBuf->ppsAddr  = demBuf->addr + demBuf->spsLen + demBuf->vpsLen;
                        demBuf->contAddr = demBuf->addr + demBuf->spsLen + demBuf->ppsLen + demBuf->vpsLen;
                        if (demBuf->contLen >= (demBuf->spsLen + demBuf->ppsLen + demBuf->vpsLen))
                        {
                            demBuf->contLen -= (demBuf->spsLen + demBuf->ppsLen + demBuf->vpsLen);
                        }
                        else
                        {
                            printf("###### error: contLen %d small than (spsLen + ppsLen + vpsLen) %d \n", \
                                demBuf->contLen, demBuf->spsLen + demBuf->ppsLen + demBuf->vpsLen);
                            demBuf->contLen = 0;   
                        }
                    }
					else
					{
					    demBuf->spsAddr  = demBuf->addr;
	                    demBuf->ppsAddr  = demBuf->addr + demBuf->spsLen;
	                    demBuf->contAddr = demBuf->addr + demBuf->spsLen + demBuf->ppsLen;
	                    //加保护。偶尔出现contLen小于(spsLen + ppsLen)，导致contLen异常大从而崩溃
                        if (demBuf->contLen >= (demBuf->spsLen + demBuf->ppsLen))
                        {
	                        demBuf->contLen -= (demBuf->spsLen + demBuf->ppsLen);
                        }
                        else
                        {
                            printf("###### error: contLen %d small than (spsLen + ppsLen) %d \n", \
                                demBuf->contLen, demBuf->spsLen + demBuf->ppsLen);
                            demBuf->contLen = 0;
                        }
					}
                    
                }

                ret = DEMUX_S_OK;
            }
			else
			{
				pStreamParse->bufUsed = 0;
			}
        }
		else if(STREAM_FRAME_TYPE_METADATA == demBuf->bufType)
		{
			/* 获取解封装后得到的负载数据长度 */
			pStreamParse = (HIK_STREAM_PARSE *)pDemChn->pOutBuf;
			if (pStreamParse->bufUsed)
			{
				pStreamParse->bufUsed = 0;
				demBuf->datalen = pStreamParse->frameLen;
			}
		
			/* 将解封装后的负载数据送到外部处理单元 */
			pData = pDemChn->pOutBuf + strParseSize;
			if((demBuf->length >= demBuf->datalen) && demBuf->addr)
			{
				memcpy(demBuf->addr, pData, demBuf->datalen);
			}
			else if(NULL == demBuf->addr)
			{
				demBuf->addr = pData;
			}
		
			ret = DEMUX_S_OK;
		}

		else
		{
			//非音视频数据不允许占用解复用通道
			pStreamParse = (HIK_STREAM_PARSE *)pDemChn->pOutBuf;
			if(pStreamParse->bufUsed && (STREAM_FRAME_TYPE_AUDIO != pStreamParse->frameType) && (STREAM_FRAME_TYPE_VIDEO != pStreamParse->frameType))
            {
                pStreamParse->bufUsed = 0;
			}
		}
    }
    else if(DEM_PS == pDemChn->demuxType)
    {
        PSDEMUX_PROCESS_PARAM param;
        memset(&param, 0, sizeof(param));
        param.outbuf    = demBuf->addr;
        param.outbuflen = demBuf->length;
        if(STREAM_FRAME_TYPE_VIDEO == demBuf->bufType)
        {
            param.payload = 0;
        }
        else if(STREAM_FRAME_TYPE_AUDIO == demBuf->bufType)
        {
            param.payload = 1;
        }
        else
        {
            param.payload = 2;
        }
                    
        ret = PSDEMUX_FullBuf(&param,pDemChn->psDemuxHdl);
        if(HIK_PSDEMUX_LIB_S_OK != ret)
        {
            ret = DEMUX_S_FAIL;
			printf("error chan:%d,  demBuf->addr %p, w*h %dx%d\n", demIdx, demBuf->addr,demBuf->width,demBuf->height);
            printf("param: payload %d, width %d, height %d \n", param.payload, param.width, param.height);
            pthread_mutex_unlock(&pDemChn->demlock);
            return ret;
        }
        ret = DEMUX_S_OK;
        demBuf->contLen   = param.outdatalen;
        demBuf->datalen   = param.outdatalen;
        demBuf->timestamp = param.stamp;
        demBuf->frmType   = 0;
        demBuf->width     = param.width;
        demBuf->height    = param.height;
        demBuf->addr      = param.outbuf;
		demBuf->frame_num = param.frameNum;
		demBuf->time_info = param.time_info;
		demBuf->glb_time.year  = param.glb_time.year;
		demBuf->glb_time.month = param.glb_time.month;
		demBuf->glb_time.date  = param.glb_time.date;
		demBuf->glb_time.hour  = param.glb_time.hour;
		demBuf->glb_time.minute= param.glb_time.minute;
		demBuf->glb_time.second= param.glb_time.second;
		demBuf->glb_time.msecond = param.glb_time.msecond;
		demBuf->audEncType = param.frmType;
        if(STREAM_FRAME_TYPE_VIDEO == demBuf->bufType)
        {
            demBuf->streamType = H264;
			if(STREAM_TYPE_VIDEO_H265 == param.streamType)
			{
			    demBuf->streamType = H265;
			}
            demBuf->frmType   = (FRAME_TYPE_VIDEO_IFRAME == param.frmType ? VID_I_FRAME : VID_P_FRAME);
            if(VID_I_FRAME == demBuf->frmType)
            {
                demBuf->vpsLen    = param.vpsLen;
                demBuf->spsLen    = param.spsLen;
                demBuf->ppsLen    = param.ppsLen;
				if (STREAM_TYPE_VIDEO_H265 == param.streamType)
                {
                    demBuf->vpsAddr  = demBuf->addr;
					demBuf->spsAddr  = demBuf->addr + demBuf->vpsLen;
                    demBuf->ppsAddr  = demBuf->addr + demBuf->spsLen + demBuf->vpsLen;
                    demBuf->contAddr = demBuf->addr + demBuf->spsLen + demBuf->ppsLen + demBuf->vpsLen;
                    demBuf->contLen -= (demBuf->spsLen + demBuf->ppsLen + demBuf->vpsLen);
                }
				else
				{
				    demBuf->spsAddr  = demBuf->addr;
                    demBuf->ppsAddr  = demBuf->addr + demBuf->spsLen;
                    demBuf->contAddr = demBuf->addr + demBuf->spsLen + demBuf->ppsLen;
                    demBuf->contLen -= (demBuf->spsLen + demBuf->ppsLen);
				}
            }
        }
        else if(STREAM_FRAME_TYPE_PRIVT == demBuf->bufType)
        {
            demBuf->privtType = param.privtType;
            demBuf->privtSubType = param.privtSubType;
        }
        
    }
    pthread_mutex_unlock(&pDemChn->demlock);
    return ret;
}

static int Dem_Reset(void *inBuf,void *outBuf)
{
    int demIdx;
	unsigned int vencType = 0;
    int ret = DEMUX_S_FAIL;
    DEMUX_CHN_INFO_ST *pDemChn    = NULL;
	DEMUX_INFO_ST     *pdemuxChn  = NULL;

    if(NULL == inBuf || NULL == outBuf)
    {
        printf("NULL point Dem_Clear inBuf %p outBuf %p\n",inBuf,outBuf);
        return DEMUX_S_NULL_POINTER_ERR;
    }
	
    vencType= *((int *)outBuf);
    demIdx  = *((int *)inBuf);
    if(MAX_DEMUX_NUM <= demIdx)
    {
        printf("Dem_SetOrgBuf demux err %d \n",demIdx);
        return DEMUX_S_CHAN_ERR;
    }

	pdemuxChn = demChnInfo + demIdx;
   	if (pdemuxChn->demuxType >= DEM_NONE)
	{
	    printf("Dem_Reset chan:%d demuxType %d error\n", demIdx, pdemuxChn->demuxType);
        return -1;
	}
	
    pDemChn  = &demChnInfo[demIdx].demChnInfo[pdemuxChn->demuxType];
    pthread_mutex_lock(&pDemChn->demlock);

    pDemChn->frmInBuf   =  0;
    pDemChn->orgReadIdx =  0;
    pDemChn->outWIdx    =  0;
    
    if(DEM_PS == pDemChn->demuxType)
    {
        ret = PSDEMUX_Reset(pDemChn->psDemuxHdl);
    }
	else if (DEM_RTP == pDemChn->demuxType)
	{
	    pDemChn->rtpInternal.prcParam.stream_type = STREAM_TYPE_UNDEF;
		if (1 == vencType)
		{
		    pDemChn->rtpInternal.prcParam.stream_type = STREAM_TYPE_VIDEO_H264;
		}
		else if (2 == vencType)
		{
		    pDemChn->rtpInternal.prcParam.stream_type = STREAM_TYPE_VIDEO_H265;
		}
	}
    pthread_mutex_unlock(&pDemChn->demlock);

    return ret;
}

static int Dem_SetOrgBuf(void *inBuf,void *outBuf)
{
    int       demIdx;
    DEMUX_CHN_INFO_ST *pDemChn    = NULL;
    DEM_ORGBUF_PRM    *pDemOrgBuf = NULL;
	DEMUX_INFO_ST     *pdemuxChn  = NULL;
    
    if((NULL == inBuf) || (NULL == outBuf))
    {
        printf("NULL point Dem_SetOrgBuf inBuf %p outBuf %p\n",inBuf,outBuf);
        return DEMUX_S_NULL_POINTER_ERR;
    }

    demIdx  = *((int *)inBuf);
    if(MAX_DEMUX_NUM <= demIdx)
    {
        printf("Dem_SetOrgBuf demux err %d \n",demIdx);
        return DEMUX_S_CHAN_ERR;
    }


	pdemuxChn = demChnInfo + demIdx;
   	if (pdemuxChn->demuxType >= DEM_NONE)
	{
	    printf("Dem_SetOrgBuf chan:%d demuxType %d error\n", demIdx, pdemuxChn->demuxType);
        return -1;
	}

    pDemChn  = &demChnInfo[demIdx].demChnInfo[pdemuxChn->demuxType];
    pDemOrgBuf = (DEM_ORGBUF_PRM *)outBuf;

    /*将应用共用缓存地址赋给本地解封装变量变量*/
    pDemChn->pOrgBuf   = pDemOrgBuf->orgBuf;
    pDemChn->orgBufLen = pDemOrgBuf->orgbuflen;

    if(DEM_PS == pDemChn->demuxType)
    {
        PSDEMUX_SetRingBuf(pDemChn->pOrgBuf,pDemChn->orgBufLen,pDemChn->psDemuxHdl);
    }

    return    DEMUX_S_OK;
}

static int Dem_CalcData(void *inBuf,void *outBuf)
{
    int                 muxHdl    = 0;
    DEMUX_CHN_INFO_ST *pDemChn    = NULL;
	DEMUX_INFO_ST     *pdemuxChn  = NULL;
    DEM_CALC_PRM      *demCalcPrm = (DEM_CALC_PRM *)inBuf;
    
    if((NULL == inBuf) || (NULL == outBuf))
    {
        printf("Dem_GetChan NULL point inBuf %p outBuf %p\n",inBuf,outBuf);
        return DEMUX_S_NULL_POINTER_ERR;
    }

    muxHdl = demCalcPrm->demHdl;
    if(MAX_DEMUX_NUM <= muxHdl)
    {
        printf("Dem_GetChan muxHdl %d \n",muxHdl);
        return DEMUX_S_NO_CHAN_ERR;
    }

	pdemuxChn = demChnInfo + muxHdl;
   	if (pdemuxChn->demuxType >= DEM_NONE)
	{
	    printf("Dem_CalcData chan:%d demuxType %d error\n", muxHdl, pdemuxChn->demuxType);
        return -1;
	}

    pDemChn  = &demChnInfo[muxHdl].demChnInfo[pdemuxChn->demuxType];
    pthread_mutex_lock(&pDemChn->demlock);
    pDemChn->orgWriteIdx = demCalcPrm->orgWIdx;

    /*通过读写位置和包长计算当前包长度*/
    pDemChn->orgDataLen  = MEMCALC(pDemChn->orgWriteIdx,pDemChn->orgReadIdx,pDemChn->orgBufLen);
    *((unsigned int*)outBuf) = pDemChn->orgDataLen;
    pthread_mutex_unlock(&pDemChn->demlock);

    return    DEMUX_S_OK;
}

static int Dem_Process(void *inBuf,void *outBuf)
{
    int  muxHdl = 0;
	int     ret = 0; 
    DEMUX_CHN_INFO_ST* pDemChn = NULL;
    DEM_PRCO_OUT*  pDemProcOut = NULL;
    DEMUX_INFO_ST       *pdemuxChn = NULL;
	HIK_STREAM_PARSE  *pStreamParse = NULL;

    if((NULL == inBuf) || (NULL == outBuf))
    {
        printf("Dem_Process NULL point inBuf %p outBuf %p\n",inBuf,outBuf);
        return DEMUX_S_NULL_POINTER_ERR;
    }

    muxHdl = *((int *)inBuf);
    if(MAX_DEMUX_NUM <= muxHdl)
    {
        printf("Dem_Process muxHdl %d \n",muxHdl);
        return DEMUX_S_NO_CHAN_ERR;
    }

	pdemuxChn = demChnInfo + muxHdl;
   	if (pdemuxChn->demuxType >= DEM_NONE)
	{
	    printf("Dem_Process chan:%d demuxType %d error\n", muxHdl, pdemuxChn->demuxType);
        return -1;
	}

    pDemChn  = &demChnInfo[muxHdl].demChnInfo[pdemuxChn->demuxType];

    pthread_mutex_lock(&pDemChn->demlock);
    pDemProcOut = (DEM_PRCO_OUT*)outBuf;
    if(DEM_RTP == pDemChn->demuxType)
    {
    	//SAL_LOGW("========== rtp unpack enter! chan %d \n", muxHdl);
        ret = RtpUnpack(muxHdl);
		if (ret < 0)
		{
		    pthread_mutex_unlock(&pDemChn->demlock);
		    return -1;
		}
		
        pDemProcOut->orgRIdx   = pDemChn->orgReadIdx;
        pDemProcOut->newFrm    = pDemChn->frmInBuf;
        pDemProcOut->timestamp = pDemChn->timestamp;
        pDemProcOut->frmType   = STREAM_FRAME_TYPE_UNDEF;
		//printf("Dem_Process frmType %x,frmInBuf %d\n",pDemChn->frmType,pDemChn->frmInBuf);
        if(FRAME_TYPE_VIDEO_IFRAME == pDemChn->frmType || FRAME_TYPE_VIDEO_PFRAME == pDemChn->frmType)
        {
            //printf("Dem_Process newFrm %x vOut %x\n",pDemProcOut->newFrm,pDemChn->vOut);
            pDemProcOut->frmType   = STREAM_FRAME_TYPE_VIDEO;
            if(pDemProcOut->newFrm && (0 == pDemChn->vOut))
            {
                pDemProcOut->newFrm = 0;
            }
        }
        else if(FRAME_TYPE_AUDIO_FRAME == pDemChn->frmType)
        {
            pDemProcOut->frmType   = STREAM_FRAME_TYPE_AUDIO;
            if(pDemProcOut->newFrm && (0 == pDemChn->aOut))
            {
                pDemProcOut->newFrm = 0;
            }
        }
		/* 增加metadata解析帧类型，当前metadata数据传输仅使用rtp封装格式 */
		else if (RTP_PAYLOAD_TYPE_META_DATA == pDemChn->frmType)
		{
			//printf("======== proc out metadata \n");
			pDemProcOut->frmType = STREAM_FRAME_TYPE_METADATA;
		}
		else
		{
			{
				pStreamParse = (HIK_STREAM_PARSE *)pDemChn->pOutBuf;
				if (pStreamParse != NULL && pStreamParse->bufUsed == 1)
				{
					pStreamParse->bufUsed = 0;
				}
			}
		}
        
        if(pDemChn->frmInBuf)
        {
            pDemChn->frmInBuf  = 0;
            pDemChn->outWIdx   = 0;
            pDemChn->frmType   = FRAME_TYPE_UNDEF;
        }
    }
    else if(DEM_PS == pDemChn->demuxType)
    {
        PsUnpack(muxHdl);
        pDemProcOut->orgRIdx   = pDemChn->orgReadIdx;
        pDemProcOut->newFrm    = pDemChn->frmInBuf;
        pDemProcOut->timestamp = pDemChn->timestamp;
        pDemProcOut->frmType   = STREAM_FRAME_TYPE_UNDEF;
        if(FRAME_TYPE_VIDEO_IFRAME == pDemChn->frmType || FRAME_TYPE_VIDEO_PFRAME == pDemChn->frmType)
        {
            pDemProcOut->frmType   = STREAM_FRAME_TYPE_VIDEO;
            if(pDemProcOut->newFrm && (0 == pDemChn->vOut))
            {
                pDemProcOut->newFrm = 0;
            }
        }
        else if(FRAME_TYPE_AUDIO_FRAME == pDemChn->frmType)
        {
            pDemProcOut->frmType   = STREAM_FRAME_TYPE_AUDIO;
            if(pDemProcOut->newFrm && (0 == pDemChn->aOut))
            {
                pDemProcOut->newFrm = 0;
            }
        }        
        else if(FRAME_TYPE_PRIVT_FRAME == pDemChn->frmType)
        {
            pDemProcOut->frmType   = STREAM_FRAME_TYPE_PRIVT;
            if(pDemProcOut->newFrm && (0 == pDemChn->vOut))
            {
                pDemProcOut->newFrm = 0;
            }
        }        
    }
    else
    {
    }
    pthread_mutex_unlock(&pDemChn->demlock);
    return    DEMUX_S_OK;
}

static int Dem_GetChan(void *inBuf,void *outBuf)
{
    //int       nRet = -1,
	int i = 0;
    unsigned int memSize   = 0;
    DEM_CHN_PRM *demChnPrm = NULL;
    //DEM_TYPE* type;
    DEM_PARAM         *pDemPrm   = NULL;
    DEMUX_CHN_INFO_ST* pDemChn   = NULL;
	unsigned int       demuxType = 0;

    if((NULL == inBuf) || (NULL == outBuf))
    {
        printf("Dem_GetChan NULL point inBuf %p outBuf %p\n",inBuf,outBuf);
        return DEMUX_S_NULL_POINTER_ERR;
    }
    
    demChnPrm = (DEM_CHN_PRM *)inBuf;
    pDemPrm   = outBuf;

    if((DEM_RTP != demChnPrm->demType) && (DEM_PS != demChnPrm->demType))
    {
        printf("demux type err %d \n",demChnPrm->demType);
        return DEMUX_S_TYPE_ERR;
    }

    //printf("demux type %d --- 0 DEM_RTP,1 DEM_PS \n",demChnPrm->demType);

    for(i = 0;i < MAX_DEMUX_NUM;i++)
    {
		demuxType = demChnPrm->demType;
		pDemChn  = &demChnInfo[i].demChnInfo[demuxType];
        if(0 == pDemChn->bUsed)
        {
            demChnInfo[i].demuxType = demChnPrm->demType;
            pDemChn->bUsed     = 1;
            pDemChn->demuxType = demChnPrm->demType;
            pDemChn->aOut      = ((demChnPrm->outType & DEM_AUDIO_OUT) ? 1 : 0);
            pDemChn->vOut      = ((demChnPrm->outType & DEM_VIDEO_OUT) ? 1 : 0);
            Dem_MemSize(i,0,&memSize);
            pDemPrm->demHdl    = i;
            pDemPrm->bufLen    = memSize;
            pDemPrm->bufAddr   = NULL;
            break;
        }
    }

    if(MAX_DEMUX_NUM <= i)
    {
        return DEMUX_S_NO_CHAN_ERR;
    }
        
    return DEMUX_S_OK;
}

static int Dem_SetChnDmuxType(void *inBuf,void *outBuf)
{
    int                demIdx;
	unsigned int       demuxType;
	
    if((NULL == inBuf) || (NULL == outBuf))
    {
        printf("Dem_GetChan NULL point inBuf %p outBuf %p\n",inBuf,outBuf);
        return DEMUX_S_NULL_POINTER_ERR;
    }

	demIdx  = *((int *)inBuf);
    if(MAX_DEMUX_NUM <= demIdx)
    {
        printf("Dem_SetOrgBuf demux err %d \n",demIdx);
        return DEMUX_S_CHAN_ERR;
    }
	demuxType = *((unsigned int *)outBuf);

	if (demuxType >= DEM_NONE)
	{
	    printf("Dem_SetChnDmuxType chan:%d demuxType %d error\n", demIdx, demuxType);
        return -1;
	}

	demChnInfo[demIdx].demuxType = demuxType;

	return DEMUX_S_OK;
}

static int Dem_SetChnEncType(void *inBuf,void *outBuf)
{
    int                demIdx;
	unsigned int       encType;
	unsigned int       type;
	unsigned int       demuxType;
	
    if((NULL == inBuf) || (NULL == outBuf))
    {
        printf("Dem_GetChan NULL point inBuf %p outBuf %p\n",inBuf,outBuf);
        return DEMUX_S_NULL_POINTER_ERR;
    }

	demIdx  = *((int *)inBuf);
    if(MAX_DEMUX_NUM <= demIdx)
    {
        printf("Dem_SetOrgBuf demux err %d \n",demIdx);
        return DEMUX_S_CHAN_ERR;
    }
	encType = *((unsigned int *)outBuf);

	if (encType == 1)
	{
	    type = STREAM_TYPE_VIDEO_H264;
	}
	else if (encType == 2)
	{
	    type = STREAM_TYPE_VIDEO_H265;
	}
	else
	{
	    type = STREAM_TYPE_UNDEF;
		printf("chn %d,will parse enc type\n",demIdx);
	}

	demuxType = demChnInfo[demIdx].demuxType;

	demChnInfo[demIdx].demChnInfo[demuxType].videoStreamType = type;

	return DEMUX_S_OK;
}



static int Dem_Create(void *inBuf,void *outBuf)
{
    int                 nRet   = -1;
    DEM_PARAM         *pDemPrm = NULL;
    DEMUX_CHN_INFO_ST* pDemChn = NULL;
	DEMUX_INFO_ST     *pdemuxChn = NULL;

    pDemPrm = (DEM_PARAM*)inBuf;
    if(NULL == pDemPrm)
    {
        printf("pDemPrm %p not exsit!\n",pDemPrm);
        return DEMUX_S_NULL_POINTER_ERR;
    }
    
    if(MAX_DEMUX_NUM <= pDemPrm->demHdl)
    {
        printf("demHdl %d not exsit!\n",pDemPrm->demHdl);
        return DEMUX_S_CHAN_ERR;
    }

	pdemuxChn = demChnInfo + pDemPrm->demHdl;
	pDemChn  = &demChnInfo[pDemPrm->demHdl].demChnInfo[pdemuxChn->demuxType];
    pDemChn->memLen  = pDemPrm->bufLen;
    pDemChn->buffer  = pDemPrm->bufAddr;
    pDemChn->memUsed = 0;

    if(DEM_RTP == pDemChn->demuxType)
    {
        nRet = RtpDemCreate(pDemPrm->demHdl);
    }
    else if(DEM_PS == pDemChn->demuxType)
    {
        nRet = PsDemCreate(pDemPrm->demHdl);
    }
    else
    {
    }

    if(DEMUX_S_OK == nRet)
    {
        pthread_mutex_init(&pDemChn->demlock, NULL);
    }

    return nRet;
}

int DemuxControl(DEM_IDX idx,void *inBuf,void *outBuf)
{
    int                 nRet      = -1;
//    int                 chan      = 0;
//    DEMUX_CHN_INFO_ST *pRtpDem    = demChnInfo + chan;
//    DEM_ORGBUF_PRM    *pDemOrgBuf = 0;
//    if(MAX_DEMUX_NUM <= chan)
//    {
//        printf("chan %d,MAX_RTPDEM_NUM %d\n",chan,MAX_DEMUX_NUM);
//        return nRet;
//    }

    switch(idx)
    {
        case DEM_SET_ORGBUF:
            nRet = Dem_SetOrgBuf(inBuf,outBuf);
            break;
         
        case DEM_CALC_DATA:
            nRet = Dem_CalcData(inBuf,outBuf);
            break;

        case DEM_PROC_DATA:
            nRet = Dem_Process(inBuf,outBuf);
            break;

        case DEM_RESET:
            nRet = Dem_Reset(inBuf,outBuf);
            break;

        case DEM_FULL_BUF:
            nRet = Dem_FullBuf(inBuf,outBuf);
            break;

        case DEM_GET_HANDLE:
            nRet = Dem_GetChan(inBuf,outBuf);
            break;

        case DEM_CREATE:
            nRet = Dem_Create(inBuf,outBuf);
            break;
			
		case DEM_SET_TYPE:
            nRet = Dem_SetChnDmuxType(inBuf,outBuf);
            break;

		case DEM_SET_ENC_TYPE:
            nRet = Dem_SetChnEncType(inBuf,outBuf);
            break;

        default:
            nRet = -1;
            break;
    }
    
    return nRet;
}

int DemuxInit()
{
    int        i = 0,j = 0;
    DEMUX_CHN_INFO_ST *pDemChn = 0;  
	DEMUX_INFO_ST     *pdemuxChn = NULL;

    for(i = 0;i < MAX_DEMUX_NUM;i++)
    {
        pdemuxChn = demChnInfo + i;
		pdemuxChn->demuxType = DEM_NONE;
		for (j = 0;j < DEM_NONE;j++)
		{
		    pDemChn = &demChnInfo[i].demChnInfo[j];
            pDemChn->bUsed     = 0;
            pDemChn->demuxType = DEM_NONE;
		}
    }

    return DEMUX_S_OK;
}

void DemuxSetDbLeave(int level,int unLevel)
{
     demDbLevel = (level > 0) ? level : 0;
	 SAL_INFO("demDbLevel %d\n",demDbLevel);
	 RtpUnpack_SetDbLeave(unLevel);
}


