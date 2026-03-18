/*******************************************************************************
 * jpeg_soft.h
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : wangweiqin <wangweiqin@hikvision.com.cn>
 * Version: V1.0.0  2018年01月16日 Create
 *
 * Description : JPEG 软件解码
 * Modification:
 *******************************************************************************/


#ifndef _JPEG_SOFT_H_
#define _JPEG_SOFT_H_

#include "sal.h"
#include "video_dec_commom.h"
#include "jpgd_lib.h"

#define HIK_DIC_MAX_WIDTH   (4096)
#define HIK_DIC_MAX_HEIGHT  (4096)

INT32 HikJpgDec(UINT8* inBuf, UINT32 inSize, UINT8* outBuf, UINT32 *outbuflen, UINT32* width, UINT32* height);


#endif /* _JPEG_SOFT_H_ */

