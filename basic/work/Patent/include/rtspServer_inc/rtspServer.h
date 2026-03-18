#ifndef __RTSP_SERVER_H__
#define __RTSP_SERVER_H__

typedef enum nalu_type {
	NALU_PSLICE = 1,    //P帧
	NALU_ISLICE = 5,    //I帧
	NALU_SEI = 6,       //SEI 
	NALU_SPS = 7,       //SPS
	NALU_PPS = 8,	    //PPS
	NALU_VPS = 9,	    //VPS
	NALU_AUDIO = 0,     //AUDIO
} nalu_type_t;

typedef enum enc_type {
	ENC_UNDEF = 0,
	ENC_H264  = 1,   //H264
	ENC_H265  = 2,   //H265
	ENC_PS    = 3,   //H265
	ENC_VIDEO = 4,
	ENC_PCMA  = 5,   //PCM_A
	ENC_PCMU  = 6,   //PCM_U
	ENC_AAC   = 7,   //AAC
    ENC_AUDIO
} enc_type_t;


enum {
    H264_RTP_STREAM_TYPE    = 1, //H264经过打包后后的RTP数据流，包含了RTP的TCP头部4个字节，目前DSP在用的方式    
    H264_FRAME_STREAM_TYPE  = 2, //整帧的H264数据流 
    H264_NAL_STREAM_TYPE    = 3, //分包的H264数据流，I帧的时候分成4个数据包.(sps + pps + sei + I)
    H265_RTP_STREAM_TYPE    = 4, //H265经过打包后后的RTP数据流，包含了RTP的TCP头部4个字节，目前DSP在用的方式    
    H265_FRAME_STREAM_TYPE  = 5, //整帧的H265数据流 
    H265_NAL_STREAM_TYPE    = 6, //分包的H265数据流，I帧的时候分成4个数据包.(vps + sps + pps + sei + I)
};


/*******************************************************************************
* 函数名  : start_strm_t
* 描  述  : rtsp回调函数，启动编码
* 输  入  : - user_no:客户编号
*         : - chn:vi通道
*         : - chn_type:venc通道
	    : -venc_type:视频编码类型,1:264,2:H:265 见enc_type_t
           : -aenc_type:音频编码类型,4:PCMA,5:PCMU 见enc_type_t
* 输  出  : 无
* 返回值  : 无视
*******************************************************************************/
typedef int (*start_strm_t)(unsigned int user_no,unsigned int chn,unsigned int chn_type
                              ,unsigned int venc_type,unsigned int aenc_type);

/*******************************************************************************
* 函数名  : stop_strm_t
* 描  述  : rtsp回调函数，停止编码
* 输  入  : - user_no:客户编号
*         : - chn:vi通道
*         : - chn_type:venc通道
* 输  出  : 无
* 返回值  : 无视
*******************************************************************************/
typedef int (*stop_strm_t)(unsigned int user_no,unsigned int chn,unsigned int chn_type);
/*******************************************************************************
* 函数名  : get_strm_t
* 描  述  : rtsp回调函数
* 输  入  : - buf 数据地址
*         : - buf_size: 数据buff大小
*         : - chn:vi通道
*         : - chn_type:venc通道
*         : - user_no:rtsp client 号。第几个客户端
*         : - data_type:数据类型(H264_RTP_STREAM_TYPE,H264_FRAME_STREAM_TYPE,H264_NAL_STREAM_TYPE)三种，目前只做了2种。
*         : - timestamp:数据时间戳(data_type为H264_RTP_STREAM_TYPE时不用赋值)
*         : - frame_type:帧类型[I帧、P帧、A帧](data_type为H264_RTP_STREAM_TYPE时不用赋值)
* 输  出  : 无
* 返回值  : 返回获取到的RTP包的长度 : 成功
*           -1 : 失败
*******************************************************************************/
typedef int (*get_strm_t)(void *buf,unsigned int buf_size, unsigned int chn,unsigned int chn_type,
    unsigned int user_no,unsigned int *data_type,unsigned int *timestamp,unsigned int *frame_type);

/*******************************************************************************
* 函数名  : UnitTest_rtspServerInitCallBack
* 描  述  : rtsp回调函数，停止编码
* 输  入  : - rtspport:rtsp server 端口
*         : - start_stream:start venc 回调
*         : - stop_stream:stop venc 回调
*         : - get_stream:获取数据 回调(目前只做了RTP包)
* 输  出  : 无
* 返回值  : -1: 失败
          : 0 :  成功
*******************************************************************************/
int UnitTest_rtspServerInitCallBack(unsigned short rtspport,void *start_stream, void *stop_stream, void *get_stream);

/*******************************************************************************
* 函数名  : UnitTest_rtspServerStartSevervice
* 描  述  : rtsp server启动服务器，执行一次即可。
* 输  入  : - 无
* 输  出  : 无
* 返回值  : -1: 失败
          : 0 :  成功
*******************************************************************************/
int UnitTest_rtspServerStartSevervice(char videType, char audType);

#endif
