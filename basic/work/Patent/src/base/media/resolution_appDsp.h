/**
 * @file   resolution_appDsp.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief   分辨率的值 DSP与应用保持一致 由应用提供给我们，任何人不得随意修改
 * @author
 * @date
 * @note
 * @note \n History
   1.Date        : 2021.8.19
     Author      : yeyanzhong
     Modification: 分辨率的宏定义值从system_prm.h移到base/media/resolution_appDsp.h，供各个模块使用;同时也为了func下的模块与system_prm解耦
 */

#ifndef _RESOLUTION_APPDSP_H_
#define _RESOLUTION_APPDSP_H_

/*****************************************************************************
                                宏定义
*****************************************************************************/



/**********************************************************************************/
/* 此分辨率的值 与应用 与 SDK 保持一致 由应用提供给我们，任何人不得随意修改 */
#define CIF_FORMAT				0x00001001
#define QCIF_FORMAT				0x00001002
#define FCIF_FORMAT				0x00001003   /*add 2002-6-25*/
#define HCIF_FORMAT				0x00001004
#define QQCIF_FORMAT			0x00001005   /* PAL: 96*80 NTSC: 96*64*/
#define NCIF_FORMAT				0x00001006   /* 320*240*/
#define QNCIF_FORMAT			0x00001007   /* 160*120*/
#define DCIF_FORMAT				0x00001008   /* PAL:528*384 NTSC:528*320 */
#define D1_FORMAT				0x0000100F   /* 720*576 仅测试用 2003-12-25 */
#define VGA_FORMAT				0x00001010   /* VGA:640*480，PAL和NTSC相同 */
#define Q2CIF_FORMAT			0x00001009
#define QQ720P_FORMAT			0x0000100A  /*320*192 (1/16 720p) added 20090306*/
#define RES_320_180				0x0000100B   /*320*180 added 20091217*/
		
#define UXGA_FORMAT				0x00001011   /* UXGA:1600*1200 added  2007-4-1*/
#define SVGA_FORMAT				0x00001012  /*800*600 added  2007-12-12*/
#define HD720p_FORMAT			0x00001013  /*1280*720 added  2007-12-12*/
		
		
#define XVGA_FORMAT				0x00001014  /*1280x960 added  2008-12-17*/
#define HD900p_FORMAT			0x00001015  /*1600x900 added  2008-12-17*/
#define SXGAp_FORMAT			0x00001016  /*1360*1024 added  2009-05-13*/

#define HD_H_FORMAT				0x00001017 /*前端如果进来的是1920*1080,则该分辨率为
                                              1920*540,如果前端进来时720p，则进行1280*360 added 20090712*/
#define HD_D_FORMAT				0x00001018 /*前端如果进来的是1920*1080,则该分辨率为
                                              1440*720,如果前端进来时720p，则进行960*480 added 20090712*/
#define HD_Q_FORMAT				0x00001019 /*前端如果进来的是1920*1080,则该分辨率为
                                              60*540,如果前端进来时720p，则进行640*360 added 20090712*/
#define HD_QQ_FORMAT			0x0000101a /*前端如果进来的是1920*1080,则该分辨率为
                                              480*270,如果前端进来时720p，则进行320*180 added 20090712*/
#define HD1080i_FORMAT			0x0000101b /*1920*1080i added 200904010*/
#define HD1080p_FORMAT			0x0000101b /*注意和1920*1080i一样!! 以后修正, 1920*1080p*/
#define RES_2560_1920_FORMAT	0x0000101c /*2560*1920  added 2009-10-19*/
#define RES_1600_304_FORMAT		0x0000101d /*1600*304  added 2009-10-30*/
#define RES_2048_1536_FORMAT	0x0000101e /*2048*1536  added 2009-12-23*/
#define RES_2448_2048_FORMAT	0x0000101f /* 2448*2048 added 2010-01-06 */
#define XGA_FORMAT				0x00001020       /* 1024*768 */
#define RES_1440_900_FORMAT		0x00001021       /* 1440*900 */
#define SXGA_FORMAT				0x00001022       /* 1280x1024 */



/* #define SXGA_FORMAT            0x00001023        / * 1280x1024 * / */

#define RES_2208_1242_FORMAT	0x00001024
#define RES_2304_1296_FORMAT	0x00001025
#define RES_2304_1536_FORMAT	0x00001026

#define RES_720_1280_FORMAT		0x000010a7
#define RES_1080_1920_FORMAT	0x000010ac
#define RES_1080_720_FORMAT		0x000010d7
#define RES_360_640_FORMAT		0x000010d8
#define RES_1366_768_FORMAT		0x000010d9


#endif

