/**
 * @file   vca_pack_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  vca封装函数接口
 * @author huangshuxin
 * @date   2019年5月24日 Create
 *
 * @note \n History
   1.Date        : 2019年5月24日 Create
     Author      : huangshuxin
     Modification: 新建文件
   2.Date        : 2021/08/19
     Author      : yindongping
     Modification: 组件开发，整理接口
 */


#ifndef _VCA_PACK_API_H_
#define _VCA_PACK_API_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */




/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
/* *****************控制信息私有类型**************** / */
typedef struct _HIK_COMMAND_INFO_
{
    unsigned int tag;         /* "CMMD" 大写 */
    unsigned int version;     /* 版本，默认 */
    unsigned int flag;        /* 命令类型 */
    unsigned int resvered;    /* 保留位 */
} HIK_COMMAND_INFO;


typedef struct tagVcaPakcInfost
{
    UINT32 u32Dev;       
    UINT32 u32Chn;
    UINT32 u32StreamId;

    UINT32 u32Width;
    UINT32 u32Height;    
    UINT64 u64Pts;
    
    UINT32 isUsePs;
    UINT32 isUseRtp;
    UINT32 u32PsHandle;
    UINT32 u32RtpHandle;
    UINT32 u32PackBufSize;
    UINT8 *pPackOutputBuf;
} VCA_PACK_INFO_ST;

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/**
 * @function   vca_pack_init
 * @brief      初始化vca pack通道
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_pack_init(UINT32 u32Chan);

/**
 * @function   vca_pack_makeAlgSyncCmd
 * @brief      ivs同步信息封装
 * @param[in]  void *pstBuff 源数据
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_pack_makeAlgSyncCmd(void *pstBuff);

/**
 * @function   vca_pack_makeAlgIvsInfoHdr
 * @brief      ivs信息封装
 * @param[in]  void *pstBuff 源数据
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_pack_makeAlgIvsInfoHdr(void *pstBuff);

/**
 * @function   vca_pack_makeAlgPosInfoHdr
 * @brief      pos信息封装
 * @param[in]  void *pstBuff 源数据
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_pack_makeAlgPosInfoHdr(void *pstBuff);

/**
 * @function   vca_pack_process
 * @brief      vca pack
 * @param[in]  VCA_PACK_INFO_ST *pstVcaPackInfo 封装信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_pack_process(VCA_PACK_INFO_ST *pstVcaPackInfo);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _VCA_PACK_H_ */




