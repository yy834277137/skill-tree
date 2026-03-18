/*******************************************************************************
 * parseVideo.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : 
 * Version: V1.0.0  2014年12月24日 Create
 *
 * Description : 分析视频
 * Modification: 
 *******************************************************************************/



/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "type_dscrpt_common.h"
#include "get_es_info.h"
#include "sal.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */


/* video object layer startcode 0x00000120 ~ 0x0000012f */
#define VIDOBJLAYER_StartCode               0x20010000
#define Min_ParseBytes                      12
#define DIMINSION_LIMIT_MIN                 32
#define	DIMINSION_LIMIT_MAX                 2048
#define VIDOBJLAY_AR_EXTPAR                 15
#define VIDOBJLAY_SHAPE_RECTANGULAR         0
#define VIDOBJLAY_SHAPE_BINARY_ONLY         2
#define VIDOBJLAY_SHAPE_GRAYSCALE           3
#define READ_MARKER()                       BitstreamSkip(bs, 1)

static const unsigned char log2_tab_16[16] =  { 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 };

/******************************************************************************
* 功  能：log2bin
* 参  数：
*
* 返回值：成功获取到视频信息则返回1，否则返回0
* 备  注：
******************************************************************************/
static unsigned int __inline log2bin(unsigned int value)
{
    int n = 0;
    if (value & 0xffff0000)
    {
        value >>= 16;
        n += 16;
    }
    if (value & 0xff00)
    {
        value >>= 8;
        n += 8;
    }
    if (value & 0xf0)
    {
        value >>= 4;
        n += 4;
    }
    return n + log2_tab_16[value];
}


typedef struct _M4V_BITSTREAM_
{
    int pos;
    unsigned char *  tail;
} M4V_Bitstream;

/******************************************************************************
* 功  能：BitstreamGetBits
* 参  数：
*
* 返回值：成功获取到视频信息则返回1，否则返回0
* 备  注：
******************************************************************************/
static unsigned int __inline BitstreamGetBits(M4V_Bitstream * bs, const int n)
{
    unsigned int tmp;
    unsigned int ret;

    tmp = (*(bs->tail))<<24;
    tmp += (*(bs->tail+1))<<16;
    tmp += (*(bs->tail+2))<<8;
    tmp += (*(bs->tail+3));


    ret = (tmp << bs->pos) >> (32 - n);

    bs->pos += n;
    bs->tail += bs->pos>>3;
    bs->pos &= 7;

    return  ret;
}

/******************************************************************************
* 功  能：BitstreamGetBits
* 参  数：
*
* 返回值：成功获取到视频信息则返回1，否则返回0
* 备  注：
******************************************************************************/
static unsigned int __inline BitstreamGetBit(M4V_Bitstream * bs)
{
    return BitstreamGetBits(bs, 1);
}


/******************************************************************************
* 功  能：skip n bits forward in bitstream
* 参  数：
*
* 返回值：成功获取到视频信息则返回1，否则返回0
* 备  注：
******************************************************************************/
static __inline void BitstreamSkip(M4V_Bitstream * bs, const int bits)
{
    bs->pos += bits;
    bs->tail += bs->pos>>3;
    bs->pos &= 7;
}


/******************************************************************************
* 功  能：检查帧头信息
* 参  数：
*
* 返回值：成功获取到视频信息则返回1，否则返回0
* 备  注：
******************************************************************************/
int checkFrameHeadM4v(unsigned char *buffer, int length)
{
    if (buffer == 0 || length < 5)
        return HIK_NO_HEADER;
    if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x01)
    {
        if (buffer[3] == 0xb6)
        {
            switch(buffer[4] & 0xc0)
            {
            case 0x00:
                return HIK_VIDEO_FRAME_IFRAME;
            case 0x40:
                return HIK_VIDEO_FRAME_PFRAME;
            case 0x80:
                return HIK_VIDEO_FRAME_BFRAME;
            default:
                return HIK_HEADER_UNDEFINED;
            }
        }
        else if (buffer[3] == 0xb0)
        {
            return HIK_VIDEO_PARAM;
        }
        else
        {
            return HIK_HEADER_UNDEFINED;
        }
    }
    else
    {
        return HIK_NO_HEADER;
    }
}


/******************************************************************************
* 功  能：获取视频信息
* 参  数：
*
* 返回值：成功获取到视频信息则返回1，否则返回0
* 备  注：
******************************************************************************/
int seekVideoInfoM4v(unsigned char *buffer, unsigned int length, VIDEO_ES_INFO *video_info)
{
    unsigned char *ptr = buffer;
    unsigned int i;
    int bit_cnt;
    unsigned int tmp;
    M4V_Bitstream str_bs;
    M4V_Bitstream *bs = &str_bs;
    int vol_ver_id;
    int aspect_ratio;
    int shape;
    int time_inc_resolution;
    int time_inc_bits;

    video_info->interlace = 0;
    if (length < Min_ParseBytes)
    {
        return HIK_STREAM_INFO_CHECK_FAIL;
    }


    for (i = 0; i < length-4; i++)
    {
        if((ptr[i]==0)&&(ptr[i+1]==0)&&(ptr[i+2]==1)&&(ptr[i+3]==0xb6))
        {
            break;
        }
        /* 1st, find start code */
        if((ptr[i]==0)&&(ptr[i+1]==0)&&(ptr[i+2]==1)&&((ptr[i+3]&0xf0)==0x20))
        {
            /* BitstreamSkip(bs, 32);	   video_object_layer_start_code    */
            bs->tail = ptr + i + 4;
            bs->pos = 0;

            if (length - i - 4 < 9)
            {
                return HIK_STREAM_INFO_CHECK_FAIL;
            }

            BitstreamSkip(bs, 1);	      /* random_accessible_vol */
            BitstreamSkip(bs, 8);       /* video_object_type_indication */

            bit_cnt = 32 + 10;
            if (BitstreamGetBit(bs))	   /* is_object_layer_identifier */
            {
                /* video_object_layer_verid */
                vol_ver_id = BitstreamGetBits(bs, 4);
                BitstreamSkip(bs, 3);	  /* video_object_layer_priority */
                bit_cnt += 7;
            }
            else
            {
                vol_ver_id = 1;
            }

            aspect_ratio = BitstreamGetBits(bs, 4);
            bit_cnt += 4;
            /* aspect_ratio_info */
            if (aspect_ratio == VIDOBJLAY_AR_EXTPAR)
            {
                /* par_width */
                BitstreamGetBits(bs, 8);
                /* par_height */
                BitstreamGetBits(bs, 8);
                bit_cnt += 16;
            }
            bit_cnt += 1;
            if (BitstreamGetBit(bs))	   /* vol_control_parameters */
            {
                BitstreamSkip(bs, 2);	  /* chroma_format */
                /* low_delay */
                video_info->low_delay = BitstreamGetBit(bs);
                bit_cnt += 4;
                /* vbv_parameters */
                if (BitstreamGetBit(bs))
                {
                    tmp = i + (bit_cnt >> 3);
                    if (tmp + 10 + 7 > length)
                    {
                        return HIK_STREAM_INFO_CHECK_FAIL;
                    }
                    /* first_half_bit_rate */
                    BitstreamGetBits(bs,15);
                    READ_MARKER();
                    /* latter_half_bit_rate */
                    BitstreamGetBits(bs,15);
                    READ_MARKER();

                    /* first_half_vbv_buffer_size */
                    BitstreamGetBits(bs, 15);
                    READ_MARKER();
                    /* latter_half_vbv_buffer_size */
                    BitstreamGetBits(bs, 3);

                    /* first_half_vbv_occupancy */
                    BitstreamGetBits(bs, 11) ;
                    READ_MARKER();
                    /* latter_half_vbv_occupancy */
                    BitstreamGetBits(bs, 15);
                    READ_MARKER();
                    bit_cnt += 79;
                }

            }
            else
            {
                video_info->low_delay = 1;
            }

            /* video_object_layer_shape */
            shape = BitstreamGetBits(bs, 2);

            if (shape != VIDOBJLAY_SHAPE_RECTANGULAR)
            {
            }

            if (shape == VIDOBJLAY_SHAPE_GRAYSCALE && vol_ver_id != 1)
            {
                BitstreamSkip(bs, 4);	  /* video_object_layer_shape_extension */
            }

            READ_MARKER();

            /********************** for decode B-frame time ***********************/
            /* vop_time_increment_resolution */
            video_info->frame_rate = time_inc_resolution = BitstreamGetBits(bs, 16);
            if(time_inc_resolution==1)
            {
                video_info->frame_rate=0;
            }
            if (time_inc_resolution > 0)
            {
                time_inc_bits = log2bin(time_inc_resolution-1) > 1 ? log2bin(time_inc_resolution-1) : 1;
            }
            else
            {
                /* for "old" xvid compatibility, set time_inc_bits = 1 */
                time_inc_bits = 1;
            }

            READ_MARKER();

            if (BitstreamGetBit(bs))	   /* fixed_vop_rate */
            {

                /* fixed_vop_time_increment */
                BitstreamSkip(bs, time_inc_bits);
            }

            if (shape != VIDOBJLAY_SHAPE_BINARY_ONLY)
            {

                if (shape == VIDOBJLAY_SHAPE_RECTANGULAR)
                {
                    READ_MARKER();
                    /* video_object_layer_width */
                    video_info->width = BitstreamGetBits(bs, 13);
                    READ_MARKER();
                    /* video_object_layer_height */
                    video_info->height = BitstreamGetBits(bs, 13);
                    READ_MARKER();
                    video_info->interlace = BitstreamGetBits(bs, 1);
                }

                if (video_info->width < DIMINSION_LIMIT_MIN || video_info->height < DIMINSION_LIMIT_MIN
                        || video_info->width > DIMINSION_LIMIT_MAX || video_info->height > DIMINSION_LIMIT_MAX)
                {
                    return HIK_STREAM_INFO_CHECK_FAIL;
                }

                return HIK_STREAM_INFO_CHECK_OK;
            }
            return HIK_STREAM_INFO_CHECK_FAIL;
        }
    }
    return HIK_STREAM_INFO_CHECK_FAIL;
}

#define H264_GetVLC1(bs) H264_GetVLCNx(bs, 1)

/******************************************************************************
* 功  能：获取视频信息
* 参  数：
*
* 返回值：成功获取到视频信息则返回1，否则返回0
* 备  注：
******************************************************************************/
unsigned int H264_GetVLCNx (AVC_Bitstream * bs, unsigned int n)
{
    unsigned int rack = bs->b_rack;
    unsigned int num = bs->b_num;
    int code;

    code = rack >> (32 - n);
    rack <<= n;
    num -= n;

    while(num<=24)
    {
        rack |= *bs->Rdptr++ << (24 - num);
        num += 8;
    }
    bs->b_num = num;
    bs->b_rack = rack;

    return code;
}


/******************************************************************************
* 功  能：获取视频信息
* 参  数：
*
* 返回值：成功获取到视频信息则返回1，否则返回0
* 备  注：
******************************************************************************/
unsigned int H264_GetVLCSymbolx (AVC_Bitstream * bs, unsigned int * info)
{
    unsigned int rack = bs->b_rack;
    unsigned int num = bs->b_num;
    unsigned int len;
    int i;
    unsigned int mask = 1u<<31;

    len = 1;
    for(i = 0; (i < 16 && (!(rack&mask))); i ++, mask >>= 1)
    {
        len ++;
    }

    num -= len;
    rack <<= len;

    while(num<=24)
    {
        rack |= *bs->Rdptr++ << (24 - num);
        num += 8;
    }

    if(len <= 1)                        /* len<=1时,读出的数是1 */
    {
        *info = 0;
        bs->b_num = num;
        bs->b_rack = rack;
        return 1;
    }
    *info = (rack >> (32 - (len-1)));

    num -= len-1;
    rack <<= len-1;
    while(num<=24)
    {
        rack |= *bs->Rdptr++ << (24 - num);
        num += 8;
    }

    bs->b_num = num;
    bs->b_rack = rack;
    return (len+len-1);
}


/******************************************************************************
* 功  能：获取视频信息
* 参  数：
*
* 返回值：成功获取到视频信息则返回1，否则返回0
* 备  注：
******************************************************************************/
unsigned int H264_read_linfox(AVC_Bitstream *bs)
{
    unsigned int len, inf;
    len =  H264_GetVLCSymbolx (bs, &inf);
    return ((1<<(len>>1))+inf-1);
}

/* fill sps with content of p */
#define CHECK_ENCODED_PARAM(code, n) {if(code != n) return 0;}

/******************************************************************************
* 功  能：检查帧头信息
* 参  数：
*
* 返回值：成功获取到视频信息则返回1，否则返回0
* 备  注：
******************************************************************************/
int checkFrameHeadAvc(unsigned char *buffer, int length)
{
    if (buffer == 0 || length < 5)
    {
        return HIK_NO_HEADER;
    }

    if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x00 && buffer[3] == 0x01)
    {
        switch(buffer[4] & 0x1f)
        {
        case 0x01:
            if (buffer[4] & 0x60)
            {
                return HIK_VIDEO_FRAME_PFRAME;
            }
            else
            {
                return HIK_VIDEO_FRAME_BFRAME;
            }
        case 0x05:
            return HIK_VIDEO_FRAME_IFRAME;
        case 0x07:
            return HIK_VIDEO_PARAM;
        case 0x09:
            return HIK_VIDEO_AVC_AUD;
        default:
            return HIK_HEADER_UNDEFINED;
        }
    }
    else
        return HIK_NO_HEADER;
}


/******************************************************************************
* 功  能：初始化码流解析参数结构
* 参  数：bs     - 码流解析参数结构指针   (OUT)
*         buffer - 压缩码流缓冲区指针     (IN)
*         size   - 一帧（或一场）码流大小 (IN)
* 返回值：
* 备  注：
* 修  改: 2008/01/07
******************************************************************************/
void H264_init_bitstreamx(AVC_Bitstream * bs, unsigned char *buffer, int size)
{
    bs->b_num = 32;
    bs->Rdbfr   = buffer;
    bs->b_rack = (bs->Rdbfr[0]<<24)+(bs->Rdbfr[1]<<16)+(bs->Rdbfr[2]<<8)+(bs->Rdbfr[3]);
    bs->Rdptr = bs->Rdbfr + 4;
    bs->Rdmax = bs->Rdbfr + size;
}

/******************************************************************************
* 功  能： 获取H264的slice_type
*
* 参  数： srcbuf - slice缓冲区首指针 (IN)
*          len    - 数据长度          (IN)
* 返回值： 返回状态码
******************************************************************************/
int get_h264_slice_type_avc(unsigned char* srcbuf, int len)
{
    int video_frame_type;
    AVC_Bitstream Bitstream;
    AVC_Bitstream *bs = &Bitstream;

    H264_init_bitstreamx(bs, srcbuf, len);

    H264_read_linfox(bs);               /* first mb in slice */

    /* slice type */
    video_frame_type = H264_read_linfox(bs);

    switch (video_frame_type)
    {
    case 2:
    case 7:
    {
        video_frame_type = FRAME_TYPE_VIDEO_IFRAME;
        break;
    }
    case 0:
    case 5:
    {
        video_frame_type = FRAME_TYPE_VIDEO_PFRAME;
        break;
    }
    case 1:
    case 6:
    {
        video_frame_type = FRAME_TYPE_VIDEO_BFRAME;
        break;
    }
    default:
    {
        video_frame_type = FRAME_TYPE_VIDEO_PFRAME;
        break;
    }
    }

    return video_frame_type;
}

#define MAX_POC_REF_NUM                 16
/******************************************************************************
* 功  能：获取视频信息
* 参  数：
*
* 返回值：成功获取到视频信息则返回1，否则返回0
* 备  注：
******************************************************************************/
unsigned int H264_read_se_x(AVC_Bitstream *bs)
{
    unsigned int  k;
    unsigned int  code_num = H264_read_linfox(bs);

    k = (code_num & 1) ? (code_num + 1) >> 1 : -(int)(code_num >> 1);

    return k;
}


/******************************************************************************
* 功  能：获取视频帧率信息
* 参  数：
*
* 返回值：成功获取到视频信息则返回1，否则返回0
* 备  注：
******************************************************************************/
int video_info_H264_get_vui_parameters(AVC_Bitstream *bs, VIDEO_ES_INFO *video_info)
{
    /* int hr; */

    unsigned int aspect_ratio_info_present_flag;
    unsigned int overscan_info_present_flag;
    unsigned int video_signal_type_present_flag;
    unsigned int chroma_loc_info_present_flag;
    unsigned int num_units_in_tick;
    unsigned int timing_info_present_flag;
    unsigned int time_scale;

    /* unsigned int nal_hrd_parameters_present_flag;
      unsigned int vlc_hrd_parameters_present_flag;
      unsigned int bitstream_restriction_flag; */

    /* vui: aspect_ratio_info_present_flag */
    aspect_ratio_info_present_flag = H264_GetVLC1(bs);
    if (aspect_ratio_info_present_flag)
    {
        unsigned int aspect_ratio_idc;
        /* vui: aspect_ratio_idc */
        aspect_ratio_idc = H264_GetVLCNx(bs, 8);
        if (aspect_ratio_idc == 255)          /* Extended_SAR */
        {
            H264_GetVLCNx(bs, 16);               /* vui: sar_width */
            H264_GetVLCNx(bs, 16);               /* vui: sar_height */
        }
    }

    /* vui: overscan_info_present_flag */
    overscan_info_present_flag = H264_GetVLC1(bs);
    if (overscan_info_present_flag)
    {
        H264_GetVLC1(bs);                     /* vui: overscan_appropriate_flag */
    }

    /* vui: video_signal_type_present_flag */
    video_signal_type_present_flag =H264_GetVLC1(bs);
    if (video_signal_type_present_flag)
    {
        unsigned int colour_description_present_flag;
        H264_GetVLCNx(bs, 3);                 /* vui: video_format */
        H264_GetVLC1(bs);                     /* vui: video_full_range_flag */
        /* vui: colour_description_present_flag */
        colour_description_present_flag = H264_GetVLC1(bs);
        if (colour_description_present_flag)
        {
            H264_GetVLCNx(bs, 8);                /* vui: colour_primaries */
            H264_GetVLCNx(bs, 8);                /* vui: transfer_characteristics */
            H264_GetVLCNx(bs, 8);                /* vui: matrix_coefficients */
        }
    }

    /* vui: chroma_loc_info_present_flag */
    chroma_loc_info_present_flag = H264_GetVLC1(bs);
    if (chroma_loc_info_present_flag)
    {

        /* vui: chroma_sample_loc_type_top_field		 */
        H264_read_linfox(bs);
        /* vui: chroma_sample_loc_type_bottom_field */
        H264_read_linfox(bs);
    }

    /* vui: timing_info_present_flag */
    timing_info_present_flag = H264_GetVLC1(bs);
    video_info->frame_rate = 0;
    if (timing_info_present_flag)
    {
        /* H264_GetVLCN 不支持取超过24位，因此取32位采用分两次16位 */

        /* vui: num_units_in_tick */
        num_units_in_tick = H264_GetVLCNx(bs, 16) << 16;
        num_units_in_tick += H264_GetVLCNx(bs, 16);

        /* vui: time_scale */
        time_scale = H264_GetVLCNx(bs, 16) << 16;
        time_scale += H264_GetVLCNx(bs, 16);
        H264_GetVLC1(bs);                     /* vui: fixed_frame_rate_flag */
        if (num_units_in_tick > 0)
        {
            video_info->frame_rate = (float)time_scale / (2 * num_units_in_tick);
        }
        else
        {
            video_info->frame_rate = 25.0;
        }
    }

    return 1;
}

void AVCDEC_decode_scaling_list(AVC_Bitstream *bs, int size)
{
    int i, last = 8, next = 8;

    /* matrix not written, we use the predicted one */
    if (!H264_GetVLC1(bs))
    {
    }
    else
    {
        for (i = 0; i < size; i++)
        {
            if (next)
            {
                next = (last + H264_read_se_x(bs)) & 0xff;
            }

            if (!i && !next)
                /* matrix not written, we use the preset one */
            {
                break;
            }
            last = next ? next : last;
        }
    }
}

void AVCDEC_decode_scaling_mtx(AVC_Bitstream *bs)
{

    if (H264_GetVLC1(bs))
    {
        /* Intra, Y */
        AVCDEC_decode_scaling_list(bs, 16);
        /* Intra, Cr */
        AVCDEC_decode_scaling_list(bs, 16);
        /* Intra, Cb */
        AVCDEC_decode_scaling_list(bs, 16);
        /* Inter, Y */
        AVCDEC_decode_scaling_list(bs, 16);
        /* Inter, Cr */
        AVCDEC_decode_scaling_list(bs, 16);
        /* Inter, Cb */
        AVCDEC_decode_scaling_list(bs, 16);
        /* Intra, 8x8 */
        AVCDEC_decode_scaling_list(bs, 64);
        /* Inter, 8x8 */
        AVCDEC_decode_scaling_list(bs, 64);
    }
}


#define BASELINE            66          /* !< YUV 4:2:0/8  "Baseline" */
#define MAIN                77          /* !< YUV 4:2:0/8  "Main" */
#define EXTENDED            88          /* !< YUV 4:2:0/8  "Extended" */
#define FREXT_HP            100         /* !< YUV 4:2:0/8  "High" */
#define FREXT_Hi10P         110         /* !< YUV 4:2:0/10 "High 10" */
#define FREXT_Hi422         122         /* !< YUV 4:2:2/10 "High 4:2:2" */
#define FREXT_Hi444         244         /* !< YUV 4:4:4/12 "High 4:4:4" */
#define FREXT_CAVLC444      44          /* !< YUV 4:4:4/14 "CAVLC 4:4:4" */


int video_info_H264_InterpretSPS_x(AVC_Bitstream *bs, VIDEO_ES_INFO *video_info)
{
    int i;
    int poc_cycle_length;
    int frame_mbs_only_flag;
    int poc_type;
    int profile_idc;
    int vui_parameters_present_flag,hr;

    profile_idc = H264_GetVLCNx(bs, 8);

    if ((profile_idc != BASELINE       ) &&
            (profile_idc != MAIN           ) &&
            (profile_idc != EXTENDED       ) &&
            (profile_idc != FREXT_HP       ) &&
            (profile_idc != FREXT_Hi10P    ) &&
            (profile_idc != FREXT_Hi422    ) &&
            (profile_idc != FREXT_Hi444    ) &&
            (profile_idc != FREXT_CAVLC444 )
       )
    {
        printf("AVCDEC_interpret_sps failed: unsupported profile_idc.\n");
        return 0;
    }

    H264_GetVLC1(bs);                   /* sps: constrained_set0_flag */
    H264_GetVLC1(bs);                   /* sps: constrained_set1_flag */
    H264_GetVLC1(bs);                   /* sps: constrained_set2_flag */
    H264_GetVLC1(bs);                   /* sps: constrained_set3_flag */

    /* sps: reserved_zero_4bits */
    CHECK_ENCODED_PARAM(H264_GetVLCNx(bs, 4), 0);

    H264_GetVLCNx(bs, 8);               /* level_idc */

    H264_read_linfox(bs);               /* sps: seq_parameter_set_id */

    if ((profile_idc == FREXT_HP      ) ||
            (profile_idc == FREXT_Hi10P   ) ||
            (profile_idc == FREXT_Hi422   ) ||
            (profile_idc == FREXT_Hi444   ) ||
            (profile_idc == FREXT_CAVLC444)
       )
    {
        /* "SPS: chroma_format_idc" */
        CHECK_ENCODED_PARAM(H264_read_linfox(bs), 1);

        /* sps: bit_depth_luma_minus8 */
        CHECK_ENCODED_PARAM(H264_read_linfox(bs), 0);
        /* sps: bit_depth_chroma_minus8 */
        CHECK_ENCODED_PARAM(H264_read_linfox(bs), 0);
        /* sps: lossless_qpprime_y_zero_flag */
        CHECK_ENCODED_PARAM(H264_GetVLC1(bs), 0);

        AVCDEC_decode_scaling_mtx(bs);
    }

    H264_read_linfox(bs);               /* "SPS: log2_max_frame_num_minus4" */
    poc_type = H264_read_linfox(bs);    /* sps: pic_order_cnt_type */

    if (poc_type == 0)
    {
        /* "SPS: log2_max_pic_order_cnt_lsb_minus4" */
        H264_read_linfox(bs);
    }
    else if (poc_type == 1)
    {
        H264_GetVLC1(bs);               /* delta_pic_order_always_zero_fla */
        H264_read_se_x(bs);	            /* offset_for_non_ref_pic */
        H264_read_se_x(bs);	            /* offset_for_top_to_bottom_field */

        poc_cycle_length = H264_read_linfox(bs);

        if (poc_cycle_length > 128)
        {
            return 0;
        }

        for (i = 0; i < poc_cycle_length; i++)
        {
            H264_read_se_x(bs);
        }
    }
    else if (poc_type != 2)
    {
        printf("AVCDEC_interpret_sps failed: invalid  poc type.\n");
        return 0;
    }

    video_info->num_ref_frames = H264_read_linfox(bs);

    /* sps: gaps_in_framenum_allowed_flag, 有时码流设成0，用来容错处理 */
    H264_GetVLC1(bs);

    /* SPS: pic_width_in_mbs_minus1 */
    video_info->width  = (H264_read_linfox(bs) + 1) * 16;
    /* SPS: pic_height_in_map_units_minus1 */
    video_info->height = (H264_read_linfox(bs) + 1) * 16;


    /* sps: frame_mbs_only_flag */
    frame_mbs_only_flag = H264_GetVLC1(bs);

    if (!frame_mbs_only_flag)
    {
        video_info->interlace = 1;
        video_info->height  <<= 1;
        H264_GetVLC1(bs);
    }
    else
    {
        video_info->interlace = 0;
    }
    H264_GetVLC1(bs);                   /* sps: direct_8x8_inference_flag */

    /* sps: frame_cropping_flag */
    video_info->crop_flag=H264_GetVLC1(bs);
    if(video_info->crop_flag)
    {
        H264_read_linfox(bs);
        video_info->crop_right  = H264_read_linfox(bs)*2;
        H264_read_linfox(bs);
        video_info->crop_bot    = H264_read_linfox(bs)*2*(2-frame_mbs_only_flag);
        video_info->height     -= video_info->crop_bot;
    }

    /* sps: vui_parameters_present_flag */
    vui_parameters_present_flag = H264_GetVLC1(bs);
    if(vui_parameters_present_flag)
    {
        hr = video_info_H264_get_vui_parameters(bs, video_info);
        if (hr != 1)
        {
            return hr;
        }
    }

    return 1;
}


/******************************************************************************
* 功  能：获取视频信息
* 参  数：
*
* 返回值：成功获取到视频信息则返回1，否则返回0
* 备  注：
******************************************************************************/
int seekVideoInfoAvc(unsigned char *buffer, unsigned int length, VIDEO_ES_INFO *video_info)
{
    AVC_Bitstream Bitstream;
    AVC_Bitstream *bs = &Bitstream;
    video_info->frame_rate = 0;
    buffer += 5;                        /* 0000000165   jump over */
    H264_init_bitstreamx(bs, buffer, length);
    video_info->low_delay = 1;
    if(!video_info_H264_InterpretSPS_x(bs, video_info))
    {
        return 0;
    }

    return 1;
}
/* 获取Jpeg图像宽高 */
#define JPEGDEC_PROTECT_SIZE 128

typedef enum {/* JPEG marker codes */
    M_SOF0  = 0xc0,
    M_SOF1  = 0xc1,
    M_SOF2  = 0xc2,
    M_SOF3  = 0xc3,

    M_SOF5  = 0xc5,
    M_SOF6  = 0xc6,
    M_SOF7  = 0xc7,

    M_JPG   = 0xc8,
    M_SOF9  = 0xc9,
    M_SOF10 = 0xca,
    M_SOF11 = 0xcb,

    M_SOF13 = 0xcd,
    M_SOF14 = 0xce,
    M_SOF15 = 0xcf,

    M_DHT   = 0xc4,

    M_DAC   = 0xcc,

    M_RST0  = 0xd0,
    M_RST1  = 0xd1,
    M_RST2  = 0xd2,
    M_RST3  = 0xd3,
    M_RST4  = 0xd4,
    M_RST5  = 0xd5,
    M_RST6  = 0xd6,
    M_RST7  = 0xd7,

    M_SOI   = 0xd8,
    M_EOI   = 0xd9,
    M_SOS   = 0xda,
    M_DQT   = 0xdb,
    M_DNL   = 0xdc,
    M_DRI   = 0xdd,
    M_DHP   = 0xde,
    M_EXP   = 0xdf,

    M_APP0  = 0xe0,
    M_APP1  = 0xe1,
    M_APP2  = 0xe2,
    M_APP3  = 0xe3,
    M_APP4  = 0xe4,
    M_APP5  = 0xe5,
    M_APP6  = 0xe6,
    M_APP7  = 0xe7,
    M_APP8  = 0xe8,
    M_APP9  = 0xe9,
    M_APP10 = 0xea,
    M_APP11 = 0xeb,
    M_APP12 = 0xec,
    M_APP13 = 0xed,
    M_APP14 = 0xee,
    M_APP15 = 0xef,

    M_JPG0  = 0xf0,
    M_JPG13 = 0xfd,
    M_COM   = 0xfe,

    M_TEM   = 0x01,

    M_ERROR = 0x100
} JPEGDEC_MARKER;

typedef struct
{
    int             get_buffer;     /* current bit-extraction buffer */
    int             bits_left;      /* # of unused bits in it */

    unsigned char   *ptr;           /* next byte to write in buffer */
    unsigned char   *buffer;        /* start of buffer */
    int             size;           /* buffer size */
} BITSTREAM;

/******************************************************************************
* 功  能: 获取下一个marker.
* 参  数：bs  				- 码流指针
*         unread_marker     -读取到的marker指针
* 返回值：返回状态码
* 备  注：
* Note that the result might not be a valid marker code,
* but it will never be 0 or FF.
******************************************************************************/
int JPEGDEC_Next_Marker (BITSTREAM * bs, int * unread_marker)
{
    int c;
    for (;;)
    {
        c = *bs->ptr++;
        /* Skip any non-FF bytes.
        * This may look a bit inefficient, but it will not occur in a valid file.
        * We sync after each discarded byte so that a suspending data source
        * can discard the byte from its buffer.
        */
        while ((c != 0xFF) && (((PhysAddr)bs->ptr - (PhysAddr)bs->buffer) < (bs->size - JPEGDEC_PROTECT_SIZE)) )
        {
            c = *bs->ptr++;
        }
        /* This loop swallows any duplicate FF bytes.  Extra FFs are legal as
        * pad bytes, so don't count them in discarded_bytes.  We assume there
        * will not be so many consecutive FF bytes as to overflow a suspending
        * data source's input buffer.
        */
        do
        {
            c = *bs->ptr++;
        } while ((c == 0xFF) && (((PhysAddr)bs->ptr - (PhysAddr)bs->buffer) < (bs->size - JPEGDEC_PROTECT_SIZE)) );

        if ((c != 0) || (((PhysAddr)bs->ptr - (PhysAddr)bs->buffer) >= (bs->size - JPEGDEC_PROTECT_SIZE)) )
        {
            break; /* found a valid marker, exit loop */
        }
        /* Reach here if we found a stuffed-zero data sequence (FF/00).
        * Discard it and loop back to try again.	*/
    }

    if (((PhysAddr)bs->ptr - (PhysAddr)bs->buffer) >= (bs->size - JPEGDEC_PROTECT_SIZE))
    {
        return -1;
    }

    *unread_marker = c;
    return 0;
}


/******************************************************************************
* Description:跳跃无用marker
*
* Comment    :
* Date       :2010/8/5
Skip over an unknown or uninteresting variable-length marker
******************************************************************************/
int JPEGDEC_Skip_Variable (BITSTREAM * bs)
{
    int length;

    length = ((int) *bs->ptr++) << 8;
    length += *bs->ptr++;

    length -= 2;

    bs->ptr += length;

    return 0;
}

/******************************************************************************
* Description:获取图像尺寸
*
* Comment    :
* Date       :2010/8/5
******************************************************************************/
int JPEGDEC_Get_Image_Size (unsigned char *buffer, unsigned int length, VIDEO_ES_INFO *video_info)
{
    BITSTREAM bs_temp;
    int unread_marker = 0;
    int height,width;
    int RetVal;

    if ( buffer == 0 )
    {
        return -1;
    }


    bs_temp.buffer = buffer;
    bs_temp.size   = length;
    bs_temp.ptr    = buffer;

    for (;;)
    {
        if (unread_marker == 0)
        {
            RetVal = JPEGDEC_Next_Marker(&bs_temp, &unread_marker);
            if ( RetVal != 0 )
            {
                return RetVal;
            }
        }
        switch (unread_marker)
        {
        case M_SOI:
            break;
        case M_SOF0:
        case M_SOF1:

            bs_temp.ptr += 3;
            height = ((int) *bs_temp.ptr++) << 8;
            height += *bs_temp.ptr++;
            width = ((int) *bs_temp.ptr++) << 8;
            width += *bs_temp.ptr++;

            video_info->height= height;
            video_info->width= width;

            return 0;

        case M_SOF2:
        case M_SOF9:
        case M_SOF10:
        case M_SOF3:
        case M_SOF5:
        case M_SOF6:
        case M_SOF7:
        case M_JPG:
        case M_SOF11:
        case M_SOF13:
        case M_SOF14:
        case M_SOF15:
        case M_SOS:
        case M_DRI:
        case M_EOI:
        case M_DAC:
        case M_DHT:
        case M_DQT:
        case M_APP0:
        case M_APP1:
        case M_APP2:
        case M_APP3:
        case M_APP4:
        case M_APP5:
        case M_APP6:
        case M_APP7:
        case M_APP8:
        case M_APP9:
        case M_APP10:
        case M_APP11:
        case M_APP12:
        case M_APP13:
        case M_APP14:
        case M_APP15:
        case M_COM:
        case M_RST0:
        case M_RST1:
        case M_RST2:
        case M_RST3:
        case M_RST4:
        case M_RST5:
        case M_RST6:
        case M_RST7:
        case M_TEM:
        case M_DNL:
            RetVal = JPEGDEC_Skip_Variable(&bs_temp);
            if (RetVal )
            {
                return -1;
            }
            break;

        default:
        {
            return -1;
        }
        }
        unread_marker = 0;
    } /* end loop */
}


