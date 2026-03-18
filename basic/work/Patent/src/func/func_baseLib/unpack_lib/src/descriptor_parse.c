/********************************************************************************
* 
* 版权信息：版权所有 (c) 2008, 杭州海康威视数字技术有限公司, 保留所有权利
* 
* 文件名称：descriptor_parse.c
* 文件标识：HIKDSC
* 摘    要：海康威视私有描述子解析代码
*
* 当前版本：0.3
* 作    者：俞海
* 日    期：2009年2月9日
* 备	注：
* 2008.07.22	初始版本
* 2009.02.09	将原来stream_descriptor修改为basic_descriptor, 
*				另行增加stream_descriptor，主要为rtp应用传输原始流类型和帧号
********************************************************************************/
#include "descriptor_parse.h"
#include "memory.h"

/******************************************************************************
* 功  能：  解析hik_basic_descriptor
* 参  数：  buffer      - 解析缓冲区
*   	    stream_info - 流信息结构体
* 返回值：  解析正常时返回descriptor长度，否则返回错误码
******************************************************************************/
int HIKDSC_parse_hik_basic_descriptor(unsigned char *buffer, HIK_STREAM_INFO *stream_info)
{
    unsigned int len        = buffer[1];
    unsigned int company_mark   = (buffer[2] << 8) + buffer[3];
    unsigned int def_version    = (buffer[4] << 8) + buffer[5];
    HIK_GLOBAL_TIME *time = &stream_info->glb_time;
    if (company_mark != HIKVISION_COMPANY_MARK || ((def_version > HIK_DESCRIPTOR_DEF_VERSION) && (0x100 != def_version)))/*0x100 != def_version 兼容一段描述子里版本号不太正常的码流*/
    {
        stream_info->is_hik_stream = 0;
        return -1;
    }
    else
    {
        stream_info->is_hik_stream = 1;
    }
    //8bit 全局时间年
    time->year    = buffer[6] + DESCRIPTOR_YEAR_BASE;
    //4bit 全局时间月
    time->month	= buffer[7] >> 4;
    //5bit 全局时间日
    time->date	= ((buffer[7] << 1) + (buffer[8] >> 7)) & 0x1f;
    //5bit 全局时间时，24小时制
    time->hour	= (buffer[8] >> 2) & 0x1f;
    //6bit 全局时间分
    time->minute	= ((buffer[8] << 4) + (buffer[9] >> 4)) & 0x3f;
    //6bit 全局时间秒
    time->second	= ((buffer[9] << 2) + (buffer[10] >> 6)) & 0x3f;
    //10bit	全局时间毫秒
    time->msecond	= ((buffer[10] << 5) + (buffer[11] >> 3)) & 0x03ff;
	//3bit 加密类型，111表示未加密	
    stream_info->encrypt_type	= buffer[11] & 0x07;
	//结构版本号
	 stream_info->video_info.def_version = def_version;
    return len + 2;
}

/******************************************************************************
* 功  能：  解析hik_stream_descriptor
* 参  数：  buffer      - 解析缓冲区
*   	    stream_info - 流信息结构体
* 返回值：  解析正常时返回descriptor长度，否则返回错误码
******************************************************************************/
int HIKDSC_parse_hik_stream_descriptor(unsigned char *buffer, HIK_STREAM_INFO *stream_info)
{
    unsigned int len        = buffer[1];
	unsigned int video_stream_type, audio_stream_type;
	unsigned int video_frame_num = 0;

	//获取流类型
	video_stream_type = buffer[2];
	audio_stream_type = buffer[3];

	if (video_stream_type != STREAM_TYPE_UNDEF)
	{
		stream_info->video_info.stream_type = video_stream_type;
		video_frame_num = (buffer[4] << 24)
						| (buffer[5] << 16)
						| (buffer[6] << 8)
						| (buffer[7]);
	}
	if (audio_stream_type != STREAM_TYPE_UNDEF)
	{
		stream_info->audio_info.stream_type = audio_stream_type;
	}

	//获取帧号
	stream_info->video_info.frame_num = video_frame_num;

    return len + 2;
}

/******************************************************************************
* 功  能：  解析hik_device_descriptor
* 参  数：  buffer      - 解析缓冲区
*   	    stream_info - 流信息结构体
* 返回值：  解析正常时返回descriptor长度，否则返回错误码
******************************************************************************/
int HIKDSC_parse_hik_device_descriptor(unsigned char *buffer, HIK_STREAM_INFO *stream_info)
{
    unsigned int len    = buffer[1];
    unsigned int company_mark   = (buffer[2] << 8) + buffer[3];
    if (company_mark != HIKVISION_COMPANY_MARK)
    {
        return len + 2;
    }

    memcpy(stream_info->dev_chan_id, &buffer[4], 16);
	stream_info->dev_chan_id_flg = 1;
    return len + 2;
}

/******************************************************************************
* 功  能：  解析hik_video_descriptor
* 参  数：  buffer      - 解析缓冲区
*   	    video_info  - 流信息结构体
* 返回值：  解析正常时返回descriptor长度，否则返回错误码
******************************************************************************/
int HIKDSC_parse_hik_video_descriptor(unsigned char *buffer, HIK_VIDEO_INFO *video_info)
{
    unsigned int len                = buffer[1];
	unsigned int svc_tag = 0;
    //16bit 编码器版本
    video_info->encoder_version    = (buffer[2] << 8) + buffer[3];
    //7bit 编码器版本时间 年
    video_info->encoder_year       = (buffer[4] >> 1) + DESCRIPTOR_YEAR_BASE;
    //4bit 编码器版本时间 月
    video_info->encoder_month      = (((buffer[4] & 1) << 3) + (buffer[5] >> 5)) & 0x0f;
    //5bit 编码器版本时间 日
    video_info->encoder_date       = buffer[5] & 0x1f;
    //16bit 图像编码宽度，16的倍数，禁止0
    video_info->width_orig       = (buffer[6] << 8) + buffer[7];
    //16bit 图像编码高度，16的倍数，禁止0
    video_info->height_orig      = (buffer[8] << 8) + buffer[9];
    //1bit 是否场编码
    video_info->interlace        = buffer[10] >> 7;
    //2bit b帧个数
    video_info->b_frame_num      = (buffer[10] >> 5) & 0x03;
	//1bit 是否SVC
	svc_tag = (buffer[10] >> 4) & 0x01;
    //1bit 是否有E帧	
    video_info->use_e_frame      = (buffer[10] >> 3) & 0x01;
    //3bit 最大参考帧个数
    video_info->max_ref_num      = buffer[10] & 0x07;
    //3bit 水印类型
    video_info->watermark_type   = buffer[11] >> 5;
	//1bit 显示时是否需要反隔行
	video_info->deinterlace      = (buffer[11] & 0x10 ) >> 4;	
    //23bit	以1/90000秒为单位的两帧间时间间隔，禁止0
    video_info->time_info        = (buffer[13] << 15) + (buffer[14] << 7) + (buffer[15] >> 1);
    //1bit 是否固定帧率
    video_info->fixed_frame_rate	= buffer[15] & 0x01;

	// 根据结构版本判断是否真实svc
	if((video_info->def_version > 0x0001)
	&& (0 == svc_tag))
	{
		video_info->is_svc_stream = 0;  // 0表示为svc码流，1不是svc码流  
	}
	else
	{
		video_info->is_svc_stream = 1;  // 为了不和初始化0是起冲突，增加非svc赋值
	}

    return len + 2;
}

/******************************************************************************
* 功  能：  解析hik_audio_descriptor
* 参  数：  buffer      - 解析缓冲区
*   	    audio_info  - 流信息结构体
* 返回值：  解析正常时返回descriptor长度，否则返回错误码
******************************************************************************/
int HIKDSC_parse_hik_audio_descriptor(unsigned char *buffer, HIK_AUDIO_INFO *audio_info)
{
    unsigned int len    = buffer[1];
    //16bit 音频帧长度，禁止0值
    audio_info->frame_len = (buffer[2] << 8) + buffer[3];
    //1bit 声道数，0单声道，1双声道
    audio_info->audio_num = buffer[4] & 0x01;
	//22bit 音频采样率，单位Hz
    audio_info->sample_rate  = (buffer[5] << 14) + (buffer[6] << 6) + (buffer[7] >> 2);
    //22bit 码率，单位bps
    audio_info->bit_rate     = (buffer[8] << 14) + (buffer[9] << 6) + (buffer[10] >> 2);				
    return len + 2;
}

/******************************************************************************
* 功  能：  解析hik_video_clip_descriptor
* 参  数：  buffer      - 解析缓冲区
*   	    video_info  - 流信息结构体
* 返回值：  解析正常时返回descriptor长度，否则返回错误码
******************************************************************************/
int HIKDSC_parse_hik_video_clip_descriptor(unsigned char *buffer, HIK_VIDEO_INFO *video_info)
{
    unsigned int len = buffer[1];
    video_info->play_clip = 1;
    //16bit 显示区域左上角座标x
    video_info->start_pos_x = (buffer[2] << 8) + buffer[3];
    //15bit 显示区域左上角座标y
    video_info->start_pos_y = ((buffer[4] & 0x7f) << 7) + (buffer[5] >> 1);
    //16bit 显示宽度，禁止0值
    video_info->width_play   = (buffer[6] << 8) + buffer[7];
    //16bit 显示高度，禁止0值
    video_info->height_play  = (buffer[8] << 8) + buffer[9];
    return len + 2;
}

/******************************************************************************
* 功  能：  解析描述子区域（该区域可能包含多个描述子）
* 参  数：  buffer				- 解析缓冲区
*           len					- 解析缓冲区的长度
*			parse_stream_dsc	- 是否解析stream_descriptor, 一般ps流不解析，rtp流需要解析
*   	    stream_info			- 流信息结构体
* 返回值：  解析正常时返回descriptor area长度，否则返回错误码
******************************************************************************/
int HIKDSC_parse_descriptor_area(unsigned char *buffer, 
								 unsigned int len, 
								 unsigned int parse_stream_desc, 
								 HIK_STREAM_INFO *stream_info)
{
    unsigned int proc_len = 0;
    int ret;
    while (proc_len < len)
    {
        switch(buffer[proc_len])
        {
        case HIK_BASIC_DESCRIPTOR_TAG:
            ret = HIKDSC_parse_hik_basic_descriptor(&buffer[proc_len], stream_info);
            break;
		case HIK_STREAM_DESCRIPTOR_TAG:
			if (parse_stream_desc)
			{
				ret = HIKDSC_parse_hik_stream_descriptor(&buffer[proc_len], stream_info);
			}
			else
			{
				ret = buffer[proc_len + 1] + 2;//跳过不解析
			}
            break;
        case HIK_DEVICE_DESCRIPTOR_TAG:
            ret = HIKDSC_parse_hik_device_descriptor(&buffer[proc_len], stream_info);
            break;
        case HIK_VIDEO_DESCRIPTOR_TAG:
            ret = HIKDSC_parse_hik_video_descriptor(&buffer[proc_len], &stream_info->video_info);
            break;
        case HIK_AUDIO_DESCRIPTOR_TAG:
            ret = HIKDSC_parse_hik_audio_descriptor(&buffer[proc_len], &stream_info->audio_info);
            break;
        case HIK_VIDEO_CLIP_DESCRIPTOR_TAG:
            ret = HIKDSC_parse_hik_video_clip_descriptor(&buffer[proc_len], &stream_info->video_info);
            break;
        default:
            ret = buffer[proc_len + 1] + 2;
            break;
        }
        if (ret < 0)
        {
			break;
            //return HIKDSC_E_STM_ERR;
        }
        else
        {
            proc_len += ret;
        }
    }
    return proc_len;
}