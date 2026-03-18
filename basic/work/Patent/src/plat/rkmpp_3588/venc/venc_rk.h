/**
 * @file   venc_rk.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  编码组件---plat层接口封装
 * @author wangzhenya5
 * @date   2022/03/29
 * @note   rk3588 venc 平台层接口封装
 */

#ifndef _VENC_RK_H_
#define _VENC_RK_H_

#include <sal.h>
#include <dspcommon.h>
#include <platform_hal.h>
#include "hal_inc_inter/venc_hal_inter.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define VENC_RK_CHN_NUM       (6 + 2 + 2 + 16 + 2) /*6 VI_ENC,其他:抓图2路vi抓图2路智能抓图16路解码抓图2路转存*/
#define FRAME_MAX_NALU_NUM    (4)                  /* rk h265 有4个nal包信息,该值应该从plat层获取 */

#define VENC_MIN_WIDTH        (32)
#define VENC_MIN_HEIGHT       (32)

#define VENC_MAX_WIDTH        (4096)
#define VENC_MAX_HEIGHT       (4096)

#define VENC_RK_JPEG_MIN_WIDTH   (96)
#define VENC_RK_JPEG_MIN_HEIGHT  (96)

#define VENC_JPEG_MAX_WIDTH   (4096)
#define VENC_JPEG_MAX_HEIGHT  (2160)

#define MPI_ENC_IO_COUNT      (4)

#define VENC_ALIGN(x, a)      (((x) + (a) - 1) & ~((a) - 1))
#define MPP_VSWAP(a, b)       { a ^= b; b ^= a; a ^= b; }

#if 0
/**
 * @function   venc_saveJpegStream
 * @brief    调试接口，保存Jpeg编码码流
 * @param[in]  VENC_STREAM_S *pstStream 码流信息
 * @param[out] FILE *fpH264File 目标文件
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
HI_S32 venc_saveJpegStream(FILE *fpMJpegFile, VENC_STREAM_S *pstStream);

/**
 * @function   venc_saveH264Stream
 * @brief    调试接口，保存H264编码码流
 * @param[in]  VENC_STREAM_S *pstStream 码流信息
 * @param[out] FILE *fpH264File 目标文件
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
HI_S32 venc_saveH264Stream(FILE *fpH264File, VENC_STREAM_S *pstStream);

/**
 * @function   venc_saveH265Stream
 * @brief    调试接口，保存H265编码码流
 * @param[in]  VENC_STREAM_S *pstStream 码流信息
 * @param[out] FILE *fpH264File 目标文件
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
HI_S32 venc_saveH265Stream(FILE *fpH265File, VENC_STREAM_S *pstStream);

/**
 * @function   venc_query
 * @brief   Venc通道查询当前帧的码流包个数
 * @param[in]  UINT32 u32Chan 编码通道
 * @param[out]  UINT32 *puiFrameCnt 码流包个数
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_query(UINT32 u32Chan, UINT32 *pFrameCnt);

/**
 * @function   venc_select
 * @brief   Venc通道事件数据
 * @param[in]  UINT32 u32Chan 编码通道
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_select(UINT32 u32Chan);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /*_VENC_HISI_H_*/



