
#ifndef _LIBDEMUX_H_
#define _LIBDEMUX_H_

#ifdef __cplusplus
extern "C"{
#endif

#define DEMUX_S_OK                    (0x0)
#define DEMUX_S_FAIL                  (-1)
#define DEMUX_S_NULL_POINTER_ERR      (0x4D555901)
#define DEMUX_S_NO_CHAN_ERR           (0x4D555902)
#define DEMUX_S_TYPE_ERR              (0x4D555903)
#define DEMUX_S_CHAN_ERR              (0x4D555904)
#define DEMUX_S_NOT_ENOUGH_MEM_ERR    (0x4D555905)
#define DEMUX_S_CREATE_ERR            (0x4D555906)
#define DEMUX_S_NOT_USED              (0x4D555907)
#define DEMUX_S_NOT_CREATED           (0x4D555908)
#define DEMUX_S_PROC_ERR              (0x4D555909)
#define DEMUX_S_PROC_NO_DATA          (0x4D55590A)

/* 码流类型 */
#define STREAM_FRAME_TYPE_UNDEF       (0x00)
#define STREAM_FRAME_TYPE_AUDIO       (0x01)
#define STREAM_FRAME_TYPE_VIDEO       (0x02)
#define STREAM_FRAME_TYPE_PRIVT       (0x03)
#define STREAM_FRAME_TYPE_METADATA    (0x04)

#define VID_I_FRAME                   (0x11)
#define VID_P_FRAME                   (0x12)

/* 音频编码类型 */
#define AUDIO_UNKOWN                0x0000
#define AUDIO_ADPCM                 0x1001
#define AUDIO_G729                  0x1002
#define AUDIO_G722_16               0x1011
#define AUDIO_G722_24               0x1012
#define AUDIO_G722_32               0x1013
#define AUDIO_G726_32               0x7260          //默认G726-32
#define AUDIO_AMR_NB                0x3000
#define AUDIO_MPEG                  0x2000          /*MPEG 系列音频，解码器能自适应各种MPEG*/
#define AUDIO_G711_U                0x7110
#define AUDIO_G711_A                0x7111
#define AUDIO_G722_1                0x7221          //G722.1
#define AUDIO_G723_1                0x7231          //G732.1
#define AUDIO_G726_U                0x7260          //默认G726-32
#define AUDIO_G726_A                0x7261
#define AUDIO_G726_16               0x7262
#define AUDIO_L16                   0x7001
#define AUDIO_AAC                   0x2001
#define AUDIO_RAW_DATA8             0x7000          //采样率为8k的原始数据
#define AUDIO_RAW_UDATA16           0x7001          //采样率为16k的原始数据，即L16
/*海康封装头标识，仅再海康封装使用*/
#define HIK_AUDIO_ADPCM             0x1001
#define HIK_AUDIO_G729              0x1002
#define HIK_AUDIO_G726_A            0x1026
#define HIK_AUDIO_G722_16           0x1011
#define HIK_AUDIO_G722_24           0x1012
#define HIK_AUDIO_G722_32           0x1013

#define DEM_AUDIO_OUT               1
#define DEM_VIDEO_OUT               (1<<1)
#define DEM_PRIVATE_OUT             (1<<2)

typedef enum tagDemIo
{
    DEM_SET_ORGBUF = 0,
    DEM_RESET,
    DEM_CALC_DATA,
    DEM_PROC_DATA,
    DEM_FULL_BUF,
    DEM_GET_HANDLE,
    DEM_CREATE,
    DEM_SET_TYPE,
    DEM_SET_ENC_TYPE,
    DEM_MAX
}DEM_IDX;

typedef enum _DEMUX_TYPE_
{
    DEM_RTP    = 0,  /* RTP 封装*/
    DEM_PS     ,     /* PS  封装*/
    DEM_NONE
}DEM_TYPE;

typedef struct  _DEM_GLOBAL_TIME_
{
    unsigned int    year;        /* 全局时间年，2008 年输入2008*/
    unsigned int    month;       /* 全局时间月，1 - 12*/
    unsigned int    date;        /* 全局时间日，1 - 31*/
    unsigned int    hour;        /* 全局时间时，0 - 23*/
    unsigned int    minute;      /* 全局时间分，0 - 59*/
    unsigned int    second;      /* 全局时间秒，0 - 59*/
    unsigned int    msecond;     /* 全局时间毫秒，0 - 999*/
} DEM_GLOBAL_TIME;


typedef struct
{
    unsigned int    bufType;
    unsigned int    frmType;
    unsigned int    timestamp;
    unsigned int    datalen;
    unsigned int    length;
    unsigned int    width;
    unsigned int    height;
	unsigned int    vpsLen;
    unsigned int    spsLen;
    unsigned int    ppsLen;
    unsigned int    contLen;
	unsigned char*  vpsAddr;  /*VPS信息所在地址 输出*/
    unsigned char*  spsAddr;  /*SPS信息所在地址 输出*/
    unsigned char*  ppsAddr;  /*PPS信息所在地址 输出*/
    unsigned char*  contAddr; /*数据所在地址;H264 P帧和I帧，音频帧 输出*/
    unsigned char*  addr;     /*输入地址*/
	unsigned int    isKeyFrame;
	unsigned int    frame_num;
	unsigned int    time_info;             /*两帧间pts时间间隔 */
	DEM_GLOBAL_TIME glb_time;              /* 全局时间                     */
    unsigned int    privtType;    
    unsigned int    privtSubType; 
	unsigned int    streamType; //0 h264,1 h265
	unsigned int    audEncType;
}DEM_BUFFER;

typedef struct
{
    unsigned int     bufLen;
    unsigned int     lenPerFrm;
    unsigned int     frmCnt;
    unsigned char*   bufAddr;
}DEM_OUT_BUF;

typedef struct
{
    int              demHdl;
    unsigned char*   bufAddr;
    unsigned int     bufLen; 
    unsigned int     demLevel;
}DEM_PARAM;

typedef struct
{
    int              demHdl;
    unsigned int     orgWIdx;
}DEM_CALC_PRM;

typedef struct
{
    unsigned int     orgRIdx;
    unsigned int     newFrm;
    unsigned int     frmType;
    unsigned int     timestamp;
}DEM_PRCO_OUT;

typedef struct
{
    unsigned int   orgbuflen;
    unsigned char* orgBuf;
}DEM_ORGBUF_PRM;

typedef struct
{
    int      demType;
    int      outType;
}DEM_CHN_PRM;

typedef enum tagStreamSize
{
    SIZE_2M = 0,
    SIZE_4M,
    SIZE_STREAM_MAX,
}STREAM_SIZE;

int DemuxInit();
int DemuxControl(DEM_IDX idx,void *inBuf,void *outBuf);
void DemuxSetDbLeave(int level,int unLevel);


#ifdef __cplusplus
}
#endif

#endif /* _LIBCOM_H_ */


