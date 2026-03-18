/***********************************************************************
* 
* 版权信息：Copyright (c) 2017, 杭州海康威视数字技术股份有限公司
* 
* 文件名称：hik_downsmp.h
* 文件标识：HIKVISION_GENERAL_DOWN_SAMPLE_HIK_DOWNSMP_H_
* 摘    要：海康威视下采样接口头文件
*
************************************************************************
*/

#ifndef _HIK_DOWNSMP_INTERFACE_HIK_DOWNSMP_H_
#define _HIK_DOWNSMP_INTERFACE_HIK_DOWNSMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HIK_DOWNSMP_INTERFACE_VERSION            1.1
#define HIK_DOWNSMP_INTERFACE_DATE                0x20130604

#define HIK_DOWNSMP_INTERFACE_S_OK                1            /* 下采样成功 */
#define HIK_DOWNSMP_INTERFACE_S_FAIL            0            /* 下采样失败 */

#define HIK_DOWNSMP_INTERFACE_E_INPUT            0x80000000    /* 输入下采样参数错误 */    
#define HIK_DOWNSMP_INTERFACE_E_PARA_NULL        0x80000001    /* 参数指针为空 */

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
    typedef int HRESULT;
#endif 


//下采样结构体
typedef struct _DOWNSMP_PARAM_STR_ 
{
    unsigned char *src_buf;        //输入图像缓存地址
    unsigned int  src_width;    //输入图像的宽度, 必须为4的整数倍
    unsigned int  src_height;   //输入图像高度, 必须为4的整数倍
    int           src_pitch;    //跨距
    int           padH;         //4字节对齐后多余的高

    unsigned char *dst_buf;        //输出图像缓存地址
    unsigned int  dst_width;    //输出图像的宽度, 必须为4的整数倍
    unsigned int  dst_height;    //输出图像高度, 必须为4的整数倍

}DOWNSMP_PARAM;
 
/******************************************************************************
* 功  能：对一个YUV帧进行下采样，输出QCIF格式YUV帧
* 参  数：param                -  下采样参数结构体指针        [IN:OUT]

* 返回值：成功返回            -    HIK_DOWNSMP_INTERFACE_S_OK
*          输入参数空返回    -   HIK_DOWNSMP_INTERFACE_E_PARA_NULL
*          输入参数错误返回    -    HIK_DOWNSMP_INTERFACE_E_INPUT
*          下采样失败返回    -    HIK_DOWNSMP_INTERFACE_S_FAIL
* 备  注：因为是下采样，所以输入图像的长宽必须是 大于或者等于输出图像的长宽
* 修  改: 2008/9/16
******************************************************************************/
HRESULT HIKDS_DownSmpOneFrame(DOWNSMP_PARAM *param);



/******************************************************************************
* 功  能：对一个YUV帧进行下采样，输出QCIF格式YUV帧
* 参  数：param                -  下采样参数结构体指针        [IN:OUT]
*         
* 返回值：成功返回            -    HIK_DOWNSMP_INTERFACE_S_OK
*          输入参数空返回    -   HIK_DOWNSMP_INTERFACE_E_PARA_NULL
*          输入参数错误返回    -    HIK_DOWNSMP_INTERFACE_E_INPUT
*          下采样失败返回    -    HIK_DOWNSMP_INTERFACE_S_FAIL
* 备  注：因为是下采样，所以输入图像的长宽必须是 大于或者等于输出图像的长宽
* 修  改: 2008/9/16
******************************************************************************/
HRESULT HIKDS_DownSmpOneFrame_NV12(DOWNSMP_PARAM *param);


HRESULT HIKDS_DownSmpOneFrame_I420(DOWNSMP_PARAM *param);

HRESULT HIKDS_DownSmpOneFrame_Y(DOWNSMP_PARAM *param);


#ifdef __cplusplus
}
#endif

#endif



