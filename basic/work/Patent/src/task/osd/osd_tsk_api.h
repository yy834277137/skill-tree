/**
 * @file   osd_tsk_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  osd 模块 tsk 层
 * @author heshengyuan
 * @date   2014年10月28日 Create
 * @note
 * @note \n History
   1.Date        : 2014年10月28日 Create
     Author      : heshengyuan
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#ifndef _OSD_TSK_API_H_
#define _OSD_TSK_API_H_
/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include <sal.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数声明                                        */
/* ========================================================================== */
/**
 * @function    osd_tsk_init
 * @brief       初始化OSD字符，给编码、解码、预览
 * @param[in]   None
 * @param[out]  None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_init(void);

/**
 * @function    osd_tsk_setEncPrm
 * @brief       主机命令 HOST_CMD_SET_ENC_OSD 设置编码视频的OSD
 * @param[in]   UINT32 u32Chan vi通道
 * @param[in]   UINT32 *pParam 使能标记
 * @param[in]   UINT32 *pBuf OSD参数
 * @param[out]  None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_setEncPrm(UINT32 u32Chan, UINT32 *pParam, UINT32 *pBuf);

/**
 * @function    osd_tsk_startEncProc
 * @brief      主机命令 HOST_CMD_START_ENC_OSD 启动编码OSD
 * @param[in]   UINT32 u32Chan vi通道
 * @param[in]   UINT32 *pParam 使能标记
 * @param[in]   UINT32 *pBuf OSD参数，inv
 * @param[out]  None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_startEncProc(UINT32 u32Chan, UINT32 *pParam, UINT32 *pBuf);

/**
 * @function    osd_tsk_stopEncProc
 * @brief       主机命令 HOST_CMD_STOP_ENC_OSD 关闭编码OSD
 * @param[in]   UINT32 u32Chan vi通道
 * @param[in]   UINT32 *pParam 使能标记
 * @param[in]   UINT32 *pBuf OSD参数
 * @param[out]  None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_stopEncProc(UINT32 u32Chan, UINT32 *pParam, UINT32 *pBuf);

/**
 * @function    osd_tsk_startDST
 * @brief       osd启用夏令时
 * @param[in]   UINT32 u32ViChan vi通道
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_startDST(UINT32 u32Chan, UINT32 *pBuf);


/**
 * @function    osd_tsk_hangEncProc
 * @brief       停止编码OSD显示
 * @param[in]   UINT32 u32Chan vi通道
 * @param[in]   UINT32 u32StreamId 码流id
 * @param[in]   UINT32 u32VenChnId venc hal通道id
 * @param[out]  None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_hangEncProc(UINT32 u32Chan, UINT32 u32StreamId, UINT32 u32VenChnId);

/**
 * @function    osd_tsk_restart
 * @brief       重新启动OSD
 * @param[in]   UINT32 u32Chan vi通道
 * @param[out]  None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
void osd_tsk_restart(UINT32 u32Chan);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /*_OSD_TSK_H_*/


