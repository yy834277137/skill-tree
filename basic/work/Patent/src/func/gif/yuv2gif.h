/**
 * @file   rgb2gif.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  对外接口
 * @author 
 * @date   2020/10/22
 * @note :
 * @note \n History:
   1.Date        : 2019/10/22
     Author      : 
     Modification: Created file
 */
#ifndef __YUV2GIF_H__
#define __YUV2GIF_H__

#include "gif_lib.h"

/**
 * @struct YUV_ATTR_S
 * @brief  YUV属性，目前为节约内存，仅支持NV21，且仅支持一帧
 */
typedef struct
{
    UINT32 uiWidth;   /*宽*/
    UINT32 uiHeight;  /*高*/
    UINT32 u32Stride; /*yuv数据的跨距*/
    UINT32 uiType;    /*类型，暂未使用*/
    UINT64 u64PhyAddr;        /*物理地址*/
    //ATTRIBUTE void *pVirAddr; /*虚拟地址*/
    void *pVirAddr; /*虚拟地址     此处为 代码重构替换平台为编译通过修改，若有问题可尝试上一个*/
}YUV_ATTR_S;

/**
 * @struct RGB_ATTR_S
 * @brief  RGB属性
 */
typedef struct
{
    UINT32 uiWidth;     /*宽*/
    UINT32 uiHeight;    /*高*/
    UINT32 uiType;      /*类型，暂未使用*/
    U08* pRgb24Buf[3];  /*RGB24通道，R,G,B分别存放*/
    U08* pRgb8Buf;      /*RGB8通道*/
    UINT64 u64PhyAddr;  /* yuv转换成rgb格式时存储rgb数据的物理地址*/
}RGB_ATTR_S;

/**
 * @struct GIF_ATTR_S
 * @brief  GIF属性
 */
typedef struct
{
    UINT32 uiWidth;    /*宽*/
    UINT32 uiHeight;   /*高*/
    UINT32 uiWriteId;  /*写id，用于gif编码过程*/
    UINT32 uiReadId;   /*读id，用于gif解码过程，暂未使用*/
    UINT32 uiSizeMax;  /*存储空间最大size*/
    U08* pGifBuf;      /*GIF数据*/
}GIF_ATTR_S;

//int yuv_to_gif(char *yuvbuf, int w, int h, char *outfile);

INT32 Gif_DrvYuv2Gif(YUV_ATTR_S *pstYuvStr, RGB_ATTR_S *pstRgbAttr, GIF_ATTR_S *pstGifAttr);
INT32 Gif_DrvRgb2Gif(RGB_ATTR_S *pstRgbAttr, GIF_ATTR_S *pstGifAttr);

#endif

