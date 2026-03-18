/***********************************************************************************************************************
* 
* 版权信息：版权所有 (c) 2020, 杭州海康威视软件有限公司, 保留所有权利
* 
* 文件名称：aacenc.h
* 摘    要：海康威视 AAC编码库的接口函数声明文件
*
* 历史版本：2.2.4
* 作    者：付加飞
* 日    期：2020年8月11日
* 备    注：修复某些情况下通道数判断异常的bug
*
* 历史版本：2.2.3
* 作    者：付加飞
* 日    期：2020年4月28日
* 备    注：开源扫描修改的版本
*
* 历史版本：2.1.3
* 作    者：付加飞
* 日    期：2019年12月31日
* 备    注：修复数组越界bug，当前版本的编码结果和 v1.7.4 -- v2.1.0版本之间的结果一致
*
* 历史版本：2.1.2
* 作    者：付加飞
* 日    期：2019年05月22日
* 备    注：修复bug，使最新版的编码结果和1.7.4之前版本的一致
*
* 历史版本：2.1.0
* 作    者：钱能锋
* 日    期：2018年08月27日
* 备    注：解决sfBandTab表格访问越界的问题
*
* 历史版本：2.0.0
* 作    者：钱能锋
* 日    期：2018年01月15日
* 备    注：解决不同平台编码结果不一致的bug
*
* 历史版本：1.7.4
* 作    者：陈展
* 日    期：2017年06月20日
* 备    注：在aac编解码库表格增加static关键字
*
* 历史版本：1.7.0
* 作    者：陈展
* 日    期：2015年03月26日
* 备    注：增加AAC V1.7.0版本

* 历史版本：1.5.0
* 作    者：陈扬坤
* 日    期：2014年12月6日
* 备    注：增加aac解码库中imdct增加判断，解决可能由于错误而出现挂死bug
*
* 历史版本：1.4.0
* 作    者：陈扬坤
* 日    期：2014年3月7日
* 备    注：升级AAC编码库，使支持48K采样率

* 历史版本：1.3.0
* 作    者：陈扬坤
* 日    期：2013年6月7日
* 备    注：解决因第三方码流，而出现解码声音错误bug，并解决其他因码流错误而出现的死循环bug

* 历史版本：1.2.0
* 作    者：陈扬坤
* 日    期：2011年10月12日
* 备    注：更改接口，改有包含audio_common.h

* 历史版本：1.1.0
* 作    者：陈扬坤
* 日    期：2011年9月25日
* 备    注：控制AAC编码码率，并优化编码库,整合编解码库，统一接口

* 历史版本：1.0.0
* 作    者：陈扬坤
* 日    期：2010年12月25日
* 备    注：AAC编码
***********************************************************************************************************************/

#ifndef _AACCODEC_H_
#define _AACCODEC_H_

#ifdef __cplusplus
extern "C"{
#endif

//开启宏定义使用新版本V1.7.0编码库，否则使用旧版本编码库
#define CZ_NEW_AAC


#include "datatypedef.h"
#include "mem_tab.h"
#include "audio_codec_common.h"

/***********************************************************************************************************************
*文件版本和时间宏声明
***********************************************************************************************************************/
// 当前版本号
// 主版本号，接口改动、功能增加、架构变更时递增，最大63
#define			HIK_AACCODEC_MAJOR_VERSION                     2
// 子版本号，性能优化、局部结构调整、模块内集成其他库的主版本提升时递增，最大31
#define			HIK_AACCODEC_SUB_VERSION                       2
// 修正版本号，修正bug后递增，最大31
#define			HIK_AACCODEC_REVISION_VERSION                  4
// 版本日期
#define			HIK_AACCODEC_VER_YEAR						   20             // 年
#define			HIK_AACCODEC_VER_MONTH						    8             // 月
#define			HIK_AACCODEC_VER_DAY						   11             // 日

#define			HIK_AACDEC_LIB_E_INDATA_UNDERFLOW			   0x81000000
#define  		HIK_AACDEC_LIB_E_NULL_POINTER                  0x81000001
#define  		HIK_AACDEC_LIB_E_INVALID_ADTS_HEADER           0x81000002
#define 		HIK_AACDEC_LIB_E_INVALID_ADIF_HEADER           0x81000003
#define 		HIK_AACDEC_LIB_E_INVALID_FRAME                 0x81000004
#define 		HIK_AACDEC_LIB_E_MPEG4_UNSUPPORTED             0x81000005
#define 		HIK_AACDEC_LIB_E_CHANNEL_MAP                   0x81000006
#define 		HIK_AACDEC_LIB_E_SYNTAX_ELEMENT                0x81000007
#define 		HIK_AACDEC_LIB_E_DEQUANT                       0x81000008
#define 		HIK_AACDEC_LIB_E_STEREO_PROCESS                0x81000009
#define 		HIK_AACDEC_LIB_E_AAC_PNS                       0x8100000a
#define 		HIK_AACDEC_LIB_E_SHORT_BLOCK_DEINT             0x8100000b
#define 		HIK_AACDEC_LIB_E_TNS                           0x8100000c
#define 		HIK_AACDEC_LIB_E_AAC_IMDCT                     0x8100000d
#define 		HIK_AACDEC_LIB_E_NCHANS_TOO_HIGH               0x8100000e 
#define 		HIK_AACDEC_LIB_E_SBR_INIT                      0x81000010
#define 		HIK_AACDEC_LIB_E_SBR_BITSTREAM                 0x81000011
#define 		HIK_AACDEC_LIB_E_SBR_DATA                      0x81000012
#define 		HIK_AACDEC_LIB_E_SBR_PCM_FORMAT                0x81000013
#define 		HIK_AACDEC_LIB_E_SBR_NCHANS_TOO_HIGH           0x81000014
#define 		HIK_AACDEC_LIB_E_SBR_SINGLERATE_UNSUPPORTED    0x81000015 
#define 		HIK_AACDEC_LIB_E_RAWBLOCK_PARAMS               0x81000016 
#define 		HIK_AACDEC_LIB_E_UNKNOWN					   0x81000017

#ifndef		    AAC_MAX_NCHANS                          
#define		    AAC_MAX_NCHANS                                 2
#endif
#define		    AAC_MAX_NSAMPS                                 1024
#define		    AAC_MAINBUF_SIZE                               (768 * AAC_MAX_NCHANS)

#define		    AAC_NUM_PROFILES                               3
#define		    AAC_PROFILE_MP                                 0
#define		    AAC_PROFILE_LC                                 1
#define		    AAC_PROFILE_SSR                                2

//#define AAC_ENABLE_SBR

// define these to enable decoder features
#if defined(HELIX_FEATURE_AUDIO_CODEC_AAC_SBR)
#define AAC_ENABLE_SBR
#endif //  HELIX_FEATURE_AUDIO_CODEC_AAC_SBR.
#define AAC_ENABLE_MPEG4

/***********************************************************************************************************************
* AAC编码码率设置范围如下:                                                                                                   
* 8K采样                                                                                                                     
* 单声道 bitrate  8K,  16K, 32K                                                                                              
* 双声道 bitrate  16K, 32K, 64K  
*
* 16K 采样率                                                                                                                 
* 单声道 bitrate   8K，16K，32K, 64K                                                                                          
* 双声道 bitrate  16K, 32K, 64K, 128k
*
* 32K                                                                                                                       
* 单声道 bitrate   8K，16K，32K,  64K, 128K                                                                                    
* 双声道 bitrate  16K, 32K, 64K, 128K 
*
* 44.1K采样率                                                                                                                
* 单声道 bitrate  16K，32K, 64K, 128K                                                                                        
* 双声道 bitrate  16K, 32K, 64K, 128K
*     
* 48K 采样率                                                                                                                 
* 单声道 bitrate  16K，32K, 64K, 128K                                                                                        
* 双声道 bitrate  16K, 32K, 64K, 128K 
*    
***********************************************************************************************************************/

/***********************************************************************************************************************
* 功  能：获取编码一帧输入数据大小
*
* 参  数：info_param   -I/O   信息结构指针
*
* 返回值：返回状态码
***********************************************************************************************************************/
HRESULT HIK_AACENC_GetInfoParam(AUDIOENC_INFO *info_param);

/***********************************************************************************************************************
* 功  能：获取所需内存大小
*
* 参  数：enc_param   -I    参数结构指针
*         mem_tab     -I/O  内存申请表
*
* 返回值：返回状态码
***********************************************************************************************************************/
HRESULT HIK_AACENC_GetMemSize(AUDIOENC_PARAM *enc_param, MEM_TAB *mem_tab);

/***********************************************************************************************************************
* 功  能：创建AAC编码模块
*
* 参  数： enc_param       -I   编码参数
*          mem_tab         -I   内存申请表
*          handle          -I/O 句柄        
*
* 返回值：返回状态码
***********************************************************************************************************************/
HRESULT HIK_AACENC_Create(AUDIOENC_PARAM *enc_param, MEM_TAB *mem_tab, void **handle);

/***********************************************************************************************************************
* 功  能：AAC编码模块
*
* 参  数：hEncoder         -I   编码句柄
*         process_param    -I/O 处理参数
*
* 返回值：返回状态码
***********************************************************************************************************************/
HRESULT HIK_AACENC_Encode(void* hEncoder, AUDIOENC_PROCESS_PARAM *process_param);

/***********************************************************************************************************************
* 功  能：获取所需内存大小
*
* 参  数：param       -I    参数结构指针
*         mem_tab     -I/O  内存申请表
*
* 返回值：返回状态码
***********************************************************************************************************************/
HRESULT HIK_AACDEC_GetMemSize(AUDIODEC_PARAM *param, MEM_TAB *mem_tab);

/***********************************************************************************************************************
* 功  能：创建AAC解码模块
*
* 参  数： param           -I   编码参数
*          mem_tab         -I   内存申请表
*          handle          -I/O 句柄        
*
* 返回值：返回状态码
***********************************************************************************************************************/
HRESULT HIK_AACDEC_Create(AUDIODEC_PARAM *param, MEM_TAB *mem_tab, void **handle);

/***********************************************************************************************************************
* 功  能：AAC解码
*
* 参  数：handle           -I   编码句柄
*         process_param    -I/O 处理参数
*
* 返回值：返回状态码
***********************************************************************************************************************/
HRESULT	HIK_AACDEC_Decode(void *handle, AUDIODEC_PROCESS_PARAM *proc_param);

/***********************************************************************************************************************
* 功  能：获取当前编码版本信息
*
* 参  数：无
*
* 返回值：返回版本信息
*
* 备  注：版本信息格式为：版本号＋年（7位）＋月（4位）＋日（5位）
*         其中版本号为：主版本号（6位）＋子版本号（5位）＋修正版本号（5位）
***********************************************************************************************************************/
U32 HIK_AACCODEC_GetVersion();

#ifdef __cplusplus
}
#endif


#endif

