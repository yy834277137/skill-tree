#include <string.h>
#include <stdio.h>
#include "video_dec_commom.h"
#include "sal.h"
#define OPEN_MAX_SUB_LAYERS 2

#define MAX_DPB_SIZE      16
#define SLOW_MIN(x, y)      ((x) < (y)? (x): (y))
#define MAX(_1_, _2_) ((_1_) > (_2_) ? (_1_) : (_2_))

typedef struct _OPEN_BITSTREAM_T
{
    unsigned char  *initbuf;
    unsigned int  bitscnt;
} OPEN_BITSTREAM;

typedef struct _OPENHEVC_ShortTermRPS {
    unsigned int num_negative_pics;
    int num_delta_pocs;
    unsigned int delta_poc[32];
    unsigned char used[32];
} OPENShortTermRPS;

typedef struct _OPENHEVC_HEVCWindow {
    int left_offset;
    int right_offset;
    int top_offset;
    int bottom_offset;
} OPENHEVCWindow;


typedef struct _OPENHEVC_SPS_T_
{
    unsigned char vps_id;
    int chroma_format_idc;
    unsigned char separate_colour_plane_flag;

    OPENHEVCWindow pic_conf_win;

    int seq_parameter_set_id;
    int bit_depth;

    unsigned int log2_max_poc_lsb;
    int pcm_enabled_flag;

    int max_sub_layers;
    struct {
        int max_dec_pic_buffering;
        int num_reorder_pics;
        int max_latency_increase;
    } temporal_layer[OPEN_MAX_SUB_LAYERS];

    unsigned char scaling_list_enable_flag;

    unsigned int nb_st_rps;
    OPENShortTermRPS st_rps[64];
    unsigned int max_num_pocs;

    unsigned char amp_enabled_flag;
    unsigned char sao_enabled;

    unsigned char long_term_ref_pics_present_flag;
    unsigned char num_long_term_ref_pics_sps;

    struct {
        unsigned char bit_depth;
        unsigned char bit_depth_chroma;
        unsigned int log2_min_pcm_cb_size;
        unsigned int log2_max_pcm_cb_size;
        unsigned char loop_filter_disable_flag;
    } pcm;

    unsigned int log2_min_cb_size;
    unsigned int log2_diff_max_min_coding_block_size;
    unsigned int log2_min_tb_size;
    unsigned int log2_ctb_size;

    int max_transform_hierarchy_depth_inter;
    int max_transform_hierarchy_depth_intra;

    ///< coded frame dimension in various units
    int width;
    int height;

}OPENHEVC_SPS;


// 指数哥伦布码开方表
static unsigned char OPENHEVC_LOG2_TAB[256] =
{
    0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

// 指数哥伦布码长度表
static unsigned char OPENHEVC_GOLOMB_VLC_LEN[512] =
{
    19,17,15,15,13,13,13,13,11,11,11,11,11,11,11,11,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

// 无符号指数哥伦布码字表
static unsigned char OPENHEVC_UE_GOLOMB_VLC_CODE[512] =
{
    32,32,32,32,32,32,32,32,31,32,32,32,32,32,32,32,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
    7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// 有符号指数哥伦布码字表
static char OPENHEVC_SE_GOLOMB_VLC_CODE[512]=
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    8, -8,  9, -9, 10,-10, 11,-11, 12,-12, 13,-13, 14,-14, 15,-15,
    4,  4,  4,  4, -4, -4, -4, -4,  5,  5,  5,  5, -5, -5, -5, -5,
    6,  6,  6,  6, -6, -6, -6, -6,  7,  7,  7,  7, -7, -7, -7, -7,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
   -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
   -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

static int OPENHEVC_ebsp_to_rbsp(unsigned char* streamBuffer, int len)
{
    int i, j, count;
    int x03_cnt = 0;

    j = 0;
    count = 0;
    for (i = 0; i < len; i++)
    {
        //starting from begin_bytepos to avoid header information
        if ((count == 2) && (streamBuffer[j] == 0x03))
        {
            memmove(&streamBuffer[j], &streamBuffer[j + 1], len - i - 1); //hcj20120303
            count = 0;
            i++;
            x03_cnt++;
        }
        if (streamBuffer[j] == 0x00)
        {
            count++;
        }
        else
        {
            count = 0;
        }
        j++;
    }

    return x03_cnt;
}

static void OPENHEVC_init_bitstream(OPEN_BITSTREAM * bs, unsigned char* buffer, int size)
{
    bs->bitscnt = 0;
    bs->initbuf = buffer;
}

static void OPENHEVC_skip_n_bits(OPEN_BITSTREAM *bs, int n)
{
    bs->bitscnt += n;
    return;
}

#define OPENHEVC_BITSWAP(bitswap_t) {unsigned int bitswap_tmp;                            \
    char *bitswap_src = (char *)&bitswap_t;     \
    char *bitswap_dst = (char *)&bitswap_tmp;   \
    bitswap_dst[0] = bitswap_src[3];        \
    bitswap_dst[1] = bitswap_src[2];        \
    bitswap_dst[2] = bitswap_src[1];        \
    bitswap_dst[3] = bitswap_src[0];        \
    bitswap_t      = bitswap_tmp;  }

static unsigned int OPENHEVC_read_n_bits(OPEN_BITSTREAM *bs, int n)
{
    unsigned int bcnt  = bs->bitscnt;
    unsigned int tmp   = *(unsigned int *)(bs->initbuf + (bcnt >> 3));

    OPENHEVC_BITSWAP(tmp);

    tmp <<= bcnt & 7;
    tmp >>= (32 - n);

    bs->bitscnt += n;

    return tmp;
}

static unsigned int OPENHEVC_show_n_bits(OPEN_BITSTREAM *bs, unsigned int n)
{
    unsigned int bcnt  = bs->bitscnt;
    unsigned int tmp   = *(unsigned int *)(bs->initbuf + (bcnt >> 3));

    OPENHEVC_BITSWAP(tmp);

    tmp <<= bcnt & 7;

    return tmp >> (32 - n);
}

static unsigned int OPENHEVC_show_n_bits_long(OPEN_BITSTREAM *bs, unsigned int n)
{
    if (!n) {
        return 0;
    } else if (n <= 25) {
        return OPENHEVC_show_n_bits(bs, n);
    } else {
        unsigned ret = OPENHEVC_read_n_bits(bs, 16) << (n - 16);
        ret |= OPENHEVC_read_n_bits(bs, n - 16);
        bs->bitscnt -= n;
        return ret;
    }
}

static int OPENHEVC_log2(unsigned int v)
{
    int n = 0;

    if (v & 0xffff0000)
    {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00)
    {
        v >>= 8;
        n += 8;
    }
    
    if (v > 255)
    {
       v = 255;
    }
    n += OPENHEVC_LOG2_TAB[v];

    return n;
}

static unsigned int OPENHEVC_read_n_bits_long(OPEN_BITSTREAM *bs, unsigned int n)
{
    if (!n) {
        return 0;
    } else if (n <= 25) {
        return OPENHEVC_read_n_bits(bs, n);
    } else {
        unsigned ret = OPENHEVC_read_n_bits(bs, 16) << (n - 16);
        return ret | OPENHEVC_read_n_bits(bs, n - 16);
    }
}

static unsigned int OPENHEVC_read_ue_golomb_long(OPEN_BITSTREAM *bs)
{
    unsigned int buf, log;

    buf = OPENHEVC_show_n_bits_long(bs, 32);
    log = 31 - OPENHEVC_log2(buf);
    OPENHEVC_skip_n_bits(bs, log);

    return OPENHEVC_read_n_bits_long(bs, log + 1) - 1;
    //return HEVCDEC_read_ue_golomb(bs);
}

static int OPENHEVC_read_se_golomb(OPEN_BITSTREAM *bs)
{
    int len;
    unsigned int bcnt  = bs->bitscnt;
    unsigned int tmp   = *(unsigned int *)(bs->initbuf + (bcnt >> 3));

    OPENHEVC_BITSWAP(tmp);

    tmp <<= (bcnt & 7);

    if (tmp >= (1 << 27))
    {
        tmp >>= 32 - 9;
        bs->bitscnt += OPENHEVC_GOLOMB_VLC_LEN[tmp];

        return OPENHEVC_SE_GOLOMB_VLC_CODE[tmp];
    }
    else
    {
        len = 2 * OPENHEVC_log2(tmp) - 31;
        tmp >>= len;
        bs->bitscnt += 32 - len;

        if (tmp & 1)
        {
            tmp = 0 - (tmp >> 1);
        }
        else
        {
            tmp =  (tmp >> 1);
        }

        return tmp;
    }
}

static unsigned int OPENHEVC_read_ue_golomb(OPEN_BITSTREAM *bs)
{
    int len;
    unsigned int bcnt  = bs->bitscnt;
    unsigned int tmp   = *(unsigned int *)(bs->initbuf + (bcnt >> 3));
    unsigned int re_tmp;

    OPENHEVC_BITSWAP(tmp);

    tmp <<= (bcnt & 7);

    if (tmp & 0xf8000000)
    {
        tmp >>= (32 - 9);
        bs->bitscnt += OPENHEVC_GOLOMB_VLC_LEN[tmp];

        return OPENHEVC_UE_GOLOMB_VLC_CODE[tmp];
    }
    else if (tmp & 0xff800000)
    {
        re_tmp = tmp;
        re_tmp >>= (32 - 9);

        len = OPENHEVC_GOLOMB_VLC_LEN[re_tmp];
        bs->bitscnt += len;

        tmp = tmp >> (32 - len);
        tmp--;

        return tmp;
    }
    else
    {
        len = 63 - 2 * OPENHEVC_log2(tmp);

        if ( len > 25 )
        {
            tmp = tmp | ( (bs->initbuf + (bcnt >> 3))[4] >> (8 - (bcnt & 7)) );
        }

        bs->bitscnt += len;
        tmp = tmp >> (32 - len);
        tmp--;

        return tmp;
    }
}

static int OPENHEVC_interpret_profiletilerlevel(OPEN_BITSTREAM *bs)
{
    int i;

    OPENHEVC_skip_n_bits(bs, 2);//profile_space
    OPENHEVC_skip_n_bits(bs, 1);//tier_flag
    OPENHEVC_skip_n_bits(bs, 5);//profile_idc

    for (i = 0; i < 32; i++)
    {
        OPENHEVC_skip_n_bits(bs, 1);//profile_compatibility_flag
    }

    OPENHEVC_skip_n_bits(bs, 1);//progressive_source_flag
    OPENHEVC_skip_n_bits(bs, 1);//interlaced_source_flag
    OPENHEVC_skip_n_bits(bs, 1);//non_packed_constraint_flag
    OPENHEVC_skip_n_bits(bs, 1);//frame_only_constraint_flag

    OPENHEVC_skip_n_bits(bs, 16); // XXX_reserved_zero_44bits[0..15]
    OPENHEVC_skip_n_bits(bs, 16); // XXX_reserved_zero_44bits[16..31]
    OPENHEVC_skip_n_bits(bs, 12); // XXX_reserved_zero_44bits[32..43]

    return 0;
}

static int OPENHEVC_interpret_ptl(OPENHEVC_SPS *sps, OPEN_BITSTREAM *bs, int max_num_sub_layers)
{
    int i;
    int level_idc;
    int sub_layer_profile_present_flag[7];
    int sub_layer_level_present_flag[7];
    int sub_layer_level_idc[7] = {0};

    OPENHEVC_interpret_profiletilerlevel(bs);

    level_idc   = OPENHEVC_read_n_bits(bs, 8);//general_level_idc

    for (i = 0; i < max_num_sub_layers - 1; i++)
    {
        sub_layer_profile_present_flag[i] = OPENHEVC_read_n_bits(bs, 1);
        sub_layer_level_present_flag[i]   = OPENHEVC_read_n_bits(bs, 1);
    }

    if (max_num_sub_layers - 1 > 0)
    {
        for (i = max_num_sub_layers - 1; i < 8; i++)
        {
            OPENHEVC_skip_n_bits(bs, 2); // reserved_zero_2bits[i]
        }
    }

    for (i = 0; i < max_num_sub_layers - 1; i++)
    {
        if (sub_layer_profile_present_flag[i])
        {
            OPENHEVC_interpret_profiletilerlevel(bs);
        }
        if (sub_layer_level_present_flag[i])
        {
            sub_layer_level_idc[i] = OPENHEVC_read_n_bits(bs, 8);
        }
    }

    if(0 == level_idc)
    printf("level_idc %d,sub_layer_level_idc0 %d\n",level_idc,sub_layer_level_idc[0]);

    return 0;
}

static int OPENHEVC_scaling_list_data_for_sps(OPEN_BITSTREAM *bs)
{
    unsigned char scaling_list_pred_mode_flag[4][6];
    int scaling_list_dc_coef[2][6] = {0};
    int size_id, matrix_id, i;// pos;

    for (size_id = 0; size_id < 4; size_id++)
    {
        for (matrix_id = 0; matrix_id < (size_id == 3 ? 2 : 6); matrix_id++)
        {
            scaling_list_pred_mode_flag[size_id][matrix_id] = OPENHEVC_read_n_bits(bs, 1);
            if (!scaling_list_pred_mode_flag[size_id][matrix_id])
            {
                unsigned int delta = OPENHEVC_read_ue_golomb_long(bs);//scaling_list_pred_matrix_id_delta

                if (delta)
                {
                    // Copy from previous array.
                    if (matrix_id < delta)
                    {
                        //debug_printf("Invalid delta in scaling list data: %d.\n", delta);
                        return -1;
                    }

                }
            }
            else
            {
                int coef_num;
                int scaling_list_delta_coef = 0;

                coef_num  = SLOW_MIN(64, 1 << (4 + (size_id << 1)));
                if (size_id > 1)
                {
                    scaling_list_dc_coef[size_id - 2][matrix_id] = OPENHEVC_read_se_golomb(bs) + 8;
                }
                for (i = 0; i < coef_num; i++)
                {
                    scaling_list_delta_coef = OPENHEVC_read_se_golomb(bs);
                }
                printf("scaling_list_delta_coef %d, %d\n",scaling_list_delta_coef,scaling_list_dc_coef[0][0]);
            }
        }
    }

    return 0;
}

static int OPENHEVC_decode_short_term_rps_interpret(OPEN_BITSTREAM *bs, OPENShortTermRPS *rps,
    OPENHEVC_SPS *sps, int is_slice_header)
{
    unsigned char rps_predict = 0;
    int delta_poc;
    int k0 = 0;
    int k1 = 0;
    int k  = 0;
    int i;

    if (rps != sps->st_rps && sps->nb_st_rps)
    {
        rps_predict = OPENHEVC_read_n_bits(bs, 1);
    }

    if (rps_predict)
    {
        const OPENShortTermRPS *rps_ridx;
        int delta_rps, abs_delta_rps;
        unsigned char use_delta_flag = 0;
        unsigned char delta_rps_sign;

        if (is_slice_header)
        {
            unsigned int delta_idx = OPENHEVC_read_ue_golomb(bs) + 1;
            if (delta_idx > sps->nb_st_rps)
            {
                //debug_printf("Invalid value of delta_idx in slice header RPS: %d > %d.\n",
                //  delta_idx, sps->nb_st_rps);
                return -1;
            }
            rps_ridx = &sps->st_rps[sps->nb_st_rps - delta_idx];
        }
        else
        {
            rps_ridx = &sps->st_rps[rps - sps->st_rps - 1];
        }

        delta_rps_sign = OPENHEVC_read_n_bits(bs, 1);
        abs_delta_rps  = OPENHEVC_read_ue_golomb(bs) + 1;
        delta_rps      = (1 - (delta_rps_sign << 1)) * abs_delta_rps;
        for (i = 0; i <= rps_ridx->num_delta_pocs; i++)
        {
            int used = rps->used[k] = OPENHEVC_read_n_bits(bs, 1);

            if (!used)
            {
                use_delta_flag = OPENHEVC_read_n_bits(bs, 1);
            }

            if (used || use_delta_flag)
            {
                if (i < rps_ridx->num_delta_pocs)
                {
                    delta_poc = delta_rps + rps_ridx->delta_poc[i];
                }
                else
                {
                    delta_poc = delta_rps;
                }
                rps->delta_poc[k] = delta_poc;
                if (delta_poc < 0)
                {
                    k0++;
                }
                else
                {
                    k1++;
                }
                k++;
            }
        }

        rps->num_delta_pocs    = k;
        rps->num_negative_pics = k0;
        // sort in increasing order (smallest first)
        if (rps->num_delta_pocs != 0)
        {
            int used, tmp;
            for (i = 1; i < rps->num_delta_pocs; i++)
            {
                delta_poc = rps->delta_poc[i];
                used      = rps->used[i];
                for (k = i - 1; k >= 0; k--)
                {
                    tmp = rps->delta_poc[k];
                    if (delta_poc < tmp)
                    {
                        rps->delta_poc[k + 1] = tmp;
                        rps->used[k + 1]      = rps->used[k];
                        rps->delta_poc[k]     = delta_poc;
                        rps->used[k]          = used;
                    }
                }
            }
        }
        if ((rps->num_negative_pics >> 1) != 0)
        {
            int used;
            k = rps->num_negative_pics - 1;
            // flip the negative values to largest first
            for (i = 0; i < rps->num_negative_pics >> 1; i++)
            {
                delta_poc         = rps->delta_poc[i];
                used              = rps->used[i];
                rps->delta_poc[i] = rps->delta_poc[k];
                rps->used[i]      = rps->used[k];
                rps->delta_poc[k] = delta_poc;
                rps->used[k]      = used;
                k--;
            }
        }
    }
    else
    {
        unsigned int prev, nb_positive_pics;
        rps->num_negative_pics = OPENHEVC_read_ue_golomb(bs);
        nb_positive_pics       = OPENHEVC_read_ue_golomb(bs);

        if (rps->num_negative_pics >= 16 ||
            nb_positive_pics >= 16)
        {
            //debug_printf("Too many refs in a short term RPS.\n");
            return -1;
        }

        rps->num_delta_pocs = rps->num_negative_pics + nb_positive_pics;
        if (rps->num_delta_pocs)
        {
            prev = 0;
            for (i = 0; i < rps->num_negative_pics; i++)
            {
                delta_poc = OPENHEVC_read_ue_golomb(bs) + 1;
                prev -= delta_poc;
                rps->delta_poc[i] = prev;
                rps->used[i]      = OPENHEVC_read_n_bits(bs, 1);
            }
            prev = 0;
            for (i = 0; i < nb_positive_pics; i++)
            {
                delta_poc = OPENHEVC_read_ue_golomb(bs) + 1;
                prev += delta_poc;
                rps->delta_poc[rps->num_negative_pics + i] = prev;
                rps->used[rps->num_negative_pics + i]      = OPENHEVC_read_n_bits(bs, 1);
            }
        }
    }

    if (sps->max_num_pocs < rps->num_delta_pocs)
    {
        sps->max_num_pocs = rps->num_delta_pocs;
    }


    return 0;
}

static int OPENHEVC_interpret_sps(OPEN_BITSTREAM *bs, VIDEO_INFO *param)
{
    int i;
    int ret = 0;
    //int code;
    //int poc_cycle_length;
    //int frame_mbs_only_flag;
    //int poc_type;
    //int profile_idc;
    int pic_conformance_flag;
    int bit_depth_chroma;
    int sublayer_ordering_info;
    int start;
    int log2_diff_max_min_transform_block_size;
    //int vui_present;
    OPENHEVC_SPS sps_data, *sps = &sps_data;
    //unsigned int num_units_in_tick;//, time_scale;
    VIDEO_HEVC_INFO *hevc_info = param->codec_specific.hevc_info;

    sps->vps_id            =  OPENHEVC_read_n_bits(bs, 4);
    sps->max_sub_layers    =  OPENHEVC_read_n_bits(bs, 3) + 1;

    //sps_temporal_id_nesting_flag
    OPENHEVC_skip_n_bits(bs, 1);

    OPENHEVC_interpret_ptl(sps, bs, sps->max_sub_layers);

    sps->seq_parameter_set_id = OPENHEVC_read_ue_golomb_long(bs);

    sps->chroma_format_idc    = OPENHEVC_read_ue_golomb_long(bs);

    if (sps->chroma_format_idc == 3)
    {
        sps->separate_colour_plane_flag = OPENHEVC_read_n_bits(bs, 1);
    }

    sps->width  = OPENHEVC_read_ue_golomb_long(bs);
    sps->height = OPENHEVC_read_ue_golomb_long(bs);

    pic_conformance_flag = OPENHEVC_read_n_bits(bs, 1);
    if (pic_conformance_flag)
    {
        sps->pic_conf_win.left_offset   = OPENHEVC_read_ue_golomb_long(bs) * 2;
        sps->pic_conf_win.right_offset  = OPENHEVC_read_ue_golomb_long(bs) * 2;
        sps->pic_conf_win.top_offset    = OPENHEVC_read_ue_golomb_long(bs) * 2;
        sps->pic_conf_win.bottom_offset = OPENHEVC_read_ue_golomb_long(bs) * 2;
    }
    else
    {
        memset(&sps->pic_conf_win,  0, sizeof(OPENHEVCWindow));
    }

    hevc_info->bit_width_flag = 0;
    sps->bit_depth   = OPENHEVC_read_ue_golomb_long(bs) + 8;
    bit_depth_chroma = OPENHEVC_read_ue_golomb_long(bs) + 8;
    if (bit_depth_chroma != sps->bit_depth)
    {
        return -1;
    }
    if (sps->bit_depth > 8)
    {
        hevc_info->bit_width_flag = 1;
    }

    if (sps->chroma_format_idc == 1)
    {
        switch (sps->bit_depth) {
        case 8:   break;
        case 9:   break;
        case 10:  break;
        default:
            return -1;
        }
    }
    else
    {
        return -1;
    }

    sps->log2_max_poc_lsb = OPENHEVC_read_ue_golomb_long(bs) + 4;
    if (sps->log2_max_poc_lsb > 16)
    {
        return -1;
    }

    sublayer_ordering_info = OPENHEVC_read_n_bits(bs, 1);
    start = sublayer_ordering_info ? 0 : sps->max_sub_layers - 1;
    for (i = start; i < sps->max_sub_layers; i++) {
        sps->temporal_layer[i].max_dec_pic_buffering = OPENHEVC_read_ue_golomb_long(bs) + 1;
        sps->temporal_layer[i].num_reorder_pics      = OPENHEVC_read_ue_golomb_long(bs);
        sps->temporal_layer[i].max_latency_increase  = OPENHEVC_read_ue_golomb_long(bs) - 1;
        if (sps->temporal_layer[i].max_dec_pic_buffering > MAX_DPB_SIZE)
        {
            return -1;
        }
        if (sps->temporal_layer[i].num_reorder_pics > sps->temporal_layer[i].max_dec_pic_buffering - 1)
        {
            return -1;
        }
    }
    //hisi修改SDK后：支持从语法获得 temporal_layer数组长度OPEN_MAX_SUB_LAYERS
    if(i <= OPEN_MAX_SUB_LAYERS)
    {
        hevc_info->reffrm_num = MAX((sps->temporal_layer[i - 1].max_dec_pic_buffering - 1),1);
    }
    else
    {
        hevc_info->reffrm_num = 1;
    }

    if (!sublayer_ordering_info && (start < OPEN_MAX_SUB_LAYERS))
    {
        for (i = 0; i < start; i++)
        {
                sps->temporal_layer[i].max_dec_pic_buffering = sps->temporal_layer[start].max_dec_pic_buffering;
                sps->temporal_layer[i].num_reorder_pics      = sps->temporal_layer[start].num_reorder_pics;
                sps->temporal_layer[i].max_latency_increase  = sps->temporal_layer[start].max_latency_increase;
        }
    }

    sps->log2_min_cb_size                    = OPENHEVC_read_ue_golomb_long(bs) + 3;
    sps->log2_diff_max_min_coding_block_size = OPENHEVC_read_ue_golomb_long(bs);
    sps->log2_min_tb_size                    = OPENHEVC_read_ue_golomb_long(bs) + 2;
    log2_diff_max_min_transform_block_size   = OPENHEVC_read_ue_golomb_long(bs);

    if (sps->log2_min_tb_size >= sps->log2_min_cb_size)
    {
        printf("invalid sps  sps->log2_min_tb_size >= sps->log2_min_cb_size %d\n",log2_diff_max_min_transform_block_size);
        return -1;
    }
    sps->max_transform_hierarchy_depth_inter = OPENHEVC_read_ue_golomb_long(bs);
    sps->max_transform_hierarchy_depth_intra = OPENHEVC_read_ue_golomb_long(bs);

    sps->scaling_list_enable_flag = OPENHEVC_read_n_bits(bs, 1);
    if (sps->scaling_list_enable_flag)
    {
        if (OPENHEVC_read_n_bits(bs, 1))
        {
            ret = OPENHEVC_scaling_list_data_for_sps(bs);
            if (ret < 0)
            {
                return -1;
            }
        }
    }

    sps->amp_enabled_flag = OPENHEVC_read_n_bits(bs, 1);
    sps->sao_enabled      = OPENHEVC_read_n_bits(bs, 1);
    if (sps->sao_enabled)
    {

    }

    sps->pcm_enabled_flag = OPENHEVC_read_n_bits(bs, 1);
    if (sps->pcm_enabled_flag)
    {
        sps->pcm.bit_depth   = OPENHEVC_read_n_bits(bs, 4) + 1;
        sps->pcm.bit_depth_chroma = OPENHEVC_read_n_bits(bs, 4) + 1;
        sps->pcm.log2_min_pcm_cb_size = OPENHEVC_read_ue_golomb_long(bs) + 3;
        sps->pcm.log2_max_pcm_cb_size = sps->pcm.log2_min_pcm_cb_size +
            OPENHEVC_read_ue_golomb_long(bs);
        if (sps->pcm.bit_depth > sps->bit_depth)
        {
            return -1;
        }

        sps->pcm.loop_filter_disable_flag = OPENHEVC_read_n_bits(bs, 1);
    }

    sps->nb_st_rps = OPENHEVC_read_ue_golomb_long(bs);
    if (sps->nb_st_rps > 64)
    {
        return -1;
    }

    sps->max_num_pocs = 0;
    for (i = 0; i < sps->nb_st_rps; i++) {
        if ((ret = OPENHEVC_decode_short_term_rps_interpret(bs, &sps->st_rps[i], sps, 0)) < 0)
            return -1;
    }

    sps->long_term_ref_pics_present_flag = OPENHEVC_read_n_bits(bs, 1);
    sps->num_long_term_ref_pics_sps = 0;
    if (sps->long_term_ref_pics_present_flag)
    {
        sps->num_long_term_ref_pics_sps = OPENHEVC_read_ue_golomb_long(bs);
    }

    // 关键参数正确性判断
    if ((sps->width < (sps->pic_conf_win.left_offset + sps->pic_conf_win.right_offset))
        || (sps->height < (sps->pic_conf_win.top_offset + sps->pic_conf_win.bottom_offset)))
    {
        return -1;
    }

    //OUTPUT
    param->img_width  = sps->width;
    param->img_height = sps->height;

    hevc_info->log2_ctb_size = sps->log2_min_cb_size + sps->log2_diff_max_min_coding_block_size;

    // 这边short长度 + 1为short
    //hevc_info->reffrm_num = sps->max_num_pocs + 1 + sps->num_long_term_ref_pics_sps;

    hevc_info->crop_left   = sps->pic_conf_win.left_offset;
    hevc_info->crop_right  = sps->pic_conf_win.right_offset;
    hevc_info->crop_top    = sps->pic_conf_win.top_offset;
    hevc_info->crop_bottom = sps->pic_conf_win.bottom_offset;

    return 0;
}

static int OPENHEVC_interpret_pps(OPEN_BITSTREAM *bs, VIDEO_INFO *param)
{
    unsigned int pps_id;
    unsigned int sps_id;
    int num_ref_idx_l0_default_active; ///< num_ref_idx_l0_default_active_minus1 + 1
    int num_ref_idx_l1_default_active; ///< num_ref_idx_l1_default_active_minus1 + 1
    unsigned char cu_qp_delta_enabled_flag;
    int cb_qp_offset;
    int cr_qp_offset;
    unsigned char entropy_coding_sync_enabled_flag;

    // Coded parameters
    pps_id = OPENHEVC_read_ue_golomb(bs);
    if (pps_id >= 4)
    {
        //debug_printf("PPS id out of range: %d\n", pps_id);
        return -1;
    }
    sps_id = OPENHEVC_read_ue_golomb(bs);
    if (sps_id >= 2) {
        //debug_printf("SPS id out of range: %d\n", sps_id);
        return -1;
    }

    OPENHEVC_skip_n_bits(bs, 1);//dependent_slice_segments_enabled_flag
    OPENHEVC_skip_n_bits(bs, 1);//output_flag_present_flag
    OPENHEVC_skip_n_bits(bs, 3);//num_extra_slice_header_bits
    OPENHEVC_skip_n_bits(bs, 1);//sign_data_hiding_flag
    OPENHEVC_skip_n_bits(bs, 1);//cabac_init_present_flag

    num_ref_idx_l0_default_active = OPENHEVC_read_ue_golomb_long(bs) + 1;
    num_ref_idx_l1_default_active = OPENHEVC_read_ue_golomb_long(bs) + 1;

    OPENHEVC_read_se_golomb(bs);//pic_init_qp_minus26

    OPENHEVC_skip_n_bits(bs, 1);//constrained_intra_pred_flag
    OPENHEVC_skip_n_bits(bs, 1);//transform_skip_enabled_flag

    cu_qp_delta_enabled_flag = OPENHEVC_read_n_bits(bs, 1);
    if (cu_qp_delta_enabled_flag)
    {
        OPENHEVC_read_ue_golomb_long(bs);//diff_cu_qp_delta_depth
    }

    cb_qp_offset = OPENHEVC_read_se_golomb(bs);
    if (cb_qp_offset < -12 || cb_qp_offset > 12)
    {
        //debug_printf("pps_cb_qp_offset out of range: %d\n",
        //  cb_qp_offset);
        return -1;
    }
    cr_qp_offset = OPENHEVC_read_se_golomb(bs);
    if (cr_qp_offset < -12 || cr_qp_offset > 12)
    {
        //debug_printf("pps_cr_qp_offset out of range: %d\n",
        //  cr_qp_offset);
        return -1;
    }
    OPENHEVC_skip_n_bits(bs, 1);//pic_slice_level_chroma_qp_offsets_present_flag

    OPENHEVC_skip_n_bits(bs, 1);//weighted_pred_flag
    OPENHEVC_skip_n_bits(bs, 1);//weighted_bipred_flag

    OPENHEVC_skip_n_bits(bs, 1);//transquant_bypass_enable_flag
    OPENHEVC_skip_n_bits(bs, 1);//tiles_enabled_flag
    entropy_coding_sync_enabled_flag = OPENHEVC_read_n_bits(bs, 1);

    param->codec_specific.hevc_info->wpp_sync_flag = entropy_coding_sync_enabled_flag;
    printf("%d,%d\n",num_ref_idx_l0_default_active,num_ref_idx_l1_default_active);
    return 0;

}

static int OPENHEVC_preread_slice_header(OPEN_BITSTREAM * bs, int nal_unit_type)
{
    int first_slice_in_pic_flag, slice_type;

    first_slice_in_pic_flag = OPENHEVC_read_n_bits(bs, 1);

    if (first_slice_in_pic_flag == 0)
    {
        return -2; // 不是第一个slice，无法确定帧类型
    }

    if (nal_unit_type >= 16 && nal_unit_type <= 23)
    {
        OPENHEVC_read_n_bits(bs, 1);//no_output_of_prior_pics_flag
    }

    OPENHEVC_read_ue_golomb(bs);//pps_id

    //当前标准下，num_extra_slice_header_bits永远为0

    slice_type = OPENHEVC_read_ue_golomb(bs);

    if (slice_type >= 0 && slice_type <= 2)
    {
        return slice_type;
    }
    else
    {
        return -2;//错误的类型（码流有错）
    }
}

/***************************************************************************************************
* 功  能：从SPS的NALU单元中解析图像宽高
* 参  数：nalu_buf    [in]    包含SPS的NALU单元缓冲区（不含起始码）
*         buf_size    [in]    缓冲区长度
*         param       [io]    图像参数结构体
*
* 返回值：0 ：成功
*         -1：参数错误
* 备  注：
* 修  改:
***************************************************************************************************/
int OPENHEVC_GetPicSizeFromSPS_dsp(unsigned char *nalu_buf, int buf_size, VIDEO_INFO *param)
{
    OPEN_BITSTREAM bitstream;
    int x03_cnt;
    unsigned char * dataBuff = NULL;

    if (nalu_buf == NULL || param == NULL || param->codec_specific.hevc_info == NULL || buf_size <= 0)
    {
        return -1;
    }

    dataBuff = (unsigned char * )malloc(buf_size);

    if (NULL == dataBuff)
    { 
        return -1;
    }
    param->img_width              = 0;
    param->img_height             = 0;

    memcpy(dataBuff,nalu_buf,buf_size);
    x03_cnt = OPENHEVC_ebsp_to_rbsp(dataBuff, buf_size);
    OPENHEVC_init_bitstream(&bitstream, dataBuff + 2, buf_size - x03_cnt);
    OPENHEVC_interpret_sps(&bitstream, param);
    free(dataBuff);
    return 0;
}

/***************************************************************************************************
* 功  能：从PPS的NALU单元中解析图像宽高
* 参  数：nalu_buf    [in]    包含PPS的NALU单元缓冲区（不含起始码）
*         buf_size    [in]    缓冲区长度
*         param       [io]    图像参数结构体
*
* 返回值：0 ：成功
*         -1：参数错误
* 备  注：
* 修  改:
***************************************************************************************************/
int OPENHEVC_GetPicSizeFromPPS_dsp(unsigned char* nalu_buf, int buf_size, VIDEO_INFO *param)
{
    OPEN_BITSTREAM bitstream;
    int x03_cnt;


    if (nalu_buf == NULL || param == NULL || param->codec_specific.hevc_info == NULL || buf_size <= 0)
    {
        return -1;
    }

    x03_cnt = OPENHEVC_ebsp_to_rbsp(nalu_buf, buf_size);
    OPENHEVC_init_bitstream(&bitstream, nalu_buf + 2, buf_size - x03_cnt);
    OPENHEVC_interpret_pps(&bitstream, param);

    return 0;
}

/***************************************************************************************************
* 功  能：从SLICE的NALU单元中解析帧类型
* 参  数：nalu_buf    [in]    包含SPS的NALU单元缓冲区（不含起始码）
*         buf_size    [in]    缓冲区长度
*
* 返回值：0 ：B帧
*         1 ：P帧
*         2 ：I帧
*         -1：输入参数有误
*         -2：码流中没有帧类型信息
* 备  注：
* 修  改:
***************************************************************************************************/
int OPENHEVC_GetFrameTypeFromSlice_dsp(unsigned char *nalu_buf, int buf_size)
{
    OPEN_BITSTREAM bitstream, *bs = &bitstream;
    int x03_cnt;
    int nal_type, nuh_layer_id, temporal_id;

    if (nalu_buf == NULL || buf_size <= 0)
    {
        return -1;
    }

    x03_cnt = OPENHEVC_ebsp_to_rbsp(nalu_buf, buf_size);
    OPENHEVC_init_bitstream(&bitstream, nalu_buf, buf_size - x03_cnt);

    OPENHEVC_read_n_bits(bs, 1);
    nal_type = OPENHEVC_read_n_bits(bs, 6);
    nuh_layer_id = OPENHEVC_read_n_bits(bs, 6);
    temporal_id = OPENHEVC_read_n_bits(bs, 3) - 1;

    if (nuh_layer_id == 0)
    {
        if ((nal_type >= 0 && nal_type <= 9) || (nal_type >= 16 && nal_type <= 21))
        {
            return OPENHEVC_preread_slice_header(bs, nal_type);
        }
    }

    printf("temporal_id %d\n",temporal_id);
    return -2;
}

/***************************************************************************************************
* 功  能：从NALU单元中读取NALU类型
* 参  数：nalu_buf    [in]    NALU单元缓冲区（不含起始码）
*         buf_size    [in]    缓冲区长度
*
* 返回值：>=0：nalu类型
*         -1 ：参数错误
* 备  注：
* 修  改:
***************************************************************************************************/
int OPENHEVC_GetNaluType_dsp(unsigned char *nalu_buf, int buf_size)
{
    unsigned short marker;
    int nalu_type;

    if (nalu_buf == NULL || buf_size < 2)
    {
        return -1;
    }

    marker    = *(short *)(nalu_buf);
    nalu_type = ((marker & 0x0000007e) >> 1);

    return nalu_type;
}
