/*
*******************************************************************************
* 
* 版权信息：版权所有 (c) 2012, 杭州海康威视数字技术有限公司, 保留所有权利
* 
* 文件名称：PSMuxLib.h
* 文件标识：HIK_PSMUX_LIB
* 摘    要：海康威视 PSMUX 库的接口函数声明文件
*
* 当前版本：2.00.08
* 作    者：PlaySDK
* 日    期：2015年06月23日
* 备    注：
* 2008 07 04 初始版本
* 2008 09 18 修改 descriptor 为 rtp 和 ps 通用的模式，每次处理一个nal unit
* 2009 01 08 修改 PSH 长度，从16 byte变成20 byte，在末尾 stuffing byte 中添加帧号
* 2009 01 19 根据 dsp 组要求将帧号改成外部传入
* 2009 02 19 根据新解码接口的要求传入一帧最后一个nalu的标志
* 2009 03 23 一个pes包至少填充两个字节，去掉user_data的添加功能
* 2009 08 11 为低延时应用的需求，将送入数据的方式改为可以送进一帧或一个nalu的一段
* 9010 03 25 增加desriptor设置接口和音频帧、B帧前添加PSH的接口
* 2012 08 11 升级版本至2.00.00
* 2013 07 04 升级版本至2.00.01,再打basic私有描述子的时候，外面需要设置公司类型和相机类型
* 2013 11 13 升级版本至2.00.02,统一ts，ps，rtp私有描述子打包文件
* 2014.01.13 添加SVAC码流打包，版本号升级至2.00.03
* 2014.06.23 添加视频描述子中的轻存储标记,版本号升级至2.00.04
* 2014 07 14 升级版本至2.00.05,统一ts，ps，rtp私有描述子打包文件
* 2014 09 26 升级版本至2.00.06,在type_dscrpt_common.h增加H265，修改7226音频宏
* 2015 01 04 升级版本至2.00.07,增加温度私有信息打包
* 2015 06 23 升级版本至2.00.08,增加加密描述子，H.265加密开始使用
*******************************************************************************
*/
#ifndef _HIK_PSMUX_LIB_H_
#define _HIK_PSMUX_LIB_H_

/******************************************************************************
* 作用域控制宏 (必须使用，以便C++语言能调用)
******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
    
#include "descriptor_set.h"
/******************************************************************************
* 宏声明
******************************************************************************/
#define HIK_PSMUX_LIB_VERSION			2.00.08		/* 当前版本号 */
#define HIK_PSMUX_LIB_DATE			    0x20150623  /* 版本日期	  */

/* 状态码：小于零表示有错误(0x80000000开始)，零表示失败，大于零表示成功  */
#define HIK_PSMUX_LIB_S_OK			0x00000001	/* 成功		    */
#define HIK_PSMUX_LIB_S_FAIL		0x00000000	/* 失败		    */
#define HIK_PSMUX_LIB_E_PARA_NULL	0x80000000	/* 参数指针为空 */
#define HIK_PSMUX_LIB_E_MEM_OVER	0x80000001	/* 内存溢出     */
#define HIK_PSMUX_LIB_E_STREAM_TYPE 0x80000003	/* 流类型错误   */


    // 加密算法
#define HIK_PSPACK_SECRET_ARITH_NONE           0x00   // 不加密
#define HIK_PSPACK_SECRET_ARITH_AES            0x01   // AES加密
    // 加密类型
#define HIK_PSPACK_SECRET_TYPE_ZERO            0x00   // 保留
#define HIK_PSPACK_SECRET_TYPE_16B             0x01   // 各帧前16byte加密
#define HIK_PSPACK_SECRET_TYPE_4KB             0x02   // 各帧前4096byte加密
    // 加密轮数
#define HIK_PSPACK_SECRET_ROUND00              0x00   // 保留
#define HIK_PSPACK_SECRET_ROUND03              0x01   // 3轮加密
#define HIK_PSPACK_SECRET_ROUND10              0x02   // 10轮加密
    // 密钥长度
#define HIK_PSPACK_SECRET_KEYLEN000            0x00   // 保留
#define HIK_PSPACK_SECRET_KEYLEN128            0x01   // 128bit
#define HIK_PSPACK_SECRET_KEYLEN192            0x02   // 192bit
#define HIK_PSPACK_SECRET_KEYLEN256            0x03   // 256bit


/******************************************************************************
* 结构体声明
******************************************************************************/
typedef struct _HIK_PSMUX_ES_INFO_
{
    unsigned int	stream_mode;        /* 输入流模式*/
	unsigned int	max_byte_rate;	    /* 码率，以byte为单位*/
    unsigned int    max_packet_len;     /* 最大 pes 包长度 */

    unsigned int	video_stream_type;  /* 输入视频流类型 */
    unsigned int	audio_stream_type;  /* 输入音频流类型 */
	unsigned int	privt_stream_type;	/* 输入私有流类型 */

	unsigned int    dscrpt_sets;       /* 各种descriptor的设置开关，例如要添加video descriptor和
									      audio_descriptor,则置为INCLUDE_VIDEO_DESCRIPTOR | INCLUDE_AUDIO_DESCRIPTOR*/
	unsigned int    bframe_audio_set_psh; /* B帧和音频帧前是否添加PSH, 1为是，0为否 */
	unsigned int    set_frame_end_flg;    /* 是否在pes包头的填充字节里设置帧或nalu结束开始标记 */

    HIK_STREAM_INFO stream_info; 
} HIKPSMX_ES_INFO;

/* 复合器参数 */
typedef struct _PSMUX_PARAM_
{
    unsigned int        buffer_size;
    unsigned char*      buffer;
    HIKPSMX_ES_INFO     info;
} PSMUX_PARAM;

/* 数据块处理参数 */
typedef struct _PSMUX_UNIT_
{
    unsigned int	frame_type;         /* 输入帧类型                           */
    unsigned int    is_first_unit;      /* 是否是一帧的第一个unit。标准H.264每帧会分成多个unit*/
                                        /* 其余编码每帧都只有一个unit */
	unsigned int	is_last_unit;		/* 是否是一帧的最后一个unit		*/
    unsigned int    is_key_frame;       /* 是否关键数据(I帧)                    */
	unsigned int    is_unit_start;      /* 若是一个nalu或一帧的第一段数据，则置1，若送进的是完整的一帧或一个nalu也置1*/
	unsigned int    is_unit_end;        /* 若是一个nalu或一帧的最后一段数据，则置1，若送进的是完整的一帧或一个nalu也置1*/
    unsigned int    sys_clk_ref;        /* 系统参考时钟，以 1/45000 秒为单位    */
    unsigned int    ptime_stamp;        /* 该帧在接收端的显示时标，单位同上     */
	unsigned int	frame_num;			/* 当前帧号		  */
    unsigned char * unit_in_buf;        /* 输入 unit 指针 */
    unsigned int    unit_in_len;        /* 输出 unit 长度 */

	unsigned char *	out_buf;			/* 输出缓冲区           */
	unsigned int	out_buf_len;	    /* 输出缓冲区长度       */
	unsigned int	out_buf_size;	    /* 输出缓冲区大小       */

    //新增加加密参数
    unsigned char   encrypt_round;       /* 填入加密轮数 */
    unsigned char   encrypt_type;        /* 加密类型     */
    unsigned char   encrypt_arith;       /* 加密算法     */
    unsigned char   key_len;             /* 密匙长度     */

	unsigned int    company_mark;       /* 公司描述符*/
	unsigned int    camera_mark;        /* 相机描述  */
    HIK_GLOBAL_TIME global_time;        /* 全局时间  */
} PSMUX_PROCESS_PARAM;

/******************************************************************************
* 接口函数声明
******************************************************************************/

/******************************************************************************
* 功  能：获取所需内存大小
* 参  数：param - 参数结构指针
* 返回值：返回错误码
* 备  注：参数结构中 buffer_size变量用来表示所需内存大小
******************************************************************************/
int PSMUX_GetMemSize(PSMUX_PARAM *param); 

/******************************************************************************
* 功  能：创建PSMUX模块
* 参  数：param  	- 参数结构指针
*         **handle	- 返回PSMUX模块句柄
* 返回值：返回错误码
******************************************************************************/
int PSMUX_Create(PSMUX_PARAM *param, void **handle); 

/******************************************************************************
* 功  能：复合一段数据块
* 参  数：handle - 句柄(handle由PSMUX_Create返回)
* 返回值：返回错误码
******************************************************************************/
int PSMUX_Process(void *handle, PSMUX_PROCESS_PARAM *param);

/******************************************************************************
* 功  能：重置参考数据
* 参  数：handle - 句柄(handle由PSMUX_Create返回)
*         info   - 参考数据句柄
* 返回值：返回错误码
******************************************************************************/
int PSMUX_ResetStreamInfo(void *handle, HIKPSMX_ES_INFO *info);


/******************************************************************************
* 功  能：更新视频的帧率
* 参  数：handle - 句柄fps 帧率
*         info   - 参考数据句柄
* 返回值：返回错误码
******************************************************************************/
int MUX_Ps_update_vfps(void *handle, unsigned int fps);

int MUX_Ps_update_vdp(void *handle, HIKPSMX_ES_INFO *info);



#ifdef __cplusplus
}
#endif 

#endif /* _HIK_PSMUX_LIB_H_ */
