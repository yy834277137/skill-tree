/**@file RtpUnpackLib.c
*  @note Hangzhou Hikvision Digital Technology Co., Ltd. All Rights Reserved.
*  @brief 海康威视 RTPUNPACK 库的接口函数实现文件
*
*  @author   辛安民
*  @date     2014/10/24
*
*  @note V1.1.5 按照代码规范修改代码
*
*  @author    辛安民
*  @date      2014/10/13
*
*  @note V1.1.4 增加对H.265的解析支持
*
*  @author    许江浩
*  @date      2014/7/8
*
*  @note V1.1.3 增加SVC码流的判断
*
*  @author    洪瑜
*  @date      2010/3/19
*
*  @note V1.0.5 增加对负载H.264时没有去除起始码的异常码流的兼容
*
*  @author    洪瑜
*  @date      2010/2/23
*
*  @note V1.0.4 添加对负载mjpeg的rtp包解包的支持，输出图像添加了jpeg文件头
*
*  @author    俞海 黄崇基
*  @date      2009/6/8
*
*  @note V1.0.3 添加私有数据解包接口
*
*  @author    俞海 黄崇基
*  @date      2009/2/10
*
*  @note V1.0.2 根据重新定义的私有数据格式进行解包
*
*  @author    俞海
*  @date      2008/7/24
*
*  @note V1.0.1 rtp解封装初版
*
*  @warning 无
*/

#include <stdio.h>
#include <memory.h>
#include "JpegMarker.h"
//#include "RtpUnpackLib.h"
#include "descriptor_parse.h"
#include "RtpUnpackDefine.h"
#include "sal_trace.h"
#ifdef SRTP_SUPPORT
#include "hik_srtp.h"
#endif

#define  MJPEG_VIDEO_PT      0x1a   ///< motion jpeg
#define  SVAC_VIDEO_PT       0x63   ///<svac码流
#define  FENGHUO_VIDEO_PT    0x23   ///< 烽火RTP
#define  SONY_VIDEO_PT       0x69   ///< 索尼视频
#define  MPEG2_VIDEO_PT      0x20   ///< mpeg2视频
#define  HIK_VIDEO_264_PT    0X71   ///< 海康264

static int unpackDbLevel = DEBUG_LEVEL0;


/**  @struct RTP_UNPACK_PRG
*  @brief 数据块处理参数结构
*/
typedef struct _HIK_RTP_UNPACK_PROGRAM_
{
  unsigned int       frame_len;  //输出帧长度
  unsigned int       unit_end;   //如果解析到h.264的nalu结尾或其他格式的帧末尾则置1
  unsigned int       stream_type;//流类型
  HIK_STREAM_INFO    stream_info;//流信息结构
  JPEG_MARKER_PARAM  jpg_param;  //jpeg参数结构

  #ifdef SRTP_SUPPORT
  SRTP_INFO       srtp_info[RTP_MAX_STREAMNUM]; //srtp信息结构体数组
  #endif
} RTP_UNPACK_PRG;

#ifdef SRTP_SUPPORT
static HRESULT RtpUnpack_SrtpUnPack(RTPUNPACK_PRC_PARAM *param, RTP_UNPACK_PRG *prg); //仅作函数声明
#endif

static int RtpUnpack_StreamType(unsigned char *inBuf)
{
    if (inBuf[0] == 0x0 && inBuf[1] == 0x0 && inBuf[2] == 0x1)
    {
        printf("GetStreamType:STREAM_TYPE_VIDEO_MPEG4\n");
        return STREAM_TYPE_VIDEO_MPEG4;
    }
    else if (inBuf[0] == 0x0 && inBuf[1] == 0x0 && inBuf[2] == 0x0 && inBuf[3] == 0x0)
    {
        printf("GetStreamType:STREAM_TYPE_VIDEO_MJPG\n");
        return STREAM_TYPE_VIDEO_MJPG;
    }
    else if ((inBuf[0] & 0x1f) <= 24 || (inBuf[0] & 0x1f) == 28)
    {
		printf("RTP GetStreamType: %x %x %x %x %x %x %x %x %x %x %x %x \n",
               inBuf[0], inBuf[1], inBuf[2], inBuf[3],
               inBuf[4], inBuf[5], inBuf[6], inBuf[7],
               inBuf[8], inBuf[9], inBuf[10], inBuf[11]);
		if (0x40 == (inBuf[0]) || 0x02 == (inBuf[0]))
		{
		    return STREAM_TYPE_VIDEO_H265;
		}
		printf("GetStreamType:STREAM_TYPE_VIDEO_H264,0x%x\n",(inBuf[0] & 0x1f));
        return STREAM_TYPE_VIDEO_H264;
    }
    else
    {
        printf("RTP GetStreamType: %x %x %x %x %x %x %x %x %x %x %x %x \n",
               inBuf[0], inBuf[1], inBuf[2], inBuf[3],
               inBuf[4], inBuf[5], inBuf[6], inBuf[7],
               inBuf[8], inBuf[9], inBuf[10], inBuf[11]);
        return STREAM_TYPE_UNDEF;
    }
}

/** @fn  HRESULT RTP_UNPACK_output_frame(RTPUNPACK_PRC_PARAM *prc, RTP_UNPACK_PRG *prg)
*  @brief  <输出 rtp 解析获得的帧数据>
*  @param prc [IO]输入输出数据结构体.
*  @param prg [IN]内部参数结构体
*  @return  返回错误码
*/
HRESULT RTP_UNPACK_output_frame(RTPUNPACK_PRC_PARAM *prc, RTP_UNPACK_PRG *prg)
{
  if (prg->stream_info.is_hik_stream)
  {
      prc->stream_info  = &prg->stream_info;
  }
  else
  {
      prc->stream_info  = NULL;
  }
  prc->outbuf_len = prg->frame_len;
  prc->info_valid = prg->stream_info.is_hik_stream;
  prg->frame_len  = 0;
  prg->unit_end  = 0;
  return HIK_RTP_UNPACK_LIB_S_OK;
}

/**@fn  HRESULT RTP_UNPACK_parse_privt_info(unsigned char *srcbuf,  unsigned int ext_len,
                                            RTP_UNPACK_PRG *prg, RTPUNPACK_PRC_PARAM *prc)
*  @brief  <解析 rtp 头扩展字段>
*  @param srcbuf   [IN]rtp扩展部分缓冲区.
*  @param ext_len  [IN] rtp扩展部分长度
*  @param prc      [OUT]输入输出数据结构体.
*  @param prg      [IN] 内部参数结构体
*  @return  返回错误码
*/
HRESULT RTP_UNPACK_parse_privt_info(unsigned char *srcbuf, unsigned int ext_len,
                                    RTP_UNPACK_PRG *prg, RTPUNPACK_PRC_PARAM *prc)
{
  unsigned int    length;
  unsigned int    privt_type;
  unsigned char * dstbuf;

  privt_type = (srcbuf[0] << 8) + srcbuf[1];    //2个字节标示私有帧类型
  length = ((srcbuf[2] << 8) + srcbuf[3]) << 2; //4字节为单位

  // 私有信息是基本流信息，水印信息，加密信息FIXME!
  if ((privt_type == HIK_PRIVT_BASIC_STREAM_INFO) || (privt_type == HIK_PRIVT_CODEC_INFO))
  {
    if (HIKDSC_parse_descriptor_area(&srcbuf[4], length, 1, &prg->stream_info) < 0)
    {
      return HIK_RTP_UNPACK_LIB_E_STM_ERR;
    }
    if (prg->stream_type == STREAM_TYPE_UNDEF)
    {
      prg->stream_type = prg->stream_info.video_info.stream_type;
    }

    //return HIK_RTP_UNPACK_LIB_S_FAIL;
    return HIK_RTP_UNPACK_LIB_S_OK;
  }
  else
  {
    dstbuf = &prc->outbuf[prg->frame_len];

    if (prg->frame_len + length > prc->outbuf_size)
    {
      return HIK_RTP_UNPACK_LIB_E_MEM_OVER;
    }
    prc->is_privt_info = 1;

    //组帧开始需要记录私有帧的信息
    if (prg->frame_len == 0)
    {
      prc->privt_info_type = privt_type;

      prc->privt_info_subtype = (srcbuf[4] << 8) + srcbuf[5];
      prc->privt_info_frame_num = (srcbuf[7] << 24) + (srcbuf[8] << 16)
                    + (srcbuf[10] << 8) + srcbuf[11];
	  if (ext_len < 12)
	  {
	     return HIK_RTP_UNPACK_LIB_S_FAIL;
	  }
      memcpy(dstbuf, srcbuf + 12, ext_len - 12);
      prg->frame_len += ext_len - 12;
    }
    else
    {
      memcpy(dstbuf, srcbuf, ext_len);
      prg->frame_len += ext_len;
    }

    return HIK_RTP_UNPACK_LIB_S_OK;
  }
}

/**@fn  unsigned int add_avc_annexb_startcode(unsigned char *buffer)
*  @brief  <添加 H264 起始码>
*  @param buffer  [IN] 添加缓冲区
*  @return  返回起始码长度
*/
unsigned int add_avc_annexb_startcode(unsigned char *buffer)
{
  *buffer++ = 0x00;
  *buffer++ = 0x00;
  *buffer++ = 0x00;
  *buffer++ = 0x01;
  return 4;
}

/**  @fn  HRESULT RTP_UNPACK_process_stream(unsigned char *  srcbuf,
                                            unsigned int  length,
                                            RTPUNPACK_PRC_PARAM *prc,
                                            RTP_UNPACK_PRG *prg)
*  @brief  <解析除 H.264 以外的其他 rtp 数据包>
*  @param srcbuf   [IN]缓冲区.
*  @param length   [IN] 缓冲区长度
*  @param prc      [OUT]输入输出数据结构体.
*  @param prg      [IN] 内部参数结构体
*  @return  返回错误码
*/
HRESULT RTP_UNPACK_process_stream(unsigned char *  srcbuf,
                                  unsigned int  length,
                                  RTPUNPACK_PRC_PARAM *prc,
                                  RTP_UNPACK_PRG *prg)
{
  unsigned char * dstbuf;

  dstbuf  = &prc->outbuf[prg->frame_len];

  if (prg->frame_len + length > prc->outbuf_size)
  {
    return HIK_RTP_UNPACK_LIB_E_MEM_OVER;
  }

  memcpy(dstbuf, srcbuf, length);
  prg->frame_len += length;
  return HIK_RTP_UNPACK_LIB_S_OK;
}

/**  @fn  HRESULT RTP_UNPACK_process_h264(unsigned char *  srcbuf,
                                          unsigned int  length,
                                          RTPUNPACK_PRC_PARAM *prc,
                                          RTP_UNPACK_PRG *prg)
*  @brief  <解析 H.264 的 rtp 数据包>
*  @param srcbuf   [IN]缓冲区.
*  @param length   [IN] 缓冲区长度
*  @param prc      [OUT]输入输出数据结构体.
*  @param prg      [IN] 内部参数结构体
*  @return  返回错误码
*/
HRESULT RTP_UNPACK_process_h264(unsigned char *  srcbuf,
                                unsigned int  length,
                                RTPUNPACK_PRC_PARAM *prc,
                                RTP_UNPACK_PRG *prg)
{
  unsigned char * dstbuf = &prc->outbuf[prg->frame_len];
  unsigned int  pack_type, len_tmp;

  pack_type = srcbuf[0] & 0x1f;

  if (pack_type == RTP_DEF_H264_FU_A) // FRAGMENT UNIT(一个 NALU 分成多个 rtp 包)的传输形式
  {
    if ((srcbuf[1] & 0x80) == 0x80)//start of fu_a
    {
      if (((srcbuf[0] & 0xe0) | (srcbuf[1] & 0x1f)) == 0x00)
      {
        // 可能是异常的打包方式：打包时没有去掉起始码

        if (srcbuf[2] == 0x00 && srcbuf[3] == 0x00 && srcbuf[4] == 0x01)
        {
          len_tmp = 0;
        }
        else
        {
          return HIK_RTP_UNPACK_LIB_E_STM_ERR;
        }
      }
      else
      {
        len_tmp = add_avc_annexb_startcode(dstbuf);
      }

      dstbuf[len_tmp] = (srcbuf[0] & 0xe0) | (srcbuf[1] & 0x1f);
      prg->frame_len += len_tmp + 1;
      dstbuf = &dstbuf[len_tmp + 1];
    }

    /* memcpy保护: 防止length太大或者length太小(即(length -2)变成负值)导致越界*/
    if ((prg->frame_len + length > prc->outbuf_size) || (length <= 2))
    {
      printf("err: length %d is illegal \n", length);
      return HIK_RTP_UNPACK_LIB_E_MEM_OVER;
    }

    memcpy(dstbuf, &srcbuf[2], length - 2);
    prg->frame_len += (length-2);

    /*if ((srcbuf[1] & 0x40) != 0x40)//end of fu_a
    {
      prc->outbuf_len = 0;
      printf("end of fu_a\n");
      return HIK_RTP_UNPACK_LIB_S_FAIL;
    }

    prg->unit_end = 1;
    */
    if((srcbuf[1] & 0x40) == 0x40)
    {
        prg->unit_end = 1;
    }
  }
  else if (pack_type >= 1 && pack_type <= 23)  // SINGLE NALU 的传输形式
  {
    if (length + 4 > prc->outbuf_size)
    {
      return HIK_RTP_UNPACK_LIB_E_MEM_OVER;
    }

    len_tmp = add_avc_annexb_startcode(dstbuf);
    prg->frame_len += len_tmp;
    dstbuf = &dstbuf[len_tmp];

    memcpy(dstbuf, srcbuf, length);
    prg->frame_len += length;
    prg->unit_end = 1;
  }
  else if (pack_type == 0x00)
  {
    //可能是异常的打包方式：打包时没有去掉起始码

    if (srcbuf[0] == 0x00 && srcbuf[1] == 0x00 && srcbuf[2] == 0x00 && srcbuf[3] == 0x01)
    {
      if (length > prc->outbuf_size)
      {
        return HIK_RTP_UNPACK_LIB_E_MEM_OVER;
      }

      memcpy(dstbuf, srcbuf, length);
      prg->frame_len += length;
      prg->unit_end = 1;
    }
    else
    {
      return HIK_RTP_UNPACK_LIB_E_STM_ERR;
    }
  }
  else
  {
    return HIK_RTP_UNPACK_LIB_E_STM_ERR;
  }

  return HIK_RTP_UNPACK_LIB_S_OK;
}

/**  @fn  HRESULT RTP_UNPACK_process_h265(unsigned char *  srcbuf,
                                          unsigned int  length,
                                          RTPUNPACK_PRC_PARAM *prc,
                                          RTP_UNPACK_PRG *prg)
*  @brief  <解析 H.265 的 rtp 数据包>
*  @param srcbuf   [IN]缓冲区.
*  @param length   [IN]缓冲区长度
*  @param prc      [OUT]输入输出数据结构体.
*  @param prg      [IN]内部参数结构体
*  @return  返回错误码
*/
HRESULT RTP_UNPACK_process_h265(unsigned char *  srcbuf,
                                unsigned int  length,
                                RTPUNPACK_PRC_PARAM *prc,
                                RTP_UNPACK_PRG *prg)
 #if 1
{
	unsigned char *dstbuf = NULL;
	unsigned int len_tmp;
	int pack_type = 0;
	int protecLen = 0;

	/*参数有效性判断*/
	if ((srcbuf == NULL) || (prc == NULL) || (prg == NULL) || (length == 0))
	{
		return HIK_RTP_UNPACK_LIB_E_PARA_ERR;
	}

	dstbuf = prc->outbuf;
	if (dstbuf == NULL)
	{
		return HIK_RTP_UNPACK_LIB_E_PARA_ERR;
	}

	pack_type = (srcbuf[0] & 0x7f) >> 1;
	/* PRT("pack_type 0x%x srcbuf[0] 0x%x\n",pack_type,srcbuf[0]); */
	if (pack_type == 48 || pack_type == 50) /* 两种PayloadHdr类型不支持 */
	{
		return HIK_RTP_UNPACK_LIB_E_STM_ERR;
	}
	else if (pack_type == 49)
	{
		if ((srcbuf[2] & 0x80)) /* start of fu_a */
		{
			len_tmp = add_avc_annexb_startcode(dstbuf);          /* 添加起始码，复用H.264起始码 */
			prg->frame_len = len_tmp;
			dstbuf[prg->frame_len++] = (srcbuf[0] & 0x81) | ((srcbuf[2] & 0x3f) << 1); /* 第0个字节和第2个字节合成起始码后的第一个字节 */
			dstbuf[prg->frame_len++] = srcbuf[1]; /* 直接使用第一个字节 */
			/* dstbuf = &dstbuf[len_tmp + 2]; */
			/* prg->frame_len = 6; //帧长度增加6 */
			/* PRT("start of fu_a 0x%x 0x%x srcbuf[0-2] 0x%x 0x%x 0x%x\n",dstbuf[4],dstbuf[5],srcbuf[0],srcbuf[1],srcbuf[2]); */
		}
		else
		{
			/*PRT("----------------------------len %d data 0x%x 0x%x 0x%x 0x%x frame_len %d mk %d \n",
			    length,srcbuf[0],srcbuf[1],srcbuf[2],srcbuf[3],prg->frame_len,prc->marker_bit);*/
		}

		if (prg->frame_len + length > prc->outbuf_size)
		{
			return HIK_RTP_UNPACK_LIB_E_MEM_OVER;
		}

		protecLen = (int)(length - 3);
	 #if 1
		if (protecLen <= 0)
		{
			printf("--------err length %d,protecLen:%d--------\n", length, protecLen);
			return HIK_RTP_UNPACK_LIB_E_MEM_OVER;
		}

		#endif
		memcpy(&dstbuf[prg->frame_len], &srcbuf[3], length - 3);
		prg->frame_len += (length - 3);
#if 0
		if ((srcbuf[2] & 0x40) != 0x40)  /* end of fu_a */
		{
			prc->outbuf_len = 0;
			return HIK_RTP_UNPACK_LIB_S_FAIL;
		}

		prg->unit_end = 1;
#else
		if ((srcbuf[2] & 0x40) == 0x40) /* end of fu_a */
		{
			prc->is_end_nalu = 1;
		}

#endif
	}
	else if (pack_type >= NAL_UNIT_CODED_SLICE_TRAIL_N && pack_type < NAL_UNIT_INVALID) /* SINGLE NALU 的传输形式 */
	{
		if (length + 4 > prc->outbuf_size)
		{
			return HIK_RTP_UNPACK_LIB_E_MEM_OVER;
		}

		len_tmp = add_avc_annexb_startcode(dstbuf);
		prg->frame_len = len_tmp;
		/* dstbuf = &dstbuf[len_tmp]; */

		memcpy(&dstbuf[prg->frame_len], srcbuf, length);
		prg->frame_len += length;
		prg->unit_end = 1;
		prc->is_end_nalu = 1;
		/*PRT(" =- packType %d data 0x%x 0x%x length %d \n",pack_type,srcbuf[0],srcbuf[1],length);*/
	}
	else
	{
		return HIK_RTP_UNPACK_LIB_E_STM_ERR;
	}

	return HIK_RTP_UNPACK_LIB_S_OK;
}

#else
{
  unsigned char * dstbuf = &prc->outbuf[prg->frame_len];
  unsigned int  pack_type, len_tmp;

  pack_type = (srcbuf[0] & 0x7f) >> 1;

  if (pack_type == 48 || pack_type == 50) //两种PayloadHdr类型不支持
  {
    printf("process h265 error\n");
    return HIK_RTP_UNPACK_LIB_E_STM_ERR;
  }
  else if (pack_type == 49)
  {
    if ((srcbuf[2] & 0x80))//start of fu_a
    {
      len_tmp      = add_avc_annexb_startcode(dstbuf); //添加起始码，复用H.264起始码
      dstbuf[len_tmp]   = (srcbuf[0] & 0x81) | ((srcbuf[2] & 0x3f) << 1); //第0个字节和第2个字节合成起始码后的第一个字节
      dstbuf[len_tmp + 1] = srcbuf[1]; //直接使用第一个字节
      dstbuf = &dstbuf[len_tmp + 2];
      prg->frame_len += 6; //帧长度增加6
    }

    if (prg->frame_len + length > prc->outbuf_size)
    {
      return HIK_RTP_UNPACK_LIB_E_MEM_OVER;
    }

    memcpy(dstbuf, &srcbuf[3], length - 3);
    prg->frame_len += (length - 3);

     if ((srcbuf[2] & 0x40) != 0x40) //end of fu_a
     {
       prc->outbuf_len = 0;
       return HIK_RTP_UNPACK_LIB_S_FAIL;
     }

    prg->unit_end = 1;
  }
  else
  {
    if (length + 4 > prc->outbuf_size)
    {
      return HIK_RTP_UNPACK_LIB_E_MEM_OVER;
    }

    len_tmp = add_avc_annexb_startcode(dstbuf);
    prg->frame_len += len_tmp;
    dstbuf = &dstbuf[len_tmp];

    memcpy(dstbuf, srcbuf, length);
    prg->frame_len += length;
    prg->unit_end = 1;
  }

  return HIK_RTP_UNPACK_LIB_S_OK;
}
#endif
/**  @fn  HRESULT RTP_UNPACK_process_mjpeg(unsigned char *  srcbuf,
                                           unsigned int  length,
                                           RTPUNPACK_PRC_PARAM *prc,
                                           RTP_UNPACK_PRG *prg)
*  @brief  <解析 MJPEG 的 rtp 数据包>
*  @param srcbuf   [IN]缓冲区.
*  @param length   [IN] 缓冲区长度
*  @param prc    [OUT]输入输出数据结构体.
*  @param prg    [IN] 内部参数结构体
*  @return  返回错误码
*/
HRESULT RTP_UNPACK_process_mjpg(unsigned char *  srcbuf,
                                unsigned int  length,
                                RTPUNPACK_PRC_PARAM *prc,
                                RTP_UNPACK_PRG *prg)
{
  unsigned char * dstbuf;
  int quality;
  int width;
  int height;
  BS_PTR bs;

  quality = srcbuf[5];
  width   = srcbuf[6] << 3;
  height  = srcbuf[7] << 3;

  if (quality <= 0 || quality > 99)
  {
    return HIK_RTP_UNPACK_LIB_S_FAIL;
  }
  dstbuf = &prc->outbuf[prg->frame_len];

  if (((srcbuf[1] << 16) + (srcbuf[2] << 8) + srcbuf[3]) == 0)
  {
    jpg_set_quality(&prg->jpg_param, quality);
    bs.ptr = dstbuf;
    jpg_write_file_hdr(&prg->jpg_param, &bs, width, height);
    prg->frame_len += bs.ptr - dstbuf;
  }
  dstbuf = &prc->outbuf[prg->frame_len];
  length -= 8;//mjpeg有8字节扩展头无效
  srcbuf  += 8;


  if (prg->frame_len + length > prc->outbuf_size)
  {
    return HIK_RTP_UNPACK_LIB_E_MEM_OVER;
  }

  memcpy(dstbuf, srcbuf, length);

  prg->frame_len += length;

  return HIK_RTP_UNPACK_LIB_S_OK;
}

/**@fn  unsigned int add_avc_annexb_startcode(unsigned char *buffer)
*  @brief  <获取RTP_UNPACK所需内存大小>
*  @param param  [IN] 参数结构指针
*  @return  返回起始码长度
*/
HRESULT RtpUnpack_GetMemSize(RTPUNPACK_PARAM *param)
{
  if (NULL == param)
  {
    return HIK_RTP_UNPACK_LIB_E_PARA_NULL;
  }
  param->buf_size = sizeof(RTP_UNPACK_PRG);

  //printf("RtpUnpack_GetMemSize buf_size %d\n",param->buf_size);
  return HIK_RTP_UNPACK_LIB_S_OK;
}

/**  @fn  HRESULT RtpUnpack_Create(RTPUNPACK_PARAM *param, void **handle)
*  @brief  <创建 RTP_UNPACK 模块>
*  @param param   [IN] 参数结构指针
*  @param handle  [IN] 内部参数模块句柄
*  @return  返回起始码长度
*/
HRESULT RtpUnpack_Create(RTPUNPACK_PARAM *param, void **handle)
{
  RTP_UNPACK_PRG *prg = NULL;
  if (NULL == param || NULL == handle)
  {
    return HIK_RTP_UNPACK_LIB_E_PARA_NULL;
  }
  if (NULL == param->buffer || param->buf_size != sizeof(RTP_UNPACK_PRG))
  {
    return HIK_RTP_UNPACK_LIB_E_PARA_NULL;
  }

  prg = (RTP_UNPACK_PRG *)param->buffer;
  prg->frame_len    = 0;
  prg->unit_end     = 0;
  prg->stream_type  = param->stream_type;
  prg->stream_info.is_hik_stream   = 0;
  prg->stream_info.dev_chan_id_flg = 0;
  prg->stream_info.privt_stream_type = STREAM_TYPE_UNDEF;

  if (param->stream_type == STREAM_TYPE_VIDEO_MJPG)
  {
    jpg_std_huff_tables(&prg->jpg_param);
  }

  #ifdef SRTP_SUPPORT

  memset(prg->srtp_info, 0, RTP_MAX_STREAMNUM * sizeof(SRTP_INFO));

  #endif

  *handle = (void *)prg;
  return HIK_RTP_UNPACK_LIB_S_OK;
}

/**@fn  HRESULT RtpUnpack_Process(RTPUNPACK_PRC_PARAM * param, void *handle)
*  @brief  <解析一段 rtp 数据>
*  @param param  [IN] 参数结构指针
*  @param handle  [IN] 内部参数模块句柄
*  @return  返回起始码长度
*/
HRESULT RtpUnpack_Process(RTPUNPACK_PRC_PARAM * param, void *handle)
{
	RTP_UNPACK_PRG * prg;
	unsigned char *  buffer;
	unsigned char *  payload_buf;
	unsigned int  last_markerbit = 0;
	unsigned int  payload_len, padding_len,ext_header_len, sequence_number;
	HRESULT hr = HIK_RTP_UNPACK_LIB_S_FAIL;
	int payload_type = 0;

	if (param == NULL || handle == NULL)
	{
	    return HIK_RTP_UNPACK_LIB_E_PARA_NULL;
	}
	if (param->inbuf == NULL || param->outbuf == NULL)
	{
	    return HIK_RTP_UNPACK_LIB_E_PARA_NULL;
	}

	last_markerbit = param->marker_bit;

#ifdef SRTP_SUPPORT

	hr = RtpUnpack_SrtpUnPack(param, (RTP_UNPACK_PRG *)handle);
	if (HIK_RTP_UNPACK_LIB_S_OK != hr)
	{
	    return hr;
	}

#endif

	prg    = (RTP_UNPACK_PRG *)handle;

	buffer  = param->inbuf;
	param->outbuf_len     = 0;
	param->is_privt_info  = 0;
	param->is_end_nalu    = 0;
	param->status         = 0;
	param->time_stamp = (buffer[4] << 24) + (buffer[5] << 16) + (buffer[6] << 8) + buffer[7];
	param->marker_bit = buffer[1] >> 7;
	sequence_number = (buffer[2] << 8) | (buffer[3]);

	if(param->inbuf_len <= 0)
	{
	    return HIK_RTP_UNPACK_LIB_S_OK;
	}

	payload_type        = buffer[1]&0x7f;	  // szl_debug
	param->payload_type = payload_type;
	
	/*扩展头去掉 */
	if ((buffer[0] & 0x10) && (param->payload_type != RTP_PAYLOAD_TYPE_HIK_PRIVT))
	{
	    ext_header_len = ((buffer[14] << 8) + buffer[15]) << 2;
	    payload_buf = &buffer[12 + ext_header_len + 4];
	    if (param->inbuf_len <= 12 + ext_header_len + 4)
	    {            
	        //printf("Ext header %d err, \n",ext_header_len);
	        return HIK_RTP_UNPACK_LIB_S_FAIL;
	    }

	    payload_len = param->inbuf_len - 12 - ext_header_len - 4;
	    if (payload_len == 0)
	    {
	        /*有些ipc的私有信息sequence_number和视频的连续*/
	        if (sequence_number == param->sequence_number + 1)
	        {
	            param->sequence_number = sequence_number;
	        }

	        printf("Ext header payload_len err\n");
	        return HIK_RTP_UNPACK_LIB_S_OK;
	    }

	}
	else
	{
	    payload_buf = &buffer[12];
	    payload_len = param->inbuf_len - 12;
	}

	if (buffer[0] & 0x20)//有padding bytes则先去掉
	{
	    padding_len = buffer[param->inbuf_len - 1];
	    payload_len -= padding_len;
	}

	if (payload_len == 0 || payload_len >= param->inbuf_len)
	{
	    //printf("RTPUnpack: payload_len=%d inbuf_len = %d \n", payload_len, param->inbuf_len);
	    return HIK_RTP_UNPACK_LIB_S_OK;
	}

	SAL_dbPrintf(unpackDbLevel,DEBUG_LEVEL2,"%s|%d:payload_type is 0x%x,last_markerbit 0x%x,0x%x,stream_type 0x%x\n",__func__, __LINE__,
		payload_type,last_markerbit, param->marker_bit,param->stream_type);
	//printf("            payload_type is 0x%x,last_markerbit 0x%x,0x%x, param->stream_type 0x%x\n",payload_type,last_markerbit, param->marker_bit,param->stream_type);
	/*此处必须使用last_markerbit 表示私有包的结束，开始判断H264流里面的SPS格式是否符合*/

	/* 此处stream_type在初始化写死了，目前仅支持video和audio两种 */
	if((STREAM_TYPE_UNDEF == param->stream_type) && last_markerbit && (DEFAULT_VIDEO_PT == payload_type))
	{
	  if (STREAM_TYPE_UNDEF == param->streamType)
	  {
	      prg->stream_type   = RtpUnpack_StreamType(payload_buf);
	      param->stream_type = prg->stream_type;
	  }
	  else
	  {
	      prg->stream_type   = param->streamType;//将app层配置的streamType赋给rtp包数据码流类型
	      param->stream_type = prg->stream_type; //将码流类型赋解包内stream_type用于解码确定码流类型
	  }
	}

	if (param->payload_type == param->videoPayloadType)
    {
        if (param->sequence_number && (sequence_number > param->sequence_number + 1) && (!last_markerbit))
        {
            printf("sequence_number:%d, param->sequence_number:%d\n", sequence_number, param->sequence_number);
            param->sequence_number = 0;
            
            param->status = HIK_RTP_UNPACK_LIB_E_SN_ERR;
        }
        else
        {
            param->sequence_number = sequence_number;
        }
	}
	
	//printf("process payload_type 0x%x, prg->stream_type 0x%x, marker_bit %d \n", payload_type, prg->stream_type, param->marker_bit);
	
	switch(payload_type)
	{
		case RTP_PAYLOAD_TYPE_HIK_PRIVT:
		{
		   hr = RTP_UNPACK_parse_privt_info(payload_buf, payload_len, prg, param);
		   break;
		}
		case DEFAULT_VIDEO_PT: ///<普通视频
		case MJPEG_VIDEO_PT: ///< motion jpeg
		case SVAC_VIDEO_PT: ///<svac码流
		case FENGHUO_VIDEO_PT: ///< 烽火RTP
		case SONY_VIDEO_PT: ///< 索尼视频
		case MPEG2_VIDEO_PT: ///< mpeg2视频
		case HIK_VIDEO_264_PT: ///< 海康264
		{
		    switch(prg->stream_type)
			{
			    case STREAM_TYPE_VIDEO_H264:
			    {
			      hr = RTP_UNPACK_process_h264(payload_buf,  payload_len, param, prg);
			      break;
			    }
			    case STREAM_TYPE_VIDEO_H265:
			    {
			      hr = RTP_UNPACK_process_h265(payload_buf, payload_len, param, prg);
			      break;
			    }
			    case STREAM_TYPE_VIDEO_MJPG:
			    {
			      hr = RTP_UNPACK_process_mjpg(payload_buf,  payload_len, param, prg);
			      break;
			    }
			    case STREAM_TYPE_UNDEF: // 无法识别的流类型先不处理
			    {
			        printf("error process payload_type 0x%x(UNDEF),stream_type 0x%x(0x%x),%p\n",payload_type,prg->stream_type,param->streamType,handle);
			        break;
			    }
			    default:
			    {
					printf("error process payload_type 0x%x,stream_type 0x%x\n",payload_type,prg->stream_type);
			        break;
			    }
			  }
		  break;
		}
		default:
		{
			if (param->payload_type != HIK_PAYLOAD_TYPE_G711U_AUDIO
            && param->payload_type != HIK_PAYLOAD_TYPE_G711A_AUDIO
            && param->payload_type != HIK_PAYLOAD_TYPE_AAC_AUDIO) /* audio type defined in RFC-3551 */
            { 
                SAL_dbPrintf(unpackDbLevel,DEBUG_LEVEL2,"error process audio payload_type 0x%x\n",payload_type);
			}
		    hr = RTP_UNPACK_process_stream(payload_buf,  payload_len, param, prg);
		}
	}
	//一个nalu结束或一帧结束，送出数据
	//if ((prg->unit_end || (buffer[1] >> 7)) && (hr == HIK_RTP_UNPACK_LIB_S_OK))
	if(hr == HIK_RTP_UNPACK_LIB_S_OK)
	{
	    return RTP_UNPACK_output_frame(param, prg);
	}
	else
	{   prg->frame_len      = 0;
        //prg->stream_type    = STREAM_TYPE_UNDEF;
        //param->stream_type  = STREAM_TYPE_UNDEF;
	    return hr;
	}
}

/**  @fn  HRESULT RtpUnpack_SetSecureInfo(RTP_SECURE_INFO *info, void *handle)
*  @brief  <设置密钥初始化>
*  @param info    [IN] 参数结构指针
*  @param handle  [IN] 内部参数模块句柄
*  @return  返回起始码长度
*/
HRESULT RtpUnpack_SetSecureInfo(RTP_SECURE_INFO *info, void *handle)
{
  #ifdef SRTP_SUPPORT

  RTP_UNPACK_PRG *prg = (RTP_UNPACK_PRG *)handle;
  HRESULT hr = HIK_RTP_UNPACK_LIB_S_OK;
  int i = 0;

  if (NULL == info)
  {
    return HIK_RTP_UNPACK_LIB_E_PARA_NULL;
  }

  if (0 == info->rtp_secure)
  {
    return HIK_RTP_UNPACK_LIB_S_OK;
  }

  if (NULL == prg)
  {
    return HIK_RTP_UNPACK_LIB_E_PARA_NULL;
  }


  for(i = 0; i < RTP_MAX_STREAMNUM;i++)
  {
    if (0 == prg->srtp_info[i].setsecure)
    {
      hr = hik_srtp_init(&prg->srtp_info[i], info);//初始化
      if (HIK_SRTP_OK == hr)
      {
        prg->srtp_info[i].ssrc    = info->ssrc;
        prg->srtp_info[i].setsecure = 1;
      }

      break;
    }
    else
    {
      if (prg->srtp_info[i].ssrc == info->ssrc)
      {
        break;
      }
    }

  }

  if (HIK_SRTP_OK == hr)
  {
    return HIK_RTP_UNPACK_LIB_S_OK;
  }
  else
  {
    return HIK_RTP_UNPACK_LIB_E_SRTP;
  }

  #else

  return HIK_RTP_UNPACK_LIB_E_SRTP;

  #endif
}

void RtpUnpack_SetDbLeave(int level)
{
     unpackDbLevel = (level > 0) ? level : 0;
	 SAL_INFO("unpackDbLevel %d\n",unpackDbLevel);
}


#ifdef SRTP_SUPPORT

/**  @fn  HRESULT RtpUnpack_SrtpUnPack(RTPUNPACK_PRC_PARAM *param, RTP_UNPACK_PRG *prg)
*  @brief  <解析一段 srtp 数据>
*  @param prc    [IN]输入输出数据结构体.
*  @param prg    [IN] 内部参数结构体
*  @return  返回起始码长度
*/
HRESULT RtpUnpack_SrtpUnPack(RTPUNPACK_PRC_PARAM *param, RTP_UNPACK_PRG *prg)
{
  unsigned int ssrc     = 0;
  int srtp_index      = 0;
  int ext_marker      = 0;
  unsigned short seq_num  = 0;
  unsigned int csrc_count   = 0;

  unsigned char *rtp_buf  = NULL;
  unsigned int  rtp_len   = 0;

  unsigned char *enc_start  = NULL;
  unsigned int enc_len    = 0;
  unsigned char *auth_start = NULL;
  unsigned char *auth_tag   = NULL;
  int index_delta       = 0;
  int prefix_len      = 0;
  int hr          = HIK_SRTP_OK;
  xtd_seq_num_t est_seq_num = {0};

  unsigned char tmp_tag[SRTP_MAX_TAG_LEN] = {0};

  if (NULL == param || NULL == param->inbuf)
  {
    return HIK_RTP_UNPACK_LIB_E_PARA_NULL;
  }

  if (NULL == prg)
  {
    return HIK_RTP_UNPACK_LIB_E_PARA_NULL;
  }

  rtp_buf = param->inbuf;
  rtp_len = param->inbuf_len;

  ext_marker = (rtp_buf[0] >> 4)& 0x01;
  csrc_count = rtp_buf[0] & 0xf;
  seq_num  = (rtp_buf[2] << 8) + rtp_buf[3];
  ssrc     = (rtp_buf[8] << 24)
            + (rtp_buf[9] << 16)
            + (rtp_buf[10] << 8)
            + rtp_buf[11];

  srtp_index = hik_srtp_getindex(prg->srtp_info, RTP_MAX_STREAMNUM, ssrc); //获取SRTP的索引序号
  if (-1 == srtp_index)
  {
    return HIK_RTP_UNPACK_LIB_S_OK;
  }

 /*
 * given an rdbx and a sequence number s (from a newly arrived packet),
 * sets the contents of *guess to contain the best guess of the packet
 * index to which s corresponds, and returns the difference between
 * *guess and the locally stored synch info
 */
  hr = hik_srtp_rdbx_index(&prg->srtp_info[srtp_index],
                           &est_seq_num,
                           seq_num,
                           &index_delta); //
  if (HIK_SRTP_OK != hr)
  {
    return HIK_RTP_UNPACK_LIB_E_SRTP;
  }

  /*
 *  checks to see if the xtd_seq_num_t
 * which is at rdbx->index + delta is in the rdb
 */
  hr = hik_srtp_rdbx_check(&prg->srtp_info[srtp_index], index_delta);
  if (HIK_SRTP_OK != hr)
  {
    return HIK_RTP_UNPACK_LIB_E_SRTP;
  }

  if (SEC_SERV_AUTH & prg->srtp_info[srtp_index].sec_serv)
  {
    rtp_len -= prg->srtp_info[srtp_index].auth_tag_len;
  }

  if (SEC_SERV_CONF & prg->srtp_info[srtp_index].sec_serv)
  {
    enc_start = rtp_buf + (RTP_HEAD_LEN + (csrc_count << 2));
    enc_len   = rtp_len - (RTP_HEAD_LEN + (csrc_count << 2));

    if (1 == ext_marker)
    {
      unsigned int ext_len = 0;

      ext_len  = ((enc_start[2] << 8) + enc_start[3]) << 2;
      ext_len   += 4;

      enc_start += ext_len;
      enc_len   -= enc_len;

    }
  }

  if (SEC_SERV_AUTH & prg->srtp_info[srtp_index].sec_serv)
  {
    auth_start = rtp_buf;
    auth_tag   = rtp_buf + rtp_len;
  }

  if (NULL != auth_start)
  {
    int i = 0;
    hr = hik_srtp_encrypt_auth(&prg->srtp_info[srtp_index],
                               &est_seq_num,
                               auth_start,
                               tmp_tag,
                               rtp_len); //加密
    if (HIK_SRTP_OK != hr)
    {
      return HIK_RTP_UNPACK_LIB_E_SRTP;
    }

    for(i = 0; i < prg->srtp_info[srtp_index].auth_tag_len; i++)
    {
      if (tmp_tag[i] != auth_tag[i])
      {
        return HIK_RTP_UNPACK_LIB_E_SRTP;
      }

    }
  }

  hr = hik_srtp_keylimit_update(&prg->srtp_info[srtp_index]);//更新
  if (HIK_SRTP_OK != hr)
  {
    return HIK_RTP_UNPACK_LIB_E_SRTP;
  }

  if (NULL != enc_start)
  {
    hr = hik_srtp_encrypt_data(&prg->srtp_info[srtp_index],
                               &est_seq_num,
                               ssrc,
                               enc_start,
                               &enc_len);//加密
    if (HIK_SRTP_OK != hr)
    {
      return HIK_RTP_UNPACK_LIB_E_SRTP;
    }
  }

  /*
 * hik_srtp_rdbx_addindex adds the xtd_seq_num_t at rdbx->window_start + d to
 * replay_db (and does *not* check if that xtd_seq_num_t appears in db)
 *
 * this function should be called only after replay_check has
 * indicated that the index does not appear in the rdbx, e.g., a mutex
 * should protect the rdbx between these calls if need be
 */
  hr = hik_srtp_rdbx_addindex(&prg->srtp_info[srtp_index], index_delta);
  if (HIK_SRTP_OK != hr)
  {
    return HIK_RTP_UNPACK_LIB_E_SRTP;
  }

  param->inbuf_len = rtp_len;

  return HIK_RTP_UNPACK_LIB_S_OK;
}

#endif


