/********************************************************************************
*
* 版权信息：版权所有 (c) 200X, 杭州海康威视数字技术有限公司, 保留所有权利
*
* 文件名称：PSDemuxLib.h
* 文件标识：HIK_PSDEMUX_LIB
* 摘    要：海康威视PSDEMUX 库的接口函数声明文件
*
* 当前版本：0.97
* 作    者：俞海 黄崇基
* 日    期：2009年12月07日
* 备    注：
* 2008 07 06	初始版本
* 2008 07 29	修改 descriptor 为 rtp 和 ps 通用的模式，支持非海康规范PS流，
*				但输出数据缺少部分信息，并且每次输出不一定是完整的帧
* 2009 01 08	修改PSH解析，加入帧号获取
* 2009 02 09	增加接口设置输出缓冲区位置，减少外部拷贝次数，同时
*				由于修改了type_dscrpt_common.h，需要随之修改
* 2009 03 04    不再将ps数据拷贝到内部缓冲区，而是直接在外部提供的缓冲区中解析，每次
*               解析完告诉库外部还剩多长的数据没解析
* 2009 03 07    如果发现包中出现同步字节错误，改重新搜索起始码为重新搜索PSH，使解析输出
*               的为整帧的数据
* 2009 05 22    添加私有数据privt_info_type，privt_info_sub_type，privt_info_frame_num三个接口
*               库外部通过这三个接口就可以获知私有数据(如智能信息)的主类型，子类型，和起作用的帧号
* 2009 06 25    将输出数据方式改为每解析完一个pes包就输出数据，去掉函数PSDEMUX_SetOutBuf
* 2009 07 03    增加一帧开始的接口is_frame_start
* 2009 08 06    增加一个nalu结束的接口is_nalu_end
* 2009 12 07    增加一些新的音频类型
********************************************************************************/

#include <stdio.h>
#include <memory.h>
#include "PSDemuxLib.h"
#include "get_es_info.h"
#include "DSP_hevc_sps_parse.h"

/* PS 包头标志*/
#define PROGRAM_STREAM_PACKET   0xba
#define PROGRAM_STREAM_MAP      0xbc

#define NEED_MORE_DATA          (-1)
#define GB_RTP_HEADER_LEN       (12)
#define MAX_PSH_SIZE            20  /* PSH长度 */
#define MAX_PES_PACKET_LEN      65496

/* 数据块处理参数结构 */
typedef struct _HIK_PSDEMUX_PROGRAM_
{
    //global param
    unsigned int    ps_type;                //
    unsigned int    stream_mode;            // 流模式
    unsigned int    max_frame_size;         // 输出缓冲区的大小
    unsigned int    ps_stream_map_count;    // ps stream map 计数器，大于0时表示流可用
    unsigned int    sync_lost_flg;          // 码流中是否出现了同步字节丢失，1为是，0为否
    unsigned short  video_stream_id;        // 视频流ID
    unsigned short  audio_stream_id;        // 音频流ID
    unsigned int    privt_stream_id;        // 私有流ID

    PSDEMUX_DATA_OUTPUT frame_tmp;          // 当前处理的帧结构
    HIK_STREAM_INFO     info;               // 当前流信息
    unsigned  int    last_time_stamp;

    unsigned char*   ringBuf;
    unsigned int     ringBufLen;
    unsigned int     readIdx;
    unsigned int     writeIdx;
    unsigned int     dataLen;

    unsigned int     vidOut;
    unsigned int     audOut;
    unsigned char*   psdemAddr;
    unsigned int     psdemBuflen;
    
    unsigned int     frameLen;
} HIK_PSDEMUX_PRG;

unsigned int CheckStartCode(unsigned char *buf)
{
    if ((buf[0] != 0x00) || (buf[1] != 0x00) || (buf[2] != 0x01))
    {
        return 0;
    }

    return (buf[3] & 0xff);
}
int PSDEMUX_parse_PSH(unsigned char  *buf, int len, HIK_PSDEMUX_PRG *prg);

/******************************************************************************
* 功  能：  搜索可识别的起始码
* 参  数：  buf     - 搜索的缓冲区
*           len     - buf的长度
*           prg     - 模块参数结构体
* 返回值：  解析正常时返回所找到的起始码到输入buf指针之间的偏移，否则返回错误码
******************************************************************************/
int PSDEMUX_search_start_code(void *handle)
{
    HIK_PSDEMUX_PRG* prg = (HIK_PSDEMUX_PRG*)handle;
    unsigned int pos = prg->readIdx;
    unsigned int dataLen = prg->dataLen;
    unsigned char *buf = prg->ringBuf;
    unsigned int len;

    /*四字节对齐 */
    if (pos & 3)
    {
        /* LOG("PSDEMUX_search_start_code_Not_Align,r=%d,w=%d",pDecOrg->orgReadIdx,pDecOrg->orgWriteIdx); */
        printf("PSDEMUX_search_start_code_Not_Align,r=%u,w=%u\n", prg->readIdx, prg->writeIdx);
        pos = (pos + 3) & (~3);
        dataLen &= ~3;
    }

    /*目前的操作都基于PSH和PSM四字节对齐 */
    for (len = 0; len < dataLen; len += 4)
    {
        if (buf[pos] == 0x00 && buf[pos + 1] == 0x00 && buf[pos + 2] == 0x01)
        {
            if (buf[pos + 3] == prg->audio_stream_id
                || buf[pos + 3] == prg->video_stream_id
                || buf[pos + 3] == prg->privt_stream_id
                || buf[pos + 3] == PROGRAM_STREAM_PACKET
                || buf[pos + 3] == PROGRAM_STREAM_MAP)
            {
                prg->readIdx = pos;
                /*PRT("audio_stream_id = %d, %d\n",prg->audio_stream_id,buf[pos + 3]); */
                return HIK_PSDEMUX_LIB_S_OK;
            }
        }

        pos += 4;
        if (pos >= prg->ringBufLen)
        {
            pos -= prg->ringBufLen;
        }
    }

    prg->readIdx = pos;
    return NEED_MORE_DATA;
}

/******************************************************************************
* 功  能：  搜索PSM
* 参  数：  buf     - 搜索的缓冲区
*           len     - buf的长度
* 返回值：  解析正常时返回下一个PSM之前的PSH到输入buf指针之间的偏移，
			若找到PSM但找不到PSH则返回PSM到buf指针之间的偏移，
			都找不到则返回错误码
******************************************************************************/
int PSDEMUX_search_PSM(void *handle)
{
    int              ret = 0;
    int              pos_psm, pos_psh,pos;
    unsigned int     dataLen;
    unsigned char*   buf;
    //unsigned char*   data;
    HIK_PSDEMUX_PRG *prg = (HIK_PSDEMUX_PRG *)handle;

    prg->dataLen = (prg->writeIdx >= prg->readIdx ? prg->writeIdx - prg->readIdx : prg->ringBufLen - prg->readIdx + prg->writeIdx); 
    dataLen      = prg->dataLen;
    pos          = prg->readIdx;
    /*四字节对齐 */
    if (pos & 3)
    {
        printf("PSDEMUX_search_PSM_Not_Align,r=%d,w=%d", prg->readIdx, prg->writeIdx);
        pos = (pos + 3) & (~3);
        prg->readIdx = pos;
        dataLen &= (~3);    
    }

    //data = prg->ringBuf + prg->readIdx;
    //printf("readIdx %d writeIdx %d ringBufLen %d ringBuf %p | data %x %x %x %x %x %x %x %x \n",prg->readIdx,prg->writeIdx,prg->ringBufLen,prg->ringBuf,*data,*(data+1)
    //,*(data+2),*(data+3),*(data+4),*(data+5),*(data+6),*(data+7));

    buf = prg->ringBuf;
    /*目前的操作都基于PSH和PSM四字节对齐 */
    for (pos_psm = 0; pos_psm < dataLen; pos_psm += 4)
    {
        if (buf[pos] == 0x00 && buf[pos + 1] == 0x00 && buf[pos + 2] == 0x01 && buf[pos + 3] == PROGRAM_STREAM_MAP)
        {
            pos_psh = pos - 20;
            if (pos_psh < 0)
            {
                pos_psh += prg->ringBufLen;
                for ( ; pos_psh < prg->ringBufLen; pos_psh += 4)
                {
                    if (buf[pos_psh] == 0x00 && buf[pos_psh + 1] == 0x00 && buf[pos_psh + 2] == 0x01 && buf[pos_psh + 3] == PROGRAM_STREAM_PACKET)
                    {
                        memcpy(prg->ringBuf + prg->ringBufLen, prg->ringBuf + pos_psh, (prg->ringBufLen - pos_psh));
                        memcpy(prg->ringBuf + prg->ringBufLen + (prg->ringBufLen - pos_psh), prg->ringBuf, pos);
                        ret = PSDEMUX_parse_PSH(prg->ringBuf + prg->ringBufLen, pos + (prg->ringBufLen - pos_psh), prg);
						if (ret<= 0)
						{
						    printf("0 PSDEMUX_parse_PSH error\n");
						}
                    }
                }

                for (pos_psh = 0; pos_psh < pos; pos_psh += 4)
                {
                    if (buf[pos_psh] == 0x00 && buf[pos_psh + 1] == 0x00 && buf[pos_psh + 2] == 0x01 && buf[pos_psh + 3] == PROGRAM_STREAM_PACKET)
                    {
                        ret = PSDEMUX_parse_PSH(prg->ringBuf + pos_psh, pos - pos_psh, prg);
						if (ret<= 0)
						{
						    printf("1 PSDEMUX_parse_PSH error\n");
						}
                    }
                }
            }
            else
            {
                for ( ; pos_psh < pos; pos_psh += 4)
                {
                    if (buf[pos_psh] == 0x00 && buf[pos_psh + 1] == 0x00 && buf[pos_psh + 2] == 0x01 && buf[pos_psh + 3] == PROGRAM_STREAM_PACKET)
                    {
                        ret = PSDEMUX_parse_PSH(prg->ringBuf + pos_psh, pos - pos_psh, prg);
						if (ret<= 0)
						{
						    printf("2 PSDEMUX_parse_PSH error\n");
						}
                    }
                }
            }
            printf("find psm pos %d\n",pos);

            prg->readIdx = pos;
            return HIK_PSDEMUX_LIB_S_OK;
        }

        pos += 4;
        if (pos >= prg->ringBufLen)
        {
            pos -= prg->ringBufLen;
        }
    }

    prg->readIdx = pos;
    return NEED_MORE_DATA;
}
 
/******************************************************************************
* 功  能：  搜索下一个psh
* 参  数：  buf     - 搜索的缓冲区
*           len     - buf的长度
* 返回值：  解析正常时返回所找到的起始码到输入buf指针之间的偏移，否则返回错误码
******************************************************************************/
int PSDEMUX_search_PSH(unsigned char *buf, int len)
{
    int pos = 0;

    for (pos = 0; pos < len - 4; pos ++)
    {
        if (buf[pos] == 0x00 && buf[pos+1] == 0x00 && buf[pos+2] == 0x01)
        {
            if (buf[pos + 3] == PROGRAM_STREAM_PACKET)
            {
                return pos;
            }
        }
    }
    return NEED_MORE_DATA;
}

/******************************************************************************
* 功  能：  解析PSH
* 参  数：  buf     - 解析缓冲区
*           len     - buf的长度
*   	    prg     - 模块参数结构体
* 返回值：  解析正常时返回解析获得的包长度，否则返回错误码
******************************************************************************/
int PSDEMUX_parse_PSH(unsigned char  *buf, int len, HIK_PSDEMUX_PRG *prg)
{
    int stuff_len = 0;
    int video_frame_count = 0;

    if ((buf[4] & 0xc0) != 0x40 || (buf[13] & 0xf8) != 0xf8)
    {

        printf("PSDEMUX_parse_PSH_Error!!%x\n", buf[4]);
        return HIK_PSDEMUX_LIB_E_SYNC_LOST;
    }

    stuff_len = (int)(buf[13] & 0x07); /* 填充字节 */
    prg->frame_tmp.sys_clk_ref = (unsigned int)((buf[4] & 0x38) << 26)   /* 3bits	system_clock_reference_base [32..30] */
                                 + (unsigned int)((buf[4] & 0x03) << 27) /* 1bits	marker_bit */
                                 + (unsigned int)((buf[5] & 0xff) << 19) /* 2bits	system_clock_reference_base [29..28] */
                                 + (unsigned int)((buf[6] & 0xf8) << 11) /* 8bits	system_clock_reference_base [27..20] */
                                 + (unsigned int)((buf[6] & 0x03) << 12) /* 5bits	system_clock_reference_base [19..15] */
                                 + (unsigned int)((buf[7] & 0xff) << 4)  /* 1bits	marker_bit */
                                 + (unsigned int)((buf[8] & 0xf0) >> 4); /* 2bits	system_clock_reference_base [14..13] */
                                                                          /* 8bits	system_clock_reference_base [12..5] */
                                                                          /* 5bits	system_clock_reference_base [4..0] */
                                                                          /* 1bits	marker_bit */
                                                                          /* 2bits	system_clock_reference_ext [8..7] */
                                                                          /* 7bits	system_clock_reference_ext [6..0] */
                                                                          /* 1bits	marker_bit */

    /* 重置帧相关的数据 */
    prg->frame_tmp.output_type = FRAME_TYPE_VIDEO_PFRAME;                 /* 默认为PFRAME */
    prg->frame_tmp.encrypt = 0;                                           /* 是否加密         */
    prg->frame_tmp.data_len = 0;

    if (stuff_len == 6) /* 包含帧号 */
    {
        video_frame_count = (buf[16] << 24) + (buf[17] << 16) + (buf[18] << 8) + buf[19];
        prg->info.video_info.frame_num = video_frame_count;
        prg->info.video_info.refType   = buf[15];
    }
    //prg->frame_tmp.refType = buf[15];

    return (14 + stuff_len);
}

/******************************************************************************
* 功  能：  解析PSM
* 参  数：  buf     - 解析缓冲区
*   	    prg     - 模块参数结构体
* 返回值：  解析正常时返回解析获得的包长度，否则返回错误码
******************************************************************************/
int PSDEMUX_parse_PSM(unsigned char *buf, HIK_PSDEMUX_PRG *prg)
{
    int pos;
    int ret;
    int map_len = 0;
    unsigned int descriptor_len = 0;
    unsigned int stream_type_cur;
    HIK_STREAM_INFO * info_ptr = &prg->info;

    prg->stream_mode = 0;
    map_len = (buf[4] << 8) + buf[5];
    descriptor_len = (unsigned short)(buf[8] << 8) + (unsigned short)buf[9];
    ret = HIKDSC_parse_descriptor_area(&buf[10], descriptor_len, 0, info_ptr);
    if (ret < 0)
    {
        return HIK_PSDEMUX_LIB_E_STM_ERR;
    }
    pos = 10 + descriptor_len;
    pos += 2;

    while (pos < map_len + 2)
    {
        stream_type_cur = buf[pos++];
        switch(stream_type_cur)
        {
            case STREAM_TYPE_VIDEO_MPEG1 :
            case STREAM_TYPE_VIDEO_MPEG2 :
            case STREAM_TYPE_VIDEO_MPEG4 :
            case STREAM_TYPE_VIDEO_H264  :
            case STREAM_TYPE_VIDEO_MJPG	 :
            case STREAM_TYPE_HIK_H264    :
			case STREAM_TYPE_VIDEO_H265  :
                prg->stream_mode |= INCLUDE_VIDEO_STREAM;
                prg->video_stream_id = buf[pos ++];//视频流id
                info_ptr->video_info.stream_type = stream_type_cur;
                break;

            case STREAM_TYPE_AUDIO_MPEG1  :
            case STREAM_TYPE_AUDIO_MPEG2  :
            case STREAM_TYPE_AUDIO_AAC    :
            case STREAM_TYPE_AUDIO_G711A  :
            case STREAM_TYPE_AUDIO_G711U  :
            case STREAM_TYPE_AUDIO_G722_1 :
            case STREAM_TYPE_AUDIO_G723_1 :
            case STREAM_TYPE_AUDIO_G726   :
            case STREAM_TYPE_AUDIO_G726_16:
            case STREAM_TYPE_AUDIO_G729 :
            case STREAM_TYPE_AUDIO_AMR_NB :
            case STREAM_TYPE_AUDIO_AMR_WB :
            case STREAM_TYPE_AUDIO_L16 :
            case STREAM_TYPE_AUDIO_L8 :
            case STREAM_TYPE_AUDIO_DVI4 :
            case STREAM_TYPE_AUDIO_GSM :
            case STREAM_TYPE_AUDIO_GSM_EFR :
            case STREAM_TYPE_AUDIO_LPC :
            case STREAM_TYPE_AUDIO_QCELP :
            case STREAM_TYPE_AUDIO_VDVI :
                prg->stream_mode |= INCLUDE_AUDIO_STREAM;
                prg->audio_stream_id = buf[pos ++];//音频流id
                info_ptr->audio_info.stream_type = stream_type_cur;
                break;

            case STREAM_TYPE_HIK_PRIVT:
                prg->stream_mode |= INCLUDE_PRIVT_STREAM;
                prg->privt_stream_id = buf[pos ++];//私有流id
                info_ptr->privt_stream_type = stream_type_cur;
                break;
                
            default:
                pos++;
                break;
        }
        descriptor_len = (unsigned short)(buf[pos] << 8) + (unsigned short)buf[pos + 1];
        if (prg->info.is_hik_stream)
        {
            ret = HIKDSC_parse_descriptor_area(&buf[pos+2], descriptor_len, 0, info_ptr);
            if (ret < 0)
            {
                return HIK_PSDEMUX_LIB_E_STM_ERR;
            }
        }
        pos += (2 + descriptor_len);
    }

    prg->ps_stream_map_count++;

    if (prg->ps_stream_map_count == 0) //防止prg->ps_stream_map_count回复到0
    {
        prg->ps_stream_map_count = 1;
    }
    //PSM后跟随的第一帧肯定是I帧
    prg->frame_tmp.output_type  = FRAME_TYPE_VIDEO_IFRAME;
    prg->frame_tmp.search_sps   = 1;
    return (int)(map_len + 6);
}

/******************************************************************************
* 功  能：  解析元素流私有流的12个字节的头
* 参  数：  buf     - 解析缓冲区
*   	    prg     - 模块参数结构体
* 返回值：
******************************************************************************/
void PSDEMUX_parse_private_data(unsigned char *buf, HIK_PSDEMUX_PRG *prg)
{
    PSDEMUX_DATA_OUTPUT * frame_cur = &prg->frame_tmp;

    frame_cur->privt_info_type = (buf[0] << 8) + buf[1];
    frame_cur->privt_info_sub_type = (buf[4] << 8) + buf[5];
    frame_cur->privt_info_frame_num = (buf[7] << 24) + (buf[8] << 16) + (buf[10] << 8) + buf[11];
    return;
}

/******************************************************************************
* 功  能：  解析元素流pes
* 参  数：  buf     - 解析缓冲区
*   	    prg     - 模块参数结构体
* 返回值：  解析正常时返回解析获得的包长度，否则返回错误码
******************************************************************************/
int PSDEMUX_parse_elementary_PES(unsigned char *buf, HIK_PSDEMUX_PRG *prg)
{
    unsigned int temp_len = 0;
    unsigned int rest_len = 0;
    unsigned int pts_dts_flags;
    unsigned int pes_payload_len;
    unsigned int frame_flag = 0;
    unsigned int is_hik_ps  = prg->info.is_hik_stream;
    unsigned int max_frame_size = 0;
    unsigned char * pes_header_ext_ptr;
    unsigned char * pes_payload_buf;
    unsigned char * pDest;
    PSDEMUX_DATA_OUTPUT *frame_cur = &prg->frame_tmp;
    unsigned int         vidOrAud  = 0;/*1-vid,2-aud,3-privt*/
    unsigned int      strParseSize = sizeof(HIK_STREAM_PARSE);
    HIK_STREAM_PARSE  *pStreamParse = NULL;

    if ((buf[6] & 0x80) != 0x80)
    {
        return HIK_PSDEMUX_LIB_E_SYNC_LOST;
    }

    if (buf[3] == prg->video_stream_id && is_hik_ps)
    {//默认frame_cur->output_type是视频P帧，当发现PSM时修改成I帧
        if (((buf[6] >> 3) & 0x01) == 0)//PES_priority
        {
            frame_cur->output_type = FRAME_TYPE_VIDEO_BFRAME;
        }
    }
    else if (buf[3] == prg->audio_stream_id)
    {
        frame_cur->output_type = FRAME_TYPE_AUDIO_FRAME;
        prg->frameLen = 0;
    }
    else if (buf[3] == prg->privt_stream_id)
    {
        frame_cur->output_type = FRAME_TYPE_PRIVT_FRAME;
    }

    temp_len = (buf[4] << 8) + buf[5];
    rest_len = buf[8];                          //剩余pes包头长
    pes_payload_len = temp_len - rest_len - 3;  //原始流长度
    pes_payload_buf = &buf[9 + rest_len];       //原始流起始处
    pts_dts_flags   = (buf[7] & 0xc0) >> 6;
    frame_cur->is_frame_start = 0;

    if (pts_dts_flags != 0x00)
    {
        prg->frame_tmp.data_time_stamp  = (unsigned int)((buf[ 9] & 0x0e) << 28)
                                        + (unsigned int)(buf[10] << 21)
                                        + (unsigned int)((buf[11] & 0xfe) << 13)
                                        + (unsigned int)(buf[12] <<  6)
                                        + (unsigned int)((buf[13] & 0xfc) >>  2);

        if (prg->frame_tmp.data_time_stamp != prg->last_time_stamp)
        {
            frame_cur->is_frame_start = 1;
        }

        prg->last_time_stamp = prg->frame_tmp.data_time_stamp;

    }

    frame_cur->encrypt = (buf[6] >> 4) & 3;

    if ((buf[7] & 0x01) && is_hik_ps)
    {
        if (!pts_dts_flags)
        {
            pes_header_ext_ptr = &buf[9];
        }
        else
        {
            pes_header_ext_ptr = &buf[14];
        }
        if ((*pes_header_ext_ptr) >> 7)
        {
            frame_cur->have_userdata    = 1;
            frame_cur->userdata         = pes_header_ext_ptr + 1;
        }
    }

    if(is_hik_ps)
    {
        frame_flag = (~buf[8 + rest_len]) & 0xff;
        frame_cur->is_frame_end = (frame_flag & 0x04) >> 2;
    }

    //如果不是海康PS清空缓冲区，因为每次pes分析结束都会推送出来
    if (!is_hik_ps)
    {
        frame_cur->data_len = 0;
    }

    pDest = NULL;
    max_frame_size = prg->psdemBuflen;
    if(FRAME_TYPE_AUDIO_FRAME == frame_cur->output_type)
    {
        pStreamParse   = (HIK_STREAM_PARSE  *)prg->psdemAddr;
        /*缓冲满，丢帧处理*/
        if((!pStreamParse->bufUsed) && prg->audOut)
        {
            pDest = prg->psdemAddr;
        }
        vidOrAud = 2;
    }
    else if(FRAME_TYPE_VIDEO_IFRAME == frame_cur->output_type 
         || FRAME_TYPE_VIDEO_PFRAME == frame_cur->output_type)
    {
        pStreamParse   = (HIK_STREAM_PARSE  *)prg->psdemAddr;
        /*缓冲满，丢帧处理*/
        if((!pStreamParse->bufUsed) && prg->vidOut)
        {
            pDest = prg->psdemAddr;
        }
        vidOrAud = 1;
    }
    else if(FRAME_TYPE_PRIVT_FRAME == frame_cur->output_type)
    {
        pStreamParse   = (HIK_STREAM_PARSE  *)prg->psdemAddr;
        /*缓冲满，丢帧处理*/
        if((!pStreamParse->bufUsed) && prg->vidOut)
        {
            pDest = prg->psdemAddr;
        }
        vidOrAud = 3;
    }

    //解析私有数据头
    if (frame_cur->output_type == FRAME_TYPE_PRIVT_FRAME)
    {
        PSDEMUX_parse_private_data(pes_payload_buf, prg);
        pes_payload_buf += 12;
        pes_payload_len -= 12;
    }
	
    if ((prg->frameLen + pes_payload_len > max_frame_size - strParseSize) && ((2 == vidOrAud && prg->audOut) || (1 == vidOrAud && prg->vidOut) || (3 == vidOrAud && prg->vidOut)))
    {
        printf("data_len %d pes_payload_len %d,max_frame_size %d\n",prg->frameLen,pes_payload_len,max_frame_size);
        return HIK_PSDEMUX_LIB_E_MEM_OVER;
    }

    //////////////////////////////////////////////////////////////////////////
    //每一个pes包都输出数据
    frame_cur->data_buffer = pes_payload_buf;
    frame_cur->data_len    = pes_payload_len;

    if(frame_cur->is_frame_start)
    {
        prg->frameLen  = 0;
    }
    
    if(pDest)
    {
        memcpy(pDest + strParseSize + prg->frameLen, pes_payload_buf, pes_payload_len);
        if((FRAME_TYPE_VIDEO_IFRAME == frame_cur->output_type))
        {
            unsigned char  *pData = pes_payload_buf;
            if((0x0 == *pData) && (0x0 == *(pData+1)) && (0x0 == *(pData+2)) && (0x1 == *(pData+3)) 
                && ((0x67 == *(pData+4)) || (0x42 == *(pData+4)) || ((*(pData + 4) & 0x1f) == 0x07))) /* 大华有的IPC(比如IPC-HFW4833F-ZAS)SPS对应的BYTE是0x27*/
            {
                if(prg->frame_tmp.search_sps)
                { 
                    if ((0x67 == *(pData+4)) || ((*(pData + 4) & 0x1f) == 0x07)) /* 大华有的IPC(比如IPC-HFW4833F-ZAS)SPS对应的BYTE是0x27*/
                    {
                        VIDEO_ES_INFO  esInfo; 
                        seekVideoInfoAvc(pes_payload_buf,pes_payload_len,&esInfo);
                        prg->frame_tmp.search_sps = 0;
                        prg->info.video_info.width_orig  = esInfo.width;
                        prg->info.video_info.height_orig = esInfo.height;
                    }
					else if (0x42 == *(pData+4))
					{
					    VIDEO_INFO               esInfoHevc = {0}; 
						VIDEO_HEVC_INFO          hevc_info  = {0};

						esInfoHevc.codec_specific.hevc_info = &hevc_info;
	   					if (OPENHEVC_GetPicSizeFromSPS_dsp(pes_payload_buf + 4,pes_payload_len + 4, &esInfoHevc) != 0)
						{
							printf("HEVCDEC_InterpretSPS fail ! restLen %d\n", pes_payload_len);
							return -1;
						}

						prg->frame_tmp.search_sps = 0;
	                    prg->info.video_info.width_orig  = esInfoHevc.img_width;
	                    prg->info.video_info.height_orig = esInfoHevc.img_height;
					}
		
                }
                prg->frame_tmp.spsLen            = pes_payload_len;
            }
            else if((0x0 == *pData) && (0x0 == *(pData+1)) && (0x0 == *(pData+2)) && (0x1 == *(pData+3)) && ((0x44 == *(pData+4)) || (0x68 == *(pData+4))))
            {
                prg->frame_tmp.ppsLen            = pes_payload_len;
            }
			else if((0x0 == *pData) && (0x0 == *(pData+1)) && (0x0 == *(pData+2)) && (0x1 == *(pData+3)) && (0x40 == *(pData+4)))
			{
			    prg->frame_tmp.vpsLen            = pes_payload_len;
			}
        }
    }
    
    prg->frameLen += frame_cur->data_len;
    if (frame_flag & 1)
    {
        frame_cur->is_nalu_end = 1;
        if (frame_flag >> 2 & 1)
        {
            frame_cur->is_frame_end = 1;
            if(pDest)
            {
                pStreamParse = (HIK_STREAM_PARSE  *)pDest;
                pStreamParse->frameLen  =  prg->frameLen;
                pStreamParse->stampMs   =  frame_cur->data_time_stamp;
                pStreamParse->frameType =  frame_cur->output_type;
                pStreamParse->bufUsed   =  1;
				pStreamParse->frameNum   = prg->info.video_info.frame_num;

                if(1 == vidOrAud)
                {
                    pStreamParse->width     =  prg->info.video_info.width_orig;
                    pStreamParse->height    =  prg->info.video_info.height_orig;
                    pStreamParse->spsLen    =  frame_cur->spsLen;
                    pStreamParse->ppsLen    =  frame_cur->ppsLen;
					pStreamParse->vpsLen    =  frame_cur->vpsLen;
				}
                if(3 == vidOrAud)
                {
                    pStreamParse->privtType = frame_cur->privt_info_type;
                    pStreamParse->privtSubType = frame_cur->privt_info_sub_type;
                }
            }
            
            prg->frameLen = 0;
            //prg->info.video_info.width_orig  = 0;
            //prg->info.video_info.height_orig = 0;
            frame_cur->ppsLen = 0;
            frame_cur->spsLen = 0;
			frame_cur->vpsLen = 0;
        }
    }
    
    frame_cur->stream_mode = prg->stream_mode;
    frame_cur->data_len = 0;
    //frame_cur->is_frame_end = 0;
    frame_cur->is_nalu_end = 0;
    frame_cur->have_userdata = 0;
    frame_cur->encrypt = 0;
    //////////////////////////////////////////////////////////////////////////
    return (int)temp_len + 6;
}

/******************************************************************************
* 功  能：  解析pes包和psh
* 参  数：  buf     - 需要解析的缓冲区
*           len     - 需要解析的缓冲区长度
*   	    prg     - 模块参数结构体
* 返回值：  解析正常时返回解析获得的包长度，否则返回错误码
******************************************************************************/
int PSDEMUX_parse_PES(void *handle)
{
    int            ret   = 0;
    unsigned int pes_len = 0;
    unsigned int pes_type;
    HIK_PSDEMUX_PRG  *prg = (HIK_PSDEMUX_PRG *)handle;
    unsigned int rest_len = prg->ringBufLen - prg->readIdx;
    unsigned int readyIdx = prg->readIdx;
    unsigned char *buf    = prg->ringBuf + prg->readIdx;


    if (prg->dataLen < MAX_PSH_SIZE)
    {
        return NEED_MORE_DATA;
    }

    if (prg->readIdx & 3)
    {
        printf("readIdx %d\n",prg->readIdx);
        return HIK_PSDEMUX_LIB_E_SYNC_LOST;
    }

    pes_type = buf[3];
    /* 流中出现数据错误，开始不是起始码 */
    if ((buf[0] != 0x00) || (buf[1] != 0x00) || (buf[2] != 0x01))
    {
        return HIK_PSDEMUX_LIB_E_SYNC_LOST;
    }

    /* ps 包头特殊处理 */
    if (pes_type == PROGRAM_STREAM_PACKET)
    {
        /*拷贝处理 */
        if (rest_len < MAX_PSH_SIZE)
        {
            memcpy(prg->ringBuf + prg->ringBufLen, prg->ringBuf, 32);
        }

        ret = PSDEMUX_parse_PSH(buf, MAX_PSH_SIZE, prg);
        return ret;
    }

    /*只考虑四字节对齐情况 */
    if (rest_len <= 6)
    {
        pes_len = (prg->ringBuf[0] << 8) + prg->ringBuf[1] + 6;
    }
    else
    {
        pes_len = (buf[4] << 8) + buf[5] + 6;
    }

    if (pes_len > MAX_PES_PACKET_LEN)
    {
        printf("PSDEMUX_parse_PES: pes_len =%d>MAX_PES_PACKET_LEN!!\n", pes_len);
        return HIK_PSDEMUX_LIB_E_SYNC_LOST;
    }

    if (pes_len > prg->dataLen)
    {
        return NEED_MORE_DATA;
    }

    if (rest_len < pes_len)
    {
        memcpy(prg->ringBuf + prg->ringBufLen, prg->ringBuf, (pes_len - rest_len));
    }

    prg->frame_tmp.is_end_group = 0;
    if (prg->dataLen - pes_len > 4)
    {
        readyIdx += pes_len;
        if (readyIdx >= prg->ringBufLen)
        {
            readyIdx -= prg->ringBufLen;
        }

        ret = CheckStartCode(prg->ringBuf + readyIdx);
        if (ret == 0)
        {
            return HIK_PSDEMUX_LIB_E_SYNC_LOST;
        }

        if (ret != prg->video_stream_id)
        {
            prg->frame_tmp.is_end_group = 0x01;
        }
    }
    else
    {
        prg->frame_tmp.is_end_group = 0x02;
    }

    /*视频流或者音频流数据处理*/
    if ((pes_type == prg->audio_stream_id)
        || (pes_type == prg->video_stream_id)
        || (pes_type == prg->privt_stream_id))
    {
        ret = PSDEMUX_parse_elementary_PES(buf, prg);
    }
    /*PSM处理*/
    else if (pes_type == PROGRAM_STREAM_MAP)
    {
        ret = PSDEMUX_parse_PSM(buf, prg);
    }
    /*未知类型*/
    else
    {
        ret = pes_len;
    }

    return ret;
}

HRESULT PSDEMUX_SetRingBuf(unsigned char* bufaddr,unsigned int bufLen,void *handle)
{
    HIK_PSDEMUX_PRG *prg = (HIK_PSDEMUX_PRG *)handle;
    if (bufaddr == NULL || handle == NULL || 0 == bufLen)
    {
        return HIK_PSDEMUX_LIB_E_PARA_NULL;
    }
    
    prg->ringBuf = bufaddr;
    prg->ringBufLen = bufLen;

    return HIK_PSDEMUX_LIB_S_OK;
}

HRESULT PSDEMUX_SetOutBuf(PSDEMUX_OUT_PARAM *outPrm,void *handle)
{
    HIK_PSDEMUX_PRG *prg = (HIK_PSDEMUX_PRG *)handle;
    if (outPrm == NULL || handle == NULL)
    {
        return HIK_PSDEMUX_LIB_E_PARA_NULL;
    }
    
    prg->vidOut        = outPrm->vOut;
    prg->audOut        = outPrm->aOut;
    prg->psdemAddr     = outPrm->bufAddr;
    prg->psdemBuflen   = outPrm->bufLen;

    return HIK_PSDEMUX_LIB_S_OK;
}

HRESULT PSDEMUX_FullBuf(PSDEMUX_PROCESS_PARAM * param,void *handle)
{
    HIK_PSDEMUX_PRG *prg = (HIK_PSDEMUX_PRG *)handle;
    unsigned int     strParseSize   = sizeof(HIK_STREAM_PARSE);
    HIK_STREAM_PARSE  *pStreamParse = NULL;
    
    if (param == NULL || handle == NULL)
    {
        printf("error %p,%p\n",param,handle);
        return HIK_PSDEMUX_LIB_E_PARA_NULL;
    }

    if(0 == param->payload)
    {
        pStreamParse          = (HIK_STREAM_PARSE  *)prg->psdemAddr;
		param->glb_time.year  = prg->info.glb_time.year;
		param->glb_time.month = prg->info.glb_time.month;
		param->glb_time.date  = prg->info.glb_time.date;
		param->glb_time.hour  = prg->info.glb_time.hour;
		param->glb_time.minute= prg->info.glb_time.minute;
		param->glb_time.second= prg->info.glb_time.second;
		param->glb_time.msecond = prg->info.glb_time.msecond;
		param->time_info        = prg->info.video_info.time_info;
        if(pStreamParse->bufUsed && (FRAME_TYPE_AUDIO_FRAME != pStreamParse->frameType))
        {
            pStreamParse->bufUsed = 0;
            param->stamp      = pStreamParse->stampMs;
            param->outdatalen = pStreamParse->frameLen;
            param->frmType    = pStreamParse->frameType;
            param->width      = pStreamParse->width;
            param->height     = pStreamParse->height;
			param->vpsLen     = pStreamParse->vpsLen;
            param->spsLen     = pStreamParse->spsLen;
            param->ppsLen     = pStreamParse->ppsLen;
			param->frameNum   = pStreamParse->frameNum;
			param->streamType = pStreamParse->streamType;

            if((param->outbuflen >= param->outdatalen) && param->outbuf)
            {
                memcpy(param->outbuf ,prg->psdemAddr + strParseSize, pStreamParse->frameLen);
            }
            else if(NULL == param->outbuf)
            {
                param->outbuf = prg->psdemAddr + strParseSize;
            }
            
            return HIK_PSDEMUX_LIB_S_OK;
        }
        else
        {
        }
        return HIK_PSDEMUX_LIB_S_FAIL;
    }
    else if(1 == param->payload)
    {
        pStreamParse          = (HIK_STREAM_PARSE  *)prg->psdemAddr;
        if(pStreamParse->bufUsed && (FRAME_TYPE_AUDIO_FRAME == pStreamParse->frameType))
        {
            pStreamParse->bufUsed = 0;
            param->stamp      = pStreamParse->stampMs;
            param->outdatalen = pStreamParse->frameLen;
            param->frmType    = pStreamParse->audEncType;
			param->frameNum   = pStreamParse->frameNum;

            if((param->outbuflen >= param->outdatalen) && param->outbuf)
            {
                memcpy(param->outbuf ,prg->psdemAddr + strParseSize, pStreamParse->frameLen);
            }
            else if(NULL == param->outbuf)
            {
                param->outbuf = prg->psdemAddr + strParseSize;
            }
            return HIK_PSDEMUX_LIB_S_OK;
        }
        else
        {
        }
        return HIK_PSDEMUX_LIB_S_FAIL;
    }
    else if(2 == param->payload)
    {
        pStreamParse          = (HIK_STREAM_PARSE  *)prg->psdemAddr;
        if(pStreamParse->bufUsed && (FRAME_TYPE_PRIVT_FRAME == pStreamParse->frameType))
        {
            pStreamParse->bufUsed = 0;
            param->stamp      = pStreamParse->stampMs;
            param->outdatalen = pStreamParse->frameLen;
            param->frmType    = pStreamParse->frameType;
			param->frameNum   = pStreamParse->frameNum;            
            param->privtType  = pStreamParse->privtType;
            param->privtSubType = pStreamParse->privtSubType;

            if((param->outbuflen >= param->outdatalen) && param->outbuf)
            {
                memcpy(param->outbuf ,prg->psdemAddr + strParseSize, pStreamParse->frameLen);
            }
            else if(NULL == param->outbuf)
            {
                param->outbuf = prg->psdemAddr + strParseSize;
            }
            return HIK_PSDEMUX_LIB_S_OK;
        }
        else
        {
        }
        return HIK_PSDEMUX_LIB_S_FAIL;
    }

    return HIK_PSDEMUX_LIB_S_OK;
}

HRESULT PSDEMUX_ClearFrame(void *handle)
{
    HIK_PSDEMUX_PRG *prg = (HIK_PSDEMUX_PRG *)handle;
    if (handle == NULL)
    {
        return HIK_PSDEMUX_LIB_E_PARA_NULL;
    }
    prg->frame_tmp.is_frame_end = 0;
    return HIK_PSDEMUX_LIB_S_OK;
}

HRESULT PSDEMUX_Reset(void *handle)
{
    HIK_PSDEMUX_PRG *prg = (HIK_PSDEMUX_PRG *)handle;
    if (handle == NULL)
    {
        return HIK_PSDEMUX_LIB_E_PARA_NULL;
    }
    
    prg->frameLen   =  0;
    prg->frame_tmp.is_frame_end = 0;
	prg->readIdx    =  0;
	prg->writeIdx   =  0;
    
    return HIK_PSDEMUX_LIB_S_OK;
}

/******************************************************************************
* 功  能：获取PSDemux所需内存大小
* 参  数：param - 参数结构指针
* 返回值：返回状态码
******************************************************************************/
HRESULT PSDEMUX_GetMemSize(PSDEMUX_PARAM *param)
{
    if (param == NULL)
    {
        return HIK_PSDEMUX_LIB_E_PARA_NULL;
    }

    param->buf_size = sizeof(HIK_PSDEMUX_PRG);
    
    //printf("PSDEMUX_GetMemSize buf_size %d\n",param->buf_size);

    return HIK_PSDEMUX_LIB_S_OK;
}

/******************************************************************************
* 功  能：创建PSDemux模块
* 参  数：param  	- 初始控制参数结构指针
*         handle	- 内部参数模块句柄
* 返回值：返回状态码
******************************************************************************/
HRESULT PSDEMUX_Create(PSDEMUX_PARAM *param, void **handle)
{
    HIK_PSDEMUX_PRG *prg;
    HIK_STREAM_INFO *info_ptr;
    HIK_VIDEO_INFO *video_info;
    HIK_AUDIO_INFO *audio_info;
    HIK_GLOBAL_TIME *global_time;
    PSDEMUX_DATA_OUTPUT *data_ptr;
    int i;

    if (param == NULL || handle == NULL)
    {
        printf("PSDEMUX_Create null \n");
        return HIK_PSDEMUX_LIB_E_PARA_NULL;
    }
    if (param->buf_size < sizeof(HIK_PSDEMUX_PRG))
    {    
        printf("PSDEMUX_Create buf_size error \n");
        return HIK_PSDEMUX_LIB_E_MEM_OVER;
    }

    prg = (HIK_PSDEMUX_PRG *)param->buffer;

    if (prg == NULL)
    {
        printf("PSDEMUX_Create prg error \n");
        return HIK_PSDEMUX_LIB_E_PARA_NULL;
    }

    info_ptr = &prg->info;
    data_ptr = &prg->frame_tmp;
    video_info  = &info_ptr->video_info;
    audio_info  = &info_ptr->audio_info;
    global_time = &info_ptr->glb_time;
    info_ptr->encrypt_type      = 0;       /* 加密模式 */
    info_ptr->is_hik_stream     = 0;
    info_ptr->dev_chan_id_flg   = 0;

    info_ptr->privt_stream_type = STREAM_TYPE_UNDEF;
    global_time->year           = 2000;        /* 全局时间年，2008 年输入2008*/
    global_time->month          = 1;       /* 全局时间月，1 - 12*/
    global_time->date           = 1;        /* 全局时间日，1 - 31*/
    global_time->hour           = 0;        /* 全局时间时，0 - 24*/
    global_time->minute         = 0;      /* 全局时间分，0 - 59*/
    global_time->second         = 0;      /* 全局时间秒，0 - 59*/
    global_time->msecond        = 0;     /* 全局时间毫秒，0 - 999*/

    video_info->stream_type        = STREAM_TYPE_UNDEF;
    video_info->encoder_version    = 0;    /* 编码器版本   */
    video_info->encoder_year       = 2000;       /* 编码器日期 年 */
    video_info->encoder_month      = 1;      /* 编码器日期 月 */
    video_info->encoder_date       = 1;       /* 编码器日期 日 */

    video_info->frame_num          = 0;
    video_info->width_orig         = 0;       /* 编码图像宽高，16的倍数*/
    video_info->height_orig        = 0;
    video_info->interlace          = 0;        /* 是否场编码   */
    video_info->b_frame_num        = 0;      /* b帧个数      */
    video_info->use_e_frame        = 0;      /* 是否使用e帧  */
    video_info->max_ref_num        = 0;      /* 最大参考帧个数 */
    video_info->fixed_frame_rate   = 0; /* 是否固定帧率 */
    video_info->time_info          = 1800;        /* 以1/45000 秒为单位的两帧间时间间隔 */
    video_info->watermark_type     = 0;   /* 水印类型 */
    video_info->deinterlace        = 0;
    video_info->play_clip          = 0;        /* 是否需要显示裁剪 */
    video_info->start_pos_x        = 0;     /* 裁剪后显示区域左上角点在原区域的坐标x */
    video_info->start_pos_y        = 0;     /* 裁剪后显示区域左上角点在原区域的坐标y */
    video_info->width_play         = 0;       /* 裁剪后显示区域的宽度 */
    video_info->height_play        = 0;      /* 裁剪后显示区域的高度 */

    audio_info->stream_type     = STREAM_TYPE_UNDEF;
    audio_info->frame_len       = 0;        /* 音频帧长度 */
    audio_info->audio_num       = 0;        /* 音频声道数 0 单声道，1 双声道*/
    audio_info->sample_rate     = 16000;      /* 音频采样率 */
    audio_info->bit_rate        = 16000;         /* 音频码率 */

    for (i = 0; i < 16; i++)
    {
        info_ptr->dev_chan_id[i]    = 0;    /* 设备和通道id */
    }

    data_ptr->output_type   = 0;
    data_ptr->channel_num   = param->channel_num;
    data_ptr->sys_clk_ref   = 0;
    data_ptr->encrypt       = 0;            /* 是否加密         */
    data_ptr->data_len      = 0;
    data_ptr->data_time_stamp = 0;
    data_ptr->is_frame_start  = 1;
    data_ptr->is_frame_end  = 0;
    data_ptr->is_nalu_end   = 0;
    data_ptr->have_userdata = 0;           /* 是否包含用户信息 */
    data_ptr->userdata      = NULL;           /* 用户信息指针     */
    data_ptr->stream_mode   = 0;
    data_ptr->info          = &prg->info;

    prg->stream_mode         = 0;        //  输出流类型
    prg->ps_stream_map_count = 0;        //ps stream map 计数器，大于0时表示流可用
    //prg->max_frame_size      = param->max_frame_size;//输出缓冲区的大小
    prg->sync_lost_flg       = 0;
    prg->video_stream_id    = 0xe0;     //视频流ID
    prg->audio_stream_id    = 0xc0;     //音频流ID
    prg->privt_stream_id    = 0xbd;     //私有流ID
    prg->frame_tmp.search_sps = 0;

    //prg->StreamOutputCallBack = param->output_callback_func;
    prg->ps_type = 1;//param->ps_type;

    *handle = (void *)prg;
    return HIK_PSDEMUX_LIB_S_OK;
}

/******************************************************************************
* 功  能：解析一段数据块
* 参  数：param  - 处理所需要的参数结构体
		  handle - 输入参数指针
* 返回值：返回错误码，函数返回后通过param的成员unprocessed_len告诉库外部还有多长
*         的数据没有解析，库外部下次要在未解析的数据后面接一段数据，下次送进库内
*         进行解析
******************************************************************************/
HRESULT PSDEMUX_Process(PSDEMUX_PROCESS_PARAM * param, void *handle)
{
    int ret = 0, jump = 0;
    //unsigned int  process_buf_len = 0;
    //unsigned char * process_buf = NULL;
    HIK_PSDEMUX_PRG *prg = (HIK_PSDEMUX_PRG *)handle;    
    HIK_STREAM_PARSE  *pStreamParse = NULL;

    if(param == NULL || prg == NULL)
    {
        return HIK_PSDEMUX_LIB_E_PARA_NULL;
    }

    pStreamParse          = (HIK_STREAM_PARSE  *)prg->psdemAddr;
    if(pStreamParse->bufUsed)
    {
        return HIK_PSDEMUX_LIB_E_MEM_OVER;
    }
    
    prg->writeIdx = param->orgWIdx;
    prg->dataLen  = prg->writeIdx >= prg->readIdx ? \
                       prg->writeIdx - prg->readIdx : prg->ringBufLen - prg->readIdx + prg->writeIdx;

    // 未发现ps stream map前，先搜索
    if (prg->ps_stream_map_count == 0)
    {
        ret = PSDEMUX_search_PSM(handle);
        /*搜索过以后都要重新计算有效数据 */
        prg->dataLen = prg->writeIdx >= prg->readIdx ? \
                       prg->writeIdx - prg->readIdx : prg->ringBufLen - prg->readIdx + prg->writeIdx;

        if (ret == NEED_MORE_DATA)
        {
            printf("PSDEMUX_search_PSM_NEED_MORE_DATA,r=%d,w=%d\n",prg->readIdx,prg->writeIdx);
            return HIK_PSDEMUX_LIB_S_OK;
        }
    }

    ret = 0;
    /* 处理缓冲区内数据 */
    do
    {
        ret = PSDEMUX_parse_PES(handle);

        if(0 <= ret)
        {
            prg->readIdx += ret;
            if (prg->readIdx >= prg->ringBufLen)
            {
                prg->readIdx -= prg->ringBufLen;
            }
            prg->dataLen -= ret;
        }
        
        if(prg->frame_tmp.is_frame_end)
        {
            ret = NEED_MORE_DATA;
            param->frmInBuf = 1;
            param->stamp    = prg->frame_tmp.data_time_stamp;
            param->frmType  = prg->frame_tmp.output_type;
            prg->frame_tmp.is_frame_end = 0;
        }
    }
    while (ret > 0);
    pStreamParse->streamType = prg->info.video_info.stream_type;
	pStreamParse->audEncType = prg->info.audio_info.stream_type;
    param->orgRIdx = prg->readIdx;
    switch (ret)
    {
        case HIK_PSDEMUX_LIB_E_MEM_OVER:
        case HIK_PSDEMUX_LIB_E_STM_ERR:
            /*I帧解析出错*/
            if (prg->readIdx != prg->writeIdx)
            {
                prg->readIdx += 4;
                if (prg->readIdx >= prg->ringBufLen)
                {
                    prg->readIdx -= prg->ringBufLen;
                }

                prg->dataLen -= 4;
                prg->ps_stream_map_count = 0;
            }
            param->orgRIdx = prg->readIdx;
            return ret;
        case HIK_PSDEMUX_LIB_E_SYNC_LOST:
            /* 重新搜索起始码 */
            if (prg->readIdx != prg->writeIdx)
            {
                prg->readIdx += 4;
                if (prg->readIdx >= prg->ringBufLen)
                {
                    prg->readIdx -= prg->ringBufLen;
                }

                prg->dataLen -= 4;
                jump = PSDEMUX_search_start_code(handle);
				if (jump <=0)
				{
				    return HIK_PSDEMUX_LIB_S_FAIL;
				}
                /*搜索过以后都要重新计算有效数据 */
                prg->dataLen = prg->writeIdx >= prg->readIdx ? \
                                      prg->writeIdx - prg->readIdx : prg->ringBufLen - prg->readIdx + prg->writeIdx;
            }
            param->orgRIdx = prg->readIdx;
            return HIK_PSDEMUX_LIB_S_OK;
        case NEED_MORE_DATA:
            return HIK_PSDEMUX_LIB_S_OK;
        default:
            return HIK_PSDEMUX_LIB_E_PARA_NULL;
    }
}

/******************************************************************************
* 功  能：设置输出缓冲区
* 参  数：	buffer - 输出缓冲区
            size   - 缓冲区大小
*			handle - 输入参数指针
* 返回值：返回错误码
******************************************************************************/
//HRESULT PSDEMUX_SetOutBuf(unsigned char *buffer, unsigned int size, void *handle)
//{
//	HIK_PSDEMUX_PRG *prg = (HIK_PSDEMUX_PRG *)handle;
//
//    if(buffer == NULL || prg == NULL)
//    {
//		return HIK_PSDEMUX_LIB_E_PARA_NULL;
//    }
//    prg->frame_tmp.data_buffer	= buffer;
//    prg->max_frame_size			= size;
//	return HIK_PSDEMUX_LIB_S_OK;
//}
