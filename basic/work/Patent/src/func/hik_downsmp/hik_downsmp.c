/***********************************************************************
*
* 版权信息：Copyright (c) 2017, 杭州海康威视数字技术股份有限公司
*
* 文件名称：hik_downsmp.c
* 文件标识：HIKVISION_GENERAL_DOWN_SAMPLE_INTERFACE_HIK_DOWNSMP_C_
* 摘    要：海康威视通用下采样接口实现模块
*
************************************************************************
*/

#include "hik_downsmp.h"

#include <string.h>
#include "sal_trace.h"

#define REDUCE_SHIFT_BITS	10
#define EXPAND_MUL			1024

#define LUMA_BLACK_VALUE	0
#define CHROMA_BLACK_VALUE	128


/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
__inline unsigned char downsmp_one_value(unsigned char *src, int smp_len, int pitch)
{
	int i, j;
	unsigned char *pix_ptr;
	unsigned char ret;
	unsigned int smp_num = 0;
	unsigned int sum_sum = 0;

	for (i = 0; i < smp_len; i += 2)
	{
		pix_ptr = src + i*pitch;
		for (j = 0; j < smp_len; j++)
		{
			sum_sum += pix_ptr[j];
			smp_num++;
		}
	}

    if(smp_num)
    {
	    ret = (unsigned char)(sum_sum/smp_num);
    }

	return ret;

}


/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：原版本耗时为1641us，更新版本为200us
* 修  改: 2013/5/29
******************************************************************************/
void HIKDS_downsmp_proc(  unsigned char *dst,
						  unsigned char *src,
						  int dst_width,
						  int dst_height,
						  int dst_pitch,
						  int src_pitch,
						  float scale )
{
	int i, j;
	int smp_stride = (int)(scale * 1024);
	int smp_offset = (int)(scale / 2);
	int src_inc;

	unsigned char *src_line;
	unsigned char *dst_pix, *src_pix;

	src += smp_offset * src_pitch;

	for (i = 0; i < dst_height; i++)
	{
		src_line = src + ((smp_stride * i) >> 10) * src_pitch;
		dst_pix  = dst + i * dst_pitch;
		src_pix  = src_line;
		src_inc  = 0;

		for (j = 0; j < dst_width; j++)
		{
			//dst_pix[j] = downsmp_one_value(src_pix, smp_len, src_pitch);

			dst_pix[j] = (src_pix[0] + src_pix[smp_offset] + 1) >> 1; // 采用简单的抽点下采样方式来实现

			src_inc += smp_stride;
			src_pix  = src_line + (src_inc >> 10);

		}
	}


}
/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
void HIKDS_fill_pad_ver(unsigned char *dst, int pad_size, int height, int pitch, int rightpad_flag)
{
	int i, uv;

	for ( i = 0; i < height; i++)
	{
		memset(dst, LUMA_BLACK_VALUE, pad_size);
		dst += pitch;
	}


	height	>>= 1;
	pitch	>>= 1;
	pad_size>>= 1;

	dst += rightpad_flag * (pad_size - pitch);

	for (uv = 0; uv < 2; uv++)
	{
		for (i = 0; i < height; i++)
		{
			memset(dst, CHROMA_BLACK_VALUE, pad_size);
			dst += pitch;
		}
	}
}

/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
void HIKDS_fill_pad_hor(unsigned char *dst, int pad_size, int width, int height, int botpad_flag)
{
	int i, uv;
	int luma_size = width * height;
	unsigned char *pix_ptr = dst;

	for ( i = 0; i < pad_size; i++)
	{
		memset(pix_ptr, LUMA_BLACK_VALUE, width);
		pix_ptr += width;
	}

	dst += luma_size;


	width	>>= 1;
	pad_size>>= 1;

	dst -= (((luma_size * 3) >> 2) - pad_size * width * 3) * botpad_flag;

	for (uv = 0; uv < 2; uv++)
	{
		pix_ptr = dst + uv * (luma_size >> 2);

		for (i = 0; i < pad_size; i++)
		{
			memset(pix_ptr, CHROMA_BLACK_VALUE, width);
			pix_ptr += width;
		}
	}
}

/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
HRESULT HIKDS_downsmp_frame_horpad(DOWNSMP_PARAM *param)
{
	float aspect_ratio = (float)param->src_width / param->src_height;
	float scale        = (float)param->src_width / param->dst_width;

	int uv;
	int img_width     = param->dst_width;
	int img_height    = (int)(img_width / aspect_ratio);
	int pad_size      = (param->dst_height - img_height) >> 1;
	int src_luma_size = param->src_height * param->src_width;
	int dst_luma_size = param->dst_height * param->dst_width;

	unsigned char *dst_buf, *src_buf;


	if (param->src_height * param->dst_width == param->src_width *param->dst_height)
	{
		img_height = param->dst_height;
		pad_size   = 0;
	}

	pad_size    &= 0xfffffffe;
	img_height  &= 0xfffffffe;

	dst_buf = param->dst_buf;


	HIKDS_fill_pad_hor(dst_buf, pad_size, param->dst_width, param->dst_height, 0);


	dst_buf = param->dst_buf + pad_size * param->dst_width;
	src_buf = param->src_buf;

	HIKDS_downsmp_proc( dst_buf,
						src_buf,
						img_width,
						img_height,
						param->dst_width,
						param->src_width,
						scale );//对亮度进行下采样


	for (uv = 0; uv < 2; uv++)
	{
		dst_buf = param->dst_buf + dst_luma_size + uv * (dst_luma_size >> 2) + (pad_size >> 1) * (img_width >> 1);
		src_buf = param->src_buf + src_luma_size + uv * (src_luma_size >> 2);

		HIKDS_downsmp_proc( dst_buf,
							src_buf,
							img_width>>1,
							img_height>>1,
							param->dst_width>>1,
							param->src_width>>1,
							scale );//对两个色度进行下采样
	}


	dst_buf  = param->dst_buf + (pad_size + img_height) * img_width;
	pad_size = param->dst_height - img_height - pad_size;

	HIKDS_fill_pad_hor(dst_buf, pad_size, param->dst_width, param->dst_height, 1);

	return HIK_DOWNSMP_INTERFACE_S_OK;
}



/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
HRESULT HIKDS_downsmp_frame_verpad(DOWNSMP_PARAM *param)
{
	float aspect_ratio = (float)param->src_width / param->src_height;
	float scale        = (float)param->src_height / param->dst_height;

	int uv;
	int img_height    = param->dst_height;
	int img_width     = (int)(img_height * aspect_ratio);
	int pad_size      = (param->dst_width - img_width) >> 1;
	int src_luma_size = param->src_height * param->src_width;
	int dst_luma_size = param->dst_height * param->dst_width;

	unsigned char *dst_buf, *src_buf;


	pad_size   &= 0xfffffffe;
	img_width  &= 0xfffffffe;

	dst_buf = param->dst_buf;

	HIKDS_fill_pad_ver(dst_buf, pad_size, img_height, param->dst_width, 0);//对左边的黑框进行填充


	dst_buf = param->dst_buf + pad_size;
	src_buf = param->src_buf;

	HIKDS_downsmp_proc( dst_buf,
						src_buf,
						img_width,
						img_height,
						param->dst_width,
						param->src_width,
						scale );//对亮度进行下采样


	for (uv = 0; uv < 2; uv++)
	{
		dst_buf = param->dst_buf + dst_luma_size + uv * (dst_luma_size >> 2) + (pad_size >> 1);
		src_buf = param->src_buf + src_luma_size + uv * (src_luma_size >> 2);

		HIKDS_downsmp_proc( dst_buf,
							src_buf,
							img_width>>1,
							img_height>>1,
							param->dst_width>>1,
							param->src_width>>1,
							scale );//对两个色度进行下采样
	}



	dst_buf  = param->dst_buf + pad_size + img_width;
	pad_size = (param->dst_width - img_width - pad_size);

	HIKDS_fill_pad_ver(dst_buf, pad_size, img_height, param->dst_width, 1);//对右边的黑框进行填充

	return HIK_DOWNSMP_INTERFACE_S_OK;
}

/******************************************************************************
* 功  能：对一个YUV帧进行下采样
* 参  数：param				-  下采样参数结构体指针		[IN:OUT]
*
* 返回值：成功返回			-	HIK_DOWNSMP_INTERFACE_S_OK
*		  输入参数空返回	-   HIK_DOWNSMP_INTERFACE_E_PARA_NULL
*		  输入参数错误返回	-	HIK_DOWNSMP_INTERFACE_E_INPUT
*		  下采样失败返回	-	HIK_DOWNSMP_INTERFACE_S_FAIL
* 备  注：
* 修  改: 2008/9/16
******************************************************************************/
HRESULT HIKDS_DownSmpOneFrame(DOWNSMP_PARAM *param)
{
    if (param == NULL)
	{
		return HIK_DOWNSMP_INTERFACE_E_PARA_NULL;
	}
		
	int src_width  = param->src_width;
	int src_height = param->src_height;
	int dst_width  = param->dst_width;
	int dst_height = param->dst_height;
	float hor_mul = (float)src_width/dst_width;
	float ver_mul = (float)src_height/dst_height;

	if (param->src_buf == NULL || param->dst_buf == NULL)
	{
		return HIK_DOWNSMP_INTERFACE_E_INPUT;
	}

	//只支持下采样的方式
	if (src_height < dst_height || src_width < dst_width)
	{
		return HIK_DOWNSMP_INTERFACE_E_INPUT;
	}

	//输入、输出图像的大小必须为4的整数倍
	if ((src_height | dst_height | src_width | dst_width) & 3)
	{
		return HIK_DOWNSMP_INTERFACE_E_INPUT;
	}


	if (hor_mul >= ver_mul)
	{
		HIKDS_downsmp_frame_horpad(param);
	}
	else
	{
		HIKDS_downsmp_frame_verpad(param);
	}

	return HIK_DOWNSMP_INTERFACE_S_OK;
}



////////////////////////////////////////////////////////////////////////




/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
__inline unsigned char downsmp_one_value_NV12(unsigned char *src, int smp_len, int pitch)
{
    int i= 0, j = 0;
    unsigned char *pix_ptr;
    unsigned char ret = 0;
    unsigned int smp_num = 0;
    unsigned int sum_sum = 0;

    for (i = 0; i < smp_len; i += 2)
    {
        pix_ptr = src + i*pitch;
        for (j = 0; j < smp_len; j++)
        {
            sum_sum += pix_ptr[j];
            smp_num++;
        }
    }
    if(smp_num)
    {
        ret = (unsigned char)(sum_sum/smp_num);
    }

    return ret;
}


/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：原版本耗时为1641us，更新版本为200us
* 修  改: 2013/5/29
******************************************************************************/
void HIKDS_downsmp_proc_NV12( unsigned char *dst,
                             unsigned char *src,
                             int dst_width,
                             int dst_height,
                             int dst_pitch,
                             int src_pitch,
                             float scale )
{
    int i, j;
    int smp_stride = (int)(scale * 1024);
    int smp_offset = (int)(scale / 2);
    int src_inc;

    unsigned char *src_line;
    unsigned char *dst_pix, *src_pix;

    src += smp_offset * src_pitch;

    for (i = 0; i < dst_height; i++)
    {
        src_line = src + ((smp_stride * i) >> 10) * src_pitch;
        dst_pix  = dst + i * dst_pitch;
        src_pix  = src_line;
        src_inc  = 0;

        for (j = 0; j < dst_width; j++)
        {
            //dst_pix[j] = downsmp_one_value(src_pix, smp_len, src_pitch);

            dst_pix[j] = (src_pix[0] + src_pix[smp_offset] + 1) >> 1; // 采用简单的抽点下采样方式来实现

            src_inc += smp_stride;
            src_pix  = src_line + (src_inc >> 10);
        }
    }
}

/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2015/12/7
******************************************************************************/
void HIKDS_downsmp_proc_uv_NV12( unsigned short *dst,
                                unsigned short *src,
                                int dst_width,
                                int dst_height,
                                int dst_pitch,
                                int src_pitch,
                                float scale )
{
    int i, j;
    int ave_u, ave_v;
    int smp_stride = (int)(scale * 1024);
    int smp_offset = (int)(scale / 2);
    int src_inc;

    unsigned short *src_line;
    unsigned short *dst_pix, *src_pix;

    src += smp_offset * src_pitch;

    for (i = 0; i < dst_height; i++)
    {
        src_line = src + ((smp_stride * i) >> 10) * src_pitch;
        dst_pix  = dst + i * dst_pitch;
        src_pix  = src_line;
        src_inc  = 0;

        for (j = 0; j < dst_width; j++)
        {
            ave_v      = ((src_pix[0] & 0xff) + (src_pix[smp_offset] & 0xff) + 1) >> 1;   // v数据处理
            ave_u      = ((src_pix[0] >> 8)   + (src_pix[smp_offset] >> 8)   + 1) >> 1;   // u数据处理

            dst_pix[j] = (ave_v + (ave_u << 8));                     // 采用简单的抽点下采样方式来实现

            src_inc += smp_stride;
            src_pix  = src_line + (src_inc >> 10);

        }
    }
}

/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
void HIKDS_fill_pad_ver_NV12(unsigned char *dst, int pad_size, int height, int pitch)
{
    int i;

    for ( i = 0; i < height; i++)
    {
        memset(dst, LUMA_BLACK_VALUE, pad_size);
        dst += pitch;
    }

    height	>>= 1;

    for (i = 0; i < height; i++)
    {
        memset(dst, CHROMA_BLACK_VALUE, pad_size);
        dst += pitch;
    }
}

/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
void HIKDS_fill_pad_hor_NV12(unsigned char *dst, int pad_size, int width, int height, int botpad_flag)
{
    int i;
    int luma_size = width * height;
    unsigned char *pix_ptr = dst;

    for ( i = 0; i < pad_size; i++)
    {
        memset(pix_ptr, LUMA_BLACK_VALUE, width);
        pix_ptr += width;
    }

    dst += luma_size;

    pad_size >>= 1;

    dst -= ((luma_size >> 1) - (pad_size * width)) * botpad_flag;

    pix_ptr = dst;

    for (i = 0; i < pad_size; i++)
    {
        memset(pix_ptr, CHROMA_BLACK_VALUE, width);
        pix_ptr += width;
    }
}

/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
HRESULT HIKDS_downsmp_frame_horpad_NV12(DOWNSMP_PARAM *param)
{
    float aspect_ratio = (float)param->src_width / param->src_height;
    float scale        = (float)param->src_width / param->dst_width;

    int img_width     = param->dst_width;
    int img_height    = (int)(img_width / aspect_ratio);
    int pad_size      = (param->dst_height - img_height) >> 1;
    int src_luma_size = param->src_height * param->src_width;
    int dst_luma_size = param->dst_height * param->dst_width;

    unsigned char *dst_buf, *src_buf;


    if (param->src_height * param->dst_width == param->src_width *param->dst_height)
    {
        img_height = param->dst_height;
        pad_size   = 0;
    }

    pad_size    &= 0xfffffffe;
    img_height  &= 0xfffffffe;

    dst_buf = param->dst_buf;

    HIKDS_fill_pad_hor_NV12(dst_buf, pad_size, param->dst_width, param->dst_height, 0);

    dst_buf = param->dst_buf + pad_size * param->dst_width;
    src_buf = param->src_buf;

    // y数据下采样
    HIKDS_downsmp_proc_NV12( dst_buf,
        src_buf,
        img_width,
        img_height,
        param->dst_width,
        param->src_width,
        scale );//对亮度进行下采样

    // uv数据下采样
    dst_buf = param->dst_buf + dst_luma_size + (pad_size >> 1) * img_width;
    src_buf = param->src_buf + src_luma_size;

    HIKDS_downsmp_proc_uv_NV12( (unsigned short *)dst_buf,
        (unsigned short *)src_buf,
        img_width>>1,
        img_height>>1,
        param->dst_width>>1,
        param->src_width>>1,
        scale );//对两个色度进行下采样

    dst_buf  = param->dst_buf + (pad_size + img_height) * img_width;
    pad_size = param->dst_height - img_height - pad_size;

    HIKDS_fill_pad_hor_NV12(dst_buf, pad_size, param->dst_width, param->dst_height, 1);

    return HIK_DOWNSMP_INTERFACE_S_OK;
}


/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
HRESULT HIKDS_downsmp_frame_verpad_NV12(DOWNSMP_PARAM *param)
{
    float aspect_ratio = (float)param->src_width / param->src_height;
    float scale        = (float)param->src_height / param->dst_height;

    int img_height    = param->dst_height;
    int img_width     = (int)(img_height * aspect_ratio);
    int pad_size      = (param->dst_width - img_width) >> 1;
    int src_luma_size = param->src_height * param->src_width;
    int dst_luma_size = param->dst_height * param->dst_width;

    unsigned char *dst_buf, *src_buf;

    pad_size   &= 0xfffffffe;
    img_width  &= 0xfffffffe;

    dst_buf = param->dst_buf;

    HIKDS_fill_pad_ver_NV12(dst_buf, pad_size, img_height, param->dst_width);//对左边的黑框进行填充

    dst_buf = param->dst_buf + pad_size;
    src_buf = param->src_buf;

    HIKDS_downsmp_proc_NV12( dst_buf,
        src_buf,
        img_width,
        img_height,
        param->dst_width,
        param->src_width,
        scale );//对亮度进行下采样

    // uv数据下采样
    dst_buf = param->dst_buf + dst_luma_size + pad_size;
    src_buf = param->src_buf + src_luma_size;

    HIKDS_downsmp_proc_uv_NV12( (unsigned short *)dst_buf,
        (unsigned short *)src_buf,
        img_width>>1,
        img_height>>1,
        param->dst_width>>1,
        param->src_width>>1,
        scale );//对两个色度进行下采样

    dst_buf  = param->dst_buf   + pad_size  + img_width;
    pad_size = param->dst_width - img_width - pad_size;

    HIKDS_fill_pad_ver_NV12(dst_buf, pad_size, img_height, param->dst_width);//对右边的黑框进行填充

    return HIK_DOWNSMP_INTERFACE_S_OK;
}



/******************************************************************************
* 功  能：对一个YUV帧进行下采样
* 参  数：param				-  下采样参数结构体指针		[IN:OUT]
*
* 返回值：成功返回			-	HIK_DOWNSMP_INTERFACE_S_OK
*		  输入参数空返回	-   HIK_DOWNSMP_INTERFACE_E_PARA_NULL
*		  输入参数错误返回	-	HIK_DOWNSMP_INTERFACE_E_INPUT
*		  下采样失败返回	-	HIK_DOWNSMP_INTERFACE_S_FAIL
* 备  注：
* 修  改: 2008/9/16
******************************************************************************/
HRESULT HIKDS_DownSmpOneFrame_NV12(DOWNSMP_PARAM *param)
{
    if (param == NULL)
    {
        return HIK_DOWNSMP_INTERFACE_E_PARA_NULL;
    }
	
    int src_width  = param->src_width;
    int src_height = param->src_height;
    int dst_width  = param->dst_width;
    int dst_height = param->dst_height;
    float hor_mul = (float)src_width/dst_width;
    float ver_mul = (float)src_height/dst_height;

    if (param->src_buf == NULL || param->dst_buf == NULL)
    {
        return HIK_DOWNSMP_INTERFACE_E_INPUT;
    }

    //只支持下采样的方式
    if (src_height < dst_height || src_width < dst_width)
    {
        return HIK_DOWNSMP_INTERFACE_E_INPUT;
    }

    //输入图像的大小必须为4的整数倍
    if ((src_height | src_width) & 3)
    {
        return HIK_DOWNSMP_INTERFACE_E_INPUT;
    }

    //输出图像的大小必须为2的整数倍
    if ((dst_height | dst_width) & 1)
    {
        return HIK_DOWNSMP_INTERFACE_E_INPUT;
    }

    if (hor_mul >= ver_mul)
    {
        HIKDS_downsmp_frame_horpad_NV12(param);
    }
    else
    {
        HIKDS_downsmp_frame_verpad_NV12(param);
    }

    return HIK_DOWNSMP_INTERFACE_S_OK;
}

/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
void HIKDS_fill_pad_ver_I420(unsigned char *dst, int pad_size, int height, int pitch)
{
    int i;
    unsigned char *locDst = dst;

    for ( i = 0; i < height; i++)
    {
        memset(locDst, LUMA_BLACK_VALUE, pad_size);
        locDst += pitch;
    }

    for (i = 0; i < height; i++)
    {
        memset(locDst, CHROMA_BLACK_VALUE, pad_size/2);
        locDst += (pitch/2);
    }

    locDst = dst + pitch - pad_size;
    for ( i = 0; i < height; i++)
    {
        memset(locDst, LUMA_BLACK_VALUE, pad_size);
        locDst += pitch;
    }

    locDst = dst + height * pitch + pitch /2 - pad_size /2;
    for (i = 0; i < height; i++)
    {
        memset(locDst, CHROMA_BLACK_VALUE, pad_size/2);
        locDst += (pitch/2);
    }
}

/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
void HIKDS_fill_pad_hor_I420(unsigned char *dst, int pad_size, int width, int height, int botpad_flag)
{
    int i;
    unsigned char* locDst = dst;
    int luma_size = width * height;
    unsigned char *pix_ptr = locDst;

    /****y****/
    for ( i = 0; i < pad_size; i++)
    {
        memset(pix_ptr, LUMA_BLACK_VALUE, width);
        pix_ptr += width;
    }

    pix_ptr = locDst + luma_size - pad_size*width;
    for ( i = 0; i < pad_size; i++)
    {
        memset(pix_ptr, LUMA_BLACK_VALUE, width);
        pix_ptr += width;
    }

    locDst += luma_size;
    pad_size >>= 1;
    locDst -= ((luma_size >> 1) - (pad_size * width)) * botpad_flag;

    #if 0
    /****u****/
    pix_ptr = locDst;
    for (i = 0; i < pad_size; i++)
    {
        memset(pix_ptr, CHROMA_BLACK_VALUE, (width / 2));
        pix_ptr += (width /2);
    }

    pix_ptr = locDst + (luma_size >> 2) - pad_size*(width/2);
    for (i = 0; i < pad_size; i++)
    {
        memset(pix_ptr, CHROMA_BLACK_VALUE, (width / 2));
        pix_ptr += (width /2);
    }


    locDst += (luma_size / 4);
    /****v****/
    pix_ptr = locDst;
    for (i = 0; i < pad_size; i++)
    {
        memset(pix_ptr, CHROMA_BLACK_VALUE, (width / 2));
        pix_ptr += (width /2);
    }

    pix_ptr = locDst + (luma_size >> 2) - pad_size*(width/2);
    for (i = 0; i < pad_size; i++)
    {
        memset(pix_ptr, CHROMA_BLACK_VALUE, (width / 2));
        pix_ptr += (width /2);
    }
    #endif

}

void HIKDS_downsmp_proc_I420( unsigned char *dst,
                             unsigned char *src,
                             int dst_width,
                             int dst_height,
                             int dst_pitch,
                             int src_pitch,
                             float scale )
{
    int i, j;
    int smp_stride = (int)(scale * 1024);
    int smp_offset = (int)(scale / 2);
    int src_inc;

    unsigned char *src_line;
    unsigned char *dst_pix, *src_pix;

    src += smp_offset * src_pitch;

    for (i = 0; i < dst_height; i++)
    {
        src_line = src + ((smp_stride * i) >> 10) * src_pitch;
        dst_pix  = dst + i * dst_pitch;
        src_pix  = src_line;
        src_inc  = 0;

        for (j = 0; j < dst_width; j++)
        {
            //dst_pix[j] = downsmp_one_value(src_pix, smp_len, src_pitch);

            dst_pix[j] = (src_pix[0] + src_pix[smp_offset] + 1) >> 1; // 采用简单的抽点下采样方式来实现

            src_inc += smp_stride;
            src_pix  = src_line + (src_inc >> 10);
        }
    }
}


/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2015/12/7
******************************************************************************/
void HIKDS_downsmp_proc_uv_I420( unsigned char *dst_chroma,
                                unsigned char *src_chroma,
                                int dst_width,
                                int dst_height,
                                int dst_pitch,
                                int src_pitch,
                                float scale )
{
    int i, j;
    int ave_chroma;
    int smp_stride = (int)(scale * 1024);
    int smp_offset = (int)(scale / 2);
    int src_inc;

    unsigned char *src_line;
    unsigned char *dst_pix, *src_pix;

    src_chroma += smp_offset * src_pitch;

    for (i = 0; i < dst_height; i++)
    {
        src_line = src_chroma + ((smp_stride * i) >> 10) * src_pitch;
        dst_pix  = dst_chroma + i * dst_pitch;
        src_pix  = src_line;
        src_inc  = 0;

        for (j = 0; j < dst_width; j++)
        {
            ave_chroma  = ((src_pix[0] & 0xff) + (src_pix[smp_offset] & 0xff) + 1) >> 1;   // v数据处理
            dst_pix[j] = ave_chroma ;                     // 采用简单的抽点下采样方式来实现
            src_inc += smp_stride;
            src_pix  = src_line + (src_inc >> 10);
        }
    }
}


/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
HRESULT HIKDS_downsmp_frame_horpad_I420(DOWNSMP_PARAM *param)
{
	float aspect_ratio = (float)param->src_width / param->src_height;
    float scale        = (float)param->src_width / param->dst_width;

    int img_width     = param->dst_width;
    int img_height    = (int)(img_width / aspect_ratio);
    int pad_size      = (param->dst_height - img_height) >> 1;
    int src_luma_size = (param->src_height + param->padH) * (param->src_pitch);
    int dst_luma_size = param->dst_height * param->dst_width;
    unsigned char *dst_buf, *src_buf,*dstU_buf,*dstV_buf,*srcU_buf,*srcV_buf;

    if (param->src_height * param->dst_width == param->src_width * param->dst_height)
    {
        img_height = param->dst_height;
        pad_size   = 0;
    }

    pad_size    &= 0xfffffffe;
    img_height  &= 0xfffffffe;

    dst_buf = param->dst_buf;

    HIKDS_fill_pad_hor_I420(dst_buf, pad_size, param->dst_width, param->dst_height, 0);

    dst_buf = param->dst_buf + pad_size * param->dst_width;
    src_buf = param->src_buf;

    // y数据下采样
    HIKDS_downsmp_proc_I420( dst_buf,
        src_buf,
        img_width,
        img_height,
        param->dst_width,
        param->src_pitch,
        scale );//对亮度进行下采样


    #if 1
    // u数据下采样
    dstU_buf = param->dst_buf + dst_luma_size + (pad_size >> 1) * (img_width>>1);
    srcU_buf = param->src_buf + src_luma_size;
    HIKDS_downsmp_proc_uv_I420( (unsigned char *)dstU_buf,
        (unsigned char *)srcU_buf,
        img_width>>1,
        img_height>>1,
        param->dst_width>>1,
        param->src_pitch>>1,
        scale );//对两个色度进行下采样

    // v数据下采样
    dstV_buf = param->dst_buf + ((dst_luma_size * 5) >> 2) + (pad_size >> 1) * (img_width >> 1);
    srcV_buf = param->src_buf + ((src_luma_size * 5) >> 2);

    HIKDS_downsmp_proc_uv_I420( (unsigned char *)dstV_buf,
        (unsigned char *)srcV_buf,
        img_width>>1,
        img_height>>1,
        param->dst_width>>1,
        param->src_pitch>>1,
        scale );

    dst_buf  = param->dst_buf + (pad_size + img_height) * img_width;
    pad_size = param->dst_height - img_height - pad_size;
    #endif
    //HIKDS_fill_pad_hor_I420(dst_buf, pad_size, param->dst_width, param->dst_height, 1);

    return HIK_DOWNSMP_INTERFACE_S_OK;
}

/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
HRESULT HIKDS_downsmp_frame_verpad_I420(DOWNSMP_PARAM *param)
{
	float aspect_ratio = (float)param->src_width / param->src_height;
    float scale        = (float)param->src_height / param->dst_height;

    int img_height    = param->dst_height;
    int img_width     = (int)(img_height * aspect_ratio);
    int pad_size      = (param->dst_width - img_width) >> 1;
    int src_luma_size = (param->src_height + param->padH) * (param->src_pitch);
    int dst_luma_size = param->dst_height * param->dst_width;
    unsigned char *dst_buf, *src_buf,*dstU_buf,*dstV_buf,*srcU_buf,*srcV_buf;

    pad_size   &= 0xfffffffe;
    img_width  &= 0xfffffffe;

    dst_buf = param->dst_buf;

    HIKDS_fill_pad_ver_I420(dst_buf, pad_size, img_height, param->dst_width);//对左边的黑框进行填充

    dst_buf = param->dst_buf + pad_size;
    src_buf = param->src_buf;

    HIKDS_downsmp_proc_I420( dst_buf,
        src_buf,
        img_width,
        img_height,
        param->dst_width,
        param->src_pitch,
        scale );//对亮度进行下采样

    #if 1
    // u数据下采样
    dstU_buf = param->dst_buf + dst_luma_size + (pad_size >> 1);
    srcU_buf = param->src_buf + src_luma_size;

    HIKDS_downsmp_proc_uv_I420( (unsigned char *)dstU_buf,
        (unsigned char *)srcU_buf,
        img_width>>1,
        img_height>>1,
        param->dst_width>>1,
        param->src_pitch>>1,
        scale );//对两个色度进行下采样

    dstV_buf = param->dst_buf + ((dst_luma_size * 5) >> 2) + (pad_size >> 1);
    srcV_buf = param->src_buf + ((src_luma_size * 5) >> 2);

    HIKDS_downsmp_proc_uv_I420( (unsigned char *)dstV_buf,
        (unsigned char *)srcV_buf,
        img_width>>1,
        img_height>>1,
        param->dst_width>>1,
        param->src_pitch>>1,
        scale );
    #endif
    //dst_buf  = param->dst_buf   + pad_size  + img_width;
    //pad_size = param->dst_width - img_width - pad_size;
    //HIKDS_fill_pad_ver_NV12(dst_buf, pad_size, img_height, param->dst_width);//对右边的黑框进行填充

    return HIK_DOWNSMP_INTERFACE_S_OK;
}

/*输出dstW 要按照4对齐来输出*/
HRESULT HIKDS_DownSmpOneFrame_I420(DOWNSMP_PARAM *param)
{
    
    if (param == NULL)
    {
        return HIK_DOWNSMP_INTERFACE_E_PARA_NULL;
    }
	
	int src_width  = param->src_width;
    int src_height = param->src_height;
    int dst_width  = param->dst_width;
    int dst_height = param->dst_height;
    float hor_mul = (float)src_width/dst_width;
    float ver_mul = (float)src_height/dst_height;

    if (param->src_buf == NULL || param->dst_buf == NULL)
    {
        printf("11111\n");
        return HIK_DOWNSMP_INTERFACE_E_INPUT;
    }

    //只支持下采样的方式
    if (src_height < dst_height || src_width < dst_width)
    {
        printf("2222\n");
        return HIK_DOWNSMP_INTERFACE_E_INPUT;
    }

    //输入图像的大小必须为4的整数倍
    if ((src_height | src_width) & 3)
    {
        printf("3333\n");
        return HIK_DOWNSMP_INTERFACE_E_INPUT;
    }

    //输出图像的大小必须为2的整数倍
    if ((dst_height | dst_width) & 1)
    {
        printf("4444\n");
        return HIK_DOWNSMP_INTERFACE_E_INPUT;
    }

    if (hor_mul >= ver_mul)
    {
        HIKDS_downsmp_frame_horpad_I420(param);
    }
    else
    {
        HIKDS_downsmp_frame_verpad_I420(param);
    }

    return HIK_DOWNSMP_INTERFACE_S_OK;
}

/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
void HIKDS_fill_pad_ver_Y(unsigned char *dst, int pad_size, int height, int pitch)
{
    int i;
    unsigned char *locDst = dst;

    for ( i = 0; i < height; i++)
    {
        memset(locDst, LUMA_BLACK_VALUE, pad_size);
        locDst += pitch;
    }

    locDst = dst + pitch - pad_size;
    for ( i = 0; i < height; i++)
    {
        memset(locDst, LUMA_BLACK_VALUE, pad_size);
        locDst += pitch;
    }
}

/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
void HIKDS_fill_pad_hor_Y(unsigned char *dst, int pad_size, int width, int height, int botpad_flag)
{
    int i;
    unsigned char* locDst = dst;
    int luma_size = width * height;
    unsigned char *pix_ptr = locDst;

    /****y****/
    for ( i = 0; i < pad_size; i++)
    {
        memset(pix_ptr, LUMA_BLACK_VALUE, width);
        pix_ptr += width;
    }

    pix_ptr = locDst + luma_size - pad_size*width;
    for ( i = 0; i < pad_size; i++)
    {
        memset(pix_ptr, LUMA_BLACK_VALUE, width);
        pix_ptr += width;
    }
}

/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
HRESULT HIKDS_downsmp_frame_horpad_Y(DOWNSMP_PARAM *param)
{
    float aspect_ratio = (float)param->src_width / param->src_height;
    float scale        = (float)param->src_width / param->dst_width;

    int img_width     = param->dst_width;
    int img_height    = (int)(img_width / aspect_ratio);
    int pad_size      = (param->dst_height - img_height) >> 1;
    //int src_luma_size = param->src_height * param->src_width;
    //int dst_luma_size = param->dst_height * param->dst_width;
    unsigned char *dst_buf, *src_buf;

    if (param->src_height * param->dst_width == param->src_width *param->dst_height)
    {
        img_height = param->dst_height;
        pad_size   = 0;
    }

    pad_size    &= 0xfffffffe;
    img_height  &= 0xfffffffe;

    dst_buf = param->dst_buf;

    HIKDS_fill_pad_hor_Y(dst_buf, pad_size, param->dst_width, param->dst_height, 0);

    dst_buf = param->dst_buf + pad_size * param->dst_width;
    src_buf = param->src_buf;

    // y数据下采样
    HIKDS_downsmp_proc_I420( dst_buf,
        src_buf,
        img_width,
        img_height,
        param->dst_width,
        param->src_width,
        scale );//对亮度进行下采样

    return HIK_DOWNSMP_INTERFACE_S_OK;
}

/******************************************************************************
* 功  能：
* 参  数：
*
* 返回值：
* 备  注：
* 修  改: 2008/9/20
******************************************************************************/
HRESULT HIKDS_downsmp_frame_verpad_Y(DOWNSMP_PARAM *param)
{
    float aspect_ratio = (float)param->src_width / param->src_height;
    float scale        = (float)param->src_height / param->dst_height;

    int img_height    = param->dst_height;
    int img_width     = (int)(img_height * aspect_ratio);
    int pad_size      = (param->dst_width - img_width) >> 1;
    //int src_luma_size = param->src_height * param->src_width;
    //int dst_luma_size = param->dst_height * param->dst_width;
    unsigned char *dst_buf, *src_buf;

    pad_size   &= 0xfffffffe;
    img_width  &= 0xfffffffe;

    dst_buf = param->dst_buf;

    HIKDS_fill_pad_ver_Y(dst_buf, pad_size, img_height, param->dst_width);//对左边的黑框进行填充

    dst_buf = param->dst_buf + pad_size;
    src_buf = param->src_buf;

    HIKDS_downsmp_proc_I420( dst_buf,
        src_buf,
        img_width,
        img_height,
        param->dst_width,
        param->src_width,
        scale );//对亮度进行下采样


    return HIK_DOWNSMP_INTERFACE_S_OK;
}

/*输出dstW 要按照4对齐来输出*/
HRESULT HIKDS_DownSmpOneFrame_Y(DOWNSMP_PARAM *param)
{
    if (param == NULL)
    {
        return HIK_DOWNSMP_INTERFACE_E_PARA_NULL;
    }
	
    int src_width  = param->src_width;
    int src_height = param->src_height;
    int dst_width  = param->dst_width;
    int dst_height = param->dst_height;
    float hor_mul = (float)src_width/dst_width;
    float ver_mul = (float)src_height/dst_height;

    if (param->src_buf == NULL || param->dst_buf == NULL)
    {
        return HIK_DOWNSMP_INTERFACE_E_INPUT;
    }

    //只支持下采样的方式
    if (src_height < dst_height || src_width < dst_width)
    {
        return HIK_DOWNSMP_INTERFACE_E_INPUT;
    }

    //输入图像的大小必须为4的整数倍
    if ((src_height | src_width) & 3)
    {
        return HIK_DOWNSMP_INTERFACE_E_INPUT;
    }

    //输出图像的大小必须为2的整数倍
    if ((dst_height | dst_width) & 1)
    {
        return HIK_DOWNSMP_INTERFACE_E_INPUT;
    }

    if (hor_mul >= ver_mul)
    {
        HIKDS_downsmp_frame_horpad_Y(param);
    }
    else
    {
        HIKDS_downsmp_frame_verpad_Y(param);
    }

    return HIK_DOWNSMP_INTERFACE_S_OK;
}


