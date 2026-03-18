/********************************************************************************
* 
* 版权信息：版权所有 (c) 200X, 杭州海康威视数字技术有限公司, 保留所有权利
* 
* 文件名称：PSDemuxLib.h
* 文件标识：HIK_PSDEMUX_LIB
* 摘    要：海康威视PSDEMUX 库的接口函数声明文件
*
* 当前版本：0.98
* 作    者：俞海 黄崇基 辛安民
* 日    期：2015年3月13日
* 备    注：
* 2008 07 06	初始版本
* 2008 07 29	修改 descriptor 为 rtp 和 ps 通用的模式，支持非海康规范PS流，
*				但输出数据缺少部分信息，并且每次输出不一定是完整的帧
* 2009 01 08	修改PSH解析，加入帧号获取
* 2009 02 09	增加接口设置输出缓冲区位置，减少外部拷贝次数，同时
*				由于修改了type_dscrpt_common.h，需要随之修改
* 2009 03 04    不再将ps数据拷贝到内部缓冲区，而是直接在外部提供的缓冲区中解析，每次
*               解析完告诉库外部还剩多长的数据没解析
* 2009 03 07    如果发现包中出现同步字节错误，改重新搜索起始码为重新搜索PSH，使解析输出
*               的为整帧的数据
* 2009 05 22    添加私有数据privt_info_type，privt_info_sub_type，privt_info_frame_num三个接口
*               库外部通过这三个接口就可以获知私有数据(如智能信息)的主类型，子类型，和起作用的帧号
* 2009 06 25    将输出数据方式改为每解析完一个pes包就输出数据，去掉函数PSDEMUX_SetOutBuf
* 2009 07 03    增加一帧开始的接口is_frame_start
* 2009 08 06    增加一个nalu结束的接口is_nalu_end
* 2009 12 07    增加一些新的音频类型
* 2015 03 13    增加对PS over RTP的解析
* 2015 03 13    增加对PS over RTP的解析，在PSDEMUX_PARAM增加一个ps_type区分是普通PS还是PS over RTP
********************************************************************************/
#ifndef _HIK_PSDEMUX_LIB_H_
#define _HIK_PSDEMUX_LIB_H_

/******************************************************************************
* 作用域控制宏 (必须使用，以便C++语言能调用)
******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include "descriptor_parse.h"
/******************************************************************************
* 宏声明
******************************************************************************/
#define HIK_PSDEMUX_LIB_VERSION		0.97	    /* 当前版本号	*/
#define HIK_PSDEMUX_LIB_DATE		0x20091207	/* 版本日期	    */

/* 状态码：小于零表示有错误(0x80000000开始)，零表示失败，大于零表示成功  */
#define HIK_PSDEMUX_LIB_S_OK			0x00000001  /* 成功					    */
#define HIK_PSDEMUX_LIB_S_FAIL			0x00000000  /* 失败					    */
#define HIK_PSDEMUX_LIB_E_PARA_NULL		0x80000000	/* 参数指针为空			    */
#define HIK_PSDEMUX_LIB_E_SYNC_LOST		0x80000001	/* 同步丢失，数据出错       */
#define HIK_PSDEMUX_LIB_E_MEM_OVER		0x80000002	/* 内存溢出				    */
#define HIK_PSDEMUX_LIB_E_STM_ERR		0x80000003	/* 不支持的流   		    */

/******************************************************************************
* 结构体声明
******************************************************************************/
#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef int HRESULT;
#endif // !_HRESULT_DEFINED

typedef struct _HIK_PSDEMUX_DATA_OUTPUT 
{
    /* !!!!!重要!!!!! */
    /* 注：当该结构内 info.stream_info.is_hik_stream = 0 时，说明该PS流不符合海康PS流定义 */
    /* 每次输出的数据是正确的原始流，但不一定是完整的帧或者单元，需要在外部缓冲区进行重组 */
    /* 同时本结构体包含的 output_type 和 info.stream_info 内的其他信息无效                */

    unsigned int    output_type;        /* 输出数据帧类型 */
    unsigned int    privt_info_type;    /* 私有流类型，如智能信息、CODEC信息 */
    unsigned int    privt_info_sub_type; /* 私有流子类型，如MetaData，EventData */
    unsigned int    privt_info_frame_num; /* 私有数据作用的帧的帧号 */
    unsigned int    channel_num;        /* 输出流所在的通道号 */
    unsigned int    sys_clk_ref;        /* 当前参考时钟 */
    unsigned int    encrypt;            /* 是否加密         */
    unsigned int    data_len;           /* 输出数据长度 */
    unsigned int    data_time_stamp;    /* 输出数据时标 */
    unsigned char * data_buffer;        /* 输出数据缓冲区 */
    unsigned int    is_frame_start;     /* 是否为一帧的开始 */
    unsigned int    is_frame_end;       /* 是否为一帧的结束 */
    unsigned int    is_nalu_end;        /* 是否为nalu的结束，只有输出H264视频帧才有效 */
    unsigned  int   is_end_group;
    unsigned int    have_userdata;      /* 是否包含用户信息 */
    unsigned char * userdata;           /* 用户信息指针     */
    unsigned int    stream_mode;		/* 流模式		*/
    unsigned int    search_sps;
	unsigned int    vpsLen;
    unsigned int    spsLen;
    unsigned int    ppsLen;
    HIK_STREAM_INFO *info;             /* 流信息指针 */
} PSDEMUX_DATA_OUTPUT;

/* 分离器参数 */
typedef struct _PSDEMUX_PARAM_
{
    unsigned int    ps_type;                /* 0 标示海康PS, 1标示 ps over rtp*/    
    unsigned int    channel_num;            /* 帧所在的通道号                           */
    void          * ptr_callback;           /* 保留给回调函数的指针                     */
    unsigned int    max_frame_size;         /* 最大帧长度                               */
    void          * output_callback_func;   /* 输出回调函数指针                         */
    unsigned char * init_out_buffer;		/* 初始时码流输出缓冲区，可调用PSDEMUX_SetOutBuf更改 */
    unsigned int    buf_size;		        /* 内部缓冲区内存大小						*/
    unsigned char * buffer;                 /* 内部缓冲区内存指针，内存由用户申请、释放 */
    unsigned char * ringBuf;                /* PS解析环形缓冲*/
    unsigned char   ringBufLen;             /* PS解析环形缓冲大小*/

} PSDEMUX_PARAM;

typedef struct _PSDEMUX_OUT_PARAM_
{
    unsigned int      bufLen;
    unsigned int      lenPerFrm;
    unsigned int      frmCnt;
    unsigned int      vOut;
    unsigned int      aOut;
    unsigned char*    bufAddr;
} PSDEMUX_OUT_PARAM;

/* 处理过程参数 */
typedef struct _PSDEMUX_PROCESS_PARAM_
{
    unsigned int      orgWIdx;
    unsigned int      orgRIdx;
    unsigned int      payload;         //0表示视频，1表示音频,2表示智能帧
    unsigned char*    outbuf;
    unsigned int      outbuflen;
    unsigned int      outdatalen;

    unsigned int      stamp;
    unsigned int      frmType;    
    unsigned int      frmInBuf;
    unsigned int      width;
    unsigned int      height;
	unsigned int      vpsLen;
    unsigned int      spsLen;
    unsigned int      ppsLen;
	unsigned int      frameNum;
	unsigned int      streamType;
	unsigned int      time_info;             /* 两帧间时间间隔 */
	HIK_GLOBAL_TIME   glb_time;              /* 全局时间                     */
    unsigned int    privtType;    
    unsigned int    privtSubType; 
} PSDEMUX_PROCESS_PARAM;

/******************************************************************************
* 功  能：获取PSDemux所需内存大小
* 参  数：param - 参数结构指针
* 返回值：返回状态码
******************************************************************************/
HRESULT PSDEMUX_SetRingBuf(unsigned char* bufaddr,unsigned int bufLen,void *handle);

/******************************************************************************
* 接口函数声明
******************************************************************************/
/******************************************************************************
* 功  能：获取PSDemux所需内存大小
* 参  数：param - 参数结构指针
* 返回值：返回状态码
******************************************************************************/
HRESULT PSDEMUX_GetMemSize(PSDEMUX_PARAM *param); 

/******************************************************************************
* 功  能：创建PSDemux模块
* 参  数：param  	- 初始控制参数结构指针
*         handle	- 内部参数模块句柄
* 返回值：返回状态码
******************************************************************************/
HRESULT PSDEMUX_Create(PSDEMUX_PARAM *param, void **handle); 

/******************************************************************************
* 功  能：解析一段数据块
* 参  数：param  - 处理所需要的参数结构体
		  handle - 输入参数指针
* 返回值：返回错误码，函数返回后通过param的成员unprocessed_len告诉库外部还有多长
*         的数据没有解析，库外部下次要在未解析的数据后面接一段数据，下次送进库内
*         进行解析
******************************************************************************/
HRESULT PSDEMUX_Process(PSDEMUX_PROCESS_PARAM * param, void *handle);

/******************************************************************************
* 功  能：设置输出缓冲区
* 参  数：	buffer - 输出缓冲区

*			handle - 输入参数指针
* 返回值：返回错误码
******************************************************************************/
HRESULT PSDEMUX_SetOutBuf(PSDEMUX_OUT_PARAM *outPrm,void *handle);

HRESULT PSDEMUX_FullBuf(PSDEMUX_PROCESS_PARAM * param,void *handle);

HRESULT PSDEMUX_Reset(void *handle);


#ifdef __cplusplus
}
#endif 

#endif /* _HIK_PSDEMUX_LIB_H_ */
