/*
 * @Copyright: HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
 * @file name: xsp_mod_tc_enhance.cpp
 * @Data: 2024-05-14
 * @LastEditor: sunguojian5
 * @LastData: 2024-11-12
 * @Describe: 图像方案-测试体增强模块
 */

 #include "xsp_mod_tc_enhance.hpp"
 #include <string>
 
 using namespace std;
 
 /*-------------------------------------Isl图像通用处理函数---------------------------------------*/
 /**
  * @function   Isl_ximg_get_bpp
  * @brief      获取不同格式图像每个像素点占用内存的字节数
  * @param[IN]  ISL_IMG_DATFMT format       图像格式
  * @param[IN]  int32_t          u32PlaneIdx  多平面存储图像的图像平面索引
  * @return     int32_t  ISL_FAIL 失败 ISL_SOK  成功
  */
 static int32_t Isl_ximg_get_bpp(ISL_IMG_DATFMT format, int32_t u32PlaneIdx)
 {
	 int32_t bpp = 0;
	 switch (format)
	 {
	 case ISL_IMG_DATFMT_YUV420SP_UV:
	 case ISL_IMG_DATFMT_YUV420SP_VU:
		 bpp = (1 == u32PlaneIdx) ? 2 : 1; /* UV平面按单像素占两字节计算，宽高均为Y平面一半 */
		 break;
	 case ISL_IMG_DATFMT_BGRP:
	 case ISL_IMG_DATFMT_BGRAP:
	 case ISL_IMG_DATFMT_SP8:
		 bpp = 1;
		 break;
	 case ISL_IMG_DATFMT_BGR24:
		 bpp = 3;
		 break;
	 case ISL_IMG_DATFMT_BGRA32:
		 bpp = 4;
		 break;
	 case ISL_IMG_DATFMT_SP16:
	 case ISL_IMG_DATFMT_LHP:
	 case ISL_IMG_DATFMT_LHZP:
		 bpp = (2 == u32PlaneIdx) ? 1 : 2; /* Z平面按单像素占单字节 */
		 break;
	 case ISL_IMG_DATFMT_LHZ16P:
		 bpp = 2;
		 break;
	 default:
		 log_info("Unsupported image format 0x%x, plane %d\n", format, u32PlaneIdx);
		 bpp = -1;
	 }
 
	 return bpp;
 }
 
 /**
  * @function   Isl_ximg_get_size
  * @brief      获取不同格式图像数据尺寸
  * @param[IN]  XIMAGE_DATA_ST *pstImg        输入图像
  * @param[OUT] uint32_t         *pu32DataSize  输出的图像数据尺寸
  * @return     int32_t  ISL_FAIL 失败 ISL_SOK  成功
  */
 int32_t Isl_ximg_get_size(XIMAGE_DATA_ST* pstImg, uint32_t* pu32DataSize)
 {
	 uint32_t u32ImgHeight = 0;
	 uint32_t u32ImgStride = 0;
	 ISL_IMG_DATFMT enDataFmt = ISL_IMG_DATFMT_UNKNOWN;
 
	 CHECK_PTR_IS_NULL(pstImg, ISL_FAIL);
	 CHECK_PTR_IS_NULL(pu32DataSize, ISL_FAIL);
 
	 enDataFmt = pstImg->enImgFmt;
	 u32ImgHeight = pstImg->u32Height;
	 u32ImgStride = pstImg->u32Stride[0];
 
	 switch (enDataFmt)
	 {
	 case ISL_IMG_DATFMT_YUV420SP_UV:
	 case ISL_IMG_DATFMT_YUV420SP_VU:
	 case ISL_IMG_DATFMT_YUV420P:
		 *pu32DataSize = u32ImgStride * u32ImgHeight * 3 / 2;
		 break;
	 case ISL_IMG_DATFMT_YUV422_MASK:
		 *pu32DataSize = u32ImgStride * u32ImgHeight * 2;
		 break;
	 case ISL_IMG_DATFMT_BGRA16_1555:
		 *pu32DataSize = u32ImgStride * u32ImgHeight * 2;
		 break;
	 case ISL_IMG_DATFMT_BGRP:
	 case ISL_IMG_DATFMT_BGR24:
		 *pu32DataSize = u32ImgStride * u32ImgHeight * 3;
		 break;
	 case ISL_IMG_DATFMT_BGRA32:
	 case ISL_IMG_DATFMT_BGRAP:
		 *pu32DataSize = u32ImgStride * u32ImgHeight * 4;
		 break;
	 case ISL_IMG_DATFMT_SP16:
		 *pu32DataSize = u32ImgStride * u32ImgHeight * 2;
		 break;
	 case ISL_IMG_DATFMT_SP8:
		 *pu32DataSize = u32ImgStride * u32ImgHeight;
		 break;
	 case ISL_IMG_DATFMT_LHP:
		 *pu32DataSize = u32ImgStride * u32ImgHeight * 4;
		 break;
	 case ISL_IMG_DATFMT_LHZP:
		 *pu32DataSize = u32ImgStride * u32ImgHeight * 5;
		 break;
	 case ISL_IMG_DATFMT_LHZ16P:
		 *pu32DataSize = u32ImgStride * u32ImgHeight * 6;
		 break;
	 case ISL_IMG_DATFMT_MASK:
		 *pu32DataSize = u32ImgStride * u32ImgHeight * 4;
		 break;
	 default:
		 *pu32DataSize = 0;
		 log_info("Not supported image format[0x%x]\n", enDataFmt);
		 return ISL_FAIL;
	 }
 
	 return ISL_OK;
 }
 
 int32_t Isl_ximg_fill_color(XIMAGE_DATA_ST* pstImageData, uint32_t u32StartRow, uint32_t u32FillHeight, uint32_t u32StartCol, uint32_t u32FillWidth, uint32_t u32BgValue)
 {
	 uint8_t u8YValue = 0, u8UValue = 0, u8VValue = 0;
	 int32_t bpp = 0;
	 uint32_t i = 0, j = 0, u32Offset = 0, u32PixelNum = 0;
	 uint32_t u32RawVal = 0;
	 uint32_t u32ImgWidth = 0, u32ImgHeight = 0, u32ImgStride = 0;
	 uint32_t u32EndRow = 0, u32EndCol = 0; // 填充结束行，不包含自身行
	 uint32_t step = 0;                     // 数据行步长，单位：字节
	 uint32_t u32LineDataByte = 0;          // 按行字节数
	 uint8_t* pu8Y = NULL, * pu8UV = NULL;
	 uint16_t* pu16 = NULL, u16Value = 0;
	 uint32_t* pRgb32Addr = NULL;
	 void* pSrcDataVir = NULL;
 
	 struct rgb24
	 {
		 uint8_t r;
		 uint8_t g;
		 uint8_t b;
	 };
	 struct rgb24 stRgb24 = { 0, 0, 0 };
	 struct rgb24* pRgb24Addr = NULL;
 
	 CHECK_PTR_IS_NULL(pstImageData, ISL_FAIL);
	 CHECK_PTR_IS_NULL(pstImageData->pPlaneVir[0], ISL_FAIL);
	 if (ISL_IMG_DATFMT_YUV420_MASK == pstImageData->enImgFmt)
	 {
		 CHECK_PTR_IS_NULL(pstImageData->pPlaneVir[1], ISL_FAIL);
	 }
 
	 u32ImgWidth = pstImageData->u32Width;
	 u32ImgHeight = pstImageData->u32Height;
	 u32ImgStride = pstImageData->u32Stride[0];
	 u32EndRow = u32StartRow + u32FillHeight;
	 u32EndCol = u32StartCol + u32FillWidth;
	 if (0 == u32ImgWidth || 0 == u32ImgStride || u32ImgWidth > u32ImgStride ||
		 u32StartRow + u32FillHeight > u32ImgHeight || u32StartCol + u32FillWidth > u32ImgWidth)
	 {
		 log_info("the input parameters are invalid, w:%u, h:%u, s:%u, start row:%u, fill height:%u, start col:%u, fill width:%u\n",
			 u32ImgWidth, u32ImgHeight, u32ImgStride, u32StartRow, u32FillHeight, u32StartCol, u32FillWidth);
		 return ISL_FAIL;
	 }
 
	 if (ISL_IMG_DATFMT_BGR24 == pstImageData->enImgFmt)
	 {
		 stRgb24.r = XIMG_32BIT_R_VAL(u32BgValue);
		 stRgb24.g = XIMG_32BIT_G_VAL(u32BgValue);
		 stRgb24.b = XIMG_32BIT_B_VAL(u32BgValue);
		 if (u32ImgWidth == u32ImgStride && u32ImgWidth == u32FillWidth) // 内存宽度等于stride时，减少for循环,加快速度
		 {
			 /* rgb */
			 pRgb24Addr = (struct rgb24*)pstImageData->pPlaneVir[0] + u32StartRow * u32ImgWidth;
			 u32PixelNum = u32FillHeight * u32ImgWidth; /*填充像素数*/
			 for (i = 0; i < u32PixelNum; i++)
			 {
				 *pRgb24Addr = stRgb24;
				 pRgb24Addr++;
			 }
		 }
		 else
		 {
			 for (i = u32StartRow; i < u32EndRow; i++)
			 {
				 pRgb24Addr = (struct rgb24*)pstImageData->pPlaneVir[0] + i * u32ImgStride + u32StartCol;
				 for (j = 0; j < u32FillWidth; j++)
				 {
					 *pRgb24Addr = stRgb24;
					 pRgb24Addr++;
				 }
			 }
		 }
	 }
	 else if (ISL_IMG_DATFMT_BGRA32 == pstImageData->enImgFmt)
	 {
		 if (u32ImgWidth == u32ImgStride && u32ImgWidth == u32FillWidth) // 内存宽度等于stride时，减少for循环,加快速度
		 {
			 /* argb */
			 pRgb32Addr = (uint32_t*)pstImageData->pPlaneVir[0] + u32StartRow * u32ImgWidth;
			 u32PixelNum = u32FillHeight * u32ImgWidth; /*填充像素数*/
			 for (i = 0; i < u32PixelNum; i++)
			 {
				 *pRgb32Addr = u32BgValue;
				 pRgb32Addr++;
			 }
		 }
		 else
		 {
			 for (i = u32StartRow; i < u32EndRow; i++)
			 {
				 pRgb32Addr = (uint32_t*)pstImageData->pPlaneVir[0] + i * u32ImgStride + u32StartCol;
				 for (j = 0; j < u32FillWidth; j++)
				 {
					 *pRgb32Addr = u32BgValue;
					 pRgb32Addr++;
				 }
			 }
		 }
	 }
	 else if (ISL_IMG_DATFMT_BGRP == pstImageData->enImgFmt)
	 {
		 for (j = 0; j < 3; j++) // 3 Planes, B - G - R
		 {
			 pu8Y = (uint8_t*)pstImageData->pPlaneVir[j];
			 u8YValue = (u32BgValue >> ((2 - j) * 8)) & 0xFF;
			 u32ImgStride = pstImageData->u32Stride[j];
			 if (u32ImgWidth == u32ImgStride && u32ImgWidth == u32FillWidth) // 内存宽度等于stride时，减少for循环，加快速度
			 {
				 pu8Y += u32StartRow * u32ImgStride;         /* 跳过前面u32StartRow行，开始覆盖 */
				 u32PixelNum = u32FillHeight * u32ImgStride; /* 填充像素数 */
				 memset(pu8Y, u8YValue, u32PixelNum);
			 }
			 else
			 {
				 for (i = u32StartRow; i < u32EndRow; i++)
				 {
					 u32Offset = i * u32ImgStride + u32StartCol;
					 memset(pu8Y + u32Offset, u8YValue, u32FillWidth);
				 }
			 }
		 }
	 }
	 else if (ISL_IMG_DATFMT_YUV420SP_VU == pstImageData->enImgFmt || ISL_IMG_DATFMT_YUV420SP_UV == pstImageData->enImgFmt)
	 {
		 pu8Y = (uint8_t*)pstImageData->pPlaneVir[0];
		 pu8UV = (uint8_t*)pstImageData->pPlaneVir[1];
		 u8YValue = XIMG_32BIT_Y_VAL(u32BgValue);
		 u8UValue = XIMG_32BIT_U_VAL(u32BgValue);
		 u8VValue = XIMG_32BIT_V_VAL(u32BgValue);
		 if (u32ImgWidth == u32ImgStride && u32ImgWidth == u32FillWidth) // 内存宽度等于stride时，减少for循环,加快速度
		 {
			 /* Y */
			 pu8Y += u32StartRow * u32ImgStride;         /* 跳过前面u32StartRow行，开始覆盖 */
			 u32PixelNum = u32FillHeight * u32ImgStride; /*填充像素数*/
			 memset(pu8Y, u8YValue, u32PixelNum);
 
			 /* UV */
			 pu8UV += u32StartRow * u32ImgStride >> 1; /*色度地址*/
			 u32PixelNum >>= 1;                        /*数据个数*/
			 if (u8UValue == u8VValue)                 /* U、V相等的情况 */
			 {
				 memset(pu8UV, u8UValue, u32PixelNum);
			 }
			 else /* U、V不相等 */
			 {
				 for (i = 0; i < u32PixelNum / 2; i++)
				 {
					 *pu8UV = u8VValue; /* V */
					 pu8UV++;
					 *pu8UV = u8UValue; /* U */
					 pu8UV++;
				 }
			 }
		 }
		 else
		 {
			 /* Y */
			 for (i = u32StartRow; i < u32EndRow; i++)
			 {
				 u32Offset = i * u32ImgStride + u32StartCol;
				 memset(pu8Y + u32Offset, u8YValue, u32FillWidth);
 
				 /* V,U */
				 if (i % 2 == 0)
				 {
					 u32Offset = ((i * u32ImgStride) >> 1) + u32StartCol;
					 if (u8UValue == u8VValue) /* U、V相等的情况 */
					 {
						 memset(pu8UV + u32Offset, u8UValue, u32FillWidth);
					 }
					 else /* U、V不相等 */
					 {
						 for (j = u32StartCol; j < u32EndCol; j += 2)
						 {
							 *(pu8UV + u32Offset + j) = u8VValue;
							 *(pu8UV + u32Offset + j + 1) = u8UValue;
						 }
					 }
				 }
			 }
		 }
	 }
	 else if (ISL_IMG_DATFMT_SP16 == pstImageData->enImgFmt)
	 {
		 pu16 = (uint16_t*)pstImageData->pPlaneVir[0];
		 u16Value = u32BgValue & 0xFFFF;
		 for (i = u32StartRow; i < u32EndRow; i++)
		 {
			 u32Offset = i * u32ImgStride + u32StartCol;
			 for (j = 0; j < u32FillWidth; j++)
			 {
				 *(pu16 + u32Offset + j) = u16Value;
			 }
		 }
	 }
	 else if (ISL_IMG_DATFMT_SP8 == pstImageData->enImgFmt)
	 {
		 pu8Y = (uint8_t*)pstImageData->pPlaneVir[0];
		 u8YValue = u32BgValue & 0xFF;
		 if (u32ImgWidth == u32ImgStride && u32ImgWidth == u32FillWidth) // 内存宽度等于stride时，减少for循环，加快速度
		 {
			 pu8Y += u32StartRow * u32ImgStride;         /* 跳过前面u32StartRow行，开始覆盖 */
			 u32PixelNum = u32FillHeight * u32ImgStride; /* 填充像素数 */
			 memset(pu8Y, u8YValue, u32PixelNum);
		 }
		 else
		 {
			 for (i = u32StartRow; i < u32EndRow; i++)
			 {
				 u32Offset = i * u32ImgStride + u32StartCol;
				 memset(pu8Y + u32Offset, u8YValue, u32FillWidth);
			 }
		 }
	 }
	 else if (ISL_IMG_DATFMT_LHZP == pstImageData->enImgFmt)
	 {
		 for (i = 0; i < ISL_XIMAGE_PLANE_MAX; i++)
		 {
			 pSrcDataVir = pstImageData->pPlaneVir[i];
			 if (NULL == pSrcDataVir)
			 {
				 break;
			 }
			 u32RawVal = 0xFF;
			 if (2 == i)
			 {
				 u32RawVal = 0x00;
			 }
 
			 bpp = Isl_ximg_get_bpp(pstImageData->enImgFmt, i);
			 if (0 > (int32_t)bpp)
			 {
				 log_info("Isl_ximg_get_bpp failed, fmt:0x%x, plane idx:%d, bpp:%d\n", pstImageData->enImgFmt, i, bpp);
				 return ISL_FAIL;
			 }
 
			 step = pstImageData->u32Stride[i] * bpp;
			 u32LineDataByte = u32ImgWidth * bpp;
			 for (j = 0; j < u32FillHeight; j++)
			 {
				 memset(pSrcDataVir, u32RawVal, u32LineDataByte);
				 pSrcDataVir = (int8_t*)pSrcDataVir + step;
			 }
		 }
	 }
	 else
	 {
		 log_info("Not supported data format[0x%x] for Isl_ximg_fill_color\n", pstImageData->enImgFmt);
		 return ISL_FAIL;
	 }
 
	 return ISL_OK;
 }
 
 /* 在原图内存基础上，创建另外一张图像 */
 int32_t Isl_ximg_create_sub(XIMAGE_DATA_ST* pstSrcImg, XIMAGE_DATA_ST* pstDstImg, ISL_VideoCrop* pstCropPrm)
 {
	 int32_t i = 0, bpp = 1;
	 uint32_t u32SrcStride = 0; // 源数据跨距，单位：像素
	 uint32_t u32MemoryOffset = 0;
	 ISL_VideoCrop stLocCropPrm = { 0, 0, 0, 0 };
 
	 CHECK_PTR_IS_NULL(pstSrcImg, ISL_FAIL);
	 CHECK_PTR_IS_NULL(pstDstImg, ISL_FAIL);
 
	 if (NULL == pstCropPrm)
	 {
		 stLocCropPrm.top = 0;
		 stLocCropPrm.left = 0;
		 stLocCropPrm.width = pstSrcImg->u32Width;
		 stLocCropPrm.height = pstSrcImg->u32Height;
	 }
	 else
	 {
		 memcpy(&stLocCropPrm, pstCropPrm, sizeof(ISL_VideoCrop));
	 }
 
	 /* 判断抠图区域是否超出图像范围 */
	 if ((stLocCropPrm.top + stLocCropPrm.height) > pstSrcImg->u32Height ||
		 (stLocCropPrm.left + stLocCropPrm.width) > pstSrcImg->u32Width)
	 {
		 log_info("crop size exceeds image size, src[%u x %u], crop[%u %u %u %u].\n",
			 pstSrcImg->u32Width, pstSrcImg->u32Height,
			 stLocCropPrm.left, stLocCropPrm.top, stLocCropPrm.width, stLocCropPrm.height);
		 return ISL_FAIL;
	 }
 
	 pstDstImg->enImgFmt = pstSrcImg->enImgFmt;
	 pstDstImg->u32Width = stLocCropPrm.width;
	 pstDstImg->u32Height = stLocCropPrm.height;
 
	 for (i = 0; i < ISL_XIMAGE_PLANE_MAX; i++)
	 {
		 if (NULL == pstSrcImg->pPlaneVir[i])
		 {
			 break; // 遇到空平面指针直接退出，不再处理后续平面
		 }
 
		 u32SrcStride = pstSrcImg->u32Stride[i];
		 bpp = Isl_ximg_get_bpp(pstSrcImg->enImgFmt, i);
		 if (0 > bpp)
		 {
			 log_info("Isl_ximg_get_bpp failed, fmt:0x%x, plane idx:%d, bpp:%d\n", pstSrcImg->enImgFmt, i, bpp);
			 return ISL_FAIL;
		 }
		 if ((ISL_IMG_DATFMT_YUV420SP_VU == pstSrcImg->enImgFmt || ISL_IMG_DATFMT_YUV420SP_UV == pstSrcImg->enImgFmt) && 1 == i)
		 {
			 u32SrcStride >>= 1;
			 stLocCropPrm.top >>= 1;
			 stLocCropPrm.left >>= 1;
		 }
		 u32MemoryOffset = (u32SrcStride * stLocCropPrm.top + stLocCropPrm.left) * bpp;
 
		 pstDstImg->u32Stride[i] = pstSrcImg->u32Stride[i];
		 pstDstImg->pPlaneVir[i] = (int8_t*)pstSrcImg->pPlaneVir[i] + u32MemoryOffset;
	 }
 
	 return ISL_OK;
 }
 
 /**
  * @function       Isl_ximg_copy_kernel
  * @brief          将源图像拷贝一份给目标图像，不做任何修改，不支持就地处理
  * @param[IN]      XIMAGE_DATA_ST   *pstSrcImg  源图像数据
  * @param[IN/OUT]  XIMAGE_DATA_ST   *pstDstImg  目标图像数据
  * @return         int32_t  ISL_FAIL 失败 ISL_SOK  成功
  */
 static int32_t Isl_ximg_copy_kernel(XIMAGE_DATA_ST* pstSrcImg, XIMAGE_DATA_ST* pstDstImg)
 {
	 int32_t i = 0, j = 0, bpp = 1;
	 uint32_t u32SrcWidth = 0, u32SrcHeight = 0, u32SrcStride = 0; // 源数据宽、高、跨距，单位：像素
	 uint32_t u32DstStride = 0;                                    // 目标数据跨距，单位：像素
	 uint32_t sstep = 0;                                           // 源数据行步长，单位：字节
	 uint32_t dstep = 0;                                           // 目标数据行步长，单位：字节
	 uint32_t u32LineDataCpyByte = 0;                              // 按行拷贝字节数
	 uint32_t u32AllDataCpyByte = 0;                               // 一次性拷贝字节数
	 void* pSrcDataVir = NULL;
	 void* pDstDataVir = NULL;
	 int32_t copyPlanes = ISL_XIMAGE_PLANE_MAX;
 
	 CHECK_PTR_IS_NULL(pstSrcImg, ISL_FAIL);
	 CHECK_PTR_IS_NULL(pstDstImg, ISL_FAIL);
 
	 if (pstSrcImg->enImgFmt != pstDstImg->enImgFmt)
	 {
		 if (pstSrcImg->enImgFmt >= ISL_IMG_DATFMT_LHP && pstSrcImg->enImgFmt <= ISL_IMG_DATFMT_LHZ16P &&
			 pstDstImg->enImgFmt >= ISL_IMG_DATFMT_LHP && pstDstImg->enImgFmt <= ISL_IMG_DATFMT_LHZ16P) // 格式不同，仅拷贝低高能
		 {
			 copyPlanes = 2;
		 }
		 else
		 {
			 printf("Only support ximg copy of same ximg format, src:0x%x, dst:0x%x\n", pstSrcImg->enImgFmt, pstDstImg->enImgFmt);
			 return ISL_FAIL;
		 }
	 }
 
	 u32SrcWidth = pstSrcImg->u32Width;
	 u32SrcHeight = pstSrcImg->u32Height;
 
	 for (i = 0; i < copyPlanes; i++)
	 {
		 /* 依次拷贝各个平面 */
		 pSrcDataVir = pstSrcImg->pPlaneVir[i];
		 pDstDataVir = pstDstImg->pPlaneVir[i];
		 if (NULL == pSrcDataVir || NULL == pDstDataVir)
		 {
			 break;
		 }
 
		 u32SrcStride = pstSrcImg->u32Stride[i];
		 u32DstStride = pstDstImg->u32Stride[i];
		 bpp = Isl_ximg_get_bpp(pstSrcImg->enImgFmt, i);
		 if (0 > bpp)
		 {
			 log_info("Isl_ximg_get_bpp failed, fmt:0x%x, plane idx:%d, bpp:%d\n", pstSrcImg->enImgFmt, i, bpp);
			 return ISL_FAIL;
		 }
		 /* UV平面按单像素占两字节计算，宽高均为Y平面一半 */
		 if ((ISL_IMG_DATFMT_YUV420SP_VU == pstSrcImg->enImgFmt || ISL_IMG_DATFMT_YUV420SP_UV == pstSrcImg->enImgFmt) && (1 == i)) // 1表示UV平面
		 {
			 u32SrcWidth >>= 1;
			 u32SrcHeight >>= 1;
			 u32SrcStride >>= 1;
			 u32DstStride >>= 1;
		 }
		 sstep = u32SrcStride * bpp;
		 dstep = u32DstStride * bpp;
		 u32LineDataCpyByte = u32SrcWidth * bpp;
 
		 if (u32SrcWidth == u32SrcStride && u32SrcStride == u32DstStride)
		 {
			 /* SrcWidth、SrcStride、DstStride三者相同，数据连续，直接拷贝 */
			 u32AllDataCpyByte = u32LineDataCpyByte * u32SrcHeight;
			 memmove(pDstDataVir, pSrcDataVir, u32AllDataCpyByte);
		 }
		 else
		 {
			 /* 数据不连续时，按行分别拷贝 */
			 for (j = 0; j < (int32_t)u32SrcHeight; j++)
			 {
				 memmove(pDstDataVir, pSrcDataVir, u32LineDataCpyByte);
				 pSrcDataVir = (int8_t*)pSrcDataVir + sstep;
				 pDstDataVir = (int8_t*)pDstDataVir + dstep;
			 }
		 }
	 }
 
	 return ISL_OK;
 }
 /**
  * @function   Isl_ximg_verify_size
  * @brief      校验图片宽高跨距是否合适，校验图片内存，图片宽高和格式所需内存不应超过实际buf最大内存
  * @param[IN]  XIMAGE_DATA_ST *pstImg 校验的图像
  * @param[IN]  const char     *_func_ 调用处函数名，用于异常信息打印
  * @param[IN]  const uint   _line_  调用处行号，用于异常信息打印
  * @return     int32_t  ISL_FAIL 校验不通过，图像超出内存范围 ISL_SOK 校验通过
  */
 static int32_t Isl_ximg_verify_size(XIMAGE_DATA_ST* pstImg, const char* _func_, const uint _line_)
 {
	 ISL_IMG_DATFMT enDataFmt = ISL_IMG_DATFMT_UNKNOWN;
	 uint32_t u32ImgWidth = 0;
	 uint32_t u32ImgHeight = 0;
	 uint32_t u32ImgStride = 0;
	 uint32_t u32ImgSize = 0;
 
	 CHECK_PTR_IS_NULL(pstImg, ISL_FAIL);
	 CHECK_PTR_IS_NULL(_func_, ISL_FAIL);
	 enDataFmt = pstImg->enImgFmt;
	 u32ImgWidth = pstImg->u32Width;
	 u32ImgHeight = pstImg->u32Height;
	 u32ImgStride = pstImg->u32Stride[0];
 
	 if (0 == u32ImgWidth || 0 == u32ImgHeight || 0 == u32ImgStride || u32ImgWidth > u32ImgStride)
	 {
		 log_info("ximg size is invalid, w:%u, h:%u, s:%u, fmt:0x%x, Func:%s|%u\n",
			 u32ImgWidth, u32ImgHeight, u32ImgStride, enDataFmt, _func_, _line_);
		 return ISL_FAIL;
	 }
 
	 if (ISL_OK != Isl_ximg_get_size(pstImg, &u32ImgSize))
	 {
		 log_info("Isl_ximg_get_size failed, fmt[0x%x], Func:%s|%u\n", enDataFmt, _func_, _line_);
		 return ISL_FAIL;
	 }
 
	 return ISL_OK;
 }
 
 /**
  * @function       Isl_ximg_crop
  * @brief          从源图像中抠取指定范围内的图像贴到目标图像的指定位置
  * @param[IN]      XIMAGE_DATA_ST *pstImageSrc   源图像数据
  * @param[IN/OUT]  XIMAGE_DATA_ST *pstImageDst   目标图像数据，为空时，在原图上操作
  * @param[IN]           enImgFmt  图像格式，见枚举ISL_IMG_DATFMT
  * @param[IN]           u32Width  目标图像宽，单位：Pixel
  * @param[IN]           u32Height 目标图像高，单位：Pixel
  * @param[IN]           u32Stride 内存跨距，单位：Byte，N个平面即N个元素有效
  * @param[OUT]          pPlaneVir 抠取完成后的目标图像数据
  * @param[IN/OUT]  ISL_VideoCrop *pstSrcCropPrm 源图像抠取范围，为空时表示抠取全图
  * @param[IN/OUT]  ISL_VideoCrop *pstDstCropPrm 目标图像中存放抠图位置，宽高与源图抠取宽高相同，为空时表示抠取全图，
  *                                              源抠取参数和目标抠取参数均为空时，原图和目标图片需相同尺寸
  * @return         int32_t  ISL_FAIL 失败 ISL_SOK  成功
  */
 int32_t Isl_ximg_crop(XIMAGE_DATA_ST* pstImageSrc, XIMAGE_DATA_ST* pstImageDst, ISL_VideoCrop* pstSrcCropPrm, ISL_VideoCrop* pstDstCropPrm)
 {
	 ISL_VideoCrop stLocSrcCropPrm;
	 ISL_VideoCrop stLocDstCropPrm;
	 XIMAGE_DATA_ST stSrcCropImg;
	 XIMAGE_DATA_ST stDstCropImg;
	 XIMAGE_DATA_ST* pstLocDstImg = NULL;
	 memset(&stLocSrcCropPrm, 0, sizeof(ISL_VideoCrop));
	 memset(&stLocDstCropPrm, 0, sizeof(ISL_VideoCrop));
	 memset(&stSrcCropImg, 0, sizeof(XIMAGE_DATA_ST));
	 memset(&stDstCropImg, 0, sizeof(XIMAGE_DATA_ST));
 
	 CHECK_PTR_IS_NULL(pstImageSrc, ISL_FAIL);
	 pstLocDstImg = pstImageDst;
	 if (NULL == pstImageDst)
	 {
		 pstLocDstImg = pstImageSrc;
	 }
	 if (ISL_OK != Isl_ximg_verify_size(pstImageSrc, __FUNCTION__, __LINE__))
	 {
		 return ISL_FAIL;
	 }
	 if (ISL_OK != Isl_ximg_verify_size(pstLocDstImg, __FUNCTION__, __LINE__))
	 {
		 return ISL_FAIL;
	 }
 
	 /* 抠图参数为空时，默认抠取全图 */
	 if (NULL == pstSrcCropPrm)
	 {
		 stLocSrcCropPrm.top = 0;
		 stLocSrcCropPrm.left = 0;
		 stLocSrcCropPrm.width = pstImageSrc->u32Width;
		 stLocSrcCropPrm.height = pstImageSrc->u32Height;
	 }
	 else
	 {
		 memcpy(&stLocSrcCropPrm, pstSrcCropPrm, sizeof(ISL_VideoCrop));
	 }
 
	 if (NULL != pstImageDst)
	 {
		 stLocDstCropPrm.top = 0;
		 stLocDstCropPrm.left = 0;
		 stLocDstCropPrm.width = pstImageDst->u32Width;
		 stLocDstCropPrm.height = pstImageDst->u32Height;
	 }
	 if (NULL != pstDstCropPrm)
	 {
		 memcpy(&stLocDstCropPrm, pstDstCropPrm, sizeof(ISL_VideoCrop));
	 }
 
	 /* 判断原图和目标抠图区域的宽高是否一致 */
	 if ((stLocSrcCropPrm.width != stLocDstCropPrm.width) || (stLocSrcCropPrm.height != stLocDstCropPrm.height))
	 {
		 log_info("crop prm src[%u x %u] and dst[%u x %u] are invalid, which are supposed to be equal, crop_prm_vir[src:%p, dst:%p].\n",
			 stLocSrcCropPrm.width, stLocSrcCropPrm.height, stLocDstCropPrm.width, stLocDstCropPrm.height, pstSrcCropPrm, pstDstCropPrm);
		 return ISL_FAIL;
	 }
 
	 if (ISL_OK != Isl_ximg_create_sub(pstImageSrc, &stSrcCropImg, &stLocSrcCropPrm))
	 {
		 log_info("Isl_ximg_create_sub for src ximg faild\n");
		 return ISL_FAIL;
	 }
	 if (ISL_OK != Isl_ximg_create_sub(pstLocDstImg, &stDstCropImg, &stLocDstCropPrm))
	 {
		 log_info("Isl_ximg_create_sub for dst ximg faild\n");
		 return ISL_FAIL;
	 }
	 Isl_ximg_copy_kernel(&stSrcCropImg, &stDstCropImg);
 
	 return ISL_OK;
 }
 
 /**
  * @function   Isl_ximg_flip_vert
  * @brief      竖直方向镜像图像，支持就地处理，每次操作16字节数据以加速
  * @param[IN]  XIMAGE_DATA_ST *pstSrcImg 源图像数据
  * @param[IN]  XIMAGE_DATA_ST *pstDstImg 目标图像数据
  * @return     int32_t  ISL_FAIL 失败 ISL_SOK  成功
  */
 static int32_t Isl_ximg_flip_vert(XIMAGE_DATA_ST* pstSrcImg, XIMAGE_DATA_ST* pstDstImg)
 {
	 int32_t i = 0, bpp = 1;
	 uint32_t u32SrcWidth = 0, u32SrcHeight = 0, u32SrcStride = 0; // 源数据宽、高、跨距，单位：像素
	 uint32_t u32DstStride = 0;                                    // 目标数据跨距，单位：像素
	 uint32_t sstep = 0;                                           // 源数据行步长，单位：字节
	 uint32_t dstep = 0;                                           // 目标数据行步长，单位：字节
	 uint32_t u32SrcByteWidth = 0, j = 0;
	 const uint8_t* src0 = NULL;
	 const uint8_t* src1 = NULL;
	 uint8_t* dst0 = NULL;
	 uint8_t* dst1 = NULL;
 
	 CHECK_PTR_IS_NULL(pstSrcImg, ISL_FAIL);
	 CHECK_PTR_IS_NULL(pstDstImg, ISL_FAIL);
	 if (ISL_OK != Isl_ximg_verify_size(pstSrcImg, __FUNCTION__, __LINE__))
	 {
		 return ISL_FAIL;
	 }
 
	 u32SrcWidth = pstSrcImg->u32Width;
	 u32SrcHeight = pstSrcImg->u32Height;
 
	 for (i = 0; i < ISL_XIMAGE_PLANE_MAX; i++)
	 {
		 src0 = (uint8_t*)pstSrcImg->pPlaneVir[i];
		 dst0 = (uint8_t*)pstDstImg->pPlaneVir[i];
 
		 if (NULL == src0 || NULL == dst0)
		 {
			 break;
		 }
 
		 u32SrcStride = pstSrcImg->u32Stride[i];
		 u32DstStride = pstDstImg->u32Stride[i];
		 bpp = Isl_ximg_get_bpp(pstSrcImg->enImgFmt, i);
		 if (0 > bpp)
		 {
			 log_info("Isl_ximg_get_bpp failed, fmt:0x%x, plane idx:%d, bpp:%d", pstSrcImg->enImgFmt, i, bpp);
			 return ISL_FAIL;
		 }
		 /* UV平面按单像素占两字节计算，宽高均为Y平面一半 */
		 if ((ISL_IMG_DATFMT_YUV420SP_VU == pstSrcImg->enImgFmt || ISL_IMG_DATFMT_YUV420SP_UV == pstSrcImg->enImgFmt) && 1 == i)
		 {
			 u32SrcWidth >>= 1;
			 u32SrcHeight >>= 1;
			 u32SrcStride >>= 1;
			 u32DstStride >>= 1;
		 }
 
		 sstep = u32SrcStride * bpp;
		 dstep = u32DstStride * bpp;
		 src1 = src0 + (u32SrcHeight - 1) * sstep;
		 dst1 = dst0 + (u32SrcHeight - 1) * dstep;
 
		 for (uint32_t y = 0; y < (u32SrcHeight + 1) / 2; y++, src0 += sstep, src1 -= sstep,
			 dst0 += dstep, dst1 -= dstep)
		 {
			 j = 0;
			 u32SrcByteWidth = u32SrcWidth * bpp;
			 for (; j <= u32SrcByteWidth - 16; j += 16)
			 {
				 int32_t t0 = ((int32_t*)(src0 + j))[0];
				 int32_t t1 = ((int32_t*)(src1 + j))[0];
 
				 ((int32_t*)(dst0 + j))[0] = t1;
				 ((int32_t*)(dst1 + j))[0] = t0;
 
				 t0 = ((int32_t*)(src0 + j))[1];
				 t1 = ((int32_t*)(src1 + j))[1];
 
				 ((int32_t*)(dst0 + j))[1] = t1;
				 ((int32_t*)(dst1 + j))[1] = t0;
 
				 t0 = ((int32_t*)(src0 + j))[2];
				 t1 = ((int32_t*)(src1 + j))[2];
 
				 ((int32_t*)(dst0 + j))[2] = t1;
				 ((int32_t*)(dst1 + j))[2] = t0;
 
				 t0 = ((int32_t*)(src0 + j))[3];
				 t1 = ((int32_t*)(src1 + j))[3];
 
				 ((int32_t*)(dst0 + j))[3] = t1;
				 ((int32_t*)(dst1 + j))[3] = t0;
			 }
 
			 for (; j <= u32SrcByteWidth - 4; j += 4)
			 {
				 int32_t t0 = ((int32_t*)(src0 + j))[0];
				 int32_t t1 = ((int32_t*)(src1 + j))[0];
 
				 ((int32_t*)(dst0 + j))[0] = t1;
				 ((int32_t*)(dst1 + j))[0] = t0;
			 }
 
			 for (; j < u32SrcByteWidth; j++)
			 {
				 uint8_t t0 = src0[j];
				 uint8_t t1 = src1[j];
 
				 dst0[j] = t1;
				 dst1[j] = t0;
			 }
		 }
	 }
	 return ISL_OK;
 }
 
 /**
  * @function   Isl_ximg_flip_horz
  * @brief      水平方向镜像图像，支持就地处理
  * @param[IN]  XIMAGE_DATA_ST *pstSrcImg 源图像数据
  * @param[IN]  XIMAGE_DATA_ST *pstDstImg 目标图像数据
  * @return     int32_t  ISL_FAIL 失败 ISL_SOK  成功
  */
 static int32_t Isl_ximg_flip_horz(XIMAGE_DATA_ST* pstSrcImg, XIMAGE_DATA_ST* pstDstImg)
 {
	 int32_t j = 0, k = 0, l = 0;
	 uint32_t i = 0;
	 int32_t bpp = 1;
	 int32_t limit = 0;
	 uint32_t u32SrcWidth = 0, u32SrcHeight = 0, u32SrcStride = 0; // 源数据宽、高、跨距，单位：像素
	 uint32_t u32DstStride = 0;                                    // 目标数据跨距，单位：像素
	 uint32_t sstep = 0;
	 uint32_t dstep = 0;
	 const uint8_t* src = NULL;
	 uint8_t* dst = NULL;
	 uint8_t cData0 = 0, cData1 = 0;
	 short tab[7680] = { 0 };
 
	 CHECK_PTR_IS_NULL(pstSrcImg, ISL_FAIL);
	 CHECK_PTR_IS_NULL(pstDstImg, ISL_FAIL);
	 if (ISL_OK != Isl_ximg_verify_size(pstSrcImg, __FUNCTION__, __LINE__))
	 {
		 return ISL_FAIL;
	 }
 
	 u32SrcWidth = pstSrcImg->u32Width;
	 u32SrcHeight = pstSrcImg->u32Height;
	 limit = (int32_t)(((u32SrcWidth + 1) / 2) * 2);
 
	 for (l = 0; l < ISL_XIMAGE_PLANE_MAX; l++)
	 {
		 src = (uint8_t*)pstSrcImg->pPlaneVir[l];
		 dst = (uint8_t*)pstDstImg->pPlaneVir[l];
 
		 if (NULL == src || NULL == dst)
		 {
			 break;
		 }
 
		 u32SrcStride = pstSrcImg->u32Stride[l];
		 u32DstStride = pstDstImg->u32Stride[l];
		 bpp = Isl_ximg_get_bpp(pstSrcImg->enImgFmt, l);
		 if (0 > bpp)
		 {
			 log_info("Isl_ximg_get_bpp failed, fmt:0x%x, plane idx:%d, bpp:%d", pstSrcImg->enImgFmt, l, bpp);
			 return ISL_FAIL;
		 }
		 /* UV平面按单像素占两字节计算，宽高均为Y平面一半 */
		 if ((ISL_IMG_DATFMT_YUV420SP_VU == pstSrcImg->enImgFmt || ISL_IMG_DATFMT_YUV420SP_UV == pstSrcImg->enImgFmt) && 1 == l)
		 {
			 u32SrcWidth >>= 1;
			 u32SrcHeight >>= 1;
			 u32SrcStride >>= 1;
			 u32DstStride >>= 1;
		 }
 
		 sstep = u32SrcStride * bpp;
		 dstep = u32DstStride * bpp;
 
		 for (i = 0; i < u32SrcWidth; i++)
		 {
			 for (k = 0; k < bpp; k++)
			 {
				 tab[i * bpp + k] = (short)((u32SrcWidth - i - 1) * bpp + k);
			 }
		 }
 
		 for (i = 0; i < u32SrcHeight; i++, src += sstep, dst += dstep)
		 {
			 for (k = 0; k < limit; k++)
			 {
				 j = tab[k];
				 cData0 = src[k], cData1 = src[j];
				 dst[k] = cData1;
				 dst[j] = cData0;
			 }
		 }
	 }
 
	 return ISL_OK;
 }
 
 /**
  * @function       Isl_ximg_flip
  * @brief          将源图像按照指定镜像方向拷贝给目标图像，镜像时支持就地处理，直接修改原图像
  * @param[IN]      XIMAGE_DATA_ST   *pstSrcImg  源图像数据
  * @param[IN/OUT]  XIMAGE_DATA_ST   *pstDstImg  目标图像数据
  * @param[IN/OUT]  XIMAGE_FLIP_MODE enFlipMode  图像镜像方向
  * @return         int32_t  ISL_FAIL 失败 ISL_SOK  成功
  */
 int32_t Isl_ximg_flip(XIMAGE_DATA_ST* pstSrcImg, XIMAGE_DATA_ST* pstDstImg, XIMAGE_FLIP_MODE enFlipMode)
 {
	 CHECK_PTR_IS_NULL(pstSrcImg, ISL_FAIL);
	 CHECK_PTR_IS_NULL(pstDstImg, ISL_FAIL);
	 if (ISL_OK != Isl_ximg_verify_size(pstSrcImg, __FUNCTION__, __LINE__) || ISL_OK != Isl_ximg_verify_size(pstDstImg, __FUNCTION__, __LINE__))
	 {
		 return ISL_FAIL;
	 }
 
	 if (0 == pstSrcImg->u32Width || 0 == pstSrcImg->u32Height)
	 {
		 log_info("Variable size copy is not supported, src[%u x %u]\n", pstSrcImg->u32Width, pstSrcImg->u32Height);
		 return ISL_FAIL;
	 }
	 if (pstSrcImg->enImgFmt != pstDstImg->enImgFmt)
	 {
		 log_info("Variable format copy is not supported, fmt_src[0x%x], fmt_dst[0x%x]\n", pstSrcImg->enImgFmt, pstDstImg->enImgFmt);
		 return ISL_FAIL;
	 }
 
	 if (XIMG_FLIP_NONE == enFlipMode)
	 {
		 Isl_ximg_copy_kernel(pstSrcImg, pstDstImg); // 无Flip时直接执行拷贝操作
	 }
	 else if (XIMG_FLIP_VERT == enFlipMode)
	 {
		 Isl_ximg_flip_vert(pstSrcImg, pstDstImg);
	 }
	 else if (XIMG_FLIP_HORZ == enFlipMode)
	 {
		 Isl_ximg_flip_horz(pstSrcImg, pstDstImg);
	 }
	 else if (XIMG_FLIP_HORZ_VERT == enFlipMode)
	 {
		 Isl_ximg_flip_horz(pstSrcImg, pstDstImg);
		 Isl_ximg_flip_vert(pstDstImg, pstDstImg);
	 }
	 else
	 {
		 log_info("Unsupported flip copy mode[0x%x]\n", enFlipMode);
		 return ISL_FAIL;
	 }
 
	 return ISL_OK;
 }
 
 /**
  * @fn      Isl_ximg_copy_nb
  * @brief   非常牛掰的ximg拷贝，支持各种局部拷贝&镜像拷贝
  *
  * @param   pstImageSrc[IN] 源图像数据，除stMbAttr外其他参数均需赋值
  * @param   pstImageDst[IN/OUT] 目标图像内存，
  *                         [IN] 当pstDstCropPrm非NULL时，除stMbAttr外其他参数均需赋值，反之仅需赋值enImgFmt、u32Stride和pPlaneVir
  *                        [OUT] 图像平面pPlaneVir内存填充图像数据
  * @param   pstSrcCropPrm[IN] 源图像抠取范围，非NULL时该区域必须在pstImageSrc指定的图像（Width×Height）内，为NULL时抠取全图
  * @param   pstDstCropPrm[IN] 目标内存中存放图像位置，
  *                            非NULL时，宽高必须与pstSrcCropPrm中宽高相同，且该区域必须在pstImageDst指定的图像（Width×Height）内，
  *                            为NULL时，则直接从源图全图或抠图拷贝到目的图像内存
  * @param   enFlipMode[IN] 图像镜像方向
  *
  * @return  int32_t ISL_FAIL：失败，ISL_SOK：成功
  */
 int32_t Isl_ximg_copy_nb(XIMAGE_DATA_ST* pstImageSrc, XIMAGE_DATA_ST* pstImageDst, ISL_VideoCrop* pstSrcCropPrm, ISL_VideoCrop* pstDstCropPrm, XIMAGE_FLIP_MODE enFlipMode)
 {
	 int32_t sRet = ISL_OK;
	 XIMAGE_DATA_ST stImgSubSrc, stImgSubDst;
	 XIMAGE_DATA_ST* pstImgSrc = pstImageSrc, * pstImgDst = pstImageDst;
	 memset(&stImgSubSrc, 0, sizeof(XIMAGE_DATA_ST));
	 memset(&stImgSubDst, 0, sizeof(XIMAGE_DATA_ST));
 
	 if (enFlipMode == XIMG_FLIP_NONE)
	 {
		 sRet = Isl_ximg_crop(pstImageSrc, pstImageDst, pstSrcCropPrm, pstDstCropPrm);
	 }
	 else
	 {
		 if (NULL != pstSrcCropPrm)
		 {
			 sRet = Isl_ximg_create_sub(pstImageSrc, &stImgSubSrc, pstSrcCropPrm);
			 if (ISL_OK != sRet)
			 {
				 log_info("Isl_ximg_create_sub faild\n");
				 return ISL_FAIL;
			 }
			 pstImgSrc = &stImgSubSrc;
		 }
		 if (NULL != pstDstCropPrm)
		 {
			 sRet = Isl_ximg_create_sub(pstImageDst, &stImgSubDst, pstDstCropPrm);
			 if (ISL_OK != sRet)
			 {
				 log_info("Isl_ximg_create_sub faild\n");
				 return ISL_FAIL;
			 }
			 pstImgDst = &stImgSubDst;
		 }
		 if (sRet == ISL_OK)
		 {
			 sRet = Isl_ximg_flip(pstImgSrc, pstImgDst, enFlipMode);
			 if (ISL_OK != sRet)
			 {
				 log_info("Isl_ximg_flip faild\n");
				 return ISL_FAIL;
			 }
		 }
	 }
 
	 return sRet;
 }
 
 int32_t Isl_ximg_dump(const char* filepath, const char* name, XIMAGE_DATA_ST* pstImageData)
 {
	 int32_t nHei = pstImageData->u32Height;
	 int32_t nWid = pstImageData->u32Width;
	 //获取系统时钟滴答数，链接字符串用于文件名
	 auto nowTime = chrono::system_clock::now();
 
	 std::string strFilePath(filepath);
	 std::string strName(name);
	 std::string strSavePath = strFilePath + "/" + strName + "_time_" +
		 std::to_string(chrono::duration_cast<chrono::milliseconds>(nowTime.time_since_epoch()).count()) + "_" +
		 std::to_string(nHei) + "_" +
		 std::to_string(nWid) + ".raw";
 
	 FILE* file = nullptr;
	 file = fopen(strSavePath.c_str(), "wb");
	 for (int32_t i = 0; i < nHei; ++i)
	 {
		 fwrite((uint16_t *)pstImageData->pPlaneVir[0] + i * pstImageData->u32Stride[0], sizeof(uint16_t), nWid, file);
	 }
 
	 //fwrite(pstImageData->pPlaneVir[1], sizeof(uint16_t), nHei * pstImageData->u32Stride[1], file);
	 fflush(file);
	 fclose(file);
 
	 return ISL_OK;
 }
 
 /**************************************************************************
  *                                变量初始化
  ***************************************************************************/
 TcProcModule::TcProcModule()
 {
	 m_pSharedPara = NULL;
	 m_nStripeBufferNum = 6;
 
	 // 包裹空间分辨增强变量初始化
	 m_stTestASlice.eSliceState = TEST_SLICE_UNKNOW;
	 m_stTestASlice.u32Test4BinTh = 1000; // TEST4边缘初始化
	 m_stTestASlice.u32PackageBinTh = 50000;
	 m_stTestASlice.u32PackageAreaTh = 1000;
	 memset(&m_stTestASlice.stRectTest4, 0, sizeof(ROTATED_RECT));
	 memset(&m_stTestASlice.stRectTest3PackDir, 0, sizeof(RECT));
	 m_stTestASlice.u32Neighbor = 4;
 
	 /* 测试体增强等级 默认40，大于100时，则展示增强信息，并将该值减去100用作融合增强等级 */
	 m_sTcProcParam.testAEnhanceGrade = 40;
	 /* 是否展示增强信息 */
	 m_sTcProcParam.bShowEnhanceInfo = false;
	 /* 是否进行测试体B检测 */
	 m_sTcProcParam.bOpenTestBeDet = false;
	 m_nTestBStartIdx = 0;
	 /* 融合增强参数初始化 */
	 m_stTestASlice.stTest4Fused.u32BlendCnt = 0;
	 /* 图像融合参数初始化 */
	 m_stTestASlice.uBlendImgNum = 3; // 融合图像数量
	 m_stTestASlice.stTEST4CacheVector.resize(m_stTestASlice.uBlendImgNum);
	 m_stTestASlice.enPreTest4State = TESTA_SLICE_END_MATCH_SECOND;
 
	 /* 图像数据初始化 */
	 for (int32_t i = 0; i < 3; ++i)
	 {
		 m_stTestASlice.stNrawMatchSlice.pPlaneVir[i] = LIBXRAY_NULL;
		 m_stTestASlice.stPreNrawSliceData.pPlaneVir[i] = LIBXRAY_NULL;
		 m_stTestASlice.stNrawBinarySlice.pPlaneVir[i] = LIBXRAY_NULL;
		 m_stTestASlice.stTest4Lhe.pPlaneVir[i] = LIBXRAY_NULL;
		 m_stTestASlice.stMaskImg.pPlaneVir[i] = LIBXRAY_NULL;
		 m_stTestASlice.stTest4Area.pPlaneVir[i] = LIBXRAY_NULL;
		 m_stTestASlice.stTEST4BlendTemp.pPlaneVir[i] = LIBXRAY_NULL;
	 }
	 m_stTestASlice.stNrawMatchSlice.enImgFmt = ISL_IMG_DATFMT_SP16;
	 m_stTestASlice.stPreNrawSliceData.enImgFmt = ISL_IMG_DATFMT_SP16;
	 m_stTestASlice.stNrawBinarySlice.enImgFmt = ISL_IMG_DATFMT_SP8;
	 m_stTestASlice.stTest4Lhe.enImgFmt = ISL_IMG_DATFMT_SP16;
	 m_stTestASlice.stTest4Area.enImgFmt = ISL_IMG_DATFMT_SP16;
	 m_stTestASlice.stMaskImg.enImgFmt = ISL_IMG_DATFMT_SP8;
	 m_stTestASlice.stTEST4BlendTemp.enImgFmt = ISL_IMG_DATFMT_SP16;
 
	 m_stTestBSlice.TESTBFastPix.total = 0;	
	 m_stTestBSlice.FastPixDefault_0.total = 0;
	 m_stTestBSlice.FastPixDefault_0.height_index = LIBXRAY_NULL;
	 m_stTestBSlice.FastPixDefault_0.width_index = LIBXRAY_NULL;
	 m_stTestBSlice.FastPixDefault_0.brief = LIBXRAY_NULL;
 
	 m_stTestBSlice.FastPixDefault_1.total = 0;
	 m_stTestBSlice.FastPixDefault_1.height_index = LIBXRAY_NULL;
	 m_stTestBSlice.FastPixDefault_1.width_index = LIBXRAY_NULL;
	 m_stTestBSlice.FastPixDefault_1.brief = LIBXRAY_NULL;
 
	 // 使用循环初始化每个对象
	 for (uint32_t i = 0; i < m_stTestASlice.uBlendImgNum; ++i)
	 {
		 m_stTestASlice.stTEST4CacheVector[i].stNrawTEST4ABlend.enImgFmt = ISL_IMG_DATFMT_SP16;
		 m_stTestASlice.stTEST4CacheVector[i].stNrawTEST4BBlend.enImgFmt = ISL_IMG_DATFMT_SP16;
	 }
 
	 m_stTestASlice.Nrawsliceindex = 0;
 }
 
 /**************************************************************************
  *                                初始化
  ***************************************************************************/
 XRAY_LIB_HRESULT TcProcModule::Init(SharedPara* pPara, int32_t nStripeNum, int32_t nUseTc)
 {
	if (nUseTc != XRAYLIB_ON)
	{
		return XRAY_LIB_OK;
	}
	else
	{
		m_pSharedPara = pPara;
 
		/*缓存条带序列号，默认缓存6个条带，输出首条带*/
		m_nStripeBufferNum = nStripeNum;
	
		for (int32_t i = 0; i < m_nStripeBufferNum; i++)
		{
			m_vecSliceSeq.push_back(i);
		}
	
		// 读取ORB中的配置文件
		if(true == m_sTcProcParam.bOpenTestBeDet)
		{
			LORB_read_FastBriefPix("./TESTBFastBrief_0.bin", &m_stTestBSlice.FastPixDefault_0);
			LORB_read_FastBriefPix("./TESTBFastBrief_1.bin", &m_stTestBSlice.FastPixDefault_1);	
		}
	}

	 return XRAY_LIB_OK;
 }
 
 /**************************************************************************
  *                             内存申请与释放
  ***************************************************************************/
 XRAY_LIB_HRESULT TcProcModule::Release()
 {
	 return XRAY_LIB_OK;
 }
 
 /******************************************************
  *                 图像处理内存
  *******************************************************/
 XRAY_LIB_HRESULT TcProcModule::GetMemSize(XRAY_LIB_MEM_TAB& MemTab, XRAY_LIB_ABILITY& ability)
 {
	 MemTab.size = 0;

	 if (ability.nUseTc)
	 {
		/* 这里的条带高度多申请了很多内存   */
		int32_t nRTProcessHeight = int32_t(ability.nMaxHeightRealTime * ability.fResizeScale);
		int32_t nRTProcessWidth = int32_t(ability.nDetectorWidth * ability.fResizeScale);
	
		// 初始化存储轮廓中每个点坐标的内存
		uint32_t u32BufSize = sizeof(POS_ATTR) * nRTProcessWidth * nRTProcessHeight;
		XspMalloc((void**)&m_stTestASlice.paContourPoints, ISL_align(u32BufSize, 16), MemTab);
	
		// 初始化轮廓包围上下边界线
		u32BufSize = sizeof(uint32_t) * nRTProcessWidth;
		XspMalloc((void**)&m_stTestASlice.stBorderCircum.stTop.pu32VerYCoor, ISL_align(u32BufSize, 16), MemTab);
		XspMalloc((void**)&m_stTestASlice.stBorderCircum.stBottom.pu32VerYCoor, ISL_align(u32BufSize, 16), MemTab);
	
		// 初始化轮廓包围左右边界线
		u32BufSize = sizeof(uint32_t) * nRTProcessHeight;
		XspMalloc((void**)&m_stTestASlice.stBorderCircum.stLeft.pu32HorXCoor, ISL_align(u32BufSize, 16), MemTab);
		XspMalloc((void**)&m_stTestASlice.stBorderCircum.stRight.pu32HorXCoor, ISL_align(u32BufSize, 16), MemTab);
	
		/* 用以测试体实时检测的内存申请,因为条带的处理有扩边，所以增加条带宽高两侧各一个像素 */
		XspMalloc((void**)&m_stTestASlice.stPreNrawSliceData.pPlaneVir[0], 30 * (nRTProcessHeight + 2) * (nRTProcessWidth + 2) * sizeof(uint8_t), MemTab);
		XspMalloc((void**)&m_stTestASlice.stPreNrawSliceData.pPlaneVir[1], 30 * (nRTProcessHeight + 2) * (nRTProcessWidth + 2) * sizeof(uint8_t), MemTab);
		XspMalloc((void**)&m_stTestASlice.stNrawMatchSlice.pPlaneVir[0], 30 * (nRTProcessHeight + 2) * (nRTProcessWidth + 2) * sizeof(uint8_t), MemTab);
		XspMalloc((void**)&m_stTestASlice.stNrawMatchSlice.pPlaneVir[1], 30 * (nRTProcessHeight + 2) * (nRTProcessWidth + 2) * sizeof(uint8_t), MemTab);
		XspMalloc((void**)&m_stTestASlice.stNrawBinarySlice.pPlaneVir[0], 30 * (nRTProcessHeight + 2) * (nRTProcessWidth + 2) * sizeof(uint8_t), MemTab);
		XspMalloc((void**)&m_stTestASlice.stNrawBinarySlice.pPlaneVir[1], 30 * (nRTProcessHeight + 2) * (nRTProcessWidth + 2) * sizeof(uint8_t), MemTab);
	
		/* TESTB区域识别 使用到LORB算法，需要额外申请内存 */
		XspMalloc((void**)&m_stTestBSlice.TESTBFastPix.height_index, XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH * sizeof(int32_t), MemTab);
		XspMalloc((void**)&m_stTestBSlice.TESTBFastPix.width_index, XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH * sizeof(int32_t), MemTab);
		XspMalloc((void**)&m_stTestBSlice.TESTBFastPix.brief, XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH * 8 * sizeof(uint8_t), MemTab);
	
		XspMalloc((void**)&m_stTestBSlice.FastPixDefault_0.height_index, XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH * sizeof(int32_t), MemTab);
		XspMalloc((void**)&m_stTestBSlice.FastPixDefault_0.width_index, XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH * sizeof(int32_t), MemTab);
		XspMalloc((void**)&m_stTestBSlice.FastPixDefault_0.brief, XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH * 8 * sizeof(uint8_t), MemTab);
	
		XspMalloc((void**)&m_stTestBSlice.FastPixDefault_1.height_index, XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH * sizeof(int32_t), MemTab);
		XspMalloc((void**)&m_stTestBSlice.FastPixDefault_1.width_index, XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH * sizeof(int32_t), MemTab);
		XspMalloc((void**)&m_stTestBSlice.FastPixDefault_1.brief, XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH * 8 * sizeof(uint8_t), MemTab);
	
		// 向量中缓存数据的地址重新分配
		for (uint32_t i = 0; i < m_stTestASlice.uBlendImgNum; ++i)
		{
			// 为 stNrawTEST4ABlend 和 stNrawTEST4BBlend 分配地址
			XspMalloc((void**)&m_stTestASlice.stTEST4CacheVector[i].stNrawTEST4ABlend.pPlaneVir[0], 30 * (nRTProcessHeight + 2) * (nRTProcessWidth + 2) * sizeof(uint8_t), MemTab);
			XspMalloc((void**)&m_stTestASlice.stTEST4CacheVector[i].stNrawTEST4ABlend.pPlaneVir[1], 30 * (nRTProcessHeight + 2) * (nRTProcessWidth + 2) * sizeof(uint8_t), MemTab);
			XspMalloc((void**)&m_stTestASlice.stTEST4CacheVector[i].stNrawTEST4BBlend.pPlaneVir[0], 30 * (nRTProcessHeight + 2) * (nRTProcessWidth + 2) * sizeof(uint8_t), MemTab);
			XspMalloc((void**)&m_stTestASlice.stTEST4CacheVector[i].stNrawTEST4BBlend.pPlaneVir[1], 30 * (nRTProcessHeight + 2) * (nRTProcessWidth + 2) * sizeof(uint8_t), MemTab);
		}
		XspMalloc((void**)&m_stTestASlice.stTEST4BlendTemp.pPlaneVir[0], 30 * (nRTProcessHeight + 2) * (nRTProcessWidth + 2) * sizeof(uint8_t), MemTab);
		XspMalloc((void**)&m_stTestASlice.stTEST4BlendTemp.pPlaneVir[1], 30 * (nRTProcessHeight + 2) * (nRTProcessWidth + 2) * sizeof(uint8_t), MemTab);
	
		/* 用以TEST4实时增强使用的内存申请30个处理条带高度 */
		XspMalloc((void**)&m_stTestASlice.stTest4Lhe.pPlaneVir[0], 30 * (nRTProcessHeight + 2 * m_stTestASlice.u32Neighbor) * (nRTProcessWidth + 2 * m_stTestASlice.u32Neighbor) * sizeof(uint8_t), MemTab);
		XspMalloc((void**)&m_stTestASlice.stTest4Lhe.pPlaneVir[1], 30 * (nRTProcessHeight + 2 * m_stTestASlice.u32Neighbor) * (nRTProcessWidth + 2 * m_stTestASlice.u32Neighbor) * sizeof(uint8_t), MemTab);
		XspMalloc((void**)&m_stTestASlice.stTest4Area.pPlaneVir[0], 30 * (nRTProcessHeight + 2 * m_stTestASlice.u32Neighbor) * (nRTProcessWidth + 2 * m_stTestASlice.u32Neighbor) * sizeof(uint8_t), MemTab);
		XspMalloc((void**)&m_stTestASlice.stTest4Area.pPlaneVir[1], 30 * (nRTProcessHeight + 2 * m_stTestASlice.u32Neighbor) * (nRTProcessWidth + 2 * m_stTestASlice.u32Neighbor) * sizeof(uint8_t), MemTab);
		XspMalloc((void**)&m_stTestASlice.stMaskImg.pPlaneVir[0], 30 * (nRTProcessHeight + 2 * m_stTestASlice.u32Neighbor) * (nRTProcessWidth + 2 * m_stTestASlice.u32Neighbor) * sizeof(uint8_t), MemTab);
		XspMalloc((void**)&m_stTestASlice.stMaskImg.pPlaneVir[1], 30 * (nRTProcessHeight + 2 * m_stTestASlice.u32Neighbor) * (nRTProcessWidth + 2 * m_stTestASlice.u32Neighbor) * sizeof(uint8_t), MemTab);
	 }
 
	 return XRAY_LIB_OK;
 }
 
 /*-------------------------旋转矩形寻找轮廓相关函数------------------------------*/
 
 static int32_t contourAreaSort(const void* pA, const void* pB) // 从大到小排序
 {
	 return (int32_t)((int32_t)((CONTOUR_ATTR*)pB)->u32Area - (int32_t)((CONTOUR_ATTR*)pA)->u32Area);
 }
 
 static int32_t contourHorSort(const void* pA, const void* pB) // 从左到右排序
 {
	 return (int32_t)((int32_t)((CONTOUR_ATTR*)pA)->stBoundingBox.uiX - (int32_t)((CONTOUR_ATTR*)pB)->stBoundingBox.uiX);
 }
 
 
 /***************** 点 *****************/
 struct Point
 {
	 double x;
	 double y;
 
	 Point(double _x, double _y) : x(_x), y(_y) {}
 
	 Point() : x(0.0), y(0.0) {}
 
	 // 重载 < 运算符
	 bool operator<(const Point& p) const
	 {
		 return ((x < p.x) || (x == p.x && y < p.y));
	 }
 
	 // 重载 << 运算符
	 friend std::ostream& operator<<(std::ostream& os, const Point& p)
	 {
		 return os << "(" << p.x << ", " << p.y << ")";
	 }
 };
 
 // →OA与→OB的叉乘
 static inline double cross(const Point& O, const Point& A, const Point& B)
 {
	 return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
 }
 
 #if 0
 static inline double area(const Point& a, const Point& b, const Point& c)
 {
	 return fabs((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x));
 }
 #endif
 
 static inline double dist(const Point& a, const Point& b)
 {
	 return hypot(a.x - b.x, a.y - b.y);
 }
 
 /***************** 向量 *****************/
 typedef Point Vector;
 
 // 向量加法
 static inline Vector operator+(const Vector& OA, const Vector& OB)
 {
	 return Vector(OA.x + OB.x, OA.y + OB.y);
 }
 
 // 向量减法，→OA - →OB = →BA，即 B -> A
 static inline Vector operator-(const Vector& OA, const Vector& OB)
 {
	 return Vector(OA.x - OB.x, OA.y - OB.y);
 }
 
 // 向量乘常数
 static inline Vector operator*(const Vector& OA, const double& r)
 {
	 return Vector(OA.x * r, OA.y * r);
 }
 
 // 向量除常数
 static inline Vector operator/(const Vector& OA, const double& r)
 {
	 return Vector(OA.x / r, OA.y / r);
 }
 
 // →OA与→OB的叉乘
 static inline double cross(const Vector& OA, const Vector& OB)
 {
	 return OA.x * OB.y - OA.y * OB.x;
 }
 
 // →OA与→OB的点乘
 static inline double dot(const Vector& OA, const Vector& OB)
 {
	 return OA.x * OB.x + OA.y * OB.y;
 }
 
 /***************** 旋转矩形 *****************/
 struct RotatedRect
 {
	 double angleW;           // 宽边Width旋转角度，范围[0, 180)
	 double angleH;           // 高边Height旋转角度，范围[0, 180)
	 double edgeW;            // 以angleW旋转后的边定义为宽Width
	 double edgeH;            // 以angleH旋转后的边定义为高Height
	 double area;             // 旋转矩形的面积
	 Point center;            // 中心点
	 vector<size_t> pointIdx; // 4个定位点索引，依次为：遍历点base、对踵点antipodal、右侧顶点right、左侧顶点left
	 vector<Point> corner;    // 4个顶点，依次为：bottomLeft、bottomRight、topLeft、topRight
 
	 RotatedRect() : angleW(0.0), angleH(0.0), edgeW(0.0), edgeH(0.0), area(numeric_limits<double>::max()), pointIdx(4) {}
 };
 
 /* 转换成凸多边形 */
 vector<Point> convexHull(vector<Point> pointSet)
 {
	 size_t n = pointSet.size();
	 size_t k = 0;
 
	 if (n <= 3)
	 {
		 return pointSet;
	 }
 
	 vector<Point> hull(2 * n);
	 sort(pointSet.begin(), pointSet.end());
 
	 /* 根据叉积求算顺时针方向的凸边点 */
	 for (size_t i = 0; i < n; ++i)
	 {
		 while (k >= 2 && cross(hull[k - 2], hull[k - 1], pointSet[i]) <= 0)
		 {
			 k--;
		 }
		 hull[k++] = pointSet[i];
	 }
 
	 /* 根据叉积求算逆时针方向的凸边点 */
	 for (size_t i = n - 1, t = k + 1; i > 0; --i)
	 {
		 while (k >= t && cross(hull[k - 2], hull[k - 1], pointSet[i - 1]) <= 0)
		 {
			 k--;
		 }
		 hull[k++] = pointSet[i - 1];
	 }
	 hull.resize(k - 1);
 
	 return hull;
 }
 
 RotatedRect IslminAreaBoundingRect(const vector<Point>& pointSet)
 {
	 /* 转换为凸多边    形 */
	 vector<Point> polygonPoint = convexHull(pointSet);
	 size_t edgeNum = polygonPoint.size(); // 凸多边形边数
 
	 //
	 //   O +-------------------------------------->
	 //     |                                      X
	 //     | Point(i)
	 //     |      +---------- Bottom ---------+ ----> 遍历的边向量
	 //     |      |                           |
	 //     |      |                           |
	 //     |    Left                        Right
	 //     |      |                           |
	 //     |      |                           |
	 //     |      +----------- Top -----------+ ----> 经过对踵点Antipodal的边
	 //     |
	 //     |
	 //     V Y
	 //
	 size_t i = 0, base = 0, antipodal = 0, right = 0, left = 0;
	 vector<Vector> polygonEdge(edgeNum); // 边向量（指向下一个点的向量）
	 bool bPolygonCw = true;              // 多边形边向量是否为顺时针
	 double cp = 0.0;
	 RotatedRect rotated_rect;
 
	 polygonPoint.push_back(polygonPoint[0]); // 多边形最后一个点与第一个点重合，精简计算
	 for (i = 0; i < edgeNum; i++)
	 {
		 polygonEdge[i] = polygonPoint[i + 1] - polygonPoint[i];
 
		 if (i > 0 && cp != 0.0)
		 {
			 cp = cross(polygonEdge[i - 1], polygonEdge[i]);
			 bPolygonCw = (cp > 0) ? false : true;
		 }
	 }
 
	 for (i = 0, antipodal = 2, right = 1; i < edgeNum; i++)
	 {
		 // 从上一个antipodal开始，查找对踵点
		 while (cross(polygonEdge[i], polygonPoint[antipodal] - polygonPoint[i]) <
			 cross(polygonEdge[i], polygonPoint[antipodal + 1] - polygonPoint[i]))
		 {
			 antipodal = (antipodal + 1) % edgeNum; // 对踵点
		 }
 
		 // 在(i, antipodal]区间内，从上一个right开始，查找右边界点
		 while (dot(polygonEdge[i], polygonPoint[right] - polygonPoint[i]) <
			 dot(polygonEdge[i], polygonPoint[right + 1] - polygonPoint[i]))
		 {
			 right = (right + 1) % edgeNum; // 右边界点
			 if (right == antipodal)
			 {
				 break;
			 }
		 }
 
		 // 在[antipodal, i]区间内，从上一个left开始，查找左边界点
		 if (0 == i)
		 {
			 left = antipodal;
		 }
		 while (dot(polygonEdge[i], polygonPoint[left] - polygonPoint[i]) >=
			 dot(polygonEdge[i], polygonPoint[left + 1] - polygonPoint[i]))
		 {
			 left = (left + 1) % edgeNum; // 左边界点
			 if (left == i)
			 {
				 break;
			 }
		 }
 
		 /**
		  * 注：
		  * 这里的Width不是长边，而是定义为与polygonEdge[i]重合的边，Height是与之垂直的边
		  * AngleWidth是坐标轴与Width边的夹角，angleHeight是坐标轴与Height边的夹角，取值范围[0, 180)
		  */
		 double edgeLen = dist(polygonPoint[i + 1], polygonPoint[i]);
		 double height = cross(polygonEdge[i], polygonPoint[antipodal] - polygonPoint[i]) / edgeLen;
		 double width = (dot(polygonEdge[i], polygonPoint[right] - polygonPoint[i]) -
			 dot(polygonEdge[i], polygonPoint[left] - polygonPoint[i])) /
			 edgeLen;
		 double area = width * height;
 
		 if (area < rotated_rect.area)
		 {
			 rotated_rect.area = area;
			 rotated_rect.edgeW = width;
			 rotated_rect.edgeH = height;
			 rotated_rect.pointIdx.clear();
			 rotated_rect.pointIdx.push_back(i);
			 rotated_rect.pointIdx.push_back(antipodal);
			 rotated_rect.pointIdx.push_back(right);
			 rotated_rect.pointIdx.push_back(left);
		 }
	 }
 
	 base = rotated_rect.pointIdx[0], antipodal = rotated_rect.pointIdx[1];
	 right = rotated_rect.pointIdx[2], left = rotated_rect.pointIdx[3];
	 double edgeLen = dist(polygonPoint[base + 1], polygonPoint[base]);
 
	 // 先计算Width边两端的bottomRight与bottomLeft点
	 Vector vectWidth = polygonEdge[base] / edgeLen;
	 Vector pt2right = polygonPoint[right] - polygonPoint[base];
	 double pt2rightCast = dot(polygonEdge[base], pt2right) / edgeLen;  // 向量pt2right在多边形上的投影，|pt2right| * cos(θ)
	 Point bottomRight = polygonPoint[base] + vectWidth * pt2rightCast; // 从polygonPoint[base]开始沿polygonEdge[base]步进pt2rightCast的长度
 
	 Vector pt2left = polygonPoint[left] - polygonPoint[base];
	 double pt2leftCast = dot(polygonEdge[base], pt2left) / edgeLen;
	 Point bottomLeft = polygonPoint[base] + vectWidth * pt2leftCast;
 
	 // 再通过bottomRight与bottomLeft点计算与Width边垂直的Height边另一端的topRight与topLeft
	 Vector vectHeight(bPolygonCw ? -vectWidth.y : vectWidth.y, bPolygonCw ? vectWidth.x : -vectWidth.x);
	 Point topRight = bottomRight + vectHeight * rotated_rect.edgeH;
	 Point topLeft = bottomLeft + vectHeight * rotated_rect.edgeH;
 
	 // 最后计算旋转角度
	 rotated_rect.angleW = atan2(vectWidth.y, vectWidth.x) * 180.0 / M_PI;   // fmod(atan2(vectWidth.y, vectWidth.x) * 180.0 / M_PI + 180.0, 180.0);
	 rotated_rect.angleH = atan2(vectHeight.y, vectHeight.x) * 180.0 / M_PI; // fmod(atan2(vectHeight.y, vectHeight.x) * 180.0 / M_PI + 180.0, 180.0);
 
	 rotated_rect.corner.push_back(bottomLeft);
	 rotated_rect.corner.push_back(bottomRight);
	 rotated_rect.corner.push_back(topRight);
	 rotated_rect.corner.push_back(topLeft);
	 rotated_rect.center = (bottomRight + topLeft) * 0.5;
 
	 return rotated_rect;
 }
 
 static inline void pointd2i(POINT_INT& po, const Point& pi)
 {
	 po.x = static_cast<int32_t>(pi.x);
	 po.y = static_cast<int32_t>(pi.y);
 
	 return;
 }
 
 static inline int32_t p2plen(const POINT_INT& pa, const POINT_INT& pb)
 {
	 return (int32_t)hypot(fabs(pa.x - pb.x) + 1, fabs(pa.y - pb.y) + 1);
 }
 
 void rotatedRectFitting(POINT_INT astPointSet[], int32_t s32PointNum, ROTATED_RECT* pstRotatedRect)
 {
	 vector<Point> pointSet(s32PointNum);
 
	 for (int32_t i = 0; i < s32PointNum; i++)
	 {
		 pointSet[i].x = static_cast<double>(astPointSet[i].x);
		 pointSet[i].y = static_cast<double>(astPointSet[i].y);
	 }
 
	 RotatedRect rotatedRect = IslminAreaBoundingRect(pointSet);
 
	 if (rotatedRect.angleW > -45.0 && rotatedRect.angleW <= 45.0)
	 {
		 pstRotatedRect->angle = rotatedRect.angleW;
		 for (size_t j = 0; j < rotatedRect.corner.size(); j++)
		 {
			 pointd2i(pstRotatedRect->corner[j], rotatedRect.corner[j]);
		 }
	 }
	 else if (rotatedRect.angleW > 45.0 && rotatedRect.angleW <= 135.0)
	 {
		 pstRotatedRect->angle = rotatedRect.angleW - 90.0;
		 pointd2i(pstRotatedRect->corner[RECT_CORNER_BOTTOMLEFT], rotatedRect.corner[RECT_CORNER_TOPLEFT]);
		 pointd2i(pstRotatedRect->corner[RECT_CORNER_BOTTOMRIGHT], rotatedRect.corner[RECT_CORNER_BOTTOMLEFT]);
		 pointd2i(pstRotatedRect->corner[RECT_CORNER_TOPRIGHT], rotatedRect.corner[RECT_CORNER_BOTTOMRIGHT]);
		 pointd2i(pstRotatedRect->corner[RECT_CORNER_TOPLEFT], rotatedRect.corner[RECT_CORNER_TOPRIGHT]);
	 }
	 else if (rotatedRect.angleW > -135.0 && rotatedRect.angleW <= -45.0)
	 {
		 pstRotatedRect->angle = rotatedRect.angleW + 90.0;
		 pointd2i(pstRotatedRect->corner[RECT_CORNER_BOTTOMLEFT], rotatedRect.corner[RECT_CORNER_BOTTOMRIGHT]);
		 pointd2i(pstRotatedRect->corner[RECT_CORNER_BOTTOMRIGHT], rotatedRect.corner[RECT_CORNER_TOPRIGHT]);
		 pointd2i(pstRotatedRect->corner[RECT_CORNER_TOPRIGHT], rotatedRect.corner[RECT_CORNER_TOPLEFT]);
		 pointd2i(pstRotatedRect->corner[RECT_CORNER_TOPLEFT], rotatedRect.corner[RECT_CORNER_BOTTOMLEFT]);
	 }
	 else if ((rotatedRect.angleW > 135.0 && rotatedRect.angleW <= 180.0) ||
		 (rotatedRect.angleW > -180.0 && rotatedRect.angleW <= -135.0))
	 {
		 pstRotatedRect->angle = (rotatedRect.angleW > 0) ? (rotatedRect.angleW - 180.0) : (rotatedRect.angleW + 180.0);
		 pointd2i(pstRotatedRect->corner[RECT_CORNER_BOTTOMLEFT], rotatedRect.corner[RECT_CORNER_TOPRIGHT]);
		 pointd2i(pstRotatedRect->corner[RECT_CORNER_BOTTOMRIGHT], rotatedRect.corner[RECT_CORNER_TOPLEFT]);
		 pointd2i(pstRotatedRect->corner[RECT_CORNER_TOPRIGHT], rotatedRect.corner[RECT_CORNER_BOTTOMLEFT]);
		 pointd2i(pstRotatedRect->corner[RECT_CORNER_TOPLEFT], rotatedRect.corner[RECT_CORNER_BOTTOMRIGHT]);
	 }
	 pstRotatedRect->width = static_cast<int32_t>(p2plen(pstRotatedRect->corner[RECT_CORNER_BOTTOMRIGHT], pstRotatedRect->corner[RECT_CORNER_BOTTOMLEFT]));
	 pstRotatedRect->height = static_cast<int32_t>(p2plen(pstRotatedRect->corner[RECT_CORNER_BOTTOMRIGHT], pstRotatedRect->corner[RECT_CORNER_TOPRIGHT]));
	 pstRotatedRect->area = pstRotatedRect->width * pstRotatedRect->height;
	 pointd2i(pstRotatedRect->center, rotatedRect.center);
 
	 return;
 }
 
 /**
  * @fn      bRectEnveloped
  * @brief   两正交矩形是否完全包络
  * @param   [IN] pstInner 内矩形
  * @param   [IN] pstOuter 外矩形
  * @return  bool TRUE：完全包络，FALSE：非完全包络
  */
 static bool bRectEnveloped(const RECT* pstInner, const RECT* pstOuter)
 {
	 if (pstInner->uiX >= pstOuter->uiX && // 左侧
		 pstInner->uiX + pstInner->uiWidth <= pstOuter->uiX + pstOuter->uiWidth &&
		 pstInner->uiY >= pstOuter->uiY &&
		 pstInner->uiY + pstInner->uiHeight <= pstOuter->uiY + pstOuter->uiHeight)
	 {
		 return true;
	 }
	 else
	 {
		 return false;
	 }
 }
 
 static void set_position(POS_ATTR* p, uint32_t x, uint32_t y)
 {
	 if (NULL != p)
	 {
		 p->x = x;
		 p->y = y;
	 }
	 return;
 }
 
 static void set_pixel_val(XIMAGE_DATA_ST* pstBinary, POS_ATTR* p, uint8_t val)
 {
	 *((uint8_t*)pstBinary->pPlaneVir[0] + pstBinary->u32Stride[0] * p->y + p->x) = val;
 
	 return;
 }
 
 static uint8_t get_pixel_val(XIMAGE_DATA_ST* pstBinary, POS_ATTR* p)
 {
	 return *((uint8_t*)pstBinary->pPlaneVir[0] + pstBinary->u32Stride[0] * p->y + p->x);
 }
 
 // 8-step around a pixel counter-clockwise
 static void step_ccw8(POS_ATTR* current, POS_ATTR* center)
 {
	 if (current->x > center->x && current->y >= center->y) // 右下->右 或 右->右上
	 {
		 current->y--;
	 }
	 else if (current->x < center->x && current->y <= center->y) // 左上->左 或 左->左下
	 {
		 current->y++;
	 }
	 else if (current->x <= center->x && current->y > center->y) // 左下->下 或 下->右下
	 {
		 current->x++;
	 }
	 else if (current->x >= center->x && current->y < center->y) // 右上->上 或 上->左上
	 {
		 current->x--;
	 }
 
	 return;
 }
 
 // 8-step around a pixel clockwise
 static void step_cw8(POS_ATTR* current, POS_ATTR* center)
 {
	 if (current->x > center->x && current->y <= center->y) // 右上->右 或 右->右下
	 {
		 current->y++;
	 }
	 else if (current->x < center->x && current->y >= center->y) // 左下->左 或 左->左上
	 {
		 current->y--;
	 }
	 else if (current->x >= center->x && current->y > center->y) // 右下->下 或 下->左下
	 {
		 current->x--;
	 }
	 else if (current->x <= center->x && current->y < center->y) // 左上->上 或 上->右上
	 {
		 current->x++;
	 }
 
	 return;
 }
 
 static bool isTheSamePoint(POS_ATTR* pA, POS_ATTR* pB)
 {
	 return (pA->x == pB->x && pA->y == pB->y);
 }
 
 static void swapPoint(POS_ATTR* pA, POS_ATTR* pB)
 {
	 ISL_SWAP(pA->x, pB->x);
	 ISL_SWAP(pA->y, pB->y);
 
	 return;
 }
 
 static int32_t pointSortX(const void* pA, const void* pB)
 {
	 return (int32_t)((int32_t)((POS_ATTR*)pA)->x - (int32_t)((POS_ATTR*)pB)->x);
 }
 
 static int32_t pointSortY(const void* pA, const void* pB)
 {
	 return (int32_t)((int32_t)((POS_ATTR*)pA)->y - (int32_t)((POS_ATTR*)pB)->y);
 }
 
 static void follow_straight_line(STRAIGHT_LINE* pstLine, bool bHor, POS_ATTR* pPointsArr, uint32_t pointsNum)
 {
	 uint32_t* pMajorDirMin = NULL, * pMajorDirMax = NULL;
	 uint32_t* pMinorDirMin = NULL, * pMinorDirMax = NULL;
	 uint32_t* pLineLenTh = NULL, * pLineError = NULL;
	 POS_ATTR* pstPointTail = NULL;
	 POS_ATTR* pPointNew = pPointsArr + ISL_SUB_SAFE(pointsNum, 1);
 
	 pstLine->horHist[pPointNew->x]++;
	 pstLine->verHist[pPointNew->y]++;
 
	 if (pstLine->pixelCnt == 0)
	 {
		 pstLine->pixelCnt = 1;
		 pstLine->horMin = pPointNew->x;
		 pstLine->horMax = pPointNew->x;
		 pstLine->verMin = pPointNew->y;
		 pstLine->verMax = pPointNew->y;
	 }
	 else
	 {
		 pstLine->pixelCnt++;
		 if (pstLine->horMin > pPointNew->x)
		 {
			 pstLine->horMin = pPointNew->x;
		 }
		 if (pstLine->horMax < pPointNew->x)
		 {
			 pstLine->horMax = pPointNew->x;
		 }
 
		 if (pstLine->verMin > pPointNew->y)
		 {
			 pstLine->verMin = pPointNew->y;
		 }
		 if (pstLine->verMax < pPointNew->y)
		 {
			 pstLine->verMax = pPointNew->y;
		 }
	 }
 
	 if (bHor)
	 {
		 pMajorDirMin = &pstLine->horMin;
		 pMajorDirMax = &pstLine->horMax;
		 pLineLenTh = &pstLine->horTh;
		 pMinorDirMin = &pstLine->verMin;
		 pMinorDirMax = &pstLine->verMax;
		 pLineError = &pstLine->verTh;
	 }
	 else
	 {
		 pMajorDirMin = &pstLine->verMin;
		 pMajorDirMax = &pstLine->verMax;
		 pLineLenTh = &pstLine->verTh;
		 pMinorDirMin = &pstLine->horMin;
		 pMinorDirMax = &pstLine->horMax;
		 pLineError = &pstLine->horTh;
	 }
 
	 while (*pMinorDirMax - *pMinorDirMin >= *pLineError) // 剔除超过直线误差的点
	 {
		 pstPointTail = pPointsArr + pointsNum - pstLine->pixelCnt--;
 
		 pstLine->horHist[pstPointTail->x]--; // 水平直方图对应的Bin-1
		 if (pstLine->horHist[pstPointTail->x] == 0)
		 {
			 if (pstLine->horMin == pstPointTail->x) // 重新寻找horMin
			 {
				 while (pstLine->horHist[++pstLine->horMin] == 0)
				 {
					 if (pstLine->horMin >= pstLine->horMax)
					 {
						 break;
					 }
				 }
			 }
			 else if (pstLine->horMax == pstPointTail->x) // 重新寻找horMax
			 {
				 while (pstLine->horHist[--pstLine->horMax] == 0)
				 {
					 if (pstLine->horMax <= pstLine->horMin)
					 {
						 break;
					 }
				 }
			 }
		 }
 
		 pstLine->verHist[pstPointTail->y]--; // 垂直直方图对应的Bin-1
		 if (pstLine->verHist[pstPointTail->y] == 0)
		 {
			 if (pstLine->verMin == pstPointTail->y) // 重新寻找verMin
			 {
				 while (pstLine->verHist[++pstLine->verMin] == 0)
				 {
					 if (pstLine->verMin >= pstLine->verMax)
					 {
						 break;
					 }
				 }
			 }
			 else if (pstLine->verMax == pstPointTail->y) // 重新寻找verMax
			 {
				 while (pstLine->verHist[--pstLine->verMax] == 0)
				 {
					 if (pstLine->verMax <= pstLine->verMin)
					 {
						 break;
					 }
				 }
			 }
		 }
	 }
 
	 uint32_t lineLenNew = *pMajorDirMax - *pMajorDirMin + 1;
	 uint32_t startIdxNew = pointsNum - pstLine->pixelCnt;
	 if (lineLenNew >= *pLineLenTh)
	 {
		 // 剔除无效像素：像素点增加了，但线长度未增加
		 if (lineLenNew < pstLine->lineLen) // 长度变短且与原线段有重合
		 {
			 if (bHor)
			 {
				 if (((int32_t)pPointsArr[startIdxNew].x - (int32_t)pPointsArr[pstLine->endpoints.u32IdxStart].x) *
					 ((int32_t)pPointsArr[startIdxNew].x - (int32_t)pPointsArr[pstLine->endpoints.u32IdxEnd].x) <
					 0)
				 {
					 return;
				 }
			 }
			 else
			 {
				 if (((int32_t)pPointsArr[startIdxNew].y - (int32_t)pPointsArr[pstLine->endpoints.u32IdxStart].y) *
					 ((int32_t)pPointsArr[startIdxNew].y - (int32_t)pPointsArr[pstLine->endpoints.u32IdxEnd].y) <
					 0)
				 {
					 return;
				 }
			 }
		 }
		 else if (lineLenNew == pstLine->lineLen && startIdxNew == pstLine->endpoints.u32IdxStart) // 长度一致且起始点也一致
		 {
			 return;
		 }
 
		 pstLine->lineLen = lineLenNew;
		 pstLine->lineError = *pMinorDirMax - *pMinorDirMin + 1;
		 pstLine->endpoints.u32IdxStart = startIdxNew;
		 pstLine->endpoints.u32IdxEnd = pointsNum - 1;
	 }
 
	 return;
 }
 
 static void recognize_right_angle(RIGHT_ANGLE* pstRightAngle, POS_ATTR* pPointsArr)
 {
	 STRAIGHT_LINE* pstLineHor = &pstRightAngle->lineHor;
	 STRAIGHT_LINE* pstLineVer = &pstRightAngle->lineVer;
	 CORNER_ATTR* pstConner = NULL;
 
	 if (pstLineHor->lineLen > 0 && pstLineVer->lineLen > 0)
	 {
		 if (pstLineHor->endpoints.u32IdxStart < pstLineVer->endpoints.u32IdxStart && // 水平线在前，垂直线在后
			 pstLineVer->endpoints.u32IdxStart <= pstLineHor->endpoints.u32IdxEnd)    // 水平线与垂直线有交叉
		 {
			 if (pstRightAngle->cornersNum > 0) // 当为同一水平线时，只更新直角的垂直线
			 {
				 pstConner = pstRightAngle->connersAttr + (pstRightAngle->cornersNum - 1);
				 if (pstConner->horEp.u32IdxStart == pstLineHor->endpoints.u32IdxStart &&
					 pstConner->horEp.u32IdxEnd == pstLineHor->endpoints.u32IdxEnd &&
					 pstLineVer->endpoints.u32IdxStart >= pstConner->verEp.u32IdxStart &&
					 pstLineVer->endpoints.u32IdxStart <= pstConner->verEp.u32IdxEnd) // 新垂直线起始点在原垂直线中间
				 {
					 goto NEXT;
				 }
			 }
			 else if (pstRightAngle->cornersNum >= ISL_arraySize(pstRightAngle->connersAttr))
			 {
				 return;
			 }
 
			 // 寻找轮廓是逆时针顺序的，水平线向右&垂直线向下、水平线向左&垂直线向上为凹角
			 pstConner = pstRightAngle->connersAttr + pstRightAngle->cornersNum;
			 if (pPointsArr[pstLineHor->endpoints.u32IdxEnd].x > pPointsArr[pstLineHor->endpoints.u32IdxStart].x &&
				 pPointsArr[pstLineVer->endpoints.u32IdxEnd].y > pPointsArr[pstLineVer->endpoints.u32IdxStart].y)
			 {
				 pstConner->bConcave = true;
				 pstConner->enOrient = PIE_ORIENT_BOTTOMLEFT;
			 }
			 else if (pPointsArr[pstLineHor->endpoints.u32IdxEnd].x < pPointsArr[pstLineHor->endpoints.u32IdxStart].x &&
				 pPointsArr[pstLineVer->endpoints.u32IdxEnd].y < pPointsArr[pstLineVer->endpoints.u32IdxStart].y)
			 {
				 pstConner->bConcave = true;
				 pstConner->enOrient = PIE_ORIENT_TOPRIGHT;
			 }
			 else // 只记录凹角
			 {
				 pstConner->bConcave = false;
				 return;
			 }
 
			 pstRightAngle->cornersNum++;
			 pstConner->pointPos.x = pPointsArr[pstLineHor->endpoints.u32IdxEnd].x;   // 与水平线最后一个点的X相同
			 pstConner->pointPos.y = pPointsArr[pstLineVer->endpoints.u32IdxStart].y; // 与垂直线第一个点的Y相同
		 }
		 else if (pstLineVer->endpoints.u32IdxStart < pstLineHor->endpoints.u32IdxStart && // 垂直线在前，水平线在后
			 pstLineHor->endpoints.u32IdxStart <= pstLineVer->endpoints.u32IdxEnd)    // 水平线与垂直线有交叉
		 {
			 if (pstRightAngle->cornersNum > 0) // 当为同一垂直线时，只更新直角的水平线
			 {
				 pstConner = pstRightAngle->connersAttr + (pstRightAngle->cornersNum - 1);
				 if (pstConner->verEp.u32IdxStart == pstLineVer->endpoints.u32IdxStart &&
					 pstConner->verEp.u32IdxEnd == pstLineVer->endpoints.u32IdxEnd &&
					 pstLineHor->endpoints.u32IdxStart >= pstConner->horEp.u32IdxStart &&
					 pstLineHor->endpoints.u32IdxStart <= pstConner->horEp.u32IdxEnd) // 新水平线起始点在原水平线中间
				 {
					 goto NEXT;
				 }
			 }
			 else if (pstRightAngle->cornersNum >= ISL_arraySize(pstRightAngle->connersAttr))
			 {
				 return;
			 }
 
			 // 寻找轮廓是逆时针顺序的，垂直线向上&水平线向右、垂直线向下&水平线向左为凹角
			 pstConner = pstRightAngle->connersAttr + pstRightAngle->cornersNum;
			 if (pPointsArr[pstLineHor->endpoints.u32IdxEnd].x > pPointsArr[pstLineHor->endpoints.u32IdxStart].x &&
				 pPointsArr[pstLineVer->endpoints.u32IdxEnd].y < pPointsArr[pstLineVer->endpoints.u32IdxStart].y)
			 {
				 pstConner->bConcave = true;
				 pstConner->enOrient = PIE_ORIENT_BOTTOMRIGHT;
			 }
			 else if (pPointsArr[pstLineHor->endpoints.u32IdxEnd].x < pPointsArr[pstLineHor->endpoints.u32IdxStart].x &&
				 pPointsArr[pstLineVer->endpoints.u32IdxEnd].y > pPointsArr[pstLineVer->endpoints.u32IdxStart].y)
			 {
				 pstConner->bConcave = true;
				 pstConner->enOrient = PIE_ORIENT_TOPLEFT;
			 }
			 else // 只记录凹角
			 {
				 pstConner->bConcave = false;
				 return;
			 }
 
			 pstRightAngle->cornersNum++;
			 pstConner->pointPos.x = pPointsArr[pstLineHor->endpoints.u32IdxStart].x; // 与水平线第一个点的X相同
			 pstConner->pointPos.y = pPointsArr[pstLineVer->endpoints.u32IdxEnd].y;   // 与垂直线最后一个点的Y相同
		 }
 
	 NEXT:
		 if (NULL != pstConner)
		 {
			 pstConner->horEp = pstLineHor->endpoints;
			 pstConner->verEp = pstLineVer->endpoints;
			 pstConner->horLen = pstLineHor->lineLen;
			 pstConner->verLen = pstLineVer->lineLen;
		 }
	 }
 
	 return;
 }
 
 /**
  * @fn      follow_contour
  * @brief   跟踪外轮廓
  *
  * @param   [IN] pstBinary 二值图，这里是需要膨胀腐蚀后的
  * @param   [IN] pBorderOuter 判定起始点时的左侧点，在轮廓外，不属于轮廓
  * @param   [IN] pBorderInner 判定起始点时的右侧点，在轮廓内，属于轮廓
  * @param   [IN] paContourPoints 存储边界点的数组缓存
  * @param   [IN] u32ContourPixelVal 轮廓像素值，便于区分不同的轮廓
  *
  * @return  uint32_t 轮廓像素点数量，0则表示跟踪轮廓异常
  */
 static uint32_t follow_contour(XIMAGE_DATA_ST* pstBinary, POS_ATTR* pBorderOuter, POS_ATTR* pBorderInner, POS_ATTR* paContourPoints,
	 uint32_t u32ContourPixelVal, RIGHT_ANGLE* pstRightAngle)
 {
	 uint32_t i = 0, u32ContourPointsNum = 0;
	 int32_t s32CycleCnt = 0;
	 uint8_t u8PixelVal = 0;
	 POS_ATTR pointCurrent, pointCenter;
	 POS_ATTR pointStart; // 指向border的起始点，在轮廓跟踪过程中，该指针始终不变，在判断跟踪是否结束的时候用到它
	 POS_ATTR pointLast;  // 指向border的最后一个点，在轮廓跟踪过程中，该指针始终不变，在判断跟踪是否结束的时候用到它
	 // 轮廓算法中首先找到起始点，紧接着就寻找这个点，找到这个点之后才开始寻找轮廓的第二个像素点
	 memset(&pointCurrent, 0, sizeof(POS_ATTR));
	 memset(&pointCenter, 0, sizeof(POS_ATTR));
	 memset(&pointStart, 0, sizeof(POS_ATTR));
	 memset(&pointLast, 0, sizeof(POS_ATTR));
 
	 STRAIGHT_LINE* pstLineHor = NULL;
	 STRAIGHT_LINE* pstLineVer = NULL;
 
	 set_position(&pointStart, pBorderInner->x, pBorderInner->y);
	 set_position(&pointCurrent, pBorderOuter->x, pBorderOuter->y);
	 pointCenter = pointStart;
 
	 if (NULL != pstRightAngle)
	 {
		 pstLineHor = &pstRightAngle->lineHor;
		 memset(pstLineHor->horHist, 0, sizeof(uint32_t) * pstBinary->u32Width);
		 memset(pstLineHor->verHist, 0, sizeof(uint32_t) * pstBinary->u32Height);
		 pstLineHor->pixelCnt = 0;
		 pstLineHor->lineLen = 0;
 
		 pstLineVer = &pstRightAngle->lineVer;
		 memset(pstLineVer->horHist, 0, sizeof(uint32_t) * pstBinary->u32Width);
		 memset(pstLineVer->verHist, 0, sizeof(uint32_t) * pstBinary->u32Height);
		 pstLineVer->pixelCnt = 0;
		 pstLineVer->lineLen = 0;
	 }
 
	 s32CycleCnt = 8;
	 while (s32CycleCnt-- > 0)
	 {
		 step_cw8(&pointCurrent, &pointCenter);           // 以起点为中心，顺时针8-邻域寻找最后一个Border Point
		 if (isTheSamePoint(&pointCurrent, pBorderOuter)) // pointStart为孤点，不认为是边缘
		 {
			 return 0;
		 }
		 else
		 {
			 if (get_pixel_val(pstBinary, &pointCurrent) == 0) // Bingo the Last Border Point
			 {
				 pointLast = pointCurrent;
				 paContourPoints[u32ContourPointsNum++] = pointCenter;
				 if (NULL != pstRightAngle)
				 {
					 follow_straight_line(pstLineHor, true, paContourPoints, u32ContourPointsNum);
					 follow_straight_line(pstLineVer, false, paContourPoints, u32ContourPointsNum);
				 }
				 break;
			 }
		 }
	 }
	 if (s32CycleCnt <= 0) // 循环被动结束，非主动跳出
	 {
		 log_info("8-step clockwise out, CycleCnt %d\n", s32CycleCnt);
		 return 0;
	 }
 
	 // 以pointStart为中心，逆时针8-邻域寻找下一个Border Point
	 s32CycleCnt = 8;
	 while (s32CycleCnt-- > 0)
	 {
		 step_ccw8(&pointCurrent, &pointCenter); // 采用弱连通算法，即对角线也视为连通
		 u8PixelVal = get_pixel_val(pstBinary, &pointCurrent);
		 if (u8PixelVal == 0 || u8PixelVal == u32ContourPixelVal) // Bingo the Next Border Point
		 {
			 if (isTheSamePoint(&pointStart, &pointCurrent) && isTheSamePoint(&pointLast, &pointCenter)) // 回到起始点，结束
			 {
				 set_pixel_val(pstBinary, &pointStart, u32ContourPixelVal);
				 break;
			 }
			 else
			 {
				 paContourPoints[u32ContourPointsNum++] = pointCurrent;
				 if (NULL != pstRightAngle)
				 {
					 follow_straight_line(pstLineHor, true, paContourPoints, u32ContourPointsNum);
					 follow_straight_line(pstLineVer, false, paContourPoints, u32ContourPointsNum);
					 recognize_right_angle(pstRightAngle, paContourPoints);
				 }
				 set_pixel_val(pstBinary, &pointCurrent, u32ContourPixelVal); // 重置边界的值，便于从图像上区分各边界
				 swapPoint(&pointCurrent, &pointCenter);                      // 交换pointCurrent与pointCenter
				 s32CycleCnt = 8;                                             // 重置循环次数
			 }
		 }
	 }
 
	 if (s32CycleCnt >= 0)
	 {
		 return u32ContourPointsNum;
	 }
	 else // 循环被动结束，非主动跳出
	 {
		 for (i = 0; i < u32ContourPointsNum; i++) // 回退边界值
		 {
			 set_pixel_val(pstBinary, paContourPoints + i, 0);
		 }
		 return 0;
	 }
 }
 static void trace_border_hor(POS_ATTR* paContourPoints, uint32_t u32BorderPointsNum, BORDER_HOR* pstTop, BORDER_HOR* pstBottom)
 {
	 uint32_t k = 0;
	 int32_t s32Left = (int32_t)u32BorderPointsNum;
	 POS_ATTR* pPointStart = NULL, * pPointEnd = NULL;
 
	 qsort(paContourPoints, u32BorderPointsNum, sizeof(POS_ATTR), pointSortX); // 对横坐标进行从小到大排序
	 pstTop->u32HorStart = paContourPoints->x;                                 // 取横坐标X最小值为起始点
	 pstBottom->u32HorStart = paContourPoints->x;
	 pstTop->u32BorderLen = 0;
	 pstBottom->u32BorderLen = 0;
 
	 while (s32Left-- > 0)
	 {
		 k = 1;
		 if (s32Left > 0)
		 {
			 pPointStart = paContourPoints + (u32BorderPointsNum - s32Left - 1);
			 while ((s32Left > 0) && (pPointStart->x == (pPointStart + k)->x)) // 横坐标相同时，对纵坐标进行从小到大排序
			 {
				 k++;
				 s32Left--;
			 }
		 }
		 pPointEnd = pPointStart + k - 1;
 
		 if (k > 1)
		 {
			 qsort(pPointStart, k, sizeof(POS_ATTR), pointSortY); // 对纵坐标进行从小到大排序
		 }
 
		 if ((pPointEnd != NULL) && (pPointStart != NULL))
		 {
			 // 取纵坐标Y最小值为上沿
			 pstTop->pu32VerYCoor[pstTop->u32BorderLen] = pPointStart->y;
			 pstTop->u32BorderLen++;
 
			 // 取纵坐标Y最大值为下沿
			 pstBottom->pu32VerYCoor[pstBottom->u32BorderLen] = pPointEnd->y;
			 pstBottom->u32BorderLen++;
		 }
	 }
	 return;
 }
 
 static void trace_border_ver(POS_ATTR* paContourPoints, uint32_t u32BorderPointsNum, BORDER_VER* pstLeft, BORDER_VER* pstRight,
	 XIMAGE_DATA_ST* pstNorm, XIMAGE_DATA_ST* pstBinary, uint32_t u32BinNeighbor, uint32_t u32Norm2BinDsRatio,
	 uint32_t* pu32NormAvg, uint32_t* pu32BinSum, uint32_t u32ContourNo)
 {
	 uint32_t i = 0, k = 0, l = 0, m = 0, n = 0, z = 0;
	 POS_ATTR* pPointCur = NULL, * pPointNext = NULL, * pPointStart = NULL, * pPointEnd = NULL, stPointTmp = { 0, 0 };
	 int32_t s32Left = (int32_t)u32BorderPointsNum;
	 uint16_t** pu16NormLine = (uint16_t**)malloc(u32Norm2BinDsRatio * sizeof(uint16_t*));
	 uint32_t u32BinSum = 0, u32SumTmp = 0, u32NormSum = 0;
	 uint32_t u32BorderTotalLen = 0, u32Col = 0;
	 const uint u32Block = u32Norm2BinDsRatio * u32Norm2BinDsRatio;
 
	 qsort(paContourPoints, u32BorderPointsNum, sizeof(POS_ATTR), pointSortY); // 对纵坐标进行从小到大排序
	 pstLeft->u32VerStart = paContourPoints->y;                                // 取纵坐标Y最小值为起始点
	 pstRight->u32VerStart = paContourPoints->y;
	 u32BorderTotalLen = paContourPoints[u32BorderPointsNum - 1].y - paContourPoints[0].y + 1;
	 pstLeft->u32BorderLen = 0;
	 pstRight->u32BorderLen = 0;
 
	 while (s32Left-- > 0)
	 {
		 k = 1;
		 if (s32Left > 0)
		 {
			 pPointStart = paContourPoints + (u32BorderPointsNum - s32Left - 1);
			 while ((s32Left > 0) && (pPointStart->y == (pPointStart + k)->y)) // 纵坐标相同时，对横坐标进行从小到大排序
			 {
				 k++;
				 s32Left--;
			 }
		 }
		 pPointEnd = pPointStart + k - 1;
 
		 if (k > 1)
		 {
			 qsort(pPointStart, k, sizeof(POS_ATTR), pointSortX); // 对横坐标进行从小到大排序
			 pPointNext = pPointCur = pPointStart;
			 while (pPointNext < pPointEnd)
			 {
				 // 从pPointNext开始检测连续点
				 stPointTmp.y = pPointNext->y;
				 while (pPointNext < pPointEnd)
				 {
					 stPointTmp.x = pPointNext->x + 1;
					 if ((pPointNext + 1)->x - pPointNext->x == 1 || 0 == get_pixel_val(pstBinary, &stPointTmp))
					 {
						 pPointNext++;
					 }
					 else
					 {
						 break;
					 }
				 }
 
				 // 分段统计
				 m = pPointCur->x - u32BinNeighbor;
				 n = pPointNext->x - u32BinNeighbor + 1;
				 if (NULL != pstNorm)
				 {
					 for (l = 0; l < u32Norm2BinDsRatio; l++)
					 {
						 pu16NormLine[l] = (uint16_t*)pstNorm->pPlaneVir[0] + pstNorm->u32Width * ((pPointStart->y - u32BinNeighbor) * u32Norm2BinDsRatio + l);
					 }
				 }
				 for (i = m; i < n; i++)
				 {
					 if (NULL != pstNorm)
					 {
						 if (u32Norm2BinDsRatio > 1)
						 {
							 u32SumTmp = 0;
							 u32Col = i * u32Norm2BinDsRatio;
							 for (l = 0; l < u32Norm2BinDsRatio; l++)
							 {
								 for (z = 0; z < u32Norm2BinDsRatio; z++) // 以Norm2BinDsRatio×Norm2BinDsRatio小方块进行统计
								 {
									 u32SumTmp += pu16NormLine[l][u32Col + z];
								 }
							 }
							 u32NormSum += u32SumTmp / u32Block;
						 }
						 else
						 {
							 u32NormSum += pu16NormLine[0][i];
						 }
					 }
					 u32BinSum++;
				 }
 
				 pPointCur = ++pPointNext;
			 }
		 }
 
		 if ((pPointStart != NULL) && (pPointEnd != NULL))
		 {
			 // 取横坐标X最小值为左沿
			 pstLeft->pu32HorXCoor[pstLeft->u32BorderLen] = pPointStart->x;
			 pstLeft->u32BorderLen++;
 
			 // 取横坐标X最大值为右沿
			 pstRight->pu32HorXCoor[pstRight->u32BorderLen] = pPointEnd->x;
			 pstRight->u32BorderLen++;
		 }
	 }
 
	 if (u32BorderTotalLen != pstLeft->u32BorderLen)
	 {
		 log_info("the BorderTotalLen(%u) is not equal to LeftBorderLen(%u), ContourNo %u, BorderPointsNum %u, BinSum %u\n",
			 u32BorderTotalLen, pstLeft->u32BorderLen, u32ContourNo, u32BorderPointsNum, u32BinSum);
	 }
	 if (NULL != pu32NormAvg)
	 {
		 *pu32NormAvg = ISL_DIV_SAFE(u32NormSum, u32BinSum);
	 }
	 if (NULL != pu32BinSum)
	 {
		 *pu32BinSum = u32BinSum;
	 }
	 free(pu16NormLine);
	 return;
 }
 
 /**
  * @fn      clear_contour
  * @brief   根据轮廓编号清除指定轮廓，轮廓范围内所有像素置0xFF
  *
  * @param   [IN] pstBinary 轮廓二值图
  * @param   [IN] u32ContourNo 轮廓编号
  * @param   [IN] aContourGrpRange trace_border_hor()输出的轮廓组
  * @param   [IN] paContourPoints trace_border_hor()输出的轮廓点
  * @param   [IN] enClearMode 清除方式
  *
  * @return  void 无
  */
 static void clear_contour(XIMAGE_DATA_ST* pstBinary, uint32_t u32ContourNo, GROUP_RANGE aContourGrpRange[],
	 POS_ATTR* paContourPoints, CONTOUR_CLEAR_MODE enClearMode)
 {
	 uint32_t i = 0, j = 0, k = 0, y = 0;
	 uint32_t u32ContourGrpIdx = ISL_SUB_SAFE(u32ContourNo, 1); // 轮廓编号转为轮廓组序号
	 uint32_t u32IdxStart = 0, u32IdxEnd = 0;
	 POS_ATTR* pPointCur = NULL, * pPointNext = NULL, stPointTmp = { 0, 0 };
 
	 if (0 == u32ContourNo || u32ContourNo > XIMG_CONTOURS_GRP_IDX_MAX)
	 {
		 // log_info("the u32ContourNo(%u) is invalid, range: [1, %d]\n", u32ContourNo, XIMG_CONTOURS_GRP_IDX_MAX);
		 return;
	 }
 
	 u32IdxStart = aContourGrpRange[u32ContourGrpIdx].u32IdxStart;
	 u32IdxEnd = aContourGrpRange[u32ContourGrpIdx].u32IdxEnd;
 
	 if (CONTOUR_INSIDE_2_WHITE_HOR == enClearMode) // 重置轮廓边缘及其内部所有像素为白色（0xFF），横向遍历轮廓内部每个像素
	 {
		 // 沿Y轴纵向遍历
		 for (i = u32IdxStart; i <= u32IdxEnd; i += k)
		 {
			 // 沿X轴横向遍历
			 k = 1;
			 while (i + k <= u32IdxEnd && paContourPoints[i].y == paContourPoints[i + k].y)
			 {
				 k++; // 寻找Y坐标相同的轮廓点
			 }
 
			 stPointTmp.y = paContourPoints[i].y;
			 for (j = 0; j < k; j++) // 当前遍历范围：[i, i+k)
			 {
				 if (j + 1 < k) // 至少有2个点
				 {
					 pPointCur = paContourPoints + i + j;
					 pPointNext = pPointCur + 1;
					 j++;                                                              // move j to pPointNext
					 while ((j + 1 < k) && (pPointNext->x - (pPointNext - 1)->x == 1)) // 从pPointCur->x到(pPointNext-1)->x为连续点
					 {
						 pPointNext++;
						 j++;
					 }
					 while (j + 1 < k) // 从pPointNext->x后也为连续点
					 {
						 stPointTmp.x = pPointNext->x + 1;
						 if ((pPointNext + 1)->x - pPointNext->x == 1 || 0 == get_pixel_val(pstBinary, &stPointTmp))
						 {
							 pPointNext++;
							 j++;
						 }
						 else
						 {
							 break;
						 }
					 }
					 memset((uint8_t*)pstBinary->pPlaneVir[0] + pstBinary->u32Stride[0] * pPointCur->y + pPointCur->x, 0xFF,
						 pPointNext->x - pPointCur->x + 1);
				 }
				 else // 仅剩下1个孤立的点
				 {
					 set_pixel_val(pstBinary, paContourPoints + i + j, 0xFF);
				 }
			 }
		 }
	 }
	 else if (CONTOUR_INSIDE_2_WHITE_VER == enClearMode) // 重置轮廓边缘及其内部所有像素为白色（0xFF），纵向遍历轮廓内部每个像素
	 {
		 // 沿X轴横向遍历
		 for (i = u32IdxStart; i <= u32IdxEnd; i += k)
		 {
			 // 沿Y轴纵向遍历
			 k = 1;
			 while (i + k <= u32IdxEnd && paContourPoints[i].x == paContourPoints[i + k].x)
			 {
				 k++; // 寻找X坐标相同的轮廓点
			 }
 
			 stPointTmp.x = paContourPoints[i].x;
			 for (j = 0; j < k; j++) // 当前遍历范围：[i, i+k)
			 {
				 if (j + 1 < k) // 至少有2个点
				 {
					 pPointCur = paContourPoints + i + j;
					 pPointNext = pPointCur + 1;
					 j++;                                                              // move j to pPointNext
					 while ((j + 1 < k) && (pPointNext->y - (pPointNext - 1)->y == 1)) // 从pPointCur->y到(pPointNext-1)->y为连续点
					 {
						 pPointNext++;
						 j++;
					 }
					 while (j + 1 < k) // 从pPointNext->y后也为连续点
					 {
						 stPointTmp.y = pPointNext->y + 1;
						 if ((pPointNext + 1)->y - pPointNext->y == 1 || 0 == get_pixel_val(pstBinary, &stPointTmp))
						 {
							 pPointNext++;
							 j++;
						 }
						 else
						 {
							 break;
						 }
					 }
 
					 for (y = pPointCur->y; y <= pPointNext->y; y++)
					 {
						 stPointTmp.y = y;
						 set_pixel_val(pstBinary, &stPointTmp, 0xFF);
					 }
				 }
				 else // 仅剩下1个孤立的点
				 {
					 set_pixel_val(pstBinary, paContourPoints + i + j, 0xFF);
				 }
			 }
		 }
	 }
	 else // 重置轮廓边缘像素为黑色（0）
	 {
		 for (i = u32IdxStart; i <= u32IdxEnd; i++)
		 {
			 set_pixel_val(pstBinary, paContourPoints + i, 0);
		 }
	 }
 
	 return;
 }
 
 /**
  * @fn      find_contours
  * @brief   查找二值图pstBinary中的轮廓，二值图外围需至少1Pixel的空白
  * @warning 该接口只能检测外轮廓，无法检测内轮廓（即轮廓内的凹洞）
  *
  * @param   [IN] pstNorm RAW图，用于统计轮廓的灰度均值
  * @param   [IN] pstBinary 二值图，背景为白色0xFF，内容为黑色0x0
  * @param   [IN] u32BinNeighbor 二值图外边界扩边，边界为白色，不小于1
  * @param   [IN] u32Norm2BinDsRatio 从RAW图到二值图的下采样比例，不小于1，1表示无下采样
  * @param   [IN] bClearMirco 是否清除小面积轮廓
  * @param   [IN] u32MircoTh 小面积轮廓阈值，小于该值则清为空白0xFF，bClearMirco为True时有效
  * @param   [OUT] astContAttr 各轮廓属性
  * @param   [IN/OUT] pu32ContourNum 输入为astContAttr数组个数，输出为实际检测出的轮廓数
  * @param   [OUT] aContourGrpRange 各轮廓在paContourPoints中的索引范围，即起始点与结束点
  * @param   [OUT] paContourPoints 轮廓点集，内存需外部申请，内存不小于二值图尺寸
  * @param   [OUT] pstBorderCircum 各轮廓的正交矩形包围盒，内存需外部申请
  *
  * @return  int32_t 查找到的轮廓数
  */
 static int32_t find_contours(XIMAGE_DATA_ST* pstNorm, XIMAGE_DATA_ST* pstBinary, uint32_t u32BinNeighbor, uint32_t u32Norm2BinDsRatio,
	 bool bClearMirco, uint32_t u32MircoTh, CONTOUR_ATTR astContAttr[], uint32_t* pu32ContourNum, GROUP_RANGE aContourGrpRange[],
	 POS_ATTR* paContourPoints, BORDER_CIRCUM* pstBorderCircum, RIGHT_ANGLE* pstRightAngle)
 {
	 uint32_t i = 0, j = 0, k = 0;
	 uint32_t u32ContourPointOffset = 0, u32ContourPointsNum = 0, u32ContourNum = 0;
	 int32_t s32ContourGrpIdx = 0;
	 uint32_t u32NbRight = 0, u32NbBottom = 0;
	 uint32_t u32ContourArea = 0, u32ContourAvg = 0; // 轮廓面积与灰度均值
	 uint8_t* pu8Line = NULL;
	 POS_ATTR pointLeft = { 0, 0 }, pointRight = { 0, 0 };
	 CONTOUR_ATTR* pstAttr = NULL;
 
	 if (NULL == pstBinary || NULL == astContAttr || NULL == pu32ContourNum ||
		 NULL == aContourGrpRange || NULL == paContourPoints || NULL == pstBorderCircum)
	 {
		 log_info("pstBinary(%p) OR astContAttr(%p) OR pu32ContourNum(%p) OR "
			 "aContourGrpRange(%p) OR paContourPoints(%p) OR pstBorderCircum(%p) OR is NULL\n",
			 pstBinary, astContAttr, pu32ContourNum, aContourGrpRange, paContourPoints, pstBorderCircum);
		 return -1;
	 }
	 if (0 == u32BinNeighbor || 0 == u32Norm2BinDsRatio)
	 {
		 log_info("the u32BinNeighbor(%u) OR u32Norm2BinDsRatio(%u) is ZERO\n", u32BinNeighbor, u32Norm2BinDsRatio);
		 return -1;
	 }
 
	 u32NbRight = pstBinary->u32Width - u32BinNeighbor;
	 if (NULL != pstNorm)
	 {
		 // 注：pstBinary中的u32Width和u32Height是不变的，存储的是申请二值图Buffer时的宽高
		 u32NbBottom = pstNorm->u32Height / u32Norm2BinDsRatio + u32BinNeighbor;
	 }
	 else
	 {
		 u32NbBottom = pstBinary->u32Height - u32BinNeighbor;
	 }
	 for (i = u32BinNeighbor; i < u32NbBottom; i++)
	 {
		 pu8Line = (uint8_t*)pstBinary->pPlaneVir[0] + pstBinary->u32Width * i;
		 for (j = u32BinNeighbor; j < u32NbRight; j++)
		 {
			 /* 像素点从白色变到黑色 */
			 if ((pu8Line[j - 1] == 0xFF) && (pu8Line[j] == 0)) // find the contour
			 {
				 set_position(&pointLeft, j - 1, i);
				 set_position(&pointRight, j, i); // 起始点
 
				 u32ContourPointsNum = follow_contour(pstBinary, &pointLeft, &pointRight,
					 paContourPoints + u32ContourPointOffset, s32ContourGrpIdx + 1, pstRightAngle);
				 if (u32ContourPointsNum > 0)
				 {
					 trace_border_ver(paContourPoints + u32ContourPointOffset, u32ContourPointsNum, &pstBorderCircum->stLeft, &pstBorderCircum->stRight,
						 pstNorm, pstBinary, u32BinNeighbor, u32Norm2BinDsRatio, &u32ContourAvg, &u32ContourArea, s32ContourGrpIdx + 1);
					 if (u32ContourArea < u32MircoTh) // 过滤太小区域
					 {
						 if (bClearMirco) // 清除轮廓，若保留，则会记录在aContourGrpRange中，但不会记录在astContAttr中
						 {
							 aContourGrpRange[s32ContourGrpIdx].u32IdxStart = u32ContourPointOffset;
							 aContourGrpRange[s32ContourGrpIdx].u32IdxEnd = u32ContourPointOffset + u32ContourPointsNum - 1;
							 clear_contour(pstBinary, s32ContourGrpIdx + 1, aContourGrpRange, paContourPoints, CONTOUR_INSIDE_2_WHITE_HOR);
							 continue;
						 }
					 }
					 else
					 {
						 trace_border_hor(paContourPoints + u32ContourPointOffset, u32ContourPointsNum, &pstBorderCircum->stTop, &pstBorderCircum->stBottom);
 
						 pstAttr = astContAttr + u32ContourNum++;
						 memset(pstAttr, 0, sizeof(CONTOUR_ATTR));
						 pstAttr->u32ContourNo = s32ContourGrpIdx + 1;
						 pstAttr->u32Area = u32ContourArea;
						 pstAttr->u32Avg = u32ContourAvg;
						 pstAttr->stBoundingBox.uiX = pstBorderCircum->stTop.u32HorStart;
						 pstAttr->stBoundingBox.uiY = pstBorderCircum->stLeft.u32VerStart;
						 pstAttr->stBoundingBox.uiWidth = pstBorderCircum->stTop.u32BorderLen;
						 pstAttr->stBoundingBox.uiHeight = pstBorderCircum->stLeft.u32BorderLen;
						 memcpy(&pstAttr->stStartPoint, &pointRight, sizeof(POS_ATTR));
					 }
 
					 // 记录每组轮廓的点集
					 aContourGrpRange[s32ContourGrpIdx].u32IdxStart = u32ContourPointOffset;
					 u32ContourPointOffset += u32ContourPointsNum;
					 aContourGrpRange[s32ContourGrpIdx].u32IdxEnd = u32ContourPointOffset - 1;
 
					 s32ContourGrpIdx++;
					 if (s32ContourGrpIdx >= XIMG_CONTOURS_GRP_IDX_MAX || u32ContourNum >= *pu32ContourNum)
					 {
						 log_info("contours are too many, ContourGrpIdx %d, ContourNum %u...\n", s32ContourGrpIdx, u32ContourNum);
						 goto END;
					 }
 
					 for (k = u32NbRight - 1; k > j; k--) // jump the current border
					 {
						 if (pu8Line[k] == pu8Line[j])
						 {
							 break;
						 }
					 }
					 j = k;
				 }
				 else
				 {
					 pu8Line[j] = 0xFF; // 置当前点为背景
				 }
			 }
			 /* 跳出内部出现异色区域 */
			 else if ((pu8Line[j - 1] == 0xFF) && (pu8Line[j] > 0 && pu8Line[j] < 0xFF)) // jump the known border
			 {
				 for (k = u32NbRight - 1; k > j; k--)
				 {
					 if (pu8Line[k] == pu8Line[j])
					 {
						 break;
					 }
				 }
				 j = k;
			 }
		 }
	 }
 
 END:
	 *pu32ContourNum = u32ContourNum;
	 return s32ContourGrpIdx;
 }
 
 /* 统计矩形边的相似程度 */
 static int32_t GetRectSimilarity(RECT* pRect, XIMAGE_DATA_ST* RectImage)
 {
	 uint8_t* pu8Now = NULL;
	 uint32_t RectNum = 0;
	 int32_t Ratio = 0;
	 for (uint32_t m = pRect->uiY; m < pRect->uiY + pRect->uiHeight; m++)
	 {
		 pu8Now = (uint8_t*)RectImage->pPlaneVir[0] + RectImage->u32Stride[0] * m;
		 for (uint32_t n = pRect->uiX; n < pRect->uiX + pRect->uiWidth; n++)
		 {
			 if (pu8Now[n] < 10)
			 {
				 RectNum++;
			 }
		 }
	 }
	 Ratio = RectNum * 100 / (pRect->uiHeight * pRect->uiWidth);
	 return Ratio;
 }
 
 static int32_t piechartConfSort(const void* pA, const void* pB) // 圆饼拟合置信度，从大到小排序
 {
	 return *(int32_t*)pB - *(int32_t*)pA;
 }
 
 static uint32_t calc_hor_continuous_max_len(uint16_t* line, uint32_t width, uint16_t th)
 {
	 uint32_t j = 0, k = 0;
	 uint32_t len = 0;
 
	 for (j = 0; j < width; j++)
	 {
		 if (line[j] < th) // 计数连续小于th的像素点数
		 {
			 k++;
		 }
		 else
		 {
			 len = ISL_MAX(len, k); // 取最大连续值
			 k = 0;
		 }
	 }
 
	 return ISL_MAX(len, k);
 }
 
 static uint32_t calc_ver_continuous_max_len(uint16_t* line[], uint32_t height, uint32_t col, uint16_t th)
 {
	 uint32_t j = 0, k = 0;
	 uint32_t len = 0;
 
	 for (j = 0; j < height; j++)
	 {
		 if (line[j][col] < th) // 计数连续小于th的像素点数
		 {
			 k++;
		 }
		 else
		 {
			 len = ISL_MAX(len, k); // 取最大连续值
			 k = 0;
		 }
	 }
 
	 return ISL_MAX(len, k);
 }
 
 /**
  * @fn      Isl_test4_locate_fast
  * @brief   快速定位Test4的边界区域,包括旋转矩形和正交矩形
  * @param   [IN] pstTest4 包含扩边的Test4图像，扩边需小于其所在水平或垂直方向实际长度的1/2
  * @param   [OUT] pstOrthogonalRect Test4正交矩形区域，不可为NULL
  * @param   [OUT] pstRotatedRect Test4旋转矩形区域，可为NULL，仅corner中的Y坐标有效
  */
 static void Isl_test4_locate_fast(XIMAGE_DATA_ST* pstTest4, RECT* pstOrthogonalRect, ROTATED_RECT* pstRotatedRect)
 {
	 uint32_t i = 0;
	 uint16_t** pu16Lines = (uint16_t**)malloc(pstTest4->u32Height * sizeof(uint16_t*));
	 const uint heth = 1000; // TEST4区域阈值
	 /* 传入第一平面高能数据 */
	 for (i = 0; i < pstTest4->u32Height; i++)
	 {
		 pu16Lines[i] = (uint16_t*)pstTest4->pPlaneVir[1] + pstTest4->u32Stride[1] * i;
	 }
 
	 // 简单但精确地计算TEST4区域的正交矩形几何信息
	 for (i = 0; i < pstTest4->u32Height; i++) // 上边界，从上往下找
	 {
		 if (calc_hor_continuous_max_len(pu16Lines[i], pstTest4->u32Width, heth) > (pstTest4->u32Width >> 2))
		 {
			 pstOrthogonalRect->uiY = i;
			 break;
		 }
	 }
 
	 for (i = pstTest4->u32Height - 1; i > pstOrthogonalRect->uiY; i--) // 下边界，从下往上找
	 {
		 if (calc_hor_continuous_max_len(pu16Lines[i], pstTest4->u32Width, heth) > (pstTest4->u32Width >> 2))
		 {
			 pstOrthogonalRect->uiHeight = i - pstOrthogonalRect->uiY + 1;
			 break;
		 }
	 }
 
	 for (i = 0; i < pstTest4->u32Width; i++) // 左边界，从左往右找
	 {
		 if (calc_ver_continuous_max_len(pu16Lines + pstOrthogonalRect->uiY, pstOrthogonalRect->uiHeight, i, heth) >
			 (pstOrthogonalRect->uiHeight >> 1))
		 {
			 pstOrthogonalRect->uiX = i;
			 break;
		 }
	 }
 
	 for (i = pstTest4->u32Width - 1; i > pstOrthogonalRect->uiX; i--) // 右边界，从右往左找
	 {
		 if (calc_ver_continuous_max_len(pu16Lines + pstOrthogonalRect->uiY, pstOrthogonalRect->uiHeight, i, heth) >
			 (pstOrthogonalRect->uiHeight >> 1))
		 {
			 pstOrthogonalRect->uiWidth = i - pstOrthogonalRect->uiX + 1;
			 break;
		 }
	 }
 
	 // 计算TEST4区域的旋转矩形几何信息
	 if (NULL != pstRotatedRect)
	 {
		 // 最多倾斜3°，tan3°，5%个像素左右
		 XIMG_BOX stBorder = { 0, 0, 0, 0 };
		 stBorder.top = pstOrthogonalRect->uiY * 2;
		 stBorder.bottom = ISL_SUB_SAFE((pstOrthogonalRect->uiY + pstOrthogonalRect->uiHeight) * 2, pstTest4->u32Height);
		 stBorder.left = pstOrthogonalRect->uiX + ISL_CEIL(pstOrthogonalRect->uiHeight, 20);
		 stBorder.right = ISL_SUB_SAFE(pstOrthogonalRect->uiX + pstOrthogonalRect->uiWidth, 1 + ISL_CEIL(pstOrthogonalRect->uiHeight, 20));
 
		 for (i = 0; i < (uint32_t)stBorder.bottom && i < pstTest4->u32Height; i++)
		 {
			 if (pu16Lines[i][stBorder.left] < heth)
			 {
				 pstRotatedRect->corner[RECT_CORNER_BOTTOMLEFT].y = i;
				 pstRotatedRect->corner[RECT_CORNER_BOTTOMLEFT].x = stBorder.left;
				 break;
			 }
		 }
		 for (i = 0; i < (uint32_t)stBorder.bottom && i < pstTest4->u32Height; i++)
		 {
			 if (pu16Lines[i][stBorder.right] < heth)
			 {
				 pstRotatedRect->corner[RECT_CORNER_BOTTOMRIGHT].y = i;
				 pstRotatedRect->corner[RECT_CORNER_BOTTOMRIGHT].x = stBorder.right;
				 break;
			 }
		 }
		 for (i = pstTest4->u32Height - 1; i > (uint32_t)stBorder.top; i--)
		 {
			 if (pu16Lines[i][stBorder.left] < heth)
			 {
				 pstRotatedRect->corner[RECT_CORNER_TOPLEFT].y = i;
				 pstRotatedRect->corner[RECT_CORNER_TOPLEFT].x = stBorder.left;
				 break;
			 }
		 }
		 for (i = pstTest4->u32Height - 1; i > (uint32_t)stBorder.top; i--)
		 {
			 if (pu16Lines[i][stBorder.right] < heth)
			 {
				 pstRotatedRect->corner[RECT_CORNER_TOPRIGHT].y = i;
				 pstRotatedRect->corner[RECT_CORNER_TOPRIGHT].x = stBorder.right;
				 break;
			 }
		 }
 
		 // 二次校验
		 int32_t hleft = pstRotatedRect->corner[RECT_CORNER_TOPLEFT].y - pstRotatedRect->corner[RECT_CORNER_BOTTOMLEFT].y;
		 int32_t hright = pstRotatedRect->corner[RECT_CORNER_TOPRIGHT].y - pstRotatedRect->corner[RECT_CORNER_BOTTOMRIGHT].y;
		 pstRotatedRect->height = pstRotatedRect->corner[RECT_CORNER_TOPRIGHT].y - pstRotatedRect->corner[RECT_CORNER_BOTTOMRIGHT].y;
		 pstRotatedRect->width = pstRotatedRect->corner[RECT_CORNER_BOTTOMRIGHT].x - pstRotatedRect->corner[RECT_CORNER_BOTTOMLEFT].x;
		 if (ISL_SUB_ABS(hleft, hright) <= 2 &&
			 ISL_SUB_ABS(hleft, pstOrthogonalRect->uiHeight) <= ISL_CEIL(pstOrthogonalRect->uiHeight, 20) &&
			 ISL_SUB_ABS(hright, pstOrthogonalRect->uiHeight) <= ISL_CEIL(pstOrthogonalRect->uiHeight, 20))
		 {
			 pstRotatedRect->angle = atan(((double)pstRotatedRect->corner[RECT_CORNER_BOTTOMRIGHT].y -
				 (double)pstRotatedRect->corner[RECT_CORNER_BOTTOMLEFT].y) /
				 (double)pstOrthogonalRect->uiWidth) *
				 180.0 / M_PI;
			 pstRotatedRect->width = (int32_t)(cos(pstRotatedRect->angle) * pstOrthogonalRect->uiWidth + 0.5);
		 }
	 }
	 // 释放动态分配的内存
	 free(pu16Lines);
	 return;
 }
 
 /**
  * @fn      Isl_column_avg_statistic
  * @brief   列均值统计，仅支持16Bit Raw数据
  *
  * @param   [IN] pstImg 图像数据
  * @param   [IN] u32PlaneIdx 需统计的场索引
  * @param   [IN] pstArea 统计区域
  * @param   [OUT] au32Avg 列均值
  */
 static int32_t Isl_column_avg_statistic(XIMAGE_DATA_ST* pstImg, uint32_t u32PlaneIdx, RECT* pstArea, uint32_t au32Avg[], ROTATED_RECT* pRotatedRect)
 {
	 int32_t i = 0, j = 0;
	 uint32_t k = 0;
	 XIMG_BOX statArea = { 0, 0, 0, 0 };
	 uint16_t* pu16Line = NULL;
 
	 if (NULL == pstImg || (pstImg->enImgFmt & ISL_IMG_DATFMT_XRAY_MASK) == 0)
	 {
		 return ISL_FAIL;
	 }
	 if (NULL == pstImg->pPlaneVir[u32PlaneIdx] || 0 == pstImg->u32Stride[u32PlaneIdx])
	 {
		 return ISL_FAIL;
	 }
 
	 if (NULL == pRotatedRect)
	 {
		 statArea.top = pstArea->uiY;
		 statArea.bottom = statArea.top + pstArea->uiHeight;
		 statArea.left = pstArea->uiX;
		 statArea.right = pstArea->uiX + pstArea->uiWidth;
		 memset(au32Avg, 0, sizeof(uint32_t) * pstArea->uiWidth);
		 for (i = statArea.top; i < statArea.bottom; i++)
		 {
			 pu16Line = (uint16_t*)pstImg->pPlaneVir[u32PlaneIdx] + pstImg->u32Stride[u32PlaneIdx] * i;
			 for (j = statArea.left, k = 0; j < statArea.right; j++, k++)
			 {
				 au32Avg[k] += pu16Line[j];
			 }
		 }
 
		 for (k = 0; k < pstArea->uiWidth; k++)
		 {
			 au32Avg[k] = (au32Avg[k] + pstArea->uiHeight / 2) / pstArea->uiHeight;
		 }
	 }
	 /* 旋转矩形的列均值统计 */
	 else
	 {
		 /* 求解直线斜率和截距 */
		 double dk = (double)(pRotatedRect->corner[1].y - pRotatedRect->corner[0].y) /
			 (double)(pRotatedRect->corner[1].x - pRotatedRect->corner[0].x);
		 double db = (double)pRotatedRect->corner[0].y - (double)(k * pRotatedRect->corner[0].x);
		 int64_t llSumAvg = 0;
		 /* 这里的top是矩形的左上角的点 */
		 statArea.left = pstArea->uiX;
		 statArea.right = pstArea->uiX + pstArea->uiWidth;
 
		 log_info("left[%d] right[%d] uiHeight[%d] base[%d] kb(%f %f)\n", statArea.left, statArea.right, pstArea->uiHeight, pstArea->uiY, dk, db);
		 memset(au32Avg, 0, sizeof(uint32_t) * (pstArea->uiWidth));
		 for (i = statArea.left, k = 0; i < statArea.right; i++, k++)
		 {
			 for (j = 0; j < (int32_t)pstArea->uiHeight; j++)
			 {
				 pu16Line = (uint16_t*)pstImg->pPlaneVir[u32PlaneIdx] + pstImg->u32Stride[u32PlaneIdx] * ((int32_t)(dk * i + db) + j + pstArea->uiY);
				 llSumAvg += pu16Line[i];
			 }
			 au32Avg[k] = (uint32_t)(llSumAvg / pstArea->uiHeight);
			 llSumAvg = 0;
			 log_info("au32Avg[%d][%d] is %d \n ", i, ((int32_t)(dk * i + db) + pstArea->uiY), au32Avg[k]);
		 }
	 }
 
	 return ISL_OK;
 }
 
 /**
  * @fn      Isl_win_avg_statistic
  * @brief   列均值统计，仅支持16Bit Raw数据
  *
  * @param   [IN] pstImg 图像数据
  * @param   [IN] u32PlaneIdx 需统计的场索引
  * @param   [IN] pstArea 统计区域
  * @param   [OUT] au32Avg 列均值
  */
 static uint32_t Isl_win_avg_statistic(XIMAGE_DATA_ST* pstImg, uint32_t u32PlaneIdx, RECT* pstArea, uint32_t u32WinWidth, uint32_t u32Gap, uint32_t au32Avg[])
 {
	 int32_t i = 0, j = 0, k = 0, m = 0;
	 uint32_t u32StatNum = 0;
	 std::vector<uint32_t> au32Col(pstArea->uiWidth);
 
	 if (0 == u32WinWidth || u32WinWidth > pstArea->uiWidth)
	 {
		 return 0;
	 }
 
	 if (ISL_OK != Isl_column_avg_statistic(pstImg, u32PlaneIdx, pstArea, au32Col.data(), NULL))
	 {
		 return 0;
	 }
 
	 u32Gap += 1;
	 u32StatNum = (pstArea->uiWidth - u32WinWidth + 1) / u32Gap;
	 memset(au32Avg, 0, sizeof(uint32_t) * u32StatNum);
 
	 for (i = 0; i < (int32_t)u32WinWidth; i++)
	 {
		 au32Avg[0] += au32Col[i];
	 }
	 for (k = 1; k < (int32_t)u32StatNum; k++)
	 {
		 au32Avg[k] = au32Avg[k - 1];
		 for (j = 0, i = k * u32Gap - 1, m = (int32_t)((k - 1) * u32Gap + u32WinWidth); j < (int32_t)u32Gap; j++)
		 {
			 au32Avg[k] -= au32Col[i - j];
			 au32Avg[k] += au32Col[m + j];
		 }
	 }
	 for (k = 0; k < (int32_t)u32StatNum; k++)
	 {
		 au32Avg[k] = (uint32_t)((au32Avg[k] + u32WinWidth / 2) / u32WinWidth);
	 }
 
	 return u32StatNum;
 }
 
 /**
  * @fn      Isl_test4_renormalize
  * @brief   对Test4区域低能+高能图像做二次归一化处理
  *            中间主体区域以Test4的1/8 ~ 2/8行、6/8 ~ 7/8行的列均值为模板
  *            因边缘斜穿，两侧边缘区域以Test4的2/8 ~ 3/8行、5/8 ~ 6/8行的列均值为模板
  *
  * @param   [IN] pstTest4 Test4图像，0场为低能数据，1场为高能数据
  * @param   [IN] pstAccurArea Test4实际处理区域
  * @param   [OUT] pstRenorm 二次归一化处理厚的图像，可为NULL，若为NULL，增强后图像直接输出到pstTest4上
  * @param   [OUT] au32HeCol 实际处理区域高能图像的列均值模板，可为NULL
  * @param   [IN] pRotatedRect 若有输入则为旋转角度较大的矩形
  *
  * @return  uint32_t Test4高能图像的背景均值
  */
 uint32_t Isl_test4_renormalize(XIMAGE_DATA_ST* pstTest4, RECT* pstAccurArea, XIMAGE_DATA_ST* pstRenorm, uint32_t au32HeCol[], ROTATED_RECT* pRotatedRect)
 {
	 uint32_t j = 0, k = 0;
	 uint32_t i = 0;
	 uint16_t* pu16Src = NULL, * pu16Dst = NULL;
	 uint32_t offset[2] = { 0 }, margin = pstAccurArea->uiWidth >> 4;
	 RECT statArea = { 0, 0, 0, 0 };
	 statArea.uiHeight = pstAccurArea->uiHeight >> 3;
 
	 if (NULL == pRotatedRect)
	 {
		 vector<uint32_t> au32ColAvg1(pstAccurArea->uiWidth);
		 vector<uint32_t> au32ColAvg2(pstAccurArea->uiWidth);
		 for (k = 0; k < 2; k++) // k：场索引
		 {
			 // 中间主体区域，2/8 ~ 6/8 区域作为模板
			 statArea.uiWidth = pstAccurArea->uiWidth - margin * 2;
			 statArea.uiX = pstAccurArea->uiX + margin;
			 statArea.uiY = pstAccurArea->uiY + (pstAccurArea->uiHeight >> 3);
			 Isl_column_avg_statistic(pstTest4, k, &statArea, au32ColAvg1.data() + margin, NULL); // 1/8 ~ 2/8
			 statArea.uiY = pstAccurArea->uiY + (pstAccurArea->uiHeight * 6 >> 3);
			 Isl_column_avg_statistic(pstTest4, k, &statArea, au32ColAvg2.data() + margin, NULL); // 6/8 ~ 7/8
 
			 // 两侧边缘区域
			 statArea.uiWidth = margin;
			 statArea.uiX = pstAccurArea->uiX;
			 statArea.uiY = pstAccurArea->uiY + (pstAccurArea->uiHeight * 2 >> 3);
			 Isl_column_avg_statistic(pstTest4, k, &statArea, au32ColAvg1.data(), NULL); // 2/8 ~ 3/8
			 statArea.uiY = pstAccurArea->uiY + (pstAccurArea->uiHeight * 5 >> 3);
			 Isl_column_avg_statistic(pstTest4, k, &statArea, au32ColAvg2.data(), NULL); // 5/8 ~ 6/8
 
			 statArea.uiX = pstAccurArea->uiX + pstAccurArea->uiWidth - margin;
			 statArea.uiY = pstAccurArea->uiY + (pstAccurArea->uiHeight * 2 >> 3);
			 Isl_column_avg_statistic(pstTest4, k, &statArea, au32ColAvg1.data() + (pstAccurArea->uiWidth - margin), NULL); // 2/8 ~ 3/8
			 statArea.uiY = pstAccurArea->uiY + (pstAccurArea->uiHeight * 5 >> 3);
			 Isl_column_avg_statistic(pstTest4, k, &statArea, au32ColAvg2.data() + (pstAccurArea->uiWidth - margin), NULL); // 1/8 ~ 2/8
 
			 // 这里选取图像偏向中心位置为均值模版
			 for (i = pstAccurArea->uiWidth / 8, offset[k] = 0; i < pstAccurArea->uiWidth * 7 / 8; i++)
			 {
				 au32ColAvg1[i] = (au32ColAvg1[i] + au32ColAvg2[i] + 1) >> 1;
				 offset[k] += au32ColAvg1[i];
			 }
			 offset[k] /= (pstAccurArea->uiWidth * 3 / 4);
			 if (k == 1 && NULL != au32HeCol)
			 {
				 memcpy(au32HeCol, au32ColAvg1.data(), au32ColAvg1.size() * sizeof(uint32_t));
			 }
 
			 for (j = 0; j < pstAccurArea->uiHeight; j++)
			 {
				 pu16Src = (uint16_t*)pstTest4->pPlaneVir[k] + pstTest4->u32Stride[k] * (pstAccurArea->uiY + j) + pstAccurArea->uiX;
				 if (NULL != pstRenorm)
				 {
					 pu16Dst = (uint16_t*)pstRenorm->pPlaneVir[k] + pstRenorm->u32Stride[k] * (pstAccurArea->uiY + j) + pstAccurArea->uiX;
				 }
				 else
				 {
					 pu16Dst = pu16Src;
				 }
 
				 for (i = 0; i < pstAccurArea->uiWidth; i++)
				 {
					 pu16Dst[i] = pu16Src[i] + offset[k];
					 pu16Dst[i] = ISL_SUB_SAFE(pu16Dst[i], au32ColAvg1[i]);
				 }
			 }
		 }
	 }
	 else
	 {
		 /* 求解直线斜率和截距 */
		 float32_t dk = (float32_t)((double)(pRotatedRect->corner[1].y - pRotatedRect->corner[0].y) /
			 (double)(pRotatedRect->corner[1].x - pRotatedRect->corner[0].x));
		 float32_t db = (float32_t)((double)pRotatedRect->corner[0].y - (double)(k * pRotatedRect->corner[0].x));
 
		 pRotatedRect->width = pRotatedRect->corner[1].x - pRotatedRect->corner[0].x;
		 //检查pRotatedRect->width为非负数
		 if (pRotatedRect->width < 0)
		 {
			 return 100;
		 }
		 vector<uint32_t> au32ColAvg1(pRotatedRect->width);
		 vector<uint32_t> au32ColAvg2(pRotatedRect->width);
		 margin = pRotatedRect->width >> 4;
		 statArea.uiHeight = pRotatedRect->height >> 3;
		 for (k = 0; k < 2; k++) // k：场索引
		 {
			 // 中间主体区域，2/8 ~ 6/8 区域作为模板
			 statArea.uiWidth = pRotatedRect->width - margin * 2;
			 statArea.uiX = pRotatedRect->corner[0].x + margin;
			 statArea.uiY = pRotatedRect->height >> 3;
			 Isl_column_avg_statistic(pstTest4, k, &statArea, au32ColAvg1.data() + margin, pRotatedRect); // 1/8 ~ 2/8
			 statArea.uiY = (pRotatedRect->height * 6 >> 3);
			 Isl_column_avg_statistic(pstTest4, k, &statArea, au32ColAvg2.data() + margin, pRotatedRect); // 6/8 ~ 7/8
 
			 // 两侧边缘区域
			 statArea.uiWidth = margin;
			 statArea.uiX = pRotatedRect->corner[0].x;
			 statArea.uiY = (pRotatedRect->height * 2 >> 3);
			 Isl_column_avg_statistic(pstTest4, k, &statArea, au32ColAvg1.data(), pRotatedRect); // 2/8 ~ 3/8
			 statArea.uiY = (pRotatedRect->height * 5 >> 3);
			 Isl_column_avg_statistic(pstTest4, k, &statArea, au32ColAvg2.data(), pRotatedRect); // 5/8 ~ 6/8
 
			 statArea.uiX = pRotatedRect->corner[0].x + pRotatedRect->width - margin;
			 statArea.uiY = (pRotatedRect->height * 2 >> 3);
			 Isl_column_avg_statistic(pstTest4, k, &statArea, au32ColAvg1.data() + (pRotatedRect->width - margin), pRotatedRect); // 2/8 ~ 3/8
			 statArea.uiY = (pRotatedRect->height * 5 >> 3);
			 Isl_column_avg_statistic(pstTest4, k, &statArea, au32ColAvg2.data() + (pRotatedRect->width - margin), pRotatedRect); // 1/8 ~ 2/8
 
			 for (i = 0, offset[k] = 0; i < (uint32_t)pRotatedRect->width; i++)
			 {
				 au32ColAvg1[i] = (au32ColAvg1[i] + au32ColAvg2[i] + 1) >> 1;
				 offset[k] += au32ColAvg1[i];
			 }
			 offset[k] /= pRotatedRect->width;
			 if (k == 1 && NULL != au32HeCol)
			 {
				 memcpy(au32HeCol, au32ColAvg1.data(), au32ColAvg1.size() * sizeof(uint32_t));
			 }
 
			 for (i = pRotatedRect->corner[0].x; i < (uint32_t)pRotatedRect->corner[1].x; i++)
			 {
				 for (j = 0; j < (uint32_t)(pRotatedRect->height); j++)
				 {
					 pu16Dst = (uint16_t*)pstTest4->pPlaneVir[k] + pstTest4->u32Stride[k] * ((int32_t)(dk * (int32_t)i + db) + j);
					 pu16Dst[i] = pu16Dst[i] + offset[k];
					 pu16Dst[i] = ISL_SUB_SAFE(pu16Dst[i], au32ColAvg1[i - pRotatedRect->corner[0].x]);
				 }
			 }
		 }
	 }
	 // 这里的offset需要根据当前的融合规则重新进行处理
	 // float32_t fHLRatio = (float32_t)offset[1] / 65535.0f;
	 // fHLRatio = fHLRatio * fHLRatio;
	 // int32_t fResult = (int32_t)(offset[0] * fHLRatio + offset[1] * (1.0f - fHLRatio));
	 return offset[1];
 }
 
 /**
  * @fn      Isl_test4_local_hist_equal
  * @brief   对Test4分PIE_charT_NUM_IN_TEST4个区域做直方图均衡化处理
  *
  * @param   [IN] pstTest4 Test4图像，0场为低能数据，1场为高能数据，但仅对高能数据做处理
  * @param   [IN] pstProcArea 处理区域，横向区域均分为PIE_CHART_NUM_IN_TEST4个区域
  * @param   [IN] u32RoundAvg 处理区域大致的亮度均值，此值作为直方图统计的中值
  * @param   [OUT] pstLhe 直方图均衡化处理后的高能图，若为NULL，则直接输出到pstTest4原图中
  *
  * @return  uint32_t 直方图统计的最小值
  */
 static uint32_t Isl_test4_local_hist_equal(XIMAGE_DATA_ST* pstTest4, RECT* pstProcArea, uint32_t u32RoundAvg, XIMAGE_DATA_ST* pstLhe, int32_t* pWindowCenter)
 {
	 uint32_t i = 0, j = 0, k = 0;
	 uint16_t* pu16Src = NULL, * pu16Dst = NULL, * pu16Plane = NULL;
	 uint32_t u32Stride = 0, sum = 0;
	 uint32_t histBins = (u32RoundAvg > 256) ? 512 : 256;
	 std::vector<uint32_t> hist(histBins), cdf(histBins), lut(histBins);
	 uint32_t histMin = ISL_SUB_SAFE(u32RoundAvg, histBins / 2);
	 uint32_t histMax = histMin + histBins - 1;
	 uint32_t procW = pstProcArea->uiWidth / PIE_CHART_NUM_IN_TEST4;
	 static uint32_t procX = pstProcArea->uiX;
	 procX = pstProcArea->uiX;
 
	 if (NULL == pstLhe) // 直接使用原图的高能场
	 {
		 pu16Plane = (uint16_t*)pstTest4->pPlaneVir[1];
		 u32Stride = pstTest4->u32Stride[1];
	 }
	 else
	 {
		 pu16Plane = (uint16_t*)pstLhe->pPlaneVir[0];
		 u32Stride = pstLhe->u32Stride[0];
	 }
 
	 for (k = 0; k < PIE_CHART_NUM_IN_TEST4; k++)
	 {
		 hist.assign(hist.size(), 0); // 将hist容器的所有元素都设置为0
		 cdf.assign(cdf.size(), 0);   // 将cdf容器的所有元素都设置为0
		 sum = 0;
 
		 // 统计区域直方图
		 for (i = 0; i < pstProcArea->uiHeight; i++)
		 {
			 pu16Src = (uint16_t*)pstTest4->pPlaneVir[1] + pstTest4->u32Stride[1] * (pstProcArea->uiY + i) + procX;
			 pu16Dst = pu16Plane + u32Stride * (pstProcArea->uiY + i) + procX;
			 for (j = 0; j < procW; j++)
			 {
				 pu16Dst[j] = ISL_CLIP(pu16Src[j], histMin, histMax) - histMin;
				 hist[pu16Dst[j]] = hist[pu16Dst[j]] + 1;
			 }
		 }
 
		 // 统计累计分布
		 for (i = 0; i < histBins; i++)
		 {
			 sum += hist[i];
			 cdf[i] = sum;
		 }
 
		 // 统计分布起始点，并生成映射曲线
		 for (j = 0; j < histBins; j++)
		 {
			 lut[j] = histMin;
			 if (cdf[j] > 0)
			 {
				 break;
			 }
		 }
		 for (i = j + 1; i < histBins; i++)
		 {
			 lut[i] = (histBins - 1) * (cdf[i] - cdf[j]) / (sum - cdf[j]); // + histMin;
		 }
 
		 // 重新生成图像
		 for (i = 0; i < pstProcArea->uiHeight; i++)
		 {
			 pu16Dst = pu16Plane + u32Stride * (pstProcArea->uiY + i) + procX;
			 for (j = 0; j < procW; j++)
			 {
				 pu16Dst[j] = lut[pu16Dst[j]];
			 }
		 }
 
		 // 修复直方图均衡空白间隙
		 procX = procX + pstProcArea->uiWidth / PIE_CHART_NUM_IN_TEST4;
	 }
 
	 *pWindowCenter = u32RoundAvg;
	 return histMin;
 }
 
 /**
  * @fn      Isl_test4_pie_gray_statistic
  * @brief   Test4中圆饼的灰度统计，注：圆饼区域为椭圆
  *
  * @param   [IN] pstTest4 Test4高能图像，若为LHZ格式，仅对1场高能数据统计，若为单平面SP16格式，则统计第0场
  * @param   [IN] pstCenter 基于pstTest4图像的圆饼中心点坐标
  * @param   [IN] semiMajorAxis 横向长半轴长度
  * @param   [IN] semiMinorAxis 纵向短半轴长度
  * @param   [OUT] innerAvg 圆饼4个缺口朝向的内均值，数组类型，元素个数为PIE_ORIENTATIONS_NUM，可为NULL
  * @param   [OUT] outerAvg 圆饼4个缺口朝向的外均值，数组类型，元素个数为PIE_ORIENTATIONS_NUM，可为NULL
  * @param   [IN/OUT] orientation 若输入为无效值不小于PIE_ORIENTATIONS_NUM，则输出最有可能的圆饼缺口朝向
  *                               若输入为有效值小于PIE_ORIENTATIONS_NUM，函数内部若计算得到的缺口方向
  *                                 与该输入值不一致，则直接返回，同时置信度置为0
  *
  * @return  uint32_t 圆饼及缺口朝向的置信度，千分之表示，大于1000较为可信
  */
 static uint32_t Isl_test4_pie_gray_statistic(XIMAGE_DATA_ST* pstTest4, POS_ATTR* pstCenter, int32_t semiMajorAxis, int32_t semiMinorAxis,
	 uint32_t* innerAvg, uint32_t* outerAvg, PIE_CAHRT_ORIENT* orientation)
 {
	 int32_t k = 0, m = 0, n = 0, j = 0;
	 uint32_t u32MajorAxis = semiMajorAxis * 2 + 1; // 长轴长度
	 uint32_t u32MinorAxis = semiMinorAxis * 2 + 1; // 短轴长度
	 uint32_t u32SemiMajorExt = semiMajorAxis + ISL_CEIL(semiMajorAxis, 6);
	 uint32_t u32SemiMinorExt = semiMinorAxis + ISL_CEIL(semiMinorAxis, 6);
	 uint32_t u32SemiChord = 0;                         // 半弦（平行于长轴）长度
	 uint32_t u32QtrInnerArea = 0, u32QtrOuterArea = 0; // 1/4个圆饼面积，不包括轴
	 uint32_t u32StartX = 0, u32Stride = 0;
	 double f64SemiMinorAxis = (double)u32MinorAxis / 2.0;                  // 短半轴长度，浮点型用于计算弦长度
	 double f64SemiMinorAxisSq = (double)u32MinorAxis * u32MinorAxis / 4.0; // 短半轴长度的平方，用于计算弦长度
	 uint32_t avgInner[PIE_ORIENTATIONS_NUM] = { 0 }, avgOuter[PIE_ORIENTATIONS_NUM] = { 0 };
	 uint16_t* pu16Plane = NULL, * pu16Line = NULL;
	 uint32_t u32MinQtr = 0, u32MaxQtr = 0;
	 PIE_CAHRT_ORIENT u32MaxIdx = PIE_ORIENT_TOPLEFT;
	 uint32_t u32InnerAvg = 0, u32InnerStdev = 0, u32OuterAvg = 0, u32OuterStdev = 0;
	 uint32_t confidence = 0;
 
	 /* 参数校验 */
	 if ((pstCenter->x) <= (uint32_t)u32SemiMajorExt)
	 {
		 return 0;
	 }
 
	 if (pstTest4->enImgFmt == ISL_IMG_DATFMT_LHP || pstTest4->enImgFmt == ISL_IMG_DATFMT_LHZP ||
		 pstTest4->enImgFmt == ISL_IMG_DATFMT_LHZ16P) // 取第1场高能数据
	 {
		 pu16Plane = (uint16_t*)pstTest4->pPlaneVir[1];
		 u32Stride = pstTest4->u32Stride[1];
	 }
	 else if (pstTest4->enImgFmt == ISL_IMG_DATFMT_SP16)
	 {
		 pu16Plane = (uint16_t*)pstTest4->pPlaneVir[0];
		 u32Stride = pstTest4->u32Stride[0];
	 }
	 else
	 {
		 return 0;
	 }
 
	 for (j = 1; j <= semiMinorAxis; j++)
	 {
		 u32SemiChord = (uint32_t)(sqrt(f64SemiMinorAxisSq - j * j) * ((double)u32MajorAxis / 2.0) / f64SemiMinorAxis); // 半弦长度
		 u32QtrInnerArea += u32SemiChord;
		 u32QtrOuterArea += u32SemiMajorExt - u32SemiChord;
		 for (n = -1; n < 2; n += 2) // n: {-1, 1}，短半轴两边
		 {
			 // 圆饼区域边界检查,防止超出边界等崩溃问题
			 if ((((int32_t)pstCenter->y + j * n) > (int32_t)pstTest4->u32Height) || (((int32_t)pstCenter->y + j * n) < 0))
			 {
				 return 0;
			 }
			 pu16Line = pu16Plane + u32Stride * ((int32_t)pstCenter->y + j * n);
			 for (k = -1; k < 2; k += 2) // k: {-1, 1}，长半轴两边
			 {
				 u32StartX = (k < 0) ? (pstCenter->x - u32SemiMajorExt) : (pstCenter->x + 1);
				 for (m = 0; m < (int32_t)u32SemiMajorExt; m++)
				 {
					 // 圆饼区域边界检查,防止超出边界等崩溃问题
					 if (((u32StartX + m) > pstTest4->u32Width) || (((int32_t)u32StartX + m) < 0))
					 {
						 return 0;
					 }
					 if (k < 0) // 左半轴
					 {
						 // 左上 & 左下
						 if (m < (int32_t)(u32SemiMajorExt - u32SemiChord))
						 {
							 avgOuter[n + 1] += *(pu16Line + u32StartX + m);
						 }
						 else
						 {
							 avgInner[n + 1] += *(pu16Line + u32StartX + m);
						 }
					 }
					 else // 右半轴
					 {
						 // 右上 & 右下
						 if (m < (int32_t)u32SemiChord)
						 {
							 avgInner[n + 2] += *(pu16Line + u32StartX + m);
						 }
						 else
						 {
							 avgOuter[n + 2] += *(pu16Line + u32StartX + m);
						 }
					 }
				 }
			 }
		 }
	 }
	 for (j = semiMinorAxis + 1; j <= (int32_t)u32SemiMinorExt; j++)
	 {
		 u32QtrOuterArea += u32SemiMajorExt;
		 for (n = -1; n < 2; n += 2) // n: {-1, 1}，短半轴两边
		 {
			 // 左半轴
			 pu16Line = pu16Plane + u32Stride * ((int32_t)pstCenter->y + j * n) + (pstCenter->x - u32SemiMajorExt);
			 for (m = 0; m < (int32_t)u32SemiMajorExt; m++)
			 {
				 avgOuter[n + 1] += *(pu16Line + m);
			 }
 
			 // 右半轴
			 pu16Line = pu16Plane + u32Stride * ((int32_t)pstCenter->y + j * n) + (pstCenter->x + 1);
			 for (m = 0; m < (int32_t)u32SemiMajorExt; m++)
			 {
				 avgOuter[n + 2] += *(pu16Line + m);
			 }
		 }
	 }
	 for (j = 0; j < PIE_ORIENTATIONS_NUM; j++)
	 {
		 avgInner[j] /= u32QtrInnerArea;
		 avgOuter[j] /= u32QtrOuterArea;
	 }
 
	 u32MinQtr = avgInner[0];
	 u32MaxQtr = avgInner[0];
	 u32OuterAvg = avgOuter[0];
	 for (j = 1; j < PIE_ORIENTATIONS_NUM; j++)
	 {
		 if (u32MinQtr > avgInner[j])
		 {
			 u32MinQtr = avgInner[j];
		 }
		 else if (u32MaxQtr < avgInner[j])
		 {
			 u32MaxQtr = avgInner[j];
			 u32MaxIdx = (PIE_CAHRT_ORIENT)j;
		 }
		 u32OuterAvg += avgOuter[j];
	 }
 
	 u32OuterAvg /= PIE_ORIENTATIONS_NUM;
	 if (NULL != outerAvg)
	 {
		 *outerAvg = u32OuterAvg;
	 }
	 if (*orientation < PIE_ORIENTATIONS_NUM)
	 {
		 if (*orientation != (int32_t)u32MaxIdx)
		 {
			 if (NULL != innerAvg)
			 {
				 *innerAvg = 0;
			 }
			 return 0;
		 }
	 }
	 else // orientation为非法值，先赋值再计算
	 {
		 *orientation = u32MaxIdx;
	 }
 
	 uint32_t idxIps = (u32MaxIdx + 2) % PIE_ORIENTATIONS_NUM;                                                          // 同侧索引
	 uint32_t idxOps[2] = { (u32MaxIdx + 1) % (uint32_t)PIE_ORIENTATIONS_NUM, (u32MaxIdx + 3) % (uint32_t)PIE_ORIENTATIONS_NUM }; // 对侧索引
	 double numerator = 0.0, denominator = 0.0;
	 for (j = 0; j < PIE_ORIENTATIONS_NUM; j++)
	 {
		 u32OuterStdev += ISL_SUB_ABS(avgOuter[j], u32OuterAvg) * ISL_SUB_ABS(avgOuter[j], u32OuterAvg);
		 if (j != u32MaxIdx)
		 {
			 u32InnerAvg += avgInner[j];
		 }
	 }
	 u32InnerAvg /= PIE_ORIENTATIONS_NUM - 1;
	 if (NULL != innerAvg)
	 {
		 *innerAvg = u32InnerAvg;
	 }
	 u32InnerStdev = ISL_SUB_ABS(avgInner[idxOps[0]], avgInner[idxOps[1]]) / 2;
	 u32OuterStdev = (uint32_t)sqrt((double)u32OuterStdev / PIE_ORIENTATIONS_NUM);
 
	 numerator = (double)(u32MaxQtr - u32InnerAvg) * 0.7 / u32InnerAvg + (double)(u32MaxQtr - u32MinQtr) * 0.3 / u32MinQtr;
	 denominator = (double)u32OuterStdev * 0.2 / u32OuterAvg + (double)u32InnerStdev * 0.8 * 2.0 / (avgInner[idxOps[0]] + avgInner[idxOps[1]]);
 
	 if (u32MaxQtr * 100 > avgInner[idxIps] * 105 && avgOuter[idxIps] > avgInner[idxIps] &&
		 avgOuter[idxOps[0]] > avgInner[idxOps[0]] && avgOuter[idxOps[1]] > avgInner[idxOps[1]])
	 {
		 confidence = (uint32_t)(numerator / pow(denominator, 0.3) * 1000);
	 }
 
	 // log_info("center (%u, %u), Axis %d %d -> %u %u, Inner %3u [%3u %3u %3u %3u], Outer %3u [%3u %3u %3u %3u], confidence %u(%f %f)\n",
	 // 	pstCenter->x, pstCenter->y, semiMajorAxis, semiMinorAxis, u32SemiMajorExt, u32SemiMinorExt,
	 // 	u32InnerAvg, avgInner[0], avgInner[1], avgInner[2], avgInner[3],
	 // 	u32OuterAvg, avgOuter[0], avgOuter[1], avgOuter[2], avgOuter[3], confidence, numerator, denominator);
 
	 return confidence;
 }
 
 /**
  * @fn      Isl_test4_create_mask
  * @brief   创建圆饼模板，圆饼置信度越高，圆饼模板与背景对比度越高，圆饼区域越暗
  *
  * @param   [OUT] maskImg 圆饼模板图像，取0场uchar格式图像
  * @param   [IN] center 基于pstTest4图像的圆饼中心点坐标
  * @param   [IN] semiMajor 横向长半轴长度
  * @param   [IN] semiMinor 纵向短半轴长度
  * @param   [IN] confidence 圆饼及缺口朝向的置信度，千分之表示
  * @param   [IN] orientation 圆饼缺口朝向，当置信度低于一定值时，强制朝向为未知
  * @param   [IN] testAEnhanceGrade 增强等级 用来控制蒙版的灰度
  * @param   [IN] bThintoThick TEST4厚薄方向
  * @param   [IN] ConfirmNum 单侧TEST4圆饼确认数量
  * @param   [IN] uiProcWidth 处理的区域宽度
 
  */
 static void Isl_test4_create_mask(XIMAGE_DATA_ST* maskImg, POS_ATTR* center, uint32_t semiMajor, uint32_t semiMinor,
	 uint32_t confidence, PIE_CAHRT_ORIENT orientation, int32_t testAEnhanceGrade, bool bThintoThick, int32_t ConfirmNum, uint32_t uiProcWidth)
 {
	 int32_t i = 0, j = 0, n = 0;
	 int32_t k = 0; // 缺口边缘随机噪声
	 uint32_t u32MajorAxis = 0, u32MinorAxis = 0, u32SemiChord = 0, u32DiffLen = 0;
	 double f64SemiMinorAxis = 0.0, f64SemiMinorAxisSq = 0.0;
	 const int32_t wtRange[4] = { 0xFF, 0xE0, 0x80, 0x40 }; // 不同置信度对应的mask区域值，置信度越大，该值越低，圆饼越明显
	 int32_t confRange[4] = { 0, 500 + 10 * (100 - testAEnhanceGrade),
						 800 + 10 * (100 - testAEnhanceGrade), 1800 + 10 * (100 - testAEnhanceGrade) }; // 置信度分段
	 uint8_t u8PieValue = 0, u8AroundValue = 0;
	 uint8_t* pu8Line = NULL, * pu8Plane = (uint8_t*)maskImg->pPlaneVir[0];
	 uint32_t stride = maskImg->u32Stride[0];
	 std::vector<uint32_t> au32SemiChord(semiMinor * 3 / 2 + 2); // 半弦长，索引顺序为从中心向边界
	 double f32weight = 1.2;
	 uint16_t u16PieValue = 0;
 
	 /* 对参数进行初步校验，并且去除独立位于TEST4中间位置的圆饼 */
	 if (center->x < semiMajor || center->x >= maskImg->u32Width - semiMajor ||
		 center->y < semiMinor || center->y >= maskImg->u32Height - semiMinor ||
		 ((ConfirmNum == 1) && (center->x > (PIE_CHART_NUM_IN_TEST4 - 4) * uiProcWidth / PIE_CHART_NUM_IN_TEST4) && (center->x < (PIE_CHART_NUM_IN_TEST4 - 1) * uiProcWidth / PIE_CHART_NUM_IN_TEST4)))
	 {
		 log_info("center(%u, %u) is invalid, semiMajor %u, semiMinor %u, width %u, height %u\n",
			 center->x, center->y, semiMajor, semiMinor, maskImg->u32Width, maskImg->u32Height);
		 return;
	 }
 
	 /* 根据铅块厚薄和位置关系来进行圆饼区域叠加的灰度调整，根据统计经验值，由薄到厚，灰度值逐级变成1、0.7、0.49、0.34、0.24 */
	 if (bThintoThick)
	 {
		 if (center->x > (PIE_CHART_NUM_IN_TEST4 - 1) * uiProcWidth / PIE_CHART_NUM_IN_TEST4)
		 {
			 f32weight = 1.4;
		 }
		 else if (center->x > (PIE_CHART_NUM_IN_TEST4 - 2) * uiProcWidth / PIE_CHART_NUM_IN_TEST4)
		 {
			 f32weight = 1.3;
		 }
		 else if (center->x > (PIE_CHART_NUM_IN_TEST4 - 3) * uiProcWidth / PIE_CHART_NUM_IN_TEST4)
		 {
			 f32weight = 1.2;
		 }
		 else if (center->x > (PIE_CHART_NUM_IN_TEST4 - 4) * uiProcWidth / PIE_CHART_NUM_IN_TEST4)
		 {
			 f32weight = 1.1;
		 }
		 else
		 {
			 f32weight = 1.0;
		 }
	 }
	 else
	 {
		 if (center->x > (PIE_CHART_NUM_IN_TEST4 - 1) * uiProcWidth / PIE_CHART_NUM_IN_TEST4)
		 {
			 f32weight = 1;
		 }
		 else if (center->x > (PIE_CHART_NUM_IN_TEST4 - 2) * uiProcWidth / PIE_CHART_NUM_IN_TEST4)
		 {
			 f32weight = 1.1;
		 }
		 else if (center->x > (PIE_CHART_NUM_IN_TEST4 - 3) * uiProcWidth / PIE_CHART_NUM_IN_TEST4)
		 {
			 f32weight = 1.2;
		 }
		 else if (center->x > (PIE_CHART_NUM_IN_TEST4 - 4) * uiProcWidth / PIE_CHART_NUM_IN_TEST4)
		 {
			 f32weight = 1.3;
		 }
		 else
		 {
			 f32weight = 1.4;
		 }
	 }
 
	 if (confidence > (uint32_t)confRange[3])
	 {
		 u8PieValue = wtRange[3];
	 }
	 else if (confidence > (uint32_t)confRange[2])
	 {
		 u8PieValue = wtRange[2] - (confidence - confRange[2]) * (wtRange[2] - wtRange[3]) / (confRange[3] - confRange[2]);
	 }
	 else if (confidence > (uint32_t)confRange[1])
	 {
		 u8PieValue = wtRange[1] - (confidence - confRange[1]) * (wtRange[1] - wtRange[2]) / (confRange[2] - confRange[1]);
	 }
	 else
	 {
		 u8PieValue = wtRange[0] - (confidence - confRange[0]) * (wtRange[0] - wtRange[1]) / (confRange[1] - confRange[0]);
		 orientation = PIE_ORIENT_UNDEF; // 当置信度较低时，缺口朝向强制为未知
	 }
 
	 u16PieValue = (uint16_t)(u8PieValue * f32weight); // 调整圆饼区域灰度值
	 if (u16PieValue > 0xFF)
	 {
		 u8PieValue = 0xFF;
	 }
	 else
	 {
		 u8PieValue = (uint8_t)u16PieValue;
	 }
	 au32SemiChord.assign(au32SemiChord.size(), 0);
	 u32MajorAxis = 2 * semiMajor + 1;
	 u32MinorAxis = 2 * semiMinor + 1;
	 f64SemiMinorAxis = (double)u32MinorAxis / 2.0;
	 f64SemiMinorAxisSq = (double)u32MinorAxis * u32MinorAxis / 4.0;
 
	 // 长轴本身
	 pu8Line = pu8Plane + stride * center->y;
	 memset(pu8Line + center->x - semiMajor, u8PieValue, u32MajorAxis);
	 au32SemiChord[0] = u32MajorAxis;
 
	 for (j = 1; j <= (int32_t)semiMinor; j++)
	 {
		 k = (int32_t)(((double)rand() / RAND_MAX) * 4.0 - 2.0);                                                            // k为缺口边缘随机噪声，范围[-2, 2]
		 au32SemiChord[j] = (uint32_t)(sqrt(f64SemiMinorAxisSq - j * j) * ((double)u32MajorAxis / 2.0) / f64SemiMinorAxis); // 半弦长度
		 for (n = -1; n < 2; n += 2)                                                                                    // n: {-1, 1}，短半轴两边
		 {
			 pu8Line = pu8Plane + stride * ((int32_t)center->y + j * n);
			 if (n < 0)
			 {
				 if (orientation == PIE_ORIENT_TOPLEFT)
				 {
					 memset(pu8Line + center->x + k, u8PieValue, au32SemiChord[j] + 1 - k);
				 }
				 else if (orientation == PIE_ORIENT_TOPRIGHT)
				 {
					 memset(pu8Line + center->x - au32SemiChord[j] + k, u8PieValue, au32SemiChord[j] + 1 - k);
				 }
				 else
				 {
					 memset(pu8Line + center->x - au32SemiChord[j] + k, u8PieValue, au32SemiChord[j] * 2 + 1 - k);
				 }
			 }
			 else
			 {
				 if (orientation == PIE_ORIENT_BOTTOMLEFT)
				 {
					 memset(pu8Line + center->x + k, u8PieValue, au32SemiChord[j] + 1 - k);
				 }
				 else if (orientation == PIE_ORIENT_BOTTOMRIGHT)
				 {
					 memset(pu8Line + center->x - au32SemiChord[j] + k, u8PieValue, au32SemiChord[j] + 1 - k);
				 }
				 else
				 {
					 memset(pu8Line + center->x - au32SemiChord[j] + k, u8PieValue, au32SemiChord[j] * 2 + 1 - k);
				 }
			 }
		 }
	 }
 
	 uint32_t semiMajorNew = 0, semiMinorNew = 0;
	 uint32_t feather = ISL_MAX(semiMinor, semiMajor) / 2 - 1; // 羽化范围，较长半轴的1/2
	 if (center->x < semiMajor + feather)
	 {
		 log_info("fix the hor feather %u -> %u, semiMajor %u, center-x %u\n",
			 feather, center->x - semiMajor, semiMajor, center->x);
		 feather = center->x - semiMajor;
	 }
	 if (center->x + semiMajor + feather > maskImg->u32Width)
	 {
		 log_info("fix the hor feather %u -> %u, semiMajor %u, center-x %u, width %u\n",
			 feather, maskImg->u32Width - center->x - semiMajor, semiMajor, center->x, maskImg->u32Width);
		 feather = maskImg->u32Width - center->x - semiMajor;
	 }
	 if (center->y < semiMinor + feather)
	 {
		 log_info("fix the ver feather %u -> %u, semiMinor %u, center-y %u\n",
			 feather, center->y - semiMinor, semiMinor, center->y);
		 feather = center->y - semiMinor;
	 }
	 if (center->y + semiMinor + feather > maskImg->u32Height)
	 {
		 log_info("fix the ver feather %u -> %u, semiMinor %u, center-y %u, height %u\n",
			 feather, maskImg->u32Height - center->y - semiMinor, semiMinor, center->y, maskImg->u32Height);
		 feather = maskImg->u32Height - center->y - semiMinor;
	 }
 
	 u8PieValue = (0xFF + u8PieValue) >> 1;
	 for (i = 1; i <= (int32_t)feather; i++)
	 {
		 u8AroundValue = u8PieValue + (0xFF - u8PieValue) * i / (feather + 1);
		 semiMajorNew = semiMajor + i;
		 semiMinorNew = semiMinor + i;
		 u32MajorAxis = 2 * semiMajorNew + 1;
		 u32MinorAxis = 2 * semiMinorNew + 1;
		 f64SemiMinorAxis = (double)u32MinorAxis / 2.0;
		 f64SemiMinorAxisSq = (double)u32MinorAxis * u32MinorAxis / 4.0;
 
		 pu8Line = pu8Plane + stride * center->y;
		 pu8Line[center->x - semiMajorNew] = u8AroundValue;                             // 左顶点
		 pu8Line[center->x + semiMajorNew] = u8AroundValue;                             // 右顶点
		 *(pu8Plane + stride * (center->y - semiMinorNew) + center->x) = u8AroundValue; // 上顶点
		 *(pu8Plane + stride * (center->y + semiMinorNew) + center->x) = u8AroundValue; // 下顶点
 
		 for (j = 1; j <= (int32_t)semiMinorNew; j++)
		 {
			 // 检查 j 是否在合法范围内
			 if (j >= (int32_t)au32SemiChord.size())
			 {
				 break;
			 }
 
			 u32SemiChord = (uint32_t)(sqrt(f64SemiMinorAxisSq - j * j) * ((double)u32MajorAxis / 2.0) / f64SemiMinorAxis); // 半弦长度
			 u32DiffLen = u32SemiChord - au32SemiChord[j];
			 if (u32DiffLen > 0)
			 {
				 for (n = -1; n < 2; n += 2) // n: {-1, 1}，短半轴两边
				 {
					 pu8Line = pu8Plane + stride * ((int32_t)center->y + j * n);
					 // 添加越界检查
					 if ((int32_t)(center->y + j * n) < 0)
					 {
						 continue; // 跳过当前循环，进入下一个
					 }
					 memset(pu8Line + center->x - u32SemiChord, u8AroundValue, u32DiffLen);
					 memset(pu8Line + center->x + au32SemiChord[j] + 1, u8AroundValue, u32DiffLen);
				 }
				 au32SemiChord[j] = u32SemiChord;
			 }
		 }
	 }
 
	 // 消除多余的羽化处理
	 if (orientation < PIE_ORIENTATIONS_NUM)
	 {
		 u32MajorAxis >>= 1;
		 u32MinorAxis >>= 1;
		 if (orientation == PIE_ORIENT_TOPLEFT)
		 {
			 pu8Line = pu8Plane + stride * (center->y - u32MinorAxis) + (center->x - u32MajorAxis);
		 }
		 else if (orientation == PIE_ORIENT_TOPRIGHT)
		 {
			 pu8Line = pu8Plane + stride * (center->y - u32MinorAxis) + (center->x + 1);
		 }
		 else if (orientation == PIE_ORIENT_BOTTOMLEFT)
		 {
			 pu8Line = pu8Plane + stride * (center->y + 1) + (center->x - u32MajorAxis);
		 }
		 else if (orientation == PIE_ORIENT_BOTTOMRIGHT)
		 {
			 pu8Line = pu8Plane + stride * (center->y + 1) + (center->x + 1);
		 }
		 for (i = 0; i < (int32_t)u32MinorAxis; i++)
		 {
			 k = (int32_t)(((double)rand() / RAND_MAX) * 4.0 - 2.0); // k为缺口边缘随机噪声，范围[-2, 2]
			 memset(pu8Line + k, 0xFF, u32MajorAxis - k);
			 pu8Line += stride;
		 }
	 }
 
	 return;
 }
 
 /**
  * @fn: lslTest3AnalyEnhance
  * @berif: TEST3区域低能数据根据同区域的对比度进行重写,只重写水平过包方向线对（黑白黑白只占一个像素）
  * @param[in]: pstImageData: 待处理图像数据
  * @param[in]: nHProcStart: 待处理图像水平方向起始位置
  * @param[in]: nHProcEnd: 待处理图像水平方向结束位置
  * @param[in]: nWProcStart: 待处理图像竖直方向起始位置
  * @param[in]: nWProcEnd: 待处理图像竖直方向结束位置
  * @return {*}
  */
 int32_t TcProcModule::lslTest3AnalyEnhance(
	 XIMAGE_DATA_ST* pstImageData,
	 int32_t nHProcStart,
	 int32_t nHProcEnd,
	 int32_t nWProcStart,
	 int32_t nWProcEnd)
 {
	 /* 一维两领域求解对比度统计单个像素和周围的对比 */
	 float32_t fLowContrast = 0.0f, fHighContrast = 0.0f;
	 float32_t fHLRatio = 0.0f;
	 const int32_t MergeProcPiexlNum = 5;  // 对比度差异高低能融合处理像素长度
	 const int32_t MedianProcPiexlNum = 5; // 均值处理像素长度
	 int32_t MedianProcPiexl = 0;          // 中值处理
 
	 int32_t NeedProcPiexlNum = 0; // 需要进行竖向处理像素数量
	 uint16_t* pu16L[ISL_MAX(MergeProcPiexlNum, MedianProcPiexlNum)];
	 uint16_t* pu16LDst[ISL_MAX(MergeProcPiexlNum, MedianProcPiexlNum)];
	 uint16_t* pu16H[ISL_MAX(MergeProcPiexlNum, MedianProcPiexlNum)];
 
	 /* 将低能重写数据更新过去 */
	 m_stTestASlice.stNrawMatchSlice.u32Width = pstImageData->u32Width;
	 m_stTestASlice.stNrawMatchSlice.u32Height = pstImageData->u32Height;
	 m_stTestASlice.stNrawMatchSlice.u32Stride[0] = pstImageData->u32Stride[0];
	 memcpy((int8_t*)m_stTestASlice.stNrawMatchSlice.pPlaneVir[0], (int8_t*)pstImageData->pPlaneVir[0], pstImageData->u32Stride[0] * pstImageData->u32Height * sizeof(uint16_t));
 
	 /* 先对过包方向线对进行低能数据重写处理 */
	 for (int32_t n = nWProcStart + 1; n < nWProcEnd - 1; n++)
	 {
		 for (int32_t m = nHProcStart + (MergeProcPiexlNum - 1) / 2; m < nHProcEnd - (MergeProcPiexlNum - 1) / 2; m++)
		 {
			 /* 统计出处理长度范围的高低能数据 */
			 for (int32_t i = 0; i < MergeProcPiexlNum; i++)
			 {
				 pu16LDst[i] = (uint16_t*)pstImageData->pPlaneVir[0] + pstImageData->u32Stride[0] * (m - (MergeProcPiexlNum - 1) / 2 + i);
				 pu16L[i] = (uint16_t*)m_stTestASlice.stNrawMatchSlice.pPlaneVir[0] + m_stTestASlice.stNrawMatchSlice.u32Stride[0] * (m - (MergeProcPiexlNum - 1) / 2 + i);
				 pu16H[i] = (uint16_t*)pstImageData->pPlaneVir[1] + pstImageData->u32Stride[1] * (m - (MergeProcPiexlNum - 1) / 2 + i);
				 fLowContrast = (ISL_ABS((int32_t)pu16L[i][n] - (int32_t)pu16L[i][n - 1])) / (float32_t)pu16L[i][n];
				 fHighContrast = (ISL_ABS((int32_t)pu16H[i][n] - (int32_t)pu16H[i][n - 1])) / (float32_t)pu16H[i][n];
				 if (fHighContrast > fLowContrast)
				 {
					 NeedProcPiexlNum++;
				 }
			 }
			 /* 处理区域内像素全部进行覆盖 */
			 if (NeedProcPiexlNum >= MergeProcPiexlNum)
			 {
				 for (int32_t i = 0; i < MergeProcPiexlNum; i++)
				 {
					 fHLRatio = (float32_t)pu16H[i][n] / 65535.0f;
					 fHLRatio = fHLRatio * fHLRatio;
					 pu16LDst[i][n] = (uint16_t)(pu16L[i][n] * fHLRatio + pu16H[i][n] * (1.0f - fHLRatio));
				 }
			 }
			 NeedProcPiexlNum = 0;
		 }
	 }
 
	 memcpy((int8_t*)m_stTestASlice.stNrawMatchSlice.pPlaneVir[0], (int8_t*)pstImageData->pPlaneVir[0], pstImageData->u32Stride[0] * pstImageData->u32Height * sizeof(uint16_t));
 
	 /* 再对过包方向线对进行一维中值滤波 */
	 for (int32_t n = nWProcStart + 1; n < nWProcEnd - 1; n++)
	 {
		 for (int32_t m = nHProcStart + (MedianProcPiexlNum - 1) / 2; m < nHProcEnd - (MedianProcPiexlNum - 1) / 2; m++)
		 {
			 /* 统计出处理长度范围的低能数据 */
			 for (int32_t i = 0; i < MedianProcPiexlNum; i++)
			 {
				 pu16L[i] = (uint16_t*)m_stTestASlice.stNrawMatchSlice.pPlaneVir[0] + m_stTestASlice.stNrawMatchSlice.u32Stride[0] * (m - (MedianProcPiexlNum - 1) / 2 + i);
				 pu16LDst[i] = (uint16_t*)pstImageData->pPlaneVir[0] + pstImageData->u32Stride[0] * (m - (MedianProcPiexlNum - 1) / 2 + i);
			 }
 
			 /* 中值计算 */
			 std::vector<uint16_t> Medianvalues;
			 for (int32_t i = 0; i < MedianProcPiexlNum; i++)
			 {
				 Medianvalues.push_back(pu16L[i][n]);
			 }
			 std::sort(Medianvalues.begin(), Medianvalues.end());
			 if (Medianvalues.size() % 2 == 0)
				 MedianProcPiexl = (Medianvalues[Medianvalues.size() / 2 - 1] + Medianvalues[Medianvalues.size() / 2]) / 2;
			 else
				 MedianProcPiexl = Medianvalues[Medianvalues.size() / 2];
 
			 pu16LDst[(MedianProcPiexlNum - 1) / 2][n] = MedianProcPiexl;
		 }
	 }
	 return XRAY_LIB_OK;
 }
 
 /**
  * @fn: IslTest4Expand
  * @berif: TEST4解析增强前的TEST4区域获取和TEST4区域拷贝
  * @param[in]:  pSrcPackData 输入连续的缓存条带数据
  * @param[OUT]: pDstPackData 输出TEST4所在的坐标信息
  * @param[OUT]: pstTest4Cont 输出TEST4所在的坐标信息
  * @return void 
  */
 void TcProcModule::IslTest4Expand(XIMAGE_DATA_ST* pSrcPackData,XIMAGE_DATA_ST* pDstPackData,CONTOUR_ATTR* pstTest4Cont)
 {
	 uint16_t* pu16Src = NULL, * pu16Dst = NULL;
	 uint16_t* pu16HSrc = NULL, * pu16HDst = NULL;
	 uint8_t* pu8Dst = NULL;
	 uint32_t u32Neighbor = 1;
 
	 /* 长条带原始数据宽高赋值，复用stMaskImg源数据 */
	 m_stTestASlice.stMaskImg.u32Height = pSrcPackData->u32Height + 2 * u32Neighbor;
	 m_stTestASlice.stMaskImg.u32Width = pSrcPackData->u32Width + 2 * u32Neighbor;
	 m_stTestASlice.stMaskImg.u32Stride[0] = m_stTestASlice.stMaskImg.u32Width;
	 m_stTestASlice.stMaskImg.u32Stride[1] = m_stTestASlice.stMaskImg.u32Width;
 
	 /* 图像填充处理 */
	 Isl_ximg_fill_color(&m_stTestASlice.stMaskImg,
		 0, m_stTestASlice.stMaskImg.u32Height,
		 0, m_stTestASlice.stMaskImg.u32Width,
		 0XFF);
 
	 /* 根据包裹阈值进行二值化，找到TEST4区域大致轮廓 */
	 for (uint32_t m = 0; m < pSrcPackData->u32Height; m++)
	 {
		 pu16Src = (uint16_t*)pSrcPackData->pPlaneVir[0] + pSrcPackData->u32Stride[0] * m;
		 pu8Dst = (uint8_t*)m_stTestASlice.stMaskImg.pPlaneVir[0] + m_stTestASlice.stMaskImg.u32Stride[0] * (m + u32Neighbor);
		 for (uint32_t n = 0; n < pSrcPackData->u32Width; n++)
		 {
			 pu8Dst[n + u32Neighbor] = (pu16Src[n] > m_stTestASlice.u32Test4BinTh) ? 0xFF : 0;
		 }
	 }
 
	 /* 查找整体条带边缘,并生成轮廓 */
	 m_stTestASlice.u32ContourNum = ISL_arraySize(m_stTestASlice.astContAttr);
	 memset(&m_stTestASlice.astContAttr, 0, XIMG_CONTOURS_NUM_MAX * sizeof(CONTOUR_ATTR));
	 find_contours(NULL, &m_stTestASlice.stMaskImg, u32Neighbor, 1,
		 true, 1000, m_stTestASlice.astContAttr, &m_stTestASlice.u32ContourNum,
		 m_stTestASlice.aContourGrpRange, m_stTestASlice.paContourPoints, &m_stTestASlice.stBorderCircum, NULL);
	 if (m_stTestASlice.u32ContourNum > 0)
	 {
		 qsort(m_stTestASlice.astContAttr, m_stTestASlice.u32ContourNum, sizeof(CONTOUR_ATTR), contourAreaSort); // 对轮廓面积从大到小排序
		 memcpy(pstTest4Cont, m_stTestASlice.astContAttr, sizeof(CONTOUR_ATTR));
		 pstTest4Cont->stBoundingBox.uiX = pstTest4Cont->stBoundingBox.uiX - u32Neighbor;
		 pstTest4Cont->stBoundingBox.uiY = pstTest4Cont->stBoundingBox.uiY - u32Neighbor;
	 }
 
	 /* 图像数据复制，并填充宽高各邻域像素 */
	 pDstPackData->u32Height = pstTest4Cont->stBoundingBox.uiHeight + 2 * m_stTestASlice.u32Neighbor;
	 pDstPackData->u32Width = pstTest4Cont->stBoundingBox.uiWidth + 2 * m_stTestASlice.u32Neighbor;
	 pDstPackData->u32Stride[0] = pDstPackData->u32Width;
	 pDstPackData->u32Stride[1] = pDstPackData->u32Width;
 
	 /* 根据包裹阈值进行二值化，找到条带边界 */
	 for (uint32_t m = 0; m < pDstPackData->u32Height; m++)
	 {
		 pu16Src = (uint16_t*)pDstPackData->pPlaneVir[0] + pDstPackData->u32Stride[0] * m;
		 pu16HSrc = (uint16_t*)pDstPackData->pPlaneVir[1] + pDstPackData->u32Stride[1] * m;
		 for (uint32_t n = 0; n < pDstPackData->u32Width; n++)
		 {
			 pu16Src[n] = 0XFFFF;
			 pu16HSrc[n] = 0XFFFF;
		 }
	 }
	 for (uint32_t m = pstTest4Cont->stBoundingBox.uiY; m < pstTest4Cont->stBoundingBox.uiY + pstTest4Cont->stBoundingBox.uiHeight; m++)
	 {
		 pu16HSrc = (uint16_t*)pSrcPackData->pPlaneVir[1] + pSrcPackData->u32Stride[1] * m;
		 pu16HDst = (uint16_t*)pDstPackData->pPlaneVir[1] + pDstPackData->u32Stride[1] * (m + m_stTestASlice.u32Neighbor - pstTest4Cont->stBoundingBox.uiY);
		 pu16Src = (uint16_t*)pSrcPackData->pPlaneVir[0] + pSrcPackData->u32Stride[0] * m;
		 pu16Dst = (uint16_t*)pDstPackData->pPlaneVir[0] + pDstPackData->u32Stride[0] * (m + m_stTestASlice.u32Neighbor - pstTest4Cont->stBoundingBox.uiY);
		 for (uint32_t n = pstTest4Cont->stBoundingBox.uiX; n < pstTest4Cont->stBoundingBox.uiX + pstTest4Cont->stBoundingBox.uiWidth; n++)
		 {
			 pu16HDst[n + m_stTestASlice.u32Neighbor - pstTest4Cont->stBoundingBox.uiX] = pu16HSrc[n];
			 pu16Dst[n + m_stTestASlice.u32Neighbor - pstTest4Cont->stBoundingBox.uiX] = pu16Src[n];
		 }
	 }
 }
 
 // 一定区域的标准差计算
 static double CalculateStandardDeviation(XIMAGE_DATA_ST* pstFuseSrcData, RECT stStrRect)
 {
	 uint16_t* pu16Src = NULL;
	 std::vector<uint16_t> pixelValues;
	 int32_t startX = static_cast<int32_t>(stStrRect.uiX);
	 int32_t startY = static_cast<int32_t>(stStrRect.uiY);
	 int32_t width = static_cast<int32_t>(stStrRect.uiWidth);
	 int32_t height = static_cast<int32_t>(stStrRect.uiHeight);
 
	 // 提取stStrRect区域的像素值
	 for (int32_t y = startY; y < startY + height; ++y)
	 {
		 for (int32_t x = startX; x < startX + width; ++x)
		 {
			 // 假设pstFuseSrcData是一个包含图像数据的结构，您需要根据实际数据结构来访问像素
			 pu16Src = (uint16_t*)pstFuseSrcData->pPlaneVir[0] + y * pstFuseSrcData->u32Stride[0]; // 假设为一维数组
			 uint16_t pixelValue = pu16Src[x];                                                      // 假设为一维数组
			 pixelValues.push_back(pixelValue);
		 }
	 }
 
	 // 计算均值
	 double mean = 0;
	 for (uint16_t value : pixelValues)
	 {
		 mean += value;
	 }
	 mean /= pixelValues.size();
 
	 // 计算标准差
	 double variance = 0;
	 for (uint16_t value : pixelValues)
	 {
		 variance += (value - mean) * (value - mean);
	 }
	 variance /= pixelValues.size();
	 double stddev = std::sqrt(variance);
 
	 return stddev;
 }
 
 // 计算连续大于阈值的的数量
 static int32_t countConsecutiveOnes(const std::vector<int32_t>& data, int32_t start, int32_t end, int32_t threshold)
 {
	 int32_t count = 0;
	 if (start <= end)
	 {
		 while (start <= end)
		 {
			 if (data[start] >= threshold)
			 {
				 count++;
				 start++;
			 }
			 else
			 {
				 break;
			 }
		 }
	 }
	 else
	 {
		 while (end <= start)
		 {
			 if (data[start] >= threshold)
			 {
				 count++;
				 start--;
			 }
			 else
			 {
				 break;
			 }
		 }
	 }
	 return count;
 }
 
 // 计算Test4区域的窗口标准差变化趋势，用于判断TEST4为垂直镜像,这种检测方案只适用于“TEST”和“4”之间的存在空隙
 static int32_t IsTest44onTop(XIMAGE_DATA_ST* pstFuseSrcData, RECT stStrRect)
 {
	 std::vector<int32_t> stddevArray; // 存储标准差的数组
	 RECT currentRect = stStrRect; // 窗口矩形框
	 currentRect.uiHeight = stStrRect.uiWidth / 2;
 
	 // 获取矩形框的起始点和结束点
	 int32_t startY = stStrRect.uiY;
	 int32_t endY = stStrRect.uiY + stStrRect.uiHeight; // 结束点根据宽度计算
 
	 // 遍历矩形框，每间隔stStrRect.uiWidth的长度
	 for (int32_t y = startY; y < endY; y += (stStrRect.uiWidth / 2))
	 {
		 currentRect.uiY = y; // 更新当前矩形框的起始y坐标
		 // 调用标准差计算函数
		 double stddev = CalculateStandardDeviation(pstFuseSrcData, currentRect);
		 stddevArray.push_back((int32_t)stddev); // 存储当前标准差
	 }
 
	 // 分析stddevArray的变化趋势
	 int32_t minStddev = *std::min_element(stddevArray.begin(), stddevArray.end());
	 int32_t threshold = 3 * minStddev; // 阈值设置为2倍的最小标准差
 
	 int32_t left_count = 0;
	 for (int32_t i = 0; i < (int32_t)stddevArray.size(); ++i)
	 {
		 if (stddevArray[i] >= threshold)
		 {
			 left_count = countConsecutiveOnes(stddevArray, i, (int32_t)stddevArray.size(), threshold);
			 break;
		 }
	 }
 
	 int32_t right_count = 0;
	 for (int32_t i = (int32_t)stddevArray.size() - 1; i >= 0; --i)
	 {
		 if (stddevArray[i] >= threshold)
		 {
			 right_count = countConsecutiveOnes(stddevArray, i, 0, threshold);
			 break;
		 }
	 }
 
	 // 比较两个方向上连续1的数量
	 if (left_count > right_count)
	 {
		 return false;
	 }
	 else
	 {
		 return true;
	 }
 }
 
//  /* 图像融合起始行、列偏差计算 返回值为融合行和原始数据偏差，大于0，则说明融合数据需要进行上或左偏移offset行，小于0，则说明需要进行下或右偏移offset行 */
//  static std::pair<int32_t, int32_t> getFuseStartOffset(XIMAGE_DATA_ST* pstFuseSrcData, RECT* pFuseRect, XIMAGE_DATA_ST* pstBlendSrcData, RECT* pBlendRect, bool bVerticalFuse)
//  {
// 	 int32_t offsetY = 0, offsetX = 0;
// 	 int32_t startY = pFuseRect->uiY;
// 	 int32_t startX = pFuseRect->uiX;
// 	 uint16_t* pu16LineNewHe = NULL, * pu16LineBlendHe = NULL;
// 	 int32_t width = ISL_MIN(pFuseRect->uiWidth, pBlendRect->uiWidth);
// 	 int32_t height = ISL_MIN(pFuseRect->uiHeight, pBlendRect->uiHeight);
 
// 	 int32_t DiffMatchNum = 16;
// 	 std::vector<uint32_t> uDiffSum(DiffMatchNum, 0); // 使用std::vector初始化
 
// 	 // Y方向，寻找差异最小行
// 	 for (int32_t DiffIndex = -(DiffMatchNum / 2); DiffIndex < DiffMatchNum / 2; DiffIndex++)
// 	 {
// 		 for (int32_t y = 0; y < height; ++y)
// 		 {
// 			 int32_t blendIdx = bVerticalFuse ? (pFuseRect->uiHeight - y) : y;
 
// 			 pu16LineNewHe = (uint16_t*)pstFuseSrcData->pPlaneVir[1] + (startY + blendIdx + DiffIndex) * pstFuseSrcData->u32Stride[1];
// 			 pu16LineBlendHe = (uint16_t*)pstBlendSrcData->pPlaneVir[1] + (y + pBlendRect->uiY) * pstBlendSrcData->u32Stride[1]; // 示例逻辑
// 			 for (int32_t x = 0; x < width; ++x)
// 			 {
// 				 int32_t diff = abs(pu16LineNewHe[x + startX] - pu16LineBlendHe[x + pBlendRect->uiX]);
// 				 uDiffSum[DiffIndex + DiffMatchNum / 2] += diff;
// 			 }
// 		 }
// 	 }
 
// 	 uint32_t uminValue = uDiffSum[0];
// 	 for (size_t i = 0; i < uDiffSum.size(); ++i)
// 	 {
// 		 if (uDiffSum[i] < uminValue)
// 		 {
// 			 uminValue = uDiffSum[i];
// 			 offsetY = (int32_t)i - (DiffMatchNum / 2);
// 		 }
// 	 }
// 	 std::fill(uDiffSum.begin(), uDiffSum.end(), 0); // 将所有元素设置为 0
// 	 // X方向，寻找差异最小列
// 	 for (int32_t DiffIndex = -(DiffMatchNum / 2); DiffIndex < DiffMatchNum / 2; DiffIndex++)
// 	 {
// 		 for (int32_t y = 0; y < height; ++y)
// 		 {
// 			 int32_t blendIdx = bVerticalFuse ? (pFuseRect->uiHeight - y) : y;
 
// 			 pu16LineNewHe = (uint16_t*)pstFuseSrcData->pPlaneVir[1] + (startY + blendIdx + offsetY) * pstFuseSrcData->u32Stride[1];
// 			 pu16LineBlendHe = (uint16_t*)pstBlendSrcData->pPlaneVir[1] + (y + pBlendRect->uiY) * pstBlendSrcData->u32Stride[1]; // 示例逻辑
// 			 for (int32_t x = 0; x < width; ++x)
// 			 {
// 				 int32_t diff = abs(pu16LineNewHe[x + startX + DiffIndex] - pu16LineBlendHe[x + pBlendRect->uiX]);
// 				 uDiffSum[DiffIndex + DiffMatchNum / 2] += diff;
// 			 }
// 		 }
// 	 }
// 	 uminValue = uDiffSum[0];
// 	 for (size_t i = 0; i < uDiffSum.size(); ++i)
// 	 {
// 		 if (uDiffSum[i] < uminValue)
// 		 {
// 			 uminValue = uDiffSum[i];
// 			 offsetX = (int32_t)i - (DiffMatchNum / 2);
// 		 }
// 	 }
// 	 return std::make_pair(offsetX, offsetY); // 返回Y和X方向的偏移
//  }
 
 /**
  * @fn: TwoImageFusion
  * @berif: 两帧图像融合函数
  * @param: [in] pstBlendDstData: 融合后图像数据结构
  * @param: [in] pstFuseSrcData: 融合源图像数据结构
  * @param: [in] pstBoundingBox: 融合区域矩形框
  * @param: [in] pstNrawTEST4Blend: 待融合TEST4图像数据结构
  * @param: [in] pTest4BlendRect: 待融合TEST4图像矩形框
  * @param: [in] bTEST4VertMirror: 融合是否需要进行垂直镜像
  * @return void
  */
 static void TwoImageFusion(XIMAGE_DATA_ST* pstBlendDstData,
	 XIMAGE_DATA_ST* pstFuseSrcData,
	 RECT* pstBoundingBox,
	 XIMAGE_DATA_ST* pstNrawTEST4Blend,
	 RECT* pTest4BlendRect,
	 bool bTEST4VertMirror)
 {
	 // std::pair<int32_t, int32_t> offset = getFuseStartOffset(pstFuseSrcData, pstBoundingBox, pstNrawTEST4Blend, pTest4BlendRect, bTEST4VertMirror);
	 std::pair<int32_t, int32_t> offset;
	 offset.first = 0;  // 临时代码，用于测试
	 offset.second = 0; // 临时代码，用于测试
 
	 int32_t blendHeight = ISL_MIN(pTest4BlendRect->uiHeight, pstBoundingBox->uiHeight);
	 int32_t blendWidth = ISL_MIN(pTest4BlendRect->uiWidth, pstBoundingBox->uiWidth);
	 int32_t iNeighborNum = pstBoundingBox->uiHeight / 10;
	 int32_t ProcHeight = blendHeight - 2 * iNeighborNum;
	 int32_t SmoothRatio = 4; // 平滑处理分段
 
	 // 控制融合范围，中间60%的区域强制融合比例1:1
	 for (int32_t i = 0; i < ProcHeight; i++)
	 {
		 float32_t blendFactor = (float32_t)(i + 1) / (float32_t)ProcHeight;
		 float32_t blendWeight = SmoothRatio * SmoothRatio * blendFactor * blendFactor / 2;
 
		 // 融合权重按照分段函数 二次函数实现融合
		 if (i > ProcHeight / SmoothRatio)
		 {
			 blendWeight = 0.5f;
		 }
		 if (i > ProcHeight * (SmoothRatio - 1) / SmoothRatio)
		 {
			 blendWeight = SmoothRatio * SmoothRatio * (blendFactor - 1) * (blendFactor - 1) / 2;
		 }
 
		 // 获得当前行的指针
		 uint16_t* pu16LineNewLe = (uint16_t*)pstFuseSrcData->pPlaneVir[0] + (i + pstBoundingBox->uiY + iNeighborNum + offset.second) * pstFuseSrcData->u32Stride[0];
		 uint16_t* pu16LineNewHe = (uint16_t*)pstFuseSrcData->pPlaneVir[1] + (i + pstBoundingBox->uiY + iNeighborNum + offset.second) * pstFuseSrcData->u32Stride[1];
 
		 // 获得当前行的指针
		 uint16_t* pu16LineDstLe = (uint16_t*)pstBlendDstData->pPlaneVir[0] + (i + pstBoundingBox->uiY + iNeighborNum + offset.second) * pstBlendDstData->u32Stride[0];
		 uint16_t* pu16LineDstHe = (uint16_t*)pstBlendDstData->pPlaneVir[1] + (i + pstBoundingBox->uiY + iNeighborNum + offset.second) * pstBlendDstData->u32Stride[1];
 
		 // 处理垂直镜像
		 int32_t blendIdx = bTEST4VertMirror ? (pTest4BlendRect->uiHeight - 2 * iNeighborNum - i) : i;
		 uint16_t* pu16LineBlendLe = (uint16_t*)pstNrawTEST4Blend->pPlaneVir[0] + (blendIdx + pTest4BlendRect->uiY + iNeighborNum) * pstFuseSrcData->u32Stride[0];
		 uint16_t* pu16LineBlendHe = (uint16_t*)pstNrawTEST4Blend->pPlaneVir[1] + (blendIdx + pTest4BlendRect->uiY + iNeighborNum) * pstFuseSrcData->u32Stride[1];
 
		 // 遍历宽度
		 for (int32_t j = 0; j < (blendWidth - 2 * iNeighborNum); j++)
		 {
			 uint16_t blendValueLe = pu16LineBlendLe[j + pTest4BlendRect->uiX + iNeighborNum];
			 uint16_t blendValueHe = pu16LineBlendHe[j + pTest4BlendRect->uiX + iNeighborNum];
			 if ((ISL_SUB_ABS(pu16LineNewLe[j + pstBoundingBox->uiX + iNeighborNum], blendValueLe) < 2000) || (blendWeight == 0.5f))
			 {
				 pu16LineDstLe[j + pstBoundingBox->uiX + iNeighborNum + offset.first] = (uint16_t)(blendValueLe * blendWeight) + (uint16_t)(pu16LineNewLe[j + pstBoundingBox->uiX + iNeighborNum + offset.first] * (1 - blendWeight));
			 }
			 if ((ISL_SUB_ABS(pu16LineNewHe[j + pstBoundingBox->uiX + iNeighborNum], blendValueHe) < 2000) || (blendWeight == 0.5f))
			 {
				 pu16LineDstHe[j + pstBoundingBox->uiX + iNeighborNum + offset.first] = (uint16_t)(blendValueHe * blendWeight) + (uint16_t)(pu16LineNewHe[j + pstBoundingBox->uiX + iNeighborNum + offset.first] * (1 - blendWeight));
			 }
		 }
	 }
 }
 /**
  * @fn: CopySrcDataToCache
  * @berif: 拷贝源数据到缓存队列中
  * @param: [in] pstFuseSrcData: 融合源图像数据结构
  * @param: [in] pTest4AorB: 融合缓存队列
  * @param: [in] isTest44OnTop: 融合是否需要进行垂直镜像
  * @param: [in] pFuseRect: 融合区域矩形框
  * @return {*}
  */
 static void CopySrcDataToCache(XIMAGE_DATA_ST* pstFuseSrcData,
	 XIMAGE_DATA_ST* pTest4AorBCache,
	 bool isTest44OnTop,
	 RECT* pFuseRect)
 {
	 uint16_t* pu16LSrc = NULL;
	 uint16_t* pu16HSrc = NULL;
	 uint16_t* pu16LDst = NULL;
	 uint16_t* pu16HDst = NULL;
	 uint32_t ustartY = pFuseRect->uiY;
	 uint32_t ustartX = pFuseRect->uiX;
	 uint32_t uwidth = pFuseRect->uiWidth;
	 uint32_t uheight = pFuseRect->uiHeight;
 
	 pTest4AorBCache->u32Width = pstFuseSrcData->u32Width;
	 pTest4AorBCache->u32Height = pstFuseSrcData->u32Height;
	 pTest4AorBCache->u32Stride[0] = pstFuseSrcData->u32Stride[0];
	 pTest4AorBCache->u32Stride[1] = pstFuseSrcData->u32Stride[1];
	 memcpy(pTest4AorBCache->pPlaneVir[0], pstFuseSrcData->pPlaneVir[0], pstFuseSrcData->u32Stride[0] * pstFuseSrcData->u32Height * sizeof(uint16_t));
	 memcpy(pTest4AorBCache->pPlaneVir[1], pstFuseSrcData->pPlaneVir[1], pstFuseSrcData->u32Stride[1] * pstFuseSrcData->u32Height * sizeof(uint16_t));
 
	 // 强制要求TEST4中4向上
	 if (false == isTest44OnTop)
	 {
		 for (uint32_t i = ustartY, k = 0; i < ustartY + uheight; i++, k++)
		 {
			 pu16LSrc = (uint16_t*)pstFuseSrcData->pPlaneVir[0] + i * pstFuseSrcData->u32Stride[0];
			 pu16HSrc = (uint16_t*)pstFuseSrcData->pPlaneVir[1] + i * pstFuseSrcData->u32Stride[1];
			 pu16LDst = (uint16_t*)pTest4AorBCache->pPlaneVir[0] + (ustartY + uheight - k) * pTest4AorBCache->u32Stride[0];
			 pu16HDst = (uint16_t*)pTest4AorBCache->pPlaneVir[1] + (ustartY + uheight - k) * pTest4AorBCache->u32Stride[1];
			 for (uint32_t j = ustartX; j < ustartX + uwidth; j++)
			 {
				 pu16LDst[j] = pu16LSrc[j];
				 pu16HDst[j] = pu16HSrc[j];
			 }
		 }
	 }
 }
 
 /* TEST4区域20240929 sunguojian5 ADD:
	 1、测试体融合正反向过包融合，根据TEST4标准差来判断过包方向;
	 2、对角度保持在3度以内的图像进行融合;
	 3、多存两组图像
	 pstTest4Cont: TEST4区域轮廓信息
  */
 int32_t TcProcModule::lslTest4FuseEnhance(
	 XIMAGE_DATA_ST* pstFuseSrcData,
	 CONTOUR_ATTR* pstTest4Cont,
	 TEST_SLICE_PROCESS_STATE enTestCase)
 {
	 // 定义内部变量
	 RECT stStrRect;
	 double stddevleft, stddevright;
 
	 // 根据是否需要垂直镜像融合，确定融合数据
	 uint16_t* pu16LineNewLe = NULL, * pu16LineNewHe = NULL;
	 XIMAGE_DATA_ST* pstNrawTEST4Blend = NULL;
	 RECT* pTest4BlendRect = NULL;
 
	 // 用于定义融合缓存的数组
	 XIMAGE_DATA_ST* pstTEST4BlendArr[XIMG_FUSE_ENHANCE_NUM_MAX] = { nullptr };
	 RECT* pTest4BlendRectArr[XIMG_FUSE_ENHANCE_NUM_MAX] = { nullptr };
	 double* pAngle;
 
	 // 确定"TEST4"字符的方位，Left or Right
	 stStrRect.uiWidth = (uint32_t)(0.06 * pstTest4Cont->stBoundingBox.uiWidth);
	 stStrRect.uiX = pstTest4Cont->stBoundingBox.uiX - m_stTestASlice.u32Neighbor - stStrRect.uiWidth;
	 stStrRect.uiY = pstTest4Cont->stBoundingBox.uiY;
	 stStrRect.uiHeight = pstTest4Cont->stBoundingBox.uiHeight;
 
	 stddevleft = CalculateStandardDeviation(pstFuseSrcData, stStrRect);
	 stStrRect.uiX = pstTest4Cont->stBoundingBox.uiX + pstTest4Cont->stBoundingBox.uiWidth + m_stTestASlice.u32Neighbor;
	 stddevright = CalculateStandardDeviation(pstFuseSrcData, stStrRect);
 
	 // 根据当前状态和前一次状态判断是否要把融合次数置0
	 if (enTestCase == m_stTestASlice.enPreTest4State)
	 {
		 m_stTestASlice.stTest4Fused.u32BlendCnt = 0;
	 }
	 m_stTestASlice.enPreTest4State = enTestCase;
 
	 // 第一组TEST4才进行过包方向和方位判断,并且融合次数为0
	 if ((TESTA_SLICE_END_MATCH_FIRST == enTestCase) && (m_stTestASlice.stTest4Fused.u32BlendCnt == 0))
	 {
		 m_stTestASlice.stTest4Deque.clear();
		 // 过包角度记录
		 m_stTestASlice.stTest4Fused.angle = m_stTestASlice.stRectTest4.angle;
		 if (stddevleft > stddevright)
		 {
			 m_stTestASlice.stTest4Fused.bTEST4onLeft = true;
			 stStrRect.uiX = pstTest4Cont->stBoundingBox.uiX - m_stTestASlice.u32Neighbor - stStrRect.uiWidth;
		 }
		 else
		 {
			 m_stTestASlice.stTest4Fused.bTEST4onLeft = false;
		 }
 
		 m_stTestASlice.stTEST4BlendTemp.u32Width = pstFuseSrcData->u32Width;
		 m_stTestASlice.stTEST4BlendTemp.u32Height = pstFuseSrcData->u32Height;
		 m_stTestASlice.stTEST4BlendTemp.u32Stride[0] = pstFuseSrcData->u32Stride[0];
		 m_stTestASlice.stTEST4BlendTemp.u32Stride[1] = pstFuseSrcData->u32Stride[1];
 
		 m_stTestASlice.stTest4Fused.bTEST44onTop = IsTest44onTop(pstFuseSrcData, stStrRect);
		 pstNrawTEST4Blend = &(m_stTestASlice.stTest4Fused.bTEST44onTop ? m_stTestASlice.stTEST4CacheVector[0].stNrawTEST4ABlend : m_stTestASlice.stTEST4CacheVector[0].stNrawTEST4BBlend);
		 pTest4BlendRect = &(m_stTestASlice.stTest4Fused.bTEST44onTop ? m_stTestASlice.stTEST4CacheVector[0].test4BlendRectA : m_stTestASlice.stTEST4CacheVector[0].test4BlendRectB);
		 *pTest4BlendRect = pstTest4Cont->stBoundingBox;
		 pAngle = &(m_stTestASlice.stTest4Fused.bTEST44onTop ? m_stTestASlice.stTEST4CacheVector[0].test4AngleA : m_stTestASlice.stTEST4CacheVector[0].test4AngleB);
		 *pAngle = m_stTestASlice.stRectTest4.angle;
		 CopySrcDataToCache(pstFuseSrcData, pstNrawTEST4Blend, m_stTestASlice.stTest4Fused.bTEST44onTop, &pstTest4Cont->stBoundingBox);
	 }
	 else if ((TESTA_SLICE_END_MATCH_SECOND == enTestCase) && (m_stTestASlice.stTest4Fused.u32BlendCnt == 0))
	 {
		 if (stddevleft > stddevright)
		 {
			 stStrRect.uiX = pstTest4Cont->stBoundingBox.uiX - m_stTestASlice.u32Neighbor - stStrRect.uiWidth;
		 }
		 if (m_stTestASlice.stTest4Fused.bTEST44onTop != (bool)IsTest44onTop(pstFuseSrcData, stStrRect))
		 {
			 m_stTestASlice.stTest4Fused.u32BlendCnt = 0;
			 return ISL_FAIL;
		 }
 
		 // 查找前一个缓存记录的TEST4位置信息
		 pTest4BlendRect = &(m_stTestASlice.stTest4Fused.bTEST44onTop ? m_stTestASlice.stTEST4CacheVector[0].test4BlendRectA : m_stTestASlice.stTEST4CacheVector[0].test4BlendRectB);
		 // 与前一个TEST4A的位置信息作比较
		 int32_t heightDiff = ISL_ABS((int32_t)pTest4BlendRect->uiHeight - (int32_t)pstTest4Cont->stBoundingBox.uiHeight);
		 int32_t widthDiff = ISL_ABS((int32_t)pTest4BlendRect->uiWidth - (int32_t)pstTest4Cont->stBoundingBox.uiWidth);
 
		 // 判断前后两次TEST4的宽度和高度以及角度误差是否过大
		 if ((50 * heightDiff > (int32_t)pTest4BlendRect->uiHeight) ||
			 (50 * widthDiff > (int32_t)pTest4BlendRect->uiWidth) ||
			 (ISL_ABS(ISL_ABS(m_stTestASlice.stRectTest4.angle) - ISL_ABS(m_stTestASlice.stTest4Fused.angle) > 3.0)))
		 {
			 m_stTestASlice.stTest4Fused.u32BlendCnt = 0;
			 return ISL_FAIL;
		 }
 
		 // 复制TEST4A图像数据和图像信息到融合缓存
		 pstNrawTEST4Blend = &(m_stTestASlice.stTest4Fused.bTEST44onTop ? m_stTestASlice.stTEST4CacheVector[0].stNrawTEST4BBlend : m_stTestASlice.stTEST4CacheVector[0].stNrawTEST4ABlend);
		 pTest4BlendRect = &(m_stTestASlice.stTest4Fused.bTEST44onTop ? m_stTestASlice.stTEST4CacheVector[0].test4BlendRectB : m_stTestASlice.stTEST4CacheVector[0].test4BlendRectA);
		 *pTest4BlendRect = pstTest4Cont->stBoundingBox;
		 pAngle = &(m_stTestASlice.stTest4Fused.bTEST44onTop ? m_stTestASlice.stTEST4CacheVector[0].test4AngleB : m_stTestASlice.stTEST4CacheVector[0].test4AngleA);
		 *pAngle = m_stTestASlice.stRectTest4.angle;
		 CopySrcDataToCache(pstFuseSrcData, pstNrawTEST4Blend, m_stTestASlice.stTest4Fused.bTEST44onTop, &pstTest4Cont->stBoundingBox);
 
		 m_stTestASlice.stTest4Fused.u32BlendCnt++; // 融合次数加1,一次完整的TEST4识别结束
 
		 // 压入队列中
		 m_stTestASlice.stTest4Deque.push_front(m_stTestASlice.stTEST4CacheVector[0]);
		 return ISL_OK;
	 }
 
	 // 测试体TEST4融合流程，融合次数大于0
	 if (m_stTestASlice.stTest4Fused.u32BlendCnt > 0)
	 {
		 ISL_TEST4_FUSE_DEQUE* pstTEST4Vector;
 
		 // 根据融合次数选择缓存数据地址
		 uint32_t u32BlendIdx = m_stTestASlice.stTest4Fused.u32BlendCnt % m_stTestASlice.uBlendImgNum;
		 pstTEST4Vector = &m_stTestASlice.stTEST4CacheVector[u32BlendIdx];
 
		 // 只对TEST4A部分做几何验证和数据记录
		 if (TESTA_SLICE_END_MATCH_FIRST == enTestCase)
		 {
			 // 水平镜像则直接重新进行融合初始数据的采集，否则重新进行初始数据采集
			 if (stddevleft > stddevright)
			 {
				 if (false == m_stTestASlice.stTest4Fused.bTEST4onLeft)
				 {
					 m_stTestASlice.stTest4Fused.u32BlendCnt = 0;
					 return ISL_FAIL;
				 }
				 stStrRect.uiX = pstTest4Cont->stBoundingBox.uiX - m_stTestASlice.u32Neighbor - stStrRect.uiWidth;
			 }
			 else
			 {
				 if (true == m_stTestASlice.stTest4Fused.bTEST4onLeft)
				 {
					 m_stTestASlice.stTest4Fused.u32BlendCnt = 0;
					 return ISL_FAIL;
				 }
			 }
 
			 // TEST4中4是否垂直镜像,由于前面的所有缓存数据强制为TEST中4onTop，所以此处只要判断TEST4中4是否onTop即可
			 int32_t bTest44onTop = IsTest44onTop(pstFuseSrcData, stStrRect);
			 m_stTestASlice.stTest4Fused.bTEST44onTop = (bTest44onTop) ? true : false;
			 m_stTestASlice.stTest4Fused.bTEST4VertMirror = !bTest44onTop;
			 // 选择队列中最新的上一级缓存数据的位置
			 pTest4BlendRect = &(m_stTestASlice.stTest4Fused.bTEST4VertMirror ? m_stTestASlice.stTest4Deque[0].test4BlendRectA : m_stTestASlice.stTest4Deque[0].test4BlendRectB);
			 pAngle = &(m_stTestASlice.stTest4Fused.bTEST4VertMirror ? m_stTestASlice.stTest4Deque[0].test4AngleA : m_stTestASlice.stTest4Deque[0].test4AngleB);
 
			 int32_t heightDiff = ISL_ABS((int32_t)pTest4BlendRect->uiHeight - (int32_t)pstTest4Cont->stBoundingBox.uiHeight);
			 int32_t widthDiff = ISL_ABS((int32_t)pTest4BlendRect->uiWidth - (int32_t)pstTest4Cont->stBoundingBox.uiWidth);
 
			 // 判断前后两次TEST4的宽度和高度以及角度误差是否过大
			 if ((50 * heightDiff > (int32_t)pTest4BlendRect->uiHeight) ||
				 (50 * widthDiff > (int32_t)pTest4BlendRect->uiWidth) ||
				 (ISL_ABS(ISL_ABS(m_stTestASlice.stRectTest4.angle) - ISL_ABS(*pAngle) > 3.0)))
			 {
				 m_stTestASlice.stTest4Fused.u32BlendCnt = 0;
				 return ISL_FAIL;
			 }
			 *pAngle = m_stTestASlice.stRectTest4.angle;
			 // 新来的数据处理流程：数据缓存->融合处理
			 pstNrawTEST4Blend = &(m_stTestASlice.stTest4Fused.bTEST44onTop ? pstTEST4Vector->stNrawTEST4ABlend : pstTEST4Vector->stNrawTEST4BBlend);
			 pTest4BlendRect = &(m_stTestASlice.stTest4Fused.bTEST44onTop ? pstTEST4Vector->test4BlendRectA : pstTEST4Vector->test4BlendRectB);
			 *pTest4BlendRect = pstTest4Cont->stBoundingBox;
			 CopySrcDataToCache(pstFuseSrcData, pstNrawTEST4Blend, m_stTestASlice.stTest4Fused.bTEST44onTop, &pstTest4Cont->stBoundingBox);
 
			 for (uint32_t i = 0; i < m_stTestASlice.uBlendImgNum; i++)
			 {
				 pstTEST4BlendArr[i] = &(m_stTestASlice.stTest4Fused.bTEST4VertMirror ? m_stTestASlice.stTest4Deque[0].stNrawTEST4BBlend : m_stTestASlice.stTest4Deque[0].stNrawTEST4ABlend);
				 pTest4BlendRectArr[i] = &(m_stTestASlice.stTest4Fused.bTEST4VertMirror ? m_stTestASlice.stTest4Deque[0].test4BlendRectB : m_stTestASlice.stTest4Deque[0].test4BlendRectA);
			 }
 
			 // 直接和存在的一组数据进行融合
			 if (m_stTestASlice.stTest4Deque.size() == 1)
			 {
				 TwoImageFusion(pstFuseSrcData, pstFuseSrcData, &pstTest4Cont->stBoundingBox, pstTEST4BlendArr[0], pTest4BlendRectArr[0], m_stTestASlice.stTest4Fused.bTEST4VertMirror);
			 }
			 // 前两组TEST4融合完成之后，再和当前组进行融合
			 else if (m_stTestASlice.stTest4Deque.size() == 2)
			 {
				 TwoImageFusion(&m_stTestASlice.stTEST4BlendTemp, pstTEST4BlendArr[0], pTest4BlendRectArr[0], pstTEST4BlendArr[1], pTest4BlendRectArr[1], false);
				 TwoImageFusion(pstFuseSrcData, pstFuseSrcData, &pstTest4Cont->stBoundingBox, &m_stTestASlice.stTEST4BlendTemp, pTest4BlendRectArr[0], m_stTestASlice.stTest4Fused.bTEST4VertMirror);
			 }
			 // 前三组TEST4融合完成之后，再和当前组进行融合
			 else
			 {
				 TwoImageFusion(&m_stTestASlice.stTEST4BlendTemp, pstTEST4BlendArr[1], pTest4BlendRectArr[1], pstTEST4BlendArr[2], pTest4BlendRectArr[2], false);
				 TwoImageFusion(&m_stTestASlice.stTEST4BlendTemp, pstTEST4BlendArr[0], pTest4BlendRectArr[0], &m_stTestASlice.stTEST4BlendTemp, pTest4BlendRectArr[1], false);
				 TwoImageFusion(pstFuseSrcData, pstFuseSrcData, &pstTest4Cont->stBoundingBox, &m_stTestASlice.stTEST4BlendTemp, pTest4BlendRectArr[0], m_stTestASlice.stTest4Fused.bTEST4VertMirror);
			 }
		 }
		 // second
		 else
		 {
			 // 新来的数据处理流程：数据缓存->融合处理
			 pstNrawTEST4Blend = &(m_stTestASlice.stTest4Fused.bTEST44onTop ? pstTEST4Vector->stNrawTEST4BBlend : pstTEST4Vector->stNrawTEST4ABlend);
			 pTest4BlendRect = &(m_stTestASlice.stTest4Fused.bTEST44onTop ? pstTEST4Vector->test4BlendRectB : pstTEST4Vector->test4BlendRectA);
			 *pTest4BlendRect = pstTest4Cont->stBoundingBox;
			 CopySrcDataToCache(pstFuseSrcData, pstNrawTEST4Blend, m_stTestASlice.stTest4Fused.bTEST44onTop, &pstTest4Cont->stBoundingBox);
 
			 for (uint32_t i = 0; i < m_stTestASlice.uBlendImgNum; i++)
			 {
				 pstTEST4BlendArr[i] = &(m_stTestASlice.stTest4Fused.bTEST4VertMirror ? m_stTestASlice.stTest4Deque[0].stNrawTEST4ABlend : m_stTestASlice.stTest4Deque[0].stNrawTEST4BBlend);
				 pTest4BlendRectArr[i] = &(m_stTestASlice.stTest4Fused.bTEST4VertMirror ? m_stTestASlice.stTest4Deque[0].test4BlendRectA : m_stTestASlice.stTest4Deque[0].test4BlendRectB);
			 }
 
			 // 直接和存在的一组数据进行融合
			 if (m_stTestASlice.stTest4Deque.size() == 1)
			 {
				 TwoImageFusion(pstFuseSrcData, pstFuseSrcData, &pstTest4Cont->stBoundingBox, pstTEST4BlendArr[0], pTest4BlendRectArr[0], m_stTestASlice.stTest4Fused.bTEST4VertMirror);
			 }
			 // 前两组TEST4融合完成之后，再和当前组进行融合
			 else if (m_stTestASlice.stTest4Deque.size() == 2)
			 {
				 TwoImageFusion(&m_stTestASlice.stTEST4BlendTemp, pstTEST4BlendArr[0], pTest4BlendRectArr[0], pstTEST4BlendArr[1], pTest4BlendRectArr[1], false);
				 TwoImageFusion(pstFuseSrcData, pstFuseSrcData, &pstTest4Cont->stBoundingBox, &m_stTestASlice.stTEST4BlendTemp, pTest4BlendRectArr[0], m_stTestASlice.stTest4Fused.bTEST4VertMirror);
			 }
			 // 前三组TEST4融合完成之后，再和当前组进行融合
			 else
			 {
				 TwoImageFusion(&m_stTestASlice.stTEST4BlendTemp, pstTEST4BlendArr[1], pTest4BlendRectArr[1], pstTEST4BlendArr[2], pTest4BlendRectArr[2], false);
				 TwoImageFusion(&m_stTestASlice.stTEST4BlendTemp, pstTEST4BlendArr[0], pTest4BlendRectArr[0], &m_stTestASlice.stTEST4BlendTemp, pTest4BlendRectArr[1], false);
				 TwoImageFusion(pstFuseSrcData, pstFuseSrcData, &pstTest4Cont->stBoundingBox, &m_stTestASlice.stTEST4BlendTemp, pTest4BlendRectArr[0], m_stTestASlice.stTest4Fused.bTEST4VertMirror);
			 }
 
			 // 限制队列的数量为 m_stTestSlice.uBlendImgNum 个
			 while (m_stTestASlice.stTest4Deque.size() >= m_stTestASlice.uBlendImgNum)
			 {
				 m_stTestASlice.stTest4Deque.pop_back();
			 }
 
			 // 将新的融合数据加入队列中
			 m_stTestASlice.stTest4Deque.push_front(*pstTEST4Vector);
			 m_stTestASlice.stTest4Fused.u32BlendCnt++; // 融合次数加1，一次完整的TEST4识别结束
 
			 // 在融合结束后，在原图数据上右上角根据融合次数绘制白色方框,若为垂直镜像融合，则从下向上绘制，否则从上向下绘制
			 if (m_sTcProcParam.bShowEnhanceInfo == true)
			 {
				 int32_t blendIdx = m_stTestASlice.stTest4Fused.bTEST4VertMirror ? (pTest4BlendRect->uiHeight - 10) : 10;
				 int32_t AddHeigh = m_stTestASlice.stTest4Fused.bTEST4VertMirror ? (-3) : 3;
				 for (int32_t i = blendIdx, k = 0; k < (int32_t)(m_stTestASlice.stTest4Fused.u32BlendCnt - 1); i = i + AddHeigh, k++)
				 {
					 // 获得当前行的指针
					 pu16LineNewLe = (uint16_t*)pstFuseSrcData->pPlaneVir[0] + (i + pstTest4Cont->stBoundingBox.uiY) * pstFuseSrcData->u32Stride[0];
					 pu16LineNewHe = (uint16_t*)pstFuseSrcData->pPlaneVir[1] + (i + pstTest4Cont->stBoundingBox.uiY) * pstFuseSrcData->u32Stride[1];
 
					 // 遍历宽度
					 for (int32_t j = 0; j < (int32_t)10 + k * 5; j++)
					 {
						 pu16LineNewLe[j + 10] = 0;
						 pu16LineNewHe[j + 10] = 0;
					 }
				 }
			 }
		 }
	 }
 
	 // 更新解析增强使用的TEST4缓存
	 for (uint32_t m = pstTest4Cont->stBoundingBox.uiY; m < pstTest4Cont->stBoundingBox.uiY + pstTest4Cont->stBoundingBox.uiHeight; m++)
	 {
		 uint16_t* pu16HSrc = (uint16_t*)pstFuseSrcData->pPlaneVir[1] + pstFuseSrcData->u32Stride[1] * m;
		 uint16_t* pu16HDst = (uint16_t*)m_stTestASlice.stTest4Area.pPlaneVir[1] + m_stTestASlice.stTest4Area.u32Stride[1] * (m + m_stTestASlice.u32Neighbor - pstTest4Cont->stBoundingBox.uiY);
		 uint16_t* pu16Src = (uint16_t*)pstFuseSrcData->pPlaneVir[0] + pstFuseSrcData->u32Stride[0] * m;
		 uint16_t* pu16Dst = (uint16_t*)m_stTestASlice.stTest4Area.pPlaneVir[0] + m_stTestASlice.stTest4Area.u32Stride[0] * (m + m_stTestASlice.u32Neighbor - pstTest4Cont->stBoundingBox.uiY);
		 for (uint32_t n = pstTest4Cont->stBoundingBox.uiX; n < pstTest4Cont->stBoundingBox.uiX + pstTest4Cont->stBoundingBox.uiWidth; n++)
		 {
			 pu16HDst[n + m_stTestASlice.u32Neighbor - pstTest4Cont->stBoundingBox.uiX] = pu16HSrc[n];
			 pu16Dst[n + m_stTestASlice.u32Neighbor - pstTest4Cont->stBoundingBox.uiX] = pu16Src[n];
		 }
	 }
	 return ISL_OK;
 }
 
 /**
  * @fn: lslTest4AnalyEnhance
  * @berif: TEST4区域解析增强函数
  * @param: [IN]pstTest4Img: TEST4图像数据
  * @param: [IN/OUT]pstFuseSrcData: 融合后图像数据
  * @param: [OUT]test4ThRange: TEST4区域阈值范围
  * @param: [IN]pstTest4Cont: TEST4区域轮廓信息
  * @return {*}
  */
 int32_t TcProcModule::lslTest4AnalyEnhance(
	 XIMAGE_DATA_ST* pstTest4Img,
	 XIMAGE_DATA_ST* pstFuseSrcData,
	 int32_t*  pWindowCenter,
	 CONTOUR_ATTR* pstTest4Cont)
 {
	 int32_t m = 0, n = 0, k = 0, i = 0, j = 0;
	 RECT stProcAreaThin;
	 ROTATED_RECT stRRectThin;
	 bool bThintoThick = false; // 判断TEST4区域是否是自左而右，从薄到厚
	 memset(&stProcAreaThin, 0, sizeof(RECT));
	 memset(&stRRectThin, 0, sizeof(ROTATED_RECT));
 
	 /*
		 倾斜角度过大，使用二次校正来进行叠加高低能图像,上一步的TEST4就不会走到这里来
		 640宽限制倾斜角度：0-1.5
		 1152宽机型限制倾斜角度：0-3.0
	 */
	 if ((fabs(m_stTestASlice.stRectTest4.angle) > 1.5 && fabs(m_stTestASlice.stRectTest4.angle) < 11.0 && m_pSharedPara->m_nDetectorWidth == 640) ||
		 (fabs(m_stTestASlice.stRectTest4.angle) > 3.0 && fabs(m_stTestASlice.stRectTest4.angle) < 11.0 && m_pSharedPara->m_nDetectorWidth == 1152))
	 {
		 printf(" m_stTestSlice.stRectTest4.angle bigger 3.0  %f begin start weighted approach\n ", fabs(m_stTestASlice.stRectTest4.angle));
		 /* 将TEST4区域矩形框位置从全图TEST4区域更换为TEST4抠图区域,注意这里的框起始位置不一定是图像中的角点 */
		 Isl_test4_locate_fast(pstTest4Img, &stProcAreaThin, &stRRectThin);
		 if (stRRectThin.height == 0 || stRRectThin.width == 0)
		 {
			 printf("locate test4 failed, w %d, h %d, corner %u %u %u %u\n", stRRectThin.width, stRRectThin.height,
				 stRRectThin.corner[0].y, stRRectThin.corner[1].y, stRRectThin.corner[2].y, stRRectThin.corner[3].y);
			 return ISL_FAIL;
		 }
 
		 std::vector<uint32_t> hanglcol(stProcAreaThin.uiWidth);
		 uint16_t* pu16Dst = NULL;
		 uint16_t* pu16Src = NULL;
 
		 *pWindowCenter = Isl_test4_renormalize(pstTest4Img, &stProcAreaThin, NULL, hanglcol.data(), &stRRectThin); // high energy average
 
		 /* 局部增强最低窗位限制 */
		 if (100 > *pWindowCenter)
		 {
			*pWindowCenter = 100;
		 }
 
		 for (k = 0; k < 2; k++)
		 {
			 for (m = 0; m < (int32_t)pstTest4Cont->stBoundingBox.uiHeight; m++)
			 {
				 for (n = (int32_t)pstTest4Cont->stBoundingBox.uiX; n < (int32_t)pstTest4Cont->stBoundingBox.uiX + (int32_t)pstTest4Cont->stBoundingBox.uiWidth; n++)
				 {
					 pu16Dst = (uint16_t*)pstFuseSrcData->pPlaneVir[k] + pstFuseSrcData->u32Stride[k] * (m + pstTest4Cont->stBoundingBox.uiY);
					 pu16Src = (uint16_t*)pstTest4Img->pPlaneVir[k] + pstTest4Img->u32Stride[k] * (m + m_stTestASlice.u32Neighbor);
					 pu16Dst[n] = pu16Src[n - pstTest4Cont->stBoundingBox.uiX + m_stTestASlice.u32Neighbor];
				 }
			 }
		 }
		 // 在融合结束后，在原图数据上右上角根据融合次数绘制白色方框,若为垂直镜像融合，则从下向上绘制，否则从上向下绘制
		 if (m_sTcProcParam.bShowEnhanceInfo == true)
		 {
			 for (int32_t i = 5, k = 0; k < 5; i = i + 1, k++)
			 {
				 // 获得当前行的指针
				 uint16_t* pu16LineNewLe = (uint16_t*)pstFuseSrcData->pPlaneVir[0] + i * pstFuseSrcData->u32Stride[0];
				 uint16_t* pu16LineNewHe = (uint16_t*)pstFuseSrcData->pPlaneVir[1] + i * pstFuseSrcData->u32Stride[1];
				 // 遍历宽度
				 for (int32_t j = 0; j < 10; j++)
				 {
					 pu16LineNewLe[j + 5] = 0;
					 pu16LineNewHe[j + 5] = 0;
				 }
			 }
		 }
		 return ISL_OK;
	 }
 
	 printf(" m_stTestSlice.stRectTest4.angle smaller than 3.0  %f begin start merge approach\n ", fabs(m_stTestASlice.stRectTest4.angle));
	 // 以旋转矩形为模板快速定位TEST4区域
	 Isl_test4_locate_fast(pstTest4Img, &stProcAreaThin, &stRRectThin);
	 if (stRRectThin.height == 0 || stRRectThin.width == 0)
	 {
		 printf("locate test4 failed, w %d, h %d, corner %u %u %u %u\n", stRRectThin.width, stRRectThin.height,
			 stRRectThin.corner[0].y, stRRectThin.corner[1].y, stRRectThin.corner[2].y, stRRectThin.corner[3].y);
		 return ISL_FAIL;
	 }
	 // 根据查找到的TEST4区域 确定局增阈值上下限,使用高能归一化图像 实现：直方图统计 使用分布占据最高点
	 // 以非圆饼区域作为背景，对Test4区域重新做归一化（均匀度校正）
	 std::vector<uint32_t> hlcol(stProcAreaThin.uiWidth);
	 uint32_t hlavg = Isl_test4_renormalize(pstTest4Img, &stProcAreaThin, NULL, hlcol.data(), NULL); // high energy average
 
	 // 根据窗口灰度均值前一半均值和后一半均值来判定是否是自左而右，从薄到厚
	 uint32_t halfSize = stProcAreaThin.uiWidth / 2;
	 uint32_t sumFrontHalf = 0, sumBackHalf = 0;
	 for (uint32_t i = 0; i < halfSize; ++i)
	 {
		 sumFrontHalf += hlcol[i];
	 }
	 for (uint32_t i = halfSize; i < stProcAreaThin.uiWidth; ++i)
	 {
		 sumBackHalf += hlcol[i];
	 }
	 if (sumFrontHalf > sumBackHalf)
	 {
		 bThintoThick = true;
	 }
 
	 // 创建直方图均衡化图像，SP16格式，TEST4中间的2/3区域做处理，其他复制原高能图像
	 XIMAGE_DATA_ST stThinHe = *pstTest4Img;
	 stThinHe.enImgFmt = ISL_IMG_DATFMT_SP16;
	 stThinHe.pPlaneVir[0] = pstTest4Img->pPlaneVir[1];
	 stThinHe.u32Stride[0] = pstTest4Img->u32Stride[1];
 
	 // 类中的均衡直方图进行宽高赋值
	 m_stTestASlice.stTest4Lhe.u32Height = pstTest4Img->u32Height;
	 m_stTestASlice.stTest4Lhe.u32Width = pstTest4Img->u32Width;
	 m_stTestASlice.stTest4Lhe.u32Stride[0] = pstTest4Img->u32Width;
	 m_stTestASlice.stTest4Lhe.u32Stride[1] = pstTest4Img->u32Width;
 
	 // 图像填充白色
	 for (uint32_t m = 0; m < m_stTestASlice.stTest4Lhe.u32Height; m++)
	 {
		 uint16_t* pu16Src = (uint16_t*)m_stTestASlice.stTest4Lhe.pPlaneVir[0] + m_stTestASlice.stTest4Lhe.u32Stride[0] * m;
		 uint16_t* pu16HSrc = (uint16_t*)m_stTestASlice.stTest4Lhe.pPlaneVir[1] + m_stTestASlice.stTest4Lhe.u32Stride[1] * m;
		 for (uint32_t n = 0; n < m_stTestASlice.stTest4Lhe.u32Width; n++)
		 {
			 pu16Src[n] = 0XFFFF;
			 pu16HSrc[n] = 0XFFFF;
		 }
	 }
	 // 直方图均衡化处理区域，TEST4中间的2/3区域
	 RECT stLheArea = {
		 stProcAreaThin.uiX,
		 stProcAreaThin.uiY + (stProcAreaThin.uiHeight / 6),
		 stProcAreaThin.uiWidth,
		 stProcAreaThin.uiHeight * 4 / 6 };
	 ISL_VideoCrop stCopyArea = {
		 0,
		 0,
		 stThinHe.u32Width,
		 stLheArea.uiY };
 
	 Isl_ximg_copy_nb(&stThinHe, &m_stTestASlice.stTest4Lhe, &stCopyArea, &stCopyArea, XIMG_FLIP_NONE); // 上
	 stCopyArea.top = stLheArea.uiY + stLheArea.uiHeight;
	 stCopyArea.height = stThinHe.u32Height - stCopyArea.top;
	 Isl_ximg_copy_nb(&stThinHe, &m_stTestASlice.stTest4Lhe, &stCopyArea, &stCopyArea, XIMG_FLIP_NONE); // 下
	 stCopyArea.top = stLheArea.uiY;
	 stCopyArea.width = stLheArea.uiX;
	 stCopyArea.height = stLheArea.uiHeight;
	 Isl_ximg_copy_nb(&stThinHe, &m_stTestASlice.stTest4Lhe, &stCopyArea, &stCopyArea, XIMG_FLIP_NONE); // 左
	 stCopyArea.left = stLheArea.uiX + stLheArea.uiWidth;
	 stCopyArea.width = stThinHe.u32Width - stCopyArea.left;
	 Isl_ximg_copy_nb(&stThinHe, &m_stTestASlice.stTest4Lhe, &stCopyArea, &stCopyArea, XIMG_FLIP_NONE); // 右
	 uint32_t hlbase = Isl_test4_local_hist_equal(pstTest4Img, &stLheArea, hlavg, &m_stTestASlice.stTest4Lhe, pWindowCenter);
 
	 /* 局部增强最低窗位限制 */
	 if (100 > *pWindowCenter)
	 {
		 *pWindowCenter = 100;
	 }
 
	 // 将处理区域置为中间的圆饼区域
	 stProcAreaThin.uiY += stProcAreaThin.uiHeight * 32 / 100;     // 64% / 2
	 stProcAreaThin.uiHeight = stProcAreaThin.uiHeight * 36 / 100; // 36%
 
	 std::vector<uint32_t> au32WinAvg(stProcAreaThin.uiWidth);
	 uint32_t u32WinNum = 0;
	 uint32_t u32WinWidth = stProcAreaThin.uiWidth * 9 / 100;
	 uint32_t u32LocalExtrema = 0, u32PiesNum = 0;
	 uint32_t u32SemiMajorAxis = u32WinWidth >> 1, u32SemiMinorAxis = stProcAreaThin.uiHeight >> 1;
	 struct
	 {
		 int32_t confidence;               // 置信度，千分之表示，大于1000较为可信，当bValid为FALSE时，该值会置-1，用于排序到最后
		 bool bValid;                  // 是否有效
		 uint32_t extremaIdx;              // 极值点X坐标
		 int32_t fallingSum;               // 极值点前的下降沿Y轴高度
		 int32_t risingSum;                // 极值点后的上升沿Y轴高度
		 uint32_t pieIdx;                  // 圆饼索引，第几个圆饼，[0, PIE_CHART_NUM_IN_TEST4-1]
		 POS_ATTR center;              // 极值点对应的圆饼中心点
		 PIE_CAHRT_ORIENT orientation; // 缺口朝向
	 } astPiesAttr[PIE_CHART_NUM_IN_TEST4 * 2];
 
	 // 以矩形窗口每间隔1列粗略统计各窗口内的亮度均值，并根据极值点判断圆饼中心点可能的位置
	 memset(astPiesAttr, 0, sizeof(astPiesAttr));
	 /* 计算窗口内部的灰度均值 */
	 u32WinNum = Isl_win_avg_statistic(&m_stTestASlice.stTest4Lhe, 0, &stProcAreaThin, u32WinWidth, 1, au32WinAvg.data());
	 std::vector<int32_t> as32Diff1D(u32WinNum);
	 for (i = 1, j = 0; i < (int32_t)u32WinNum; i++, j++)
	 {
		 as32Diff1D[j] = au32WinAvg[i] - au32WinAvg[j];
	 }
 
	 for (i = 1; i <= (int32_t)(u32WinNum - u32SemiMajorAxis); i++)
	 {
		 if ((as32Diff1D[i] * as32Diff1D[i - 1] < 0) || (as32Diff1D[i] == 0 && as32Diff1D[i + 1] * as32Diff1D[i - 1] <= 0)) // 存在极值点
		 {
			 for (k = 0, j = i - 1, m = 0; k < (int32_t)(u32SemiMajorAxis / 2); k++, j--)
			 {
				 if (j > 0 && j < (int32_t)u32WinNum - 1)
					 m += as32Diff1D[j];
			 }
			 for (k = 0, j = i, n = 0; k < (int32_t)(u32SemiMajorAxis / 2); k++, j++)
			 {
				 n += as32Diff1D[j];
			 }
			 if (m < 0 && n > 0) // au32WinAvg[i]处有极小值
			 {
				 u32LocalExtrema = au32WinAvg[i];
				 for (k = -(int32_t)(u32SemiMajorAxis / 2); k < (int32_t)(u32SemiMajorAxis / 2); k++) // 从[i-u32SemiMajorAxis, i+u32SemiMajorAxis-1]校验极小值
				 {
					 if (i + k >= 0 && i + k < (int32_t)stProcAreaThin.uiWidth)
					 {
						 if (au32WinAvg[i + k] > au32WinAvg[i])
						 {
							 if (au32WinAvg[i + k] > u32LocalExtrema)
							 {
								 u32LocalExtrema = au32WinAvg[i + k];
							 }
						 }
						 else if (au32WinAvg[i + k] < au32WinAvg[i])
						 {
							 break;
						 }
					 }
				 }
				 if (k == (int32_t)(u32SemiMajorAxis / 2) && u32LocalExtrema - au32WinAvg[i] > au32WinAvg[i] / 20) // 极小值成立
				 {
					 if (u32PiesNum < ISL_arraySize(astPiesAttr))
					 {
						 astPiesAttr[u32PiesNum].bValid = false;
						 astPiesAttr[u32PiesNum].extremaIdx = i;
						 astPiesAttr[u32PiesNum].fallingSum = m;
						 astPiesAttr[u32PiesNum].risingSum = n;
						 u32PiesNum++;
					 }
				 }
			 }
		 }
	 }
 
	 if (u32PiesNum == 1) // 只有一个极值点的情况
	 {
		 astPiesAttr[0].bValid = true;
	 }
	 else if (u32PiesNum > 1)
	 {
		 for (i = 1; i < (int32_t)u32PiesNum; i++)
		 {
			 if (astPiesAttr[i].extremaIdx - astPiesAttr[i - 1].extremaIdx < u32SemiMajorAxis)
			 {
				 if (ISL_SUB_ABS(astPiesAttr[i].fallingSum, astPiesAttr[i].risingSum) >
					 ISL_SUB_ABS(astPiesAttr[i - 1].fallingSum, astPiesAttr[i - 1].risingSum))
				 {
					 astPiesAttr[i - 1].bValid = false;
					 astPiesAttr[i].bValid = true;
				 }
				 else
				 {
					 astPiesAttr[i - 1].bValid = true;
					 astPiesAttr[i].bValid = false;
					 i++;
				 }
			 }
			 else
			 {
				 astPiesAttr[i - 1].bValid = true;
				 if (i == (int32_t)(u32PiesNum - 1)) // 最后一个
				 {
					 astPiesAttr[i].bValid = true;
				 }
			 }
		 }
	 }
	 else
	 {
		 printf("pies number is 0000000000000\n");
		 return false;
	 }
 
	 // 根据圆饼可能的中心点位置，以椭圆区域为mask，左右小范围内再次统计置信度最高的圆饼位置
	 uint32_t confidence = 0;
	 PIE_CAHRT_ORIENT orientation;
	 POS_ATTR stCenterPos = { 0, 0 };
	 typedef struct
	 {
		 POS_ATTR center;
		 uint32_t innerAvg;
		 uint32_t confidence;
		 PIE_CAHRT_ORIENT orientation;
	 } StastPieStat;
	 std::vector<StastPieStat> astPieStat(u32WinWidth + 1);
	 uint32_t u32LocalSum = 0, u32MinSum = 0, u32MinIdx = 0, u32MaxIdx = 0;
 
	 for (i = 0, n = 0; i < (int32_t)u32PiesNum; i++)
	 {
		 if (astPiesAttr[i].bValid)
		 {
			 n++; // 临时记录有效的圆饼数量
			 for (j = 0, u32LocalSum = 0; j <= (int32_t)u32WinWidth; j++)
			 {
				 /* 极值点位置往左偏移半轴长开始查找点位 */
				 astPieStat[j].center.x = stProcAreaThin.uiX + astPiesAttr[i].extremaIdx * 2 + j - u32WinWidth / 4;
				 astPieStat[j].center.y = (stRRectThin.corner[RECT_CORNER_TOPLEFT].y + stRRectThin.corner[RECT_CORNER_BOTTOMLEFT].y) / 2 +
					 (int32_t)(tan(stRRectThin.angle * M_PI / 180.0) * (astPieStat[j].center.x - stProcAreaThin.uiX));
				 astPieStat[j].orientation = PIE_ORIENT_UNDEF;
				 /* 圆饼置信度计算和开口方向统计，核心代码 */
				 astPieStat[j].confidence = Isl_test4_pie_gray_statistic(&m_stTestASlice.stTest4Lhe, &astPieStat[j].center, u32SemiMajorAxis, u32SemiMinorAxis,
					 &astPieStat[j].innerAvg, NULL, &astPieStat[j].orientation);
				 if (j < (int32_t)u32SemiMajorAxis)
				 {
					 u32LocalSum += (uint32_t)astPieStat[j].innerAvg;
				 }
			 }
 
			 u32MinSum = u32LocalSum;
			 for (j = u32SemiMajorAxis, u32MinIdx = 0, k = 0; j <= (int32_t)u32WinWidth; j++, k++)
			 {
				 u32LocalSum -= astPieStat[k].innerAvg;
				 u32LocalSum += astPieStat[j].innerAvg;
				 if (u32MinSum > u32LocalSum)
				 {
					 u32MinSum = u32LocalSum;
					 u32MinIdx = k + 1;
				 }
			 }
 
			 for (j = u32MinIdx, confidence = 0; j < (int32_t)(u32MinIdx + u32SemiMajorAxis); j++)
			 {
				 if (astPieStat[j].confidence > confidence)
				 {
					 confidence = astPieStat[j].confidence;
					 u32MaxIdx = j;
				 }
			 }
			 astPiesAttr[i].center = astPieStat[u32MaxIdx].center;
			 astPiesAttr[i].pieIdx = (astPiesAttr[i].center.x - stProcAreaThin.uiX) * PIE_CHART_NUM_IN_TEST4 / stProcAreaThin.uiWidth;
			 astPiesAttr[i].confidence = astPieStat[u32MaxIdx].confidence;
			 astPiesAttr[i].orientation = astPieStat[u32MaxIdx].orientation;
		 }
		 else
		 {
			 astPiesAttr[i].confidence = -1; // 强制为-1，排序自动归到末尾
			 astPiesAttr[i].orientation = PIE_ORIENT_UNDEF;
		 }
		 printf("[%d] bValid %d, extremaIdx %u, falling %d, rising %d, idx %u ~ %u, center (%u, %u), conf %d, orient %d\n",
			 i, astPiesAttr[i].bValid, astPiesAttr[i].extremaIdx, astPiesAttr[i].fallingSum, astPiesAttr[i].risingSum, u32MinIdx,
			 u32MaxIdx, astPiesAttr[i].center.x, astPiesAttr[i].center.y, astPiesAttr[i].confidence, astPiesAttr[i].orientation);
	 }
 
	 /******************** 以最大置信度的圆饼为参考，计算长半轴和短半轴的长度 ********************/
	 qsort(astPiesAttr, u32PiesNum, sizeof(astPiesAttr[0]), piechartConfSort); // 对圆饼置信度从大到小排序
	 u32PiesNum = n;                                                           // 更新有效的圆饼数量
 
	 /************* 先计算长半轴，短半轴值不变 *************/
	 uint32_t semiMajorMax = (stProcAreaThin.uiWidth + 10) / 20 + 1;
	 uint32_t semiMajorStatNum = semiMajorMax - u32SemiMajorAxis;
	 uint32_t outerDivByInner = 0;
 
	 typedef struct
	 {
		 uint32_t innerAvg;
		 uint32_t outerAvg;
	 } StastMajorStat;
 
	 std::vector<StastMajorStat> astMajorStat(semiMajorStatNum);
	 for (i = u32SemiMajorAxis, k = 0; i < (int32_t)semiMajorMax; i++, k++)
	 {
		 confidence = Isl_test4_pie_gray_statistic(&m_stTestASlice.stTest4Lhe, &astPiesAttr[0].center, i, u32SemiMinorAxis,
			 &astMajorStat[k].innerAvg, &astMajorStat[k].outerAvg, &astPiesAttr[0].orientation);
		 if (confidence > 0 && (astMajorStat[k].outerAvg << 10) / astMajorStat[k].innerAvg > outerDivByInner * 105 / 100)
		 {
			 outerDivByInner = (astMajorStat[k].outerAvg << 10) / astMajorStat[k].innerAvg;
			 u32SemiMajorAxis = i;
		 }
	 }
 
	 /************* 再计算短半轴，长半轴值不变 *************/
	 uint32_t semiMinorMax = u32SemiMinorAxis * 50 / 36 + 1;
	 uint32_t semiMinorStatNum = semiMinorMax - u32SemiMinorAxis;
	 std::vector<StastMajorStat> astMinorStat(semiMinorStatNum);
	 for (j = u32SemiMinorAxis + 1, k = 0; j < (int32_t)semiMinorMax; j++, k++)
	 {
		 confidence = Isl_test4_pie_gray_statistic(&m_stTestASlice.stTest4Lhe, &astPiesAttr[0].center, u32SemiMajorAxis, j,
			 &astMinorStat[k].innerAvg, &astMinorStat[k].outerAvg, &astPiesAttr[0].orientation);
		 if (confidence > 0 && (astMinorStat[k].outerAvg << 10) / astMinorStat[k].innerAvg > outerDivByInner * 105 / 100)
		 {
			 outerDivByInner = (astMinorStat[k].outerAvg << 10) / astMinorStat[k].innerAvg;
			 u32SemiMinorAxis = j;
		 }
	 }
	 // log_info("Test4 Thin Process Area: (%u, %u) - %u x %u, Corner-Y %u %u %u %u, Angle %f, SemiAxis %u %u, PiesNum %u, hlavg %u, hlbase %u\n",
	 // 	stProcAreaThin.uiX, stProcAreaThin.uiY, stProcAreaThin.uiWidth, stProcAreaThin.uiHeight,
	 // 	stRRectThin.corner[0].y, stRRectThin.corner[1].y, stRRectThin.corner[2].y, stRRectThin.corner[3].y,
	 // 	stRRectThin.angle, u32SemiMajorAxis, u32SemiMinorAxis, u32PiesNum, hlavg, hlbase);
 
	 // 基于置信度高的圆饼，执行X坐标的自校正
	 int32_t s32ExpCenterX[PIE_CHART_NUM_IN_TEST4] = { 0 }; // 相邻圆饼距离差满足等差数列
	 bool bPieIdxExist[PIE_CHART_NUM_IN_TEST4] = { 0 }; // 第n个圆饼属性是否已计算得到
	 struct
	 {
		 double a;
		 double d;
		 int32_t s1;
		 int32_t n1;
		 int32_t s2;
		 int32_t n2;
	 } arithmetic;
	 memset(&arithmetic, 0, sizeof(arithmetic));
 
	 for (i = 0; i < (int32_t)u32PiesNum; i++)
	 {
		 if (astPiesAttr[i].confidence > 0)
		 {
			 if (!bPieIdxExist[astPiesAttr[i].pieIdx])
			 {
				 bPieIdxExist[astPiesAttr[i].pieIdx] = true;
				 if (0 == s32ExpCenterX[astPiesAttr[i].pieIdx])
				 {
					 s32ExpCenterX[astPiesAttr[i].pieIdx] = astPiesAttr[i].center.x;
				 }
				 if (i >= 2 && astPiesAttr[2].confidence > 1000) // 大于2个且置信度大于1000才能做自校正
				 {
					 if (i == 2) // 计算校正参数
					 {
						 for (j = 0; j < PIE_CHART_NUM_IN_TEST4; j++)
						 {
							 if (s32ExpCenterX[j] > 0) // the first one
							 {
								 for (k = j + 1; k < PIE_CHART_NUM_IN_TEST4; k++)
								 {
									 if (s32ExpCenterX[k] > 0)
									 {
										 if (0 == arithmetic.n1)
										 {
											 arithmetic.s1 = s32ExpCenterX[k] - s32ExpCenterX[j];
											 arithmetic.n1 = k - j;
										 }
										 else
										 {
											 arithmetic.s2 = s32ExpCenterX[k] - s32ExpCenterX[j];
											 arithmetic.n2 = k - j;
										 }
									 }
								 }
								 break;
							 }
						 }
						 if (1 == arithmetic.n1)
						 {
							 arithmetic.a = arithmetic.s1;
							 arithmetic.d = ((double)arithmetic.s2 - arithmetic.a * arithmetic.n2) * 2 / (double)(arithmetic.n2 * (arithmetic.n2 - 1));
						 }
						 else
						 {
							 arithmetic.d = ((double)arithmetic.s2 / arithmetic.n2 - (double)arithmetic.s1 / arithmetic.n1) * 2 / (arithmetic.n1 - 1);
							 arithmetic.a = (double)arithmetic.s1 / arithmetic.n1 - arithmetic.d * (arithmetic.n1 - 1) / 2;
						 }
 
						 for (k = 0; k < PIE_CHART_NUM_IN_TEST4; k++)
						 {
							 if (s32ExpCenterX[k] == 0)
							 {
								 if (k < j) // 递减
								 {
									 s32ExpCenterX[k] = s32ExpCenterX[j] -
										 (int32_t)((arithmetic.a - arithmetic.d) * (j - k) - arithmetic.d * (j - k) * (j - k - 1) / 2);
								 }
								 else // 递增
								 {
									 s32ExpCenterX[k] = s32ExpCenterX[j] +
										 (int32_t)(arithmetic.a * (k - j) + arithmetic.d * (k - j) * (k - j - 1) / 2);
								 }
							 }
						 }
					 }
					 else
					 {
						 orientation = PIE_ORIENT_UNDEF;
						 stCenterPos.x = (s32ExpCenterX[astPiesAttr[i].pieIdx] + astPiesAttr[i].center.x) / 2;
						 stCenterPos.y = (stRRectThin.corner[RECT_CORNER_TOPLEFT].y + stRRectThin.corner[RECT_CORNER_BOTTOMLEFT].y) / 2 +
							 (int32_t)(tan(stRRectThin.angle * M_PI / 180.0) * (stCenterPos.x - stProcAreaThin.uiX));
						 confidence = Isl_test4_pie_gray_statistic(&m_stTestASlice.stTest4Lhe, &stCenterPos, u32SemiMajorAxis, u32SemiMinorAxis,
							 NULL, NULL, &orientation);
						 // 与预期相差较小取均值，相差较大直接取消
						 if (ISL_SUB_ABS(s32ExpCenterX[astPiesAttr[i].pieIdx], astPiesAttr[i].center.x) <= 4 &&
							 orientation == astPiesAttr[i].orientation)
						 {
							 astPiesAttr[i].confidence = confidence;
						 }
						 else
						 {
							 astPiesAttr[i].confidence = ISL_MIN(confidence, 500);
							 astPiesAttr[i].orientation = PIE_ORIENT_UNDEF;
						 }
						 astPiesAttr[i].center = stCenterPos;
					 }
				 }
			 }
			 else // 存在重复的pieIdx，取消置信度低的圆饼
			 {
				 for (m = i + 1; m < (int32_t)u32PiesNum; m++) // 后面的依次往前移一个单位
				 {
					 memcpy(astPiesAttr + (m - 1), astPiesAttr + m, sizeof(astPiesAttr[0]));
				 }
				 u32PiesNum--;
				 i--;
			 }
		 }
		 else
		 {
			 break;
		 }
	 }
 
	 // 有预估参数，补全剩下的圆饼信息
	 if (u32PiesNum > 2 && arithmetic.n1 > 0)
	 {
		 for (i = 0; i < PIE_CHART_NUM_IN_TEST4; i++)
		 {
			 if (!bPieIdxExist[i])
			 {
				 k = u32PiesNum++;
				 astPiesAttr[k].pieIdx = i;
				 astPiesAttr[k].center.x = s32ExpCenterX[i];
				 astPiesAttr[k].center.y = (stRRectThin.corner[RECT_CORNER_TOPLEFT].y + stRRectThin.corner[RECT_CORNER_BOTTOMLEFT].y) / 2;
				 astPiesAttr[k].orientation = PIE_ORIENT_UNDEF;
				 astPiesAttr[k].confidence = Isl_test4_pie_gray_statistic(&m_stTestASlice.stTest4Lhe, &astPiesAttr[k].center, u32SemiMajorAxis, u32SemiMinorAxis,
					 NULL, NULL, &astPiesAttr[k].orientation);
			 }
		 }
	 }
 
	 // 类中的均衡直方图进行宽高赋值
	 m_stTestASlice.stMaskImg.u32Height = pstTest4Img->u32Height;
	 m_stTestASlice.stMaskImg.u32Width = pstTest4Img->u32Width;
	 m_stTestASlice.stMaskImg.u32Stride[0] = pstTest4Img->u32Width;
	 Isl_ximg_fill_color(&m_stTestASlice.stMaskImg, 0, m_stTestASlice.stMaskImg.u32Height, 0, m_stTestASlice.stMaskImg.u32Width, 0xFFFFFF);
 
	 int32_t ConfirmNum = 0;
	 // 确认TEST4区域内的置信度较大圆饼数量
	 for (i = 0; i < (int32_t)u32PiesNum; i++)
	 {
		 if (astPiesAttr[i].confidence > 800)
		 {
			 ConfirmNum++;
		 }
	 }
 
	 // 根据圆饼信息创建mask
	 for (i = 0; i < (int32_t)u32PiesNum; i++)
	 {
		 /* 添加根据铅块厚度方向进行灰度叠加，提高真实性 */
		 Isl_test4_create_mask(&m_stTestASlice.stMaskImg, &astPiesAttr[i].center, u32SemiMajorAxis, u32SemiMinorAxis, astPiesAttr[i].confidence,
			 astPiesAttr[i].orientation, m_sTcProcParam.testAEnhanceGrade, bThintoThick, ConfirmNum, stProcAreaThin.uiWidth);
	 }
 
	 uint8_t* pu8Weight = NULL;
	 uint16_t* pu16Weight = NULL;
	 uint16_t* pu16He = NULL;
	 uint32_t hloffs = hlavg - hlbase; // 偏移值：均值 - 基值
	 hlbase = ISL_MAX(hlbase, 32); // 基值太小，圆饼与背景对比度不明显，限制至少在32
	 // 为了能使探测器的不均匀性与原图一致，每列（均值-偏移值）再与一致性的基值进行加权，这样图像略显自然
	 for (k = 0; k < (int32_t)stProcAreaThin.uiWidth; k++)
	 {
		 hlcol[k] = ISL_SUB_SAFE(hlcol[k], hloffs);
		 hlcol[k] = (hlcol[k] * 4 + hlbase * 12) >> 4;
	 }
 
	 /**
	  * 以mask为权重模板，Test4再归一化高能值+偏移值，圆饼区域的偏移值低，背景区域的偏移值高
	  * 这里的计算方法要与算法库内部的局增逻辑（除了窗宽窗位，还有低通滤波、二次校正等）相匹配，不然效果会不理想
	  */
	  /* 将mask模版与抠图区域的高能值进行加权 */
	 if (m_sTcProcParam.testAEnhanceGrade >= 10)
	 {
		 for (m = 0; m < (int32_t)pstTest4Img->u32Height; m++)
		 {
			 pu16He = (uint16_t*)pstTest4Img->pPlaneVir[1] + pstTest4Img->u32Stride[1] * m;
			 pu8Weight = (uint8_t*)m_stTestASlice.stMaskImg.pPlaneVir[0] + m_stTestASlice.stMaskImg.u32Stride[0] * m;
			 for (n = 0; n < (int32_t)stProcAreaThin.uiX; n++)
			 {
				 pu16He[n] += hlbase * pu8Weight[n] >> 8;
			 }
			 for (k = 0; k < (int32_t)stProcAreaThin.uiWidth; n++, k++)
			 {
				 pu16He[n] += hlcol[k] * pu8Weight[n] >> 8;
			 }
			 for (; n < (int32_t)pstTest4Img->u32Width; n++)
			 {
				 pu16He[n] += hlbase * pu8Weight[n] >> 8;
			 }
		 }
	 }
 
	 for (k = 0; k < 2; k++)
	 {
		 for (m = 0; m < (int32_t)pstTest4Img->u32Height - 2 * (int32_t)m_stTestASlice.u32Neighbor; m++)
		 {
			 pu16He = (uint16_t*)pstFuseSrcData->pPlaneVir[k] + pstFuseSrcData->u32Stride[k] * (m + pstTest4Cont->stBoundingBox.uiY);
			 pu16Weight = (uint16_t*)pstTest4Img->pPlaneVir[k] + pstTest4Img->u32Stride[k] * (m + m_stTestASlice.u32Neighbor);
			 for (n = 0; n < (int32_t)pstTest4Img->u32Width - 2 * (int32_t)m_stTestASlice.u32Neighbor; n++)
			 {
				 pu16He[n + pstTest4Cont->stBoundingBox.uiX] = pu16Weight[n + m_stTestASlice.u32Neighbor];
			 }
		 }
	 }
 
	 // 在融合结束后，在原图数据上右上角根据融合次数绘制白色方框,若为垂直镜像融合，则从下向上绘制，否则从上向下绘制
	 if (m_sTcProcParam.bShowEnhanceInfo == true)
	 {
		 for (int32_t i = 5, k = 0; k < 10; i = i + 1, k++)
		 {
			 // 获得当前行的指针
			 uint16_t* pu16LineNewLe = (uint16_t*)pstFuseSrcData->pPlaneVir[0] + i * pstFuseSrcData->u32Stride[0];
			 uint16_t* pu16LineNewHe = (uint16_t*)pstFuseSrcData->pPlaneVir[1] + i * pstFuseSrcData->u32Stride[1];
			 // 遍历宽度
			 for (int32_t j = 0; j < 5; j++)
			 {
				 pu16LineNewLe[j + 5] = 0;
				 pu16LineNewHe[j + 5] = 0;
			 }
		 }
	 }
 
	 return ISL_OK;
 }
 
 
 //-----------------------------------------LORB start-------------------------------------//
 /* TODO：
	 1、这里有两套方案可以选，因为TESTB是固定不变的所以可以弄四个bin文件用于匹配；耗时较短
	 2、计算全图的的主特征方向，然后再做图像旋转操作，耗时较长 */
 
 /* PCA图像主特征计算 */
 struct PCAResult {
	 double angle; // 主方向角度（弧度）
	 double eigenvalue; // 最大特征值
	 vector<double> eigenvector; // 对应的特征向量
	 vector<vector<double>> covariance_matrix; // 协方差矩阵
 };
 
 // 首先对图像进行边缘处理
 vector<pair<int32_t, int32_t>> edgeDetection(XIMAGE_DATA_ST* img) {
 
	 int32_t height = img->u32Height;
	 int32_t width = img->u32Width;
 
	 int32_t sobel_x[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
	 int32_t sobel_y[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };
 
	 vector<pair<int32_t, int32_t>> edges;
 
	 for (int32_t y = 0; y < height; ++y) 
	 {
		 for (int32_t x = 0; x < width; ++x) 
		 {
			 int32_t Gx = 0, Gy = 0;
 
			 for (int32_t k = 0; k < 3; ++k) 
			 {
				 for (int32_t l = 0; l < 3; ++l) 
				 {
					 int32_t xi = y - 1 + k;
					 int32_t xj = x - 1 + l;
 
					 if (xi >= 0 && xi < height && xj >= 0 && xj < width) 
					 {
						 uint16_t* pPlane = (uint16_t*)img->pPlaneVir[0];
						 Gx += pPlane[xi * img->u32Stride[0] + xj] * sobel_x[k][l];
						 Gy += pPlane[xi * img->u32Stride[0] + xj] * sobel_y[k][l];
					 }
				 }
			 }
 
			 if (abs(Gx) + abs(Gy) > 100) 
			 {
				 edges.emplace_back(x, y);
			 }
		 }
	 }
 
	 return edges;
 }
 
 PCAResult ComputePCA(XIMAGE_DATA_ST* img) {
	 PCAResult result;
 
	 vector<pair<int32_t, int32_t>> points = edgeDetection(img);
	 int32_t count = points.size();
	 if (count == 0) return result;
 
	 // 计算均值
	 double mean_x = 0, mean_y = 0;
	 for (const auto& p : points) {
		 mean_x += p.first;
		 mean_y += p.second;
	 }
	 mean_x /= count;
	 mean_y /= count;
 
	 // 计算协方差矩阵
	 result.covariance_matrix.resize(2, vector<double>(2, 0));
	 for (const auto& p : points) {
		 double x = p.first - mean_x;
		 double y = p.second - mean_y;
		 result.covariance_matrix[0][0] += x * x;
		 result.covariance_matrix[0][1] += x * y;
		 result.covariance_matrix[1][0] += y * x;
		 result.covariance_matrix[1][1] += y * y;
	 }
	 result.covariance_matrix[0][0] /= count;
	 result.covariance_matrix[0][1] /= count;
	 result.covariance_matrix[1][0] /= count;
	 result.covariance_matrix[1][1] /= count;
 
	 // 特征值分解
	 double a = result.covariance_matrix[0][0];
	 double b = result.covariance_matrix[0][1];
	 double c = result.covariance_matrix[1][1];
	 double discriminant = sqrt((a - c) * (a - c) + 4 * b * b);
	 double eig1 = (a + c + discriminant) / 2;
	 double eig2 = (a + c - discriminant) / 2;
	 result.eigenvalue = max(eig1, eig2);
 
	 double angle_rad = 0;
	 if (eig1 > eig2) {
		 angle_rad = 0.5 * atan2(2 * b, a - c);
	 }
	 else {
		 angle_rad = 0.5 * atan2(-2 * b, a - c);
	 }
	 result.angle = angle_rad;
 
	 return result;
 }
 
 /* 从文件中读取FastBriefPix */
 int32_t LORB_read_FastBriefPix(const char* filename, FastBriefPix *fastpix)
 {
	 FILE* fp = fopen(filename, "rb");
	 if (fp == NULL) 
	 {
		 log_error("open file %s failed\n", filename);
		 return -1;
	 }

	 size_t read_total = fread(&fastpix->total, sizeof(int32_t), 1, fp);
	 if (read_total != 1) 
	 {
		 log_error("Failed to read total from file %s\n", filename);
		 fclose(fp);
		 return -1;
	 }
 
	 size_t read_height_index = fread(fastpix->height_index, sizeof(int32_t), XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH, fp);
	 if (read_height_index != XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH) 
	 {
		 log_error("Failed to read height_index from file %s\n", filename);
		 fclose(fp);
		 return -1;
	 }
 
	 size_t read_width_index = fread(fastpix->width_index, sizeof(int32_t), XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH, fp);
	 if (read_width_index != XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH) 
	 {
		 log_error("Failed to read width_index from file %s\n", filename);
		 fclose(fp);
		 return -1;
	 }
 
	 size_t read_brief = fread(fastpix->brief, sizeof(uint8_t), 8 * XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH, fp);
	 if (read_brief != 8 * XING_TESTB_FB_HEIGHT * XING_TESTB_FB_WIDTH) 
	 {
		 log_error("Failed to read brief from file %s\n", filename);
		 fclose(fp);
		 return -1;
	 }
 
	 fclose(fp);
 
	 return 0;
 }
 
 /**
  * @fn: LORB_get_FastPoint
  * fast:features from accelerated segment test	
  * @berif: 角点提取
  * @param: [IN]img: 区域图像，尽量精简，减少不需要的轮次计算
  * @param: [IN]threshold: 角点查找阈值
  * @param: [OUT]fastpix: FAST找点信息
  * @return {*}
  */
 int32_t LORB_get_FastPointAndBrief(XIMAGE_DATA_ST* img, int32_t threshold, FastBriefPix *fastpix, bool bOutFile)
 {
	 // 用于计算对应周围4*4各像素的灰度值
	 const int32_t offset_row[16] = { -3, -3, -2, -1, 0, 1, 2, 3, 3, 3, 2, 1, 0, -1, -2, -3 };
	 const int32_t offset_col[16] = { 0, 1, 2, 3, 3, 3, 2, 1, 0, -1, -2, -3, -3, -3, -2, -1 };
 
	 // 64个维度
	 const int32_t r1_height[64] = { 8 ,6 ,-7 ,-2 ,-10 ,4 ,12 ,10 ,6 ,-6 ,-5 ,1 ,-1 ,5 ,8 ,-2 ,0 ,7 ,3 ,-10 ,-5 ,\
	 - 5 ,-8 ,-1 ,1 ,9 ,4 ,7 ,-5 ,2 ,10 ,-4 ,4 ,8 ,-6 ,3 ,-6 ,-3 ,-4 ,-3 ,-2 ,-5 ,-4 ,3 ,10 ,-10 ,\
	 - 2 ,2 ,3 ,6 ,1 ,-11 ,4 ,6 ,-3 ,-2 ,-4 ,-1 ,-6 ,7 ,-8 ,-2 ,8 ,2 };
	 const int32_t r1_width[64] = { -5 ,3 ,6 ,2 ,-1 ,0 ,-1 ,-9 ,2 ,7 ,0 ,-4 ,4 ,7 ,3 ,3 ,1 ,-4 ,1 ,7 ,0 ,4 ,0 ,4 ,\
		 - 4 ,-7 ,-15 ,5 ,2 ,5 ,4 ,-11 ,1 ,12 ,6 ,-1 ,-3 ,-2 ,2 ,1 ,-6 ,-3 ,-2 ,7 ,6 ,2 ,-8 ,1 ,5 ,-1 ,-8 ,\
		 - 5 ,5 ,0 ,2 ,2 ,5 ,6 ,3 ,3 ,2 ,-7 ,6 ,6 };
	 const int32_t r2_height[64] = { -2 ,-3 ,2 ,6 ,4 ,11 ,-6 ,-1 ,-13 ,3 ,5 ,1 ,-10 ,-13 ,11 ,-8 ,6 ,-1 ,0 ,-4 ,4 ,\
		 - 1 ,-7 ,-2 ,0 ,-1 ,3 ,2 ,-16 ,-5 ,-1 ,3 ,4 ,3 ,-2 ,9 ,-5 ,-9 ,3 ,6 ,6 ,3 ,5 ,1 ,10 ,\
		 8 ,1 ,5 ,7 ,9 ,-1 ,8 ,7 ,6 ,7 ,-1 ,4 ,7 ,-3 ,-8 ,4 ,1 ,3 ,0 };
	 const int32_t r2_width[64] = { 2 ,-1 ,-4 ,-10 ,0 ,4 ,6 ,-1 ,-3 ,11 ,11 ,15 ,-6 ,-8 ,1 ,2 ,4 ,-3 ,-1 ,\
		 - 2 ,-12 ,-5 ,0 ,-3 ,5 ,4 ,2 ,4 ,13 ,-2 ,17 ,12 ,8 ,-12 ,-4 ,-8 ,1 ,-7 ,-3 ,9 ,-8 ,-11 ,\
		 - 5 ,-2 ,4 ,1 ,-2 ,1 ,-7 ,2 ,6 ,0 ,7 ,-2 ,6 ,-8 ,0 ,4 ,-6 ,-9 ,-4 ,-2 ,8 ,-6 };
 
	 for (uint32_t i = 3; i < (img->u32Height - 3); i++)
	 {
		 for (uint32_t j = 3; j < (img->u32Width - 3); j++)
		 {
			 int32_t sum = 0;
			 // 简化运算，首先计算1、5、9、13位置是否满足候选特征点要求,减少运算量
			 for (int32_t k = 0; k < 4; k++)
			 {
				 uint16_t* pu16CenterRow = (uint16_t*)img->pPlaneVir[0] + img->u32Stride[0] * (img->u32Height - 1 - i);
				 uint16_t* pu16NeighborRow = (uint16_t*)img->pPlaneVir[0] + img->u32Stride[0] * (img->u32Height - 1 - (i + offset_row[4 * k]));
 
				 if (ISL_ABS((pu16NeighborRow[j + offset_col[4 * k]] - pu16CenterRow[j])) > threshold)
				 {
					 sum++;
				 }
			 }
 
			 if (sum < 3)
			 {
				 continue;
			 }
 
			 sum = 0;
 
			 for (int32_t k = 0; k < 16; k++)
			 {
				 uint16_t* pu16CenterRow = (uint16_t*)img->pPlaneVir[0] + img->u32Stride[0] * (img->u32Height - 1 - i);
				 uint16_t* pu16NeighborRow = (uint16_t*)img->pPlaneVir[0] + img->u32Stride[0] * (img->u32Height - 1 - (i + offset_row[k]));
				 if (ISL_ABS((pu16NeighborRow[j + offset_col[k]] - pu16CenterRow[j])) > threshold)
				 {	
					 sum++;
				 }
			 }
			 // 超过9个，则认为center是Fast特征点,并记录角点坐标
			 if (sum > 9)
			 {
				 if ((i > 16 && i < (img->u32Height - 16)) && (j > 16 && j < (img->u32Width - 16)))
				 {
					 fastpix->height_index[fastpix->total] = i;
					 fastpix->width_index[fastpix->total] = j;
					 fastpix->total++;
				 }
			 }
		 }
	 }
 
	 /* 查找所有的角点,并获取描述子 */
	 for (int32_t i = 0; i < fastpix->total; i++)
	 {
		 int32_t heigh_index = 0;
		 int32_t width_index = 0;
		 heigh_index = fastpix->height_index[i];
		 width_index = fastpix->width_index[i];
		 uint8_t brief[8] = { 0 };
		 for (int32_t j = 0; j < 64; j++)
		 {
			 int32_t index = j / 8;
			 uint16_t* r1_value = (uint16_t*)img->pPlaneVir[0] + img->u32Stride[0] * (img->u32Height - 1 - (heigh_index + r1_height[j]));
			 uint16_t* r2_value = (uint16_t*)img->pPlaneVir[0] + img->u32Stride[0] * (img->u32Height - 1 - (heigh_index + r2_height[j]));
 
			 brief[index] = brief[index] << 1;
			 if (r1_value[width_index + r1_width[j]] > r2_value[width_index + r2_width[j]])
			 {
				 brief[index] += 1;
			 }
			 int32_t temp_index = i * 8 + index;
			 fastpix->brief[temp_index] = brief[index];
		 }
	 }
 
	 /* 输出整个fastpix文件到本地作为匹配文件 */
	 if (bOutFile)
	 {
		 FILE* fp = fopen("./fastpix.bin", "wb");
		 if (NULL == fp)
		 {
			 log_error("open file failed\n");
			 return -1;
		 }
 
		 // 先写入total
		 fwrite(&fastpix->total, sizeof(int32_t), 1, fp);
 
		 // 写入角点坐标信息
		 fwrite(fastpix->height_index, sizeof(int32_t), 400 * 150, fp);
		 fwrite(fastpix->width_index, sizeof(int32_t), 400 * 150, fp);
 
		 // 写入brief信息
		 fwrite(fastpix->brief, sizeof(uint8_t), 8 * 400 * 150, fp);
		 fclose(fp);
	 }
	 return fastpix->total;
 }
 
 /* 匹配特征点 */
 float32_t LORB_match_FastBrief(FastBriefPix* fastpix1, FastBriefPix* fastpix2)
 {
	 int32_t i, j, k, m;
	 int32_t total_point = 0;
	 int32_t match_thres = 8;//15
 
	 for (m = 0; m < (fastpix1->total); m++)
	 {
		 int32_t Mincount = 64;
		 for (i = 0; i < fastpix2->total; i++)
		 {
			 int32_t count = 0;
			 for (j = 0; j < 8; j++)
			 {
				 uint8_t p1, p2, one;
				 p1 = fastpix1->brief[m * 8 + j];
				 p2 = fastpix2->brief[i * 8 + j]; 
				 one = (p1 ^ p2);
				 for (k = 0; k < 8; k++)
				 {
					 if (one % 2 == 1)
					 {
						 count++;
					 }
					 one = one >> 1;
				 }
			 }
			 if (count < Mincount)
			 {
				 Mincount = count;
			 }
		 }
		 if (Mincount < match_thres)
		 {
			 total_point++;
		 }
	 }
 
	 return (total_point *1.0 / fastpix1->total);
 }
 //-----------------------------------------LORB end--------------------------------------//
 
 /**
  * @fn      Isl_testA_realtime_detect_by_slice
  * @brief   国标测试体A的实时检测,根据拼接的条带进行处理
  * @param   [IN] pstSourceSliceNraw 条带的NRaw数据
  * @return  [return] TESTA_SLICE_PROCESS_STATE             测试体A的TEST区域的各个状态
			 TESTA_SLICE_UNKNOW                      分片状态未知等待匹配
			 TESTA_SLICE_END_MATCH_FIRST             第一块穿透力区域识别结束
			 TESTA_SLICE_END_MATCH_LINE_PAIR         空间分辨条带一部分识别结束
			 TESTA_SLICE_END_MATCH_SECOND            第二块穿透力区域识别结束
  */
 TEST_SLICE_PROCESS_STATE TcProcModule::lslTestASliceDetect(XIMAGE_DATA_ST* pstSourceSliceNraw, int32_t rtSliceSeqsize)
 {
	 uint16_t* pu16Src = NULL, * pu16Dst = NULL;
	 uint8_t* pu8Dst = NULL;
	 CONTOUR_ATTR stSliceNowCont; // 暂存轮廓参数
	 uint32_t u32PointNum = 0, u32Neighbor = 1;
	 POS_ATTR* pstPointStart = NULL;
	 memset(&stSliceNowCont, 0, sizeof(CONTOUR_ATTR));
 
	 /* 条带宽高赋值并进行扩边1像素处理 */
	 m_stTestASlice.stNrawMatchSlice.u32Height = pstSourceSliceNraw->u32Height + 2 * u32Neighbor;
	 m_stTestASlice.stNrawMatchSlice.u32Width = pstSourceSliceNraw->u32Width + 2 * u32Neighbor;
	 m_stTestASlice.stNrawMatchSlice.u32Stride[0] = m_stTestASlice.stNrawMatchSlice.u32Width;
 
	 m_stTestASlice.stNrawBinarySlice.u32Height = pstSourceSliceNraw->u32Height + 2 * u32Neighbor;
	 m_stTestASlice.stNrawBinarySlice.u32Width = pstSourceSliceNraw->u32Width + 2 * u32Neighbor;
	 m_stTestASlice.stNrawBinarySlice.u32Stride[0] = m_stTestASlice.stNrawBinarySlice.u32Width;
 
	 if (m_stTestASlice.stNrawMatchSlice.u32Height > 1152)
	 {
		 log_info("the input parameters are invalid, h:%u\n",
			 m_stTestASlice.stNrawMatchSlice.u32Height);
		 return TEST_SLICE_UNKNOW;
	 }
 
	 /* 参数校验 */
	 if (NULL == pstSourceSliceNraw)
	 {
		 log_info("pstSourceSliceNraw is NULL!!! \n");
		 return TEST_SLICE_UNKNOW;
	 }
 
	 /* 条带处理图像填充 */
	 Isl_ximg_fill_color(&m_stTestASlice.stNrawMatchSlice,
		 0, m_stTestASlice.stNrawMatchSlice.u32Height,
		 0, m_stTestASlice.stNrawMatchSlice.u32Width,
		 0XFFFF);
	 Isl_ximg_fill_color(&m_stTestASlice.stNrawBinarySlice,
		 0, m_stTestASlice.stNrawBinarySlice.u32Height,
		 0, m_stTestASlice.stNrawBinarySlice.u32Width,
		 0XFF);
 
	 /* 图像数据复制，并填充宽高各两个像素 */
	 for (uint32_t m = 0; m < pstSourceSliceNraw->u32Height; m++)
	 {
		 pu16Src = (uint16_t*)pstSourceSliceNraw->pPlaneVir[0] + pstSourceSliceNraw->u32Stride[0] * m;
		 pu16Dst = (uint16_t*)m_stTestASlice.stNrawMatchSlice.pPlaneVir[0] + m_stTestASlice.stNrawMatchSlice.u32Stride[0] * (m + u32Neighbor);
		 for (uint32_t n = 0; n < pstSourceSliceNraw->u32Width; n++)
		 {
			 pu16Dst[n + u32Neighbor] = pu16Src[n];
		 }
	 }
 
	 if ((TEST_SLICE_UNKNOW == m_stTestASlice.eSliceState) || (TESTA_SLICE_START_MATCH_SECOND == m_stTestASlice.eSliceState))
	 {		
		 if (TESTA_SLICE_START_MATCH_SECOND == m_stTestASlice.eSliceState)
		 {
			 /* 如果到结束条带仍然没有找到数字部分，直接跳出，重新进行匹配 */
			 if (2 * rtSliceSeqsize < (int32_t)m_stTestASlice.Nrawsliceindex)
			 {
				 m_stTestASlice.Nrawsliceindex = 0; // 重置内部条带计数
				 m_stTestASlice.eSliceState = TEST_SLICE_UNKNOW;
				 goto EXIT;
			 }
		 }
		 /* 根据包裹阈值进行二值化，找到条带边界 */
		 for (uint32_t m = 0; m < m_stTestASlice.stNrawMatchSlice.u32Height; m++)
		 {
			 pu16Src = (uint16_t*)m_stTestASlice.stNrawMatchSlice.pPlaneVir[0] + m_stTestASlice.stNrawMatchSlice.u32Stride[0] * m;
			 pu8Dst = (uint8_t*)m_stTestASlice.stNrawBinarySlice.pPlaneVir[0] + m_stTestASlice.stNrawBinarySlice.u32Stride[0] * m;
			 for (uint32_t n = 0; n < m_stTestASlice.stNrawMatchSlice.u32Width; n++)
			 {
				 if (pu16Src[n] > m_stTestASlice.u32PackageBinTh)
				 {
					 pu8Dst[n] = 0XFF;
				 }
				 else
				 {
					 pu8Dst[n] = 0;
				 }
			 }
		 }
		 /* 查找整体条带边缘,并生成轮廓 */
		 m_stTestASlice.u32ContourNum = ISL_arraySize(m_stTestASlice.astContAttr);
		 memset(&m_stTestASlice.astContAttr, 0, XIMG_CONTOURS_NUM_MAX * sizeof(CONTOUR_ATTR));
		 find_contours(&m_stTestASlice.stNrawMatchSlice, &m_stTestASlice.stNrawBinarySlice, u32Neighbor, 1,
			 true, 5000, m_stTestASlice.astContAttr, &m_stTestASlice.u32ContourNum,
			 m_stTestASlice.aContourGrpRange, m_stTestASlice.paContourPoints, &m_stTestASlice.stBorderCircum, NULL);
		 if (m_stTestASlice.u32ContourNum > 0)
		 {
			 qsort(m_stTestASlice.astContAttr, m_stTestASlice.u32ContourNum, sizeof(CONTOUR_ATTR), contourAreaSort); // 对轮廓面积从大到小排序
			 memcpy(&m_stTestASlice.stPackageCont, m_stTestASlice.astContAttr, sizeof(CONTOUR_ATTR));
		 }
		 else
		 {
			 goto EXIT;
		 }
 
		 // 测试体B检测
		 if(true == m_sTcProcParam.bOpenTestBeDet)
		 {
			 /* 根据包裹轮廓切割出包裹区域 */
			 XIMAGE_DATA_ST stXIMAGE_PKG;
			 stXIMAGE_PKG.enImgFmt = m_stTestASlice.stNrawMatchSlice.enImgFmt;
			 stXIMAGE_PKG.u32Width = m_stTestASlice.stPackageCont.stBoundingBox.uiWidth;
			 stXIMAGE_PKG.u32Height = m_stTestASlice.stPackageCont.stBoundingBox.uiHeight;
			 stXIMAGE_PKG.u32Stride[0] = m_stTestASlice.stNrawMatchSlice.u32Stride[0];
			 stXIMAGE_PKG.pPlaneVir[0] = (uint16_t*)m_stTestASlice.stNrawMatchSlice.pPlaneVir[0] + 
				 m_stTestASlice.stPackageCont.stBoundingBox.uiY * m_stTestASlice.stNrawMatchSlice.u32Stride[0] + m_stTestASlice.stPackageCont.stBoundingBox.uiX;
 
			 // 这里条带处理耗时会增加2ms左右，在原先ORB算法基础上，减少了多尺寸和旋转尺寸的计算
			 LORB_get_FastPointAndBrief(&stXIMAGE_PKG, 10000, &m_stTestBSlice.TESTBFastPix, true);
			 float32_t Result_0 = LORB_match_FastBrief(&m_stTestBSlice.FastPixDefault_0, &m_stTestBSlice.TESTBFastPix);
			 float32_t Result_1 = LORB_match_FastBrief(&m_stTestBSlice.FastPixDefault_1, &m_stTestBSlice.TESTBFastPix);
			 m_stTestBSlice.TESTBFastPix.total = 0;
 
			 // 不论首尾，只要匹配到了对应的TESTB就立马调用TESTB颜色微调
			 if ((Result_0 > 0.5) || (Result_1 > 0.5))
			 {
				 m_stTestASlice.Nrawsliceindex = 0; // 重置内部条带计数
				 m_stTestASlice.eSliceState = TESTB_SLICE_BEGIN;
				 m_nTestBStartIdx = m_vecSliceSeq.back(); // 记录当前测试体B的起始位置
				 goto EXIT;
			 }
		 }
 
		 /* 根据TEST4阈值进行二值化，找到条带边界 */
		 for (uint32_t m = u32Neighbor; m < m_stTestASlice.stNrawMatchSlice.u32Height - u32Neighbor; m++)
		 {
			 pu16Src = (uint16_t*)m_stTestASlice.stNrawMatchSlice.pPlaneVir[0] + m_stTestASlice.stNrawMatchSlice.u32Stride[0] * m;
			 pu8Dst = (uint8_t*)m_stTestASlice.stNrawBinarySlice.pPlaneVir[0] + m_stTestASlice.stNrawBinarySlice.u32Stride[0] * m;
			 for (uint32_t n = u32Neighbor; n < m_stTestASlice.stNrawMatchSlice.u32Width - u32Neighbor; n++)
			 {
				 if (pu16Src[n] > m_stTestASlice.u32Test4BinTh)
				 {
					 pu8Dst[n] = 0XFF;
				 }
				 else
				 {
					 pu8Dst[n] = 0;
				 }
			 }
		 }
 
		 /* 查找TEST4包裹边缘,并生成轮廓 */
		 m_stTestASlice.u32ContourNum = ISL_arraySize(m_stTestASlice.astContAttr);
		 memset(&m_stTestASlice.astContAttr, 0, XIMG_CONTOURS_NUM_MAX * sizeof(CONTOUR_ATTR));
		 find_contours(&m_stTestASlice.stNrawMatchSlice, &m_stTestASlice.stNrawBinarySlice, u32Neighbor, 1,
			 true, 1000, m_stTestASlice.astContAttr, &m_stTestASlice.u32ContourNum,
			 m_stTestASlice.aContourGrpRange, m_stTestASlice.paContourPoints, &m_stTestASlice.stBorderCircum, NULL);
		 if (m_stTestASlice.u32ContourNum > 0)
		 {
			 qsort(m_stTestASlice.astContAttr, m_stTestASlice.u32ContourNum, sizeof(CONTOUR_ATTR), contourAreaSort); // 对轮廓面积从大到小排序
			 memcpy(&stSliceNowCont, m_stTestASlice.astContAttr, sizeof(CONTOUR_ATTR));
		 }
 
		 /* TESTA中TEST4的轮廓宽度占据整个包裹的一半以上,且置于中间位置，和四边无接触 */
		 if (bRectEnveloped(&stSliceNowCont.stBoundingBox, &m_stTestASlice.stPackageCont.stBoundingBox) &&
			 (stSliceNowCont.stBoundingBox.uiWidth * 10 > m_stTestASlice.stPackageCont.stBoundingBox.uiWidth * 5) &&
			 ((stSliceNowCont.stBoundingBox.uiY + stSliceNowCont.stBoundingBox.uiHeight) < m_stTestASlice.stNrawMatchSlice.u32Height - 2 * u32Neighbor) &&
			 (stSliceNowCont.stBoundingBox.uiY > u32Neighbor))
		 {
			 /* 计算最小外接矩形 */
			 pstPointStart = m_stTestASlice.paContourPoints + m_stTestASlice.aContourGrpRange[m_stTestASlice.astContAttr[0].u32ContourNo - 1].u32IdxStart;
			 u32PointNum = m_stTestASlice.aContourGrpRange[m_stTestASlice.astContAttr[0].u32ContourNo - 1].u32IdxEnd -
				 m_stTestASlice.aContourGrpRange[m_stTestASlice.astContAttr[0].u32ContourNo - 1].u32IdxStart + 1;
			 rotatedRectFitting((POINT_INT*)pstPointStart, u32PointNum, &m_stTestASlice.stRectTest4);
			 if (ISL_SUB_ABS((uint32_t)m_stTestASlice.stRectTest4.area, m_stTestASlice.astContAttr[0].u32Area) < (uint32_t)ISL_MAX(m_stTestASlice.stRectTest4.width, m_stTestASlice.stRectTest4.height) * 5)
			 {
				 if (TESTA_SLICE_START_MATCH_SECOND == m_stTestASlice.eSliceState)
				 {
					 m_stTestASlice.eSliceState = TESTA_SLICE_END_MATCH_SECOND;
				 }
				 else
				 {
					 m_stTestASlice.eSliceState = TESTA_SLICE_END_MATCH_FIRST;
				 }
				 goto EXIT;
			 }
		 }
		 if (TESTA_SLICE_START_MATCH_SECOND == m_stTestASlice.eSliceState)
			 m_stTestASlice.Nrawsliceindex++;
	 }
 
	 /* 测试体B颜色微调状态 */
	 if ((TESTB_SLICE_BEGIN == m_stTestASlice.eSliceState) || (TESTB_SLICE_WAITING_END == m_stTestASlice.eSliceState))
	 {
		 m_stTestASlice.eSliceState = TESTB_SLICE_WAITING_END;
		 /* 直接对接下来的TESTB未知区域都做颜色微调操作，不做TESTB结束的判断 */
		 if (6 * rtSliceSeqsize < (int32_t)m_stTestASlice.Nrawsliceindex)
		 {
			 m_stTestASlice.Nrawsliceindex = 0; // 重置内部条带计数
			 m_stTestASlice.eSliceState = TEST_SLICE_UNKNOW;
			 goto EXIT;
		 }
		 m_stTestASlice.Nrawsliceindex++;
	 }
 
	 /* 这一状态过度，是为了让外部读取到条带状态，以此来对之前缓存的数据进行不同的处理 */
	 if (TESTA_SLICE_END_MATCH_FIRST == m_stTestASlice.eSliceState)
	 {
		 m_stTestASlice.eSliceState = TESTA_SLICE_START_MATCH_LINE_PAIR;
	 }
 
	 if (TESTA_SLICE_END_MATCH_LINE_PAIR == m_stTestASlice.eSliceState)
	 {
		 m_stTestASlice.eSliceState = TESTA_SLICE_START_MATCH_LINE_PAIR;
	 }
 
	 /* 开始匹配线对区域，这部分需要进行对缓存图像做处理, 条带的高度需要小于TEST3空间分辨区域的长度
		 1、四对竖线段计划分成两部分，需要统计竖线段从上到下的宽的大小 来确定 进口方向：1.6 -> 0.8 or 0.8 -> 1.6
		 2、两对竖线段的区分 依赖第一条竖线段的长度,这里面为了适配TEST3增强部分，所以只使用三像素矩阵进行线对匹配和增强
	 */
	 if (TESTA_SLICE_START_MATCH_LINE_PAIR == m_stTestASlice.eSliceState)
	 {
		 if (2 * rtSliceSeqsize < (int32_t)m_stTestASlice.Nrawsliceindex)
		 {
			 m_stTestASlice.Nrawsliceindex = 0; // 重置内部条带计数
			 m_stTestASlice.uTest3LineNum = 0;
			 m_stTestASlice.eSliceState = TESTA_SLICE_START_MATCH_SECOND; // 强制跳转到寻找第二块TEST4区域
			 goto EXIT;
		 }
 
		 uint16_t* pu16RawLine[3];
		 uint8_t* pu8DstLine[3];
 
		 /* 对TEST3区域阈值进行横向膨胀，将线对连接在一起，再做二值化 */
		 for (uint32_t m = 2; m < m_stTestASlice.stNrawMatchSlice.u32Height - 2; m++)
		 {
			 for (int32_t i = 0; i < 3; i++)
			 {
				 pu16RawLine[i] = (uint16_t*)m_stTestASlice.stNrawMatchSlice.pPlaneVir[0] + m_stTestASlice.stNrawMatchSlice.u32Stride[0] * (m + i - 1);
				 pu8DstLine[i] = (uint8_t*)m_stTestASlice.stNrawBinarySlice.pPlaneVir[0] + m_stTestASlice.stNrawBinarySlice.u32Stride[0] * (m + i - 1);
			 }
			 /* 这里的阈值查找TEST4 */
			 for (uint32_t n = 2; n < m_stTestASlice.stNrawMatchSlice.u32Width - 2; n++)
			 {
				 if (((pu16RawLine[1][n - 1] > 20000) && (pu16RawLine[1][n - 1] < 40000)) &&
					 ((pu16RawLine[1][n + 1] > 20000) && (pu16RawLine[1][n + 1] < 40000)) &&
					 ((pu16RawLine[0][n] > 20000) && (pu16RawLine[0][n] < 40000)) &&
					 ((pu16RawLine[2][n] > 20000) && (pu16RawLine[2][n] < 40000)))
				 {
					 for (int32_t k = 0; k < 3; k++)
					 {
						 pu8DstLine[k][n - 1] = 0;
						 pu8DstLine[k][n] = 0;
						 pu8DstLine[k][n + 1] = 0;
					 }
				 }
				 if ((pu16RawLine[1][n] - pu16RawLine[0][n] > 10000) &&
					 (pu16RawLine[1][n] - pu16RawLine[2][n] > 10000) &&
					 (ISL_ABS(pu16RawLine[1][n + 1] - pu16RawLine[1][n]) < 2000) &&
					 (ISL_ABS(pu16RawLine[1][n - 1] - pu16RawLine[1][n]) < 2000))
				 {
					 for (int32_t k = 0; k < 3; k++)
					 {
						 pu8DstLine[k][n - 1] = 0;
						 pu8DstLine[k][n] = 0;
						 pu8DstLine[k][n + 1] = 0;
					 }
				 }
			 }
		 }
 
		 /* 查找TEST4包裹边缘,并生成轮廓 */
		 m_stTestASlice.u32ContourNum = ISL_arraySize(m_stTestASlice.astContAttr);
		 find_contours(&m_stTestASlice.stNrawMatchSlice, &m_stTestASlice.stNrawBinarySlice, u32Neighbor, 1,
			 true, 70, m_stTestASlice.astContAttr, &m_stTestASlice.u32ContourNum,
			 m_stTestASlice.aContourGrpRange, m_stTestASlice.paContourPoints, &m_stTestASlice.stBorderCircum, NULL);
		 qsort(m_stTestASlice.astContAttr, m_stTestASlice.u32ContourNum, sizeof(CONTOUR_ATTR), contourHorSort); // 对轮廓从左到右排序
 
		 /* 对矩形面积进行相似度过滤，这里的线对判断根据空间位置强绑定
			 竖线对的宽度占比：<6%
			 竖线对相对空位置比例：55%~85% 镜像时为15%~45%*/
		 RECT stTEST3BoundingBox = { 0, 0, 0, 0 };       // TEST3区域轮廓包围盒
		 RECT stTEST3MirrorBoundingBox = { 0, 0, 0, 0 }; // TEST3区域镜像轮廓包围盒
		 stTEST3BoundingBox.uiX = (uint32_t)(m_stTestASlice.stPackageCont.stBoundingBox.uiX + 0.55 * m_stTestASlice.stPackageCont.stBoundingBox.uiWidth);
		 stTEST3BoundingBox.uiWidth = (uint32_t)(0.3 * m_stTestASlice.stPackageCont.stBoundingBox.uiWidth);
		 stTEST3BoundingBox.uiY = m_stTestASlice.stPackageCont.stBoundingBox.uiY;
		 stTEST3BoundingBox.uiHeight = m_stTestASlice.stPackageCont.stBoundingBox.uiHeight;
 
		 stTEST3MirrorBoundingBox.uiX = (uint32_t)(m_stTestASlice.stPackageCont.stBoundingBox.uiX + 0.15 * m_stTestASlice.stPackageCont.stBoundingBox.uiWidth);
		 stTEST3MirrorBoundingBox.uiWidth = (uint32_t)(0.3 * m_stTestASlice.stPackageCont.stBoundingBox.uiWidth);
		 stTEST3MirrorBoundingBox.uiY = m_stTestASlice.stPackageCont.stBoundingBox.uiY;
		 stTEST3MirrorBoundingBox.uiHeight = m_stTestASlice.stPackageCont.stBoundingBox.uiHeight;
 
		 for (uint32_t i = 0; i < m_stTestASlice.u32ContourNum; i++)
		 {
			 /* 空间分辨查找结束标志 */
			 if (bRectEnveloped(&m_stTestASlice.astContAttr[i].stBoundingBox, &stTEST3BoundingBox) ||
				 bRectEnveloped(&m_stTestASlice.astContAttr[i].stBoundingBox, &stTEST3MirrorBoundingBox))
			 {
				 /* 矩形相似度计算，用来筛选TEST3区域 */
				 if ((85 < GetRectSimilarity(&m_stTestASlice.astContAttr[i].stBoundingBox, &m_stTestASlice.stNrawBinarySlice)) &&
					 (m_stTestASlice.astContAttr[i].stBoundingBox.uiY + m_stTestASlice.astContAttr[i].stBoundingBox.uiHeight < (m_stTestASlice.stNrawMatchSlice.u32Height - 10)) &&
					 (m_stTestASlice.astContAttr[i].stBoundingBox.uiY > 1) &&
					 (10 * m_stTestASlice.astContAttr[i].stBoundingBox.uiWidth >= 8 * m_stTestASlice.astContAttr[i].stBoundingBox.uiHeight))
				 {
					 /* 记录当前的TEST3区域大致位置 ，用来限定TEST3增强区域 */
					 m_stTestASlice.uTest3LineNum++;
					 // 外部记录TEST3区域大致位置
					 if (true == bRectEnveloped(&m_stTestASlice.astContAttr[i].stBoundingBox, &stTEST3MirrorBoundingBox))
					 {
						 memcpy(&m_stTestASlice.stRectTest3PackDir, &stTEST3MirrorBoundingBox, sizeof(RECT));
						 m_stTestASlice.stRectTest3PackDir.uiWidth = m_stTestASlice.astContAttr[i].stBoundingBox.uiX - stTEST3MirrorBoundingBox.uiX;
					 }
					 else
					 {
						 memcpy(&m_stTestASlice.stRectTest3PackDir, &stTEST3BoundingBox, sizeof(RECT));
						 m_stTestASlice.stRectTest3PackDir.uiX = m_stTestASlice.astContAttr[i].stBoundingBox.uiX + m_stTestASlice.astContAttr[i].stBoundingBox.uiWidth;
						 m_stTestASlice.stRectTest3PackDir.uiWidth = m_stTestASlice.astContAttr[i].stBoundingBox.uiWidth;
					 }
					 m_stTestASlice.eSliceState = TESTA_SLICE_END_MATCH_LINE_PAIR;
 
					 if (m_stTestASlice.uTest3LineNum >= 4)
					 {
						 m_stTestASlice.uTest3LineNum = 0;
						 m_stTestASlice.Nrawsliceindex = 0;
						 m_stTestASlice.eSliceState = TESTA_SLICE_START_MATCH_SECOND;
					 }
					 goto EXIT;
				 }
			 }
		 }
		 m_stTestASlice.Nrawsliceindex++;
	 }
 
	 if (TESTA_SLICE_END_MATCH_SECOND == m_stTestASlice.eSliceState)
	 {
		 m_stTestASlice.Nrawsliceindex = 0;
		 m_stTestASlice.eSliceState = TESTA_SLICE_WAITING_END;
		 goto EXIT;
	 }
 
	 if (TESTA_SLICE_WAITING_END == m_stTestASlice.eSliceState)
	 {
		 if (rtSliceSeqsize < (int32_t)m_stTestASlice.Nrawsliceindex)
		 {
			 m_stTestASlice.Nrawsliceindex = 0; // 重置内部条带计数
			 m_stTestASlice.eSliceState = TEST_SLICE_UNKNOW;
			 goto EXIT;
		 }
		 else
		 {
			 m_stTestASlice.Nrawsliceindex++;
		 }
	 }
 
 EXIT:
	 return m_stTestASlice.eSliceState;
 }
 
 /**
  * @fn      IslTcEnhance
  * @brief   测试体增强，Test chart Enhance
  * @note    当前仅支持国标测试体A
  * @param   [IN/OUT] rtSliceData 输入：实时过包连续条带数据，低能、高能、Z三平面分别连续，XRAYLIB_IMG_RAW_LHZx格式
  *                               输出：仅会更新低能与高能图像，且属性与输入格式均相同
  * @param   [IN] m_vecSliceSeq rtSliceData对应的条带序列号，rtSliceSeq.size()需被rtSliceData.height整除
  * @param   [OUT] test4ThRange Test4局增阈值上下限（如果有Test4），中值为均值，返回值为true且两个值都大于0时有效
  * @return  bool true-有测试体增强，且rtSliceData有更新；false-无测试体增强
  */
 TEST_SLICE_PROCESS_STATE TcProcModule::IslTcEnhance(XMat& MatLow, XMat& MatHigh, int32_t nWidth, int32_t nHeight, int32_t *pWindowCenter)
 {
	 XRAYLIB_IMAGE rtSliceData;
	 rtSliceData.width = nWidth;
	 rtSliceData.height = nHeight;
	 rtSliceData.pData[0] = MatLow.PadPtr();
	 rtSliceData.pData[1] = MatHigh.PadPtr();  
	 rtSliceData.stride[0] = nWidth;
	 rtSliceData.stride[1] = nWidth;
	 rtSliceData.format = XRAYLIB_IMG_RAW_LHZ16;
 
	 XIMAGE_DATA_ST SrcSliceData; // 当前条带的需要处理的缓存数据
	 CONTOUR_ATTR stTest4Cont;    // TEST4区域轮廓
	 int32_t nRtSliceNum = (int32_t)m_vecSliceSeq.size();
	 int32_t nProcessedSliceNum = 0;
	 TEST_SLICE_PROCESS_STATE enTestCase = TEST_SLICE_UNKNOW; // 测试体A的状态
	 memset(&stTest4Cont, 0, sizeof(CONTOUR_ATTR));
	 memset(&SrcSliceData,0,sizeof(XIMAGE_DATA_ST));
 
	 // 检查输入参数
	 if (0 == m_sTcProcParam.testAEnhanceGrade)
	 {
		 return enTestCase;
	 }
 
	 /* 将拼接条带进行结构体转换*/
	 SrcSliceData.enImgFmt = ISL_IMG_DATFMT_SP16;
	 /* 根据已经处理完的条带序号来决定送入条带 */
	 nProcessedSliceNum = ISL_ABS(static_cast<int32_t>(m_vecSliceSeq.back()) - static_cast<int32_t>(m_sTcProcParam.SliceIndex));
	 /* 需要处理的条带数量最大为输入条带数量 */
	 if (nProcessedSliceNum > (int32_t)m_vecSliceSeq.size())
	 {
		 nProcessedSliceNum = (int32_t)m_vecSliceSeq.size();
	 }
 
	 memcpy(&SrcSliceData.u32Width, &rtSliceData.width, sizeof(XRAYLIB_IMAGE) - sizeof(XRAYLIB_IMG_FORMAT));
 
	 // 保存前一组处理完的数据,这里是整个缓存条带进行缓存
	 m_stTestASlice.stPreNrawSliceData.u32Width = SrcSliceData.u32Width;
	 m_stTestASlice.stPreNrawSliceData.u32Height = SrcSliceData.u32Height;
	 m_stTestASlice.stPreNrawSliceData.u32Stride[0] = SrcSliceData.u32Stride[0];
	 m_stTestASlice.stPreNrawSliceData.u32Stride[1] = SrcSliceData.u32Stride[1];
 
	 SrcSliceData.pPlaneVir[0] = (short*)SrcSliceData.pPlaneVir[0] + (SrcSliceData.u32Stride[0] *
		 (rtSliceData.height / m_vecSliceSeq.size()) * (m_vecSliceSeq.size() - nProcessedSliceNum));
	 SrcSliceData.pPlaneVir[1] = (short*)SrcSliceData.pPlaneVir[1] + (SrcSliceData.u32Stride[1] *
		 (rtSliceData.height / m_vecSliceSeq.size()) * (m_vecSliceSeq.size() - nProcessedSliceNum));
	 SrcSliceData.u32Height = (uint32_t)(rtSliceData.height / m_vecSliceSeq.size() * nProcessedSliceNum);
	 SrcSliceData.pPlaneVir[2] = nullptr;
	 SrcSliceData.pPlaneVir[3] = nullptr;
 
	 enTestCase = lslTestASliceDetect(&SrcSliceData, nRtSliceNum);
	 // 根据测试体A的状态来判断进行怎样的增强处理
	 switch (enTestCase)
	 {
		 // 状态未知，不做处理
		 case TEST_SLICE_UNKNOW:
			 m_sTcProcParam.SliceIndex = 0;
			 break;
		 case TESTA_SLICE_END_MATCH_FIRST:
		 case TESTA_SLICE_END_MATCH_SECOND:
			 printf("End match the TEST4!!! \n");
			 // 解析增强处理之前还需要对原图像进行扩边等预处理
			 IslTest4Expand(&SrcSliceData, &m_stTestASlice.stTest4Area, &stTest4Cont);
			 // TEST4融合增强流程
			 lslTest4FuseEnhance(&SrcSliceData, &stTest4Cont, enTestCase);
			 // TEST4局增解析增强
			 lslTest4AnalyEnhance(&m_stTestASlice.stTest4Area, &SrcSliceData, pWindowCenter, &stTest4Cont);
 
			 /* 将处理完的数据缓存一份，之前处理完的条带也需要复制过去 */
			 for (int32_t i = 0; i < 2; i++)
			 {
				 memcpy((int8_t*)rtSliceData.pData[i], 
					 (int8_t*)m_stTestASlice.stPreNrawSliceData.pPlaneVir[i] + (nProcessedSliceNum * (rtSliceData.height / nRtSliceNum)) * SrcSliceData.u32Stride[i] * sizeof(uint16_t), 
					 SrcSliceData.u32Stride[i] * (nRtSliceNum - nProcessedSliceNum) * (rtSliceData.height / nRtSliceNum) * sizeof(uint16_t));
				 memcpy((int8_t*)m_stTestASlice.stPreNrawSliceData.pPlaneVir[i], (int8_t*)rtSliceData.pData[i], rtSliceData.stride[i] * rtSliceData.height * sizeof(uint16_t));
			 }
			 m_sTcProcParam.SliceIndex = m_vecSliceSeq.back();
			 break;
		 case TESTA_SLICE_END_MATCH_LINE_PAIR:
			 printf("End match the TEST3!!! \n");
			 /* 分别更新高低能数据 */
			 lslTest3AnalyEnhance(&SrcSliceData, m_stTestASlice.stRectTest3PackDir.uiY, SrcSliceData.u32Height, m_stTestASlice.stRectTest3PackDir.uiX, m_stTestASlice.stRectTest3PackDir.uiX + m_stTestASlice.stRectTest3PackDir.uiWidth);
 
			 /* 将处理完的数据缓存一份，之前处理完的条带也需要复制过去 */
			 for (int32_t i = 0; i < 2; i++)
			 {
				 memcpy((int8_t*)rtSliceData.pData[i], 
					 (int8_t*)m_stTestASlice.stPreNrawSliceData.pPlaneVir[i] + (nProcessedSliceNum * (rtSliceData.height / nRtSliceNum)) * SrcSliceData.u32Stride[i] * sizeof(uint16_t),
					 SrcSliceData.u32Stride[i] * (nRtSliceNum - nProcessedSliceNum) * (rtSliceData.height / nRtSliceNum) * sizeof(uint16_t));
				 memcpy((int8_t*)m_stTestASlice.stPreNrawSliceData.pPlaneVir[i], (int8_t*)rtSliceData.pData[i], rtSliceData.stride[i] * rtSliceData.height * sizeof(uint16_t));
			 }
			 m_sTcProcParam.SliceIndex = m_vecSliceSeq.back();
			 break;
		 default:
			 // 其他状态下，将前一组处理完的数据拷贝回给之前的数据
			 if (nProcessedSliceNum < (int32_t)m_vecSliceSeq.size())
			 {
				 for (int32_t i = 0; i < 2; i++)
				 {
					 for (uint32_t m = 0; m < ((m_vecSliceSeq.size() - nProcessedSliceNum) * rtSliceData.height / m_vecSliceSeq.size()); m++)
					 {
						 uint16_t* pu16Src = (uint16_t*)m_stTestASlice.stPreNrawSliceData.pPlaneVir[i] + m_stTestASlice.stPreNrawSliceData.u32Stride[i] * (m + nProcessedSliceNum * rtSliceData.height / m_vecSliceSeq.size());
						 uint16_t* pu16Dst = (uint16_t*)rtSliceData.pData[i] + rtSliceData.stride[i] * m;
						 for (uint32_t n = 0; n < SrcSliceData.u32Width; n++)
						 {
							 pu16Dst[n] = pu16Src[n];
						 }
					 }
				 }
			 }
			 break;
	 }
 
	 /*条带序列递增*/
	 for (size_t i = 0; i < m_vecSliceSeq.size(); i++) 
	 {
		 m_vecSliceSeq[i] += 1;
	 }
	 return enTestCase;
 }
 