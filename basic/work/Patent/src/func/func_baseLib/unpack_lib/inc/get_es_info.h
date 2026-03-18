#ifndef _GET_ELEMENTARY_STREAM_INFO_H_
#define _GET_ELEMENTARY_STREAM_INFO_H_

#define HIK_STREAM_INFO_CHECK_FAIL  0
#define HIK_STREAM_INFO_CHECK_OK    1

#define HIK_NO_HEADER               0

#define HIK_VIDEO_AVC_AUD           6
#define HIK_VIDEO_PARAM             5
#define HIK_HEADER_UNDEFINED        4
#define HIK_VIDEO_FRAME_IFRAME      3
#define HIK_VIDEO_FRAME_PFRAME      2
#define HIK_VIDEO_FRAME_BFRAME      1
#define HIK_AUDIO_FRAME_HEADER      1

typedef struct _VIDEO_ES_INFO_ 
{
    unsigned short    width;
    unsigned short    height;    
    unsigned short    crop_right;
    unsigned short    crop_bot;
    unsigned char    crop_flag;
    unsigned char   interlace;
    unsigned char   num_ref_frames;
    unsigned char   low_delay;
    float           frame_rate;
} VIDEO_ES_INFO;

typedef struct _AUDIO_ES_INFO_ 
{
    unsigned int    bit_rate;
    unsigned int    sample_rate;
    unsigned int    channel_num;
} AUDIO_ES_INFO;

typedef struct _AVC_BITSTREAM_
{
    unsigned char * Rdbfr;             // 码流缓冲区首地址
    unsigned char * Rdmax;             // 码流缓冲区最大的地址
    unsigned char * Rdptr;             // 当前读取的地址
    unsigned int b_num;             // 码字缓存中包含的有效位数
    unsigned int b_rack;            // 码字缓存
} AVC_Bitstream;

int checkFrameHeadUnknow(unsigned char *buffer, int length);

int checkFrameHeadM4v(unsigned char *buffer, int length);
int seekVideoInfoM4v(unsigned char *buffer, unsigned int length, VIDEO_ES_INFO *es_info);

int checkFrameHeadAvc(unsigned char *buffer, int length);
int seekVideoInfoAvc(unsigned char *buffer, unsigned int length, VIDEO_ES_INFO *es_info);

int checkFrameHeadMpg(unsigned char *buffer, int length);
int seekAudioInfoMpg(unsigned char *buffer, unsigned int length, AUDIO_ES_INFO *es_info);

#endif//.h
