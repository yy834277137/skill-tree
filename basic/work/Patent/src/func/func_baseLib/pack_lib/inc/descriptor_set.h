/*
*******************************************************************************
* 
* 版权信息：版权所有 (c) 2008, 杭州海康威视数字技术有限公司, 保留所有权利
* 
* 文件名称：descriptor_set.h
* 文件标识：HKDSC
* 摘    要：海康威视生成通用描述子的代码
*
* 当前版本：0.5
* 作    者  辛安民
* 日    期：2015年06月23日
*
* 当前版本：0.4
* 作    者：PlaySDK
* 日    期：2013年11月13日
* 备	注：
* 2008.07.22	初始版本
* 2009.02.09	将原来stream_descriptor修改为basic_descriptor, 
*				另行增加stream_descriptor，主要为rtp应用传输原始流类型和帧号
* 2010.03.25    增加timing_hrd_desriptor的接口
* 2015.06.23    增加encrypt_descriptor的接口，从H.265加密开始引入这个描述子
********************************************************************************/
#ifndef _HIK_DESCRIPTOR_SET_H_
#define _HIK_DESCRIPTOR_SET_H_

#include "type_dscrpt_common.h"

unsigned int HKDSC_fill_basic_descriptor(unsigned char *buffer, 
										 HIK_GLOBAL_TIME *glb_time, 
										 unsigned int encrypt_type,
										 unsigned int company_mark,
										 unsigned int camera_mark);
unsigned int HKDSC_fill_stream_descriptor(unsigned char *buffer, 
										  unsigned int video_stream_type,
										  unsigned int audio_stream_type,
										  unsigned int video_frame_num);
unsigned int HKDSC_fill_video_descriptor(unsigned char *buffer, HIK_VIDEO_INFO *video_info);
unsigned int HKDSC_fill_audio_descriptor(unsigned char *buffer, HIK_AUDIO_INFO *audio_info);
unsigned int HKDSC_fill_device_descriptor(unsigned char *buffer, unsigned char dev_chan_id[16]);
unsigned int HKDSC_fill_video_clip_descriptor(unsigned char *buffer, HIK_VIDEO_INFO *video_info);
unsigned int HKDSC_fill_timing_hrd_descriptor(unsigned char *buffer, int frame_rate, int width, int height);
unsigned int HKDSC_fill_encrypt_descriptor(unsigned char *buffer,
                                           unsigned char encrypt_round,
                                           unsigned char encrypt_type,
                                           unsigned char encrypt_arith,
                                           unsigned char key_len);
void HKDSC_fill_timing_fps_descriptor(unsigned char *buffer, unsigned int frame_rate);

#endif