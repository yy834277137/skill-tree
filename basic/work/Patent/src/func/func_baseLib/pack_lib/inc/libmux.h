
#ifndef _LIBMUX_H_
#define _LIBMUX_H_

#ifdef __cplusplus
extern "C"{
#endif

#define MUX_S_OK                    (0x0)
#define MUX_S_FAIL                  (-1)
#define MUX_S_NULL_POINTER_ERR      (0x4D555801)
#define MUX_S_NO_CHAN_ERR           (0x4D555802)
#define MUX_S_TYPE_ERR              (0x4D555803)
#define MUX_S_CHAN_ERR              (0x4D555804)
#define MUX_S_NOT_ENOUGH_MEM_ERR    (0x4D555805)
#define MUX_S_CREATE_ERR            (0x4D555806)
#define MUX_S_NOT_USED              (0x4D555807)
#define MUX_S_NOT_CREATED           (0x4D555808)
#define MUX_S_PROC_ERR              (0x4D555809)
#define MUX_S_PROC_NO_DATA          (0x4D55580A)

typedef enum _MUX_TYPE_
{
    MUX_RTP    = 0,  /* RTP 封装*/
    MUX_PS     ,     /* PS  封装*/
    MUX_NONE
}MUX_TYPE;

typedef enum
{
    MUX_VID = 0,
    MUX_AUD,
    MUX_PRIVT,
    MUX_DATA_NONE
}MUX_DATA_TYPE;

typedef struct
{
    unsigned int largestDist;
    unsigned int seq_video;
    unsigned int seq_audio;
	unsigned int time_stamp;
}MUX_INFO;

typedef struct
{
    int            muxHdl;
    unsigned char *bufAddr;
    unsigned int   bufLen; 
    unsigned int   maxPktLen;
    unsigned int   audPack;
}MUX_PARAM;

typedef struct  _MUX_GLOBAL_TIME_
{
    unsigned int    year;        /* 全局时间年，2008 年输入2008*/
    unsigned int    month;       /* 全局时间月，1 - 12*/
    unsigned int    date;        /* 全局时间日，1 - 31*/
    unsigned int    hour;        /* 全局时间时，0 - 23*/
    unsigned int    minute;      /* 全局时间分，0 - 59*/
    unsigned int    second;      /* 全局时间秒，0 - 59*/
    unsigned int    msecond;     /* 全局时间毫秒，0 - 999*/
} MUX_GLOBAL_TIME;

typedef struct
{
    unsigned char *       bufferAddr[5];   /* 最多5个slice */
    unsigned int          bufferLen[5];    /* 每个slice长度 */
	unsigned char *       vpsBufferAddr;
    unsigned int          vpsLen;
    unsigned char *       spsBufferAddr;
    unsigned int          spsLen;
    unsigned char *       ppsBufferAddr;
    unsigned int          ppsLen;

    unsigned int          bVideo;
    unsigned int          width;           /* 码流的宽 */
    unsigned int          height;          /* 码流的高 */
	unsigned int          fps;             /* 码流的帧率 */

    unsigned int          frame_type;        /* 帧的类型 */
    unsigned int          is_key_frame;      /* 是否是 I 帧 */
	unsigned int          audEnctype;        /* 音频编码的类型 */
	unsigned int          audChn;            /* 音频编码通道数*/
	unsigned int          audSampleRate;     /* 音频编码采样率*/
	unsigned int          audBitRate;        /* 音频编码bit 率*/
	unsigned int          audExInfo;         /* 音频编码补充信息;如aac码流是否包含头部*/

	MUX_GLOBAL_TIME       stGlobalTime;    /* 全局时间: 用于确认封装时是否需要重新填充时间 or 使用原先码流的封装时间 */

    unsigned int          naluNum;         /* 该帧总的 nalu 数 */
    unsigned long long    u64PTS;            /* 该帧对应的时标*/
}MUX_IN_BITS_INTO_S;

typedef struct
{
    unsigned char *   bufferAddr;   /* 码流地址 */
    unsigned int      bufferLen;    /* 码流缓冲长度 */
    unsigned int      streamLen;    /* 码流数据长度 */
}MUX_OUT_BITS_INTO_S;

typedef struct 
{
    MUX_IN_BITS_INTO_S   stInBuffer;   /* 输入数据的信息 */
    MUX_OUT_BITS_INTO_S  stOutBuffer;  /* 打包后输出数据的信息 */
}MUX_DATA_INFO_S;

typedef struct
{
    int             muxHdl;
    MUX_DATA_INFO_S muxData;
}MUX_PROC_INFO_S;

typedef struct
{
    MUX_DATA_TYPE   muxType;
    int  width;
    int  height;
}MUX_STREAM_INFO_S;

typedef enum 
{
    MUX_GET_CHAN = 0,
    MUX_CREATE,
    MUX_GET_STREAMSIZE,
    MUX_PRCESS,
    MUX_GET_INFO,
    MUX_IDX_NONE
}MUX_IDX;

int MuxInit();
int MuxControl(MUX_IDX idx,void *inBuf,void *outBuf);


#ifdef __cplusplus
}
#endif

#endif /* _LIBCOM_H_ */


