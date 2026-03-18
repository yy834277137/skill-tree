/**
 * @file   vca_pack.c
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


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <arpa/inet.h>

#include <dspcommon.h>
#include "sal.h"

#include "IVS_SYS_lib.h"
#include "vca_common.h"
#include "type_dscrpt_common.h"
#include "libmux.h"

#include "system_prm_api.h"
#include "capbility.h"
#include "task_ism.h"
#include "sva_out.h"
#include "vca_pack.h"
#include "vca_pack_api.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define VCA_PACK_MAX_CHAN_NUM (6)

#define BUFFER_SIZE					(1024 * 1024)
#define TEST_TARGET_NUM				64
#define CMMD_POS_SYNC_FLAG			(1 << 0) /* /<POS同步标记 */
#define CMMD_VIDEO_PRIV_SYNC_FLAG	(1 << 1) /* /<智能信息采用新的更精确阈值进行同步匹配以及匹配处理--此功能已废弃，改用IVS解析的type表示 */

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
/* ************************************************** / */

typedef struct ivs_hdr_s
{
    unsigned short data_type;
    unsigned char marker_bit_0 : 1;
    unsigned char def_version : 7;
    unsigned short frame_num_high;
    unsigned char marker_bit_1 : 1;
    unsigned char reserved : 7;
    unsigned short frame_num_low;
} __attribute__((packed, aligned(1))) ivs_hdr_t;

typedef struct cmd_hdr_s
{
    unsigned short data_type;
    unsigned char marker_bit_0 : 1;
    unsigned char def_version : 7;
    unsigned short frame_num_high;
    unsigned char marker_bit_1 : 1;
    unsigned char reserved : 7;
    unsigned short frame_num_low;
} __attribute__((packed, aligned(1))) cmd_hdr_t;

typedef struct pos_hdr_s
{
    unsigned short data_type;
    unsigned char marker_bit_0 : 1;
    unsigned char def_version : 7;
    unsigned char reserved[5];
} __attribute__((packed, aligned(1))) pos_hdr_t;

typedef struct _VCA_PACK_
{
    IVS_SYS_PROC_PARAM stProcPrm;

    HIK_MULT_VCA_TARGET_LIST stTargetList;

    SVA_PROCESS_IN stIn;

    SVA_PROCESS_OUT stOut;
} VCA_PACK_ST;



/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

static VCA_PACK_ST gstVcaPack[VCA_PACK_MAX_CHAN_NUM];

/**
 * @function   vca_pack_init
 * @brief      初始化vca pack通道
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_pack_init(UINT32 u32Chan)
{
    VCA_PACK_ST *pstVcaVenc = NULL;

    if (u32Chan < VCA_PACK_MAX_CHAN_NUM)
    {
        pstVcaVenc = &gstVcaPack[u32Chan];
        if (pstVcaVenc == NULL)
        {
            VCA_LOGE("error\n");
            return SAL_FAIL;
        }

        pstVcaVenc->stProcPrm.buf_size = BUFFER_SIZE;

        pstVcaVenc->stProcPrm.buf = (unsigned char *)SAL_memMalloc(BUFFER_SIZE, "vca_venc", "pack");
        if (pstVcaVenc->stProcPrm.buf == NULL)
        {
            VCA_LOGE("error\n");
            return SAL_FAIL;
        }

        pstVcaVenc->stTargetList.type = SHOW_TYPE_LINEBG_ONE_HALF | MATCH_TYPE_ACCURACY;
        pstVcaVenc->stTargetList.target_num = TEST_TARGET_NUM;
        pstVcaVenc->stTargetList.p_target = (HIK_MULT_VCA_TARGET *)SAL_memMalloc(TEST_TARGET_NUM * sizeof(HIK_MULT_VCA_TARGET), "vca_venc", "target");

        if (pstVcaVenc->stTargetList.p_target == NULL)
        {
            SAL_memfree(pstVcaVenc->stProcPrm.buf, "vca_venc", "pack");
            VCA_LOGE("error\n");
            return SAL_FAIL;
        }
    }
    else
    {
        VCA_LOGE("error\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   vca_pack_makeAlgIvsInfoHdr
 * @brief      ivs同步信息封装
 * @param[in]  void *pstBuff 源数据
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_pack_makeAlgSyncCmd(void *pstBuff)
{
    HIK_COMMAND_INFO *pstSyncCmd = NULL;
    cmd_hdr_t *pstCmdHdr = NULL;

    pstCmdHdr = (cmd_hdr_t *)pstBuff;
    if (pstCmdHdr == NULL)
    {
        VCA_LOGE("error !\n");
        return SAL_FAIL;
    }

    pstCmdHdr->data_type = htons(0x1);
    pstCmdHdr->marker_bit_0 = 1;
    pstCmdHdr->def_version = 0;
    pstCmdHdr->frame_num_high = 0;
    pstCmdHdr->marker_bit_1 = 1;
    pstCmdHdr->reserved = 1;
    pstCmdHdr->frame_num_low = 0;

    pstSyncCmd = (HIK_COMMAND_INFO *)(pstBuff + sizeof(cmd_hdr_t));
    if (pstSyncCmd == NULL)
    {
        VCA_LOGE("error !\n");
        return SAL_FAIL;
    }

    pstSyncCmd->tag = 0x434d44;
    pstSyncCmd->version = 0x0;
    pstSyncCmd->flag = 0x1;
    pstSyncCmd->resvered = 0x1;

    return SAL_SOK;
}

/**
 * @function   vca_pack_makeAlgIvsInfoHdr
 * @brief      ivs信息封装
 * @param[in]  void *pstBuff 源数据
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_pack_makeAlgIvsInfoHdr(void *pstBuff)
{
    ivs_hdr_t *pstIvsHdr = NULL;

    pstIvsHdr = (ivs_hdr_t *)pstBuff;
    if (pstIvsHdr == NULL)
    {
        VCA_LOGE("error !\n");
        return SAL_FAIL;
    }

    pstIvsHdr->data_type  = htons(0x1);
    pstIvsHdr->marker_bit_0 = 1;
    pstIvsHdr->def_version = 0;
    pstIvsHdr->frame_num_high = 0;
    pstIvsHdr->marker_bit_1 = 1;
    pstIvsHdr->reserved = 1;
    pstIvsHdr->frame_num_low = 0;

    return SAL_SOK;
}

/**
 * @function   vca_pack_makeAlgPosInfoHdr
 * @brief      pos信息封装
 * @param[in]  void *pstBuff 源数据
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_pack_makeAlgPosInfoHdr(void *pstBuff)
{
    pos_hdr_t *pstPosHdr = NULL;

    pstPosHdr = (pos_hdr_t *)pstBuff;
    if (pstPosHdr == NULL)
    {
        VCA_LOGE("error !\n");
        return SAL_FAIL;
    }

    pstPosHdr->data_type = htons(HIK_PRIVT_POS_INFO);
    pstPosHdr->def_version = 0;
    pstPosHdr->marker_bit_0 = 1;
    pstPosHdr->reserved[0] = 0x1;
    pstPosHdr->reserved[1] = 0x1;
    pstPosHdr->reserved[2] = 0x1;
    pstPosHdr->reserved[3] = 0x1;
    pstPosHdr->reserved[4] = 0x1;

    return SAL_SOK;
}

/**
 * @function   vca_pack_process
 * @brief      vca pack
 * @param[in]  VCA_PACK_INFO_ST *pstVcaPackInfo 封装信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_pack_process(VCA_PACK_INFO_ST *pstVcaPackInfo)
{
    INT32 s32Ret = 0;
    UINT32 i = 0;
    UINT32 k = 0;
    UINT32 u32LengthInt = 0;
    UINT32 u32Dev = 0;
    UINT32 u32Chn = 0;
    UINT32 u32StreamId = 0;
    UINT32 u32IvsBufLen = 0;
    UINT32 u32SyncBufLen = 0;
    UINT32 u32AlertNum = 0;
    UINT32 u32Reliability = 0;
    UINT32 u32NameLen = 0;
    UINT64 u64SvaFramePts = 0;
    UINT64 u64VencFramePts = 0;
    UINT8 u8OsdBuff[SVA_ALERT_NAME_LEN+2] = {0};  /*违禁品OSD长度= 报警物标定名称长度 + 置信度字符2*/
    static UINT32 u32RIdx[MAX_VENC_CHAN][VENC_CHN_MAX_NUM] = {0};
    float fTargetOffset = 0.0;

    UINT8 u8SyncCmdBuf[1024] = {0};
    UINT8 u8IvsPrivtBuf[10000] = {0};

    MUX_INFO stMuxInfo = {0};
    MUX_PROC_INFO_S stMuxProcInfo = {0};
    REC_STREAM_INFO_ST stRecStreamInfo = {0};
    NET_STREAM_INFO_ST stNetStreamInfo = {0};

    VCA_PACK_ST *pstVcaVenc = NULL;
    SVA_PROCESS_IN *pstIn = NULL;
    SVA_PROCESS_OUT *pstOut = NULL;
    IVS_SYS_PROC_PARAM *pstProc_param = NULL;
    HIK_MULT_VCA_TARGET_LIST *pstTarget_list = NULL;
    MUX_IN_BITS_INTO_S *pstMuxInInfo = NULL;
    MUX_OUT_BITS_INTO_S *pstMuxOutInfo = NULL;
    DSPINITPARA *pstDspInitPara = NULL;
    IS_PRIVT_INFO_CONTRABAND *pstPrivtInfo = NULL;
    static UINT32 cnt = 0;
    ENCODING_FORMAT_E enEncFormat = ENC_FMT_GB2312;

    pstDspInitPara = SystemPrm_getDspInitPara();
    enEncFormat = pstDspInitPara->stFontLibInfo.enEncFormat;
    if ((enEncFormat >= ENC_FMT_MAX_CNT))
    {
        if (cnt++ % 10 == 0) /* 减少打印刷屏 */
        {
            VCA_LOGE("sva_name encoding format err! enEncFormat:%d\n", enEncFormat);
        }
        enEncFormat = ENC_FMT_GB2312; /* 违禁品编码类型错误，使用默认值 */
    }

    /* 入参有效性检验 */
    if (NULL == pstVcaPackInfo)
    {
        VCA_LOGE("pstVcaPackInfo == NULL \n");
        return SAL_FAIL;
    }

    u32Dev = pstVcaPackInfo->u32Dev;
    u32Chn = pstVcaPackInfo->u32Chn;
    u32StreamId = pstVcaPackInfo->u32StreamId;

    if (u32Chn >= VCA_PACK_MAX_CHAN_NUM)
    {
        VCA_LOGE("Invalid u32Chn %d >= MAX %d \n", u32Chn, VCA_PACK_MAX_CHAN_NUM);
        return SAL_FAIL;
    }

    pstVcaVenc = &gstVcaPack[u32Chn];
    if (NULL == pstVcaVenc)
    {
        VCA_LOGE("error\n");
        return SAL_FAIL;
    }

    pstTarget_list = &pstVcaVenc->stTargetList;
    if (NULL == pstTarget_list)
    {
        VCA_LOGE("error\n");
        return SAL_FAIL;
    }

    pstProc_param = &pstVcaVenc->stProcPrm;
    if (NULL == pstProc_param)
    {
        VCA_LOGE("error\n");
        return SAL_FAIL;
    }

    pstOut = &pstVcaVenc->stOut;
    if (NULL == pstOut)
    {
        VCA_LOGE("error\n");
        return SAL_FAIL;
    }

    pstIn = &pstVcaVenc->stIn;
    if (NULL == pstIn)
    {
        VCA_LOGE("error\n");
        return SAL_FAIL;
    }

    sal_memset_s(pstOut, sizeof(SVA_PROCESS_OUT), 0x00, sizeof(SVA_PROCESS_OUT));
    sal_memset_s(pstIn, sizeof(SVA_PROCESS_IN), 0x00, sizeof(SVA_PROCESS_IN));

    if (SAL_SOK != Sva_DrvGetCfgParam(u32Dev, pstIn))
    {
        VCA_LOGE("Get Cfg Param Failed! chan %d \n", u32Dev);
        return SAL_FAIL;
    }

    if ((pstDspInitPara->boardType >= SECURITY_MACHINE_START) && (pstDspInitPara->boardType <= SECURITY_MACHINE_END))
    {
#ifdef DSP_ISM
        Xpack_GetSvaResultForPos(u32Dev, pstOut);
#endif
    }
    else
    {

        u64VencFramePts = pstVcaPackInfo->u64Pts;
        s32Ret = Sva_HalGetFromPoolWithPts(u32Dev, &u32RIdx[u32Dev][u32StreamId], u64VencFramePts, pstOut);
        if (SAL_SOK != s32Ret)
        {
            s32Ret = Sva_DrvGetSvaOut(u32Dev, pstOut);
            if (SAL_SOK != s32Ret)
            {
                VCA_LOGE("Get Sva Result Failed! u32Dev %d u32Chn %d \n", u32Dev, u32Chn);
                return SAL_FAIL;
            }
        }

        /* Uint: ms */
        u64SvaFramePts = pstOut->frame_stamp / 1000;

        /*前面PTS如果没有匹配上，对坐标进行偏移，改善目标框抖动的问题 */
        if ((u64VencFramePts != u64SvaFramePts) && (u64SvaFramePts > 0))
        {
            fTargetOffset = (((INT64L)u64VencFramePts - (INT64L)u64SvaFramePts) * pstOut->frame_offset / (1000.0 / ((float)Sva_HalGetViSrcRate() / Sva_DrvGetInputGapNum())));

            for (i = 0; i < pstOut->target_num; i++)
            {
                /* 根据过包方向对偏移坐标进行处理，0:R2L，1:L2R */
                if (0 == pstIn->enDirection)
                {
                    if (pstOut->target[i].rect.x > fTargetOffset)
                    {
                        pstOut->target[i].rect.x -= fTargetOffset;
                    }
                    else
                    {
                        pstOut->target[i].rect.x = 0.0;
                        if (pstOut->target[i].rect.width - fTargetOffset > 0.0)
                        {
                            pstOut->target[i].rect.width -= fTargetOffset;
                        }
                    }
                }
                else
                {
                    if (pstOut->target[i].rect.x + fTargetOffset > 1.0)
                    {
                        pstOut->target[i].rect.x = 1.0;
                    }
                    else
                    {
                        pstOut->target[i].rect.x += fTargetOffset;
                        if (pstOut->target[i].rect.x + pstOut->target[i].rect.width + fTargetOffset >= 1.0)
                        {
                            pstOut->target[i].rect.width = 1.0 - pstOut->target[i].rect.x - fTargetOffset;
                        }
                    }
                }
            }
        }
    }

    u32AlertNum = (pstOut->target_num > TEST_TARGET_NUM) ? (TEST_TARGET_NUM) : pstOut->target_num;

    for (i = 0; i < u32AlertNum; i++)
    {
		if (SAL_TRUE == pstOut->target[i].bAnotherViewTar)
        {
            continue;
        }
        /*IVS 信息*/
        pstTarget_list->p_target[k].target.type = 0;
        pstTarget_list->p_target[k].target.alarm_flg = pstOut->target[i].alarm_flg;             /* 算法给出结果中一致 */
        pstTarget_list->p_target[k].target.ID = pstOut->target[i].ID;                           /* 算法给出结果中一致 */
        pstTarget_list->p_target[k].target.rect.x = pstOut->target[i].rect.x;                   /* 0.0-1.0之间 */
        pstTarget_list->p_target[k].target.rect.y = pstOut->target[i].rect.y;                   /* 0.0-1.0之间 */
        pstTarget_list->p_target[k].target.rect.width = pstOut->target[i].rect.width;           /* 0.0-1.0之间 */
        pstTarget_list->p_target[k].target.rect.height = pstOut->target[i].rect.height;         /* 0.0-1.0之间 */

        if (sizeof(IS_PRIVT_INFO_CONTRABAND) <= IS_MAX_PRIVT_LEN)
        {
            pstTarget_list->p_target[k].privt_info.privt_type = IS_PRIVT_COLOR_CONTRABAND;
            pstTarget_list->p_target[k].privt_info.privt_len = sizeof(IS_PRIVT_INFO_CONTRABAND);

            pstPrivtInfo = (IS_PRIVT_INFO_CONTRABAND *)pstTarget_list->p_target[k].privt_info.privt_data;
            pstPrivtInfo->type = pstOut->target[i].type;
            pstPrivtInfo->type |= ((unsigned int)enEncFormat) << 24; /* 使用高8bit传输违禁品编码类型 */
            pstPrivtInfo->color.alpha = 255;
            pstPrivtInfo->color.red = (pstOut->target[i].color >> 16) & 0xFF;
            pstPrivtInfo->color.green = (pstOut->target[i].color >> 8) & 0xFF;
            pstPrivtInfo->color.blue = (pstOut->target[i].color & 0xFF);

            if (enEncFormat == ENC_FMT_ISO_8859_6) /* web端阿拉伯语乱码，不送违禁品名称 */
            {
                pstPrivtInfo->confidence = 0; /* 置信度 */
                pstPrivtInfo->pos_len = 0;
                memset(pstPrivtInfo->pos_data, 0, sizeof(pstPrivtInfo->pos_data));
            }
            else
            {
                memset(u8OsdBuff, 0, sizeof(u8OsdBuff));
                if (pstIn->stTargetPrm.enOsdExtType)
                {
                    pstPrivtInfo->confidence = pstOut->target[i].visual_confidence; /* 置信度 */
                    u32Reliability = pstPrivtInfo->confidence;
                    s32Ret = snprintf((char *)u8OsdBuff, SVA_ALERT_NAME_LEN + 2, "%s(%d%s)", pstOut->target[i].sva_name, u32Reliability, "%");
                }
                else
                {
                    pstPrivtInfo->confidence = 0; /* 置信度 */
                    s32Ret = snprintf((char *)u8OsdBuff, SVA_ALERT_NAME_LEN + 2, "%s", pstOut->target[i].sva_name);
                }

                u32NameLen = strlen((char *)u8OsdBuff);
                if (u32NameLen < sizeof(pstPrivtInfo->pos_data))
                {
                    pstPrivtInfo->pos_len = u32NameLen;
                    strncpy((char *)pstPrivtInfo->pos_data, (const char *)u8OsdBuff, sizeof(pstPrivtInfo->pos_data) - 1);
                    pstPrivtInfo->pos_data[(sizeof(pstPrivtInfo->pos_data) - 1)] = '\0';
                }
                else
                {
                    pstPrivtInfo->pos_len = 0;
                    VCA_LOGW("sva info size[%u] is bigger than buff\n", u32NameLen);
                }
            }
        }
        else
        {
            VCA_LOGW("privt info size is bigger than buff\n");
        }
		k++;
    }

    pstProc_param->scale_width = pstVcaPackInfo->u32Width;
    pstProc_param->scale_height = pstVcaPackInfo->u32Height;

    /* IVS 信息 */
    pstTarget_list->type = SHOW_TYPE_NORMAL | MATCH_TYPE_ACCURACY;
    pstTarget_list->target_num = k;

    s32Ret = IVS_MULT_META_DATA_to_system(pstTarget_list, pstProc_param);
    if (s32Ret != HIK_IVS_SYS_LIB_S_OK)
    {
        VCA_LOGE("Error to mux meta data, error coed = %x\n", s32Ret);
        return SAL_FAIL;
    }

    /* IVS 私有信息填充 */
    u32IvsBufLen = 0;
    /* 0 - 1 */
    u8IvsPrivtBuf[u32IvsBufLen++] = (unsigned char)(HIK_PRIVT_IVS_INFO >> 8);
    u8IvsPrivtBuf[u32IvsBufLen++] = (unsigned char)(HIK_PRIVT_IVS_INFO);
    /* 以4字节为单位，表示私有数据长度 */
    u32LengthInt = (pstProc_param->len + sizeof(ivs_hdr_t)) >> 2;
    /* 2 - 3 */
    u8IvsPrivtBuf[u32IvsBufLen++] = (u32LengthInt >> 8) & 0xff;
    u8IvsPrivtBuf[u32IvsBufLen++] = (u32LengthInt & 0xff);

    s32Ret = vca_pack_makeAlgIvsInfoHdr(u8IvsPrivtBuf + u32IvsBufLen);
    if (s32Ret != SAL_SOK)
    {
        VCA_LOGE("vca_pack_makeAlgIvsInfoHdr error !\n");
        return SAL_FAIL;
    }

    u32IvsBufLen += sizeof(ivs_hdr_t);

    memcpy(u8IvsPrivtBuf + u32IvsBufLen, pstProc_param->buf, pstProc_param->len);
    u32IvsBufLen += pstProc_param->len;

    /* 打包相关 */
    memset(&stMuxProcInfo, 0, sizeof(MUX_PROC_INFO_S));
    pstMuxInInfo = &stMuxProcInfo.muxData.stInBuffer;
    pstMuxOutInfo = &stMuxProcInfo.muxData.stOutBuffer;
    pstMuxInInfo->bVideo = SAL_FALSE;
    pstMuxInInfo->naluNum = 1;
    pstMuxInInfo->frame_type = FRAME_TYPE_PRIVT_FRAME;
    pstMuxInInfo->u64PTS = pstVcaPackInfo->u64Pts;
    pstMuxInInfo->is_key_frame = 0;

    u32SyncBufLen = 0;

    u8SyncCmdBuf[u32SyncBufLen++] = (unsigned char)(0x1006 >> 8);
    u8SyncCmdBuf[u32SyncBufLen++] = (unsigned char)(0x1006);
    /* 以4字节为单位，表示私有数据长度 */
    u32LengthInt = (sizeof(HIK_COMMAND_INFO) + sizeof(ivs_hdr_t)) >> 2;
    /* 2 - 3 */
    u8SyncCmdBuf[u32SyncBufLen++] = (u32LengthInt >> 8) & 0xff;
    u8SyncCmdBuf[u32SyncBufLen++] = (u32LengthInt & 0xff);

    s32Ret = vca_pack_makeAlgSyncCmd(u8SyncCmdBuf + u32SyncBufLen);
    if (s32Ret != SAL_SOK)
    {
        VCA_LOGE("vca_pack_makeAlgSyncCmd error !\n");
        return SAL_FAIL;
    }

    u32SyncBufLen += sizeof(ivs_hdr_t);

    u32SyncBufLen += sizeof(HIK_COMMAND_INFO);

    /*rtp 打包*/
    if (SAL_TRUE == pstVcaPackInfo->isUseRtp)
    {
        stMuxProcInfo.muxHdl = pstVcaPackInfo->u32RtpHandle;           /* Rtp 打包创建的通道 */

        /**********************************************************************************************************/
        pstMuxInInfo->bufferAddr[0] = u8SyncCmdBuf;
        pstMuxInInfo->bufferLen[0] = u32SyncBufLen;

        pstMuxOutInfo->streamLen = 0;
        pstMuxOutInfo->bufferAddr = pstVcaPackInfo->pPackOutputBuf;
        pstMuxOutInfo->bufferLen = pstVcaPackInfo->u32PackBufSize;

        s32Ret = MuxControl(MUX_PRCESS, &stMuxProcInfo, NULL);
        if (s32Ret != SAL_SOK)
        {
            VCA_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, s32Ret);
            return SAL_FAIL;
        }

        /* 写入共享内存 */
        memset(&stNetStreamInfo, 0, sizeof(NET_STREAM_INFO_ST));

        stNetStreamInfo.pucAddr = pstMuxOutInfo->bufferAddr;
        stNetStreamInfo.uiSize = pstMuxOutInfo->streamLen;
        stNetStreamInfo.uiType = 1;

        s32Ret = SystemPrm_writeToNetPool(u32Dev, u32StreamId, &stNetStreamInfo);
        if (s32Ret != SAL_SOK)
        {
            VCA_LOGE("SystemPrm_writeToNetPool err\n");
            return SAL_FAIL;
        }

        /**********************************************************************************************************/

        /**********************************************************************************************************/
        pstMuxInInfo->bufferAddr[0] = u8IvsPrivtBuf;
        pstMuxInInfo->bufferLen[0] = u32IvsBufLen;

        pstMuxOutInfo->streamLen = 0;
        pstMuxOutInfo->bufferAddr = pstVcaPackInfo->pPackOutputBuf;
        pstMuxOutInfo->bufferLen = pstVcaPackInfo->u32PackBufSize;

        s32Ret = MuxControl(MUX_PRCESS, &stMuxProcInfo, NULL);
        if (s32Ret != SAL_SOK)
        {
            VCA_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, s32Ret);

            return SAL_FAIL;
        }

        /* 写入共享内存 */
        memset(&stNetStreamInfo, 0, sizeof(NET_STREAM_INFO_ST));

        stNetStreamInfo.pucAddr = pstMuxOutInfo->bufferAddr;
        stNetStreamInfo.uiSize = pstMuxOutInfo->streamLen;
        stNetStreamInfo.uiType = 1;

        s32Ret = SystemPrm_writeToNetPool(u32Dev, u32StreamId, &stNetStreamInfo);
        if (s32Ret != SAL_SOK)
        {
            VCA_LOGE("SystemPrm_writeToNetPool err\n");
            return SAL_FAIL;
        }

        /**********************************************************************************************************/
    }

    /* Ps 打包 */
    if (SAL_TRUE == pstVcaPackInfo->isUsePs)
    {
        stMuxProcInfo.muxHdl = pstVcaPackInfo->u32PsHandle; /* Ps 打包创建的通道 */;

        /**********************************************************************************************************/
        /* SYNC 信息 */
        pstMuxInInfo->bufferAddr[0] = u8SyncCmdBuf;
        pstMuxInInfo->bufferLen[0] = u32SyncBufLen;

        pstMuxOutInfo->streamLen = 0;
        pstMuxOutInfo->bufferAddr = pstVcaPackInfo->pPackOutputBuf;
        pstMuxOutInfo->bufferLen = pstVcaPackInfo->u32PackBufSize;

        s32Ret = MuxControl(MUX_PRCESS, &stMuxProcInfo, NULL);
        if (s32Ret != SAL_SOK)
        {
            VCA_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, s32Ret);
            return SAL_FAIL;
        }

        s32Ret = MuxControl(MUX_GET_INFO, &pstVcaPackInfo->u32PsHandle, &stMuxInfo);
        if (s32Ret != SAL_SOK)
        {
            VCA_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, s32Ret);
            return SAL_FAIL;
        }

        /* 写入录像缓存 */
        memset(&stRecStreamInfo, 0, sizeof(REC_STREAM_INFO_ST));
        stRecStreamInfo.pucAddr = pstMuxOutInfo->bufferAddr;
        stRecStreamInfo.uiSize = pstMuxOutInfo->streamLen;
        stRecStreamInfo.uiType = 1;
        stRecStreamInfo.streamType = FRAME_TYPE_PRIVT_FRAME;
        stRecStreamInfo.IFrameInfo = (1 | (pstMuxOutInfo->streamLen << 1));
        SAL_getDateTime(&stRecStreamInfo.absTime);
        stRecStreamInfo.stdTime = stMuxInfo.time_stamp;
        stRecStreamInfo.pts = pstMuxInInfo->u64PTS;
        s32Ret = SystemPrm_writeRecPool(u32Dev, u32StreamId, &stRecStreamInfo);
        if (s32Ret != SAL_SOK)
        {
            VCA_LOGE("SystemPrm_writeRecPool err\n");
            return SAL_FAIL;
        }

        /**********************************************************************************************************/

        /**********************************************************************************************************/
        /* IVS 信息 */
        pstMuxInInfo->bufferAddr[0] = u8IvsPrivtBuf;
        pstMuxInInfo->bufferLen[0] = u32IvsBufLen;

        pstMuxOutInfo->streamLen = 0;
        pstMuxOutInfo->bufferAddr = pstVcaPackInfo->pPackOutputBuf;
        pstMuxOutInfo->bufferLen = pstVcaPackInfo->u32PackBufSize;

        s32Ret = MuxControl(MUX_PRCESS, &stMuxProcInfo, NULL);
        if (s32Ret != SAL_SOK)
        {
            VCA_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, s32Ret);
            return SAL_FAIL;
        }

        s32Ret = MuxControl(MUX_GET_INFO, &pstVcaPackInfo->u32PsHandle, &stMuxInfo);
        if (s32Ret != SAL_SOK)
        {
            VCA_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, s32Ret);
            return SAL_FAIL;
        }

        /* 写入录像缓存 */
        memset(&stRecStreamInfo, 0, sizeof(REC_STREAM_INFO_ST));
        stRecStreamInfo.pucAddr = pstMuxOutInfo->bufferAddr;
        stRecStreamInfo.uiSize = pstMuxOutInfo->streamLen;
        stRecStreamInfo.uiType = 1;
        stRecStreamInfo.streamType = FRAME_TYPE_PRIVT_FRAME;
        stRecStreamInfo.IFrameInfo = (1 | (pstMuxOutInfo->streamLen << 1));
        SAL_getDateTime(&stRecStreamInfo.absTime);
        stRecStreamInfo.stdTime = stMuxInfo.time_stamp;
        stRecStreamInfo.pts = pstMuxInInfo->u64PTS;
        s32Ret = SystemPrm_writeRecPool(u32Dev, u32StreamId, &stRecStreamInfo);
        if (s32Ret != SAL_SOK)
        {
            VCA_LOGE("SystemPrm_writeRecPool err\n");
            return SAL_FAIL;
        }

        /**********************************************************************************************************/
    }

    return SAL_SOK;
}

