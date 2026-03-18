#include "libmux.h"
#include "descriptor_set.h"
#include "type_dscrpt_common.h"
#include "RTPPackLib.h"
#include "PSMuxLib.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "stream_bits_info_def.h"
#include "dspcommon.h"
#include "libdemux.h"
#include <sal.h>

#define MAX_STREAM_PACK_CHN             (20 + MAX_VIPC_CHAN * 2) //二代安检仪 vi0 (3 ps + 2 rtp) + vi1 (3 ps + 2 rtp)*2,乘2是还有音频,9个转包 * 2是一个通道同时支持ps和rtp打包
#define MAX_RTP_SIZE                    (1400)

typedef struct tagMemForPackInfoSt
{
    unsigned char* bufaddr;
    unsigned int   buflen;
    unsigned int   bufUsed;

    unsigned char* outputBuf;       /* 存放打包后的数据 */
    unsigned int   uiOutBufferSize; /* 缓冲区的大小     */    
} MEM_FOR_PACK_ST;

/* 需要的内存的信息 */
typedef struct tagTimeStampInfoSt
{
    unsigned int       u32time_stamp_bak;
    unsigned long long u64PTS_bak;
    unsigned int       clock_8K_bak;
    unsigned int       u32TimeStamp_bak;
} TIME_STAMP_INFO;

/* 每个通道的信息 */
typedef struct
{
    int             bUsed;
    int             bCreated;
    MUX_TYPE        muxType;
    
    MEM_FOR_PACK_ST stMemForPack;   /* 打包过程中使用的内存信息 */
    void            *handle;         /* 打包handle         */
    RTPPACK_PARAM   stRtpPackPrm;   /* 码流信息，打包需要 */
    PSMUX_PARAM     stPsPackPrm;

    int             audMux;         /*PS封装里面AUD打包*/
    unsigned int    uiVideoCnt;
    unsigned int    uiAudioCnt;
    unsigned int    largestDist;    /*最大的时标间隔*/
    TIME_STAMP_INFO stVideoTimeStampInfo;
    TIME_STAMP_INFO stAudioTimeStampInfo;
} MUX_PACK_CHN_INFO_ST;

typedef struct
{
    short year;             /* 年 */
    short month;            /* 月 */
    short dayOfWeek;        /* 0:星期日 -6:星期六 */
    short day;              /* 日 */
    short hour;             /* 时 */
    short minute;           /* 分 */
    short second;           /* 秒 */
    short milliSecond;      /* 毫秒 */
} MUX_DATE_TIME;

static MUX_PACK_CHN_INFO_ST muxChnInfo[MAX_STREAM_PACK_CHN];

static int MUX_Rtp_MakeHeader(char *pBuf, unsigned int rtpnum, unsigned int timestamp, unsigned int last, unsigned char nId)
{
    char *pTemp = pBuf;

    if(NULL == pBuf)
    {
        printf("MUX_Rtp_MakeHeader %p \n",pBuf);
        return MUX_S_NULL_POINTER_ERR;
    }

    pTemp[0] = (char)0x80;
    pTemp[1] = ((last & 0x1) << 7) | (nId & 0x7f);
    pTemp[2] = (rtpnum >> 8) & 0xff;
    pTemp[3] = rtpnum & 0xff;

    pTemp[4] = (timestamp >> 24) & 0xff;
    pTemp[5] = (timestamp >> 16) & 0xff;
    pTemp[6] = (timestamp >> 8) & 0xff;
    pTemp[7] = (timestamp) & 0xff;

    *(unsigned int *)(&pTemp[8]) = 0x88776655;

    return MUX_S_OK;
}

/**
 * @function:   MUX_Get_DateTime
 * @brief:      获取当地时间(带时区timeZone)
 * @param[in]:  MUX_DATE_TIME *pDT
 * @param[out]: None
 * @return:     static INT32
 */
static ATTRIBUTE_UNUSED int MUX_Get_DateTime(MUX_DATE_TIME *pDT)
{
    struct tm   tmpSysTime;
    struct tm   *p = &tmpSysTime;
    time_t      tp;

    /* 得到日历时间，从标准时间点到现在的秒数 */
    tp = time(NULL);

    /* 格式化日历时间 */
    localtime_r(&tp, p);

    pDT->year         = p->tm_year + 1900;
    pDT->month        = p->tm_mon  + 1;
    pDT->day          = p->tm_mday;
    pDT->hour         = p->tm_hour;
    pDT->minute       = p->tm_min;
    pDT->second       = p->tm_sec;
    pDT->dayOfWeek    = p->tm_wday;
    pDT->milliSecond  = 0;

    return MUX_S_OK;
}

static int MUX_Rtp_Fill_Info(HIKRTP_PACK_ES_INFO *info)
{
    int  i   = 0;
    
    if(NULL == info)
    {
        printf("MUX_Rtp_Fill_Info info %p\n",info);
        return MUX_S_NULL_POINTER_ERR;
    }

    HIK_VIDEO_INFO  *video_info = &info->stream_info.video_info;
    HIK_AUDIO_INFO  *audio_info = &info->stream_info.audio_info;

    info->stream_mode               = INCLUDE_VIDEO_STREAM
                                    | INCLUDE_AUDIO_STREAM
                                    | INCLUDE_PRIVT_STREAM;    /* 输入流模式       */
    info->video_stream_type         = STREAM_TYPE_VIDEO_H264;  /* 输入流类型       */
    info->audio_stream_type         = 0;                       /* 输入流类型       */
    //info->max_packet_len            = info->max_packet_len;            /* 最大传输单元长度 */
    info->stream_info.encrypt_type  = 0;                       /* 加密模式      */

    info->video_ssrc                = 10;
    info->audio_ssrc                = 20;
    info->privt_ssrc                = 30;

    info->SSRC = 0x55667788;
    info->video_payload_type        = HIK_PAYLOAD_TYPE_ALL_VIDEO;     /* RTP包头负载类型  */
    info->audio_payload_type        = 0;
    info->allow_ext                 = 1;            /* 允许包含扩展头   */
    info->allow_4byte_align         = 1;

    video_info->encoder_version     = 0;    /* 编码器版本   */
    video_info->encoder_year        = 2008; /* 编码器日期 年 */
    video_info->encoder_month       = 0;    /* 编码器日期 月 */
    video_info->encoder_date        = 0;    /* 编码器日期 日 */

    video_info->width_orig          = 0;    /* 编码图像宽高，16的倍数*/
    video_info->height_orig         = 0;
    video_info->interlace           = 0;    /* 是否场编码   */
    video_info->b_frame_num         = 0;    /* b帧个数      */
    video_info->is_svc_stream       = 1;    /* 0表示为svc码流，1表示不是svc码流 */
    video_info->use_e_frame         = 0;    /* 是否使用e帧  */
    video_info->max_ref_num         = 1;    /* 最大参考帧个数 */
    video_info->fixed_frame_rate    = 0;    /* 是否固定帧率 */
    video_info->time_info           = 3600; /* 以1/90000 秒为单位的两帧间时间间隔 */
    video_info->watermark_type      = 0;    /* 水印类型 */
    video_info->deinterlace         = 0;    //注意：显示时需要反隔行处理则置0，不需要置1
    video_info->play_clip           = 0;    /* 是否需要显示裁剪，0为否，1为是 */
    video_info->start_pos_x         = 0;    /* 裁剪后显示区域左上角点在原区域的坐标x */
    video_info->start_pos_y         = 0;    /* 裁剪后显示区域左上角点在原区域的坐标y */
    video_info->width_play          = 0;    /* 裁剪后显示区域的宽度 */
    video_info->height_play         = 0;    /* 裁剪后显示区域的高度 */
    audio_info->frame_len           = 0;    /* 音频帧长度 */
    audio_info->audio_num           = 1;    /* 音频声道数 0 单声道，1 双声道*/
    audio_info->sample_rate         = 8000;     /* 音频采样率 */
    audio_info->bit_rate            = 64000;    /* 音频码率 */

    for (i = 0; i < 16; i++)
    {
        info->stream_info.dev_chan_id[i] = i;   /* 设备和通道id */
    }

    info->stream_info.is_hik_stream = 1;
    return MUX_S_OK;
}


static int MUX_Ps_Fill_Info(HIKPSMX_ES_INFO *info)
{
    int i = 0;

    info->stream_mode    = INCLUDE_VIDEO_STREAM | INCLUDE_AUDIO_STREAM | INCLUDE_PRIVT_STREAM;    /* 输入流模式*/
    info->max_byte_rate  = 800000;	            /* 码率，以byte为单位*/
    info->max_packet_len = 6000;

    info->stream_info.encrypt_type  = 0;       /* 加密模式 */
    info->stream_info.is_hik_stream = 1;
    for (i = 0; i < 16; i++)
    {
        info->stream_info.dev_chan_id[i] = i;
    }

    info->video_stream_type = STREAM_TYPE_VIDEO_H264;  /* 输入视频流类型 */
    info->audio_stream_type = STREAM_TYPE_AUDIO_G711A;  /* 输入音频流类型 */
    info->privt_stream_type = STREAM_TYPE_HIK_PRIVT;	/* 输入私有流类型 */

    //设置descriptor集
    info->dscrpt_sets = INCLUDE_BASIC_DESCRIPTOR|INCLUDE_TIMING_HRD_DESCRIPTOR|INCLUDE_DEVICE_DESCRIPTOR|INCLUDE_VIDEO_DESCRIPTOR|INCLUDE_VIDEO_CLIP_DESCRIPTOR;

    info->bframe_audio_set_psh                     = 1;
    info->set_frame_end_flg                        = 1;

    info->stream_info.video_info.encoder_version   = 123;
    info->stream_info.video_info.encoder_year      = 2007;
    info->stream_info.video_info.encoder_month     = 4;
    info->stream_info.video_info.encoder_date      = 13;

    info->stream_info.video_info.width_orig        = 0;
    info->stream_info.video_info.height_orig       = 0;
    info->stream_info.video_info.width_play        = 0;
    info->stream_info.video_info.height_play       = 0;

    info->stream_info.video_info.interlace         = 0;
    info->stream_info.video_info.b_frame_num       = 0;
    info->stream_info.video_info.is_svc_stream     = 1; ///< 0表示为svc码流，1表示不是svc码流。
    info->stream_info.video_info.use_e_frame       = 0;
    info->stream_info.video_info.max_ref_num       = 1;
    info->stream_info.video_info.fixed_frame_rate  = 1;
    info->stream_info.video_info.time_info         = 3600;
    info->stream_info.video_info.watermark_type    = 0;
    info->stream_info.video_info.deinterlace       = 0; ///< 注意：显示时需要反隔行处理则置0，不需要置1
    info->stream_info.video_info.play_clip         = 0;
    info->stream_info.video_info.start_pos_x       = 0;
    info->stream_info.video_info.start_pos_y       = 0;
    //info->stream_info.video_info.min_search_blk    = 0;
    //info->stream_info.video_info.light_storage     = 0;

    info->stream_info.audio_info.frame_len         = 144;
    info->stream_info.audio_info.audio_num         = 1;
    info->stream_info.audio_info.sample_rate       = 16000;
    info->stream_info.audio_info.bit_rate          = 16000;
    return MUX_S_OK;
}

int MUX_Rtp_Pack(int chan,MUX_DATA_INFO_S *pstStreamDataInfo)
{
    //int                           nalu_len       = 0 ;
    unsigned int                  sys_clk        = 11;
    int                           ret = 0,   pos = 0;
    unsigned char               * pTemp          = NULL;
    unsigned int                  pslen          = 0;
    unsigned int                  clock_8K       = 0;

    MUX_PACK_CHN_INFO_ST        * pstChnInfo     = NULL;
    HIKRTP_PACK_ES_INFO         * info           = NULL;
    RTPPACK_PROCESS_PARAM         unit_param;
    void                        * handle         = NULL;
    MUX_IN_BITS_INTO_S          * pstInDataBuff  = NULL;
    MUX_OUT_BITS_INTO_S        * pstOutDataBuff  = NULL;
    MUX_DATE_TIME                 sysTime;
    int                          i = 0,  rtpCnt  = 0;
    int                          rtpLen,sendLen  = 0;
    int                          sliceidx;
    unsigned char             audio_payload_type = 0x0;
	unsigned char            	*pOut;

    if(NULL == pstStreamDataInfo)
    {
        printf("MUX_Rtp_Pack prm is NULL !!!\n");
        return MUX_S_NULL_POINTER_ERR;
    }

    pstChnInfo     = muxChnInfo + chan;
    info           = &pstChnInfo->stRtpPackPrm.info;
    handle         = pstChnInfo->handle;

    pstInDataBuff  = &pstStreamDataInfo->stInBuffer;
    pstOutDataBuff = &pstStreamDataInfo->stOutBuffer;

    memset(&unit_param, 0, sizeof(unit_param));
    //MUX_Get_DateTime(&sysTime);
    /* 之前接口是获取时区时间, 修改为支持夏令时的函数接口 */
    SAL_getDateTime_DST((DATE_TIME *) &sysTime);
    unit_param.rtp_out_buf_size    = pstOutDataBuff->bufferLen;
    unit_param.global_time.year    = sysTime.year;
    unit_param.global_time.month   = sysTime.month;
    unit_param.global_time.date    = sysTime.day;
    unit_param.global_time.hour    = sysTime.hour;
    unit_param.global_time.minute  = sysTime.minute;
    unit_param.global_time.second  = sysTime.second;
    unit_param.global_time.msecond = sysTime.milliSecond;

    unit_param.company_mark = CO_TYPE_HK;
    unit_param.camera_mark  = CAMERA_TYPE_UNDEF;

    if((0 == pstChnInfo->stAudioTimeStampInfo.u64PTS_bak) && (0 == pstChnInfo->stVideoTimeStampInfo.u64PTS_bak))
    {
        pstChnInfo->stAudioTimeStampInfo.u64PTS_bak = pstInDataBuff->u64PTS;
        pstChnInfo->stVideoTimeStampInfo.u64PTS_bak = pstInDataBuff->u64PTS;
    }    

    if (pstInDataBuff->bVideo)
    {
        unit_param.is_key_frame   = pstInDataBuff->is_key_frame;
        //unit_param.frame_type     = unit_param.is_key_frame ? FRAME_TYPE_VIDEO_IFRAME : FRAME_TYPE_VIDEO_PFRAME;

        if(pstInDataBuff->bufferLen[0] <= 0)
        {
            return MUX_S_PROC_NO_DATA;
        }

        /* 更新信息 */
        if(  (info->stream_info.video_info.width_orig  != pstInDataBuff->width)
          || (info->stream_info.video_info.height_orig != pstInDataBuff->height)
          || ((pstInDataBuff->fps > 0) && (info->stream_info.video_info.time_info != (90000 / pstInDataBuff->fps))))
        {
            info->stream_info.video_info.width_orig  = pstInDataBuff->width;
            info->stream_info.video_info.width_play  = pstInDataBuff->width;
            info->stream_info.video_info.height_orig = pstInDataBuff->height;
            info->stream_info.video_info.height_play = pstInDataBuff->height;
            info->stream_info.video_info.time_info = (90000 / pstInDataBuff->fps);
            RTPPACK_ResetStreamInfo(handle,info);
            pstChnInfo->uiVideoCnt  = 0;
            //pstChnInfo->uiAudioCnt  = 0;
            pstChnInfo->largestDist = 0;
        }

        if(pstChnInfo->largestDist < pstInDataBuff->u64PTS - pstChnInfo->stVideoTimeStampInfo.u64PTS_bak)
        {
            pstChnInfo->largestDist = pstInDataBuff->u64PTS - pstChnInfo->stVideoTimeStampInfo.u64PTS_bak;
        }
        
        sys_clk = pstChnInfo->stVideoTimeStampInfo.u32time_stamp_bak + (unsigned int)((pstInDataBuff->u64PTS - pstChnInfo->stVideoTimeStampInfo.u64PTS_bak) * 90);       
        unit_param.time_stamp = sys_clk;
        unit_param.frame_num  = pstChnInfo->uiVideoCnt;
        pstChnInfo->uiVideoCnt++;

        pOut = pstOutDataBuff->bufferAddr; 
        for(i = 0;i < pstInDataBuff->naluNum;i++)
        {
            unit_param.frame_type    = pstInDataBuff->frame_type;
            unit_param.is_first_unit = (i == 0);
            unit_param.is_last_unit  = (i == pstInDataBuff->naluNum -1);
            sliceidx = 0;//sliceidx = ((3 <= pstInDataBuff->naluNum && 2 <= i) ? (i -2) : 0);
            
             /* h265 I 帧包个数不保险增加vps指针判断 */
			if ((pstInDataBuff->naluNum >= 4) && (NULL != pstInDataBuff->vpsBufferAddr))
			{
			    if (3 <= i)
			    {
			        sliceidx = (i - 3);
			    }
					
			}
			/* h264 I 帧 */
			else if ((pstInDataBuff->naluNum >=3) && (NULL != pstInDataBuff->spsBufferAddr))
			{
			    if (2 <= i)
			    {
			        sliceidx = (i - 2);
			    }
			}
			
            unit_param.unit_in_buf   = pstInDataBuff->bufferAddr[sliceidx];
            unit_param.unit_in_len   = pstInDataBuff->bufferLen[sliceidx];
			#if 0 
            if((3 <= pstInDataBuff->naluNum) && ((0 == i) || (1 == i)))
            {
                unit_param.unit_in_buf  = ((0 == i) ? pstInDataBuff->spsBufferAddr : pstInDataBuff->ppsBufferAddr);
                unit_param.unit_in_len  = ((0 == i) ? pstInDataBuff->spsLen : pstInDataBuff->ppsLen);
            }
			#endif
            if ((pstInDataBuff->naluNum >= 3) && (i < 3))
		    {
		        //h265 i帧
		        if (NULL != pstInDataBuff->vpsBufferAddr)
		        {
		            if (0 == i)
		            {
		                unit_param.unit_in_buf  =  pstInDataBuff->vpsBufferAddr;
                        unit_param.unit_in_len  =  pstInDataBuff->vpsLen;
		            }
					else if (1 ==i)
					{
					    unit_param.unit_in_buf  =  pstInDataBuff->spsBufferAddr;
                        unit_param.unit_in_len  =  pstInDataBuff->spsLen;
					}
					else if (2 == i)
					{
					    unit_param.unit_in_buf  =  pstInDataBuff->ppsBufferAddr;
                        unit_param.unit_in_len  =  pstInDataBuff->ppsLen;
					}	
		        }
				else if (i < 2) //h264 i帧
				{
				    if (0 == i)
		            {
		                unit_param.unit_in_buf  =  pstInDataBuff->spsBufferAddr;
                        unit_param.unit_in_len  =  pstInDataBuff->spsLen;
		            }
					else if (1 == i)
					{
					    unit_param.unit_in_buf  =  pstInDataBuff->ppsBufferAddr;
                        unit_param.unit_in_len  =  pstInDataBuff->ppsLen; 
					}
				}
		    }
            unit_param.rtp_out_buf   = pOut;
            unit_param.is_unit_start = 1;
            unit_param.is_unit_end   = 1;
            unit_param.frame_num     = 1;
            unit_param.nalu_tab      = 0;
            unit_param.key_len       = 0;
            unit_param.packet_type   = 0;

            ret = RTPPACK_Process(handle, &unit_param);
            if (HIK_RTPPACK_LIB_S_OK != ret)
            {
                printf("Video RTP Mux Process Failed, %x %d!!!\n", ret,chan);
                return -1;
            }

            pos = 0;
            for (rtpCnt = 0; rtpCnt < unit_param.rtp_num; rtpCnt++)
            {
                pTemp    = pOut + pos;
                rtpLen   = (pTemp[2] << 8) + pTemp[3];
                sendLen  = rtpLen + ((rtpLen & 3) ? (4 - (rtpLen & 3)) : 4);
                pTemp[0] = 0x3; 
                pTemp[1] = 0x0;
                pTemp[2] = ((rtpLen+4) & 0xff);
                pTemp[3] = (((rtpLen+4) >> 8) & 0xff);
                pslen += sendLen;
                pos   += sendLen;
            }
            pOut = pstOutDataBuff->bufferAddr + pslen;
        }
        
        pstOutDataBuff->streamLen = pslen;
        pstChnInfo->stVideoTimeStampInfo.u64PTS_bak        = pstInDataBuff->u64PTS;
        pstChnInfo->stVideoTimeStampInfo.u32time_stamp_bak = unit_param.time_stamp;
    }
    else //  处理音频
    {   
        if(FRAME_TYPE_PRIVT_FRAME == pstInDataBuff->frame_type)
        {
            unit_param.is_key_frame   = 0;
            //unit_param.frame_type     = unit_param.is_key_frame ? FRAME_TYPE_VIDEO_IFRAME : FRAME_TYPE_VIDEO_PFRAME;

            if(pstInDataBuff->bufferLen[0] <= 0)
            {
                return MUX_S_PROC_NO_DATA;
            }

            
            if(pstChnInfo->largestDist < pstInDataBuff->u64PTS - pstChnInfo->stVideoTimeStampInfo.u64PTS_bak)
            {
                pstChnInfo->largestDist = pstInDataBuff->u64PTS - pstChnInfo->stVideoTimeStampInfo.u64PTS_bak;
            }
            
            sys_clk = pstChnInfo->stVideoTimeStampInfo.u32time_stamp_bak + (unsigned int)((pstInDataBuff->u64PTS - pstChnInfo->stVideoTimeStampInfo.u64PTS_bak) * 90);       
            unit_param.time_stamp = sys_clk;
            unit_param.frame_num  = pstChnInfo->uiVideoCnt;

            pOut = pstOutDataBuff->bufferAddr; 

            unit_param.unit_in_buf  =  pstInDataBuff->bufferAddr[0];
            unit_param.unit_in_len  =  pstInDataBuff->bufferLen[0];
            unit_param.frame_type    = pstInDataBuff->frame_type;
            unit_param.is_first_unit = 1;
            unit_param.is_last_unit  = 1;
          
			/* h264 I 帧 */
            unit_param.rtp_out_buf   = pOut;
            unit_param.is_unit_start = 1;
            unit_param.is_unit_end   = 1;
            unit_param.frame_num     = 1;
            unit_param.nalu_tab      = 0;
            unit_param.key_len       = 0;
            unit_param.packet_type   = 0;

            ret = RTPPACK_Process(handle, &unit_param);
            if (HIK_RTPPACK_LIB_S_OK != ret)
            {
                printf("Video RTP Mux Process Failed, %x %d!!!\n", ret,chan);
                return -1;
            }
            #if 0
            pTemp    = pOut;

            rtpLen   = (pTemp[2] << 8) + pTemp[3];
            
            pTemp[0] = 0x3; 
            pTemp[1] = 0x0;
            pTemp[2] = ((rtpLen+4) & 0xff);
            pTemp[3] = (((rtpLen+4) >> 8) & 0xff);
            rtpLen += 4;
            #else
            pos = 0;
            for (rtpCnt = 0; rtpCnt < unit_param.rtp_num; rtpCnt++)
            {
                pTemp    = pOut + pos;
                rtpLen   = (pTemp[2] << 8) + pTemp[3];
                sendLen  = rtpLen + ((rtpLen & 3) ? (4 - (rtpLen & 3)) : 4);
                pTemp[0] = 0x3; 
                pTemp[1] = 0x0;
                pTemp[2] = ((rtpLen+4) & 0xff);
                pTemp[3] = (((rtpLen+4) >> 8) & 0xff);
                pslen += sendLen;
                pos   += sendLen;
            }
            pOut = pstOutDataBuff->bufferAddr + pslen;
            #endif
            
            pstOutDataBuff->streamLen = pslen;
            pstChnInfo->stVideoTimeStampInfo.u64PTS_bak        = pstInDataBuff->u64PTS;
            pstChnInfo->stVideoTimeStampInfo.u32time_stamp_bak = unit_param.time_stamp;
        }
        else
        {
             /* 更新信息 */
	        if(  (info->stream_info.audio_info.audio_num  != pstInDataBuff->audChn)
	          || (info->stream_info.audio_info.sample_rate != pstInDataBuff->audSampleRate))
	        {
	            info->stream_info.audio_info.audio_num  = pstInDataBuff->audChn;
	            info->stream_info.audio_info.sample_rate = pstInDataBuff->audSampleRate;
	            RTPPACK_ResetStreamInfo(handle,info);
				printf("AUDIO audio_num %d,sample_rate %d\n",info->stream_info.audio_info.audio_num,info->stream_info.audio_info.sample_rate);
	            pstChnInfo->uiAudioCnt  = 0;
	        }
            if (pstInDataBuff->audEnctype == AUDIO_G711_U)
		    {
				audio_payload_type = HIK_PAYLOAD_TYPE_G711U_AUDIO;
		    }
			else  if (pstInDataBuff->audEnctype == AUDIO_G711_A)
			{
				audio_payload_type = HIK_PAYLOAD_TYPE_G711A_AUDIO;
			}
			else if (pstInDataBuff->audEnctype == AUDIO_AAC)
			{
				audio_payload_type = HIK_PAYLOAD_TYPE_AAC_AUDIO;
			}

		    if (pstInDataBuff->audEnctype == AUDIO_AAC)
		    {
		        if (pstInDataBuff->bufferLen[0] >= MAX_RTP_SIZE)
		        {
		           printf("len %d,\n",pstInDataBuff->bufferLen[0]);
		        }
				
				if (pstInDataBuff->audExInfo)
				{
				     pTemp = (unsigned char *)(pstOutDataBuff->bufferAddr + RTP_HEAD_LEN + 4 + 4);
				     memmove(pTemp, pstInDataBuff->bufferAddr[0]+7, pstInDataBuff->bufferLen[0]-7);
				     pslen = pstInDataBuff->bufferLen[0]-7;
				}
				else
				{
				     pTemp = (unsigned char *)(pstOutDataBuff->bufferAddr + RTP_HEAD_LEN + 4);
				     memmove(pTemp, pstInDataBuff->bufferAddr[0], pstInDataBuff->bufferLen[0]);
				     pslen = pstInDataBuff->bufferLen[0];
				}
		    }
			else
			{
			    pTemp = (unsigned char *)(pstOutDataBuff->bufferAddr + RTP_HEAD_LEN + 4);
			    memmove(pTemp, pstInDataBuff->bufferAddr[0], pstInDataBuff->bufferLen[0]);
				pslen = pstInDataBuff->bufferLen[0];
			}
            
            
            pTemp = (unsigned char *)(pstOutDataBuff->bufferAddr + 4);
            clock_8K = pstChnInfo->stAudioTimeStampInfo.clock_8K_bak + (unsigned int)((pstInDataBuff->u64PTS - pstChnInfo->stAudioTimeStampInfo.u64PTS_bak) * 8);
            pstChnInfo->stAudioTimeStampInfo.clock_8K_bak = clock_8K;
            pstChnInfo->uiAudioCnt++;
			
            MUX_Rtp_MakeHeader((char *)pTemp, pstChnInfo->uiAudioCnt, clock_8K, 1, audio_payload_type);
            pslen += RTP_HEAD_LEN;
            pTemp = (unsigned char *)(pstOutDataBuff->bufferAddr);
            pslen += 4;
			//aac 打包需要加上4个字节的信息，转包的时候不加
			if ((pstInDataBuff->audEnctype == AUDIO_AAC) && (pstInDataBuff->audExInfo))
			{
			    //AU_header_length
	            pTemp[RTP_HEAD_LEN + 4] = 0x00;
	            pTemp[RTP_HEAD_LEN + 4+1] = 0x10;  ///< 16bits AU_header_length

	            //AU_header
	            pTemp[RTP_HEAD_LEN + 4+2] = ((pstInDataBuff->bufferLen[0]-7)>> 5) & 0xff;
	            pTemp[RTP_HEAD_LEN + 4+3] = ((pstInDataBuff->bufferLen[0]-7) << 3) & 0xff;  ///< 13bits AU_length + 3bits AU_index
	            pslen += 4;
			}
            pTemp[0] = 0x3; 
            pTemp[1] = 0x0; 
            pTemp[2] = (pslen & 0xff);
            pTemp[3] = ((pslen >> 8) & 0xff);
            pstChnInfo->stAudioTimeStampInfo.u64PTS_bak = pstInDataBuff->u64PTS;
            pstOutDataBuff->streamLen = pslen;
        }
    }
    
    return MUX_S_OK;
}

static int MUX_Ps_Pack(int chan,MUX_DATA_INFO_S *pstStreamDataInfo)
{
    int                         i = 0;//, nalLen  = 0;
    unsigned int                sys_clk        = 11;
    int                       ret = 0, nalNum  = 0;
    MUX_IN_BITS_INTO_S        * pstInDataBuff  = NULL;
    MUX_OUT_BITS_INTO_S      * pstOutDataBuff  = NULL;
    MUX_PACK_CHN_INFO_ST       * pstChnInfo    = NULL;
    HIKPSMX_ES_INFO            * info          = NULL;
    TIME_STAMP_INFO            * pTSInfo       = NULL;
    void                       * handle        = NULL;
    int                          psLen         = 0;
    //int                          startNalu     = 0;
    PSMUX_PROCESS_PARAM          procParam = {0};
    MUX_DATE_TIME                sysTime;
    int                          sliceidx;
    int                          sliceStart    = 0;
    unsigned char              iFrameAUD[8]    = {0x0,0x0,0x0,0x1,0x09,0x10,0x0,0x0 };
    unsigned char              pFrameAUD[8]    = {0x0,0x0,0x0,0x1,0x09,0x30,0x0,0x0 }; 
	//unsigned char              iFrameAUD_265[8] = {0x0,0x0,0x0,0x1,0x46,0x00,0x0,0x0 };
    //unsigned char              pFrameAUD_265[8] = {0x0,0x0,0x0,0x1,0x46,0x20,0x0,0x0 }; 
	//unsigned char              isH265 = 0;
	int                        audMux = 0;         /*PS封装里面AUD打包*/

    if(NULL == pstStreamDataInfo)
    {
        printf("MUX_Ps_Pack prm is NULL !!!\n");
        return MUX_S_NULL_POINTER_ERR;
    }

    pstChnInfo     = muxChnInfo + chan;
    info           = &pstChnInfo->stPsPackPrm.info;
    handle         = pstChnInfo->handle;
    pstInDataBuff  = &pstStreamDataInfo->stInBuffer;
    pstOutDataBuff = &pstStreamDataInfo->stOutBuffer;
    
    if(info->stream_info.video_info.width_orig  != pstInDataBuff->width)
    {
        info->stream_info.video_info.width_orig  = pstInDataBuff->width;
        info->stream_info.video_info.width_play  = pstInDataBuff->width;
    }
    if(info->stream_info.video_info.height_orig != pstInDataBuff->height)
    {
        info->stream_info.video_info.height_orig = pstInDataBuff->height;
        info->stream_info.video_info.height_play = pstInDataBuff->height;
    }

	if ((pstInDataBuff->fps > 0) && (info->stream_info.video_info.time_info != (90000 / pstInDataBuff->fps)))
	{
	   info->stream_info.video_info.time_info = (90000 / pstInDataBuff->fps);
	   ///MUX_Ps_update_vfps(handle,pstInDataBuff->fps);
	   printf("update time_info %u\n",info->stream_info.video_info.time_info);
	   MUX_Ps_update_vdp(handle, info);
	}

    memset(&procParam, 0, sizeof(PSMUX_PROCESS_PARAM));
    
//  MUX_Get_DateTime(&sysTime);
    /* 之前接口是获取时区时间, 修改为支持夏令时的函数接口 */
    SAL_getDateTime_DST((DATE_TIME *)&sysTime);

    procParam.out_buf               = pstOutDataBuff->bufferAddr;
    procParam.out_buf_len           = 0;
    procParam.out_buf_size          = pstOutDataBuff->bufferLen;
    procParam.global_time.year      = sysTime.year;
    procParam.global_time.month     = sysTime.month;
    procParam.global_time.date      = sysTime.day;
    procParam.global_time.hour      = sysTime.hour;
    procParam.global_time.minute    = sysTime.minute;
    procParam.global_time.second    = sysTime.second;
    procParam.global_time.msecond   = sysTime.milliSecond;
    procParam.frame_num             = 0;
    procParam.camera_mark           = CAMERA_TYPE_UNDEF;
    procParam.company_mark          = CO_TYPE_HK;

    if((0 == pstChnInfo->stAudioTimeStampInfo.u64PTS_bak) && (0 == pstChnInfo->stVideoTimeStampInfo.u64PTS_bak))
    {
        pstChnInfo->stAudioTimeStampInfo.u64PTS_bak = pstInDataBuff->u64PTS;
        pstChnInfo->stVideoTimeStampInfo.u64PTS_bak = pstInDataBuff->u64PTS;
    }

    if(pstInDataBuff->bufferLen[0] <= 0)
    {
        printf("ps pack need more data nalLen %d\n",pstInDataBuff->bufferLen[0]);
        return MUX_S_PROC_NO_DATA;
    }

    nalNum = pstInDataBuff->naluNum;
    procParam.is_key_frame   = pstInDataBuff->is_key_frame;
    if(pstInDataBuff->bVideo)
    {
        //procParam.frame_type   = (procParam.is_key_frame ? FRAME_TYPE_VIDEO_IFRAME : FRAME_TYPE_VIDEO_PFRAME);
        audMux = pstChnInfo->audMux;
        if ((STREAM_TYPE_H265_IFRAME == pstInDataBuff->frame_type)
		|| (STREAM_TYPE_H265_PFRAME == pstInDataBuff->frame_type))
		{
		    //isH265 = 1;对于支持265 aud的解码器可以开启该选项以及audMux
			audMux = 0; //测试好几款播放器不支持h265的aud(解码失败)，elecard streameye支持
		}

        procParam.frame_num    = pstChnInfo->uiVideoCnt;
        pTSInfo                = &pstChnInfo->stVideoTimeStampInfo;
        nalNum                += (audMux ? 1 : 0);
        sliceStart             = (procParam.is_key_frame ? (pstInDataBuff->naluNum - 1) : 0);
        sliceStart            += (nalNum - pstInDataBuff->naluNum);
        pstChnInfo->uiVideoCnt++;
    }
    else
    {
        if(FRAME_TYPE_PRIVT_FRAME == pstInDataBuff->frame_type)
        {
            nalNum = 1;
            procParam.frame_num    = pstChnInfo->uiVideoCnt;
            procParam.is_key_frame = 0;
            procParam.frame_type   = FRAME_TYPE_PRIVT_FRAME;
            pTSInfo                = &pstChnInfo->stVideoTimeStampInfo;
        }
        else
        {
            nalNum = 1;
            procParam.frame_num    = pstChnInfo->uiAudioCnt;
            procParam.is_key_frame = 0;
            //procParam.frame_type   = FRAME_TYPE_AUDIO_FRAME;
			procParam.frame_type   = pstInDataBuff->audEnctype;
            pTSInfo                = &pstChnInfo->stAudioTimeStampInfo;
			pstChnInfo->uiAudioCnt++;
			//printf("MUX PS chan %d,uiAudioCnt %d,bVideo %d\n",chan,pstChnInfo->uiAudioCnt,pstInDataBuff->bVideo);
        }
    }
	
    if ((pstInDataBuff->u64PTS <= pTSInfo->u64PTS_bak))
    {
//        printf("PTS PSMUX_Process curTime %llu,oldTime %llu\n",pstInDataBuff->u64PTS ,pTSInfo->u64PTS_bak);
//		printf("[y-m-d:%u-%u-%u],[h-minu-s-ms:%u-%u-%u-%u]\n"
//			,sysTime.year,sysTime.month,sysTime.day
//			,sysTime.hour,sysTime.minute,sysTime.second,sysTime.milliSecond);
    }
	
    sys_clk = pTSInfo->u32time_stamp_bak + (unsigned int)((pstInDataBuff->u64PTS - pTSInfo->u64PTS_bak) * 45);
	if (((sys_clk / 2) >= pTSInfo->u32time_stamp_bak) || (sys_clk < pTSInfo->u32time_stamp_bak))
	{
	    printf("TIME STAMP PSMUX_Process curTime %llu,oldTime %llu.Time[%u,%u]\n",pstInDataBuff->u64PTS ,pTSInfo->u64PTS_bak,sys_clk,pTSInfo->u32time_stamp_bak);
		printf("[y-m-d:%u-%u-%u],[h-minu-s-ms:%u-%u-%u-%u]\n"
			,sysTime.year,sysTime.month,sysTime.day
			,sysTime.hour,sysTime.minute,sysTime.second,sysTime.milliSecond);
	}
    procParam.sys_clk_ref  = sys_clk;
    procParam.ptime_stamp  = sys_clk;
    psLen                  = 0;
    //printf("%u\n",sys_clk);
    for(i = 0;i < nalNum; i++)
    {
        procParam.is_first_unit   = (0 == i ? 1 : 0);
        procParam.is_last_unit    = (i == (nalNum - 1) ? 1 : 0);            
        procParam.is_unit_start   = 1;
        procParam.is_unit_end     = 1;

        sliceidx                  = ((sliceStart <= i) ? (i - sliceStart) : 0);
        procParam.unit_in_buf     = pstInDataBuff->bufferAddr[sliceidx];
        procParam.unit_in_len     = pstInDataBuff->bufferLen[sliceidx];

        if(pstInDataBuff->bVideo)
        {
            procParam.frame_type   = pstInDataBuff->frame_type;
            if(audMux && (0 == i))
            {
                #if 0 //消除coverity
                if (isH265)
                {
                    procParam.unit_in_buf  = (procParam.is_key_frame ? iFrameAUD_265 : pFrameAUD_265);
                }
				else
				#endif
				{
				    procParam.unit_in_buf  = (procParam.is_key_frame ? iFrameAUD : pFrameAUD);
				}
                
                procParam.unit_in_len  = 8;
            }
            else if(procParam.is_key_frame && (sliceStart > i))
            {
                //procParam.unit_in_buf  = (((sliceStart - 2) == i) ? pstInDataBuff->spsBufferAddr : pstInDataBuff->ppsBufferAddr);
                //procParam.unit_in_len  = (((sliceStart - 2) == i) ? pstInDataBuff->spsLen : pstInDataBuff->ppsLen);
                if (NULL != pstInDataBuff->vpsBufferAddr)
		        {
		            /* 打包vps 的位置和有无音频有关*/
		            if ((0 + (sliceStart - (pstInDataBuff->naluNum - 1))) == i)
		            {
		                procParam.unit_in_buf  =  pstInDataBuff->vpsBufferAddr;
                        procParam.unit_in_len  =  pstInDataBuff->vpsLen;
		            }
					else if ((1 + (sliceStart - (pstInDataBuff->naluNum - 1))) ==i)
					{
					    procParam.unit_in_buf  =  pstInDataBuff->spsBufferAddr;
                        procParam.unit_in_len  =  pstInDataBuff->spsLen;
					}
					else if ((2 + (sliceStart - (pstInDataBuff->naluNum - 1))) == i)
					{
					    procParam.unit_in_buf  =  pstInDataBuff->ppsBufferAddr;
                        procParam.unit_in_len  =  pstInDataBuff->ppsLen;
					}	
		        }
				else
				{
				    /* 打包sps 的位置和有无音频有关*/
				    if ((0 + (sliceStart - (pstInDataBuff->naluNum - 1))) == i)
		            {
		                procParam.unit_in_buf  =  pstInDataBuff->spsBufferAddr;
                        procParam.unit_in_len  =  pstInDataBuff->spsLen;
		            }
					else if ((1 + (sliceStart - (pstInDataBuff->naluNum - 1))) ==i)
					{
					    procParam.unit_in_buf  =  pstInDataBuff->ppsBufferAddr;
                        procParam.unit_in_len  =  pstInDataBuff->ppsLen; 
					}
				}
            }
        }
        
        procParam.out_buf          = pstOutDataBuff->bufferAddr + psLen;
        procParam.out_buf_size     = pstOutDataBuff->bufferLen  - psLen;

        ret = PSMUX_Process(handle, &procParam);
        if(HIK_PSMUX_LIB_S_OK != ret)
        {
            printf("PSMUX_Process ret = %x\n", ret);
            return MUX_S_PROC_ERR;
        }
        psLen += procParam.out_buf_len;
    }

    pstOutDataBuff->streamLen   = psLen;
    pTSInfo->u64PTS_bak         = pstInDataBuff->u64PTS;
    pTSInfo->u32time_stamp_bak  = procParam.sys_clk_ref;
    
    return MUX_S_OK;
}


/**
 * @function   MUX_GetChan
 * @brief      获取转包所需要的管理通道(全局变量用于保存转包过程的码流信息)

 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
static int MUX_GetChan(void* inBuf,void* outBuf)
{
    int *pMuxType = NULL; 
    int nRet = MUX_S_FAIL,i = 0;
    MUX_PACK_CHN_INFO_ST *pPackPrm = NULL;
    int        *pMuxHdl = NULL;
    MUX_PARAM  *pMuxPrm = NULL;

    if(NULL == inBuf || NULL == outBuf)
    {
        printf("inBuf %p outBuf %p\n",inBuf,outBuf);
        return MUX_S_NULL_POINTER_ERR;
    }

    pMuxType = (int*)inBuf;
    pMuxPrm  = (MUX_PARAM *)outBuf;
    pMuxHdl  = &pMuxPrm->muxHdl;

    if(MUX_RTP == *pMuxType)
    {
        for(i = 0 ; i < MAX_STREAM_PACK_CHN ;i++)
        {
            pPackPrm = muxChnInfo + i;
            if(0 == pPackPrm->bUsed)
            {
                *pMuxHdl          = i;
                pPackPrm->bUsed   = 1;
                /*打包过程参数结构体长度，用于保存码流信息供转包使用*/
                RTPPACK_GetMemSize(&pPackPrm->stRtpPackPrm);
                pMuxPrm->bufLen   = pPackPrm->stRtpPackPrm.buffer_size + 1024;
                pMuxPrm->bufAddr  = NULL;
                pPackPrm->muxType = MUX_RTP;
                nRet = MUX_S_OK;
                break;
            }
        }

        if(MAX_STREAM_PACK_CHN == i)
        {
            nRet = MUX_S_NO_CHAN_ERR;
        }
    }
    else if(MUX_PS == *pMuxType)
    {
        for(i = 0 ; i < MAX_STREAM_PACK_CHN ;i++)
        {
            pPackPrm = muxChnInfo + i;
            if(0 == pPackPrm->bUsed)
            {
                *pMuxHdl          = i;
                pPackPrm->bUsed   = 1;

                PSMUX_GetMemSize(&pPackPrm->stPsPackPrm);
                pMuxPrm->bufLen   = pPackPrm->stPsPackPrm.buffer_size + 1024;
                pMuxPrm->bufAddr  = NULL;
                pPackPrm->muxType = MUX_PS;
                nRet = MUX_S_OK;
                break;
            }
        }
		
		if(MAX_STREAM_PACK_CHN == i)
        {
            nRet = MUX_S_NO_CHAN_ERR;
        }
    }
    else
    {
        nRet = MUX_S_TYPE_ERR;
    }
    
    return nRet;
}

static int MUX_Process(void *inBuf,void *outBuf)
{
    int                   muxHdl   = 0;
    MUX_PROC_INFO_S     *pProcInfo = NULL;
    MUX_PACK_CHN_INFO_ST *pPackPrm = NULL;
    int                   nRet     = MUX_S_OK;
    
    if(NULL == inBuf)
    {
        printf("MUX_Process inBuf %p \n",inBuf);
        return MUX_S_NULL_POINTER_ERR;
    }

    pProcInfo = (MUX_PROC_INFO_S*)inBuf;
    muxHdl    = pProcInfo->muxHdl;
    if(MAX_STREAM_PACK_CHN <= muxHdl)
    {
        printf("MUX  chan %d\n",muxHdl);
        return MUX_S_CHAN_ERR;
    }
    
    pPackPrm   = muxChnInfo + muxHdl;    
    if(MUX_RTP == pPackPrm->muxType)
    {
        nRet = MUX_Rtp_Pack(muxHdl,&pProcInfo->muxData);
    }
    else if(MUX_PS == pPackPrm->muxType)
    {
        nRet = MUX_Ps_Pack(muxHdl,&pProcInfo->muxData);
    }
    else
    {
        nRet = MUX_S_TYPE_ERR;
    }
    
    return nRet;
}

static int MUX_Create(void *inBuf,void *outBuf)
{
    int          nRet    = MUX_S_FAIL;
    int          muxHdl  = 0;
    MUX_PARAM  *pMuxPrm  = NULL;
    MUX_PACK_CHN_INFO_ST *pPackPrm = NULL;
    MEM_FOR_PACK_ST      *pMemPack;
    RTPPACK_PARAM      *rtpPackPrm;
    PSMUX_PARAM        *psPackPrm;

    if(NULL == inBuf)
    {
        printf("MUX_Create inBuf %p \n",inBuf);
        return MUX_S_NULL_POINTER_ERR;
    }

    pMuxPrm    = (MUX_PARAM*)inBuf;  
    muxHdl     = pMuxPrm->muxHdl;
    if(MAX_STREAM_PACK_CHN <= muxHdl)
    {
        printf("MUX  chan %d\n",muxHdl);
        return MUX_S_CHAN_ERR;
    }

    pPackPrm   = muxChnInfo + muxHdl;
    pMemPack   = &pPackPrm->stMemForPack;
    pMemPack->bufaddr   = pMuxPrm->bufAddr;
    pMemPack->buflen    = pMuxPrm->bufLen;
    pMemPack->bufUsed   = 0;        
    pMemPack->outputBuf = NULL;

    if(MUX_RTP == pPackPrm->muxType)
    {
        rtpPackPrm   = &pPackPrm->stRtpPackPrm;
        if(pMemPack->bufUsed + rtpPackPrm->buffer_size > pMemPack->buflen)
        {
            printf("MUX_Create bufUsed %d need %d buflen %d\n",pMemPack->bufUsed,rtpPackPrm->buffer_size,pMemPack->buflen);
            return MUX_S_NOT_ENOUGH_MEM_ERR;        
        }
        rtpPackPrm->buffer = pMemPack->bufaddr + pMemPack->bufUsed;
        pMemPack->bufUsed += rtpPackPrm->buffer_size;
        rtpPackPrm->info.max_packet_len = (pMuxPrm->maxPktLen ? pMuxPrm->maxPktLen : MAX_RTP_SIZE);
        MUX_Rtp_Fill_Info(&rtpPackPrm->info);
        nRet = RTPPACK_Create(rtpPackPrm, &pPackPrm->handle);
        if(HIK_RTPPACK_LIB_S_OK != nRet)
        {
            printf("RTPPACK_Create failed!nRet %x\n",nRet);
            return MUX_S_CREATE_ERR;
        }
        pPackPrm->bCreated = 1;
        nRet = MUX_S_OK;
    }
    else if(MUX_PS == pPackPrm->muxType)
    {
        psPackPrm = &pPackPrm->stPsPackPrm;
        if(pMemPack->bufUsed + psPackPrm->buffer_size > pMemPack->buflen)
        {
            printf("MUX_Create bufUsed %d need %d buflen %d\n",pMemPack->bufUsed,psPackPrm->buffer_size,pMemPack->buflen);
            return MUX_S_NOT_ENOUGH_MEM_ERR;        
        }
        psPackPrm->buffer = pMemPack->bufaddr + pMemPack->bufUsed;
        pMemPack->bufUsed += psPackPrm->buffer_size;

        MUX_Ps_Fill_Info(&psPackPrm->info);
        nRet = PSMUX_Create(psPackPrm, &pPackPrm->handle);
        if(HIK_RTPPACK_LIB_S_OK != nRet)
        {
            printf("RTPPACK_Create failed!nRet %x\n",nRet);
            return MUX_S_CREATE_ERR;
        }
        pPackPrm->audMux   = pMuxPrm->audPack;
        pPackPrm->bCreated = 1;
        nRet = MUX_S_OK;
    } 
    else
    {
        nRet = MUX_S_TYPE_ERR;
    }
    
    return nRet;
}

static int MUX_StreamSize(void *inBuf,void *outBuf)
{
    int nRet = MUX_S_OK;

    int *streamSize = NULL;
    MUX_STREAM_INFO_S *streamInfo;
    if(NULL == outBuf)
    {
        printf("MUX_StreamSize %p\n",outBuf);
        return MUX_S_NULL_POINTER_ERR;
    }

    streamInfo  = (MUX_STREAM_INFO_S *)inBuf;
    streamSize  = outBuf;

    if(MUX_AUD == streamInfo->muxType)
    {
        *streamSize = 1024*1024;
    }
    else if(MUX_VID == streamInfo->muxType)
    {
        *streamSize = ((1024*1024*3)>>1);
    }
    else
    {
        printf("stream type err!\n");
        nRet = MUX_S_TYPE_ERR;
    }

    return nRet;
}

static int MUX_GetInfo(void *inBuf,void *outBuf)
{
    int nRet = MUX_S_OK;
    int muxHdl = 0;
    MUX_INFO *pMuxInfo = NULL;
    HIKRTP_PACK_STREAM_INFO  streamInfo;
    MUX_PACK_CHN_INFO_ST     *pPackPrm  = NULL;

    muxHdl = *((int*)inBuf);
    if(MAX_STREAM_PACK_CHN <= muxHdl)
    {
        printf("MUX  chan %d\n",muxHdl);
        return MUX_S_CHAN_ERR;
    }

    pPackPrm   = muxChnInfo + muxHdl;
    pMuxInfo   = (MUX_INFO *)outBuf;
    if(MUX_RTP == pPackPrm->muxType)
    {
        RTPPACK_GetStreamInfo(pPackPrm->handle,&streamInfo);
        pMuxInfo->largestDist = pPackPrm->largestDist;
        pMuxInfo->seq_video   = streamInfo.seq_video;
        pMuxInfo->seq_audio   = streamInfo.seq_audio;
    }
    else if (MUX_PS == pPackPrm->muxType)
    {
		//RTPPACK_GetStreamInfo(pPackPrm->handle,&streamInfo);
        //pMuxInfo->largestDist = pPackPrm->largestDist;
        //pMuxInfo->seq_video   = streamInfo.seq_video;
        //pMuxInfo->seq_audio   = streamInfo.seq_audio;
        pMuxInfo->time_stamp = pPackPrm->stVideoTimeStampInfo.u32time_stamp_bak;
    }
	else
	{
	   
	    printf("not rtp/ps mux no muxInfo\n");
	    nRet = MUX_S_TYPE_ERR;
	}
    
    return nRet;
}

int MuxControl(MUX_IDX idx,void *inBuf,void *outBuf)
{
    int nRet = MUX_S_FAIL;
    
    switch(idx)
    {
        case MUX_GET_CHAN:
            nRet = MUX_GetChan(inBuf,outBuf);
            break;

        case MUX_CREATE:
            nRet = MUX_Create(inBuf,outBuf);
            break;
        
        case MUX_GET_STREAMSIZE:
            nRet = MUX_StreamSize(inBuf, outBuf);
            break;
            
        case MUX_PRCESS:
            nRet = MUX_Process(inBuf,outBuf);
            break;

        case MUX_GET_INFO:
            nRet = MUX_GetInfo(inBuf,outBuf);
            break;
            
        default:
            break;
    }

    return nRet;
}

int MuxInit()
{
    memset(muxChnInfo,0,sizeof(MUX_PACK_CHN_INFO_ST) * MAX_STREAM_PACK_CHN);
    return MUX_S_OK;
}

