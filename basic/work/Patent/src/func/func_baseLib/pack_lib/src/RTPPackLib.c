/********************************************************************************
*
* 版权信息：版权所有 (c) 2012, 杭州海康威视数字技术有限公司, 保留所有权利
*
* 文件名称：RTPPackLib.h
* 文件标识：HIK_RTPPACK_LIB
* 摘    要：海康威视 RTPPACK 库的接口函数声明文件
*
* 当前版本：2.00.11
* 作    者：Play SDK
* 日    期：2015年06月23日
* 备    注：
*2008.09.18 海康标准RTP封装，对不是4字节对齐的包进行对齐加padding，不用回调函数输出
*           输出缓冲区内的每个RTP包前加4字节表示长度。
*2009.02.09 根据重新定义的私有数据格式进行打包
*2009.06.08 添加私有数据打包接口RTPPACK_create_privt_info_rtp (黄崇基)
*2009.11.10 增加对AMR音频的打包支持(rfc4867)，打包时每次输入一帧数据
*2010.02.23 修改mjpeg打包取出文件头部分，并添加一些接口
*2010.06.21 针对低延时应用增加了每次输入数据为非完整一个nalu或非完整一帧的打包功能支持
*2012.08.11 在对AAC去头时判断，增加是否是音频帧的条件，并升级版本至2.00.00
*2012.10.12 增加mepg2编码的rtp打包方式，并升级版本至2.00.01
*2013.02.26 增加支持加入私有包，同时，将音视频和私有包的ssrc和包序号分开打。
*2013.07.05 增加在basic私有描述子中加入公司和相机信息，版本升至2.00.02
*2013.07.23 增加在mpeg2打包中specific header的TR字段
*2013.10.24 修改私有数据一个包打不下的情况，版本号升至2.00.03
*2013.11.13 统一ts，ps，rtp私有描述子打包文件
*2014.01.13 添加SVAC码流打包,版本号升级至2.00.04
*2014.03.01 添加H264加密码流打包,版本号升级至2.00.05
*2014.05.14 添加mpeg4、mjpeg加密码流打包,版本号升级至2.00.06
*2014.06.23 添加视频描述子中的轻存储标记,版本号升级至2.00.07
*2014.07.14 统一ts，ps，rtp私有描述子打包文件版本号升级至2.00.08
*2014.09.12 增加H265打包，版本号升级至2.00.09
*2015.01.04 增加温度信息打包，版本号升级至2.00.10
*2015.06.23 增加H.265加密打包，版本号升级至2.00.11
********************************************************************************/

#include <stdlib.h>
#include <memory.h>
#include "RTPPackLib.h"
#include "descriptor_set.h"
#include "stream_bits_info_def.h"


#ifdef SRTP_SUPPORT
#include "hik_srtp.h"
#endif

#define RTP_DEF_H264_FU_A   0x1c                        ///< Fragmentation unit
#define RTPPACK_MIN(a, b)   (((a) < (b)) ? (a) : (b))   ///< 取两个长度的较小值
#define RTPPACK_MAX(a, b)   (((a) > (b)) ? (a) : (b))   ///< 取两个长度的较大值
#define RTPPACK_HEAD_LEN    12                          ///< RTP12字节标准头长度
#define RTPMAX_PACKET_SIZE  (8 * 1024)                  ///< RTP包最大长度限制
#define RTPMIN_PACKET_SIZE  1024                        ///< RTP包最小长度限制
#define PRIVATE_HEAD_LEN     12                          ///< hik私有数据包头长度

#define RTP_MAX_STREAMNUM   3

/** @struct _HIK_RTP_PROGRAM_
 *  @brief  RTP 打包过程参数
 */
typedef struct _HIK_RTP_PROGRAM_
{
    unsigned int    time_stamp_cur;     ///<当前帧时标
    unsigned int    packed_len_cur;     ///<当前已打包的长度

    unsigned int    stream_mode;        ///<输入流模式
    unsigned int    watermark_mode;     ///<水印输入模式，比如哪些帧需要加水印
    unsigned int    video_payload_type; ///<所负载的原始流rtp头PT
    unsigned int    audio_payload_type; ///<音频PT值
    unsigned int    SSRC;               ///<rtp头SSRC

    unsigned int    video_ssrc;         ///<rtp包视频数据同步源的识别标志，以区分不同的rtp发送端
    unsigned int    audio_ssrc;         ///<rtp包音频数据同步源的识别标志，以区分不同的rtp发送端
    unsigned int    privt_ssrc;         ///<rtp包私有数据同步源的识别标志，以区分不同的rtp发送端

    unsigned int    video_seq_num;      ///<视频rtp包初始seq_num
    unsigned int    audio_seq_num;      ///<音频rtp包初始seq_num
    unsigned int    privt_seq_num;      ///<私有rtp包初始seq_num

//  unsigned int    seq_num;            ///<rtp头seq_num
    unsigned int    max_rtp_len;        ///<最大 rtp 包长度
    unsigned int    rtp_count;          ///<当前缓冲区内rtp包个数
    unsigned int    allow_ext;          ///<是否允许rtp头扩展
    unsigned int    video_stream_type;  ///<输入视频流类型
    unsigned int    audio_stream_type;  ///<输入音频流类型
    unsigned int    video_clip;         ///<视频是否需要裁剪
    unsigned int    allow_4byte_align;  ///<是否需要包长4字节对齐
    unsigned int    is_last_frame_sid;  ///<打包amr需要，记录上一帧是否为舒适噪声
    unsigned int    nalu_first_byte;    ///<h.264/h.265nalu的第一个字节
    unsigned int    nalu_second_byte;   ///<h.265nalu的第二个字节

    //描述子内存空间
    unsigned char   device_dsc[HIK_DEVICE_DESCRIPTOR_LEN];         ///<设备描述字
    unsigned char   video_dsc[HIK_VIDEO_DESCRIPTOR_LEN];           ///<视频描述字
    unsigned char   audio_dsc[HIK_AUDIO_DESCRIPTOR_LEN];           ///<音频描述字
    unsigned char   video_clip_dsc[HIK_VIDEO_CLIP_DESCRIPTOR_LEN]; ///<显示裁剪描述字

    //私有头12字节
    unsigned char   private_data_header[PRIVATE_HEAD_LEN];  ///< 12字节备份
    unsigned int    totle_packet;                          ///< 一个ivs索引共需几个rtp包
    unsigned int    current_packet;                        ///< 当前ivs索引包

    //加密扩展头
    unsigned char   encrypt_round;   ///< 填入加密轮数
    unsigned char   encrypt_type;   ///< 加密类型
    unsigned char   encrypt_arith;  ///< 加密算法
    unsigned char   packet_type;     ///< 打包类型
    unsigned char   key_len;         ///< 密匙长度
    unsigned char   reserved[3];

    #ifdef SRTP_SUPPORT

    ///< 加密信息
    SRTP_INFO       srtp_info[RTP_MAX_STREAMNUM];

    #endif
} HIK_RTPPACK_PRG;

#ifdef SRTP_SUPPORT
static int RTPPACK_Srtp_Pack(HIK_RTPPACK_PRG *prg, unsigned char *rtp_buf, unsigned int rtp_len);
#endif
/** @fn      RTPPACK_fill_rtp_header(unsigned char * buffer, HIK_RTPPACK_PRG *prg, unsigned int payload_type, unsigned int set_mark_bit)
 *  @brief   生成标准 rtp 头数据，如需扩展，扩展后的长度在调用此函数后添加。此函数只返回12字节标准头长度
 *  @param   buffer       [IN] RTP头缓冲区
 *  @param   prg          [IN] 打包信息
 *  @param   payload_type [IN] 负载类型
 *  @param   is_last_pack [IN] 是否是最后一个rtp包
 *  @return  RTP标准头数据的长度(12bytes)
 */
int RTPPACK_fill_rtp_header(unsigned char *buffer,
                            HIK_RTPPACK_PRG *prg,
                            unsigned int payload_type,
                            unsigned int set_mark_bit,
                            unsigned int *tag_len)
{
    unsigned int pos = 0;
    unsigned int time_stamp = prg->time_stamp_cur;
    unsigned int ssrc = prg->SSRC;
    //unsigned int video_ssrc = prg->video_ssrc;
    //unsigned int audio_ssrc = prg->audio_ssrc;
    //unsigned int privt_ssrc = prg->privt_ssrc;
    unsigned int need_extend = 0;

    int srtp_index  = -1;

    if ((HIK_RTPPACK_SECRET_TYPE_ZERO != prg->encrypt_type)
        && (HIK_RTPPACK_SECRET_ARITH_NONE != prg->encrypt_arith)
        && (HIK_RTPPACK_SECRET_ROUND00 != prg->encrypt_round)
        && (HIK_RTPPACK_SECRET_KEYLEN000 != prg->key_len)
        && (HIK_RTPPACK_SECRET_PACKET_FUA == prg->packet_type))
    {
        need_extend = 1;
    }

    buffer[pos++]   = 0x80                                   ///< V = 2
                | 0                                      ///< 是否有填充，需要到最后修改
                | (((prg->allow_ext && (need_extend || payload_type == HIK_RTPPACK_PRIVT_PAYLOAD_TYPE)) != 0) << 4)   ///< 是否包含扩展字段
                | 0;                                     ///< CSRC_Count = 0
    buffer[pos++]   = (unsigned char)((set_mark_bit & 1) << 7) | (payload_type & 0x7f);  ///< 1bit markbit位，7bit PT值

    if (prg->video_payload_type == payload_type)
    {
        buffer[pos++]   = (unsigned char)(prg->video_seq_num >> 8);    ///< 包序号高字节
        buffer[pos++]   = (unsigned char)(prg->video_seq_num & 0xff);    ///< 包序号低字节

        buffer[pos++]   = (unsigned char)(time_stamp >> 24);     ///< 4字节RTP包时间戳
        buffer[pos++]   = (unsigned char)(time_stamp >> 16);
        buffer[pos++]   = (unsigned char)(time_stamp >> 8);
        buffer[pos++]   = (unsigned char)(time_stamp & 0xff);

        buffer[pos++]   = (unsigned char)(ssrc >> 24);           ///< 4字节SSCR值
        buffer[pos++]   = (unsigned char)(ssrc >> 16);
        buffer[pos++]   = (unsigned char)(ssrc >> 8);
        buffer[pos++]   = (unsigned char)(ssrc & 0xff);

        #ifdef SRTP_SUPPORT

        srtp_index      = hik_srtp_getindex(prg->srtp_info, RTP_MAX_STREAMNUM,ssrc);

        #endif

        prg->video_seq_num++;
    }
    else if (prg->audio_payload_type == payload_type)
    {
        buffer[pos++]   = (unsigned char)(prg->audio_seq_num >> 8);    ///< 包序号高字节
        buffer[pos++]   = (unsigned char)(prg->audio_seq_num & 0xff);    ///< 包序号低字节

        buffer[pos++]   = (unsigned char)(time_stamp >> 24);     ///< 4字节RTP包时间戳
        buffer[pos++]   = (unsigned char)(time_stamp >> 16);
        buffer[pos++]   = (unsigned char)(time_stamp >> 8);
        buffer[pos++]   = (unsigned char)(time_stamp & 0xff);

        buffer[pos++]   = (unsigned char)(ssrc >> 24);           ///< 4字节SSCR值
        buffer[pos++]   = (unsigned char)(ssrc >> 16);
        buffer[pos++]   = (unsigned char)(ssrc >> 8);
        buffer[pos++]   = (unsigned char)(ssrc & 0xff);

        #ifdef SRTP_SUPPORT

        srtp_index      = hik_srtp_getindex(prg->srtp_info, RTP_MAX_STREAMNUM, ssrc);

        #endif

        prg->audio_seq_num++;
    }
    else if (payload_type == HIK_RTPPACK_PRIVT_PAYLOAD_TYPE)
    {
        buffer[pos++]   = (unsigned char)(prg->privt_seq_num >> 8);    ///< 包序号高字节
        buffer[pos++]   = (unsigned char)(prg->privt_seq_num & 0xff);    ///< 包序号低字节

        buffer[pos++]   = (unsigned char)(time_stamp >> 24);     ///< 4字节RTP包时间戳
        buffer[pos++]   = (unsigned char)(time_stamp >> 16);
        buffer[pos++]   = (unsigned char)(time_stamp >> 8);
        buffer[pos++]   = (unsigned char)(time_stamp & 0xff);

        buffer[pos++]   = (unsigned char)(ssrc >> 24);           ///< 4字节SSCR值
        buffer[pos++]   = (unsigned char)(ssrc >> 16);
        buffer[pos++]   = (unsigned char)(ssrc >> 8);
        buffer[pos++]   = (unsigned char)(ssrc & 0xff);

        #ifdef SRTP_SUPPORT

        srtp_index      = hik_srtp_getindex(prg->srtp_info, RTP_MAX_STREAMNUM, ssrc);

        #endif

        prg->privt_seq_num++;
    }

    if (-1 != srtp_index)
    {
        #ifdef SRTP_SUPPORT

        if (SEC_SERV_AUTH & prg->srtp_info[srtp_index].sec_serv)
        {
            *tag_len = prg->srtp_info[srtp_index].auth_tag_len;
        }
        else
        {
            *tag_len = 0;
        }

        #endif

    }
    else
    {
        *tag_len = 0;
    }

    prg->rtp_count++;
    return RTPPACK_HEAD_LEN;        ///< 返回12字节长度，如有扩展字段，扩展长度在调用这个函数后添加
}

/** @fn      RTPPACK_create_basic_stream_info_rtp(unsigned char * buffer, HIK_RTPPACK_PRG *prg, RTPPACK_PROCESS_PARAM *prc, unsigned int *tag_len)
 *  @brief   生成负载长度为0，只包含海康私有数据 basic_stream_info 的RTP包
 *  @param   buffer       [IN] rtp缓冲区
 *  @param   prg          [IN] 打包参考信息
 *  @param   prc          [IN] 处理参数
 *  @param   tag_len      [IN] srtp相关
 *  @return  生成rtp包的长度
 */
int RTPPACK_create_basic_stream_info_rtp(   unsigned char * buffer,
                                            HIK_RTPPACK_PRG *prg,
                                            RTPPACK_PROCESS_PARAM *prc,
                                            unsigned int *tag_len)
{
    int length = 16;  ///< 16表示RTP包12字节标准头+4字节扩展头长度
    int length_int;
    // 填充rtp头
    RTPPACK_fill_rtp_header(buffer, prg, HIK_RTPPACK_PRIVT_PAYLOAD_TYPE, 1, tag_len);

    buffer[12] = (unsigned char)(HIK_PRIVT_BASIC_STREAM_INFO >> 8);
    buffer[13] = (unsigned char)(HIK_PRIVT_BASIC_STREAM_INFO);

    // 填充全局描述字和流信息码流字
    length += HKDSC_fill_basic_descriptor(&buffer[length], &prc->global_time, prg->encrypt_type,prc->company_mark, prc->camera_mark);
    length += HKDSC_fill_stream_descriptor(&buffer[length],
                                            prg->video_stream_type,
                                            prg->audio_stream_type,
                                            prc->frame_num);
    memcpy(&buffer[length], prg->device_dsc, HIK_DEVICE_DESCRIPTOR_LEN);  ///< 填充设备描述字
    length +=  HIK_DEVICE_DESCRIPTOR_LEN;

    // 以4字节为单位，表示私有数据长度
    length_int = (length - 16) >> 2;
    buffer[14] = (length_int >> 8);
    buffer[15] = (length_int & 0xff);

    return length;
}

/** @fn      RTPPACK_create_codec_info_rtp(unsigned char * buffer, HIK_RTPPACK_PRG *prg)
 *  @brief   生成负载长度为0，只包含海康私有数据 codec info 的RTP包
 *  @param   buffer       [IN] rtp缓冲区
 *  @param   prg          [IN] 打包参考信息
 *  @return  生成rtp包的长度
 */
int RTPPACK_create_codec_info_rtp(unsigned char * buffer, HIK_RTPPACK_PRG *prg, unsigned int *tag_len)
{
    int length = 16;  ///< 16表示RTP包12字节标准头+4字节扩展头长度
    int length_int;
    // 填充rtp包头部
    RTPPACK_fill_rtp_header(buffer, prg, HIK_RTPPACK_PRIVT_PAYLOAD_TYPE, 1, tag_len);

    buffer[12] = (unsigned char)(HIK_PRIVT_BASIC_STREAM_INFO >> 8);
    buffer[13] = (unsigned char)(HIK_PRIVT_BASIC_STREAM_INFO);

    if (prg->stream_mode & INCLUDE_VIDEO_STREAM)
    {
        memcpy(&buffer[length], prg->video_dsc, HIK_VIDEO_DESCRIPTOR_LEN);  ///< 填充视频描述字
        length         +=  HIK_VIDEO_DESCRIPTOR_LEN;
        if (prg->video_clip)
        {
            memcpy(&buffer[length], prg->video_clip_dsc, HIK_VIDEO_CLIP_DESCRIPTOR_LEN); ///< 填充显示裁剪描述字
            length         +=  HIK_VIDEO_CLIP_DESCRIPTOR_LEN;
        }
    }
    if (prg->stream_mode & INCLUDE_AUDIO_STREAM)
    {
        memcpy(&buffer[length], prg->audio_dsc, HIK_AUDIO_DESCRIPTOR_LEN);  ///< 填充音频描述字
        length         +=  HIK_AUDIO_DESCRIPTOR_LEN;
    }

    // 以4字节为单位，表示私有数据长度
    length_int = (length - 16) >> 2;
    buffer[14] = (length_int >> 8);
    buffer[15] = (length_int & 0xff);

    return length;
}

/** @fn      RTPPACK_create_privt_info_rtp(unsigned char *buffer, HIK_RTPPACK_PRG *prg, RTPPACK_PROCESS_PARAM *prc)
 *  @brief   生成负载长度为0，只包含海康私有数据(除codec_info和basic_stream_info以外的其他私有数据)的RTP包
 *  @param   buffer       [IN] rtp缓冲区
 *  @param   prg          [IN] 打包参考信息
 *  @param   prc          [IN] 处理参数
 *  @return  生成rtp包的长度
 */
int RTPPACK_create_privt_info_rtp(unsigned char *buffer, HIK_RTPPACK_PRG *prg,
                                  RTPPACK_PROCESS_PARAM *prc, unsigned int *tag_len)
{
    unsigned int copy_len = prc->unit_in_len - prg->packed_len_cur;
    unsigned int set_mark_bit = 0;
    unsigned int padding_size = 0;
    unsigned int packet_len;
    unsigned int pos = 0;

    if (copy_len <= prg->max_rtp_len - RTPPACK_HEAD_LEN) ///< 最后一个私有数据包mark_bit置1
    {
        set_mark_bit = 1;
    }
    else
    {
        copy_len = prg->max_rtp_len - RTPPACK_HEAD_LEN - PRIVATE_HEAD_LEN; // 拷贝数据长度
    }

    if ((0x10 == prc->unit_in_buf[0] && 0x02 == prc->unit_in_buf[1])
      ||(0x01 == prc->unit_in_buf[0] && 0x01 == prc->unit_in_buf[1]))
    {
        set_mark_bit = 1;
    }

    pos = RTPPACK_fill_rtp_header(buffer, prg, HIK_RTPPACK_PRIVT_PAYLOAD_TYPE, set_mark_bit, tag_len);///< 填充rtp包头部

    // 一个私有包打不下的情况
    if (prc->unit_in_len > prg->max_rtp_len - RTPPACK_HEAD_LEN - PRIVATE_HEAD_LEN)
    {
        if (0 == prg->packed_len_cur)
        {
            prg->totle_packet = (prc->unit_in_len - PRIVATE_HEAD_LEN)
                                / (prg->max_rtp_len - RTPPACK_HEAD_LEN - PRIVATE_HEAD_LEN) + 1;

            memcpy(prg->private_data_header, prc->unit_in_buf, PRIVATE_HEAD_LEN);
        }

        // +8 是私有头除type、len字段外其它共8字节, 四字节为单位
        prg->private_data_header[2] = ((copy_len + 8) / 4) >> 8;
        prg->private_data_header[3] = ((copy_len + 8) / 4);

        // IVS索引需修改包编号字段
        if ( (0x10 == prc->unit_in_buf[0] && 0x02 == prc->unit_in_buf[1])
           || (0x01 == prc->unit_in_buf[0] && 0x01 == prc->unit_in_buf[1]))
        {
            // 修改total_pack_count 总共包数量
            prg->private_data_header[7] = prg->totle_packet;
            prg->private_data_header[8] = prg->current_packet++;
        }

        //拷贝12字节头
        memcpy(&buffer[pos], prg->private_data_header, PRIVATE_HEAD_LEN);

        //私有数据，第一次外面送了12字节所有头，得去掉
        if (0 == prg->packed_len_cur)
        {
            prg->packed_len_cur +=  PRIVATE_HEAD_LEN;
        }

        memcpy(&buffer[pos + PRIVATE_HEAD_LEN],  &prc->unit_in_buf[prg->packed_len_cur], copy_len);

        prg->packed_len_cur += copy_len;
        packet_len = copy_len + RTPPACK_HEAD_LEN + PRIVATE_HEAD_LEN;
    }
    else
    {
        memcpy(&buffer[pos], &prc->unit_in_buf[prg->packed_len_cur], copy_len);
        prg->packed_len_cur += copy_len;
        packet_len = copy_len + RTPPACK_HEAD_LEN;
    }

    //当包长不是4字节对齐的时候，填充以对齐
    if ((packet_len & 3) && prg->allow_4byte_align)
    {
        int i = 0;
        padding_size = 4 - (packet_len & 3);

        for (i = 0; i < padding_size; i++)
        {
            buffer[packet_len++] = 0x00;
        }

        buffer[packet_len - 1] = (unsigned char)padding_size; ///< 填充字段最后一位表示填充字段长度
        buffer[0] |= 0x20;    ///< 修改padding位
    }

    return packet_len;
}

/** @fn      RTPPACK_start_new_nalu_h265(unsigned char *buffer, unsigned int buf_size, HIK_RTPPACK_PRG *prg, RTPPACK_PROCESS_PARAM *prc)
 *  @brief   打包一个新的h265 nalu
 *  @param   buffer       [IN] 目标缓冲区
 *  @param   buf_size     [IN] 目标缓冲区最大长度
 *  @param   prg          [IN] 处理参数
 *  @param   prc          [IN] 输入参数
 *  @return  生成数据的长度
 */
int RTPPACK_start_new_nalu_h265(unsigned char *buffer,
                                unsigned int buf_size,
                                HIK_RTPPACK_PRG *prg,
                                RTPPACK_PROCESS_PARAM *prc)
{
    unsigned int nalu_len = prc->unit_in_len - 4;    ///< start code 0x00 00 00 01不计入总长度
    unsigned char *nalu_buf = prc->unit_in_buf + 4;
    int copy_len = 0;
    unsigned int nal_unit_type = (nalu_buf[0] & 0x7E) >> 1;
    prg->packed_len_cur = 0;

    //当该nalu可以在一个rtp包内发送时，直接打包发送
    if (nalu_len <= buf_size && prc->is_unit_end)
    {
        memcpy(buffer, nalu_buf, nalu_len);
        prg->packed_len_cur = prc->unit_in_len;
        return nalu_len;
    }
    //当该nalu不能在一个包内发送或非完整的一个nalu时，需要打成fragment unit的形式
    else
    {
        copy_len = RTPPACK_MIN(buf_size - 3, nalu_len - 2);
        copy_len = RTPPACK_MAX(copy_len, 0);
        memcpy(&buffer[3], nalu_buf + 2, copy_len);
        buffer[0] = (nalu_buf[0] & 0x81) | (49 << 1);   // Payload header (1st byte)
        buffer[1] = nalu_buf[1];                        // Payload header (2nd byte)
        buffer[2] = 0x80 | nal_unit_type;               // FU header (with S bit)
        prg->packed_len_cur = copy_len + 4 + 2;  // +4跳过起始码，+2表示type字节
        return (copy_len + 3); // +3表示fus
    }
}

/** @fn      RTPPACK_continue_fragment_nalu_h265(unsigned char *buffer, unsigned int buf_size, HIK_RTPPACK_PRG *prg, RTPPACK_PROCESS_PARAM *prc)
 *  @brief   当上一次nalu没有发送完成时，继续发送fragment_unit
 *  @param   buffer       [IN] 目标缓冲区
 *  @param   buf_size     [IN] 目标缓冲区最大长度
 *  @param   prg          [IN] 处理参数
 *  @param   prc          [IN] 输入参数
 *  @return  生成数据的长度
 */
int RTPPACK_continue_fragment_nalu_h265(unsigned char *buffer,
                                        unsigned int buf_size,
                                        HIK_RTPPACK_PRG *prg,
                                        RTPPACK_PROCESS_PARAM *prc)
{
    unsigned int copy_len, rest_len;
    unsigned char *nalu_buf = prc->unit_in_buf;
    unsigned char firs_byte = (unsigned char)prg->nalu_first_byte;

    buffer[0] = (firs_byte & 0x81) | (49 << 1);       ///< Payload header (1st byte)
    buffer[1] = (unsigned char)prg->nalu_second_byte; ///< Payload header (2nd byte)

    rest_len = prc->unit_in_len - prg->packed_len_cur;

    if ((rest_len + 3 <= buf_size) && prc->is_unit_end)   ///< 如果该包可以将nalu剩下的部分完全发送
    {
        buffer[2]   = (firs_byte & 0x3f) | 0x40;          ///< FU header
    }
    else///< 如果该包不能将nalu剩下的部分完全发送
    {
        buffer[2]   = (firs_byte & 0x3f) | 0x00;          ///< FU header
    }

    copy_len = RTPPACK_MIN(buf_size - 3, rest_len);
    memcpy(&buffer[3], &nalu_buf[prg->packed_len_cur], copy_len);
    prg->packed_len_cur += copy_len;
    return copy_len + 3;
}


/** @fn      RTPPACK_start_new_nalu_h264(unsigned char *buffer, unsigned int buf_size, HIK_RTPPACK_PRG *prg, RTPPACK_PROCESS_PARAM *prc)
 *  @brief   打包一个新的h264 nalu
 *  @param   buffer       [IN] 目标缓冲区
 *  @param   buf_size     [IN] 目标缓冲区最大长度
 *  @param   prg          [IN] 处理参数
 *  @param   prc          [IN] 输入参数
 *  @return  生成数据的长度
 */
int RTPPACK_start_new_nalu_h264(unsigned char *buffer,
                                unsigned int buf_size,
                                HIK_RTPPACK_PRG *prg,
                                RTPPACK_PROCESS_PARAM *prc)
{
    unsigned int nalu_len = prc->unit_in_len - 4;    ///< start code 0x00 00 00 01不计入总长度
    unsigned char *nalu_buf = prc->unit_in_buf + 4;
    int copy_len;
    prg->packed_len_cur = 0;

    //当该nalu可以在一个rtp包内发送时，直接打包发送
    if (nalu_len <= buf_size && prc->is_unit_end)
    {
        memcpy(buffer, nalu_buf, nalu_len);
        prg->packed_len_cur = prc->unit_in_len;
        return nalu_len;
    }
    //当该nalu不能在一个包内发送或非完整的一个nalu时，需要打成fragment unit的形式
    else
    {
        copy_len = RTPPACK_MIN(buf_size - 2, nalu_len - 1);
        copy_len = RTPPACK_MAX(copy_len, 0);
        memcpy(&buffer[2], nalu_buf + 1, copy_len);
        buffer[0] = (nalu_buf[0] & 0xe0) | RTP_DEF_H264_FU_A;   ///< FU indicator
        buffer[1] = (nalu_buf[0] & 0x1f) | 0x80;                ///< FU header
        prg->packed_len_cur = copy_len + 4 + 1;  // +4跳过起始码，+1表示type字节
        return (copy_len + 2); // +2表示fu_a
    }
}

/** @fn      RTPPACK_start_new_nalu_h264(unsigned char *buffer, unsigned int buf_size, HIK_RTPPACK_PRG *prg, RTPPACK_PROCESS_PARAM *prc)
 *  @brief   打包一个新的h264 nalu
 *  @param   buffer       [IN] 目标缓冲区
 *  @param   buf_size     [IN] 目标缓冲区最大长度
 *  @param   prg          [IN] 处理参数
 *  @param   prc          [IN] 输入参数
 *  @return  生成数据的长度
 */
int RTPPACK_start_new_nalu_h264_encrypt(unsigned char *buffer,
                                        unsigned int buf_size,
                                        HIK_RTPPACK_PRG *prg,
                                        RTPPACK_PROCESS_PARAM *prc)
{
    unsigned int nalu_len = prc->unit_in_len - 4;    ///< start code 0x00 00 00 01不计入总长度
    unsigned char *nalu_buf = prc->unit_in_buf + 4;
    int copy_len;
    prg->packed_len_cur = 0;

    //当该nalu + 1可以在一个rtp包内发送时，直接打包发送,+1表示标记nalu类型的字节
    if (nalu_len + 1 <= buf_size && prc->is_unit_end)
    {
        buffer[0] = prc->nalu_tab;
        memcpy(buffer + 1, nalu_buf, nalu_len);
        prg->packed_len_cur = prc->unit_in_len;

        return nalu_len + 1;
    }
    //不能在一个包内发送或非完整的一个nalu时，需要打成fragment unit的形式
    else
    {
        copy_len = RTPPACK_MIN(buf_size - 2, nalu_len);
        copy_len = RTPPACK_MAX(copy_len, 0);

        memcpy(&buffer[2], nalu_buf, copy_len);
        buffer[0] = (prc->nalu_tab & 0xe0) | RTP_DEF_H264_FU_A;   ///< FU indicator
        buffer[1] = (prc->nalu_tab & 0x1f) | 0x80;                ///< FU header

        prg->packed_len_cur = copy_len + 4;

        return (copy_len + 2);
    }
}

/** @fn      RTPPACK_continue_fragment_nalu_h264(unsigned char *buffer, unsigned int buf_size, HIK_RTPPACK_PRG *prg, RTPPACK_PROCESS_PARAM *prc)
 *  @brief   当上一次nalu没有发送完成时，继续发送fragment_unit
 *  @param   buffer       [IN] 目标缓冲区
 *  @param   buf_size     [IN] 目标缓冲区最大长度
 *  @param   prg          [IN] 处理参数
 *  @param   prc          [IN] 输入参数
 *  @return  生成数据的长度
 */
int RTPPACK_continue_fragment_nalu_h264(unsigned char *buffer,
                                        unsigned int buf_size,
                                        HIK_RTPPACK_PRG *prg,
                                        RTPPACK_PROCESS_PARAM *prc)
{
    unsigned int copy_len, rest_len;
    unsigned char *nalu_buf = prc->unit_in_buf;
    unsigned char firs_byte = (unsigned char)prg->nalu_first_byte;

    buffer[0] = (firs_byte & 0x60) | RTP_DEF_H264_FU_A;   ///< FU indicator
    rest_len = prc->unit_in_len - prg->packed_len_cur;


    if ((rest_len + 2 <= buf_size) && prc->is_unit_end)   ///< 如果该包可以将nalu剩下的部分完全发送
    {
        buffer[1]   = (firs_byte & 0x1f) | 0x40;          ///< FU header
    }
    else///< 如果该包不能将nalu剩下的部分完全发送
    {
        buffer[1]   = (firs_byte & 0x1f) | 0x00;          ///< FU header
    }
    copy_len = RTPPACK_MIN(buf_size - 2, rest_len);
    memcpy(&buffer[2], &nalu_buf[prg->packed_len_cur], copy_len);
    prg->packed_len_cur += copy_len;
    return copy_len + 2;
}

/** @fn      RTPPACK_fill_rtp_expend(unsigned char *buffer, HIK_RTPPACK_PRG *prg)
*  @brief   根据需要生成rtp包
*  @param   buffer       [IN] rtp头扩展部分缓冲区
*  @param   prg          [IN] 打包参考信息
*  @param   prc          [IN] 处理参数
*  @return  生成数据的长度
*/
int RTPPACK_fill_rtp_expend(unsigned char *buffer, HIK_RTPPACK_PRG *prg)
{
    unsigned int pos        = 0;
    unsigned int length_int = 0;

    // 打加密码流扩展结构体
    if (prg->allow_ext
        && (HIK_RTPPACK_SECRET_TYPE_ZERO != prg->encrypt_type)
        && (HIK_RTPPACK_SECRET_ARITH_NONE != prg->encrypt_arith)
        && (HIK_RTPPACK_SECRET_ROUND00 != prg->encrypt_round)
        && (HIK_RTPPACK_SECRET_KEYLEN000 != prg->key_len))
    {
        // private type
        buffer[pos++] = (unsigned char)(HIK_PRIVT_ENCRYPT_INFO >> 8);
        buffer[pos++] = (unsigned char)(HIK_PRIVT_ENCRYPT_INFO);

        // 以4字节为单位，表示私有数据长度
        length_int = HIK_ENCRYPT_INFO_LEN >> 2;
        buffer[pos++] = (length_int >> 8);
        buffer[pos++] = (length_int & 0xff);

        // tag
        buffer[pos++] = HIK_STREAM_ENCRYPT_TAG;

        // length
        buffer[pos++] = HIK_ENCRYPT_INFO_LEN - 2;

        // version
        buffer[pos++] = 0x00;
        buffer[pos++] = 0x01;

        // packed type、encrypt arithmetic
        buffer[pos++] = (prg->packet_type << 4)
                      | (prg->encrypt_arith);

        // encrypt round、key length
        buffer[pos++] = (prg->encrypt_round << 4)
                      | (prg->key_len);

        // encrypt type
        buffer[pos++] = prg->encrypt_type;

        // reserved
        buffer[pos++] = 0x01;
    }

    return pos;
}

/** @fn      RTPPACK_fill_rtp_pack(unsigned char *buffer, HIK_RTPPACK_PRG *prg, RTPPACK_PROCESS_PARAM *prc)
 *  @brief   根据需要生成rtp包
 *  @param   buffer       [IN] rtp头扩展部分缓冲区
 *  @param   prg          [IN] 打包参考信息
 *  @param   prc          [IN] 处理参数
 *  @return  生成数据的长度
 */
int RTPPACK_fill_rtp_pack(unsigned char *buffer,
                          HIK_RTPPACK_PRG *prg,
                          RTPPACK_PROCESS_PARAM *prc,
                          unsigned int *tag_len)
{
    unsigned int    copy_len, rest_len, padding_len;
    unsigned int    pos = RTPPACK_HEAD_LEN;         ///< 标准头部分先跳过不填充
    unsigned int    set_mark_bit = 0;
    unsigned char * src_buf = &(prc->unit_in_buf[prg->packed_len_cur]);
    unsigned int    payload_type = prg->video_payload_type;
    unsigned char   frame_type = 0;
    unsigned char   is_encrypt = 0;

    if (   (HIK_RTPPACK_SECRET_TYPE_ZERO != prg->encrypt_type)
        && (HIK_RTPPACK_SECRET_ARITH_NONE != prg->encrypt_arith)
        && (HIK_RTPPACK_SECRET_ROUND00 != prg->encrypt_round)
        && (HIK_RTPPACK_SECRET_KEYLEN000 != prg->key_len)
        && (HIK_RTPPACK_SECRET_PACKET_FUA == prg->packet_type))
    {
        is_encrypt = 1;
    }

    copy_len = prg->max_rtp_len - pos;

    if (    prc->frame_type == FRAME_TYPE_VIDEO_IFRAME
        ||  prc->frame_type == FRAME_TYPE_VIDEO_EFRAME
        ||  prc->frame_type == FRAME_TYPE_VIDEO_PFRAME
        ||  prc->frame_type == FRAME_TYPE_VIDEO_BFRAME)
    {
        switch(prg->video_stream_type)
        {
        case STREAM_TYPE_VIDEO_H264:///< H.264需要遵循rfc3984协议，对码流有一定修改
        case STREAM_TYPE_VIDEO_SVAC:///< SVAC RFC3984协议
            if (prg->packed_len_cur == 0 && prc->is_unit_start)
            {
                if (is_encrypt)
                {
                    pos = RTPPACK_HEAD_LEN + HIK_ENCRYPT_INFO_LEN + 4; // 加密码流rtp头中增加扩展字段,4是类型和长度字段
                    prg->nalu_first_byte = prc->nalu_tab;
                    copy_len = prg->max_rtp_len - pos;
                    pos += RTPPACK_start_new_nalu_h264_encrypt(&buffer[pos], copy_len, prg, prc); ///< 打包新的加密后的h264 nalu
                }
                else
                {
                    prg->nalu_first_byte = prc->unit_in_buf[4];
                    pos += RTPPACK_start_new_nalu_h264(&buffer[pos], copy_len, prg, prc); ///< 打包一个新的h264 nalu
                }
            }
            else
            {
                if (is_encrypt)
                {
                    pos = RTPPACK_HEAD_LEN + HIK_ENCRYPT_INFO_LEN + 4; // 加密码流rtp头中增加扩展字段,4是类型和长度字段
                    copy_len = prg->max_rtp_len - pos;
                }

                pos += RTPPACK_continue_fragment_nalu_h264(&buffer[pos], copy_len, prg, prc); ///< 当上一次nalu没有发送完成时，继续发送fragment_unit
            }
            if (prg->packed_len_cur == prc->unit_in_len)
            {
                set_mark_bit = prc->is_last_unit && prc->is_unit_end;
            }
            break;
        case STREAM_TYPE_VIDEO_MJPG:///< MJPEG 需要遵循rfc2435协议，每个rtp包头后面有8个扩展字节

            if (is_encrypt)
            {
                pos = RTPPACK_HEAD_LEN + HIK_ENCRYPT_INFO_LEN + 4; // 加密码流rtp头中增加扩展字段,4是类型和长度字段
                copy_len = prg->max_rtp_len - pos;
            }

            ///< 修正当前rtp包负载数据在jpg帧数据内的偏移
            prc->mjpg_spc[1] = (prg->packed_len_cur >> 16) & 0xff;
            prc->mjpg_spc[2] = (prg->packed_len_cur >> 8) & 0xff;
            prc->mjpg_spc[3] = (prg->packed_len_cur) & 0xff;

            memcpy(&buffer[pos], prc->mjpg_spc, 8);  ///< mjpg rtp打包需要的额外8字节数据
            pos += 8;
            copy_len -= 8;

            rest_len = prc->unit_in_len - prg->packed_len_cur;
            if (copy_len >= rest_len)
            {
                copy_len = rest_len;
                set_mark_bit = prc->is_last_unit && prc->is_unit_end;
            }
            memcpy(&buffer[pos], src_buf, copy_len);
            pos += copy_len;
            prg->packed_len_cur += copy_len;
            break;
        case STREAM_TYPE_VIDEO_MPEG2:  ///< 打包MPEG2

            if (prc->frame_type == FRAME_TYPE_VIDEO_IFRAME)
            {
                frame_type = 1;  ///< 1为I帧
            }
            else
            {
                frame_type = 2;  ///< 2为P帧
            }
            // 添加4字节长度MPEG Video-specific header
            buffer[pos++] = 0x00           //5bits MBZ
                          | 0x00           //1bit  T
                          | ((prc->num_in_gop >> 8) & (0x03));     //2bits TR
            buffer[pos++] = ((prc->num_in_gop) & (0xff));          //8bits TR
            buffer[pos++] = 0x00           //1bit  AN
                          | 0x00           //1bit  N
                          | ((prc->is_unit_start & prc->is_key_frame) << 5)  //1bit  S
                          | (prc->is_unit_start << 4)                        //1bit  B
                          | (prc->is_unit_end << 3)                          //1bit  E
                          | frame_type;                                      //3bits P
            if (prc->is_key_frame)
            {
                buffer[pos++] = 0x00;
            }
            else
            {
                buffer[pos++] = 0x0F;
            }

            rest_len = prc->unit_in_len - prg->packed_len_cur;
            if (copy_len >= rest_len)
            {
                copy_len = rest_len;
                set_mark_bit = prc->is_last_unit && prc->is_unit_end;
            }
            memcpy(&buffer[pos], src_buf, copy_len);
            pos += copy_len;
            prg->packed_len_cur += copy_len;
            break;
        case STREAM_TYPE_VIDEO_MPEG4:  ///< 打包MPEG4
            {
                if (is_encrypt)
                {
                    // 加密码流rtp头中增加扩展字段,4是类型和长度字段
                    pos = RTPPACK_HEAD_LEN + HIK_ENCRYPT_INFO_LEN + 4;
                    if (!prg->packed_len_cur)
                    {
                        buffer[pos++] = prc->nalu_tab;
                        buffer[pos++] = prc->mpeg_frame_tab;
                    }
                    copy_len = prg->max_rtp_len - pos;
                }

                rest_len = prc->unit_in_len - prg->packed_len_cur;
                if (copy_len >= rest_len)
                {
                    copy_len = rest_len;
                    set_mark_bit = prc->is_last_unit && prc->is_unit_end;
                }
                memcpy(&buffer[pos], src_buf, copy_len);
                pos += copy_len;
                prg->packed_len_cur += copy_len;
                break;
            }
        case STREAM_TYPE_VIDEO_H265:  ///< H265
            {
                if (prg->packed_len_cur == 0 && prc->is_unit_start)
                {
                    if (is_encrypt)
                    {
                        pos = RTPPACK_HEAD_LEN + HIK_ENCRYPT_INFO_LEN + 4; // 加密码流rtp头中增加扩展字段,4是类型和长度字段
                        prg->nalu_first_byte  = prc->unit_in_buf[4];
                        prg->nalu_second_byte = prc->unit_in_buf[5];
                        copy_len = prg->max_rtp_len - pos;
                        pos += RTPPACK_start_new_nalu_h265(&buffer[pos], copy_len, prg, prc); ///< 打包一个新的h265 nalu
                    }
                    else
                    {
                        prg->nalu_first_byte  = prc->unit_in_buf[4];
                        prg->nalu_second_byte = prc->unit_in_buf[5];

                        pos += RTPPACK_start_new_nalu_h265(&buffer[pos], copy_len, prg, prc); ///< 打包一个新的h265 nalu
                    }
                }
                else
                {
                    if (is_encrypt)
                    {
                        pos = RTPPACK_HEAD_LEN + HIK_ENCRYPT_INFO_LEN + 4; // 加密码流rtp头中增加扩展字段,4是类型和长度字段
                        copy_len = prg->max_rtp_len - pos;
                    }

                    pos += RTPPACK_continue_fragment_nalu_h265(&buffer[pos], copy_len, prg, prc); ///< 当上一次nalu没有发送完成时，继续发送fragment_unit
                }

                if (prg->packed_len_cur == prc->unit_in_len)
                {
                    set_mark_bit = prc->is_last_unit && prc->is_unit_end;
                }
                break;
            }
        default:
            rest_len = prc->unit_in_len - prg->packed_len_cur;
            if (copy_len >= rest_len)
            {
                copy_len = rest_len;
                set_mark_bit = prc->is_last_unit && prc->is_unit_end;
            }
            memcpy(&buffer[pos], src_buf, copy_len);
            pos += copy_len;
            prg->packed_len_cur += copy_len;
            break;
        }
    }
    else
    {
        if (is_encrypt)
        {
            pos = RTPPACK_HEAD_LEN + HIK_ENCRYPT_INFO_LEN + 4; // 加密码流rtp头中增加扩展字段,4是类型和长度字段
            copy_len = prg->max_rtp_len - pos;
        }

        rest_len = prc->unit_in_len - prg->packed_len_cur;
        payload_type = prg->audio_payload_type;
        if (copy_len >= rest_len)
        {
            copy_len = rest_len;
            set_mark_bit = prc->is_last_unit;
        }

        if (   prg->audio_stream_type == STREAM_TYPE_AUDIO_AMR_NB
            || prg->audio_stream_type == STREAM_TYPE_AUDIO_AMR_WB)
        {
            set_mark_bit = 0;
            buffer[pos++] = (prc->codec_mode_request << 4) & 0xf0;
            frame_type = (prc->unit_in_buf[0] >> 3) & 0x0f;
            if (prg->audio_stream_type == STREAM_TYPE_AUDIO_AMR_NB) ///< amr
            {
                if (prg->is_last_frame_sid && frame_type < 8)
                {
                    set_mark_bit = 1;
                    prg->is_last_frame_sid = 0;
                }

                if (frame_type > 8 && frame_type < 12) ///< 是否为舒适噪声
                {
                    prg->is_last_frame_sid = 1;
                }
            }
            else ///< amr_wb
            {
                if (prg->is_last_frame_sid && frame_type < 9)
                {
                    set_mark_bit = 1;
                    prg->is_last_frame_sid = 0;
                }

                if (frame_type == 9) ///< 是否为舒适噪声
                {
                    prg->is_last_frame_sid = 1;
                }
            }
        }
        else if (prg->audio_stream_type == STREAM_TYPE_AUDIO_AAC)
        {
            //AU_header_length
            buffer[pos++] = 0x00;
            buffer[pos++] = 0x10;  ///< 16bits AU_header_length

            //AU_header
            buffer[pos++] = (prc->unit_in_len >> 5) & 0xff;
            buffer[pos++] = (prc->unit_in_len << 3) & 0xff;  ///< 13bits AU_length + 3bits AU_index
        }
        else if (prg->audio_stream_type == STREAM_TYPE_AUDIO_MPEG1 || prg->audio_stream_type == STREAM_TYPE_AUDIO_MPEG2)
        {
            //mpeg specific header
            buffer[pos++] = 0x00;
            buffer[pos++] = 0x00;

            buffer[pos++] = (prg->packed_len_cur >> 8);
            buffer[pos++] = (prg->packed_len_cur & 0xff);
        }
        else
        {
            //
        }

        memcpy(&buffer[pos], src_buf, copy_len);
        pos += copy_len;
        prg->packed_len_cur += copy_len;
    }

    // 最后填充RTP头
    RTPPACK_fill_rtp_header(buffer, prg, payload_type, set_mark_bit, tag_len);
    RTPPACK_fill_rtp_expend(buffer + RTPPACK_HEAD_LEN, prg);

    //当包长不是4字节对齐的时候，填充以对齐
    if ((pos & 3) && prg->allow_4byte_align)
    {
        int i = 0;
        padding_len = 4 - (pos & 3);

        for (i = 0; i < padding_len; i++)
        {
            buffer[pos++] = 0x00;
        }
        //pos += padding_len;
        buffer[pos - 1] = (unsigned char)padding_len; ///< 填充字段最后一位表示填充字段长度
        buffer[0] |= 0x20;                            ///< 修改padding位
    }
    return  pos;
}

/** @fn      RTPPACK_GetMemSize(RTPPACK_PARAM *param)
 *  @brief   获取所需内存大小
 *  @param   param       [IN] 参数结构指针
 *  @return  返回错误码
 */
HRESULT RTPPACK_GetMemSize(RTPPACK_PARAM *param)
{
    if (NULL == param)
    {
        return HIK_RTPPACK_LIB_E_PARA_NULL;
    }
    param->buffer_size = sizeof(HIK_RTPPACK_PRG);
    return HIK_RTPPACK_LIB_S_OK;
}

/** @fn      RTPPACK_Create(RTPPACK_PARAM *param, void **handle)
 *  @brief   创建 RTPPACK 模块
 *  @param   param       [INOUT] 参数结构指针
 *  @param   **handle    [OUT]   返回RTPMUX模块句柄
 *  @return  返回错误码
 */
HRESULT RTPPACK_Create(RTPPACK_PARAM *param, void **handle)
{
    HIK_RTPPACK_PRG *prg = NULL;
    if (NULL == param)
    {
        return HIK_RTPPACK_LIB_E_PARA_NULL;
    }
    if (NULL == param->buffer)
    {
        return HIK_RTPPACK_LIB_E_PARA_NULL;
    }

    prg = (HIK_RTPPACK_PRG *)param->buffer;
    
    RTPPACK_ResetStreamInfo((void *)prg, &param->info); ///< 重置参考数据

//  prg->seq_num        = rand();
    prg->video_seq_num  = rand();
    prg->audio_seq_num  = rand();
    prg->privt_seq_num  = rand();
    prg->time_stamp_cur =   0;
    prg->packed_len_cur =   0;

    *handle = (void *)prg;
    return HIK_RTPPACK_LIB_S_OK;
}

/** @fn      RTPPACK_ResetStreamInfo(void *handle, HIKRTP_PACK_ES_INFO *info)
 *  @brief   重置参考数据
 *  @param   handle       [IN] 句柄( handle 由 RTPPACK_Create 返回)
 *  @param   info         [IN] 参考数据句柄
 *  @return  返回错误码
 */
HRESULT RTPPACK_ResetStreamInfo(void *handle, HIKRTP_PACK_ES_INFO *info)
{
    HIK_RTPPACK_PRG *prg = (HIK_RTPPACK_PRG *)handle;
    if (NULL == handle || NULL == info)
    {
        return HIK_RTPPACK_LIB_E_PARA_NULL;
    }

    prg->allow_ext          = info->allow_ext;
    prg->allow_4byte_align  = info->allow_4byte_align;
    prg->is_last_frame_sid  = 1;
    prg->encrypt_type       = info->stream_info.encrypt_type;
    prg->stream_mode        = info->stream_mode;
    prg->watermark_mode     = info->watermark_mode;
    prg->video_stream_type  = info->video_stream_type;
    prg->audio_stream_type  = info->audio_stream_type;
    prg->max_rtp_len        = info->max_packet_len;
    prg->encrypt_round      = 0;
    prg->encrypt_type       = 0;
    prg->key_len            = 0;
    prg->packet_type        = 0;

    if (prg->max_rtp_len & 3)
    {
        prg->max_rtp_len = prg->max_rtp_len + (4 - (prg->max_rtp_len & 3));

        if (prg->max_rtp_len < RTPMIN_PACKET_SIZE || prg->max_rtp_len > RTPMAX_PACKET_SIZE)
        {
            prg->max_rtp_len = 5 * 1024;
        }
    }

    prg->video_clip         = info->stream_info.video_info.play_clip;
    prg->SSRC               = info->SSRC;
    prg->video_ssrc         = info->video_ssrc;
    prg->audio_ssrc         = info->audio_ssrc;
    prg->privt_ssrc         = info->privt_ssrc;
    prg->video_payload_type = info->video_payload_type;
    prg->audio_payload_type = info->audio_payload_type;
    prg->totle_packet       = 0;
    prg->current_packet     = 0;

    HKDSC_fill_device_descriptor(prg->device_dsc, info->stream_info.dev_chan_id);   ///< 填充设备描述字

    if (info->stream_mode & INCLUDE_VIDEO_STREAM)
    {
        //printf("rtp update time_info %d\n",info->stream_info.video_info.time_info);
        HKDSC_fill_video_descriptor(prg->video_dsc, &info->stream_info.video_info); ///< 填充视频描述字
        if (prg->video_clip)
        {
            HKDSC_fill_video_clip_descriptor(prg->video_clip_dsc, &info->stream_info.video_info); ///< 填充显示裁剪描述字
        }
    }
    if (info->stream_mode & INCLUDE_AUDIO_STREAM)
    {
        HKDSC_fill_audio_descriptor(prg->audio_dsc, &info->stream_info.audio_info);  ///< 填充音频描述字
    }
    prg->time_stamp_cur =   0;
    prg->packed_len_cur =   0;
    #ifdef SRTP_SUPPORT

    memset(prg->srtp_info, 0, RTP_MAX_STREAMNUM * sizeof(SRTP_INFO));

    #endif

    return HIK_RTPPACK_LIB_S_OK;
}

/** @fn      RTPPACK_Process(void *handle, RTPPACK_PROCESS_PARAM *param)
 *  @brief   打包一段数据块
 *  @param   handle       [IN] 句柄( handle 由 RTPPACK_Create 返回)
 *  @param   param        [IN]   处理单元参数
 *  @return  返回错误码
 */
HRESULT RTPPACK_Process(void *handle, RTPPACK_PROCESS_PARAM *param)
{
    HIK_RTPPACK_PRG *prg  = (HIK_RTPPACK_PRG *)handle;
    unsigned char *buffer = NULL;
    int rtp_len           = 0;
    int tot_len           = 0;
    int out_len           = 0;
    int pos               = 0;
    unsigned char *in_buf = NULL;
    int in_len            = 0;
    int file_hdr_len      = 0;
    unsigned int tag_len  = 0;
    //unsigned int rtp_num  = 0;

    if (NULL == param || NULL == prg)
    {
        return HIK_RTPPACK_LIB_E_PARA_NULL;
    }
    if (param->rtp_out_buf_size - out_len < prg->max_rtp_len)
    {
        return HIK_RTPPACK_LIB_E_MEM_OVER;
    }

    prg->time_stamp_cur = param->time_stamp;
    prg->packed_len_cur = 0;
    prg->rtp_count      = 0;
    prg->encrypt_round  = param->encrypt_round;
    prg->encrypt_type   = param->encrypt_type;
    prg->key_len        = param->key_len;
    prg->encrypt_arith  = param->encrypt_arith;
    prg->packet_type    = param->packet_type;
	
    if (param->is_key_frame)
	{
	    if (param->frame_type == STREAM_TYPE_H264_IFRAME)
	    {
	        param->frame_type = FRAME_TYPE_VIDEO_IFRAME;
			prg->video_stream_type = STREAM_TYPE_VIDEO_H264;
	    }
		else if (param->frame_type == STREAM_TYPE_H265_IFRAME)
		{
		    param->frame_type = FRAME_TYPE_VIDEO_IFRAME;
			prg->video_stream_type = STREAM_TYPE_VIDEO_H265;
		}
		else
		{
		    SAL_WARN("encodType error %d\n",param->frame_type );
		    param->frame_type = FRAME_TYPE_VIDEO_IFRAME;
			prg->video_stream_type = STREAM_TYPE_VIDEO_H264;
		}
	}
	else 
	{
	    if (param->frame_type == STREAM_TYPE_H264_PFRAME)
	    {
	        param->frame_type = FRAME_TYPE_VIDEO_PFRAME;
			prg->video_stream_type = STREAM_TYPE_VIDEO_H264;
	    }
		else if (param->frame_type == STREAM_TYPE_H265_PFRAME)
		{
		    param->frame_type = FRAME_TYPE_VIDEO_PFRAME;
			prg->video_stream_type = STREAM_TYPE_VIDEO_H265;
		}        
		else if (param->frame_type == FRAME_TYPE_PRIVT_FRAME)
		{
		    param->frame_type = FRAME_TYPE_PRIVT_FRAME;
			prg->video_stream_type = STREAM_TYPE_HIK_PRIVT;
		}
		else
		{
		    SAL_WARN("encodType error %d\n",param->frame_type );
		    param->frame_type = FRAME_TYPE_VIDEO_PFRAME;
			prg->video_stream_type = STREAM_TYPE_VIDEO_H264;
		}
	}
	
    buffer = param->rtp_out_buf;
    in_buf = param->unit_in_buf;
    in_len = param->unit_in_len;

    //当关键帧时首先生成2个空负载rtp包，包头包含海康扩展字段
    if (param->is_key_frame && param->is_first_unit && param->is_unit_start && prg->allow_ext)
    {
        ///< 生成负载长度为0，只包含海康私有数据 basic_stream_info 的RTP包
        rtp_len   = RTPPACK_create_basic_stream_info_rtp(&buffer[rtp_len + 4],  prg, param, &tag_len);
        rtp_len  += tag_len;
        buffer[0] = '$';
        buffer[1] = 0x0;

        buffer[2] = ((rtp_len >> 8) & 0xff);
        buffer[3] = (rtp_len & 0xff);

        tot_len   = (rtp_len + 4);
        buffer   += tot_len;
        out_len  += tot_len;

        ///< 生成负载长度为0，只包含海康私有数据 codec info 的RTP包
        rtp_len   = 0;
        rtp_len   = RTPPACK_create_codec_info_rtp(&buffer[rtp_len + 4],  prg, &tag_len);
        rtp_len  += tag_len;
        buffer[0] = '$';
        buffer[1] = 0x0;
        buffer[2] = ((rtp_len >> 8) & 0xff);
        buffer[3] = (rtp_len & 0xff);
        tot_len   = (rtp_len + 4);
        buffer   += tot_len;
        out_len  += tot_len;
    }

    //mjpeg先去掉文件头
    if (prg->video_stream_type == STREAM_TYPE_VIDEO_MJPG && param->mjpg_have_file_hdr)
    {
        while (pos < in_len)
        {
            if (in_buf[pos] == 0xff && in_buf[pos+1] == 0xda)
            {
                file_hdr_len = pos + 2 + (in_buf[pos+2] << 8) + in_buf[pos+3];
                break;
            }
            pos++;
        }
        if (pos + 3 > in_len) ///< 错误的文件头
        {
            return HIK_RTPPACK_LIB_S_FAIL;
        }
        else
        {
            param->unit_in_buf += file_hdr_len;
            param->unit_in_len -= file_hdr_len;
        }
    }

    //aac先去掉7字节的adts_header
    if ((prg->audio_stream_type == STREAM_TYPE_AUDIO_AAC) && (param->frame_type == FRAME_TYPE_AUDIO_FRAME))
    {
        param->unit_in_buf += 7;
        param->unit_in_len -= 7;
    }

    while (prg->packed_len_cur < param->unit_in_len)
    {
        if (param->rtp_out_buf_size - out_len < prg->max_rtp_len)
        {
            SAL_ERROR("error %d,%d,%d\n",param->rtp_out_buf_size, out_len ,prg->max_rtp_len);
            return HIK_RTPPACK_LIB_E_MEM_OVER;
        }

        //生成私有信息rtp包，比如智能信息
        if (param->frame_type == FRAME_TYPE_PRIVT_FRAME)
        {
            rtp_len = RTPPACK_create_privt_info_rtp(&buffer[4], prg, param, &tag_len);
        }
        else
        {
            rtp_len = RTPPACK_fill_rtp_pack(&buffer[4], prg, param, &tag_len);
        }

        rtp_len  += tag_len;
        buffer[0] = '$';
        buffer[1] = 0x0;
        buffer[2] = ((rtp_len >> 8) & 0xff);
        buffer[3] = (rtp_len & 0xff);
        tot_len   = (rtp_len + 4);
        buffer   += tot_len;
        out_len  += tot_len;
    }

    param->rtp_out_len  = out_len;
    param->rtp_num      = prg->rtp_count;
    param->unit_in_buf  = in_buf;
    param->unit_in_len  = in_len;
    prg->current_packet = 0;

    #ifdef SRTP_SUPPORT

    buffer = param->rtp_out_buf;

    for(rtp_num = 0; rtp_num < param->rtp_num; rtp_num++)
    {
        int hr = HIK_RTPPACK_LIB_S_OK;

        rtp_len = (buffer[0] << 24)
                + (buffer[1] << 16)
                + (buffer[2] << 8)
                +  buffer[3];

        hr = RTPPACK_Srtp_Pack(prg, buffer + 4, rtp_len);
        if (HIK_RTPPACK_LIB_S_OK != hr)
        {
            return HIK_RTPPACK_LIB_E_SRTP;
        }

        buffer += (4 + rtp_len);
    }

    #endif

    return HIK_RTPPACK_LIB_S_OK;
}


HRESULT RTPPACK_SetSecureInfo(void *handle, RTP_SECURE_INFO *info)
{
    #ifdef SRTP_SUPPORT

    HIK_RTPPACK_PRG *prg = (HIK_RTPPACK_PRG *)handle;
    HRESULT hr = HIK_RTPPACK_LIB_S_OK;
    int i = 0;

    if (NULL == info)
    {
        return HIK_RTPPACK_LIB_E_PARA_NULL;
    }

    if (0 == info->rtp_secure)
    {
        return HIK_RTPPACK_LIB_S_OK;
    }

    if (NULL == prg)
    {
        return HIK_RTPPACK_LIB_E_PARA_NULL;
    }

    for(i = 0; i < RTP_MAX_STREAMNUM;i++)
    {
        if (0 == prg->srtp_info[i].setsecure)
        {
            hr = hik_srtp_init(&prg->srtp_info[i], info);
            if (HIK_SRTP_OK == hr)
            {
                prg->srtp_info[i].ssrc      = info->ssrc;
                prg->srtp_info[i].setsecure = info->rtp_secure;
            }

            break;
        }
        else
        {
            if (prg->srtp_info[i].ssrc == info->ssrc)
            {
                break;
            }
        }

    }

    if (HIK_SRTP_OK == hr)
    {
        return HIK_RTPPACK_LIB_S_OK;
    }
    else
    {
        return HIK_RTPPACK_LIB_E_SRTP;
    }

    #else

    return HIK_RTPPACK_LIB_E_SRTP;

    #endif
}

HRESULT RTPPACK_GetStreamInfo(void *handle, HIKRTP_PACK_STREAM_INFO *stream_info)
{
    HIK_RTPPACK_PRG *prg  = (HIK_RTPPACK_PRG *)handle;

    if (NULL == stream_info || NULL == prg)
    {
        return HIK_RTPPACK_LIB_E_PARA_NULL;
    }

    stream_info->seq_video = prg->video_seq_num;
    stream_info->seq_audio = prg->audio_seq_num;
    
    return HIK_RTPPACK_LIB_S_OK;
}


#ifdef SRTP_SUPPORT

int RTPPACK_Srtp_Pack(HIK_RTPPACK_PRG *prg, unsigned char *rtp_buf, unsigned int rtp_len)
{
    unsigned int ssrc         = 0;
    int srtp_index            = 0;
    int ext_marker            = 0;
    unsigned short seq_num    = 0;
    unsigned int csrc_count   = 0;

    unsigned char *enc_start  = NULL;
    unsigned int enc_len      = 0;
    unsigned char *auth_start = NULL;
    unsigned char *auth_tag   = NULL;
    int index_delta           = 0;
    int prefix_len            = 0;
    int hr                    = HIK_SRTP_OK;
    xtd_seq_num_t est_seq_num = {0};


    if (rtp_len < RTPPACK_HEAD_LEN)
    {
        return HIK_RTPPACK_LIB_E_PARA_NULL;
    }

    ext_marker = (rtp_buf[0] >> 4)& 0x01;
    csrc_count = rtp_buf[0] & 0xf;
    seq_num    = (rtp_buf[2] << 8) + rtp_buf[3];
    ssrc       = (rtp_buf[8] << 24)
               + (rtp_buf[9] << 16)
               + (rtp_buf[10] << 8)
               +  rtp_buf[11];

    srtp_index = hik_srtp_getindex(prg->srtp_info, RTP_MAX_STREAMNUM, ssrc);
    if (-1 == srtp_index)
    {
        return HIK_RTPPACK_LIB_S_OK;
    }

    hr = hik_srtp_keylimit_update(&prg->srtp_info[srtp_index]);
    if (HIK_SRTP_OK != hr)
    {
        return HIK_RTPPACK_LIB_E_SRTP;
    }

    if (SEC_SERV_AUTH & prg->srtp_info[srtp_index].sec_serv)
    {
        rtp_len -= prg->srtp_info[srtp_index].auth_tag_len;
    }

    if (SEC_SERV_CONF & prg->srtp_info[srtp_index].sec_serv)
    {
        enc_start = rtp_buf + (RTPPACK_HEAD_LEN + (csrc_count << 2));
        enc_len   = rtp_len - (RTPPACK_HEAD_LEN + (csrc_count << 2));

        if (1 == ext_marker)
        {
            unsigned int ext_len = 0;

            ext_len    = ((enc_start[2] << 8) + enc_start[3]) << 2;
            ext_len   += 4;

            enc_start += ext_len;
            enc_len   -= enc_len;

        }
    }

    if (SEC_SERV_AUTH & prg->srtp_info[srtp_index].sec_serv)
    {
        auth_start = rtp_buf;
        auth_tag   = rtp_buf + rtp_len;
    }

    hr = hik_srtp_rdbx_index(&prg->srtp_info[srtp_index],
                             &est_seq_num,
                             seq_num,
                             &index_delta);
    if (HIK_SRTP_OK != hr)
    {
        return HIK_RTPPACK_LIB_E_SRTP;
    }

    hr = hik_srtp_rdbx_check(&prg->srtp_info[srtp_index], index_delta);
    if (HIK_SRTP_OK != hr)
    {
        return HIK_RTPPACK_LIB_E_SRTP;
    }

    hr = hik_srtp_rdbx_addindex(&prg->srtp_info[srtp_index], index_delta);
    if (HIK_SRTP_OK != hr)
    {
        return HIK_RTPPACK_LIB_E_SRTP;
    }

    if (NULL != enc_start)
    {
        hr = hik_srtp_encrypt_data(&prg->srtp_info[srtp_index],
                                   &est_seq_num,
                                   ssrc,
                                   enc_start,
                                   &enc_len);
        if (HIK_SRTP_OK != hr)
        {
            return HIK_RTPPACK_LIB_E_SRTP;
        }
    }

    if (NULL != auth_start)
    {
        hr = hik_srtp_encrypt_auth(&prg->srtp_info[srtp_index],
                                   &est_seq_num,
                                   auth_start,
                                   auth_tag,
                                   rtp_len);
        if (HIK_SRTP_OK != hr)
        {
            return HIK_RTPPACK_LIB_E_SRTP;
        }

    }

    return HIK_RTPPACK_LIB_S_OK;
}

#endif

