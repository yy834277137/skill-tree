/**@file RtpUnpackDefine.h
*  @note Hangzhou Hikvision Digital Technology Co., Ltd. All Rights Reserved.
*  @brief 海康威视 RTPUNPACK 库的接口宏定义头文件
*
*  @author    辛安民
*  @date      2014/11/26
*
**  @note V1.0.0 宏定义与头文件分离

*  @warning   无
*/

#ifndef _RTP_UNPACK_DEFINE_H_
#define _RTP_UNPACK_DEFINE_H_

#include "type_dscrpt_common.h"
#include "DSP_hevc_sps_parse.h"
/******************************************************************************
* 宏声明
******************************************************************************/
#define HIK_RTP_UNPACK_LIB_VERSION      1.1.4               /* 当前版本号      */
#define HIK_RTP_UNPACK_LIB_DATE         0x20141013          /* 版本日期        */

#define RTP_DEF_VERSION         0x00                    /* default version         */
#define RTP_DEF_H264_FU_A       0x1c                    /* Fragmentation unit      */
#define RTP_HEAD_LEN            12                      /* rtp header length       */
#define SRTP_MAX_TAG_LEN        12                      /* srtp tag length         */
#define RTP_MAX_STREAMNUM       3                       /* max rtp stream number   */
#define DEFAULT_VIDEO_PT        0x60   ///<普通视频
#define DEFAULT_AUDIO_PT        0x0 ///<普通音频



/* 状态码：小于零表示有错误(0x80000000开始)，零表示失败，大于零表示成功  */
#define HIK_RTP_UNPACK_LIB_S_OK           0x00000001    /* 成功                    */
#define HIK_RTP_UNPACK_LIB_S_FAIL         0x00000000    /* 失败                    */
#define HIK_RTP_UNPACK_LIB_E_PARA_NULL    0x80000000    /* 参数指针为空            */
#define HIK_RTP_UNPACK_LIB_E_PARA_ERR     0x80000001    /* 外部输入参数错误        */
#define HIK_RTP_UNPACK_LIB_E_MEM_OVER     0x80000002    /* 内存溢出                */
#define HIK_RTP_UNPACK_LIB_E_STM_ERR      0x80000003    /* 不支持的流类型          */
#define HIK_RTP_UNPACK_LIB_E_SRTP         0x80000004    /* srtp相关错误            */
#define HIK_RTP_UNPACK_LIB_E_SN_ERR       0x80000005  /* RTP包序号出错     */

/******************************************************************************
* 结构体声明
******************************************************************************/
#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
    
typedef int HRESULT;
#endif // !_HRESULT_DEFINED


/**    @struct RTPUNPACK_PARAM 
*  @brief RTP 解包模块参数
*/
typedef struct _RTP_UNPACK_PARAM_
{
    unsigned int       buf_size;                 ///< 内部缓冲区内存大小
    unsigned int       stream_type;              ///< rtp所负载的原始流类型，0表示未知
    unsigned char *    buffer;                   ///< 内部缓冲区内存指针，内存由用户申请、释放

} RTPUNPACK_PARAM;

/**    @struct RTPUNPACK_PARAM 
*  @brief 处理过程参数
*/
typedef struct _RTP_UNPACK_PROCESS_PARAM_
{
    unsigned char * inbuf;          //  rtp包缓冲区，包括包头
    unsigned int    inbuf_len;
    unsigned int    stream_type;
    unsigned char * outbuf;
    unsigned char * outBufBak;
    unsigned int    outbuf_len;
    unsigned int    outbuf_size;
    unsigned int    time_stamp;
    unsigned int    is_privt_info;      //输出数据是否为私有数据(比如智能信息)
    unsigned int    privt_info_frame_num; //该私有数据对应的视频帧的帧号
    unsigned int    privt_info_type;    //私有数据类型，参见<<海康标准RTP封装语法设计V0.92>>的3.1.2
                                        //小节中关于private data type的描述
    unsigned int    privt_info_subtype; //如果输出的是私有数据，如智能信息，用来表示私有数据的子类型，
                                        //如MetaData、EventData等，其值表示的意义参见<<海康标准RTP封装
                                        //语法设计V0.92>>的3.1.3小节中关于IVS数据的描述
    unsigned int    is_new_frame;   //只表示视频
    unsigned int    is_end_nalu;    //只表示视频
    unsigned int    marker_bit;
    unsigned int    payload_type;
    unsigned int    video_frame_count;
    unsigned int    sequence_number;
    unsigned int    status;
    unsigned int    audio_frame_stock_len;
    unsigned int    imgQuant;
    unsigned int    bAggregation;
    unsigned int    bSearchNal;

    /*H264解析*/
    unsigned int    flgNal;
    unsigned int    videoPayloadType;
    unsigned int    audioPayloadType;
    unsigned int    privatePayloadType;
    //时标处理
    unsigned int    lastVRtpTime;
    unsigned int    frmCnt;
    unsigned int    lastPRtpTime;

    unsigned int    info_valid;
    unsigned int    compTime;
    unsigned int    width;
    unsigned int    height;
	unsigned int    vpsLen;
    unsigned int    spsLen;
    unsigned int    ppsLen;

    unsigned int    bFirstPrivatePt;
    unsigned int    lastPrivateRtpTime;
    unsigned int    lastPrivateTime;
    unsigned int    privateCompTime;
	unsigned int    streamType;
  
    HIK_STREAM_INFO *stream_info;         ///< 流信息指针
} RTPUNPACK_PRC_PARAM;

#ifndef RTP_SECURE_DEFINED
#define RTP_SECURE_DEFINED

/**    @struct sec_serv_t 
*  @brief services enum type
*/
typedef enum 
{
    SEC_SERV_NONE          = 0, /**< no services                        */
    SEC_SERV_CONF          = 1, /**< confidentiality                    */
    SEC_SERV_AUTH          = 2, /**< authentication                     */
    SEC_SERV_CONF_AND_AUTH = 3  /**< confidentiality and authentication */
}sec_serv_t;

/**    @struct CIPHER_TYPE 
*  @brief ciper type
*/
typedef enum
{
    NULL_CIPHER = 0,
    AES_128_ICM,
    SEAL,
    AES_128_CBC
}CIPHER_TYPE;

/**    @struct AUTH_TYPE 
*  @brief auth type
*/
typedef enum
{
    NULL_AUTH = 0,
    UST_TMMHv2,
    UST_AES_128_XMAC,
    HMAC_SHA1
}AUTH_TYPE;

/**    @struct RTP_SECURE_INFO 
*  @brief rtp secure info
*/
typedef struct _RTP_SECURE_INFO_
{
    unsigned int    rtp_secure;            ///< 是否按照SRTP方式进行打包
    unsigned int    ssrc;                  ///< 数据同步源值
    unsigned int    sec_serv;              ///< SRTP加密方式
    CIPHER_TYPE     cipher_type;           ///< 负载加密方式
    unsigned int    cipher_key_len;        ///< 负载密钥长度
    AUTH_TYPE       auth_type;             ///< 认证加密方式
    unsigned int    auth_key_len;          ///< 认证密钥长度
    unsigned int    auth_tag_len;          ///< 认证标识长度
    unsigned int    mki_tag;               ///< 是否添加主密钥               
    unsigned char   master_key[256];       ///< 主密钥
}RTP_SECURE_INFO;

#endif

/**@fn  HRESULT RtpUnpack_Process(RTPUNPACK_PRC_PARAM * param, void *handle)
*  @brief  <解析一段 rtp 数据>
*  @param param  [IN] 参数结构指针
*  @param handle  [IN] 内部参数模块句柄
*  @return  返回起始码长度
*/
HRESULT RtpUnpack_Process(RTPUNPACK_PRC_PARAM * param, void *handle);

/**@fn  unsigned int add_avc_annexb_startcode(unsigned char *buffer)
*  @brief  <获取RTP_UNPACK所需内存大小>
*  @param param  [IN] 参数结构指针
*  @return  返回起始码长度
*/

/**  @fn  HRESULT RtpUnpack_Create(RTPUNPACK_PARAM *param, void **handle)
*  @brief  <创建 RTP_UNPACK 模块>
*  @param param   [IN] 参数结构指针
*  @param handle  [IN] 内部参数模块句柄
*  @return  返回起始码长度
*/
HRESULT RtpUnpack_Create(RTPUNPACK_PARAM *param, void **handle);
HRESULT RtpUnpack_GetMemSize(RTPUNPACK_PARAM *param);

void RtpUnpack_SetDbLeave(int level);
#endif/* _RTP_UNPACK_DEFINE_H_ */