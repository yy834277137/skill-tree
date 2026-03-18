
#ifndef _DSPTTK_CMD_XPACK_H_
#define _DSPTTK_CMD_XPACK_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "dspcommon.h"
#include "sal_type.h"

/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */

typedef struct
{
    UINT32 u32SaveIdx;
    UINT32 u32Width;
    UINT32 u32Height;
    UINT64 u64Time;
    CHAR cJpgPath[256];
} DSPTTK_XPACK_SAVE_PIC_INFO;

typedef struct
{
    UINT32 u32SaveScreenCnt;           /* 当前保存屏幕的总数*/
    DSPTTK_XPACK_SAVE_PIC_INFO stSavePicInfo[1024];
} DSPTTK_XPACK_SAVE_PRM;


/**
 * @fn      dspttk_get_save_screen_list_info
 * @brief   获取save图个数
 * 
 * @param   [IN] u32Chan XRay通道号
 * 
 * @return  [OUT]  save信息结构体
 */
DSPTTK_XPACK_SAVE_PRM *dspttk_get_save_screen_list_info(UINT32 u32Chan);


/**
 * @function   dspttk_xpack_set_segment_attr
 * @brief      分片上传属性
 * @param[in]  unsigned int chn
 * @param[in]  XPACK_DSP_SEGMENT_OUT *pstPackRst
 * @param[out] None
 * @return     UINT64
 */
UINT64 dspttk_xpack_set_segment_attr(UINT32 u32Chan);

/**
 * @fn      dspttk_xpack_set_yuv_offset
 * @brief   设置设置yuv显示偏移量
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参， 
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xpack_set_yuv_offset(UINT32 u32Chan);

/**
 * @fn      dspttk_xpack_set_disp_sync_time
 * @brief   设置同步显示超时时间
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参， 
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xpack_set_disp_sync_time(UINT32 u32Chan);

/**
 * @fn      dspttk_xpack_set_jpg
 * @brief   设置JPG抓图参数
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参， 
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xpack_set_jpg(UINT32 u32Chan);

/**
 * @fn      dspttk_xpack_save_screen
 * @brief   SAVE，保存当前屏幕上的图像数据
 *
 * @param   [IN] u32Chan 无效参数，实际不区分通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xpack_save_screen(UINT32 u32Chan);

#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_CMD_XPACK_H_ */

