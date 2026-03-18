/******************************************************************************
**  版权信息：     版权所有 (c) 2007, 杭州海康威视数字技术有限公司, 保留所有权利
**
**  文件名称：     G711codec.h
**
**  文件描述:      G711音频编解码库接口函数声明文件
**
**  当前版本：     2.1
**  作    者:      黄添喜
**  日    期:      2007年12月21号
**
**  修改历史:
*********************************************************************************/

#ifndef _HIK_G711_CODEC_LIB_H_
#define _HIK_G711_CODEC_LIB_H_

/******************************************************************************
* 作用域控制宏 (必须使用，以便C++语言能调用)
******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
* 函数名  : G711ENC_Encode()
*
* 功  能  : 函数调用一次,编码sample_num个样点
*
* 输入参数:
*            type             -      指定A law 编码,type取值为非零,指定 Mu law 编码,type取值为 0
*
*		     sample_num	      -      指定每次处理的样点数,由用户指定
*
*            samples          -      输入缓存区指针,存放PCM样点,大小不小于sample_num*(sizeof(short)) byte
*
* 输出参数:
*
*            bitstream        -     输出缓存区指针,存放编码后的码流,不小于sample_num byte
*
* 返回值  : 状态码
******************************************************************************/
void G711ENC_Encode(unsigned int type, unsigned int sample_num, unsigned char *samples, unsigned char *bitstream);

/******************************************************************************
* 函数名  : G711ENC_Encode_16K()
*
* 功  能  : 函数调用一次,编码sample_num个样点
*
* 输入参数:
*            type             -      指定A law 编码,type取值为非零,指定 Mu law 编码,type取值为 0
*
*		     sample_num	      -      指定每次处理的样点数,由用户指定
*
*            samples          -      输入缓存区指针,存放PCM样点,大小不小于sample_num*(sizeof(short)) byte
*
* 输出参数:
*
*            bitstream        -     输出缓存区指针,存放编码后的码流,不小于sample_num byte
*
* 返回值  : 状态码
******************************************************************************/
void G711ENC_Encode_16K(unsigned int type, unsigned int sample_num, unsigned char *samples, unsigned char *bitstream);


/******************************************************************************
* 函数名  : G711DEC_Decode()
*
* 功  能  : 函数调用一次,解码出sample_num个PCM样点
*
* 输入参数:
*             type            -     指定A law 编码,type取值为非零,指定 Mu law 编码,type取值为 0
*
*		      sample_num	  -     指定每次处理的样点数,由用户指定
*
*             bitstream       -     输入缓存区指针,待解码码流,大小不小于 sample_num byte
*
* 输出参数:
*
*             samples         -     输出缓存区指针,存放解码后的PCM数据,大小不小于sample_num*(sizeof(short)) byte
*
* 返回值  : 状态码
******************************************************************************/
void G711DEC_Decode(unsigned int type, unsigned int sample_num, unsigned char *samples, unsigned char *bitstream);

#ifdef __cplusplus
}
#endif


#endif

