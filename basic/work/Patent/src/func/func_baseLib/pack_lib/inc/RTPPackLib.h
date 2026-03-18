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
*			输出缓冲区内的每个RTP包前加4字节表示长度。
*2009.02.09 根据重新定义的私有数据格式进行打包
*2009.06.08 添加私有数据打包接口RTPPACK_create_privt_info_rtp (黄崇基)
*2009.11.10 增加对AMR音频的打包支持(rfc4867)，打包时每次输入一帧数据
*2010.02.23 修改mjpeg打包取出文件头部分，并添加一些接口
*2010.06.21 针对低延时应用增加了每次输入数据为非完整一个nalu或非完整一帧的打包功能支持
*2012.08.11 在对AAC去头时判断，增加是否是音频帧的条件,并升级版本至2.00.00
*2012.10.12 增加mepg2编码的rtp打包方式，并升级版本至2.00.01
*2013.02.26 增加支持加入私有包，同时，将音视频和私有包的ssrc和包序号分开打。
*2013.07.05 增加在basic私有描述子中加入公司和相机信息，版本升至2.00.02
*2013.07.23 增加在mpeg2打包中specific header的TR字段
*2013.10.24 修改私有数据一个包打不下的情况，版本号升至2.00.03
*2013.11.13 统一ts，ps，rtp私有描述子打包文件
*2014.01.13 添加SVAC码流打包，版本号升级至2.00.04
*2014.03.01 添加H264加密码流打包，版本号升级至2.00.05
*2014.05.14 添加mpeg4、mjpeg加密码流打包,版本号升级至2.00.06
*2014.06.23 添加视频描述子中的轻存储标记,版本号升级至2.00.07
*2014.07.14 统一ts，ps，rtp私有描述子打包文件版本号升级至2.00.08
*2014.09.12 增加H265打包，版本号升级至2.00.09
*2015.01.04 增加温度信息打包，版本号升级至2.00.10
*2015.06.23 增加H.265加密打包，版本号升级至2.00.11
********************************************************************************/
#ifndef _HIK_RTPPACK_LIB_H_
#define _HIK_RTPPACK_LIB_H_

/******************************************************************************
* 作用域控制宏 (必须使用，以便C++语言能调用)
******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include "descriptor_set.h"
#include "type_dscrpt_common.h"
/******************************************************************************
* 宏声明
******************************************************************************/
#define HIK_RTPPACK_LIB_VERSION			2.00.08		///< 当前版本号
#define HIK_RTPPACK_LIB_DATE			0x20140714  ///< 版本日期

/* 状态码：小于零表示有错误(0x80000000开始)，零表示失败，大于零表示成功  */
#define HIK_RTPPACK_LIB_S_OK			0x00000001	///< 成功
#define HIK_RTPPACK_LIB_S_FAIL		    0x00000000	///< 失败
#define HIK_RTPPACK_LIB_E_PARA_NULL	    0x80000000	///< 参数指针为空
#define HIK_RTPPACK_LIB_E_MEM_OVER	    0x80000001	///< 内存溢出
#define HIK_RTPPACK_LIB_E_STREAM_TYPE   0x80000003	///< 流类型错误
#define HIK_RTPPACK_LIB_E_SRTP          0x80000004	///< SRTP处理错误

#define HIK_RTPPACK_PRIVT_PAYLOAD_TYPE	0x70		///< 私有负载数据的PT类型

// 打包类型
#define HIK_RTPPACK_SECRET_PACKET_RES           0x00   // 保留
#define HIK_RTPPACK_SECRET_PACKET_FUA           0x01   // FU_A打包方式
// 加密算法
#define HIK_RTPPACK_SECRET_ARITH_NONE           0x00   // 不加密
#define HIK_RTPPACK_SECRET_ARITH_AES            0x01   // AES加密
// 加密类型
#define HIK_RTPPACK_SECRET_TYPE_ZERO            0x00   // 保留
#define HIK_RTPPACK_SECRET_TYPE_16B             0x01   // 各帧前16byte加密
#define HIK_RTPPACK_SECRET_TYPE_4KB             0x02   // 各帧前4096byte加密
// 加密轮数
#define HIK_RTPPACK_SECRET_ROUND00              0x00   // 保留
#define HIK_RTPPACK_SECRET_ROUND03              0x01   // 3轮加密
#define HIK_RTPPACK_SECRET_ROUND10              0x02   // 10轮加密
// 密钥长度
#define HIK_RTPPACK_SECRET_KEYLEN000            0x00   // 保留
#define HIK_RTPPACK_SECRET_KEYLEN128            0x01   // 128bit
#define HIK_RTPPACK_SECRET_KEYLEN192            0x02   // 192bit
#define HIK_RTPPACK_SECRET_KEYLEN256            0x03   // 256bit


#define RTP_HEAD_LEN            12                      /* rtp header length       */

/******************************************************************************
* 结构体声明
******************************************************************************/
#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef int HRESULT;
#endif /*!_HRESULT_DEFINED*/ 

/** @struct _HIK_RTPPACK_ES_INFO_
 *  @brief  原始流参数
 */
typedef struct _HIK_RTPPACK_ES_INFO_
{
    unsigned int    stream_mode;           ///< 输入流模式
	unsigned int    watermark_mode;        ///< 水印输入模式，I帧或P帧或IP帧加水印
    unsigned int	video_stream_type;     ///< 输入视频流类型
    unsigned int	audio_stream_type;     ///< 输入音频流类型
    unsigned int    max_packet_len;        ///< 最大RTP包长度
	unsigned int    video_ssrc;            ///< RTP包视频数据同步源的识别标志，以区分不同的RTP发送端
	unsigned int 	audio_ssrc;			   ///< RTP包音频数据同步源的识别标志，以区分不同的RTP发送端
	unsigned int	privt_ssrc;			   ///< RTP包私有数据同步源的识别标志，以区分不同的RTP发送端

    unsigned int    SSRC; //
    unsigned int    video_payload_type;    ///< RTP包头视频负载类型，不能设置超过127
	unsigned int    audio_payload_type;    ///< RTP包头音频负载类型，不能设置超过127
    unsigned int    allow_ext;             ///< 允许包含扩展头
	unsigned int    allow_4byte_align;     ///< 是否需要包长4字节对齐

    HIK_STREAM_INFO stream_info;           ///< 流信息
} HIKRTP_PACK_ES_INFO;

/** @struct _HIK_RTP_STREAM_INFO_
 *  @brief  原始流参数
 */
typedef struct _HIK_RTP_STREAM_INFO_
{
    unsigned int    seq_video;           
    unsigned int    seq_audio; 
} HIKRTP_PACK_STREAM_INFO;

/** @struct _RTPPACK_PARAM_
 *  @brief  RTP 打包模块参数
 */
typedef struct _RTPPACK_PARAM_
{
    unsigned int            buffer_size;   ///< 数据块大小
    unsigned char       *   buffer;        ///< 数据块首地址
    HIKRTP_PACK_ES_INFO     info;          ///< 原始流信息 
} RTPPACK_PARAM;

/** @struct _RTPPACK_UNIT_
 *  @brief  数据块处理参数
 */
typedef struct _RTPPACK_UNIT_
{
    unsigned int	frame_type;			///< 输入帧类型，jpeg视频帧都设置为视频I帧
    unsigned int    is_first_unit;		///< 是否是一帧的第一个unit。标准H.264每帧会分成多个unit,其余编码每帧都只有一个unit
	unsigned int    is_last_unit;       ///< 是否是一帧的最后一个unit(或nalu)
	unsigned int    num_in_gop;         ///< 当前帧在gop中的序号
    
	/* 有些应用输入打包库的数据为非完整的一个unit（unit意义同上），
	   此时需要如下两个接口，对于每次都是输入完整一个unit的则下面两个接口都置为 1 */
	unsigned int    is_unit_start;      ///< 是否为一个unit的第一段数据
	unsigned int    is_unit_end;        ///< 是否为一个unit的最后一段数据

    unsigned int    is_key_frame;		///< 是否关键数据
    unsigned int    time_stamp;			///< 该帧的采样时标
	unsigned int	frame_num;			///< 当前视频帧的帧号

    unsigned char * unit_in_buf;		///< 输入帧缓冲区
    unsigned int    unit_in_len;		///< 输入原始流缓冲区长度

	/*	输出缓冲区rtp_out_buf内数据结构如下图，小格单位是byte
		每个输入unit形成若干rtp包，size网络序
       0 1 2 3 4 5 6 7 ... 0 1 2 3 4 5 6 7 ... 0 1 2 3 4 5 6 7 ...
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |  RTP PACK 0		  |  ...............  |  RTP PACK n		  | 
      +-------------------+-------------------+-------------------+
      |  size |  data.....|  ...............  |  size |  data.... | 
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
    unsigned char *	rtp_out_buf;		///< 输出缓冲区
	unsigned int	rtp_num;			///< 输出缓冲区内包含的RTP包个数
    unsigned int    rtp_out_len;		///< 输出缓冲区长度	
	unsigned int	rtp_out_buf_size;	///< 输出缓冲区大小
    /* 码流信息 */
	unsigned int    company_mark;       ///< 公司描述符
	unsigned int    camera_mark;        ///< 相机描述 
	unsigned int    codec_mode_request; ///< AMR音频打包需要的帧模式
    HIK_GLOBAL_TIME global_time;        ///< 全局时间信息

    /* mjpg rtp打包需要的额外数据，其他类型打包无效，"mjpg_spc"结构如下图，frag_offset由库内填充，
	   其余字节库外填充，type_specific填0，type填1，Q为编码质量(1-99)，width为 图像宽/8, 
	   height为 图像高/8 
	 0                   1                   2                   3
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	| type_specific |                frag_offset                    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|     type      |      Q        |      width    |     height    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
	unsigned char	mjpg_spc[8];        ///< mjpg rtp打包需要的额外数据        		
	unsigned int    mjpg_have_file_hdr; ///< 输入的jpeg帧是否含有文件头，"1"为是， "0"为否

	unsigned char   nalu_tab;        ///< 填入nalu类型，如0x67、0x68等; mpeg编码填0xBx。
	unsigned char   encrypt_round;   ///< 填入加密轮数
	unsigned char   encrypt_type;    ///< 加密类型
	unsigned char   encrypt_arith;   ///< 加密算法
	unsigned char   key_len;         ///< 密匙长度
	unsigned char   packet_type;     ///< 打包方式
	unsigned char   mpeg_frame_tab;  ///< mpeg4编码帧类型字段
	unsigned char   reserved[1];

} RTPPACK_PROCESS_PARAM;

#ifndef RTP_SECURE_DEFINED
#define RTP_SECURE_DEFINED

typedef enum 
{
    SEC_SERV_NONE          = 0, /**< no services                        */
    SEC_SERV_CONF          = 1, /**< confidentiality                    */
    SEC_SERV_AUTH          = 2, /**< authentication                     */
    SEC_SERV_CONF_AND_AUTH = 3  /**< confidentiality and authentication */
} SEC_SERV_T;

typedef enum
{
    NULL_CIPHER = 0,
    AES_128_ICM,
    SEAL,
    AES_128_CBC
}CIPHER_TYPE;

typedef enum
{
    NULL_AUTH = 0,
    UST_TMMHv2,
    UST_AES_128_XMAC,
    HMAC_SHA1
}AUTH_TYPE;

typedef struct _RTP_SECURE_INFO_
{
    unsigned int    rtp_secure;            ///< 是否按照SRTP方式进行打包
    unsigned int    ssrc;                  ///< 数据同步源值
    SEC_SERV_T      sec_serv;              ///< SRTP加密方式
    CIPHER_TYPE     cipher_type;           ///< 负载加密方式
    unsigned int    cipher_key_len;        ///< 负载密钥长度
    AUTH_TYPE       auth_type;             ///< 认证加密方式
    unsigned int    auth_key_len;          ///< 认证密钥长度
    unsigned int    auth_tag_len;          ///< 认证标识长度
    unsigned int    mki_tag;               ///< 是否添加主密钥               
    unsigned char   master_key[256];       ///< 主密钥
}RTP_SECURE_INFO;

#endif


/******************************************************************************
* 接口函数声明
******************************************************************************/

/** @fn      RTPPACK_GetMemSize(RTPPACK_PARAM *param)
 *  @brief   获取所需内存大小,参数结构中 buffer_size变量用来表示所需内存大小
 *  @param   param       [IN] 参数结构指针
 *  @return  返回错误码
 */
HRESULT RTPPACK_GetMemSize(RTPPACK_PARAM *param); 

/** @fn      RTPPACK_Create(RTPPACK_PARAM *param, void **handle)
 *  @brief   创建 RTPPACK 模块
 *  @param   param       [INOUT] 参数结构指针
 *  @param   **handle    [OUT]   返回RTPMUX模块句柄
 *  @return  返回错误码
 */
HRESULT RTPPACK_Create(RTPPACK_PARAM *param, void **handle); 

/** @fn      RTPPACK_Process(void *handle, RTPPACK_PROCESS_PARAM *param)
 *  @brief   打包一段数据块
 *  @param   handle       [IN] 句柄( handle 由 RTPPACK_Create 返回)
 *  @param   param        [IN]   处理单元参数
 *  @return  返回错误码
 */
HRESULT RTPPACK_Process(void *handle, RTPPACK_PROCESS_PARAM *param);

/** @fn      RTPPACK_ResetStreamInfo(void *handle, HIKRTP_PACK_ES_INFO *info)
 *  @brief   重置参考数据
 *  @param   handle       [IN] 句柄( handle 由 RTPPACK_Create 返回)
 *  @param   info         [IN] 参考数据句柄
 *  @return  返回错误码
 */
HRESULT RTPPACK_ResetStreamInfo(void *handle, HIKRTP_PACK_ES_INFO *info);

/** @fn      RTPPACK_SetSecureInfo(void *handle, RTPPACK_SECURE_INFO *info)
 *  @brief   设置SRTP加密信息
 *  @param   handle       [IN] 句柄( handle 由 RTPPACK_Create 返回)
 *  @param   info         [IN] 加密信息
 *  @return  返回错误码
 */
HRESULT RTPPACK_SetSecureInfo(void *handle, RTP_SECURE_INFO *info);

/** @fn      RTPPACK_GetStreamInfo(void *handle, HIKRTP_PACK_STREAM_INFO *info)
 *  @brief   获取封装的一些参数
 *  @param   handle       [IN] 句柄( handle 由 RTPPACK_Create 返回)
 *  @param   info         [IN] 加密信息
 *  @return  返回错误码
 */
HRESULT RTPPACK_GetStreamInfo(void *handle, HIKRTP_PACK_STREAM_INFO *stream_info);

#ifdef __cplusplus
}
#endif 

#endif /* _HIK_RTPPACK_LIB_H_ */
