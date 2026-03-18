/***
 * @file   bmp_info.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  bmp header
 * @author liwenbin
 * @date   2022-02-24
 * @note
 * @note History:
 */
#ifndef _BMP_INFO_H_
#define _BMP_INFO_H_

/* bmp image header 3*2+2*4=14 bytes */
typedef struct tagBITMAPFILEHEADER
{
    unsigned short bfType;                                          /* "BM" */
    unsigned int bfSize;                                        /* Size of file in bytes */
    unsigned short bfReserved1;                                     /* set to 0 */
    unsigned short bfReserved2;                                     /* set to 0 */
    /* Byte offset to actual bitmap data (= 54 if RGB) */
    unsigned int bfOffBits;
} __attribute__((packed)) BITMAPFILEHEADER;

/* bmp image info 2*2+9*4=40 bytes */
typedef struct tagBITMAPINFOHEADER
{
    /* Size of BITMAPINFOHEADER, in bytes (= 40) */
    unsigned int biSize;
    /* Width of image, in pixels */
    unsigned int biWidth;
    /* Height of images, in pixels */
    unsigned int biHeight;
    /* Number of planes in target device (set to 1) */
    unsigned short biPlanes;
    /* Bits per pixel (24 in this case) */
    unsigned short biBitCount;
    /* Type of compression (0 if no compression) */
    unsigned int biCompression;
    /* Image size, in bytes (0 if no compression) */
    unsigned int biSizeImage;
    /* Resolution in pixels/meter of display device */
    unsigned int biXPelsPerMeter;
    /* Resolution in pixels/meter of display device */
    unsigned int biYPelsPerMeter;
    /* Number of colors in the color table (if 0, use maximum allowed by biBitCount) */
    unsigned int biClrUsed;
    /* Number of important colors.  If 0, all colors are important */
    unsigned int biClrImportant;
} __attribute__((packed)) BITMAPINFOHEADER;

#endif /* _BMP_INFO_H_ */

